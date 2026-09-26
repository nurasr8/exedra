// Binary adapter. Faithful port of the Python implementation.
#include "adapter.h"

#include <filesystem>
#include <fstream>
#include <regex>
#include <sstream>
#include <sys/utsname.h>

#include "util.h"

namespace fs = std::filesystem;

namespace adapter {
namespace {

const std::vector<std::string> BAD_LINK_TARGETS = {"/tmp/", "/venim/build/",
                                                   "/venim/cache/"};

bool startsWithAny(const std::string& s, const std::vector<std::string>& prefs) {
    for (const auto& p : prefs)
        if (s.compare(0, p.size(), p) == 0) return true;
    return false;
}

bool fixScript(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return false;
    char head[512];
    f.read(head, sizeof head);
    std::streamsize got = f.gcount();
    if (got < 2 || head[0] != '#' || head[1] != '!') return false;
    std::string data(head, (size_t)got);
    // need full file for rewrite; read it all
    std::ifstream all(path, std::ios::binary);
    std::ostringstream ss;
    ss << all.rdbuf();
    std::string full = ss.str();
    size_t nl = full.find('\n');
    if (nl == std::string::npos) return false;
    std::string line = full.substr(0, nl);
    // strip trailing \r? Python compares text.strip() — mirror strip
    std::string t = util::trim(line);
    std::string replacement;
    if (t == "#!/usr/bin/python3" || t == "#!/usr/bin/python")
        replacement = "#!/usr/bin/env python3\n";
    else if (t == "#!/bin/bash" && !fs::exists("/bin/bash"))
        replacement = "#!/usr/bin/env bash\n";
    else
        return false;
    std::ofstream o(path, std::ios::binary | std::ios::trunc);
    o << replacement << full.substr(nl + 1);
    return true;
}

bool fixDesktop(const std::string& path) {
    std::ifstream f(path);
    if (!f) return false;
    std::ostringstream ss;
    ss << f.rdbuf();
    std::string data = ss.str();
    std::string updated = std::regex_replace(
        data, std::regex("^Exec=/usr/bin/(\\S+)",
                         std::regex::ECMAScript | std::regex::multiline),
        "Exec=$1");
    updated = std::regex_replace(
        updated, std::regex("^Exec=/usr/sbin/(\\S+)",
                            std::regex::ECMAScript | std::regex::multiline),
        "Exec=$1");
    if (updated != data) {
        std::ofstream o(path, std::ios::trunc);
        o << updated;
        return true;
    }
    return false;
}

bool isElf(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    char magic[4] = {0, 0, 0, 0};
    f.read(magic, 4);
    return f.gcount() == 4 && magic[0] == '\x7f' && magic[1] == 'E' &&
           magic[2] == 'L' && magic[3] == 'F';
}

bool fixElfRpath(const std::string& path, const std::string& pkgdir) {
    util::ProcResult r = util::runCapture("patchelf --print-rpath " +
                                          util::shellQuote(path));
    if (r.rc != 0) return false;
    std::string old = util::trim(r.out);
    if (old.find("/tmp") == std::string::npos &&
        old.find("/build") == std::string::npos &&
        old.find("/home/") == std::string::npos)
        return false;
    std::string libdir = pkgdir + "/lib";
    std::string nw = fs::is_directory(libdir) ? libdir : "$ORIGIN/../lib:$ORIGIN";
    util::ProcResult w = util::runCapture("patchelf --set-rpath " +
                                          util::shellQuote(nw) + " " +
                                          util::shellQuote(path));
    return w.rc == 0;
}

}  // namespace

void adaptTree(const std::string& staging) {
    std::error_code ec;
    for (auto it = fs::recursive_directory_iterator(
             staging, fs::directory_options::skip_permission_denied, ec);
         it != fs::recursive_directory_iterator(); ++it) {
        auto st = it->symlink_status(ec);
        if (ec) continue;
        std::string full = it->path().string();
        if (fs::is_symlink(st)) continue;
        if (fs::is_regular_file(st)) {
            fixScript(full);
            if (it->path().extension() == ".desktop") fixDesktop(full);
            continue;
        }
        if (fs::is_directory(st)) {
            // mirror os.walk(dirnames): only symlinks-to-directories are checked
            for (auto& e : fs::directory_iterator(it->path(), ec)) {
                auto es = e.symlink_status(ec);
                if (ec || !fs::is_symlink(es)) continue;
                auto ts = e.status(ec);
                if (ec || !fs::is_directory(ts)) continue;
                std::string target = fs::read_symlink(e.path(), ec).string();
                if (ec) continue;
                if (startsWithAny(target, BAD_LINK_TARGETS) ||
                    target.find("/build/") != std::string::npos)
                    throw std::runtime_error("unsafe symlink " + e.path().string() +
                                             " -> " + target);
            }
        }
    }
}

void validateTree(const std::string& staging, const std::string& pkgdir,
                  std::vector<std::string>& errors) {
    struct utsname un;
    std::string machine;
    if (uname(&un) == 0) machine = un.machine;
    std::error_code ec;
    for (auto it = fs::recursive_directory_iterator(
             staging, fs::directory_options::skip_permission_denied, ec);
         it != fs::recursive_directory_iterator(); ++it) {
        std::string full = it->path().string();
        auto st = it->symlink_status(ec);
        if (ec) continue;
        if (fs::is_symlink(st)) {
            std::string target = fs::read_symlink(it->path(), ec).string();
            if (!ec && startsWithAny(target, BAD_LINK_TARGETS))
                errors.push_back(full + ": unsafe symlink -> " + target);
            continue;
        }
        if (!fs::is_regular_file(st)) continue;
        if (isElf(full)) {
            util::ProcResult r = util::runCapture("readelf -h " + util::shellQuote(full));
            if (r.rc != 0) {
                errors.push_back(full + ": unreadable ELF");
                continue;
            }
            if (machine == "x86_64" && r.out.find("X86-64") == std::string::npos)
                errors.push_back(full + ": wrong arch (need x86_64)");
            util::runCapture("readelf -d " + util::shellQuote(full));
            fixElfRpath(full, pkgdir);
        }
    }
}

void unpack(const std::string& archive, const std::string& dest) {
    std::error_code ec;
    fs::create_directories(dest, ec);
    int rc;
    if (archive.size() >= 4 && archive.compare(archive.size() - 4, 4, ".zip") == 0) {
        rc = util::runQuiet("unzip -q " + util::shellQuote(archive) + " -d " +
                            util::shellQuote(dest));
    } else if ((archive.size() >= 4 &&
                archive.compare(archive.size() - 4, 4, ".zst") == 0) ||
               (archive.size() >= 5 &&
                archive.compare(archive.size() - 5, 5, ".zstd") == 0)) {
        rc = util::runQuiet("tar --use-compress-program=unzstd -xf " +
                            util::shellQuote(archive) + " -C " + util::shellQuote(dest));
    } else {
        rc = util::runQuiet("tar -xf " + util::shellQuote(archive) + " -C " +
                            util::shellQuote(dest));
    }
    if (rc != 0) throw std::runtime_error("unpack failed: " + archive);
}

}  // namespace adapter
