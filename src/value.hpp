#pragma once

#include <cmath>
#include <cstdio>
#include <functional>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

#include "error.hpp"

namespace kestral {

struct Value;
struct ListObj;
struct MapObj;
struct FunctionObj;
struct BuiltinObj;
struct Env;
struct Interp;

struct NothingVal {
    bool operator==(const NothingVal&) const { return true; }
};

struct RangeVal {
    long long start = 0;
    long long end = 0;
    bool operator==(const RangeVal& o) const { return start == o.start && end == o.end; }
    long long size() const { return std::abs(end - start) + 1; }
    bool contains(long long v) const {
        return start <= end ? (v >= start && v <= end) : (v <= start && v >= end);
    }
    long long at(long long idx) const {
        long long n = size();
        if (idx < 0) idx += n;
        if (idx < 0 || idx >= n) {
            throw KestralError("Position " + std::to_string(idx) +
                               " is outside this range (length " + std::to_string(n) + ").");
        }
        return start <= end ? (start + idx) : (start - idx);
    }
};

struct ListObj {
    std::vector<Value> items;
};

struct MapObj {
    // Insertion-ordered storage so keys(), entries(), and formatting are deterministic.
    std::vector<std::pair<std::string, Value>> entries;
    std::unordered_map<std::string, std::size_t> index;

    bool has(const std::string& k) const { return index.find(k) != index.end(); }

    Value get(const std::string& k) const;
    void set(const std::string& k, Value v);
    Value remove_key(const std::string& k);
};

struct ParamSpec;
struct Stmt;
struct Expr;

struct FunctionObj {
    std::string name;
    std::vector<std::string> param_names;
    std::vector<std::shared_ptr<Expr>> param_defaults;
    std::vector<std::shared_ptr<Stmt>> body;
    std::shared_ptr<Env> closure;
};

struct BuiltinObj {
    std::string name;
    std::function<Value(Interp&, const std::vector<Value>&, int, int)> fn;
};

struct Value {
    using Storage = std::variant<
        NothingVal,
        bool,
        double,
        std::string,
        RangeVal,
        std::shared_ptr<ListObj>,
        std::shared_ptr<MapObj>,
        std::shared_ptr<FunctionObj>,
        std::shared_ptr<BuiltinObj>>;

    Storage data;

    Value() : data(NothingVal{}) {}
    Value(NothingVal n) : data(n) {}
    Value(bool b) : data(b) {}
    Value(int i) : data(static_cast<double>(i)) {}
    Value(long long l) : data(static_cast<double>(l)) {}
    Value(double d) : data(d) {}
    Value(const char* s) : data(std::string(s)) {}
    Value(std::string s) : data(std::move(s)) {}
    Value(RangeVal r) : data(r) {}
    Value(std::shared_ptr<ListObj> l) : data(std::move(l)) {}
    Value(std::shared_ptr<MapObj> m) : data(std::move(m)) {}
    Value(std::shared_ptr<FunctionObj> f) : data(std::move(f)) {}
    Value(std::shared_ptr<BuiltinObj> b) : data(std::move(b)) {}

    bool is_nothing() const { return std::holds_alternative<NothingVal>(data); }
    bool is_bool() const { return std::holds_alternative<bool>(data); }
    bool is_number() const { return std::holds_alternative<double>(data); }
    bool is_text() const { return std::holds_alternative<std::string>(data); }
    bool is_range() const { return std::holds_alternative<RangeVal>(data); }
    bool is_list() const { return std::holds_alternative<std::shared_ptr<ListObj>>(data); }
    bool is_map() const { return std::holds_alternative<std::shared_ptr<MapObj>>(data); }
    bool is_function() const { return std::holds_alternative<std::shared_ptr<FunctionObj>>(data); }
    bool is_builtin() const { return std::holds_alternative<std::shared_ptr<BuiltinObj>>(data); }
    bool is_callable() const { return is_function() || is_builtin(); }

