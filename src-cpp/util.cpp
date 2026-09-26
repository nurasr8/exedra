#include "util.h"

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <sstream>
#include <sys/wait.h>

namespace fs = std::filesystem;

namespace util {

bool makeDirs(const std::string& path) {
    if (path.empty()) return false;
    std::error_code ec;
    fs::create_directories(path, ec);
    return !ec;
}

bool removeAll(const std::string& path) {
    std::error_code ec;
    fs::remove_all(path, ec);
    return !ec;
}

bool copyFile2(const std::string& src, const std::string& dst) {
    std::error_code ec;
    fs::copy_file(src, dst, fs::copy_options::overwrite_existing, ec);
    if (ec) return false;
    fs::permissions(dst, fs::status(src).permissions(), ec);
    return !ec;
}

void copyTree(const std::string& src, const std::string& dst) {
    std::error_code ec;
    fs::create_directories(dst, ec);
    for (auto it = fs::recursive_directory_iterator(
             src, fs::directory_options::skip_permission_denied, ec);
         it != fs::recursive_directory_iterator(); ++it) {
        std::string rel = fs::relative(it->path(), src, ec).string();
        if (ec) continue;
        fs::path d = fs::path(dst) / rel;
        auto st = it->symlink_status(ec);
        if (ec) continue;
        if (fs::is_directory(st)) {
            fs::create_directories(d, ec);
        } else if (fs::is_symlink(st)) {
            fs::remove(d, ec);
            ec.clear();
            fs::create_symlink(fs::read_symlink(it->path(), ec), d, ec);
        } else if (fs::is_regular_file(st)) {
            fs::create_directories(d.parent_path(), ec);
            copyFile2(it->path().string(), d.string());
        }
    }
}

std::string shellQuote(const std::string& s) {
    std::string out = "'";
    for (char c : s) {
        if (c == '\'')
            out += "'\\''";
        else
            out += c;
    }
    out += "'";
    return out;
}

std::string trim(const std::string& s) {
    size_t a = 0, b = s.size();
    while (a < b && std::isspace((unsigned char)s[a])) a++;
    while (b > a && std::isspace((unsigned char)s[b - 1])) b--;
    return s.substr(a, b - a);
}

namespace {

void appendUtf8(std::string& out, unsigned cp) {
    if (cp < 0x80)
        out += (char)cp;
    else if (cp < 0x800) {
        out += (char)(0xC0 | (cp >> 6));
        out += (char)(0x80 | (cp & 0x3F));
    } else if (cp < 0x10000) {
        out += (char)(0xE0 | (cp >> 12));
        out += (char)(0x80 | ((cp >> 6) & 0x3F));
        out += (char)(0x80 | (cp & 0x3F));
    } else {
        out += (char)(0xF0 | (cp >> 18));
        out += (char)(0x80 | ((cp >> 12) & 0x3F));
        out += (char)(0x80 | ((cp >> 6) & 0x3F));
        out += (char)(0x80 | (cp & 0x3F));
    }
}

}  // namespace

std::string jsonEscape(const std::string& s) {
    // ensure_ascii=True semantics, like CPython's json.dumps
    std::string out;
    for (size_t i = 0; i < s.size();) {
        unsigned char c = s[i];
        switch (c) {
            case '"': out += "\\\""; i++; break;
            case '\\': out += "\\\\"; i++; break;
            case '\b': out += "\\b"; i++; break;
            case '\f': out += "\\f"; i++; break;
            case '\n': out += "\\n"; i++; break;
            case '\r': out += "\\r"; i++; break;
            case '\t': out += "\\t"; i++; break;
            default:
                if (c < 0x20) {
                    char buf[8];
                    snprintf(buf, sizeof buf, "\\u%04x", c);
                    out += buf;
                    i++;
                } else if (c < 0x80) {
                    out += (char)c;
                    i++;
                } else {
                    // decode UTF-8, re-emit as \uXXXX (with surrogates if needed)
                    unsigned cp;
                    size_t len;
                    if ((c & 0xE0) == 0xC0 && i + 1 < s.size()) {
                        cp = ((c & 0x1F) << 6) | (s[i + 1] & 0x3F);
                        len = 2;
                    } else if ((c & 0xF0) == 0xE0 && i + 2 < s.size()) {
                        cp = ((c & 0x0F) << 12) | ((s[i + 1] & 0x3F) << 6) |
                             (s[i + 2] & 0x3F);
                        len = 3;
                    } else if ((c & 0xF8) == 0xF0 && i + 3 < s.size()) {
                        cp = ((c & 0x07) << 18) | ((s[i + 1] & 0x3F) << 12) |
                             ((s[i + 2] & 0x3F) << 6) | (s[i + 3] & 0x3F);
                        len = 4;
                    } else {
                        cp = c;
                        len = 1;
                    }
                    char buf[16];
                    if (cp < 0x10000) {
                        snprintf(buf, sizeof buf, "\\u%04x", cp);
                        out += buf;
                    } else {
                        cp -= 0x10000;
                        snprintf(buf, sizeof buf, "\\u%04x\\u%04x",
                                 0xD800 + (cp >> 10), 0xDC00 + (cp & 0x3FF));
                        out += buf;
                    }
                    i += len;
                }
        }
    }
    return out;
}

std::string jsonStr(const std::string& s) { return "\"" + jsonEscape(s) + "\""; }

std::string jsonArray(const std::vector<std::string>& v) {
    std::string out = "[";
    for (size_t i = 0; i < v.size(); i++) {
        if (i) out += ", ";
        out += jsonStr(v[i]);
    }
    out += "]";
    return out;
}

std::string jsonBool(bool b) { return b ? "true" : "false"; }

std::vector<std::string> parseJsonStringArray(const std::string& text) {
    // parses ["a", "b"] with standard escapes (incl. \uXXXX)
    std::vector<std::string> out;
    size_t i = 0, n = text.size();
    auto skipWs = [&]() {
        while (i < n && std::isspace((unsigned char)text[i])) i++;
    };
    skipWs();
    if (i >= n || text[i] != '[') return out;
    i++;
    while (true) {
        skipWs();
        if (i < n && text[i] == ']') {
            i++;
            break;
        }
        if (i >= n || text[i] != '"') return {};
        i++;
        std::string cur;
        bool closed = false;
        while (i < n) {
            char c = text[i++];
            if (c == '"') {
                closed = true;
                break;
            }
            if (c == '\\' && i < n) {
                char e = text[i++];
                switch (e) {
                    case '"': cur += '"'; break;
                    case '\\': cur += '\\'; break;
                    case '/': cur += '/'; break;
                    case 'b': cur += '\b'; break;
                    case 'f': cur += '\f'; break;
                    case 'n': cur += '\n'; break;
                    case 'r': cur += '\r'; break;
                    case 't': cur += '\t'; break;
                    case 'u': {
                        if (i + 4 > n) return {};
                        unsigned cp = 0;
                        for (int k = 0; k < 4; k++) {
                            char h = text[i++];
                            cp <<= 4;
                            if (h >= '0' && h <= '9') cp += h - '0';
                            else if (h >= 'a' && h <= 'f') cp += h - 'a' + 10;
                            else if (h >= 'A' && h <= 'F') cp += h - 'A' + 10;
                            else return {};
                        }
                        if (cp >= 0xD800 && cp <= 0xDBFF && i + 6 <= n &&
                            text[i] == '\\' && text[i + 1] == 'u') {
                            unsigned lo = 0;
                            bool ok = true;
                            for (int k = 0; k < 4; k++) {
                                char h = text[i + 2 + k];
                                lo <<= 4;
                                if (h >= '0' && h <= '9') lo += h - '0';
                                else if (h >= 'a' && h <= 'f') lo += h - 'a' + 10;
                                else if (h >= 'A' && h <= 'F') lo += h - 'A' + 10;
                                else ok = false;
                            }
                            if (ok && lo >= 0xDC00 && lo <= 0xDFFF) {
                                cp = 0x10000 + ((cp - 0xD800) << 10) + (lo - 0xDC00);
                                i += 6;
                            }
                        }
                        appendUtf8(cur, cp);
                        break;
                    }
                    default: cur += e; break;
                }
            } else {
                cur += c;
            }
        }
        if (!closed) return {};
        out.push_back(cur);
        skipWs();
        if (i < n && text[i] == ',') {
            i++;
            continue;
        }
        if (i < n && text[i] == ']') {
            i++;
            break;
        }
        return {};
    }
    return out;
}

ProcResult runCapture(const std::string& cmd) {    ProcResult r;
    FILE* p = popen((cmd + " 2>/dev/null").c_str(), "r");
    if (!p) return r;
    char buf[8192];
    size_t n;
    while ((n = fread(buf, 1, sizeof buf, p)) > 0) r.out.append(buf, n);
    r.rc = pclose(p);
    if (WIFEXITED(r.rc)) r.rc = WEXITSTATUS(r.rc);
    return r;
}

int runQuiet(const std::string& cmd) {
    std::string full = cmd + " >/dev/null 2>&1";
    int rc = system(full.c_str());
    if (WIFEXITED(rc)) return WEXITSTATUS(rc);
    return -1;
}

namespace {

class JsonParser {
public:
    explicit JsonParser(const std::string& t) : s(t) {}
    bool run(Json& out) {
        skip();
        if (!value(out)) return false;
        skip();
        return i == s.size();
    }

private:
    const std::string& s;
    size_t i = 0;
    void skip() {
        while (i < s.size() && std::isspace((unsigned char)s[i])) i++;
    }
    bool lit(const char* w, Json& o, Json::Type t, bool b = false) {
        size_t n = strlen(w);
        if (s.compare(i, n, w) != 0) return false;
        i += n;
        o.type = t;
        o.b = b;
        return true;
    }
    bool string(std::string& out) {
        if (i >= s.size() || s[i] != '"') return false;
        i++;
        out.clear();
        while (i < s.size()) {
            char c = s[i++];
            if (c == '"') return true;
            if (c == '\\' && i < s.size()) {
                char e = s[i++];
                switch (e) {
                    case '"': out += '"'; break;
                    case '\\': out += '\\'; break;
                    case '/': out += '/'; break;
                    case 'b': out += '\b'; break;
                    case 'f': out += '\f'; break;
                    case 'n': out += '\n'; break;
                    case 'r': out += '\r'; break;
                    case 't': out += '\t'; break;
                    case 'u': {
                        if (i + 4 > s.size()) return false;
                        unsigned cp = 0;
                        for (int k = 0; k < 4; k++) {
                            char h = s[i++];
                            cp <<= 4;
                            if (h >= '0' && h <= '9') cp += h - '0';
                            else if (h >= 'a' && h <= 'f') cp += h - 'a' + 10;
                            else if (h >= 'A' && h <= 'F') cp += h - 'A' + 10;
                            else return false;
                        }
                        appendUtf8(out, cp);
                        break;
                    }
                    default: out += e; break;
                }
            } else {
                out += c;
            }
        }
        return false;
    }
    bool value(Json& o) {
        skip();
        if (i >= s.size()) return false;
        char c = s[i];
        if (c == '"') {
            o.type = Json::Type::STR;
            return string(o.s);
        }
        if (c == '{') {
            i++;
            o.type = Json::Type::OBJ;
            skip();
            if (i < s.size() && s[i] == '}') {
                i++;
                return true;
            }
            while (true) {
                std::string k;
                skip();
                if (!string(k)) return false;
                skip();
                if (i >= s.size() || s[i] != ':') return false;
                i++;
                Json v;
                if (!value(v)) return false;
                o.obj.push_back({k, v});
                skip();
                if (i >= s.size()) return false;
                if (s[i] == ',') {
                    i++;
                    continue;
                }
                if (s[i] == '}') {
                    i++;
                    return true;
                }
                return false;
            }
        }
        if (c == '[') {
            i++;
            o.type = Json::Type::ARR;
            skip();
            if (i < s.size() && s[i] == ']') {
                i++;
                return true;
            }
            while (true) {
                Json v;
                if (!value(v)) return false;
                o.arr.push_back(v);
                skip();
                if (i >= s.size()) return false;
                if (s[i] == ',') {
                    i++;
                    continue;
                }
                if (s[i] == ']') {
                    i++;
                    return true;
                }
                return false;
            }
        }
        if (lit("true", o, Json::Type::BOOL, true)) return true;
        if (lit("false", o, Json::Type::BOOL, false)) return true;
        if (lit("null", o, Json::Type::NUL)) return true;
        if (c == '-' || (c >= '0' && c <= '9')) {
            size_t st = i;
            if (c == '-') i++;
            while (i < s.size() &&
                   (std::isdigit((unsigned char)s[i]) || s[i] == '.' ||
                    s[i] == 'e' || s[i] == 'E' || s[i] == '+' || s[i] == '-'))
                i++;
            o.type = Json::Type::NUM;
            o.s = s.substr(st, i - st);
            return true;
        }
        return false;
    }
};

}  // namespace

bool parseJson(const std::string& text, Json& out) {
    JsonParser p(text);
    return p.run(out);
}

}  // namespace util
