// Core package operations. Faithful port of the Python implementation.
#include "core.h"

#include <curl/curl.h>

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <openssl/sha.h>
#include <sstream>
#include <stdexcept>
#include <sys/wait.h>
#include <unistd.h>

#include "adapter.h"
#include "db.h"
#include "util.h"

namespace fs = std::filesystem;

namespace core {
namespace {

std::map<std::string, std::string> baseEnv(const vnb::Recipe& rec,
                                           const std::string& srcdir,
                                           const std::string& builddir,
                                           const std::string& destdir) {
    return {
        {"name", rec.name},
        {"version", rec.getStr("version", "0")},
        {"srcdir", srcdir},
        {"builddir", builddir},
        {"destdir", destdir},
        {"DESTDIR", destdir},
        {"prefix", "/usr"},
    };
}

const std::map<std::string, std::string>& dirMapping() {
    static const std::map<std::string, std::string> m = {
        {"bin", "usr/bin"},
        {"sbin", "usr/sbin"},
        {"lib", "usr/lib"},
        {"lib64", "usr/lib64"},
        {"share", "usr/share"},
    };
    return m;
}

std::string mapTop(const std::string& top, const std::string& rest) {
    const auto& m = dirMapping();
    auto it = m.find(top);
    std::string base = (it == m.end()) ? top : it->second;
    if (rest.empty()) return base;
    return base + "/" + rest;
}

// fork + chdir + merged env + exec /bin/sh -c
int runShell(const std::string& cmd, const std::string& cwd,
             const std::map<std::string, std::string>& extra) {
    pid_t pid = fork();
    if (pid < 0) throw std::runtime_error("fork failed");
    if (pid == 0) {
        if (!cwd.empty() && chdir(cwd.c_str()) != 0) _exit(127);
        for (const auto& [k, v] : extra) setenv(k.c_str(), v.c_str(), 1);
        execl("/bin/sh", "sh", "-c", cmd.c_str(), (char*)nullptr);
        _exit(127);
    }
    int st = 0;
    while (waitpid(pid, &st, 0) < 0) {
    }
    if (WIFEXITED(st)) return WEXITSTATUS(st);
    return -1;
}

int runExec(const std::vector<std::string>& args) {
    pid_t pid = fork();
    if (pid < 0) throw std::runtime_error("fork failed");
    if (pid == 0) {
        std::vector<char*> av;
        for (const auto& a : args) av.push_back(const_cast<char*>(a.c_str()));
        av.push_back(nullptr);
        execvp(av[0], av.data());
        _exit(127);
    }
    int st = 0;
    while (waitpid(pid, &st, 0) < 0) {
    }
    if (WIFEXITED(st)) return WEXITSTATUS(st);
    return -1;
}

size_t curlWrite(char* p, size_t s, size_t n, void* f) {
    return fwrite(p, s, n, (FILE*)f);
}

void copyEntry(const std::string& s, const std::string& d);

void toExedraLayout(const std::string& src, const std::string& dest) {
    std::error_code ec;
    fs::create_directories(dest, ec);
    bool hasUsr = fs::is_directory(fs::path(src) / "usr", ec);
    bool hasBin = fs::exists(fs::path(src) / "bin", ec);
    if (hasUsr && !hasBin) {
        for (auto& e : fs::directory_iterator(src, ec)) {
            std::string name = e.path().filename().string();
            std::string sp = e.path().string();
            if (name == "usr") {
                for (auto& u : fs::directory_iterator(sp, ec))
                    copyEntry(u.path().string(),
                              (fs::path(dest) / u.path().filename()).string());
            } else {
                copyEntry(sp, (fs::path(dest) / name).string());
            }
        }
        return;
    }
    for (auto& e : fs::directory_iterator(src, ec))
        copyEntry(e.path().string(),
                  (fs::path(dest) / e.path().filename()).string());
}

void copyEntry(const std::string& s, const std::string& d) {
    std::error_code ec;
    auto st = fs::symlink_status(s, ec);
    if (ec) return;
    if (fs::is_directory(st) && !fs::is_symlink(st)) {
        if (fs::exists(d, ec)) util::removeAll(d);
        util::copyTree(s, d);
    } else if (fs::is_symlink(st)) {
        if (fs::exists(d, ec) || fs::is_symlink(fs::symlink_status(d, ec))) {
            ec.clear();
            fs::remove(d, ec);
        }
        fs::create_symlink(fs::read_symlink(s, ec), d, ec);
    } else {
        util::copyFile2(s, d);
    }
}

std::string stripTopLevel(const std::string& stage) {
    std::vector<std::string> entries;
    std::error_code ec;
    for (auto& e : fs::directory_iterator(stage, ec)) {
        std::string n = e.path().filename().string();
        if (!n.empty() && n[0] != '.') entries.push_back(n);
    }
    if (entries.size() == 1) {
        std::string top = (fs::path(stage) / entries[0]).string();
        if (fs::is_directory(fs::path(top), ec)) {
            for (const char* d : {"bin", "usr", "lib", "share"}) {
                if (fs::is_directory(fs::path(top) / d, ec)) return top;
            }
        }
    }
    return stage;
}

}  // namespace

std::string sha256File(const std::string& path) {
    FILE* f = fopen(path.c_str(), "rb");
    if (!f) throw std::runtime_error("cannot open for sha256: " + path);
    SHA256_CTX ctx;
    SHA256_Init(&ctx);
    char buf[1 << 20];
    size_t n;
    while ((n = fread(buf, 1, sizeof buf, f)) > 0) SHA256_Update(&ctx, buf, n);
    fclose(f);
    unsigned char out[SHA256_DIGEST_LENGTH];
    SHA256_Final(out, &ctx);
    char hex[65];
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) sprintf(hex + 2 * i, "%02x", out[i]);
    return std::string(hex, 64);
}

