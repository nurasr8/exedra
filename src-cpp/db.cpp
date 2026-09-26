// SQLite packages database. Same schema/SQL as the Python implementation.
#include "db.h"

#include <sqlite3.h>

#include <cstdio>
#include <stdexcept>

#include "util.h"

namespace db {
namespace {

const char* SCHEMA =
    "CREATE TABLE IF NOT EXISTS packages("
    "  name TEXT,"
    "  version TEXT,"
    "  description TEXT DEFAULT '',"
    "  active INTEGER DEFAULT 1,"
    "  files TEXT DEFAULT '[]',"
    "  depends TEXT DEFAULT '[]',"
    "  checksum TEXT DEFAULT '',"
    "  source TEXT DEFAULT '',"
    "  PRIMARY KEY(name, version)"
    ");";

struct Conn {
    sqlite3* h = nullptr;
    explicit Conn(const std::string& root) {
        std::string dir = root + "/venim/db";
        std::string path = dir + "/packages.db";
        if (util::makeDirs(dir) &&
            sqlite3_open(path.c_str(), &h) == SQLITE_OK) {
            exec(SCHEMA);
            return;
        }
        // fall back to memory db (mirrors the Python impl)
        h = nullptr;
        if (sqlite3_open(":memory:", &h) == SQLITE_OK) exec(SCHEMA);
        if (!h) throw std::runtime_error("cannot open packages db");
    }
    ~Conn() {
        if (h) sqlite3_close(h);
    }
    void exec(const char* sql) {
        char* err = nullptr;
        if (sqlite3_exec(h, sql, nullptr, nullptr, &err) != SQLITE_OK) {
            std::string e = err ? err : "sql error";
            sqlite3_free(err);
            throw std::runtime_error(e);
        }
    }
};

}  // namespace

void recordInstall(const std::string& root, const std::string& name,
                   const std::string& version, const std::string& description,
                   const std::vector<std::string>& files,
                   const std::vector<std::string>& depends,
                   const std::string& checksum, const std::string& source) {
    Conn c(root);
    sqlite3_stmt* s = nullptr;
    const char* up = "UPDATE packages SET active=0 WHERE name=?";
    sqlite3_prepare_v2(c.h, up, -1, &s, nullptr);
    sqlite3_bind_text(s, 1, name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_step(s);
    sqlite3_finalize(s);
    const char* ins =
        "INSERT OR REPLACE INTO packages(name,version,description,active,files,depends,checksum,source)"
        " VALUES(?,?,?,?,?,?,?,?)";
    sqlite3_prepare_v2(c.h, ins, -1, &s, nullptr);
    std::string fj = util::jsonArray(files), dj = util::jsonArray(depends);
    sqlite3_bind_text(s, 1, name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(s, 2, version.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(s, 3, description.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(s, 4, 1);
    sqlite3_bind_text(s, 5, fj.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(s, 6, dj.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(s, 7, checksum.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(s, 8, source.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_step(s);
    sqlite3_finalize(s);
}

void recordRemove(const std::string& root, const std::string& name,
                  const std::string& version) {
    Conn c(root);
    sqlite3_stmt* s = nullptr;
    if (!version.empty()) {
        sqlite3_prepare_v2(c.h, "DELETE FROM packages WHERE name=? AND version=?",
                           -1, &s, nullptr);
        sqlite3_bind_text(s, 1, name.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(s, 2, version.c_str(), -1, SQLITE_TRANSIENT);
    } else {
        sqlite3_prepare_v2(c.h, "DELETE FROM packages WHERE name=?", -1, &s, nullptr);
        sqlite3_bind_text(s, 1, name.c_str(), -1, SQLITE_TRANSIENT);
    }
    sqlite3_step(s);
    sqlite3_finalize(s);
}

namespace {

bool fetchPkg(sqlite3* h, const char* sql, const std::vector<std::string>& binds,
              Pkg& out) {
    sqlite3_stmt* s = nullptr;
    sqlite3_prepare_v2(h, sql, -1, &s, nullptr);
    for (size_t i = 0; i < binds.size(); i++)
        sqlite3_bind_text(s, (int)i + 1, binds[i].c_str(), -1, SQLITE_TRANSIENT);
    bool ok = false;
    if (sqlite3_step(s) == SQLITE_ROW) {
        auto col = [&](int i) {
            const char* t = (const char*)sqlite3_column_text(s, i);
            return t ? std::string(t) : std::string();
        };
        out.name = col(0);
        out.version = col(1);
        out.description = col(2);
        out.active = sqlite3_column_int(s, 3);
        out.files = util::parseJsonStringArray(col(4));
        out.depends = util::parseJsonStringArray(col(5));
        ok = true;
    }
    sqlite3_finalize(s);
    return ok;
}

}  // namespace

bool getPkg(const std::string& root, const std::string& name,
            const std::string& version, Pkg& out) {
    Conn c(root);
    return fetchPkg(c.h,
                    "SELECT name,version,description,active,files,depends FROM packages"
                    " WHERE name=? AND version=?",
                    {name, version}, out);
}

bool getActive(const std::string& root, const std::string& name, Pkg& out) {
    Conn c(root);
    return fetchPkg(c.h,
                    "SELECT name,version,description,active,files,depends FROM packages"
                    " WHERE name=? AND active=1",
                    {name}, out);
}

std::vector<Row> listPkgs(const std::string& root) {
    Conn c(root);
    std::vector<Row> rows;
    sqlite3_stmt* s = nullptr;
    sqlite3_prepare_v2(c.h,
                       "SELECT name,version,active,description FROM packages ORDER BY name",
                       -1, &s, nullptr);
    while (sqlite3_step(s) == SQLITE_ROW) {
        Row r;
        const char* a = (const char*)sqlite3_column_text(s, 0);
        const char* b = (const char*)sqlite3_column_text(s, 1);
        const char* d = (const char*)sqlite3_column_text(s, 3);
        r.name = a ? a : "";
        r.version = b ? b : "";
        r.active = sqlite3_column_int(s, 2);
        r.description = d ? d : "";
        rows.push_back(r);
    }
    sqlite3_finalize(s);
    return rows;
}

std::vector<std::string> reverseDeps(const std::string& root, const std::string& name) {
    Conn c(root);
    std::vector<std::string> out;
    sqlite3_stmt* s = nullptr;
    sqlite3_prepare_v2(c.h, "SELECT name,depends FROM packages WHERE active=1",
                       -1, &s, nullptr);
    while (sqlite3_step(s) == SQLITE_ROW) {
        const char* a = (const char*)sqlite3_column_text(s, 0);
        const char* d = (const char*)sqlite3_column_text(s, 1);
        std::string pkg = a ? a : "";
        for (const auto& dep : util::parseJsonStringArray(d ? d : "")) {
            if (dep == name) {
                out.push_back(pkg);
                break;
            }
        }
    }
    sqlite3_finalize(s);
    return out;
}

}  // namespace db