    double as_number() const { return std::get<double>(data); }
    bool as_bool() const { return std::get<bool>(data); }
    const std::string& as_text() const { return std::get<std::string>(data); }
    const RangeVal& as_range() const { return std::get<RangeVal>(data); }
    const std::shared_ptr<ListObj>& as_list() const {
        return std::get<std::shared_ptr<ListObj>>(data);
    }
    const std::shared_ptr<MapObj>& as_map() const {
        return std::get<std::shared_ptr<MapObj>>(data);
    }
};

inline Value MapObj::get(const std::string& k) const {
    auto it = index.find(k);
    if (it == index.end()) return Value(NothingVal{});
    return entries[it->second].second;
}

inline void MapObj::set(const std::string& k, Value v) {
    auto it = index.find(k);
    if (it != index.end()) {
        entries[it->second].second = std::move(v);
    } else {
        index[k] = entries.size();
        entries.emplace_back(k, std::move(v));
    }
}

inline Value MapObj::remove_key(const std::string& k) {
    auto it = index.find(k);
    if (it == index.end()) return Value(NothingVal{});
    std::size_t pos = it->second;
    Value old = entries[pos].second;
    entries.erase(entries.begin() + static_cast<std::ptrdiff_t>(pos));
    index.clear();
    for (std::size_t i = 0; i < entries.size(); ++i) index[entries[i].first] = i;
    return old;
}

inline std::string type_name(const Value& v) {
    if (v.is_nothing()) return "nothing";
    if (v.is_bool()) return "bool";
    if (v.is_number()) return "number";
    if (v.is_text()) return "text";
    if (v.is_list()) return "list";
    if (v.is_map()) return "map";
    if (v.is_range()) return "range";
    if (v.is_callable()) return "function";
    return "unknown";
}

inline bool is_truthy(const Value& v) {
    if (v.is_nothing()) return false;
    if (v.is_bool()) return v.as_bool();
    if (v.is_number()) return v.as_number() != 0.0;
    if (v.is_text()) return !v.as_text().empty();
    return true; // list, map, range, function are truthy even if empty
}

inline bool is_whole_number(double x) {
    return std::isfinite(x) && std::floor(x) == x && std::abs(x) <= 9007199254740992.0;
}

inline std::string tidy_number(double x) {
    if (std::isnan(x)) return "NaN";
    if (!std::isfinite(x)) return (x < 0) ? "-Infinity" : "Infinity";
    if (x == 0.0) return "0";
    // Match 12 significant digits precision so 0.1 + 0.2 -> 0.3, 1/3 -> 0.333333333333
    double abs_x = std::abs(x);
    if (abs_x >= 1e-6 && abs_x < 1e15) {
        char buf[64];
        std::snprintf(buf, sizeof(buf), "%.12g", x);
        // %.12g prints integer part if whole, strip trailing zeros on decimals.
        std::string s(buf);
        if (s == "-0") return "0";
        return s;
    }
    std::ostringstream oss;
    oss << x;
    return oss.str();
}

inline double floored_mod(double a, double b) {
    double r = std::fmod(a, b);
    if (r != 0.0 && ((r < 0.0) != (b < 0.0))) r += b;
    if (r == 0.0) r = 0.0;
    return r;
}

inline bool is_simple_ident(const std::string& k) {
    if (k.empty()) return false;
    unsigned char first = static_cast<unsigned char>(k[0]);
    if (!((first >= 'a' && first <= 'z') || (first >= 'A' && first <= 'Z') || first == '_')) return false;
    for (std::size_t i = 1; i < k.size(); ++i) {
        unsigned char c = static_cast<unsigned char>(k[i]);
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_')) {
            return false;
        }
    }
    return true;
}

inline std::string quote_text(const std::string& s) {
    std::string out = "\"";
    for (char c : s) {
        if (c == '\\') out += "\\\\";
        else if (c == '"') out += "\\\"";
        else if (c == '\n') out += "\\n";
        else if (c == '\t') out += "\\t";
        else out += c;
    }
    out += '"';
    return out;
}

inline std::string format_value(const Value& v, bool nested = false) {
    if (v.is_nothing()) return "nothing";
    if (v.is_bool()) return v.as_bool() ? "true" : "false";
    if (v.is_number()) return tidy_number(v.as_number());
    if (v.is_text()) return nested ? quote_text(v.as_text()) : v.as_text();
    if (v.is_range()) {
        const auto& r = v.as_range();
        return std::to_string(r.start) + ".." + std::to_string(r.end);
    }
    if (v.is_list()) {
        const auto& xs = v.as_list()->items;
        std::string out = "[";
        for (std::size_t i = 0; i < xs.size(); ++i) {
            if (i > 0) out += ", ";
            out += format_value(xs[i], true);
        }
        out += "]";
        return out;
    }
    if (v.is_map()) {
        const auto& mp = v.as_map()->entries;
        std::string out = "{";
        for (std::size_t i = 0; i < mp.size(); ++i) {
            if (i > 0) out += ", ";
            const std::string& k = mp[i].first;
            out += is_simple_ident(k) ? k : quote_text(k);
            out += ": ";
            out += format_value(mp[i].second, true);
        }
        out += "}";
        return out;
    }
    if (v.is_function()) {
        const auto& fn = std::get<std::shared_ptr<FunctionObj>>(v.data);
        return "<function " + (fn->name.empty() ? "anonymous" : fn->name) + ">";
    }
    if (v.is_builtin()) {
        const auto& b = std::get<std::shared_ptr<BuiltinObj>>(v.data);
        return "<function " + b->name + ">";
    }
    return "nothing";
}

