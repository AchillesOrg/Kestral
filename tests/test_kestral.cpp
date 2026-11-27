#include "banner.hpp"
#include "error.hpp"
#include "interp.hpp"
#include "learn.hpp"
#include "nana.hpp"
#include "parser.hpp"
#include "version.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;
using namespace kestral;

namespace {

int g_pass = 0;
int g_fail = 0;
std::vector<std::string> g_failures;

void check(const std::string& name, bool ok, const std::string& detail = "") {
    if (ok) g_pass++;
    else {
        g_fail++;
        g_failures.push_back("FAIL  " + name + (detail.empty() ? "" : ("\n      " + detail)));
    }
}

void test_out(const std::string& name, const std::string& src, const std::string& expected,
              const std::vector<std::string>& inputs = {}) {
    auto r = run_captured(src, inputs);
    check(name, r.ok && r.text == expected,
          r.ok ? ("expected " + quote_text(expected) + ", got " + quote_text(r.text))
               : ("unexpected error: " + r.error_text));
}

void test_err(const std::string& name, const std::string& src, const std::string& sub,
              int expected_line = -1, long long max_steps = 5000000000LL,
              std::size_t max_output = 2000000) {
    InterpConfig cfg;
    cfg.max_steps = max_steps;
    cfg.max_output = max_output;
    auto r = run_captured(src, {}, cfg);
    bool line_ok = (expected_line < 0) || (r.error_line == expected_line);
    bool msg_ok = !r.ok && (r.error_msg.find(sub) != std::string::npos);
    check(name, msg_ok && line_ok,
          "expected error containing '" + sub + "', got '" + r.error_msg + "' line " + std::to_string(r.error_line));
}

std::string read_file_str(const fs::path& p) {
    std::ifstream f(p, std::ios::binary);
    std::ostringstream ss;
    ss << f.rdbuf();
    std::string s = ss.str();
    while (!s.empty() && (s.back() == '\n' || s.back() == '\r')) s.pop_back();
    std::string norm;
    for (char c : s) if (c != '\r') norm += c;
    return norm;
}

} // namespace

