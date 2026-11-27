# Kestral Language Reference

## Values & Types

| Type | Examples | Notes |
| --- | --- | --- |
| `number` | `42`, `3.14`, `-7` | Double-precision with tidy printing (`0.1 + 0.2 == 0.3`) |
| `text` | `"hello"`, `'hi'`, `"sum={1+2}"` | Supports `{expr}` interpolation and escapes (`\n`, `\t`, `\{`, `\}`, `\\`) |
| `bool` | `true`, `false` | Truthy except `false`, `nothing`, `0`, `""` |
| `nothing` | `nothing` | Returned by missing map keys, empty functions, or EOF `ask()` |
| `list` | `[1, "two", true]` | Reference semantics (`copy()` clones deeply), negative indexing (`xs[-1]`) |
| `map` | `{name: "Ada", 1: "one"}` | Insertion-ordered key/value store |
| `range` | `1..10`, `5..1` | Inclusive integer range; indexable and iterable |
| `function` | `fn(x) => x * 2` | First-class lexical closures |

## Operators (Precedence High to Low)

1. Call `()`, index `[]`, member `.` (supports multiline `.method()` chains)
2. `**` (right-associative power)
3. Unary `-`
4. `*`, `/`, `%` (floored modulo matching Python/Kestral spec)
5. `+` (numbers, text concatenation, list concatenation), `-`
6. `..` (inclusive range)
7. `==`, `!=`, `<`, `<=`, `>`, `>=`
8. `not`
9. `and` (short-circuiting, returns deciding operand)
10. `or` (short-circuiting, returns deciding operand)
11. `??` (`nothing`-coalescing)
12. `|>` (pipeline: `x |> f(y)` calls `f(x, y)`)

## Control Flow & Functions

- **Conditionals**: `if cond { ... } else if cond2 { ... } else { ... }` — evaluates to the last value of the taken branch.
- **Match**: `match expr { pat1 => val  pat2, pat3 => val  1..5 => { ... }  else => fallback }`
- **Loops**: `loop N times { ... }`, `while cond { ... }`, `for item in iter { ... }`, `for idx_or_key, val in iter { ... }`, with `stop` and `skip`.
- **Functions**: hoisted in their enclosing block; default parameters; `give` returns early; omitting `give` returns `nothing`.
- **Modules**: `use "tools.kes"` loads another file relative to the running script.
- **Errors**: `fail "reason"` and `try { ... } catch err { ... }`. Runaway loop/recursion limits are fatal and cannot be caught.
