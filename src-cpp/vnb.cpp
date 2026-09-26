// VenimBuild (*.vnb) recipe parser. Faithful port of the Python implementation.
#include "vnb.h"

#include <cctype>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace vnb {
namespace {

bool isIdentStart(char c) { return std::isalpha((unsigned char)c) || c == '_'; }
bool isIdentChar(char c) {
    return std::isalnum((unsigned char)c) || c == '_' || c == '-' || c == '.';
}

}  // namespace

std::vector<Token> tokenize(const std::string& text) {
    std::vector<Token> toks;
    size_t pos = 0, n = text.size();
    while (pos < n) {
        char c = text[pos];
        if (c == '#') {
            while (pos < n && text[pos] != '\n') pos++;
            continue;
        }
        if (std::isspace((unsigned char)c)) {
            pos++;
            continue;
        }
        if (c == '"') {
            size_t end = text.find('"', pos + 1);
            if (end == std::string::npos)
                throw std::runtime_error("parse error: unterminated string");
            toks.push_back({"STR", text.substr(pos + 1, end - pos - 1)});
            pos = end + 1;
            continue;
        }
        if (isIdentStart(c)) {
            size_t s = pos;
            while (pos < n && isIdentChar(text[pos])) pos++;
            toks.push_back({"ID", text.substr(s, pos - s)});
            continue;
        }
        if (c == '$' && pos + 1 < n && text[pos + 1] == '{') {
            size_t end = text.find('}', pos + 2);
            if (end == std::string::npos)
                throw std::runtime_error("parse error: unterminated ${}");
            toks.push_back({"ID", text.substr(pos, end - pos + 1)});
            pos = end + 1;
            continue;
        }
        if (c == '{' || c == '}' || c == '=') {
            toks.push_back({std::string(1, c), std::string(1, c)});
            pos++;
            continue;
        }
        throw std::runtime_error(std::string("parse error at: '") +
                                 text.substr(pos, 40) + "'");
    }
    return toks;
}

namespace {

class Parser {
public:
    explicit Parser(const std::vector<Token>& t) : toks(t) {}

    std::pair<std::string, std::string> peek() const {
        if (pos < toks.size()) return {toks[pos].kind, toks[pos].text};
        return {"", ""};
    }
    const Token& peekTok(size_t ahead = 0) const {
        static Token empty{"", ""};
        return (pos + ahead < toks.size()) ? toks[pos + ahead] : empty;
    }
    Token next() {
        Token t = peekTok();
        pos++;
        return t;
    }
    std::string expect(const std::string& kind) {
        Token t = next();
        if (t.kind != kind)
            throw std::runtime_error("expected " + kind + ", got " + t.kind + ":" + t.text);
        return t.text;
    }

    Value parseBlock() {
        std::map<std::string, Value> out;
        std::vector<std::string> items;
        std::vector<std::string> cmds;
        while (true) {
            auto [k, v] = peek();
            if (k == "}") {
                next();
                break;
            }
            if (k.empty()) throw std::runtime_error("unexpected eof in block");
            if (k == "ID" && v == "command") {
                next();
                Token c = next();
                if (c.kind != "STR" && c.kind != "ID")
                    throw std::runtime_error("command expects string");
                cmds.push_back(c.text);
                continue;
            }
            if (k == "STR" || k == "ID") {
                Token nxt = peekTok(1);
                if (nxt.kind == "=" || nxt.kind == "{") {
                    std::string key = next().text;
                    auto [nk, nv] = peek();
                    if (nk == "{") {
                        next();
                        out[key] = parseBlock();
                    } else if (nk == "=") {
                        next();
                        Token val = next();
                        if (val.kind != "STR" && val.kind != "ID")
                            throw std::runtime_error("value expected after " + key + " =");
                        out[key] = Value::str(val.text);
                    }
                    continue;
                }
                items.push_back(next().text);
                continue;
            }
            throw std::runtime_error("unexpected token " + k + ":" + v);
        }
        if (!items.empty() && out.empty() && cmds.empty())
            return Value::listOf(items);
        if (!items.empty()) {
            out["_list"] = Value::listOf(items);
            if (out.size() == 1) return Value::listOf(items);
        }
        if (!cmds.empty()) out["_cmds"] = Value::listOf(cmds);
        Value r;
        r.type = Value::Type::MAP;
        r.map = std::move(out);
        return r;
    }

