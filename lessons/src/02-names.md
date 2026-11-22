==> title
Names and Values
==> goal
Store things with let, and talk to the user with ask
==> intro
A **variable** is a name that holds a value. You create one with `let`.

```kes
let name = "Ada"
let age = 36
say "{name} is {age} years old."
```

Inside text, anything in `{curly braces}` is replaced by its value. This is called **interpolation**, and you can put any expression in there: `"{age + 1} next year"`.

## Changing a value

After a name exists, change it with `=` (no `let` needed).

```kes
let score = 10
score = score + 5
score += 1
say score
```

`score += 1` is a short way to write `score = score + 1`. The same idea works for `-=`, `*=`, `/=`, and `%=`.

## Kinds of values

Every value has a type. Ask Kestral with `type()`.

```kes
say type(42), type("hi"), type(true), type(nothing)
```

- **number**: `42`, `3.14`, `-7`
- **text**: `"hello"` or `'hello'`
- **bool**: `true` or `false`
- **nothing**: the value that means "no value yet"

## Naming rules

Names use letters, digits and `_`, and cannot start with a digit. Good names say what the value is: `totalPrice`, `user_name`. Kestral is case sensitive, so `Age` and `age` are different names.

## Asking the user

`ask` waits for the user to type a line and gives it back as **text**.

```kes input=Ada
let who = ask("What is your name? ")
say "Nice to meet you, {who}!"
```

> In the Studio app, type the answers into the **Input** box (one answer per line) before you press Run.

Even if the user types `5`, `ask` gives you the text `"5"`. To do math, convert it with `num`:

```kes input=21
let n = num(ask("Pick a number: "))
say n * 2
```
==> challenge
Write a program that asks for a name, then asks for an age, and then prints:

```
Hello, Ada! Next year you will be 31.
```

(That is the output when the name is `Ada` and the age is `30`.) Ask for the name first, then the age. Do not print the questions: pass nothing to `ask`, or leave the text out of what you `say`.
==> starter
let name = ask()
// ask for the age next, then say the sentence
==> solution
let name = ask()
let age = num(ask())
say "Hello, {name}! Next year you will be {age + 1}."
==> hints
- The age arrives as text. Wrap it in `num( ... )` so you can add 1.
- Use interpolation: `"Hello, {name}! ..."`, and put `{age + 1}` where the number goes.
==> tests
[
 {"input": ["Ada", "30"], "expect": "Hello, Ada! Next year you will be 31."},
 {"input": ["Kai", "9"], "expect": "Hello, Kai! Next year you will be 10."}
]