void fetch(const std::string& url, const std::string& dest) {
    std::error_code ec;
    fs::path dp(dest);
    if (!dp.parent_path().empty()) fs::create_directories(dp.parent_path(), ec);
    if (fs::exists(dest, ec)) return;
    if (url.compare(0, 7, "file://") == 0) {
        // mirror urlretrieve: copy local files
        std::string src = url.substr(7);
        if (!util::copyFile2(src, dest))
            throw std::runtime_error("fetch copy failed: " + url);
        return;
    }
    CURL* c = curl_easy_init();
    if (!c) throw std::runtime_error("curl init failed");
    FILE* f = fopen(dest.c_str(), "wb");
    if (!f) {
        curl_easy_cleanup(c);
        throw std::runtime_error("cannot write: " + dest);
    }
    curl_easy_setopt(c, CURLOPT_URL, url.c_str());
    curl_easy_setopt(c, CURLOPT_WRITEFUNCTION, curlWrite);
    curl_easy_setopt(c, CURLOPT_WRITEDATA, f);
    curl_easy_setopt(c, CURLOPT_FOLLOWLOCATION, 1L);
    CURLcode rc = curl_easy_perform(c);
    long code = 0;
    curl_easy_getinfo(c, CURLINFO_RESPONSE_CODE, &code);
    curl_easy_cleanup(c);
    fclose(f);
    if (rc != CURLE_OK) {
        fs::remove(dest, ec);
        throw std::runtime_error("fetch failed: " + url);
    }
}

void runCmds(const std::vector<std::string>& cmds, const std::string& cwd,
             const std::map<std::string, std::string>& env) {
    for (const auto& c : cmds) {
        std::string expanded = vnb::expandVars(c, env);
        int rc = runShell(expanded, cwd, env);
        if (rc != 0) throw std::runtime_error("command failed: " + expanded);
    }
}