    size_t pos = 0;
    const std::vector<Token>& toks;
};

}  // namespace

Recipe parseRecipe(const std::string& text) {
    std::vector<Token> toks = tokenize(text);
    Parser p(toks);
    Token t = p.next();
    if (t.kind != "ID" || t.text != "package")
        throw std::runtime_error("recipe must start with package \"name\"");
    Token nm = p.next();
    if (nm.kind != "STR") throw std::runtime_error("package name must be a string");
    Recipe rec;
    rec.name = nm.text;
    p.expect("{");
    Value body = p.parseBlock();
    if (body.isMap()) rec.fields = std::move(body.map);
    static const char* cmdBlocks[] = {"build", "install", "prepare", "check"};
    for (const char* b : cmdBlocks) {
        auto it = rec.fields.find(b);
        if (it != rec.fields.end() && it->second.isMap()) {
            auto c = it->second.map.find("_cmds");
            if (c != it->second.map.end() && c->second.isList())
                it->second = c->second;
        }
    }
    static const char* flatLists[] = {"depends", "makedepends", "provides", "conflicts"};
    for (const char* b : flatLists) {
        auto it = rec.fields.find(b);
        if (it != rec.fields.end() && it->second.isMap()) {
            std::vector<std::string> vals;
            for (const auto& [_, x] : it->second.map) {
                if (x.isList())
                    vals.insert(vals.end(), x.list.begin(), x.list.end());
                else if (x.isStr())
                    vals.push_back(x.s);
            }
            it->second = Value::listOf(vals);
        }
    }
    return rec;
}

Recipe parseFile(const std::string& path) {
    std::ifstream f(path);
    if (!f) throw std::runtime_error("cannot open recipe: " + path);
    std::ostringstream ss;
    ss << f.rdbuf();
    return parseRecipe(ss.str());
}

std::string shlexQuote(const std::string& s) {
    if (s.empty()) return "''";
    bool safe = true;
    for (char c : s) {
        if (!(std::isalnum((unsigned char)c) || c == '@' || c == '%' || c == '_' ||
              c == '+' || c == '=' || c == ':' || c == ',' || c == '.' || c == '/' ||
              c == '-')) {
            safe = false;
            break;
        }
    }
    if (safe) return s;
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

std::string expandVars(const std::string& cmd, const std::map<std::string, std::string>& env) {
    std::string out;
    size_t i = 0, n = cmd.size();
    auto identChar = [](char c) {
        return std::isalnum((unsigned char)c) || c == '_';
    };
    while (i < n) {
        if (cmd[i] == '$' && i + 1 < n) {
            if (cmd[i + 1] == '{') {
                size_t end = cmd.find('}', i + 2);
                if (end != std::string::npos && end > i + 2) {
                    std::string key = cmd.substr(i + 2, end - i - 2);
                    auto it = env.find(key);
                    out += (it == env.end()) ? shlexQuote(cmd.substr(i, end - i + 1))
                                             : shlexQuote(it->second);
                    i = end + 1;
                    continue;
                }
            } else if (std::isalpha((unsigned char)cmd[i + 1]) || cmd[i + 1] == '_') {
                size_t s = i + 1;
                size_t e = s;
                while (e < n && identChar(cmd[e])) e++;
                std::string key = cmd.substr(s, e - s);
                auto it = env.find(key);
                out += (it == env.end()) ? shlexQuote(cmd.substr(i, e - i))
                                         : shlexQuote(it->second);
                i = e;
                continue;
            }
        }
        out += cmd[i++];
    }
    return out;
}

}  // namespace vnb
