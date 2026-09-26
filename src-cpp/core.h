#pragma once
// Core package operations: fetch/build/link/install/unlink.
#include <map>
#include <string>
#include <vector>

#include "vnb.h"

namespace core {

std::string sha256File(const std::string& path);
void fetch(const std::string& url, const std::string& dest);
void runCmds(const std::vector<std::string>& cmds, const std::string& cwd,
             const std::map<std::string, std::string>& env);

std::vector<std::string> listFiles(const std::string& tree);
// returns linked target paths (relative to root)
std::vector<std::string> linkPackage(const std::string& root,
                                     const std::string& name,
                                     const std::string& version);
std::vector<std::string> unlinkPackage(const std::string& root,
                                       const std::string& name,
                                       const std::string& version);

// build recipe from source -> destdir (caller owns work dirs)
std::string buildSource(const std::string& root, const vnb::Recipe& rec,
                        bool verbose = false);
// install staged destdir into venim/packages + link + db
std::string installDestdir(const std::string& root, const vnb::Recipe& rec,
                           const std::string& destdir,
                           const std::string& source = "source",
                           const std::string& checksum = "");
std::string installBinary(const std::string& root, const vnb::Recipe& rec,
                          bool verbose = false);

}  // namespace core
