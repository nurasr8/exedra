#pragma once
// Small shared helpers: filesystem, shell quoting, JSON (Python-compatible).
#include <string>
#include <vector>

namespace util {

bool makeDirs(const std::string& path);  // mkdir -p, false on failure
bool removeAll(const std::string& path);
bool copyFile2(const std::string& src, const std::string& dst);  // copy2: data+perms
void copyTree(const std::string& src, const std::string& dst);   // symlinks kept
std::string shellQuote(const std::string& s);  // single-quote for system()
std::string trim(const std::string& s);

// JSON with Python json.dumps semantics (ensure_ascii, indent support)
std::string jsonEscape(const std::string& s);
std::string jsonStr(const std::string& s);
std::string jsonArray(const std::vector<std::string>& v);
std::string jsonBool(bool b);
std::vector<std::string> parseJsonStringArray(const std::string& text);

// Minimal JSON value (for repo index.json)
struct Json {
    enum class Type { NUL, BOOL, NUM, STR, ARR, OBJ };
    Type type = Type::NUL;
    bool b = false;
    std::string s;
    std::vector<Json> arr;
    std::vector<std::pair<std::string, Json>> obj;
    const Json* find(const std::string& key) const {
        if (type != Type::OBJ) return nullptr;
        for (const auto& [k, v] : obj)
            if (k == key) return &v;
        return nullptr;
    }
    std::string str(const std::string& dflt = "") const {
        return type == Type::STR ? s : dflt;
    }
};
bool parseJson(const std::string& text, Json& out);

struct ProcResult {
    int rc = -1;
    std::string out;
};
ProcResult runCapture(const std::string& cmd);  // popen, merged stdout
int runQuiet(const std::string& cmd);           // system(), output discarded

}  // namespace util
