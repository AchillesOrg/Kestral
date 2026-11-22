==> title
Functions
==> goal
Give a name to a piece of work and reuse it
==> intro
A **function** is a named recipe. You write it once with `fn`, then use it as often as you like.

```kes
fn greet(name) {
  say "Hello, {name}!"
}

greet("Ada")
greet("Grace")
```

`name` is a **parameter**: a name that holds whatever value you pass in. Calling `greet("Ada")` runs the body with `name` set to `"Ada"`.

## Giving a value back

Use `give` to send a result back to the caller. (Some languages call this `return`.)

```kes
fn square(n) {
  give n * n
}
say square(7)
say square(square(2))
```

A function with no `give` hands back `nothing`.

## One-line functions

When the whole body is one expression, use `=>`:

```kes
fn add(a, b) => a + b
say add(2, 3)
```

## Default values

```kes
fn greet(name = "friend", mark = "!") {
  give "Hello, {name}{mark}"
}
say greet()
say greet("Ada")
say greet("Ada", "?")
```

## Scope

Names created inside a function belong to that function and disappear when it finishes. A function can **read** names from the outside, which is how it can use other functions.

```kes
fn describe(n) {
  let kind = if n % 2 == 0 { "even" } else { "odd" }
  give "{n} is {kind}"
}
say describe(4)
// say kind    <- this would be an error: kind only exists inside describe
```

## Functions can call themselves

This is called **recursion**. It needs a case that stops.

```kes
fn factorial(n) {
  if n <= 1 {
    give 1
  }
  give n * factorial(n - 1)
}
say factorial(10)
```

You can also use a function before the line where it is written. Kestral reads the whole file first, so put your main program at the top if you like.
==> challenge
Write two functions:

- `double(n)` gives back `n` times 2
- `isEven(n)` gives back `true` if `n` is even, otherwise `false`

You can add your own `say` lines to try them out. The checker runs its own calls after your code.
==> starter
fn double(n) {
  // your code
}

fn isEven(n) {
  // your code
}

// try them:
say double(4)
say isEven(7)
==> solution
fn double(n) {
  give n * 2
}

fn isEven(n) {
  give n % 2 == 0
}
==> hints
- A function gives its answer back with `give`. If you forget, you get `nothing`.
- `n % 2 == 0` is already `true` or `false`, so you can give it back directly.
==> tests
[
 {"append": "say double(4)\nsay double(0)\nsay double(-3)", "expect": "8\n0\n-6"},
 {"append": "say isEven(4)\nsay isEven(7)\nsay isEven(0)", "expect": "true\nfalse\ntrue"}
]