inline bool deep_equals(const Value& a, const Value& b) {
    if (a.data.index() != b.data.index()) return false;
    if (a.is_nothing()) return true;
    if (a.is_bool()) return a.as_bool() == b.as_bool();
    if (a.is_number()) return a.as_number() == b.as_number();
    if (a.is_text()) return a.as_text() == b.as_text();
    if (a.is_range()) return a.as_range() == b.as_range();
    if (a.is_list()) {
        const auto& la = a.as_list()->items;
        const auto& lb = b.as_list()->items;
        if (la.size() != lb.size()) return false;
        for (std::size_t i = 0; i < la.size(); ++i) {
            if (!deep_equals(la[i], lb[i])) return false;
        }
        return true;
    }
    if (a.is_map()) {
        const auto& ma = a.as_map();
        const auto& mb = b.as_map();
        if (ma->entries.size() != mb->entries.size()) return false;
        for (const auto& kv : ma->entries) {
            if (!mb->has(kv.first)) return false;
            if (!deep_equals(kv.second, mb->get(kv.first))) return false;
        }
        return true;
    }
    if (a.is_function()) {
        return std::get<std::shared_ptr<FunctionObj>>(a.data) ==
               std::get<std::shared_ptr<FunctionObj>>(b.data);
    }
    if (a.is_builtin()) {
        return std::get<std::shared_ptr<BuiltinObj>>(a.data) ==
               std::get<std::shared_ptr<BuiltinObj>>(b.data);
    }
    return false;
}

inline std::string map_key_from_value(const Value& v, int line = 0, int col = 0) {
    if (v.is_text()) return v.as_text();
    if (v.is_number()) return tidy_number(v.as_number());
    if (v.is_bool()) return v.as_bool() ? "true" : "false";
    throw KestralError("Map keys must be text, numbers, or bools (got " + type_name(v) + ").",
                       line, col);
}

struct Env {
    std::shared_ptr<Env> parent;
    std::unordered_map<std::string, Value> vars;
    std::vector<std::string> insertion_order;

    explicit Env(std::shared_ptr<Env> p = nullptr) : parent(std::move(p)) {}

    bool has_local(const std::string& name) const {
        return vars.find(name) != vars.end();
    }

    Value get(const std::string& name, int line, int col,
              const std::vector<std::string>* extra_names = nullptr) const {
        for (const Env* e = this; e != nullptr; e = e->parent.get()) {
            auto it = e->vars.find(name);
            if (it != e->vars.end()) return it->second;
        }
        std::vector<std::string> pool;
        for (const Env* e = this; e != nullptr; e = e->parent.get()) {
            for (const auto& kv : e->vars) pool.push_back(kv.first);
        }
        if (extra_names) pool.insert(pool.end(), extra_names->begin(), extra_names->end());
        std::string hint = did_you_mean(name, pool);
        std::string msg = "I don't know \"" + name + "\".";
        if (!hint.empty()) {
            msg += " Did you mean \"" + hint + "\"?";
        } else {
            msg += " Create it first with: let " + name + " = ...";
        }
        throw KestralError(msg, line, col);
    }

    void define(const std::string& name, Value val) {
        if (vars.find(name) == vars.end()) insertion_order.push_back(name);
        vars[name] = std::move(val);
    }

    void assign(const std::string& name, Value val, int line, int col) {
        for (Env* e = this; e != nullptr; e = e->parent.get()) {
            auto it = e->vars.find(name);
            if (it != e->vars.end()) {
                it->second = std::move(val);
                return;
            }
        }
        throw KestralError("\"" + name + "\" doesn't exist yet. Create it first with: let " +
                               name + " = ...",
                           line, col);
    }
};

struct ReturnSig {
    Value value;
};
struct StopSig {};
struct SkipSig {};

} // namespace kestral
