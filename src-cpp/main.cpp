// venim CLI. Same commands, flags, messages and exit codes as the Python impl.
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "core.h"
#include "db.h"
#include "util.h"
#include "vnb.h"

namespace fs = std::filesystem;

const char* VERSION = "0.1.0";

namespace {

struct Opts {
    std::string root;
    std::string repo = ".";
    bool source = false, binary = false, verbose = false, quiet = false,
         json = false, force = false, version = false;
    std::string rootArg;
    bool hasRootArg = false;
    std::string command;
    std::vector<std::string> operands;
};

std::string toLower(std::string s) {
    for (auto& c : s) c = (char)tolower((unsigned char)c);
    return s;
}

bool scanRecipes(const std::string& repo, const std::string& want,
                 std::vector<std::string>& hits, bool all) {
    for (const char* base : {"packages", "examples"}) {
        std::string dir = (fs::path(repo) / base).string();
        std::error_code ec;
        if (!fs::is_directory(dir, ec)) continue;
        for (auto it = fs::recursive_directory_iterator(
                 dir, fs::directory_options::skip_permission_denied, ec);
             it != fs::recursive_directory_iterator(); ++it) {
            if (!it->is_regular_file(ec)) continue;
            std::string p = it->path().string();
            if (p.size() < 4 || p.compare(p.size() - 4, 4, ".vnb") != 0)
                continue;
            std::string name = it->path().stem().string();
            if (all || name == want) hits.push_back(p);
            if (!all && name == want) return true;
        }
    }
    return !hits.empty();
}

std::string findRecipe(const std::string& name, const std::string& repo) {
    std::vector<std::string> hits;
    if (scanRecipes(repo, name, hits, false)) {
        std::sort(hits.begin(), hits.end());
        return hits[0];
    }
    return "";
}

// pretty JSON like json.dumps([{"name":..,"version":..,"active":bool,"desc":..}], indent=2)
std::string dumpPrettyList(const std::vector<db::Row>& rows) {
    std::string out = "[\n";
    for (size_t i = 0; i < rows.size(); i++) {
        if (i) out += ",\n";
        out += "  {\n    \"name\": " + util::jsonStr(rows[i].name) +
               ",\n    \"version\": " + util::jsonStr(rows[i].version) +
               ",\n    \"active\": " + (rows[i].active ? "true" : "false") +
               ",\n    \"desc\": " + util::jsonStr(rows[i].description) + "\n  }";
    }
    out += "\n]";
    return out;
}

int cmdInstall(const Opts& o) {
    std::string recPath = findRecipe(o.operands[0], o.repo);
    if (recPath.empty() && fs::exists(o.operands[0])) recPath = o.operands[0];
    if (recPath.empty()) {
        std::cerr << "package not found: " << o.operands[0] << "\n";
        return 1;
    }
    vnb::Recipe rec = vnb::parseFile(recPath);
    bool hasSource = rec.has("source");
    bool hasBinary = rec.has("binary");
    bool useBinary = o.binary || (!hasSource && hasBinary);
    if (o.source) useBinary = false;
    for (const auto& dep : rec.getStrList("depends")) {
        db::Pkg tmp;
        if (!db::getActive(o.root, dep, tmp))
            std::cout << "dependency missing: " << dep << " (install it first)\n";
    }
    std::string pkgdir;
    if (useBinary)
        pkgdir = core::installBinary(o.root, rec, o.verbose);
    else {
        std::string dest = core::buildSource(o.root, rec, o.verbose);
        pkgdir = core::installDestdir(o.root, rec, dest);
    }
    if (!o.quiet)
        std::cout << "installed " << rec.name << " " << rec.getStr("version", "")
                  << " -> " << pkgdir << "\n";
    return 0;
}

int cmdRemove(const Opts& o) {
    auto rev = db::reverseDeps(o.root, o.operands[0]);
    if (!rev.empty() && !o.force) {
        std::cerr << "refused: needed by ";
        for (size_t i = 0; i < rev.size(); i++) {
            if (i) std::cerr << ", ";
            std::cerr << rev[i];
        }
        std::cerr << " (use --force)\n";
        return 1;
    }
    db::Pkg pkg;
    if (!db::getActive(o.root, o.operands[0], pkg)) {
        std::cerr << "not installed: " << o.operands[0] << "\n";
        return 1;
    }
    core::unlinkPackage(o.root, pkg.name, pkg.version);
    db::recordRemove(o.root, pkg.name, pkg.version);
    if (!o.quiet) std::cout << "removed " << pkg.name << " " << pkg.version << "\n";
    return 0;
}

int cmdList(const Opts& o) {
    auto rows = db::listPkgs(o.root);
    if (o.json) {
        std::cout << dumpPrettyList(rows) << "\n";
    } else {
        for (const auto& r : rows)
            std::cout << r.name << " " << r.version
                      << (r.active ? "" : " (inactive)") << " - " << r.description
                      << "\n";
    }
    return 0;
}

int cmdInfo(const Opts& o) {
    std::string recPath = findRecipe(o.operands[0], o.repo);
    db::Pkg inst;
    bool hasInst = db::getActive(o.root, o.operands[0], inst);
    if (o.json) {
        std::cout << "{\n  \"recipe\": "
                  << (recPath.empty() ? "null" : util::jsonStr(recPath))
                  << ",\n  \"installed\": ";
        if (!hasInst) {
            std::cout << "null\n}\n";
        } else {
            std::cout << "{\n    \"name\": " << util::jsonStr(inst.name)
                      << ",\n    \"version\": " << util::jsonStr(inst.version)
                      << ",\n    \"description\": "
                      << util::jsonStr(inst.description)
                      << ",\n    \"active\": "
                      << (inst.active ? "true" : "false")
                      << ",\n    \"files\": " << util::jsonArray(inst.files)
                      << ",\n    \"depends\": " << util::jsonArray(inst.depends)
                      << "\n  }\n}\n";
        }
        return 0;
    }
    if (!recPath.empty()) {
        vnb::Recipe rec = vnb::parseFile(recPath);
        std::cout << rec.name << " " << rec.getStr("version", "") << "\n";
        std::cout << "  desc: " << rec.getStr("description", "") << "\n";
        std::cout << "  recipe: " << recPath << "\n";
        auto deps = rec.getStrList("depends");
        std::cout << "  depends: ";
        if (deps.empty())
            std::cout << "-\n";
        else {
            for (size_t i = 0; i < deps.size(); i++) {
                if (i) std::cout << " ";
                std::cout << deps[i];
            }
            std::cout << "\n";
        }
    }
    if (hasInst)
        std::cout << "  installed: " << inst.version << " (" << inst.files.size()
                  << " files)\n";
    else if (recPath.empty()) {
        std::cout << "unknown package\n";
        return 1;
    }
    return 0;
}

int cmdSearch(const Opts& o) {
    std::vector<std::string> hits;
    scanRecipes(o.repo, "", hits, true);
    std::vector<std::pair<std::string, std::string>> named;
    std::string q = toLower(o.operands[0]);
    for (const auto& p : hits) {
        std::string name = fs::path(p).stem().string();
        if (toLower(name).find(q) != std::string::npos)
            named.push_back({name, p});
    }
    if (o.json) {
        std::cout << "[";
        for (size_t i = 0; i < named.size(); i++) {
            if (i) std::cout << ", ";
            std::cout << "[" << util::jsonStr(named[i].first) << ", "
                      << util::jsonStr(named[i].second) << "]";
        }
        std::cout << "]\n";
    } else {
        std::sort(named.begin(), named.end());
        for (const auto& [n, p] : named) std::cout << n << " - " << p << "\n";
    }
    return 0;
}

int cmdDepends(const Opts& o) {
    std::string recPath = findRecipe(o.operands[0], o.repo);
    if (recPath.empty()) return 1;
    auto deps = vnb::parseFile(recPath).getStrList("depends");
    for (size_t i = 0; i < deps.size(); i++) {
        if (i) std::cout << " ";
        std::cout << deps[i];
    }
    std::cout << "\n";
    return 0;
}

int cmdProvides(const Opts& o) {
    for (const auto& r : db::listPkgs(o.root)) {
        db::Pkg pkg;
        if (!db::getPkg(o.root, r.name, r.version, pkg)) continue;
        for (const auto& f : pkg.files) {
            bool hit = f.size() >= o.operands[0].size() &&
                       f.compare(f.size() - o.operands[0].size(),
                                 o.operands[0].size(), o.operands[0]) == 0;
            if (!hit) hit = f.find(o.operands[0]) != std::string::npos;
            if (hit) std::cout << r.name << " " << r.version << ": " << f << "\n";
        }
    }
    return 0;
}

int cmdVerify(const Opts& o) {
    db::Pkg pkg;
    if (!db::getActive(o.root, o.operands[0], pkg)) {
        std::cerr << "not installed\n";
        return 1;
    }
    std::vector<std::string> missing;
    std::string pkgdir =
        o.root + "/venim/packages/" + pkg.name + "/" + pkg.version;
    std::error_code ec;
    for (const auto& f : pkg.files) {
        if (f.compare(0, 5, "link:") == 0) {
            std::string p = (fs::path(o.root) / f.substr(5)).string();
            if (!fs::is_symlink(p, ec)) missing.push_back(f);
        } else {
            if (!fs::exists(fs::path(pkgdir) / f, ec)) missing.push_back(f);
        }
        ec.clear();
    }
    if (!missing.empty()) {
        std::cout << "BROKEN:\n";
        for (const auto& m : missing) std::cout << "  " << m << "\n";
        return 1;
    }
    std::cout << "ok\n";
    return 0;
}

int cmdDoctor(const Opts& o) {
    int bad = 0;
    for (const auto& r : db::listPkgs(o.root)) {
        if (!r.active) continue;
        db::Pkg pkg;
        if (!db::getPkg(o.root, r.name, r.version, pkg)) continue;
        for (const auto& dep : pkg.depends) {
            db::Pkg tmp;
            if (!db::getActive(o.root, dep, tmp)) {
                std::cout << r.name << ": missing dep " << dep << "\n";
                bad++;
            }
        }
    }
    std::string usrb = (fs::path(o.root) / "usr/bin").string();
    std::error_code ec;
    if (fs::is_directory(usrb, ec)) {
        for (auto& e : fs::directory_iterator(usrb, ec)) {
            auto st = e.symlink_status(ec);
            if (ec) continue;
            if (fs::is_symlink(st) && !fs::exists(e.path(), ec))
                std::cout << "dangling link: usr/bin/"
                          << e.path().filename().string() << "\n",
                    bad++;
        }
    }
    if (bad == 0)
        std::cout << "ok\n";
    else
        std::cout << bad << " problem(s)\n";
    return bad ? 1 : 0;
}

int cmdRepair(const Opts& o) {
    std::string target = o.hasRootArg ? o.rootArg : o.root;
    int count = 0;
    for (const auto& r : db::listPkgs(target)) {
        if (!r.active) continue;
        core::linkPackage(target, r.name, r.version);
        count++;
    }
    std::cout << "relinked " << count << " package(s) under " << target << "\n";
    return 0;
}

int cmdBuild(const Opts& o) {
    vnb::Recipe rec = vnb::parseFile(o.operands[0]);
    std::cout << core::buildSource(o.root, rec, o.verbose) << "\n";
    return 0;
}

int cmdClean(const Opts& o) {
    for (const char* d : {"venim/build", "venim/cache"}) {
        std::string p = (fs::path(o.root) / d).string();
        std::error_code ec;
        if (fs::is_directory(p, ec)) {
            util::removeAll(p);
            fs::create_directories(p, ec);
        }
    }
    std::cout << "cleaned\n";
    return 0;
}

int cmdUpdate(const Opts&) {
    std::cout << "recipe index is local (packages/ + examples/); nothing to fetch yet\n";
    return 0;
}

int cmdUpgrade(const Opts& o) {
    std::cout << "upgrade: re-install listed packages from recipes\n";
    for (const auto& r : db::listPkgs(o.root)) {
        if (!r.active) continue;
        std::string recPath = findRecipe(r.name, o.repo);
        if (recPath.empty()) continue;
        vnb::Recipe rec = vnb::parseFile(recPath);
        if (rec.getStr("version", "0") != r.version) {
            std::cout << r.name << ": " << r.version << " -> "
                      << rec.getStr("version", "") << "\n";
            std::string dest = core::buildSource(o.root, rec, o.verbose);
            core::installDestdir(o.root, rec, dest);
        }
    }
    return 0;
}

void printHelp() {
    std::cout
        << "usage: venim [--root ROOT] [--repo REPO] [--source] [--binary]\n"
        << "             [--verbose] [--quiet] [--json] [--force] [--version]\n"
        << "             {install,remove,list,info,search,depends,provides,verify,\n"
        << "              clean,build,doctor,repair,update,upgrade} ...\n";
}

}  // namespace