int main() {
    enable_utf8_console();

    /* ---- Core Language Semantics Tests ---- */
    test_out("say text", "say \"hi\"", "hi");
    test_out("say multiple values", "say 1, \"a\", true, nothing", "1 a true nothing");
    test_out("say blank line", "say\nsay \"x\"", "\nx");
    test_out("interpolation", "let n = 3\nsay \"n is {n}, double {n * 2}\"", "n is 3, double 6");
    test_out("escaped brace", "say \"a \\{b\\}\"", "a {b}");
    test_out("single quotes", "say 'it works'", "it works");
    test_out("string escapes", "say \"a\\tb\\\\c\"", "a\tb\\c");
    test_out("nested quotes in interpolation", "let m = {k: \"v\"}\nsay \"got {m[\"k\"]}\"", "got v");
    test_out("arithmetic", "say 2 + 3 * 4, (2 + 3) * 4, 10 / 4, 7 % 3, 2 ** 3 ** 2", "14 20 2.5 1 512");
    test_out("floored modulo", "say -1 % 3, 5 % -3", "2 -1");
    test_out("negation and power", "say -2 ** 2, 2 ** -1", "-4 0.5");
    test_out("float tidy", "say 0.1 + 0.2, 1 / 3", "0.3 0.333333333333");
    test_out("text concat", "say \"a\" + \"b\", \"n=\" + 5, 5 + \"x\"", "ab n=5 5x");
    test_out("text repeat", "say \"ab\" * 3, 2 * \"xy\"", "ababab xyxy");
    test_out("list concat", "say [1] + [2, 3]", "[1, 2, 3]");
    test_out("comparison", "say 1 < 2, 2 <= 2, \"a\" < \"b\", 3 != 3, [1,2] == [1,2], {a:1} == {a:1}", "true true true false true true");
    test_out("equal different types", "say 1 == \"1\", nothing == false", "false false");
    test_out("logic short circuit", "say true and \"yes\", false or \"fallback\", not 0, nothing ?? \"dflt\", 0 ?? \"zero\"", "yes fallback true dflt 0");
    test_out("truthiness", "for v in [0, \"\", nothing, false, 1, \"x\", [], true] { if v { say \"T\" } else { say \"F\" } }", "F\nF\nF\nF\nT\nT\nT\nT");
    test_out("if else chain", "fn g(n) { if n > 5 { give \"big\" } else if n > 2 { give \"mid\" } else { give \"small\" } }\nsay g(9), g(3), g(0)", "big mid small");
    test_out("else on next line", "if false {\n say 1\n}\nelse {\n say 2\n}", "2");
    test_out("if as expression", "let x = if 3 > 2 { \"a\" } else { \"b\" }\nsay x", "a");
    test_out("while", "let i = 0\nwhile i < 3 { say i; i += 1 }", "0\n1\n2");
    test_out("loop times", "let c = 0\nloop 5 times { c += 1 }\nsay c", "5");
    test_out("for range ascending", "for i in 1..4 { say i }", "1\n2\n3\n4");
    test_out("for range descending", "for i in 3..1 { say i }", "3\n2\n1");
    test_out("for list with index", "for i, x in [\"a\", \"b\"] { say i, x }", "0 a\n1 b");
    test_out("for map", "for k, v in {a: 1, b: 2} { say k, v }", "a 1\nb 2");
    test_out("for text", "for ch in \"hi\" { say ch }", "h\ni");
    test_out("stop and skip", "for i in 1..10 { if i == 2 { skip }; if i == 5 { stop }; say i }", "1\n3\n4");
    test_out("stop nested inner", "for i in 1..2 { for j in 1..3 { if j == 2 { stop }; say i, j } }", "1 1\n2 1");
    test_out("function and recursion", "fn fact(n) { if n <= 1 { give 1 }; give n * fact(n - 1) }\nsay fact(10)", "3628800");
    test_out("function hoisting", "say twice(4)\nfn twice(n) { give n * 2 }", "8");
    test_out("default parameters", "fn hi(name = \"friend\", mark = \"!\") { give \"hi {name}{mark}\" }\nsay hi(), hi(\"Ada\"), hi(\"Ada\", \"?\")", "hi friend! hi Ada! hi Ada?");
    test_out("expression body", "fn sq(x) => x * x\nsay sq(7)", "49");
    test_out("give with nothing", "fn f() { give }\nsay f()", "nothing");
    test_out("implicit nothing", "fn f() { let a = 1 }\nsay f()", "nothing");
    test_out("closures keep state", "fn counter() { let n = 0; give fn() { n += 1; give n } }\nlet c = counter()\nc(); c()\nsay c()", "3");
    test_out("closures separate", "fn mk() { let n = 0; give fn() { n += 1; give n } }\nlet a = mk()\nlet b = mk()\na(); a()\nsay a(), b()", "3 1");
    test_out("lambda block body", "let f = fn(a, b) { give a + b }\nsay f(2, 3)", "5");
    test_out("functions as values", "fn add1(x) => x + 1\nlet fs = [add1, fn(x) => x * 2]\nsay fs[0](5), fs[1](5)", "6 10");
    test_out("lists index", "let a = [10, 20, 30]\nsay a[0], a[-1], a[1]", "10 30 20");
    test_out("list assign", "let a = [1, 2, 3]\na[1] = 9\na[0] += 5\nsay a", "[6, 9, 3]");
    test_out("list ref semantics", "let a = [1]\nlet b = a\npush(b, 2)\nsay a\nlet c = copy(a)\npush(c, 3)\nsay a, c", "[1, 2]\n[1, 2] [1, 2, 3]");
    test_out("push pop len", "let a = []\npush(a, 1, 2, 3)\nsay pop(a), len(a), a", "3 2 [1, 2]");
    test_out("remove list map", "let a = [5, 6, 7]\nsay remove(a, 1), a\nlet m = {x: 1}\nsay remove(m, \"x\"), m", "6 [5, 7]\n1 {}");
    test_out("slice list text", "say slice([1,2,3,4], 1, 3), slice(\"hello\", 1), slice(\"hello\", -3), slice([1,2,3], 0, -1)", "[2, 3] ello llo [1, 2]");
    test_out("map literal access", "let p = {name: \"Ada\", \"full name\": \"A L\", 1: \"one\"}\nsay p.name, p[\"full name\"], p[1], p.missing", "Ada A L one nothing");
    test_out("map assign compound", "let m = {}\nm.count = 0\nm.count += 2\nm[\"k\"] = \"v\"\nsay m", "{count: 2, k: \"v\"}");
    test_out("nested structures", "let d = {list: [1, {deep: \"yes\"}]}\nsay d.list[1].deep", "yes");
    test_out("map keys values entries", "let m = {a: 1, b: 2}\nsay keys(m), values(m), entries(m), len(m)", "[\"a\", \"b\"] [1, 2] [[\"a\", 1], [\"b\", 2]] 2");
    test_out("word count idiom", "let c = {}\nfor w in split(\"a b a c a\") { c[w] = (c[w] ?? 0) + 1 }\nsay c", "{a: 3, b: 1, c: 1}");
    test_out("contains find", "say contains(\"hello\", \"ell\"), contains([1,2], 2), contains({a:1}, \"a\"), find(\"hello\", \"l\"), find([5,6], 6), find([1], 9)", "true true true 2 1 -1");
    test_out("text functions", "say upper(\"abc\"), lower(\"ABC\"), trim(\"  x  \"), replace(\"a-b-c\", \"-\", \"+\"), startsWith(\"kestral\", \"kes\"), endsWith(\"kestral\", \"x\"), repeat(\"ab\", 2)", "ABC abc x a+b+c true false abab");
    test_out("split join", "say split(\"a,b,c\", \",\"), split(\"  a   b \"), split(\"abc\", \"\"), join([1, 2, 3], \"-\"), join([\"a\", \"b\"])", "[\"a\", \"b\", \"c\"] [\"a\", \"b\"] [\"a\", \"b\", \"c\"] 1-2-3 ab");
    test_out("text indexing", "say \"hello\"[1], \"hello\"[-1], len(\"hello\")", "e o 5");
    test_out("reverse sort unique", "say reverse([1,2,3]), reverse(\"abc\"), sort([3,1,2]), sort([\"b\",\"a\"]), unique([1,2,1,3,2])", "[3, 2, 1] cba [1, 2, 3] [\"a\", \"b\"] [1, 2, 3]");
    test_out("sort with key", "say sort([\"ccc\", \"a\", \"bb\"], fn(s) => len(s))", "[\"a\", \"bb\", \"ccc\"]");
    test_out("map filter reduce", "let xs = [1, 2, 3, 4]\nsay map(xs, fn(x) => x * x), filter(xs, fn(x) => x % 2 == 0), reduce(xs, fn(a, b) => a + b, 0), reduce(xs, fn(a, b) => a * b)", "[1, 4, 9, 16] [2, 4] 10 24");
    test_out("map with index", "say map([\"a\", \"b\"], fn(x, i) => \"{i}{x}\")", "[\"0a\", \"1b\"]");
    test_out("any all", "say any([1, 2], fn(x) => x > 1), all([1, 2], fn(x) => x > 1)", "true false");
    test_out("range functions", "say sum(1..4), max(1..4), len(1..10), list(1..3), map(1..3, fn(x) => x * 2)", "10 4 10 [1, 2, 3] [2, 4, 6]");
    test_out("range contains index", "let r = 5..9\nsay contains(r, 7), r[0], r[-1], 3..1", "true 5 9 3..1");
    test_out("math", "say abs(-3), floor(2.7), ceil(2.1), sqrt(16), min(3, 1, 2), max([4, 9, 2]), sum([1.5, 2.5])", "3 2 3 4 1 9 4");
    test_out("round", "say round(2.5), round(-2.5), round(3.14159, 2), round(1.005, 2), round(1234, -2)", "3 -3 3.14 1.01 1200");
    test_out("conversions", "say num(\"42\") + 1, num(\" 3.5 \"), int(3.9), int(\"7\"), str(12) + \"!\", str([1, \"a\"])", "43 3.5 3 7 12! [1, \"a\"]");
    test_out("type", "say type(1), type(\"a\"), type(true), type([]), type({}), type(nothing), type(say_hi), type(1..2)\nfn say_hi() => 1", "number text bool list map nothing function range");
    test_out("pipe", "fn add(a, b) => a + b\nfn dbl(x) => x * 2\nsay 5 |> dbl, 5 |> add(10) |> dbl", "10 30");
    test_out("pipe across lines", "let r = [3, 1, 2]\n  |> sort\n  |> reverse\nsay r", "[3, 2, 1]");
    test_out("method calls", "say \"abc\".upper(), [3, 1, 2].sort().reverse(), \"a,b\".split(\",\").len()", "ABC [3, 2, 1] 2");
    test_out("method chain lines", "let x = \"  hi \"\n  .trim()\n  .upper()\nsay x", "HI");
    test_out("map holding fns", "let calc = {add: fn(a, b) => a + b, zero: 0}\nsay calc.add(2, 3), calc.zero", "5 0");
    test_out("match values", "fn d(n) { give match n { 1 => \"one\"\n 2, 3 => \"few\"\n else => \"many\" } }\nsay d(1), d(3), d(9)", "one few many");
    test_out("match ranges", "let g = match 85 { 90..100 => \"A\"\n 80..89 => \"B\"\n else => \"C\" }\nsay g\nsay match \"hi\" { \"hi\" => 1\n else => 2 }", "B\n1");
    test_out("match stmt blocks", "match 2 {\n 1 => say \"a\"\n 2 => {\n  say \"b\"\n  say \"c\"\n }\n}", "b\nc");
    test_out("match empty nothing", "say match 5 { 1 => \"x\" }", "nothing");
    test_out("try catch fail", "try { fail \"boom\" } catch e { say \"caught {e}\" }", "caught boom");
    test_out("try catch runtime", "try { say 1 / 0 } catch e { say e }", "Cannot divide by zero.");
    test_out("catch next line", "try {\n fail \"x\"\n}\ncatch err {\n say err\n}", "x");
    test_out("try without err", "try { say \"ok\" } catch e { say \"bad\" }", "ok");
    test_out("fail number", "try { fail 42 } catch e { say e + 1 }", "421");
    test_out("num catchable", "try { num(\"abc\") } catch e { say e }", "I can't turn \"abc\" into a number.");
    test_out("give inside try", "fn f() { try { give 1 } catch e { give 2 } }\nsay f()", "1");
    test_out("give inside loop", "fn first(l) { for x in l { if x > 1 { give x } }; give nothing }\nsay first([1, 2, 3]), first([0])", "2 nothing");
    test_out("ask reads input", "let a = ask()\nlet b = ask(\"name? \")\nsay a, b, ask()", "one two nothing", {"one", "two"});
    test_out("ask echo off", "say num(ask()) * 2", "42", {"21"});
    test_out("assert pass", "assert(1 == 1)\nsay \"fine\"", "fine");
    test_err("assert fail", "assert(1 == 2, \"nope\")", "nope");
    test_out("semicolons", "let a = 1; let b = 2; say a + b", "3");
    test_out("comments", "// top\nsay 1 // trailing\n// end", "1");
    test_out("multiline list map", "let a = [\n 1,\n 2,\n]\nlet m = {\n x: 1,\n y: 2\n}\nsay a, m", "[1, 2] {x: 1, y: 2}");
    test_out("shadowing blocks", "let x = 1\nif true { let x = 2; say x }\nsay x", "2\n1");
    test_out("assign outer scope", "let x = 1\nif true { x = 5 }\nsay x", "5");
    test_out("say nested text", "say [\"a\", {k: \"v\"}]", "[\"a\", {k: \"v\"}]");
    test_out("say functions", "fn f() => 1\nsay f", "<function f>");
    test_out("keyword map key", "let m = {in: 1, times: 2}\nsay m.in, m.times", "1 2");

    /* ---- Errors ---- */
    test_err("err unknown hint", "let score = 1\nsay scor", "Did you mean \"score\"");
    test_err("err unknown no hint", "say zzzzzz", "I don't know \"zzzzzz\"");
    test_err("err assign before let", "x = 5", "doesn't exist yet");
    test_err("err divide zero", "say 1 / 0", "Cannot divide by zero");
    test_err("err mod zero", "say 1 % 0", "Cannot divide by zero");
    test_err("err add types", "say [1] + 1", "can't add list and number");
    test_err("err minus text", "say \"a\" - 1", "needs numbers");
    test_err("err compare types", "say 1 < \"a\"", "can't compare number with text");
    test_err("err list bounds", "let a = [1]\nsay a[5]", "outside this list");
    test_err("err call non fn", "let n = 5\nsay n(1)", "not a function");
    test_err("err wrong arg count", "fn f(a) => a\nsay f(1, 2)", "needs 1 value");
    test_err("err builtin arg count", "say upper()", "upper() needs 1 value");
    test_err("err builtin type", "say upper(5)", "upper() needs text");
    test_err("err member nothing", "let p = nothing\nsay p.name", "value is nothing");
    test_err("err member number", "say (5).name", "has no \"name\"");
    test_err("err loop number", "for i in 5 { say i }", "loop 5 times");
    test_err("err bad loop count", "loop \"x\" times { }", "whole number");
    test_err("err sqrt negative", "say sqrt(-1)", "square root");
    test_err("err pop empty", "pop([])", "empty list");
    test_err("err uncaught fail", "fail \"custom\"", "custom");
    test_err("err give outside", "give 1", "\"give\" only works inside a function");
    test_err("err stop outside", "stop", "\"stop\" only works inside a loop");
    test_err("err text assign", "let s = \"abc\"\ns[0] = \"x\"", "Text can't be changed");
    test_err("err map key type", "let m = {}\nm[[1]] = 2", "Map keys must be");
    test_err("err float range", "for i in 1..2.5 { }", "whole numbers");
    test_err("err missing brace", "if true {\n say 1\n", "never closed");
    test_err("err unterminated", "say \"abc", "never closes");
    test_err("err two stmts", "say 1 say 2", "own line");
    test_err("err let name", "let = 5", "expected a name");
    test_err("err let equals", "let x 5", "expected \"=\"");
    test_err("err bad char", "say 1 @ 2", "don't understand");
    test_err("err &&", "say true && false", "and");
    test_err("err bang", "say !true", "not");
    test_err("err empty interp", "say \"a {} b\"", "Empty");
    test_err("err try no catch", "try { say 1 }", "needs a catch");
    test_err("err assign target", "1 = 2", "can only assign");
    test_err("err infinite loop", "while true { }", "ran for too long", -1, 20000);
    test_err("err infinite rec", "fn f(n) => f(n + 1)\nsay f(0)", "too deep");
    test_err("err fatal uncatchable", "try { while true { } } catch e { say \"caught\" }", "ran for too long", -1, 20000);
    test_err("err output flood", "while true { say \"x\" }", "printed too much", -1, 5000000000LL, 200);
    test_err("err line numbers", "say 1\nsay 2\nsay nope", "I don't know", 3);
    test_err("err line inside fn", "fn f() {\n  say missing\n}\nf()", "I don't know", 2);
    test_err("err misspelled say", "sya \"hi\"", "Did you mean \"say\"", 1);
    test_err("err misspelled let", "lte x = 5", "Did you mean \"let\"");
    test_out("no false hint", "let sya = 1\nsya\nsay \"ok\"", "ok");

    /* ---- Examples Golden Tests ---- */
    fs::path ex_dir = fs::path("examples");
    for (const auto& entry : fs::directory_iterator(ex_dir)) {
        if (entry.path().extension() != ".kes") continue;
        fs::path out_p = entry.path();
        out_p.replace_extension(".out");
        if (!fs::exists(out_p)) continue;
        fs::path in_p = entry.path();
        in_p.replace_extension(".in");
        std::vector<std::string> ins;
        if (fs::exists(in_p)) {
            std::ifstream inf(in_p, std::ios::binary);
            std::string ln;
            while (std::getline(inf, ln)) {
                if (!ln.empty() && ln.back() == '\r') ln.pop_back();
                ins.push_back(ln);
            }
        }
        InterpConfig cfg;
        cfg.read_file_fn = [ex_dir](const std::string& p) {
            std::ifstream f(ex_dir / p, std::ios::binary);
            if (!f) throw std::runtime_error("file not found");
            std::ostringstream ss;
            ss << f.rdbuf();
            return ss.str();
        };
        std::string src = read_file_str(entry.path());
        std::string expected = read_file_str(out_p);
        auto res = run_captured(src, ins, cfg);
        check("example " + entry.path().filename().string(),
              res.ok && res.text == expected,
              res.ok ? ("expected:\n" + expected + "\ngot:\n" + res.text)
                     : ("error: " + res.error_text));
    }

    /* ---- Nana Highlighter, Buffer, Banner & Adaptive Mica/Black TUI Tests ---- */
    {
        std::string sample = "let x = \"hi {name}\" |> upper // note";
        std::string hl = highlight_kestral_line(sample);
        check("nana highlighter preserves chars", strip_ansi(hl) == sample);
        check("nana highlighter emits ansi", hl.find("\x1b[") != std::string::npos);

        for (const auto& tpl : nana_templates()) {
            auto nb = NanaBuffer::from_template(tpl);
            auto syn = nb.check_syntax();
            check("nana template valid: " + tpl.id, syn.first, syn.second);
        }

        std::string b = render_startup_banner(KESTRAL_VERSION, true, true);
        check("banner contains Kestral", b.find("Kestral") != std::string::npos);

        NanaBuffer eb;
        eb.lines = {"fn add(a, b) {", "  give a + b", "}"};
        auto files = discover_kes_files("examples");
        check("discover kes files", files.size() >= 7);

        for (const std::string& bg : {std::string("mica"), std::string("black")}) {
            for (auto [rows, cols] : std::vector<std::pair<int, int>>{{12, 42}, {24, 80}, {45, 160}}) {
                std::string w = render_welcome_menu(1, rows, cols, false, bg);
                check("welcome no newline " + bg, w.find('\n') == std::string::npos);
                check("welcome no reverse video " + bg, w.find("\x1b[7m") == std::string::npos);
                check("welcome row 1 anchored", w.find("\x1b[1;1H") != std::string::npos);
                std::string ed = eb.render_frame(rows, cols, bg);
                check("editor no newline " + bg, ed.find('\n') == std::string::npos);
                std::string tp = render_template_picker(0, rows, cols, bg);
                check("template picker no newline", tp.find('\n') == std::string::npos);
                std::string fp = render_file_picker(files, 0, rows, cols, "examples", bg);
                check("file picker no newline", fp.find('\n') == std::string::npos);
            }
        }
    }

    /* ---- Interactive Learn Course Verification ---- */
    {
        const auto& ls = kestral_lessons();
        check("kestral learn has 10 lessons", ls.size() == 10);
        for (const auto& l : ls) {
            auto sol = evaluate_lesson(l, l.solution);
            bool all_ok = !sol.empty();
            for (const auto& r : sol) if (!r.passed) all_ok = false;
            check("learn solution passes: " + l.id, all_ok);

            auto st = evaluate_lesson(l, l.starter);
            bool starter_passes = !st.empty();
            for (const auto& r : st) if (!r.passed) starter_passes = false;
            check("learn starter does not pass: " + l.id, !starter_passes);
        }
        NanaBuffer lbuf;
        lbuf.lines = {"let x = 1"};
        std::string lw = render_learn_workspace(ls[0], lbuf, {}, 1, {}, 28, 100, "mica");
        check("learn workspace no newline", lw.find('\n') == std::string::npos);
        check("learn map no newline", render_lesson_map(0, {"01-hello-cards"}, 24, 90, "black").find('\n') == std::string::npos);
        check("curriculum summary has capstone", format_curriculum_list({}).find("Number Cruncher") != std::string::npos);
    }

    std::cout << "\nKestral test suite: " << g_pass << " passed, " << g_fail << " failed.\n";
    for (const auto& f : g_failures) std::cout << f << "\n";
    return g_fail == 0 ? 0 : 1;
}
