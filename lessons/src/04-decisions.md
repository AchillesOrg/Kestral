==> title
Making Decisions
==> goal
Run code only when something is true
==> intro
Programs get interesting when they can choose. Use `if`, and optionally `else`.

```kes
let temp = 31
if temp > 30 {
  say "Hot day. Drink water."
} else if temp > 18 {
  say "Nice day."
} else {
  say "Bring a jacket."
}
```

The code in `{ }` only runs when its condition is true. Kestral checks the conditions from the top and uses the first one that is true.

## Comparing

`==` equal, `!=` not equal, `<` `<=` `>` `>=`. Note that a single `=` stores a value, and a double `==` asks a question.

```kes
say 5 == 5, 5 != 5, 3 < 4, "apple" < "banana"
```

## and, or, not

```kes
let age = 25
let hasTicket = true
if age >= 18 and hasTicket {
  say "Welcome in."
}
if age < 13 or age > 65 {
  say "Discount price."
} else {
  say "Full price."
}
say not hasTicket
```

## What counts as true?

In a condition, these are **false**: `false`, `nothing`, `0`, and empty text `""`. Everything else is true. That is why `if name { ... }` means "if there is a name".

## if as a value

`if` can produce a value, which is useful for small choices:

```kes
let n = 7
let kind = if n % 2 == 0 { "even" } else { "odd" }
say "{n} is {kind}"
```
==> challenge
Read a score (a number) and print the letter grade:

- 90 or more: `A`
- 80 to 89: `B`
- 70 to 79: `C`
- 60 to 69: `D`
- below 60: `F`
==> starter
let score = num(ask())
// say the grade letter
==> solution
let score = num(ask())
if score >= 90 {
  say "A"
} else if score >= 80 {
  say "B"
} else if score >= 70 {
  say "C"
} else if score >= 60 {
  say "D"
} else {
  say "F"
}
==> hints
- Check the highest grade first. Because only the first true condition runs, `score >= 90` goes before `score >= 80`.
- Watch the boundaries. A score of exactly 80 is a `B`, so use `>=`, not `>`.
==> tests
[
 {"input": ["95"], "expect": "A"},
 {"input": ["90"], "expect": "A"},
 {"input": ["80"], "expect": "B"},
 {"input": ["72"], "expect": "C"},
 {"input": ["60"], "expect": "D"},
 {"input": ["59"], "expect": "F"}
]