int main(int argc, char** argv) {
    Opts o;
    if (const char* e = getenv("VENIM_ROOT")) o.root = e;
    if (o.root.empty()) o.root = "/";
    std::vector<std::string> args(argv + 1, argv + argc);
    for (size_t i = 0; i < args.size(); i++) {
        const std::string& a = args[i];
        auto val = [&](std::string& out) {
            size_t eq = a.find('=');
            if (eq != std::string::npos) {
                out = a.substr(eq + 1);
                return true;
            }
            if (i + 1 < args.size()) {
                out = args[++i];
                return true;
            }
            return false;
        };
        if (a == "--root" || a.compare(0, 7, "--root=") == 0) {
            if (!val(o.root)) {
                std::cerr << "venim: --root needs a value\n";
                return 2;
            }
        } else if (a == "--repo" || a.compare(0, 7, "--repo=") == 0) {
            if (!val(o.repo)) {
                std::cerr << "venim: --repo needs a value\n";
                return 2;
            }
        } else if (a == "--root-arg" || a.compare(0, 11, "--root-arg=") == 0) {
            // mirror argparse: --root-arg takes a value
            if (!val(o.rootArg)) {
                std::cerr << "venim: --root-arg needs a value\n";
                return 2;
            }
            o.hasRootArg = true;
        } else if (a == "--source")
            o.source = true;
        else if (a == "--binary")
            o.binary = true;
        else if (a == "--verbose")
            o.verbose = true;
        else if (a == "--quiet")
            o.quiet = true;
        else if (a == "--json")
            o.json = true;
        else if (a == "--force")
            o.force = true;
        else if (a == "--version")
            o.version = true;
        else if (a.compare(0, 2, "--") == 0) {
            std::cerr << "venim: unrecognized arguments: " << a << "\n";
            return 2;
        } else if (o.command.empty()) {
            o.command = a;
        } else {
            o.operands.push_back(a);
        }
    }
    if (o.version) {
        std::cout << "venim " << VERSION << "\n";
        return 0;
    }
    if (o.command.empty()) {
        printHelp();
        return 0;
    }
    struct Cmd {
        const char* name;
        int (*fn)(const Opts&);
        const char* field;  // required operand name or nullptr
    };
    static const Cmd table[] = {
        {"install", cmdInstall, "package"}, {"remove", cmdRemove, "package"},
        {"list", cmdList, nullptr},         {"info", cmdInfo, "package"},
        {"search", cmdSearch, "query"},     {"depends", cmdDepends, "package"},
        {"provides", cmdProvides, "file"},  {"verify", cmdVerify, "package"},
        {"clean", cmdClean, nullptr},       {"build", cmdBuild, "recipe"},
        {"doctor", cmdDoctor, nullptr},     {"repair", cmdRepair, nullptr},
        {"update", cmdUpdate, nullptr},     {"upgrade", cmdUpgrade, nullptr},
    };
    for (const auto& c : table) {
        if (o.command == c.name) {
            if (c.field && o.operands.empty()) {
                std::cerr << o.command << " needs an argument\n";
                return 1;
            }
            try {
                return c.fn(o);
            } catch (const std::exception& e) {
                std::cerr << "venim: error: " << e.what() << "\n";
                return 1;
            }
        }
    }
    std::cerr << "unknown command: " << o.command << "\n";
    return 1;
}
