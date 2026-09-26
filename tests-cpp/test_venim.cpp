// Port of tests/test_venim.py: parser + hello build/install/unlink cycle.
#include <cassert>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>

#include "core.h"
#include "db.h"
#include "util.h"
#include "vnb.h"

namespace fs = std::filesystem;

static std::string tmpDir() {
    char t[] = "/tmp/venim-test-XXXXXX";
    if (!mkdtemp(t)) throw std::runtime_error("mkdtemp failed");
    return t;
}

static void testParserMinimal() {
    std::string d = tmpDir();
    std::string p = d + "/a.vnb";
    {
        std::ofstream f(p);
        f << "package \"nano\" {\n version = \"8.6\"\n depends { \"glibc\" \"ncurses\" }\n"
             " build { command \"make\" }\n}\n";
    }
    vnb::Recipe rec = vnb::parseFile(p);
    assert(rec.name == "nano");
    assert(rec.getStr("version") == "8.6");
    auto deps = rec.getStrList("depends");
    assert(deps.size() == 2 && deps[0] == "glibc" && deps[1] == "ncurses");
    assert(rec.getCmds("build") == std::vector<std::string>{"make"});
    fs::remove_all(d);
    std::cout << "ok: parser\n";
}

static void testInstallHello() {
    std::string root = tmpDir();
    vnb::Recipe rec;
    rec.name = "hello-venim";
    rec.fields["version"] = vnb::Value::str("0.1.0");
    rec.fields["description"] = vnb::Value::str("t");
    std::string dest = core::buildSource(root, rec);
    std::string pkgdir = core::installDestdir(root, rec, dest);
    assert(fs::exists(fs::path(pkgdir) / "bin/hello"));
    assert(fs::is_symlink(fs::path(root) / "usr/bin/hello"));
    db::Pkg pkg;
    assert(db::getActive(root, "hello-venim", pkg));
    core::unlinkPackage(root, "hello-venim", "0.1.0");
    assert(!fs::exists(pkgdir));
    fs::remove_all(root);
    std::cout << "ok: install-hello\n";
}

static void testRelPath() {
    // lexical only: must NOT resolve symlinks (libstdc++ relative() does)
    assert(util::relPath("/a/b", "/a/b/c/d") == "c/d");
    assert(util::relPath("/a/b/c", "/a/x/y") == "../../x/y");
    assert(util::relPath("/r/usr/bin", "/r/venim/packages/p/1.0/bin/tool") ==
           "../../venim/packages/p/1.0/bin/tool");
    assert(util::relPath("/r", "/r/opt/t/f") == "opt/t/f");
    std::cout << "ok: relpath\n";
}

int main() {
    testParserMinimal();
    testRelPath();
    testInstallHello();
    return 0;
}
