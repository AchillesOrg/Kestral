#include "learn.hpp"
#include "interp.hpp"
#include "nana.hpp"
#include "version.hpp"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

namespace fs = std::filesystem;

namespace kestral {

namespace {

const char* RESET = "\x1b[0m";
const char* BOLD = "\x1b[1m";
const char* DIM = "\x1b[2m";
const char* BRIGHT_WHITE = "\x1b[1;97m";
const char* GRAY = "\x1b[90m";
const char* GREEN_CLR = "\x1b[92m";
const char* RED_CLR = "\x1b[91m";
const char* CYAN_CLR = "\x1b[96m";
const char* YELLOW_CLR = "\x1b[93m";

std::string render_bar_ascii(int done, int total, int width = 20) {
    double r = total > 0 ? std::max(0.0, std::min(1.0, static_cast<double>(done) / total)) : 0.0;
    int filled = static_cast<int>(std::llround(r * width));
    int pct = static_cast<int>(std::llround(r * 100.0));
    std::string bar = "[";
    for (int i = 0; i < filled; ++i) bar += "█";
    for (int i = filled; i < width; ++i) bar += "░";
    bar += "] " + std::to_string(pct) + "%";
    return bar;
}

std::vector<std::string> wrap_text(const std::string& text, int width) {
    width = std::max(width, 16);
    std::vector<std::string> out;
    std::istringstream iss(text);
    std::string para;
    while (std::getline(iss, para)) {
        if (para.empty()) { out.push_back(""); continue; }
        std::istringstream words(para);
        std::string w, cur;
        while (words >> w) {
            if (cur.empty()) cur = w;
            else if (static_cast<int>(cur.size() + 1 + w.size()) <= width) cur += " " + w;
            else { out.push_back(cur); cur = w; }
        }
        if (!cur.empty()) out.push_back(cur);
    }
    return out;
}

std::string make_hairline(int cols) {
    std::string s;
    for (int i = 0; i < cols; ++i) s += "─";
    return std::string(GRAY) + s + RESET;
}

std::string json_escape(const std::string& s) {
    std::string out = "\"";
    for (char c : s) {
        if (c == '\\') out += "\\\\";
        else if (c == '"') out += "\\\"";
        else if (c == '\n') out += "\\n";
        else if (c == '\r') out += "\\r";
        else if (c == '\t') out += "\\t";
        else out += c;
    }
    out += '"';
    return out;
}

std::string json_unescape(const std::string& raw) {
    std::string out;
    for (std::size_t i = 0; i < raw.size(); ++i) {
        if (raw[i] == '\\' && i + 1 < raw.size()) {
            char e = raw[i + 1];
            if (e == 'n') { out += '\n'; i++; continue; }
            if (e == 'r') { out += '\r'; i++; continue; }
            if (e == 't') { out += '\t'; i++; continue; }
            if (e == '"' || e == '\\') { out += e; i++; continue; }
        }
        out += raw[i];
    }
    return out;
}

std::string format_brief_overlay(const Lesson& l, const std::vector<TestResult>& res, int hint_idx) {
    std::ostringstream oss;
    oss << "[" << l.track << "] Lesson " << l.number << ": " << l.title << "\n\n"
        << l.explanation << "\n\nChallenge:\n" << l.challenge
        << "\n\nExample:\n" << l.example << "\n\nTests:\n";
    for (std::size_t i = 0; i < l.tests.size(); ++i) {
        if (i < res.size()) {
            oss << "  [" << (res[i].passed ? "x" : " ") << "] " << l.tests[i].description
                << " -> " << (res[i].passed ? "PASS" : ("FAIL: " + res[i].detail)) << "\n";
        } else {
            oss << "  [ ] " << l.tests[i].description << "\n";
        }
    }
    if (hint_idx > 0 && !l.hints.empty()) {
        oss << "\nHints:\n";
        for (int i = 0; i < std::min(hint_idx, static_cast<int>(l.hints.size())); ++i) {
            oss << "  * " << l.hints[static_cast<std::size_t>(i)] << "\n";
        }
    }
    return oss.str();
}

} // namespace

const std::vector<Lesson>& kestral_lessons() {
    static const std::vector<Lesson> lessons = {
        {
            1, "01-hello-cards", "Basics", "Hello & Text Cards",
            "Print interpolated text inside a box card",
            "Kestral strings interpolate any expression inside {braces},\n"
            "and values flow left-to-right through |> pipelines.",
            "let app = \"Kestral\"\nsay \"Ready: {app}\" |> box(\"Status\")",
            "Create `let hero = \"Kestral\"` and print `\"Hello, {hero}!\"` piped into `box(\"Greeting\")`.",
            "let hero = \"Kestral\"\n// Pipe greeting into box(\"Greeting\")\nsay hero\n",
            "let hero = \"Kestral\"\nsay \"Hello, {hero}!\" |> box(\"Greeting\")\n",
            {"Use string interpolation: \"Hello, {hero}!\"", "Pipe it: |> box(\"Greeting\")"},
            {{"Prints boxed Greeting with Hello, Kestral!", {}, {}, {"Greeting", "Hello, Kestral!", "╭"}, {}, "", false, false}}
        },
        {
            2, "02-functions-defaults", "Basics", "Functions & Defaults",
            "Write concise => functions with default arguments",
            "Declare short functions using `fn name(arg = default) => expr`.\n"
            "Functions are hoisted automatically across the file.",
            "fn hi(who = \"friend\") => \"Hi, {who}!\"\nsay hi(), hi(\"Ada\")",
            "Define `fn greet(name = \"world\", mark = \"!\") => \"Hello, {name}{mark}\"`,\n"
            "then call `say greet()` and `say greet(\"Ada\", \"?\")`.",
            "fn greet(name = \"world\", mark = \"!\") {\n  // return Hello, <name><mark>\n}\nsay greet()\nsay greet(\"Ada\", \"?\")\n",
            "fn greet(name = \"world\", mark = \"!\") => \"Hello, {name}{mark}\"\nsay greet()\nsay greet(\"Ada\", \"?\")\n",
            {"Expression body: => \"Hello, {name}{mark}\""},
            {{"Default and overridden arguments work", {}, {}, {}, {}, "Hello, world!\nHello, Ada?", true, false}}
        },
        {
            3, "03-decisions-match", "Control Flow", "If Expressions & Match",
            "Classify numbers cleanly with range-based match",
            "`if` and `match` are expressions in Kestral — both evaluate to values,\n"
            "and `match` supports comma lists and inclusive ranges like 80..89.",
            "let tag = match n { 0 => \"zero\"  1..5 => \"few\"  else => \"many\" }",
            "Write `fn label(score)` using `match` that gives `\"A\"` for `90..100`,\n"
            "`\"B\"` for `80..89`, and `\"C\"` for `else`. Print `label(95), label(82), label(60)`.",
            "fn label(score) {\n  give match score {\n    else => \"C\"\n  }\n}\nsay label(95), label(82), label(60)\n",
            "fn label(score) {\n  give match score {\n    90..100 => \"A\"\n    80..89 => \"B\"\n    else => \"C\"\n  }\n}\nsay label(95), label(82), label(60)\n",
            {"Add 90..100 => \"A\" and 80..89 => \"B\" before else."},
            {{"Outputs A B C", {}, {}, {}, {}, "A B C", true, false}}
        },
        {
            4, "04-loops-ranges", "Control Flow", "Loops, Ranges & Methods",
            "Filter and join ranges with dot-method chains",
            "Ranges like `1..10` work directly with `for`, `map`, `filter`, and dot chains\n"
            "because any function can be called as `val.fn(rest...)`.",
            "say (1..5).filter(fn(x) => x > 2).join(\"-\")",
            "Print all even squares from `1..6` separated by `\", \"`: filter `1..6` for `x % 2 == 0`,\n"
            "map each to `x * x`, and `join` with `\", \"`.",
            "let evens = filter(1..6, fn(x) => x % 2 == 0)\n// square evens and join with \", \"\n",
            "let evens = filter(1..6, fn(x) => x % 2 == 0)\nlet squares = map(evens, fn(x) => x * x)\nsay join(squares, \", \")\n",
            {"Call map(evens, fn(x) => x * x) then say join(squares, \", \")"},
            {{"Outputs 4, 16, 36", {}, {}, {}, {}, "4, 16, 36", true, false}}
        },
        {
            5, "05-lists-maps", "Data", "Lists, Maps & Word Counts",
            "Tally word counts using maps and nullish coalescing ??",
            "Missing map keys return `nothing`. Combine `m[k] ?? 0` with\n"
            "`sort(entries(m), fn(p) => -p[1])` to rank frequency tables cleanly.",
            "let counts = {}\ncounts[\"a\"] = (counts[\"a\"] ?? 0) + 1",
            "Split `\"go kes go fast go\"` by spaces, count frequencies into map `counts`,\n"
            "and print `counts.go` and `counts.kes`.",
            "let counts = {}\nfor w in split(\"go kes go fast go\") {\n  // increment counts[w]\n}\nsay counts.go, counts.kes\n",
            "let counts = {}\nfor w in split(\"go kes go fast go\") {\n  counts[w] = (counts[w] ?? 0) + 1\n}\nsay counts.go, counts.kes\n",
            {"Use: counts[w] = (counts[w] ?? 0) + 1"},
            {{"Outputs 3 1", {}, {}, {}, {}, "3 1", true, false}}
        },
        {
            6, "06-closures", "Real Programs", "Closures & State",
            "Build stateful object-like records from maps + closures",
            "Functions capture outer `let` bindings by lexical closure.\n"
            "Returning a map of functions gives private state with zero boilerplate.",
            "fn counter() { let n = 0; give fn() { n += 1; give n } }",
            "Write `fn makeWallet(start = 0)` returning a map with `deposit(amt)` (fails if `amt <= 0`),\n"
            "and `balance()` returning the balance. Deposit `50` into `makeWallet(10)` and `say w.balance()`.",
            "fn makeWallet(start = 0) {\n  let bal = start\n  give {\n    deposit: fn(amt) { },\n    balance: fn() => bal\n  }\n}\nlet w = makeWallet(10)\nw.deposit(50)\nsay w.balance()\n",
            "fn makeWallet(start = 0) {\n  let bal = start\n  give {\n    deposit: fn(amt) { if amt <= 0 { fail \"must be positive\" }; bal += amt },\n    balance: fn() => bal\n  }\n}\nlet w = makeWallet(10)\nw.deposit(50)\nsay w.balance()\n",
            {"Inside deposit: if amt <= 0 { fail \"must be positive\" }; bal += amt"},
            {{"Wallet tracks state -> 60", {}, {}, {}, {}, "60", true, false}}
        },
        {
            7, "07-cli-args", "CLI Tools", "CLI Arguments & Boxed Output",
            "Build a real CLI utility reading args()",
            "`args()` returns CLI arguments passed after your `.kes` script.\n"
            "Pair it with `if` expressions, `upper()`, and `box()` for crisp CLI utilities.",
            "let cli = args()\nlet mode = if len(cli) > 0 { cli[0] } else { \"default\" }",
            "Read `let a = args()`. Use `who = if len(a) > 0 { a[0] } else { \"friend\" }`.\n"
            "If `len(a) > 1 and a[1] == \"-u\"`, uppercase `who`. Print `\"Hi {who}!\" |> box(\"CLI\")`.",
            "let a = args()\nlet who = if len(a) > 0 { a[0] } else { \"friend\" }\n// handle -u and box(\"CLI\")\n",
            "let a = args()\nlet who = if len(a) > 0 { a[0] } else { \"friend\" }\nif len(a) > 1 and a[1] == \"-u\" {\n  who = upper(who)\n}\nsay \"Hi {who}!\" |> box(\"CLI\")\n",
            {"Set who = upper(who) when len(a) > 1 and a[1] == \"-u\"", "Finish with: say \"Hi {who}!\" |> box(\"CLI\")"},
            {
                {"Default prints Hi friend! in CLI box", {}, {}, {"CLI", "Hi friend!"}, {}, "", false, false},
                {"Arg Ada -u prints Hi ADA!", {}, {"Ada", "-u"}, {"CLI", "Hi ADA!"}, {}, "", false, false}
            }
        },
        {
            8, "08-files-errors", "Real Programs", "Files & Error Recovery",
            "Read/write files safely with try/catch and fileExists",
            "`writeFile(path, text)`, `readFile(path)`, and `fileExists(path)` let `.kes` scripts\n"
            "automate disk workflows, while `try / catch` handles bad files cleanly.",
            "writeFile(\"out.txt\", \"ok\")\nif fileExists(\"out.txt\") { say readFile(\"out.txt\") }",
            "Write `\"kestral:2.0\"` to `\"note.txt\"`. Check `fileExists(\"note.txt\")` and print `readFile(\"note.txt\")`.\n"
            "Then `try { readFile(\"missing.txt\") } catch e { say \"Recovered\" }`.",
            "writeFile(\"note.txt\", \"kestral:2.0\")\n// verify fileExists, read back, and catch missing file\n",
            "writeFile(\"note.txt\", \"kestral:2.0\")\nif fileExists(\"note.txt\") {\n  say readFile(\"note.txt\")\n}\ntry {\n  readFile(\"missing.txt\")\n} catch e {\n  say \"Recovered\"\n}\n",
            {"Call say readFile(\"note.txt\") inside if fileExists(\"note.txt\")", "Use try { readFile(\"missing.txt\") } catch e { say \"Recovered\" }"},
            {{"Writes, reads, and recovers cleanly", {}, {}, {"kestral:2.0", "Recovered"}, {}, "", false, true}}
        },
        {
            9, "09-ask-loops", "CLI + TUI", "Interactive Input Loops",
            "Keep asking input until 'done' or EOF",
            "`ask()` returns `nothing` on EOF. Loop cleanly with `while true { ... stop }`\n"
            "to build interactive terminal prompts and parsers.",
            "while true {\n  let ln = ask()\n  if ln == nothing or ln == \"done\" { stop }\n}",
            "Loop with `while true`, read `let ln = ask()`, `stop` if `ln == nothing or ln == \"done\"`,\n"
            "otherwise `push(items, ln)`. After the loop print `\"Items ({len(items)}): {join(items, \", \")}\"`.",
            "let items = []\nwhile true {\n  let ln = ask()\n  // stop on nothing or \"done\", else push ln\n}\nsay \"Items ({len(items)}): {join(items, \", \")}\"\n",
            "let items = []\nwhile true {\n  let ln = ask()\n  if ln == nothing or ln == \"done\" { stop }\n  push(items, ln)\n}\nsay \"Items ({len(items)}): {join(items, \", \")}\"\n",
            {"Use: if ln == nothing or ln == \"done\" { stop }"},
            {
                {"Collects items until done", {"a", "b", "done"}, {}, {}, {}, "Items (2): a, b", true, false},
                {"Stops cleanly on EOF", {"solo"}, {}, {}, {}, "Items (1): solo", true, false}
            }
        },
        {
            10, "10-capstone", "Capstone", "Capstone: Number Cruncher",
            "Put everything together in a fault-tolerant CLI tool",
            "Combine loops, `ask()`, `try/catch`, `num()`, `sum()`, `max()`, `round()`,\n"
            "and error skipping into the classic Kestral Capstone program.",
            "try { push(numbers, num(line)) } catch e { say \"Skipping: {line}\" }",
            "Read lines with `ask()` until `nothing` or `\"done\"`. Convert valid numbers with `num(line)`\n"
            "and skip invalid ones with `Skipping: {line}`.\n"
            "If empty print `No numbers entered.`, else print Count, Total, Average (2 decimals), Largest.",
            "let numbers = []\nwhile true {\n  let line = ask()\n  if line == nothing or line == \"done\" { stop }\n  // try num(line) or print Skipping: {line}\n}\n// print summary or No numbers entered.\n",
            "let numbers = []\nwhile true {\n  let line = ask()\n  if line == nothing or line == \"done\" { stop }\n  try {\n    push(numbers, num(line))\n  } catch e {\n    say \"Skipping: {line}\"\n  }\n}\nif len(numbers) == 0 {\n  say \"No numbers entered.\"\n} else {\n  say \"Count: {len(numbers)}\"\n  say \"Total: {sum(numbers)}\"\n  say \"Average: {round(sum(numbers) / len(numbers), 2)}\"\n  say \"Largest: {max(numbers)}\"\n}\n",
            {"Wrap push(numbers, num(line)) in try { ... } catch e { say \"Skipping: {line}\" }",
             "Compute average: round(sum(numbers) / len(numbers), 2)"},
            {
                {"6 numbers crunch cleanly", {"4", "8", "15", "16", "23", "42", "done"}, {}, {}, {}, "Count: 6\nTotal: 108\nAverage: 18\nLargest: 42", true, false},
                {"Decimals round cleanly", {"2.5", "3", "done"}, {}, {}, {}, "Count: 2\nTotal: 5.5\nAverage: 2.75\nLargest: 3", true, false},
                {"Skips invalid input cleanly", {"x", "5", "done"}, {}, {}, {}, "Skipping: x\nCount: 1\nTotal: 5\nAverage: 5\nLargest: 5", true, false},
                {"Handles immediate done", {"done"}, {}, {}, {}, "No numbers entered.", true, false},
                {"Handles EOF without done", {"7", "1"}, {}, {}, {}, "Count: 2\nTotal: 8\nAverage: 4\nLargest: 7", true, false}
            }
        }
    };
    return lessons;
}

std::vector<TestResult> evaluate_lesson(const Lesson& lesson, const std::string& source) {
    std::vector<TestResult> out;
    for (const auto& tc : lesson.tests) {
        fs::path orig_cwd = fs::current_path();
        fs::path tmp;
        if (tc.use_temp_dir) {
            tmp = fs::temp_directory_path() / ("kestral_learn_" + std::to_string(lesson.number));
            std::error_code ec;
            fs::remove_all(tmp, ec);
            fs::create_directories(tmp, ec);
            fs::current_path(tmp, ec);
        }
        InterpConfig cfg;
        cfg.cli_args = tc.cli_args;
        cfg.max_steps = 2000000;
        auto run = run_captured(source, tc.inputs, cfg);
        if (tc.use_temp_dir) {
            std::error_code ec;
            fs::current_path(orig_cwd, ec);
            fs::remove_all(tmp, ec);
        }

        if (!run.ok) {
            out.push_back({tc.description, false, run.error_msg});
            continue;
        }
        std::string clean = strip_ansi(run.text);
        if (tc.has_exact) {
            auto trim_end = [](std::string s) {
                while (!s.empty() && (s.back() == '\n' || s.back() == '\r')) s.pop_back();
                return s;
            };
            if (trim_end(clean) != trim_end(tc.expect_exact)) {
                out.push_back({tc.description, false, "Output did not match expected result"});
                continue;
            }
        }
        bool fail = false;
        for (const auto& needle : tc.expect_contains) {
            if (clean.find(needle) == std::string::npos) {
                out.push_back({tc.description, false, "Missing \"" + needle + "\""});
                fail = true;
                break;
            }
        }
        if (fail) continue;
        for (const auto& bad : tc.expect_not_contains) {
            if (clean.find(bad) != std::string::npos) {
                out.push_back({tc.description, false, "Unexpected \"" + bad + "\""});
                fail = true;
                break;
            }
        }
        if (!fail) out.push_back({tc.description, true, "PASS"});
    }
    return out;
}

std::string learn_progress_path() {
    return (fs::path(kestral_home_dir()) / "learn_progress.json").string();
}

LearnProgress load_learn_progress() {
    LearnProgress p;
    std::ifstream f(learn_progress_path(), std::ios::binary);
    if (!f) return p;
    std::ostringstream ss;
    ss << f.rdbuf();
    std::string txt = ss.str();

    // Parse simple structured fields generated by save_learn_progress
    auto cpos = txt.find("\"current\":");
    if (cpos != std::string::npos) {
        try { p.current = std::stoi(txt.substr(cpos + 10)); } catch (...) {}
    }
    auto comp_pos = txt.find("\"completed\":");
    if (comp_pos != std::string::npos) {
        auto lb = txt.find('[', comp_pos);
        auto rb = txt.find(']', lb == std::string::npos ? 0 : lb);
        if (lb != std::string::npos && rb != std::string::npos) {
            std::string inner = txt.substr(lb + 1, rb - lb - 1);
            std::size_t i = 0;
            while ((i = inner.find('"', i)) != std::string::npos) {
                std::size_t j = inner.find('"', i + 1);
                if (j == std::string::npos) break;
                p.completed.insert(inner.substr(i + 1, j - i - 1));
                i = j + 1;
            }
        }
    }
    auto dr_pos = txt.find("\"drafts\":");
    if (dr_pos != std::string::npos) {
        auto lb = txt.find('{', dr_pos);
        if (lb != std::string::npos) {
            auto read_q = [&](std::size_t& idx) -> std::string {
                std::size_t q1 = txt.find('"', idx);
                if (q1 == std::string::npos) return "";
                std::size_t q2 = q1 + 1;
                while (q2 < txt.size()) {
                    if (txt[q2] == '\\' && q2 + 1 < txt.size()) { q2 += 2; continue; }
                    if (txt[q2] == '"') break;
                    q2++;
                }
                if (q2 >= txt.size()) return "";
                idx = q2 + 1;
                return json_unescape(txt.substr(q1 + 1, q2 - q1 - 1));
            };
            std::size_t pos = lb + 1;
            while (pos < txt.size() && txt[pos] != '}') {
                std::string k = read_q(pos);
                if (k.empty()) break;
                auto col = txt.find(':', pos);
                if (col == std::string::npos) break;
                pos = col + 1;
                std::string v = read_q(pos);
                p.drafts.emplace_back(std::move(k), std::move(v));
                while (pos < txt.size() && (txt[pos] == ' ' || txt[pos] == '\n' ||
                                            txt[pos] == '\r' || txt[pos] == ',')) {
                    pos++;
                }
            }
        }
    }
    return p;
}

void save_learn_progress(const LearnProgress& p) {
    fs::path path(learn_progress_path());
    std::error_code ec;
    fs::create_directories(path.parent_path(), ec);
    std::ofstream f(path, std::ios::binary);
    if (!f) return;
    f << "{\n  \"current\": " << p.current << ",\n  \"completed\": [";
    bool first = true;
    for (const auto& id : p.completed) {
        if (!first) f << ", ";
        first = false;
        f << json_escape(id);
    }
    f << "],\n  \"drafts\": {\n";
    for (std::size_t i = 0; i < p.drafts.size(); ++i) {
        f << "    " << json_escape(p.drafts[i].first) << ": " << json_escape(p.drafts[i].second)
          << (i + 1 < p.drafts.size() ? "," : "") << "\n";
    }
    f << "  }\n}\n";
}

void reset_learn_progress() {
    std::error_code ec;
    fs::remove(learn_progress_path(), ec);
}

std::string format_curriculum_list(const std::set<std::string>& completed) {
    const auto& ls = kestral_lessons();
    int done = static_cast<int>(completed.size());
    std::ostringstream oss;
    oss << "Kestral Interactive Course (" << done << "/" << ls.size() << " completed)\n"
        << "Progress " << render_bar_ascii(done, static_cast<int>(ls.size()), 24) << "\n\n";
    for (const auto& l : ls) {
        std::string mark = completed.count(l.id) ? "✓" : " ";
        oss << "  [" << mark << "] " << (l.number < 10 ? " " : "") << l.number << ". [" << l.track;
        for (std::size_t i = l.track.size(); i < 13; ++i) oss << ' ';
        oss << "] " << l.title << " — " << l.goal << "\n";
    }
    oss << "\nRun `kestral learn` (or `kestral learn <n>`) to start in your terminal.\n";
    return oss.str();
}

std::string render_lesson_map(int selected, const std::set<std::string>& completed,
                              int rows, int cols, const std::string& bg_mode) {
    rows = std::max(rows, 10);
    cols = std::max(cols, 36);
    const auto& ls = kestral_lessons();
    int sel = std::max(0, std::min(selected, static_cast<int>(ls.size()) - 1));
    int done = static_cast<int>(completed.size());
    std::string bar = render_bar_ascii(done, static_cast<int>(ls.size()), 20);
    std::string bg_tag = (bg_mode == "black") ? "black" : "mica";
    std::string right_hdr = std::to_string(done) + "/" + std::to_string(ls.size()) + " complete · " + bg_tag + " ";
    int gap = std::max(cols - 16 - static_cast<int>(right_hdr.size()), 1);

    std::string header_bar = std::string(" ") + BOLD + "KESTRAL LEARN" + RESET +
                             std::string(static_cast<std::size_t>(gap), ' ') + GRAY + right_hdr + RESET;
    std::string hairline = make_hairline(cols);
    std::string footer_bar = std::string(" ") + DIM + "↑/↓ Lesson   Enter Start   1-9 Jump   t Surface   r Reset   Esc/q Quit" + RESET;

    int panel_w = (cols >= 60) ? std::min(cols - 4, 88) : (cols - 2);
    std::string margin(static_cast<std::size_t>(std::max((cols - panel_w) / 2, 1)), ' ');

    std::vector<std::string> lines{header_bar, hairline};
    if (rows >= 17) {
        lines.push_back("");
        lines.push_back(margin + BOLD + "Kestral Interactive Course" + RESET + "  " + GRAY + bar + RESET);
        lines.push_back(margin + GRAY + "CLI-first mastery: functions, match, closures, files & Capstone Number Cruncher" + RESET);
        lines.push_back("");
    } else {
        lines.push_back(margin + BOLD + "Curriculum" + RESET + " " + GRAY + bar + RESET);
    }

    int cap = std::max(rows - 2 - static_cast<int>(lines.size()), 2);
    int scroll = std::max(0, sel - cap + 1);
    for (int i = scroll; i < std::min(static_cast<int>(ls.size()), scroll + cap); ++i) {
        const auto& l = ls[static_cast<std::size_t>(i)];
        std::string mark = completed.count(l.id) ? (std::string(GREEN_CLR) + "✓" + RESET)
                                                 : (std::string(GRAY) + "○" + RESET);
        std::string badge = "[" + l.track + "]";
        while (badge.size() < 15) badge += ' ';
        std::string title_p = l.title.substr(0, 26);
        while (title_p.size() < 26) title_p += ' ';
        std::string num_s = (l.number < 10 ? " " : "") + std::to_string(l.number) + ".";
        int goal_w = std::max(panel_w - 52, 8);
        if (i == sel) {
            if (panel_w >= 62) {
                lines.push_back(margin + BRIGHT_WHITE + "▌ " + mark + " " + BRIGHT_WHITE + num_s + " " + RESET +
                                CYAN_CLR + badge + RESET + " " + BRIGHT_WHITE + title_p + RESET + " \x1b[37m" +
                                l.goal.substr(0, static_cast<std::size_t>(goal_w)) + RESET);
            } else {
                lines.push_back(margin + BRIGHT_WHITE + "▌ " + mark + " " + num_s + " " + l.title + RESET);
            }
        } else {
            if (panel_w >= 62) {
                lines.push_back(margin + "  " + mark + " " + GRAY + num_s + RESET + " " +
                                CYAN_CLR + badge + RESET + " \x1b[37m" + title_p + RESET + " " +
                                GRAY + l.goal.substr(0, static_cast<std::size_t>(goal_w)) + RESET);
            } else {
                lines.push_back(margin + "  " + mark + " " + GRAY + num_s + RESET + " \x1b[37m" + l.title + RESET);
            }
        }
    }

    while (static_cast<int>(lines.size()) < rows - 2) lines.push_back("");
    lines.resize(static_cast<std::size_t>(rows - 2));
    lines.push_back(hairline);
    lines.push_back(footer_bar);
    return compose_screen(lines, rows, cols, -1, -1, bg_mode);
}

std::string render_learn_workspace(const Lesson& lesson, NanaBuffer& buf,
                                   const std::vector<TestResult>& results,
                                   int hint_idx,
                                   const std::set<std::string>& completed,
                                   int rows, int cols,
                                   const std::string& bg_mode) {
    rows = std::max(rows, 10);
    cols = std::max(cols, 36);
    int body_rows = rows - 4;
    const auto& ls = kestral_lessons();
    std::string done_tag = completed.count(lesson.id) ? " ✓" : "";
    std::string right_hdr = "Ln " + std::to_string(buf.row + 1) + ", Col " + std::to_string(buf.col + 1) +
                            " · v" + KESTRAL_VERSION + " ";
    int max_left = std::max(cols - static_cast<int>(right_hdr.size()) - 4, 10);
    std::string left_plain = (" LEARN " + std::to_string(lesson.number) + "/" + std::to_string(ls.size()) +
                              " · " + lesson.title + done_tag).substr(0, static_cast<std::size_t>(max_left));
    int gap = std::max(cols - static_cast<int>(left_plain.size()) - static_cast<int>(right_hdr.size()), 1);
    std::string top_bar = std::string(" ") + BOLD + "LEARN " + std::to_string(lesson.number) + "/" +
                          std::to_string(ls.size()) + RESET + " " + GRAY + "· " +
                          lesson.title.substr(0, static_cast<std::size_t>(std::max(max_left - 12, 8))) +
                          done_tag + RESET + std::string(static_cast<std::size_t>(gap), ' ') + GRAY + right_hdr + RESET;
    std::string hairline = make_hairline(cols);

    auto syn = buf.check_syntax();
    std::string status_line = std::string(" ") +
                              (syn.first ? (std::string(GREEN_CLR) + "✓ " + syn.second + RESET)
                                         : (std::string(RED_CLR) + "✗ " + syn.second + RESET)) +
                              (buf.status.empty() ? "" : ("  │  " + buf.status));
    std::string footer_bar = std::string(DIM) +
                             std::string(" ^T Tests  ^R Run  ^H Hint  ^B Brief  ^L Map  ^E Reset  ^X Exit ")
                                 .substr(0, static_cast<std::size_t>(cols)) + RESET;

    if (cols >= 92) {
        int left_w = std::max(36, std::min(52, cols * 42 / 100));
        int right_start = left_w + 1;
        int ed_w = std::max(cols - right_start, 24);
        int gutter_w = std::max(static_cast<int>(std::to_string(buf.lines.size()).size()) + 3, 6);
        int code_w = std::max(ed_w - gutter_w, 12);
        int num_w = gutter_w - 3;

        if (buf.row < buf.scroll_y) buf.scroll_y = buf.row;
        else if (buf.row >= buf.scroll_y + body_rows) buf.scroll_y = buf.row - body_rows + 1;
        if (buf.col < buf.scroll_x) buf.scroll_x = buf.col;
        else if (buf.col >= buf.scroll_x + code_w) buf.scroll_x = buf.col - code_w + 1;

        int inner = left_w - 2;
        std::vector<std::string> lcol{
            std::string(" ") + BOLD + CYAN_CLR + "[" + lesson.track + "] " + lesson.title + RESET,
            std::string(" ") + make_hairline(std::min(inner, 46)),
        };
        for (const auto& s : wrap_text(lesson.explanation, inner)) lcol.push_back(" " + s);
        lcol.push_back("");
        lcol.push_back(std::string(" ") + BOLD + "Challenge:" + RESET);
        for (const auto& s : wrap_text(lesson.challenge, inner)) lcol.push_back(std::string(" ") + YELLOW_CLR + s + RESET);
        lcol.push_back("");
        lcol.push_back(std::string(" ") + BOLD + "Example:" + RESET);
        {
            std::istringstream eiss(lesson.example);
            std::string eln;
            while (std::getline(eiss, eln)) {
                lcol.push_back(" " + highlight_kestral_line(eln.substr(0, static_cast<std::size_t>(inner))));
            }
        }
        lcol.push_back("");
        if (hint_idx > 0 && !lesson.hints.empty()) {
            lcol.push_back(std::string(" ") + BOLD + "Hint:" + RESET);
            const std::string& h = lesson.hints[static_cast<std::size_t>(std::min(hint_idx - 1, static_cast<int>(lesson.hints.size()) - 1))];
            for (const auto& hs : wrap_text(h, inner)) lcol.push_back(std::string(" ") + CYAN_CLR + hs + RESET);
            lcol.push_back("");
        }
        lcol.push_back(std::string(" ") + BOLD + "Tests (^T):" + RESET);
        for (std::size_t i = 0; i < lesson.tests.size(); ++i) {
            std::string desc = lesson.tests[i].description.substr(0, static_cast<std::size_t>(std::max(inner - 2, 8)));
            if (i < results.size()) {
                std::string ic = results[i].passed ? (std::string(GREEN_CLR) + "✓" + RESET)
                                                   : (std::string(RED_CLR) + "✗" + RESET);
                lcol.push_back(" " + ic + " " + desc);
                if (!results[i].passed && !results[i].detail.empty()) {
                    lcol.push_back("   " + std::string(RED_CLR) + results[i].detail.substr(0, static_cast<std::size_t>(std::max(inner - 3, 8))) + RESET);
                }
            } else {
                lcol.push_back(" " + std::string(GRAY) + "○" + RESET + " " + desc);
            }
        }

        std::vector<std::string> screen{top_bar, hairline};
        for (int r = 0; r < body_rows; ++r) {
            std::string lt = (r < static_cast<int>(lcol.size())) ? clip_ansi(lcol[static_cast<std::size_t>(r)], left_w) : "";
            int lvis = static_cast<int>(visible_columns(lt));
            std::string lpad(static_cast<std::size_t>(std::max(left_w - lvis, 0)), ' ');
            int li = buf.scroll_y + r;
            std::string rt;
            if (li < static_cast<int>(buf.lines.size())) {
                const auto& raw = buf.lines[static_cast<std::size_t>(li)];
                std::string sl = (buf.scroll_x < static_cast<int>(raw.size()))
                                     ? raw.substr(static_cast<std::size_t>(buf.scroll_x),
                                                  static_cast<std::size_t>(code_w))
                                     : "";
                std::string lno = std::to_string(li + 1);
                if (static_cast<int>(lno.size()) < num_w) {
                    lno = std::string(static_cast<std::size_t>(num_w - static_cast<int>(lno.size())), ' ') + lno;
                }
                rt = std::string(GRAY) + lno + " │ " + RESET + highlight_kestral_line(sl);
            } else {
                std::string tilde = "~";
                if (num_w > 1) tilde = std::string(static_cast<std::size_t>(num_w - 1), ' ') + "~";
                rt = std::string(GRAY) + tilde + " │ " + RESET;
            }
            screen.push_back(lt + lpad + GRAY + "│" + RESET + rt);
        }
        screen.push_back(status_line);
        screen.push_back(footer_bar);
        int cr = (buf.row - buf.scroll_y) + 3;
        int cc = left_w + 1 + gutter_w + (buf.col - buf.scroll_x) + 1;
        return compose_screen(screen, rows, cols, cr, cc, bg_mode);
    }

    int gutter_w = std::max(static_cast<int>(std::to_string(buf.lines.size()).size()) + 3, 6);
    int code_w = std::max(cols - gutter_w, 10);
    int ed_rows = std::max(body_rows - 2, 3);
    if (buf.row < buf.scroll_y) buf.scroll_y = buf.row;
    else if (buf.row >= buf.scroll_y + ed_rows) buf.scroll_y = buf.row - ed_rows + 1;
    if (buf.col < buf.scroll_x) buf.scroll_x = buf.col;
    else if (buf.col >= buf.scroll_x + code_w) buf.scroll_x = buf.col - code_w + 1;

    int passed_n = 0;
    for (const auto& r : results) if (r.passed) passed_n++;
    std::string goal_bar = " Goal: " + lesson.goal.substr(0, static_cast<std::size_t>(std::max(cols - 26, 10))) +
                           "  [" + std::to_string(passed_n) + "/" + std::to_string(lesson.tests.size()) + "]";
    std::vector<std::string> screen{
        top_bar, hairline,
        std::string(" ") + CYAN_CLR + goal_bar.substr(0, static_cast<std::size_t>(std::max(cols - 2, 8))) + RESET,
        std::string(" ") + make_hairline(std::min(std::max(cols - 2, 6), 50))
    };
    int num_w = gutter_w - 3;
    for (int r = 0; r < ed_rows; ++r) {
        int li = buf.scroll_y + r;
        if (li < static_cast<int>(buf.lines.size())) {
            const auto& raw = buf.lines[static_cast<std::size_t>(li)];
            std::string sl = (buf.scroll_x < static_cast<int>(raw.size()))
                                 ? raw.substr(static_cast<std::size_t>(buf.scroll_x),
                                              static_cast<std::size_t>(code_w))
                                 : "";
            std::string lno = std::to_string(li + 1);
            if (static_cast<int>(lno.size()) < num_w) {
                lno = std::string(static_cast<std::size_t>(num_w - static_cast<int>(lno.size())), ' ') + lno;
            }
            screen.push_back(std::string(GRAY) + lno + " │ " + RESET + highlight_kestral_line(sl));
        } else {
            std::string tilde = "~";
            if (num_w > 1) tilde = std::string(static_cast<std::size_t>(num_w - 1), ' ') + "~";
            screen.push_back(std::string(GRAY) + tilde + " │ " + RESET);
        }
    }
    screen.push_back(status_line);
    screen.push_back(footer_bar);
    int cr = (buf.row - buf.scroll_y) + 5;
    int cc = gutter_w + (buf.col - buf.scroll_x) + 1;
    return compose_screen(screen, rows, cols, cr, cc, bg_mode);
}

static void set_draft(LearnProgress& p, const std::string& id, const std::string& code) {
    for (auto& kv : p.drafts) {
        if (kv.first == id) { kv.second = code; return; }
    }
    p.drafts.emplace_back(id, code);
}

static std::string get_draft_or(const LearnProgress& p, const Lesson& l) {
    for (const auto& kv : p.drafts) {
        if (kv.first == l.id) return kv.second;
    }
    return l.starter;
}

int run_learn(int lesson_num, bool list_only, bool reset,
              const std::string& bg_override, bool managed_tty,
              RawTtyGuard* parent_tty) {
    if (reset) {
        reset_learn_progress();
        if (list_only) {
            std::cout << "Course progress reset.\n" << format_curriculum_list({});
            return 0;
        }
    }
    LearnProgress prog = load_learn_progress();
    if (list_only) {
        std::cout << format_curriculum_list(prog.completed);
        return 0;
    }
    if (!stdin_stdout_are_tty()) {
        std::cerr << "kestral learn TUI requires an interactive terminal.\n"
                  << format_curriculum_list(prog.completed);
        return 1;
    }

    std::string bg_mode = bg_override.empty() ? load_bg_mode() : save_bg_mode(bg_override);
    RawTtyGuard local_tty;
    RawTtyGuard& active_tty = (managed_tty && parent_tty) ? *parent_tty : local_tty;
    if (!managed_tty) {
        active_tty.enter();
        std::cout << "\x1b[?1049h\x1b[?1000h\x1b[?1006h" << std::flush;
    }
    struct AltGuard {
        bool own;
        ~AltGuard() { if (own) std::cout << "\x1b[?1006l\x1b[?1000l\x1b[?1049l\x1b[?25h\x1b[0m" << std::flush; }
    } alt_guard{!managed_tty};

    const auto& ls = kestral_lessons();
    auto pick_map = [&](int initial_sel) -> int {
        int total = static_cast<int>(ls.size());
        int sel = std::max(0, std::min(initial_sel, total - 1));
        for (;;) {
            auto [rows, cols] = get_terminal_size();
            int rows_e = std::max(rows, 10);
            std::cout << render_lesson_map(sel, prog.completed, rows, cols, bg_mode) << std::flush;
            std::string k = read_key();
            if (k.empty() || k == "RESIZE") continue;
            MouseEvent me;
            if (parse_mouse_event(k, me)) {
                if (me.kind == "WHEEL_UP") { sel = (sel - 1 + total) % total; continue; }
                if (me.kind == "WHEEL_DOWN") { sel = (sel + 1) % total; continue; }
                int first_row = (rows_e >= 17) ? 7 : 4;
                int cap = std::max(rows_e - 2 - (first_row - 1), 2);
                int scroll = std::max(0, sel - cap + 1);
                if (me.row >= first_row && me.row < first_row + cap) {
                    int idx = scroll + (me.row - first_row);
                    if (idx >= 0 && idx < total) return idx;
                }
                continue;
            }
            if (k == "t") { bg_mode = save_bg_mode(bg_mode == "mica" ? "black" : "mica"); continue; }
            if (k == "UP") sel = (sel - 1 + total) % total;
            else if (k == "DOWN") sel = (sel + 1) % total;
            else if (k == "\r" || k == "\n") return sel;
            else if (k.size() == 1 && k[0] >= '1' && k[0] <= '9') {
                int idx = k[0] - '1';
                if (idx < total) return idx;
            } else if (k == "r") {
                bool canc = false;
                std::string ans = prompt_bar("Reset all course progress? (y/n): ", "", nullptr, bg_mode, &canc);
                if (!canc && !ans.empty() && (ans[0] == 'y' || ans[0] == 'Y')) {
                    prog.completed.clear();
                    prog.drafts.clear();
                    reset_learn_progress();
                }
            } else if (k == "ESC" || k == "q" || k == "\x18" || k == "\x03") {
                return -1;
            }
        }
    };

    int cur_idx = 0;
    if (lesson_num > 0) {
        if (lesson_num > static_cast<int>(ls.size())) return 1;
        cur_idx = lesson_num - 1;
    } else {
        cur_idx = pick_map(prog.current - 1);
        if (cur_idx < 0) return 0;
    }

    while (cur_idx >= 0 && cur_idx < static_cast<int>(ls.size())) {
        const auto& lesson = ls[static_cast<std::size_t>(cur_idx)];
        NanaBuffer buf;
        std::istringstream iss(get_draft_or(prog, lesson));
        std::string ln;
        buf.lines.clear();
        while (std::getline(iss, ln)) buf.lines.push_back(ln);
        if (buf.lines.empty()) buf.lines.push_back("");
        buf.filepath = lesson.id + ".kes";

        std::vector<TestResult> results;
        int hint_idx = 0;

        for (;;) {
            auto [rows, cols] = get_terminal_size();
            int rows_e = std::max(rows, 10);
            int cols_e = std::max(cols, 36);
            std::cout << render_learn_workspace(lesson, buf, results, hint_idx, prog.completed, rows, cols, bg_mode)
                      << std::flush;
            std::string k = read_key();
            if (k.empty() || k == "RESIZE") continue;
            buf.status.clear();

            MouseEvent me;
            if (parse_mouse_event(k, me)) {
                int body_rows = rows_e - 4;
                int gutter_w = std::max(static_cast<int>(std::to_string(buf.lines.size()).size()) + 3, 6);
                int total_lines = static_cast<int>(buf.lines.size());
                int code_start_row = 5;
                int ed_rows = std::max(body_rows - 2, 3);
                int code_start_col = gutter_w + 1;
                int editor_min_col = 1;
                if (cols_e >= 92) {
                    int left_w = std::max(36, std::min(52, cols_e * 42 / 100));
                    code_start_row = 3;
                    ed_rows = body_rows;
                    code_start_col = left_w + 1 + gutter_w + 1;
                    editor_min_col = left_w + 2;
                }
                if (me.kind == "WHEEL_UP") {
                    if (buf.scroll_y > 0) {
                        buf.scroll_y = std::max(0, buf.scroll_y - 3);
                        if (buf.row >= buf.scroll_y + ed_rows) buf.row = buf.scroll_y + ed_rows - 1;
                    } else {
                        buf.row = std::max(0, buf.row - 3);
                    }
                    buf.clamp_col();
                    continue;
                }
                if (me.kind == "WHEEL_DOWN") {
                    int max_sy = std::max(0, total_lines - ed_rows);
                    if (buf.scroll_y < max_sy) {
                        buf.scroll_y = std::min(max_sy, buf.scroll_y + 3);
                        if (buf.row < buf.scroll_y) buf.row = buf.scroll_y;
                    } else {
                        buf.row = std::min(total_lines - 1, buf.row + 3);
                    }
                    buf.clamp_col();
                    continue;
                }
                if (me.row == rows_e) {
                    std::string footer = std::string(" ^T Tests  ^R Run  ^H Hint  ^B Brief  ^L Map  ^E Reset  ^X Exit ")
                                             .substr(0, static_cast<std::size_t>(cols_e));
                    std::string mapped = footer_shortcut_hit(footer, me.col, {
                        {"^T Tests", "\x14"},
                        {"^R Run", "\x12"},
                        {"^H Hint", "\x08"},
                        {"^B Brief", "\x02"},
                        {"^L Map", "\x0c"},
                        {"^E Reset", "\x05"},
                        {"^X Exit", "\x18"},
                    });
                    if (mapped.empty()) continue;
                    k = mapped;
                } else if (me.col >= editor_min_col && me.row >= code_start_row && me.row < code_start_row + ed_rows) {
                    int tr = std::min(std::max(0, buf.scroll_y + (me.row - code_start_row)), total_lines - 1);
                    int max_c = static_cast<int>(buf.lines[static_cast<std::size_t>(tr)].size());
                    int tc = std::min(std::max(0, buf.scroll_x + std::max(0, me.col - code_start_col)), max_c);
                    buf.row = tr;
                    buf.col = tc;
                    continue;
                } else {
                    continue;
                }
            }

            if (k == "\x18" || k == "\x11" || k == "\x03") {
                prog.current = lesson.number;
                set_draft(prog, lesson.id, buf.text());
                save_learn_progress(prog);
                return 0;
            }
            if (k == "\x0c") {
                prog.current = lesson.number;
                set_draft(prog, lesson.id, buf.text());
                save_learn_progress(prog);
                int picked = pick_map(cur_idx);
                if (picked < 0) return 0;
                cur_idx = picked;
                break;
            }
            if (k == "\x14") { // Ctrl+T
                results = evaluate_lesson(lesson, buf.text());
                prog.current = lesson.number;
                set_draft(prog, lesson.id, buf.text());
                bool all_ok = !results.empty();
                for (const auto& r : results) if (!r.passed) { all_ok = false; break; }
                if (all_ok) {
                    prog.completed.insert(lesson.id);
                    save_learn_progress(prog);
                    std::ostringstream sum;
                    for (const auto& r : results) sum << "✓ " << r.description << "\n";
                    if (cur_idx + 1 < static_cast<int>(ls.size())) {
                        sum << "\nPress any key to advance to Lesson " << (lesson.number + 1) << "...";
                        show_overlay("PASS · Lesson " + std::to_string(lesson.number) + " Complete", sum.str(), bg_mode);
                        cur_idx++;
                        break;
                    } else {
                        sum << "\nAll 10 Kestral lessons passed!";
                        show_overlay("COURSE COMPLETE!", sum.str(), bg_mode);
                    }
                } else {
                    save_learn_progress(prog);
                    int pn = 0;
                    for (const auto& r : results) if (r.passed) pn++;
                    buf.status = "Tests: " + std::to_string(pn) + "/" + std::to_string(results.size()) + " passed";
                }
                continue;
            }
            if (k == "\x12") { // Ctrl+R
                const auto& tc = lesson.tests.front();
                run_interactive_code(buf.text(), lesson.title, tc.cli_args,
                                     fs::current_path().string(), active_tty);
                continue;
            }
            if (k == "\x08") { // Ctrl+H
                if (hint_idx < static_cast<int>(lesson.hints.size())) {
                    hint_idx++;
                    buf.status = "Hint " + std::to_string(hint_idx) + "/" + std::to_string(lesson.hints.size());
                } else {
                    show_overlay("Reference Solution · Lesson " + std::to_string(lesson.number), lesson.solution, bg_mode);
                }
                continue;
            }
            if (k == "\x02") { // Ctrl+B
                show_overlay("Lesson " + std::to_string(lesson.number) + " · " + lesson.title,
                             format_brief_overlay(lesson, results, hint_idx), bg_mode);
                continue;
            }
            if (k == "\x05") { // Ctrl+E
                bool canc = false;
                std::string ans = prompt_bar("Reset code to starter? (y/n): ", "", &buf, bg_mode, &canc);
                if (!canc && !ans.empty() && (ans[0] == 'y' || ans[0] == 'Y')) {
                    buf.lines.clear();
                    std::istringstream siss(lesson.starter);
                    std::string sln;
                    while (std::getline(siss, sln)) buf.lines.push_back(sln);
                    if (buf.lines.empty()) buf.lines.push_back("");
                    buf.row = 0; buf.col = 0; results.clear(); hint_idx = 0;
                    set_draft(prog, lesson.id, lesson.starter);
                    save_learn_progress(prog);
                    buf.status = "Reset to starter";
                }
                continue;
            }
            if (k == "\x13") { // Ctrl+S
                bool canc = false;
                std::string fn = prompt_bar("Save to .kes file: ", lesson.id + ".kes", &buf, bg_mode, &canc);
                if (!canc && !fn.empty()) {
                    buf.save(fn);
                    buf.filepath = lesson.id + ".kes";
                }
                continue;
            }
            if (k == "\x0b") buf.cut_line();
            else if (k == "\x15") buf.paste_line();
            else if (k == "UP") buf.move_up();
            else if (k == "DOWN") buf.move_down();
            else if (k == "LEFT") buf.move_left();
            else if (k == "RIGHT") buf.move_right();
            else if (k == "HOME") buf.move_home();
            else if (k == "END") buf.move_end();
            else if (k == "DELETE") buf.del();
            else if (k == "\x7f") buf.backspace();
            else if (k == "\r" || k == "\n") buf.insert_newline();
            else if (k == "\t") buf.insert_tab();
            else if (k.size() == 1 && static_cast<unsigned char>(k[0]) >= 32 && k[0] != 127) {
                buf.insert_char(k[0]);
            }
        }
    }
    return 0;
}

} // namespace kestral
