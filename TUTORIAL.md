# Kestral Tutorial — From Zero to Real CLI Tools

Welcome to **Kestral**, a fast, readable language stored in `.kes` files.
You can follow every lesson directly inside your terminal using the interactive split-pane course:

```sh
kestral learn
```

Or open the minimalist **`nana`** editor anytime:

```sh
kestral nana
```

---

## Lesson 1: Hello & Text Cards

Strings interpolate any expression inside `{braces}`, and values flow left-to-right with `|>`:

```kes
let hero = "Kestral"
say "Hello, {hero}!" |> box("Greeting")
```

Run it with `kestral hello.kes`.

---

## Lesson 2: Functions & Defaults

Functions can use full `{ ... }` blocks with `give`, or concise `=>` single-expression bodies. Default parameter values are written right in the signature, and functions are hoisted cleanly across your file:

```kes
fn greet(name = "world", mark = "!") => "Hello, {name}{mark}"
say greet()
say greet("Ada", "?")
```

---

## Lesson 3: `if` Expressions & `match`

Both `if` and `match` evaluate to values. `match` handles values, comma lists (`1, 2, 3`), inclusive integer ranges (`80..89`), and `else`:

```kes
fn grade(score) {
  give match score {
    90..100 => "A"
    80..89 => "B"
    else => "C"
  }
}
```

---

## Lesson 4: Loops, Ranges & Dot Methods

Any built-in or user function can be chained using `.`:

```kes
let evens = filter(1..6, fn(x) => x % 2 == 0)
let squares = map(evens, fn(x) => x * x)
say join(squares, ", ")
```

---

## Lesson 5: Lists, Maps & Word Counts

Maps preserve insertion order. Missing keys return `nothing`, making `??` (`nothing`-coalescing) ideal for counters:

```kes
let counts = {}
for w in split("go kes go fast go") {
  counts[w] = (counts[w] ?? 0) + 1
}
say counts.go, counts.kes
```

---

## Lesson 6: Closures & State

Return a map of closures to create lightweight objects with private state:

```kes
fn makeWallet(start = 0) {
  let bal = start
  give {
    deposit: fn(amt) { if amt <= 0 { fail "must be positive" }; bal += amt },
    balance: fn() => bal
  }
}
```

---

## Lesson 7: CLI Arguments & Boxed Output

Use `args()` to read arguments passed after your script name:

```kes
let a = args()
let who = if len(a) > 0 { a[0] } else { "friend" }
if len(a) > 1 and a[1] == "-u" {
  who = upper(who)
}
say "Hi {who}!" |> box("CLI")
```

Run with `kestral greet.kes Ada -u`.

---

## Lesson 8: Files & Error Recovery

Read and write files cleanly, and recover from failures with `try { ... } catch err { ... }`:

```kes
writeFile("note.txt", "kestral:2.0")
if fileExists("note.txt") {
  say readFile("note.txt")
}
try {
  readFile("missing.txt")
} catch e {
  say "Recovered: {e}"
}
```

---

## Lesson 9: Interactive Input Loops

`ask()` reads a line from stdin (or returns `nothing` on EOF):

```kes
let items = []
while true {
  let ln = ask("> ")
  if ln == nothing or ln == "done" { stop }
  push(items, ln)
}
say "Items ({len(items)}): {join(items, ", ")}"
```

---

## Lesson 10: Capstone — Number Cruncher

Put everything together in a fault-tolerant CLI statistics cruncher:

```kes
let numbers = []

while true {
  let line = ask()
  if line == nothing or line == "done" {
    stop
  }
  try {
    push(numbers, num(line))
  } catch e {
    say "Skipping: {line}"
  }
}

if len(numbers) == 0 {
  say "No numbers entered."
} else {
  say "Count: {len(numbers)}"
  say "Total: {sum(numbers)}"
  say "Average: {round(sum(numbers) / len(numbers), 2)}"
  say "Largest: {max(numbers)}"
}
```

Run `kestral learn 10` to solve and verify the Capstone live in your terminal!
