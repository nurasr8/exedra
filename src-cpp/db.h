#pragma once
// Installed-packages database (SQLite). Same schema as the Python impl,
// so existing venim/db/packages.db files keep working.
#include <cstdint>
#include <string>
#include <vector>

namespace db {

struct Pkg {
    std::string name;
    std::string version;
    std::string description;
    int active = 0;
    std::vector<std::string> files;
    std::vector<std::string> depends;
};

struct Row {
    std::string name, version, description;
    int active = 0;
};

void recordInstall(const std::string& root, const std::string& name,
                   const std::string& version, const std::string& description,
                   const std::vector<std::string>& files,
                   const std::vector<std::string>& depends,
                   const std::string& checksum, const std::string& source);
void recordRemove(const std::string& root, const std::string& name,
                  const std::string& version /* empty = all */);
// active install of name (or exact name+version)
bool getPkg(const std::string& root, const std::string& name,
            const std::string& version, Pkg& out);
bool getActive(const std::string& root, const std::string& name, Pkg& out);
std::vector<Row> listPkgs(const std::string& root);
std::vector<std::string> reverseDeps(const std::string& root, const std::string& name);

}  // namespace db