std::vector<std::string> listFiles(const std::string& tree) {
    std::vector<std::string> out;
    std::error_code ec;
    for (auto it = fs::recursive_directory_iterator(
             tree, fs::directory_options::skip_permission_denied, ec);
         it != fs::recursive_directory_iterator(); ++it) {
        auto st = it->symlink_status(ec);
        if (ec) continue;
        bool take = fs::is_regular_file(st);
        if (!take && fs::is_symlink(st)) {
            // mirror os.walk(filenames): symlinks except symlinks-to-dirs
            auto ts = it->status(ec);
            take = ec || !fs::is_directory(ts);
            ec.clear();
        }
        if (!take) continue;
        out.push_back(util::relPath(tree, it->path().string()));
    }
    std::sort(out.begin(), out.end());
    return out;
}

std::vector<std::string> linkPackage(const std::string& root,
                                     const std::string& name,
                                     const std::string& version) {
    std::string pkgdir = root + "/venim/packages/" + name + "/" + version;
    std::vector<std::string> linked;
    std::error_code ec;
    auto files = listFiles(pkgdir);
    for (const auto& rel : files) {
        std::string src = (fs::path(pkgdir) / rel).string();
        size_t slash = rel.find('/');
        std::string top = (slash == std::string::npos) ? rel : rel.substr(0, slash);
        std::string rest = (slash == std::string::npos) ? "" : rel.substr(slash + 1);
        std::string targetRel = mapTop(top, rest);
        std::string dst = (fs::path(root) / targetRel).string();
        fs::create_directories(fs::path(dst).parent_path(), ec);
        auto dstSt = fs::symlink_status(dst, ec);
        if (!ec && (fs::is_symlink(dstSt) || fs::exists(dstSt))) {
            ec.clear();
            fs::remove(dst, ec);
        }
        std::string relSrc =
            util::relPath(fs::path(dst).parent_path().string(), src);
        fs::create_symlink(relSrc, dst, ec);
        linked.push_back(targetRel);
    }
    return linked;
}

std::vector<std::string> unlinkPackage(const std::string& root,
                                       const std::string& name,
                                       const std::string& version) {
    std::string pkgdir = root + "/venim/packages/" + name + "/" + version;
    std::vector<std::string> removed;
    std::error_code ec;
    for (const auto& rel : listFiles(pkgdir)) {
        std::string src = fs::absolute(fs::path(pkgdir) / rel, ec).string();
        if (ec) {
            ec.clear();
            continue;
        }
        size_t slash = rel.find('/');
        std::string top = (slash == std::string::npos) ? rel : rel.substr(0, slash);
        std::string rest = (slash == std::string::npos) ? "" : rel.substr(slash + 1);
        std::string dst = (fs::path(root) / mapTop(top, rest)).string();
        auto dstSt = fs::symlink_status(dst, ec);
        if (!ec && fs::is_symlink(dstSt)) {
            std::string real = fs::weakly_canonical(fs::absolute(dst, ec), ec).string();
            if (!ec && real == src) {
                fs::remove(dst, ec);
                removed.push_back(mapTop(top, rest));
            }
        }
        ec.clear();
    }
    util::removeAll(pkgdir);
    std::string parent = (fs::path(pkgdir).parent_path()).string();
    if (fs::is_directory(parent, ec) && fs::is_empty(parent, ec)) fs::remove(parent, ec);
    return removed;
}

