==> title
When Things Go Wrong
==> goal
Read error messages, and handle problems on purpose
==> intro
Every programmer sees errors all day long. They are not failures, they are directions.

## Reading an error

```
Error on line 3: I don't know "scor". Did you mean "score"?
  3 | say scor
    |     ^
```

It tells you the **line**, shows the line itself, points at the spot with `^`, and often suggests a fix. Always read the message before you change anything.

There are two families:

- **Syntax errors**: the code is not shaped correctly (a missing `}`, an unclosed quote). Kestral finds these before running anything.
- **Runtime errors**: the shape is fine, but something impossible happens while running (dividing by zero, a missing name, a list position that does not exist).

## try and catch

When something might fail, wrap it in `try`. If it fails, Kestral jumps to `catch` instead of stopping your program.

```kes
try {
  let n = num("not a number")
  say n
} catch problem {
  say "Oops: {problem}"
}
say "The program keeps going."
```

`problem` is a name you choose. It holds the error message as text.

## Making your own errors with fail

Use `fail` when your code notices something wrong.

```kes
fn safeDivide(a, b) {
  if b == 0 {
    fail "cannot divide {a} by zero"
  }
  give a / b
}

try {
  say safeDivide(10, 2)
  say safeDivide(1, 0)
} catch e {
  say "Problem: {e}"
}
```

## assert

`assert(condition, message)` is a quick sanity check. It does nothing when the condition is true and fails with your message when it is false.

```kes
let age = 20
assert(age >= 0, "age cannot be negative")
say "age is fine"
```

## Runaway programs

Kestral stops programs that loop forever or recurse without end, and explains why. Those errors cannot be caught with `try`, because the program is not behaving, it is stuck.
==> challenge
Write a function `parseAge(text)` that turns text into an age:

- If `text` is a valid number that is 0 or more, give back the number.
- Otherwise give back `-1`.

So `parseAge("42")` is `42`, while `parseAge("abc")`, `parseAge("")` and `parseAge("-5")` are all `-1`.
==> starter
fn parseAge(text) {
  // your code
}

say parseAge("42")
say parseAge("abc")
==> solution
fn parseAge(text) {
  try {
    let n = num(text)
    if n < 0 {
      give -1
    }
    give n
  } catch e {
    give -1
  }
}
==> hints
- `num(text)` fails on bad text, so put it inside `try`.
- Check for negative numbers separately. `-5` converts fine but is not a valid age.
==> tests
[
 {"append": "say parseAge(\"42\")\nsay parseAge(\"3.5\")\nsay parseAge(\"0\")", "expect": "42\n3.5\n0"},
 {"append": "say parseAge(\"abc\")\nsay parseAge(\"\")\nsay parseAge(\"-5\")", "expect": "-1\n-1\n-1"}
]
