# Kestral Built-in Functions Reference

Every built-in works both standalone (`upper("hi")`, `[3, 1].sort()`) and in `|>` pipelines (`"hi" |> upper`).

## Input & Conversion
- `ask(prompt?)` — read a line from stdin (`nothing` on EOF)
- `type(value)` — type name (`"number"`, `"text"`, `"bool"`, `"list"`, `"map"`, `"range"`, `"function"`, `"nothing"`)
- `str(value)` — format value as text
- `num(text)` — parse text into a number (fails on invalid text)
- `int(value)` — truncate decimals from number or numeric text
- `list(value)` — convert range, text, or map keys into a list
- `copy(value)` — deep independent copy of a list or map

## Math
- `abs(n)`, `floor(n)`, `ceil(n)`, `sqrt(n)`
- `round(n, digits?)` — half-away-from-zero rounding (supports negative `digits`)
- `min(a, ...)` / `min(list)` and `max(a, ...)` / `max(list)`
- `sum(list)` — sum numeric list or range
- `random()` — float in `[0, 1)`
- `randomInt(low, high)` — inclusive integer in `[low, high]`

## Lists, Maps & Ranges
- `len(val)` — item/char/key/element count
- `push(list, vals...)`, `pop(list)`, `remove(list_or_map, key_or_index)`
- `slice(seq, from, to?)`, `reverse(seq)`
- `sort(seq, key_fn?)`, `unique(seq)`
- `keys(map)`, `values(map)`, `entries(map)`
- `contains(container, item)`, `find(seq, item)`

## Higher-Order Functions
- `map(seq, fn)` — passes `(item)` or `(item, index)` when `fn` takes 2+ params
- `filter(seq, fn)`, `reduce(seq, fn, start?)`, `any(seq, fn)`, `all(seq, fn)`

## Text
- `upper(s)`, `lower(s)`, `trim(s)`
- `split(s, sep?)`, `join(seq, sep?)`, `replace(s, old, new)`
- `startsWith(s, prefix)`, `endsWith(s, suffix)`, `repeat(s, count)`

## CLI, Files & Terminal Helpers
- `args()` — CLI arguments passed after the `.kes` script
- `env(name)` — environment variable (`nothing` if unset)
- `fileExists(path)`, `readFile(path)`, `writeFile(path, content)`
- `box(text, title?)` — wrap output in rounded Unicode box
- `color(text, style)` — apply ANSI color (`red`, `green`, `yellow`, `blue`, `magenta`, `cyan`, `white`, `gray`, `bold`, `dim`)
- `clear()` — clear terminal screen
- `assert(cond, msg?)` — fail with `msg` if falsy
