#pragma once
// VenimBuild (*.vnb) recipe parser: tokenizer + recursive-descent parser.
#include <map>
#include <string>
#include <vector>

namespace vnb {

struct Value {
    enum class Type { STR, LIST, MAP };
    Type type = Type::STR;
    std::string s;
    std::vector<std::string> list;
    std::map<std::string, Value> map;

    static Value str(const std::string& v) { Value x; x.type = Type::STR; x.s = v; return x; }
    static Value listOf(const std::vector<std::string>& v) { Value x; x.type = Type::LIST; x.list = v; return x; }
    bool isStr() const { return type == Type::STR; }
    bool isList() const { return type == Type::LIST; }
    bool isMap() const { return type == Type::MAP; }
};

struct Recipe {
    std::string name;
    std::map<std::string, Value> fields;

    bool has(const std::string& k) const { return fields.count(k) != 0; }
    const Value* get(const std::string& k) const {
        auto it = fields.find(k);
        return it == fields.end() ? nullptr : &it->second;
    }
    std::string getStr(const std::string& k, const std::string& dflt = "") const {
        const Value* v = get(k);
        return (v && v->isStr()) ? v->s : dflt;
    }
    // commands: list form, or dict form with _cmds (defensive, like the Python impl)
    std::vector<std::string> getCmds(const std::string& k) const {
        const Value* v = get(k);
        if (!v) return {};
        if (v->isList()) return v->list;
        if (v->isMap()) {
            auto it = v->map.find("_cmds");
            if (it != v->map.end() && it->second.isList()) return it->second.list;
        }
        return {};
    }
    // depends-style: list form, or dict flattened (lists extended, strings appended)
    std::vector<std::string> getStrList(const std::string& k) const {
        const Value* v = get(k);
        if (!v) return {};
        if (v->isList()) return v->list;
        if (v->isMap()) {
            std::vector<std::string> out;
            for (const auto& [_, x] : v->map) {
                if (x.isList()) out.insert(out.end(), x.list.begin(), x.list.end());
                else if (x.isStr()) out.push_back(x.s);
            }
            return out;
        }
        if (v->isStr()) return {v->s};
        return {};
    }
};

struct Token {
    std::string kind;  // "STR", "ID", "{", "}", "="
    std::string text;
};

std::vector<Token> tokenize(const std::string& text);
Recipe parseRecipe(const std::string& text);
Recipe parseFile(const std::string& path);

// ${var} / $var expansion with shlex.quote semantics
std::string shlexQuote(const std::string& s);
std::string expandVars(const std::string& cmd, const std::map<std::string, std::string>& env);

}  // namespace vnb
