#include "builtins.hpp"
#include "error.hpp"
#include "value.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>
#include <sstream>

namespace kestral {

namespace {

std::mt19937_64& rng_engine() {
    static std::mt19937_64 gen(std::random_device{}());
    return gen;
}

void need_args(const std::string& name, const std::vector<Value>& args,
               std::size_t min_n, std::size_t max_n, int line, int col) {
    if (args.size() < min_n || args.size() > max_n) {
        throw KestralError(name + "() needs " + std::to_string(min_n) +
                               (min_n == 1 ? " value" : " values") +
                               ", got " + std::to_string(args.size()) + ".",
                           line, col);
    }
}

double need_num(const std::string& name, const Value& v, int line, int col) {
    if (!v.is_number()) {
        throw KestralError(name + "() needs a number, got " + type_name(v) + ".", line, col);
    }
    return v.as_number();
}

const std::string& need_text(const std::string& name, const Value& v, int line, int col) {
    if (!v.is_text()) {
        throw KestralError(name + "() needs text, got " + type_name(v) + ".", line, col);
    }
    return v.as_text();
}

std::vector<Value> to_iterable_items(const std::string& name, const Value& v, int line, int col) {
    if (v.is_list()) return v.as_list()->items;
    if (v.is_range()) {
        const auto& r = v.as_range();
        std::vector<Value> xs;
        long long n = r.size();
        xs.reserve(static_cast<std::size_t>(n));
        long long step = r.start <= r.end ? 1 : -1;
        for (long long i = 0; i < n; ++i) xs.emplace_back(static_cast<double>(r.start + i * step));
        return xs;
    }
    throw KestralError(name + "() needs a list or range, got " + type_name(v) + ".", line, col);
}

Value deep_copy(const Value& v) {
    if (v.is_list()) {
        auto out = std::make_shared<ListObj>();
        for (const auto& item : v.as_list()->items) out->items.push_back(deep_copy(item));
        return Value(out);
    }
    if (v.is_map()) {
        auto out = std::make_shared<MapObj>();
        for (const auto& kv : v.as_map()->entries) out->set(kv.first, deep_copy(kv.second));
        return Value(out);
    }
    return v;
}

std::string trim_ascii(const std::string& s) {
    std::size_t i = 0, j = s.size();
    while (i < j && std::isspace(static_cast<unsigned char>(s[i]))) i++;
    while (j > i && std::isspace(static_cast<unsigned char>(s[j - 1]))) j--;
    return s.substr(i, j - i);
}

} // namespace

void register_builtins(Interp& interp) {
    auto add = [&](const std::string& name,
                   std::function<Value(Interp&, const std::vector<Value>&, int, int)> fn) {
        auto b = std::make_shared<BuiltinObj>();
        b->name = name;
        b->fn = std::move(fn);
        interp.global_env->define(name, Value(b));
        interp.builtin_names.push_back(name);
    };

    /* ---- Input & conversion ---- */
    add("ask", [](Interp& ip, const std::vector<Value>& a, int line, int col) -> Value {
        need_args("ask", a, 0, 1, line, col);
        std::string prompt = a.empty() ? "" : format_value(a[0], false);
        if (ip.cfg.ask_fn) return ip.cfg.ask_fn(prompt);
        if (!prompt.empty()) {
            std::cout << prompt << std::flush;
        }
        std::string ln;
        if (!std::getline(std::cin, ln)) return Value();
        if (!ln.empty() && ln.back() == '\r') ln.pop_back();
        return Value(ln);
    });

    add("type", [](Interp&, const std::vector<Value>& a, int line, int col) -> Value {
        need_args("type", a, 1, 1, line, col);
        return Value(type_name(a[0]));
    });

    add("str", [](Interp&, const std::vector<Value>& a, int line, int col) -> Value {
        need_args("str", a, 1, 1, line, col);
        return Value(format_value(a[0], false));
    });

    add("num", [](Interp&, const std::vector<Value>& a, int line, int col) -> Value {
        need_args("num", a, 1, 1, line, col);
        if (a[0].is_number()) return a[0];
        if (a[0].is_text()) {
            std::string s = trim_ascii(a[0].as_text());
            if (s.empty()) throw KestralError("I can't turn \"\" into a number.", line, col);
            char* endp = nullptr;
            double v = std::strtod(s.c_str(), &endp);
            if (endp == s.c_str() || *endp != '\0' || !std::isfinite(v)) {
                throw KestralError("I can't turn \"" + a[0].as_text() + "\" into a number.", line, col);
            }
            return Value(v);
        }
        throw KestralError("I can't turn " + type_name(a[0]) + " into a number.", line, col);
    });

    add("int", [](Interp&, const std::vector<Value>& a, int line, int col) -> Value {
        need_args("int", a, 1, 1, line, col);
        double d = 0.0;
        if (a[0].is_number()) d = a[0].as_number();
        else if (a[0].is_text()) {
            std::string s = trim_ascii(a[0].as_text());
            char* endp = nullptr;
            d = std::strtod(s.c_str(), &endp);
            if (s.empty() || endp == s.c_str() || *endp != '\0' || !std::isfinite(d)) {
                throw KestralError("I can't turn \"" + a[0].as_text() + "\" into an integer.", line, col);
            }
        } else {
            throw KestralError("int() needs a number or text.", line, col);
        }
        return Value(std::trunc(d));
    });

    add("list", [](Interp&, const std::vector<Value>& a, int line, int col) -> Value {
        need_args("list", a, 1, 1, line, col);
        auto out = std::make_shared<ListObj>();
        if (a[0].is_range()) {
            out->items = to_iterable_items("list", a[0], line, col);
            return Value(out);
        }
        if (a[0].is_text()) {
            for (char c : a[0].as_text()) out->items.emplace_back(std::string(1, c));
            return Value(out);
        }
        if (a[0].is_map()) {
            for (const auto& kv : a[0].as_map()->entries) out->items.emplace_back(kv.first);
            return Value(out);
        }
        if (a[0].is_list()) {
            out->items = a[0].as_list()->items;
            return Value(out);
        }
        throw KestralError("list() needs a range, text, list, or map.", line, col);
    });

    add("copy", [](Interp&, const std::vector<Value>& a, int line, int col) -> Value {
        need_args("copy", a, 1, 1, line, col);
        return deep_copy(a[0]);
    });

    /* ---- Math ---- */
    add("abs", [](Interp&, const std::vector<Value>& a, int line, int col) -> Value {
        need_args("abs", a, 1, 1, line, col);
        return Value(std::abs(need_num("abs", a[0], line, col)));
    });
    add("floor", [](Interp&, const std::vector<Value>& a, int line, int col) -> Value {
        need_args("floor", a, 1, 1, line, col);
        return Value(std::floor(need_num("floor", a[0], line, col)));
    });
    add("ceil", [](Interp&, const std::vector<Value>& a, int line, int col) -> Value {
        need_args("ceil", a, 1, 1, line, col);
        return Value(std::ceil(need_num("ceil", a[0], line, col)));
    });
    add("sqrt", [](Interp&, const std::vector<Value>& a, int line, int col) -> Value {
        need_args("sqrt", a, 1, 1, line, col);
        double d = need_num("sqrt", a[0], line, col);
        if (d < 0.0) throw KestralError("Cannot take the square root of a negative number.", line, col);
        return Value(std::sqrt(d));
    });
    add("round", [](Interp&, const std::vector<Value>& a, int line, int col) -> Value {
        need_args("round", a, 1, 2, line, col);
        double x = need_num("round", a[0], line, col);
        double digits = (a.size() == 2) ? need_num("round", a[1], line, col) : 0.0;
        if (!is_whole_number(digits)) throw KestralError("round() digits must be a whole number.", line, col);
        double factor = std::pow(10.0, digits);
        double scaled = x * factor;
        double rounded = (scaled >= 0.0) ? std::floor(scaled + 0.5 + 1e-12)
                                         : std::ceil(scaled - 0.5 - 1e-12);
        double out = rounded / factor;
        if (out == 0.0) out = 0.0;
        return Value(out);
    });
    add("min", [](Interp&, const std::vector<Value>& a, int line, int col) -> Value {
        need_args("min", a, 1, 9999, line, col);
        std::vector<Value> pool = (a.size() == 1 && (a[0].is_list() || a[0].is_range()))
                                      ? to_iterable_items("min", a[0], line, col)
                                      : a;
        if (pool.empty()) throw KestralError("min() of an empty list.", line, col);
        double best = need_num("min", pool[0], line, col);
        for (std::size_t i = 1; i < pool.size(); ++i) best = std::min(best, need_num("min", pool[i], line, col));
        return Value(best);
    });
    add("max", [](Interp&, const std::vector<Value>& a, int line, int col) -> Value {
        need_args("max", a, 1, 9999, line, col);
        std::vector<Value> pool = (a.size() == 1 && (a[0].is_list() || a[0].is_range()))
                                      ? to_iterable_items("max", a[0], line, col)
                                      : a;
        if (pool.empty()) throw KestralError("max() of an empty list.", line, col);
        double best = need_num("max", pool[0], line, col);
        for (std::size_t i = 1; i < pool.size(); ++i) best = std::max(best, need_num("max", pool[i], line, col));
        return Value(best);
    });
    add("sum", [](Interp&, const std::vector<Value>& a, int line, int col) -> Value {
        need_args("sum", a, 1, 1, line, col);
        auto pool = to_iterable_items("sum", a[0], line, col);
        double tot = 0.0;
        for (const auto& v : pool) tot += need_num("sum", v, line, col);
        return Value(tot);
    });
    add("random", [](Interp&, const std::vector<Value>& a, int line, int col) -> Value {
        need_args("random", a, 0, 0, line, col);
        std::uniform_real_distribution<double> dist(0.0, 1.0);
        return Value(dist(rng_engine()));
    });
    add("randomInt", [](Interp&, const std::vector<Value>& a, int line, int col) -> Value {
        need_args("randomInt", a, 2, 2, line, col);
        double lo = need_num("randomInt", a[0], line, col);
        double hi = need_num("randomInt", a[1], line, col);
        if (!is_whole_number(lo) || !is_whole_number(hi)) {
            throw KestralError("randomInt() bounds must be whole numbers.", line, col);
        }
        long long l = static_cast<long long>(lo), h = static_cast<long long>(hi);
        if (l > h) throw KestralError("randomInt() low cannot be greater than high.", line, col);
        std::uniform_int_distribution<long long> dist(l, h);
        return Value(dist(rng_engine()));
    });

    /* ---- Lists, maps, ranges ---- */
    add("len", [](Interp&, const std::vector<Value>& a, int line, int col) -> Value {
        need_args("len", a, 1, 1, line, col);
        if (a[0].is_list()) return Value(static_cast<long long>(a[0].as_list()->items.size()));
        if (a[0].is_text()) return Value(static_cast<long long>(a[0].as_text().size()));
        if (a[0].is_map()) return Value(static_cast<long long>(a[0].as_map()->entries.size()));
        if (a[0].is_range()) return Value(a[0].as_range().size());
        throw KestralError("len() does not work on " + type_name(a[0]) + ".", line, col);
    });

    add("push", [](Interp&, const std::vector<Value>& a, int line, int col) -> Value {
        need_args("push", a, 1, 9999, line, col);
        if (!a[0].is_list()) throw KestralError("push() expects a list first.", line, col);
        auto& xs = a[0].as_list()->items;
        for (std::size_t i = 1; i < a.size(); ++i) xs.push_back(a[i]);
        return a[0];
    });

    add("pop", [](Interp&, const std::vector<Value>& a, int line, int col) -> Value {
        need_args("pop", a, 1, 1, line, col);
        if (!a[0].is_list()) throw KestralError("pop() expects a list.", line, col);
        auto& xs = a[0].as_list()->items;
        if (xs.empty()) throw KestralError("Cannot pop() from an empty list.", line, col);
        Value back = xs.back();
        xs.pop_back();
        return back;
    });

    add("remove", [](Interp&, const std::vector<Value>& a, int line, int col) -> Value {
        need_args("remove", a, 2, 2, line, col);
        if (a[0].is_list()) {
            double idx_d = need_num("remove", a[1], line, col);
            if (!is_whole_number(idx_d)) throw KestralError("remove() position must be a whole number.", line, col);
            auto& xs = a[0].as_list()->items;
            long long idx = static_cast<long long>(idx_d);
            long long n = static_cast<long long>(xs.size());
            if (idx < 0) idx += n;
            if (idx < 0 || idx >= n) throw KestralError("Position is outside this list.", line, col);
            Value val = xs[static_cast<std::size_t>(idx)];
            xs.erase(xs.begin() + static_cast<std::ptrdiff_t>(idx));
            return val;
        }
        if (a[0].is_map()) {
            std::string k = map_key_from_value(a[1], line, col);
            return a[0].as_map()->remove_key(k);
        }
        throw KestralError("remove() expects a list or map.", line, col);
    });

    add("slice", [](Interp&, const std::vector<Value>& a, int line, int col) -> Value {
        need_args("slice", a, 2, 3, line, col);
        double from_d = need_num("slice", a[1], line, col);
        if (!is_whole_number(from_d)) throw KestralError("slice() positions must be whole numbers.", line, col);
        long long n = a[0].is_list() ? static_cast<long long>(a[0].as_list()->items.size())
                                     : (a[0].is_text() ? static_cast<long long>(a[0].as_text().size()) : -1);
        if (n < 0) throw KestralError("slice() expects a list or text.", line, col);
        long long start = static_cast<long long>(from_d);
        if (start < 0) start += n;
        start = std::max(0LL, std::min(start, n));
        long long end = n;
        if (a.size() == 3 && !a[2].is_nothing()) {
            double to_d = need_num("slice", a[2], line, col);
            if (!is_whole_number(to_d)) throw KestralError("slice() positions must be whole numbers.", line, col);
            end = static_cast<long long>(to_d);
            if (end < 0) end += n;
            end = std::max(0LL, std::min(end, n));
        }
        if (a[0].is_list()) {
            auto out = std::make_shared<ListObj>();
            for (long long i = start; i < end; ++i) {
                out->items.push_back(a[0].as_list()->items[static_cast<std::size_t>(i)]);
            }
            return Value(out);
        }
        if (end <= start) return Value(std::string(""));
        return Value(a[0].as_text().substr(static_cast<std::size_t>(start),
                                           static_cast<std::size_t>(end - start)));
    });

    add("reverse", [](Interp&, const std::vector<Value>& a, int line, int col) -> Value {
        need_args("reverse", a, 1, 1, line, col);
        if (a[0].is_list()) {
            auto out = std::make_shared<ListObj>();
            out->items = a[0].as_list()->items;
            std::reverse(out->items.begin(), out->items.end());
            return Value(out);
        }
        if (a[0].is_text()) {
            std::string s = a[0].as_text();
            std::reverse(s.begin(), s.end());
            return Value(s);
        }
        throw KestralError("reverse() expects a list or text.", line, col);
    });

    add("sort", [](Interp& ip, const std::vector<Value>& a, int line, int col) -> Value {
        need_args("sort", a, 1, 2, line, col);
        std::vector<Value> xs = to_iterable_items("sort", a[0], line, col);
        bool has_key = (a.size() == 2 && !a[1].is_nothing());
        std::vector<std::pair<Value, Value>> pairs;
        pairs.reserve(xs.size());
        for (const auto& x : xs) {
            Value k = has_key ? ip.call_value(a[1], {x}, line, col) : x;
            pairs.emplace_back(std::move(k), x);
        }
        std::stable_sort(pairs.begin(), pairs.end(), [&](const auto& pa, const auto& pb) {
            const Value& ka = pa.first;
            const Value& kb = pb.first;
            if (ka.is_number() && kb.is_number()) return ka.as_number() < kb.as_number();
            if (ka.is_text() && kb.is_text()) return ka.as_text() < kb.as_text();
            if (ka.data.index() != kb.data.index()) {
                throw KestralError("Cannot sort mixed types together.", line, col);
            }
            return format_value(ka, false) < format_value(kb, false);
        });
        auto out = std::make_shared<ListObj>();
        for (auto& p : pairs) out->items.push_back(std::move(p.second));
        return Value(out);
    });

    add("unique", [](Interp&, const std::vector<Value>& a, int line, int col) -> Value {
        need_args("unique", a, 1, 1, line, col);
        auto xs = to_iterable_items("unique", a[0], line, col);
        auto out = std::make_shared<ListObj>();
        for (const auto& item : xs) {
            bool dup = false;
            for (const auto& prev : out->items) {
                if (deep_equals(prev, item)) { dup = true; break; }
            }
            if (!dup) out->items.push_back(item);
        }
        return Value(out);
    });

    add("keys", [](Interp&, const std::vector<Value>& a, int line, int col) -> Value {
        need_args("keys", a, 1, 1, line, col);
        if (!a[0].is_map()) throw KestralError("keys() expects a map.", line, col);
        auto out = std::make_shared<ListObj>();
        for (const auto& kv : a[0].as_map()->entries) out->items.emplace_back(kv.first);
        return Value(out);
    });

    add("values", [](Interp&, const std::vector<Value>& a, int line, int col) -> Value {
        need_args("values", a, 1, 1, line, col);
        if (!a[0].is_map()) throw KestralError("values() expects a map.", line, col);
        auto out = std::make_shared<ListObj>();
        for (const auto& kv : a[0].as_map()->entries) out->items.push_back(kv.second);
        return Value(out);
    });

    add("entries", [](Interp&, const std::vector<Value>& a, int line, int col) -> Value {
        need_args("entries", a, 1, 1, line, col);
        if (!a[0].is_map()) throw KestralError("entries() expects a map.", line, col);
        auto out = std::make_shared<ListObj>();
        for (const auto& kv : a[0].as_map()->entries) {
            auto pair = std::make_shared<ListObj>();
            pair->items.emplace_back(kv.first);
            pair->items.push_back(kv.second);
            out->items.emplace_back(pair);
        }
        return Value(out);
    });

    add("contains", [](Interp&, const std::vector<Value>& a, int line, int col) -> Value {
        need_args("contains", a, 2, 2, line, col);
        if (a[0].is_text()) {
            const std::string& sub = need_text("contains", a[1], line, col);
            return Value(a[0].as_text().find(sub) != std::string::npos);
        }
        if (a[0].is_list()) {
            for (const auto& x : a[0].as_list()->items) {
                if (deep_equals(x, a[1])) return Value(true);
            }
            return Value(false);
        }
        if (a[0].is_map()) {
            std::string k = map_key_from_value(a[1], line, col);
            return Value(a[0].as_map()->has(k));
        }
        if (a[0].is_range()) {
            if (!a[1].is_number() || !is_whole_number(a[1].as_number())) return Value(false);
            return Value(a[0].as_range().contains(static_cast<long long>(a[1].as_number())));
        }
        throw KestralError("contains() does not work on " + type_name(a[0]) + ".", line, col);
    });

    add("find", [](Interp&, const std::vector<Value>& a, int line, int col) -> Value {
        need_args("find", a, 2, 2, line, col);
        if (a[0].is_text()) {
            const std::string& sub = need_text("find", a[1], line, col);
            auto pos = a[0].as_text().find(sub);
            return Value(pos == std::string::npos ? -1LL : static_cast<long long>(pos));
        }
        if (a[0].is_list()) {
            const auto& xs = a[0].as_list()->items;
            for (std::size_t i = 0; i < xs.size(); ++i) {
                if (deep_equals(xs[i], a[1])) return Value(static_cast<long long>(i));
            }
            return Value(-1LL);
        }
        throw KestralError("find() expects text or a list.", line, col);
    });

    /* ---- Functional ---- */
    add("map", [](Interp& ip, const std::vector<Value>& a, int line, int col) -> Value {
        need_args("map", a, 2, 2, line, col);
        auto xs = to_iterable_items("map", a[0], line, col);
        std::size_t param_cnt = a[1].is_function()
                                    ? std::get<std::shared_ptr<FunctionObj>>(a[1].data)->param_names.size()
                                    : 1;
        auto out = std::make_shared<ListObj>();
        out->items.reserve(xs.size());
        for (std::size_t i = 0; i < xs.size(); ++i) {
            if (param_cnt >= 2) {
                out->items.push_back(ip.call_value(a[1], {xs[i], Value(static_cast<long long>(i))}, line, col));
            } else {
                out->items.push_back(ip.call_value(a[1], {xs[i]}, line, col));
            }
        }
        return Value(out);
    });

    add("filter", [](Interp& ip, const std::vector<Value>& a, int line, int col) -> Value {
        need_args("filter", a, 2, 2, line, col);
        auto xs = to_iterable_items("filter", a[0], line, col);
        auto out = std::make_shared<ListObj>();
        for (const auto& x : xs) {
            if (is_truthy(ip.call_value(a[1], {x}, line, col))) out->items.push_back(x);
        }
        return Value(out);
    });

    add("reduce", [](Interp& ip, const std::vector<Value>& a, int line, int col) -> Value {
        need_args("reduce", a, 2, 3, line, col);
        auto xs = to_iterable_items("reduce", a[0], line, col);
        std::size_t start_i = 0;
        Value acc;
        if (a.size() == 3) {
            acc = a[2];
        } else {
            if (xs.empty()) throw KestralError("reduce() needs a start value for an empty list.", line, col);
            acc = xs[0];
            start_i = 1;
        }
        for (std::size_t i = start_i; i < xs.size(); ++i) {
            acc = ip.call_value(a[1], {acc, xs[i]}, line, col);
        }
        return acc;
    });

    add("any", [](Interp& ip, const std::vector<Value>& a, int line, int col) -> Value {
        need_args("any", a, 2, 2, line, col);
        auto xs = to_iterable_items("any", a[0], line, col);
        for (const auto& x : xs) {
            if (is_truthy(ip.call_value(a[1], {x}, line, col))) return Value(true);
        }
        return Value(false);
    });

    add("all", [](Interp& ip, const std::vector<Value>& a, int line, int col) -> Value {
        need_args("all", a, 2, 2, line, col);
        auto xs = to_iterable_items("all", a[0], line, col);
        for (const auto& x : xs) {
            if (!is_truthy(ip.call_value(a[1], {x}, line, col))) return Value(false);
        }
        return Value(true);
    });

    /* ---- Text ---- */
    add("upper", [](Interp&, const std::vector<Value>& a, int line, int col) -> Value {
        need_args("upper", a, 1, 1, line, col);
        std::string s = need_text("upper", a[0], line, col);
        for (char& c : s) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        return Value(s);
    });

    add("lower", [](Interp&, const std::vector<Value>& a, int line, int col) -> Value {
        need_args("lower", a, 1, 1, line, col);
        return Value(to_lower_ascii(need_text("lower", a[0], line, col)));
    });

    add("trim", [](Interp&, const std::vector<Value>& a, int line, int col) -> Value {
        need_args("trim", a, 1, 1, line, col);
        return Value(trim_ascii(need_text("trim", a[0], line, col)));
    });

    add("split", [](Interp&, const std::vector<Value>& a, int line, int col) -> Value {
        need_args("split", a, 1, 2, line, col);
        const std::string& s = need_text("split", a[0], line, col);
        auto out = std::make_shared<ListObj>();
        if (a.size() == 1 || a[1].is_nothing()) {
            std::istringstream iss(s);
            std::string w;
            while (iss >> w) out->items.emplace_back(w);
            return Value(out);
        }
        const std::string& sep = need_text("split", a[1], line, col);
        if (sep.empty()) {
            for (char c : s) out->items.emplace_back(std::string(1, c));
            return Value(out);
        }
        std::size_t pos = 0;
        for (;;) {
            auto found = s.find(sep, pos);
            if (found == std::string::npos) {
                out->items.emplace_back(s.substr(pos));
                break;
            }
            out->items.emplace_back(s.substr(pos, found - pos));
            pos = found + sep.size();
        }
        return Value(out);
    });

    add("join", [](Interp&, const std::vector<Value>& a, int line, int col) -> Value {
        need_args("join", a, 1, 2, line, col);
        auto xs = to_iterable_items("join", a[0], line, col);
        std::string sep = (a.size() == 2) ? need_text("join", a[1], line, col) : "";
        std::string out;
        for (std::size_t i = 0; i < xs.size(); ++i) {
            if (i > 0) out += sep;
            out += format_value(xs[i], false);
        }
        return Value(out);
    });

    add("replace", [](Interp&, const std::vector<Value>& a, int line, int col) -> Value {
        need_args("replace", a, 3, 3, line, col);
        std::string s = need_text("replace", a[0], line, col);
        const std::string& old_s = need_text("replace", a[1], line, col);
        const std::string& new_s = need_text("replace", a[2], line, col);
        if (old_s.empty()) return Value(s);
        std::size_t pos = 0;
        while ((pos = s.find(old_s, pos)) != std::string::npos) {
            s.replace(pos, old_s.size(), new_s);
            pos += new_s.size();
        }
        return Value(s);
    });

    add("startsWith", [](Interp&, const std::vector<Value>& a, int line, int col) -> Value {
        need_args("startsWith", a, 2, 2, line, col);
        const std::string& s = need_text("startsWith", a[0], line, col);
        const std::string& p = need_text("startsWith", a[1], line, col);
        return Value(s.size() >= p.size() && s.compare(0, p.size(), p) == 0);
    });

    add("endsWith", [](Interp&, const std::vector<Value>& a, int line, int col) -> Value {
        need_args("endsWith", a, 2, 2, line, col);
        const std::string& s = need_text("endsWith", a[0], line, col);
        const std::string& suf = need_text("endsWith", a[1], line, col);
        return Value(s.size() >= suf.size() && s.compare(s.size() - suf.size(), suf.size(), suf) == 0);
    });

    add("repeat", [](Interp&, const std::vector<Value>& a, int line, int col) -> Value {
        need_args("repeat", a, 2, 2, line, col);
        const std::string& s = need_text("repeat", a[0], line, col);
        double n = need_num("repeat", a[1], line, col);
        if (!is_whole_number(n) || n < 0) throw KestralError("repeat() count must be a non-negative whole number.", line, col);
        std::string out;
        for (long long i = 0; i < static_cast<long long>(n); ++i) out += s;
        return Value(out);
    });

    /* ---- Checking ---- */
    add("assert", [](Interp&, const std::vector<Value>& a, int line, int col) -> Value {
        need_args("assert", a, 1, 2, line, col);
        if (!is_truthy(a[0])) {
            std::string msg = (a.size() == 2) ? format_value(a[1], false) : "Assertion failed.";
            throw KestralError(msg, line, col);
        }
        return Value();
    });

    /* ---- CLI & TUI extras ---- */
    add("args", [](Interp& ip, const std::vector<Value>& a, int line, int col) -> Value {
        need_args("args", a, 0, 0, line, col);
        auto out = std::make_shared<ListObj>();
        for (const auto& s : ip.cfg.cli_args) out->items.emplace_back(s);
        return Value(out);
    });

    add("env", [](Interp&, const std::vector<Value>& a, int line, int col) -> Value {
        need_args("env", a, 1, 1, line, col);
        const std::string& k = need_text("env", a[0], line, col);
        const char* val = std::getenv(k.c_str());
        return val ? Value(std::string(val)) : Value();
    });

    add("fileExists", [](Interp&, const std::vector<Value>& a, int line, int col) -> Value {
        need_args("fileExists", a, 1, 1, line, col);
        return Value(std::filesystem::exists(need_text("fileExists", a[0], line, col)));
    });

    add("readFile", [](Interp&, const std::vector<Value>& a, int line, int col) -> Value {
        need_args("readFile", a, 1, 1, line, col);
        const std::string& p = need_text("readFile", a[0], line, col);
        std::ifstream f(p, std::ios::binary);
        if (!f) throw KestralError("Could not read file \"" + p + "\".", line, col);
        std::ostringstream ss;
        ss << f.rdbuf();
        return Value(ss.str());
    });

    add("writeFile", [](Interp&, const std::vector<Value>& a, int line, int col) -> Value {
        need_args("writeFile", a, 2, 2, line, col);
        const std::string& p = need_text("writeFile", a[0], line, col);
        std::string data = format_value(a[1], false);
        std::ofstream f(p, std::ios::binary);
        if (!f) throw KestralError("Could not write file \"" + p + "\".", line, col);
        f << data;
        return Value(true);
    });

    add("box", [](Interp&, const std::vector<Value>& a, int line, int col) -> Value {
        need_args("box", a, 1, 2, line, col);
        std::string txt = format_value(a[0], false);
        std::string title = (a.size() == 2) ? format_value(a[1], false) : "";
        std::vector<std::string> rows;
        std::istringstream iss(txt);
        std::string ln;
        while (std::getline(iss, ln)) {
            if (!ln.empty() && ln.back() == '\r') ln.pop_back();
            rows.push_back(ln);
        }
        if (rows.empty()) rows.push_back("");
        std::size_t w = 0;
        for (const auto& r : rows) w = std::max(w, r.size());
        std::string top = "╭";
        if (!title.empty()) {
            std::string t = " " + title + " ";
            w = std::max(w, t.size());
            top += t;
            for (std::size_t i = t.size(); i < w + 2; ++i) top += "─";
        } else {
            for (std::size_t i = 0; i < w + 2; ++i) top += "─";
        }
        top += "╮\n";
        std::string body;
        for (const auto& r : rows) {
            body += "│ " + r + std::string(w - r.size(), ' ') + " │\n";
        }
        std::string bot = "╰";
        for (std::size_t i = 0; i < w + 2; ++i) bot += "─";
        bot += "╯";
        return Value(top + body + bot);
    });

    add("color", [](Interp&, const std::vector<Value>& a, int line, int col) -> Value {
        need_args("color", a, 2, 2, line, col);
        std::string txt = format_value(a[0], false);
        std::string cname = to_lower_ascii(need_text("color", a[1], line, col));
        std::string code;
        if (cname == "red") code = "91";
        else if (cname == "green") code = "92";
        else if (cname == "yellow") code = "93";
        else if (cname == "blue") code = "94";
        else if (cname == "magenta") code = "95";
        else if (cname == "cyan") code = "96";
        else if (cname == "white") code = "97";
        else if (cname == "gray" || cname == "grey") code = "90";
        else if (cname == "bold") code = "1";
        else if (cname == "dim") code = "2";
        else throw KestralError("Unknown color \"" + cname + "\".", line, col);
        return Value("\x1b[" + code + "m" + txt + "\x1b[0m");
    });

    add("clear", [](Interp&, const std::vector<Value>& a, int line, int col) -> Value {
        need_args("clear", a, 0, 0, line, col);
        std::cout << "\x1b[2J\x1b[H" << std::flush;
        return Value();
    });
}

} // namespace kestral