std::string buildSource(const std::string& root, const vnb::Recipe& rec,
                        bool verbose, bool noCheck) {
    std::string name = rec.name, ver = rec.getStr("version", "0");
    std::string work = root + "/venim/build/" + name + "-" + ver;
    std::string srcdir = work + "/src", builddir = work + "/build",
                destdir = work + "/dest";
    std::error_code ec;
    fs::create_directories(srcdir, ec);
    fs::create_directories(builddir, ec);
    fs::create_directories(destdir, ec);
    auto env = baseEnv(rec, srcdir, builddir, destdir);

    const vnb::Value* src = rec.get("source");
    bool haveUrl = src && src->isMap() &&
                   src->map.count("url") && src->map.at("url").isStr() &&
                   !src->map.at("url").s.empty();
    if (haveUrl) {
        std::string url = src->map.at("url").s;
        std::string arc = root + "/venim/sources/" +
                          fs::path(url).filename().string();
        fetch(url, arc);
        auto it = src->map.find("sha256");
        if (!noCheck && it != src->map.end() && it->second.isStr() &&
            it->second.s.compare(0, 8, "00000000") != 0) {
            std::string got = sha256File(arc);
            if (got != it->second.s)
                throw std::runtime_error("sha256 mismatch for " + name + ": want " +
                                         it->second.s + ", got " + got);
        }
        adapter::unpack(arc, srcdir);
    } else if (name == "hello-venim") {
        std::ofstream f(srcdir + "/hello.c", std::ios::trunc);
        f << "#include <stdio.h>\nint main(void){puts(\"Hello from Exedra!\");return 0;}\n";
    }

    std::vector<std::string> entries;
    for (auto& e : fs::directory_iterator(srcdir, ec))
        entries.push_back(e.path().filename().string());
    std::string topsrc = srcdir;
    if (entries.size() == 1 &&
        fs::is_directory(fs::path(srcdir) / entries[0], ec))
        topsrc = (fs::path(srcdir) / entries[0]).string();
    std::string workdir = entries.empty() ? work : topsrc;

    for (const char* phase : {"prepare", "build"}) {
        auto cmds = rec.getCmds(phase);
        if (!cmds.empty()) runCmds(cmds, workdir, env);
    }
    {
        auto check = rec.getCmds("check");
        if (!check.empty()) {
            try {
                runCmds(check, workdir, env);
            } catch (const std::runtime_error& e) {
                if (verbose) std::cout << "check failed (non-fatal): " << e.what() << "\n";
            }
        }
    }
    {
        auto inst = rec.getCmds("install");
        if (!inst.empty()) {
            runCmds(inst, workdir, env);
        } else if (name == "hello-venim") {
            fs::create_directories(fs::path(destdir) / "bin", ec);
            std::string srcC = srcdir + "/hello.c";
            std::string out = destdir + "/bin/hello";
            int rc = runExec({"cc", srcC, "-o", out});
            if (rc != 0) throw std::runtime_error("cc failed for hello-venim");
        } else {
            throw std::runtime_error("recipe has no install steps");
        }
    }
    return destdir;
}

std::string installDestdir(const std::string& root, const vnb::Recipe& rec,
                           const std::string& destdir, const std::string& source,
                           const std::string& checksum) {
    std::string name = rec.name, ver = rec.getStr("version", "0");
    std::string dd = destdir;
    std::error_code ec;
    if (fs::is_directory(fs::path(dd) / "usr", ec) &&
        !fs::exists(fs::path(dd) / "bin", ec)) {
        std::string norm = dd;
        while (!norm.empty() && norm.back() == '/') norm.pop_back();
        norm += ".nexa";
        util::removeAll(norm);
        toExedraLayout(dd, norm);
        dd = norm;
    }
    std::string pkgdir = root + "/venim/packages/" + name + "/" + ver;
    util::removeAll(pkgdir);
    util::copyTree(dd, pkgdir);
    adapter::adaptTree(pkgdir);
    std::vector<std::string> errs;
    adapter::validateTree(pkgdir, pkgdir, errs);
    if (!errs.empty()) {
        std::string msg = "validation failed:\n";
        for (size_t i = 0; i < errs.size(); i++) {
            if (i) msg += "\n";
            msg += errs[i];
        }
        throw std::runtime_error(msg);
    }
    std::vector<std::string> files = listFiles(pkgdir);
    std::vector<std::string> linked = linkPackage(root, name, ver);
    std::vector<std::string> all = files;
    for (const auto& l : linked) all.push_back("link:" + l);
    db::recordInstall(root, name, ver, rec.getStr("description", ""), all,
                      rec.getStrList("depends"), checksum, source);
    return pkgdir;
}

std::string installBinary(const std::string& root, const vnb::Recipe& rec,
                          bool verbose, bool noCheck) {
    (void)verbose;
    const vnb::Value* b = rec.get("binary");
    bool haveUrl = b && b->isMap() && b->map.count("url") &&
                   b->map.at("url").isStr() && !b->map.at("url").s.empty();
    if (!haveUrl) throw std::runtime_error("recipe has no binary section");
    std::string url = b->map.at("url").s;
    // exact download filename: explicit `file` key or sanitized URL basename
    // (query strings stripped: ?platform=linux... -> plain name)
    std::string fn;
    auto fit = b->map.find("file");
    if (fit != b->map.end() && fit->second.isStr() && !fit->second.s.empty()) {
        fn = fit->second.s;
    } else {
        fn = fs::path(url).filename().string();
        size_t q = fn.find_first_of("?#&");
        if (q != std::string::npos) fn = fn.substr(0, q);
        if (fn.empty()) fn = rec.name;
    }
    std::string arc = root + "/venim/cache/" + fn;
    fetch(url, arc);
    auto it = b->map.find("sha256");
    if (!noCheck && it != b->map.end() && it->second.isStr()) {
        std::string got = sha256File(arc);
        if (got != it->second.s)
            throw std::runtime_error("sha256 mismatch: want " + it->second.s +
                                     ", got " + got);
    }
    std::string ver = rec.getStr("version", "0");
    std::string work = root + "/venim/build/" + rec.name + "-" + ver + "-bin";
    std::string stage = work + "/stage";
    std::error_code ec;
    util::removeAll(stage);
    fs::create_directories(stage, ec);
    // decide by extension up front: GNU tar exits 0 even on non-tar input,
    // so its exit code cannot tell archives apart from single files
    std::string low = arc;
    for (auto& c : low) c = (char)tolower((unsigned char)c);
    auto ends = [&](const char* sfx) {
        size_t n = strlen(sfx);
        return low.size() >= n && low.compare(low.size() - n, n, sfx) == 0;
    };
    bool isArchive = ends(".zip") || ends(".zst") || ends(".zstd") ||
                     ends(".tar") || ends(".txz") || ends(".tbz") ||
                     ends(".tgz") || ends(".tar.gz") || ends(".tar.xz") ||
                     ends(".tar.zst") || ends(".tar.bz2");
    if (isArchive) {
        adapter::unpack(arc, stage);
    } else if (ends(".gz") || ends(".bz2")) {
        // single compressed file: decompress into stage
        std::string fn = fs::path(arc).filename().string();
        std::string bare = fn.substr(0, fn.size() - (ends(".gz") ? 3 : 4));
        std::string tool = ends(".gz") ? "gzip -dc" : "bzip2 -dc";
        std::string out = (fs::path(stage) / bare).string();
        int rc = util::runQuiet(tool + " " + util::shellQuote(arc) + " > " +
                                util::shellQuote(out));
        if (rc != 0) throw std::runtime_error("cannot stage payload: " + arc);
    } else {
        // single-file payloads (AppImage, static binaries): stage as-is
        std::string fn = fs::path(arc).filename().string();
        if (!util::copyFile2(arc, (fs::path(stage) / fn).string()))
            throw std::runtime_error("cannot stage payload: " + arc);
    }
    std::string inner = stripTopLevel(stage);
    std::string destdir = work + "/dest";
    util::removeAll(destdir);
    fs::create_directories(destdir, ec);
    auto inst = rec.getCmds("install");
    if (!inst.empty()) {
        std::string builddir = work + "/build";
        fs::create_directories(builddir, ec);
        auto env = baseEnv(rec, fs::path(arc).parent_path().string(), builddir, destdir);
        try {
            runCmds(inst, stage, env);
        } catch (const std::runtime_error& e) {
            std::cout << "install steps failed, generic layout instead: " << e.what()
                      << "\n";
            util::removeAll(destdir);
            toExedraLayout(inner, destdir);
        }
    } else {
        toExedraLayout(inner, destdir);
    }
    std::string sum;
    if (it != b->map.end() && it->second.isStr()) sum = it->second.s;
    return installDestdir(root, rec, destdir, "binary", sum);
}

void fetchFresh(const std::string& url, const std::string& dest) {
    std::error_code ec;
    fs::remove(dest, ec);
    fetch(url, dest);
}

}  // namespace core
