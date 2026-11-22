# Learn Kestral

Kestral is a small, friendly, general-purpose programming language. Programs are plain text files that end in `.kes`. It is designed to be read easily, to give helpful error messages, and to be learned in a couple of weeks of short sessions.

This tutorial takes you from "what is a program?" to writing real programs. It has 13 lessons. Each one has:

- a short explanation with examples you can run
- a **challenge** that the Studio app can check for you automatically
- hints, and a full solution for when you are stuck

## A taste

```kes
fn greet(name = "friend") => "Hello, {name}!"

let people = ["Ada", "Grace", "Linus"]
for person in people {
  say greet(person)
}

let lengths = people |> map(fn(p) => len(p))
say "Name lengths: {lengths}"
```

## Two ways to run Kestral

**1. Kestral Studio (easiest).** A single page with an editor, a Run button, an Input box and the whole course built in. Nothing to install. Your progress and code are remembered in your browser.

**2. The command line.** You need [Node.js](https://nodejs.org) 16 or newer. In this folder:

```
node bin/kestral.js hello.kes       run a file
node bin/kestral.js                 start the interactive REPL
node bin/kestral.js -e "say 6 * 7"  run one line
```

Make a file called `hello.kes` containing `say "Hello from a file!"` and run it. That is your first real Kestral program outside the Studio. The `examples` folder has more programs to read and run.

## How to learn from this

- **Type the examples; do not paste them.** Your fingers learn syntax faster than your eyes do.
- **Change things.** After an example works, break it on purpose and see what the error says.
- **Do the challenge before you look at the hints.** Struggling for ten minutes is where the learning happens. Then use one hint at a time.
- **Little and often beats a marathon.** One lesson a day, with a short break, is a good rhythm. Thirteen lessons is about two weeks.
- **Run often.** Run your program after every small change, not after fifty lines.

---

## Contents

1. [Hello, Kestral](#lesson-1-hello-kestral) - Print your first lines with say
2. [Names and Values](#lesson-2-names-and-values) - Store things with let, and talk to the user with ask
3. [Numbers and Text](#lesson-3-numbers-and-text) - Do math, join text, and round results
4. [Making Decisions](#lesson-4-making-decisions) - Run code only when something is true
5. [Loops](#lesson-5-loops) - Repeat work without repeating yourself
6. [Functions](#lesson-6-functions) - Give a name to a piece of work and reuse it
7. [Lists](#lesson-7-lists) - Keep many values in one place
8. [Maps](#lesson-8-maps) - Look values up by name
9. [Working with Text](#lesson-9-working-with-text) - Slice, search and reshape words
10. [Functions as Values](#lesson-10-functions-as-values) - Pass functions around, and chain them into pipelines
11. [When Things Go Wrong](#lesson-11-when-things-go-wrong) - Read error messages, and handle problems on purpose
12. [Matching](#lesson-12-matching) - Choose between many cases cleanly
13. [Capstone: Number Cruncher](#lesson-13-capstone-number-cruncher) - Put everything together in a real program


---

## Lesson 1: Hello, Kestral

A program is a list of instructions. Kestral reads them from top to bottom and does each one. Kestral programs are saved in files that end in `.kes`.

## say

The `say` instruction prints something on the screen. Text goes inside quotes.

```kes
say "Hello, world!"
say "Kestral is small and friendly."
```

Press **Run** to see what it prints. Then change the words and run it again. You cannot break anything.

## More than one value

You can give `say` several values separated by commas. Kestral puts a space between them.

```kes
say "I have", 3, "apples"
say 2 + 3
```

Numbers do not need quotes. Kestral does the math first, so `say 2 + 3` prints `5`.

## Comments

Anything after `//` is a note for humans. Kestral ignores it.

```kes
// This line is only a note
say "This line runs" // so does this part, but not the note
```

## When something is wrong

Kestral tells you the line and what it did not understand. Here is a mistake on purpose (the word `say` is misspelled):

```kes
// error: this example is supposed to fail
say "This line is fine"
sya "This one is not"
```

Read the message, fix the spelling, run again. Reading error messages is a real skill, and you will get fast at it.

> **Rule to remember:** one instruction per line.

### Your turn

Print exactly these two lines, in this order:

```
Kestral flies!
Lesson 1 done
```

Press **Check answer** when your output matches.

Starter code:

```kes
// Write your two say lines below
```

<details><summary>Hints</summary>

- Each line you want to print needs its own `say`.
- Text must be inside double quotes, and match exactly, including the capital letters and the exclamation mark.

</details>

<details><summary>Solution (try first!)</summary>

```kes
say "Kestral flies!"
say "Lesson 1 done"
```

</details>

---

## Lesson 2: Names and Values

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

```kes
let who = ask("What is your name? ")
say "Nice to meet you, {who}!"
```

> In the Studio app, type the answers into the **Input** box (one answer per line) before you press Run.

Even if the user types `5`, `ask` gives you the text `"5"`. To do math, convert it with `num`:

```kes
let n = num(ask("Pick a number: "))
say n * 2
```

### Your turn

Write a program that asks for a name, then asks for an age, and then prints:

```
Hello, Ada! Next year you will be 31.
```

(That is the output when the name is `Ada` and the age is `30`.) Ask for the name first, then the age. Do not print the questions: pass nothing to `ask`, or leave the text out of what you `say`.

Starter code:

```kes
let name = ask()
// ask for the age next, then say the sentence
```

<details><summary>Hints</summary>

- The age arrives as text. Wrap it in `num( ... )` so you can add 1.
- Use interpolation: `"Hello, {name}! ..."`, and put `{age + 1}` where the number goes.

</details>

<details><summary>Solution (try first!)</summary>

```kes
let name = ask()
let age = num(ask())
say "Hello, {name}! Next year you will be {age + 1}."
```

</details>

---

## Lesson 3: Numbers and Text

## Math

```kes
say 7 + 2, 7 - 2, 7 * 2, 7 / 2
say 7 % 2
say 2 ** 10
```

- `%` is the **remainder** after division. `7 % 2` is `1`, so `n % 2 == 0` is a handy test for even numbers.
- `**` means "to the power of".
- Brackets work like in school: `(2 + 3) * 4` is `20`.
- `*` and `/` happen before `+` and `-`.
- Dividing by zero is an error with a clear message, not a mystery number.

## Tidy decimals

Computers store decimals in binary, so tiny errors are normal in many languages. Kestral prints numbers tidily, and `round` gives you control.

```kes
say 0.1 + 0.2
say round(3.14159, 2)
say round(2.5)
say floor(2.9), ceil(2.1)
say abs(-5), sqrt(81), min(4, 2, 9), max(4, 2, 9)
```

`round(x, 2)` keeps two decimal places. `floor` rounds down, `ceil` rounds up.

## Joining text

`+` joins text. If either side is text, the other side is turned into text.

```kes
let first = "Ada"
let last = "Lovelace"
say first + " " + last
say "Score: " + 10
say "ha" * 3
```

Interpolation (`"{first} {last}"`) is usually neater than joining with `+`.

## Length of text

```kes
say len("kestral")
```

### Your turn

Write a program that asks for a **price** and then a **quantity**, and prints the total including 10% tax, rounded to 2 decimal places, like this:

```
Total: 22
```

(That is the output for a price of `10` and a quantity of `2`: 10 x 2 = 20, plus 10% is 22.) Note that `round` already gives you a clean number, so do not worry about trailing zeros.

Starter code:

```kes
let price = num(ask())
// ask for the quantity next, then work out and say the total
```

<details><summary>Hints</summary>

- Adding 10% is the same as multiplying by `1.1`.
- `round(total, 2)` rounds to two decimal places.

</details>

<details><summary>Solution (try first!)</summary>

```kes
let price = num(ask())
let qty = num(ask())
let total = price * qty * 1.1
say "Total: {round(total, 2)}"
```

</details>

---

## Lesson 4: Making Decisions

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

### Your turn

Read a score (a number) and print the letter grade:

- 90 or more: `A`
- 80 to 89: `B`
- 70 to 79: `C`
- 60 to 69: `D`
- below 60: `F`

Starter code:

```kes
let score = num(ask())
// say the grade letter
```

<details><summary>Hints</summary>

- Check the highest grade first. Because only the first true condition runs, `score >= 90` goes before `score >= 80`.
- Watch the boundaries. A score of exactly 80 is a `B`, so use `>=`, not `>`.

</details>

<details><summary>Solution (try first!)</summary>

```kes
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
```

</details>

---

## Lesson 5: Loops

Computers are good at repeating things. Kestral has three loops.

## loop N times

```kes
loop 3 times {
  say "Kestral!"
}
```

## for ... in

`for` walks through a collection. `1..5` is a **range** that counts from 1 to 5, **including both ends**.

```kes
for i in 1..5 {
  say "Number {i}"
}
for letter in "abc" {
  say letter
}
for fruit in ["apple", "pear", "plum"] {
  say "I like {fruit}"
}
```

A range can also count down: `5..1`.

## while

`while` repeats as long as its condition is true. Make sure something inside changes the condition, or it will never end. (Kestral stops runaway programs for you and tells you so.)

```kes
let n = 1
while n < 100 {
  n = n * 2
}
say n
```

## stop and skip

- `stop` leaves the loop immediately.
- `skip` jumps to the next round.

```kes
for i in 1..10 {
  if i % 2 == 0 {
    skip
  }
  if i > 7 {
    stop
  }
  say i
}
```

## Building up a result

A very common pattern: start with a value, update it in the loop.

```kes
let total = 0
for i in 1..100 {
  total += i
}
say total
```

### Your turn

Print the numbers from 1 to 15, one per line, but:

- for multiples of 3, print `Fizz` instead of the number
- for multiples of 5, print `Buzz` instead
- for multiples of both 3 and 5, print `FizzBuzz`

This is the famous FizzBuzz puzzle. The first lines are `1`, `2`, `Fizz`, `4`, `Buzz`.

Starter code:

```kes
for i in 1..15 {
  // decide what to say
}
```

<details><summary>Hints</summary>

- `i % 3 == 0` is true when `i` is a multiple of 3.
- Check the "both" case (multiple of 15) **first**. Otherwise a 15 would print just `Fizz`.

</details>

<details><summary>Solution (try first!)</summary>

```kes
for i in 1..15 {
  if i % 15 == 0 {
    say "FizzBuzz"
  } else if i % 3 == 0 {
    say "Fizz"
  } else if i % 5 == 0 {
    say "Buzz"
  } else {
    say i
  }
}
```

</details>

---

## Lesson 6: Functions

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

### Your turn

Write two functions:

- `double(n)` gives back `n` times 2
- `isEven(n)` gives back `true` if `n` is even, otherwise `false`

You can add your own `say` lines to try them out. The checker runs its own calls after your code.

Starter code:

```kes
fn double(n) {
  // your code
}

fn isEven(n) {
  // your code
}

// try them:
say double(4)
say isEven(7)
```

<details><summary>Hints</summary>

- A function gives its answer back with `give`. If you forget, you get `nothing`.
- `n % 2 == 0` is already `true` or `false`, so you can give it back directly.

</details>

<details><summary>Solution (try first!)</summary>

```kes
fn double(n) {
  give n * 2
}

fn isEven(n) {
  give n % 2 == 0
}
```

</details>

---

## Lesson 7: Lists

A **list** holds values in order. Write one with square brackets.

```kes
let fruits = ["apple", "pear", "plum"]
say fruits
say len(fruits)
say fruits[0]
say fruits[2]
say fruits[-1]
```

Positions start at **0**. A negative position counts from the end, so `fruits[-1]` is the last item.

## Changing a list

```kes
let nums = [10, 20, 30]
nums[1] = 99
push(nums, 40)
say nums
say pop(nums)
say nums
```

- `push(list, value)` adds to the end.
- `pop(list)` removes and gives back the last item.
- `remove(list, position)` removes the item at a position.

## Looping over a list

```kes
let scores = [88, 92, 75]
let total = 0
for s in scores {
  total += s
}
say "Average: {total / len(scores)}"
```

If you want the position too, name two things:

```kes
for i, name in ["Ada", "Grace"] {
  say "{i}: {name}"
}
```

## Handy tools

```kes
let nums = [5, 3, 8, 1]
say sort(nums)
say reverse(nums)
say sum(nums), min(nums), max(nums)
say slice(nums, 1, 3)
say contains(nums, 8)
say [1, 2] + [3]
```

`slice(list, from, to)` copies part of a list (the `to` position is not included). `sort` and `reverse` give back a **new** list and leave the original alone.

## A list is shared, not copied

```kes
let a = [1, 2]
let b = a
push(b, 3)
say a
let c = copy(a)
push(c, 4)
say a, c
```

`a` and `b` are the same list under two names. Use `copy` when you want an independent one.

## Lists inside lists

```kes
let grid = [[1, 2], [3, 4]]
say grid[1][0]
```

### Your turn

Write a function `average(numbers)` that gives back the average of a list of numbers. Use a loop (or `sum` if you like). You can assume the list is not empty.

Starter code:

```kes
fn average(numbers) {
  // your code
}

say average([2, 4, 6])
```

<details><summary>Hints</summary>

- Add everything up first, then divide by `len(numbers)`.
- Make sure the `give` is **after** the loop, not inside it.

</details>

<details><summary>Solution (try first!)</summary>

```kes
fn average(numbers) {
  let total = 0
  for n in numbers {
    total += n
  }
  give total / len(numbers)
}
```

</details>

---

## Lesson 8: Maps

A **map** stores values under keys, like a tiny address book. Write one with curly braces.

```kes
let person = {name: "Ada", age: 36, languages: ["math", "poetry"]}
say person
say person.name
say person["age"]
say person.languages[0]
```

Use `.key` when the key is a simple name, or `["any key"]` when it is not.

## Changing a map

```kes
let pet = {name: "Kiwi"}
pet.age = 3
pet["color"] = "green"
pet.age += 1
say pet
```

## Missing keys give nothing

```kes
let m = {a: 1}
say m.b
say m.b ?? "no b here"
```

`??` means "use the right side if the left side is `nothing`". It is perfect for default values.

## Looking inside

```kes
let stock = {apples: 5, pears: 0}
say keys(stock)
say values(stock)
say len(stock)
say contains(stock, "apples")
for item, count in stock {
  say "{item}: {count}"
}
```

## Counting things

A map plus `??` is the classic way to count how often something appears:

```kes
let votes = {}
for choice in ["red", "blue", "red", "red"] {
  votes[choice] = (votes[choice] ?? 0) + 1
}
say votes
```

## Maps hold anything, including functions

```kes
let calc = {
  add: fn(a, b) => a + b,
  name: "tiny calculator"
}
say calc.name
say calc.add(2, 3)
```

Maps keep their keys in the order you added them.

### Your turn

Write a function `wordCount(text)` that gives back a map from each word to how many times it appears. Treat upper and lower case as the same word, so `"Go go GO"` counts `go` three times.

Hint: `split(text)` splits text into words, and `lower(text)` makes it lower case.

Starter code:

```kes
fn wordCount(text) {
  let counts = {}
  // your code
  give counts
}

say wordCount("the cat and the dog")
```

<details><summary>Hints</summary>

- Loop over `split(lower(text))`.
- Inside the loop: `counts[word] = (counts[word] ?? 0) + 1`.

</details>

<details><summary>Solution (try first!)</summary>

```kes
fn wordCount(text) {
  let counts = {}
  for word in split(lower(text)) {
    counts[word] = (counts[word] ?? 0) + 1
  }
  give counts
}
```

</details>

---

## Lesson 9: Working with Text

Text is a row of characters. Many of the list tools work on text too.

```kes
let word = "kestral"
say len(word)
say word[0]
say word[-1]
say slice(word, 0, 3)
say slice(word, 3)
for ch in word {
  say ch
}
```

Text cannot be changed in place, but every tool below gives you a **new** piece of text.

## Changing case and trimming

```kes
say upper("hello"), lower("HELLO")
say "[" + trim("   padded   ") + "]"
```

## Searching

```kes
let s = "the quick brown fox"
say contains(s, "quick")
say find(s, "brown")
say find(s, "zebra")
say startsWith(s, "the"), endsWith(s, "dog")
```

`find` gives the position of the first match, or `-1` when there is none.

## Splitting and joining

`split` turns text into a list of pieces. `join` does the opposite.

```kes
say split("a,b,c", ",")
say split("  many   spaces here ")
say split("abc", "")
say join(["a", "b", "c"], "-")
say join([1, 2, 3])
```

With no separator, `split` cuts at spaces and ignores extra ones. With `""` it gives you every character.

## Replacing and repeating

```kes
say replace("2024-01-15", "-", "/")
say repeat("=-", 5)
say reverse("stressed")
```

## Method style

Any built-in function can be written with a dot, with the first value in front. These two lines mean the same thing:

```kes
say upper("hi")
say "hi".upper()
```

Dots make long chains easy to read from left to right:

```kes
say "  Hello, World  ".trim().lower().replace("world", "kestral")
```

### Your turn

Write two functions:

- `initials(name)` gives back the first letter of every word, capitalized, each followed by a dot. `initials("ada lovelace")` is `"A.L."`.
- `isPalindrome(text)` gives back `true` if the text reads the same forwards and backwards, ignoring upper and lower case. `"Racecar"` is a palindrome and `"Hello"` is not.

Starter code:

```kes
fn initials(name) {
  // your code
}

fn isPalindrome(text) {
  // your code
}

say initials("ada lovelace")
say isPalindrome("Racecar")
```

<details><summary>Hints</summary>

- For `initials`: loop over `split(name)`, and use `word[0]` for the first letter.
- Build the answer by adding to a text value that starts as `""`.
- For `isPalindrome`: compare `lower(text)` with `reverse(lower(text))`.

</details>

<details><summary>Solution (try first!)</summary>

```kes
fn initials(name) {
  let result = ""
  for word in split(name) {
    result += upper(word[0]) + "."
  }
  give result
}

fn isPalindrome(text) {
  let t = lower(text)
  give t == reverse(t)
}
```

</details>

---

## Lesson 10: Functions as Values

In Kestral a function is a value, just like a number or text. You can store it, pass it to other functions and give it back.

## Anonymous functions

`fn(...) => ...` makes a function without a name.

```kes
let triple = fn(x) => x * 3
say triple(5)
```

## map, filter, reduce

These three tools take a list and a function. They do the looping for you.

```kes
let nums = [1, 2, 3, 4, 5, 6]
say map(nums, fn(n) => n * n)
say filter(nums, fn(n) => n % 2 == 0)
say reduce(nums, fn(total, n) => total + n, 0)
say any(nums, fn(n) => n > 5), all(nums, fn(n) => n > 5)
```

- `map` makes a new list by applying the function to each item.
- `filter` keeps only the items where the function is true.
- `reduce` folds the list into one value. The function gets the running total and the next item.

You can also sort by a rule:

```kes
let words = ["pear", "fig", "banana"]
say sort(words, fn(w) => len(w))
```

## Pipelines

`|>` feeds the value on its left into the function on its right. It lets you read a chain of steps from top to bottom.

```kes
let result = [5, 2, 8, 1, 9, 3]
  |> filter(fn(n) => n > 2)
  |> sort
  |> map(fn(n) => n * 10)
say result
```

When the right side is a call like `filter(fn...)`, the piped value becomes its **first** argument.

## Functions that make functions

A function can give back another function. The inner function remembers the values around it. This is called a **closure**.

```kes
fn makeCounter() {
  let count = 0
  give fn() {
    count += 1
    give count
  }
}

let a = makeCounter()
let b = makeCounter()
say a(), a(), a()
say b()
```

Each counter keeps its own `count`.

### Your turn

Write two functions:

- `makeAdder(n)` gives back a new function that adds `n` to whatever it is given. After `let add5 = makeAdder(5)`, `add5(3)` is `8`.
- `evensSquared(numbers)` gives back a list of the squares of the even numbers only. `evensSquared([1, 2, 3, 4])` is `[4, 16]`.

Try to write `evensSquared` with `filter` and `map`.

Starter code:

```kes
fn makeAdder(n) {
  // give back a function
}

fn evensSquared(numbers) {
  // your code
}

let add5 = makeAdder(5)
say add5(3)
say evensSquared([1, 2, 3, 4])
```

<details><summary>Hints</summary>

- `makeAdder` is one line: `give fn(x) => x + n`.
- Filter first (keep the even numbers), then map (square them).

</details>

<details><summary>Solution (try first!)</summary>

```kes
fn makeAdder(n) {
  give fn(x) => x + n
}

fn evensSquared(numbers) {
  give numbers
    |> filter(fn(x) => x % 2 == 0)
    |> map(fn(x) => x * x)
}
```

</details>

---

## Lesson 11: When Things Go Wrong

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

### Your turn

Write a function `parseAge(text)` that turns text into an age:

- If `text` is a valid number that is 0 or more, give back the number.
- Otherwise give back `-1`.

So `parseAge("42")` is `42`, while `parseAge("abc")`, `parseAge("")` and `parseAge("-5")` are all `-1`.

Starter code:

```kes
fn parseAge(text) {
  // your code
}

say parseAge("42")
say parseAge("abc")
```

<details><summary>Hints</summary>

- `num(text)` fails on bad text, so put it inside `try`.
- Check for negative numbers separately. `-5` converts fine but is not a valid age.

</details>

<details><summary>Solution (try first!)</summary>

```kes
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
```

</details>

---

## Lesson 12: Matching

A long `if / else if` ladder works, but `match` reads better when you compare one value against many options.

```kes
let day = 3
let name = match day {
  1 => "Monday"
  2 => "Tuesday"
  3 => "Wednesday"
  else => "Some other day"
}
say name
```

Kestral tries each line from the top. The first one that fits wins, and `else` catches everything else. `match` produces a value, so you can store it, give it back or print it.

## Several values on one line

```kes
fn describe(n) {
  give match n {
    0 => "none"
    1, 2, 3 => "a few"
    else => "many"
  }
}
say describe(0), describe(2), describe(50)
```

## Ranges

A range such as `90..100` matches any number from 90 to 100.

```kes
fn grade(score) {
  give match score {
    90..100 => "A"
    80..89 => "B"
    70..79 => "C"
    else => "keep going"
  }
}
say grade(95), grade(81), grade(12)
```

A range covers everything from its first number to its last. A score such as `89.5` falls in the gap between `80..89` and `90..100`, so it would reach `else`. If your numbers can have decimals, use `if` with `>=`, as in lesson 4.

## Doing things in a match

A line can run a statement, or a block with several steps:

```kes
match "stop" {
  "go" => say "Green light"
  "stop" => {
    say "Red light"
    say "Wait here"
  }
  else => say "Unknown signal"
}
```

If nothing matches and there is no `else`, the result is `nothing`.

### Your turn

Write a function `dayType(n)` for day numbers (1 = Monday ... 7 = Sunday):

- 1 to 5 gives `"weekday"`
- 6 or 7 gives `"weekend"`
- anything else gives `"invalid"`

Use `match`.

Starter code:

```kes
fn dayType(n) {
  give match n {
    // your lines here
  }
}

say dayType(2)
say dayType(7)
```

<details><summary>Hints</summary>

- `1..5 => "weekday"` covers the weekdays in one line.
- Do not forget `else => "invalid"` for numbers like 0 or 8.

</details>

<details><summary>Solution (try first!)</summary>

```kes
fn dayType(n) {
  give match n {
    1..5 => "weekday"
    6, 7 => "weekend"
    else => "invalid"
  }
}
```

</details>

---

## Lesson 13: Capstone: Number Cruncher

You now know the whole language. Time to build something with it. A good way to tackle any program is in small steps:

1. Say in plain words what it should do.
2. Get the simplest version working.
3. Add one feature at a time, running after each.

## Reading input until it ends

Here is the pattern for "keep asking until the user types `done`". `ask()` gives back `nothing` when there is no more input, so we stop on that too.

```kes
let names = []
while true {
  let line = ask()
  if line == nothing or line == "done" {
    stop
  }
  push(names, line)
}
say "You entered {len(names)} names: {join(names, ", ")}"
```

(In the Studio, put one answer per line in the Input box.)

## Mixing the tools you know

- a `while true` loop with `stop`
- `try` and `catch` to survive bad input
- a list to collect values
- `sum`, `max` and `round` for the results

Take it one step at a time, and run your program often.

### Your turn

Write a program that reads lines until the user types `done` (or the input ends). For each line:

- if it is a number, remember it
- if it is not, print `Skipping: ` followed by the line, and carry on

At the end, print four lines:

```
Count: 6
Total: 108
Average: 18
Largest: 42
```

Round the average to 2 decimal places. If no numbers were entered, print only `No numbers entered.` instead.

Test inputs: `4 8 15 16 23 42 done` gives the output above, and `x 5 done` prints `Skipping: x` first.

Starter code:

```kes
let numbers = []

while true {
  let line = ask()
  if line == nothing or line == "done" {
    stop
  }
  // try to turn the line into a number and remember it
}

// print the results
```

<details><summary>Hints</summary>

- Put `num(line)` inside `try`. If it fails, `catch` prints the `Skipping:` message.
- After the loop, check `len(numbers) == 0` first.
- `sum(numbers)` and `max(numbers)` save you from writing more loops.

</details>

<details><summary>Solution (try first!)</summary>

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

</details>

---

## You finished. What now?

You can read and write Kestral programs. The next step is to build things you actually want. Here are project ideas in rough order of difficulty:

1. **Temperature converter.** Ask for Celsius, print Fahrenheit.
2. **Times table.** Ask for a number and print its table up to 12.
3. **Guess the number.** Pick a number with `randomInt`, ask for guesses and say "higher" or "lower".
4. **Shopping basket.** A list of maps like `{item: "tea", price: 3.5, qty: 2}`, with a function that totals it.
5. **Vowel counter.** Count the vowels in a sentence with a map.
6. **Prime finder.** List every prime up to 100.
7. **Contact book.** A map from names to details, with functions to add, find and remove.
8. **Caesar cipher.** Shift letters along the alphabet. Hint: build the alphabet text and use `find`.
9. **Fibonacci two ways.** Write it with recursion and with a loop. Which is faster for 25?
10. **Text adventure.** Rooms as a map of maps, and a loop that reads commands.

## Habits that make you better

- **Name things well.** `total` beats `t`. You will read your code far more often than you write it.
- **Keep functions small.** If a function needs a long comment to explain, split it.
- **Test with `assert`.** After writing a function, add lines such as `assert(isEven(4) == true)` at the bottom of the file. If you ever break it, you will know immediately.
- **Read errors slowly.** Line number first, then the message, then the `^` marker.
- **Split big problems.** Write the plan as comments first, then fill each one in.

## Splitting a program into files

When a program grows, move helper functions into their own `.kes` file and load it with `use`:

```kes
use "tools.kes"
say double(21)
```

`use` runs the file once, so its functions become available. This works when you run files from the command line.

## What Kestral is, and is not

Kestral is a general-purpose, interpreted language with numbers, text, lists, maps, functions, closures, pattern matching and error handling. It is ideal for learning, for puzzles, and for scripts that work on data you can type or paste in.

This version does **not** read or write files, use the network, or draw graphics, and it is slower than a compiled language. Those are good things to add if you decide to extend the language yourself. The whole interpreter is one readable file, `src/kestral.js`, and reading it is a great next course.

---

## Quick reference

### Basics

```kes
// a comment
say "Hello", 42, true     // print values, separated by spaces
let name = "Ada"          // create a name
name = "Grace"            // change it later
say "Hi {name}, {1 + 2}"  // {…} puts values inside text
let a = 1; let b = 2     // ; also separates statements
```

One statement per line. Names use letters, digits and underscores.

### Values

```kes
42   3.14   -7           // numbers
"text"   'text'           // text (both quote styles work)
true   false               // bools
nothing                    // no value
[1, 2, 3]                  // list
{name: "Ada", age: 36}     // map
1..5                       // range (includes both ends, can count down: 5..1)
```

Falsy values: false, nothing, 0 and "". Everything else is true.

### Operators

```kes
+  -  *  /  %  **          // math (% is remainder, ** is power)
==  !=  <  <=  >  >=        // comparing
and   or   not             // logic (and / or return one of their sides)
a ?? b                     // b only when a is nothing
"a" + "b"   "ab" * 3      // join and repeat text
[1] + [2]                  // join lists
x |> f   x |> f(2)         // pipe: f(x)   f(x, 2)
x += 1   x -= 1   x *= 2   // shortcuts
```

Precedence, high to low: **, unary -, * / %, + -, .., comparisons, not, and, or, ??, |>.

### Decisions

```kes
if score >= 90 {
  say "A"
} else if score >= 80 {
  say "B"
} else {
  say "C"
}
let kind = if n % 2 == 0 { "even" } else { "odd" }
```

if is also a value. Conditions need no brackets.

### Match

```kes
let word = match n {
  0 => "none"
  1, 2, 3 => "few"
  4..9 => "some"
  else => "many"
}
```

First matching line wins. A line can be a value, a statement, or a { block }.

### Loops

```kes
loop 3 times { say "hi" }
for i in 1..5 { say i }
for item in ["a", "b"] { say item }
for i, item in ["a", "b"] { say i, item }
for key, value in {a: 1} { say key, value }
while x < 100 { x *= 2 }
stop    // leave the loop
skip    // next round
```

for also walks through text, ranges and map keys.

### Functions

```kes
fn add(a, b) {
  give a + b
}
fn square(n) => n * n
fn greet(name = "friend") => "Hi {name}"
let triple = fn(x) => x * 3
say add(1, 2), square(4), greet()
```

give sends a value back. Functions can be used above the line that declares them. They can be stored, passed around and returned (closures).

### Lists and maps

```kes
let xs = [10, 20, 30]
say xs[0], xs[-1], len(xs)
xs[1] = 99
push(xs, 40)
let m = {a: 1}
m.b = 2
m["c"] = 3
say m.a, m["b"], m.missing    // missing keys give nothing
```

Lists start at position 0; negative positions count from the end. Lists and maps are shared when assigned, use copy() for an independent one.

### Errors

```kes
try {
  fail "something broke"
} catch e {
  say "caught: {e}"
}
assert(1 + 1 == 2, "math is broken")
```

e holds the error message as text. Runaway loops and runaway recursion cannot be caught.

### Methods and files

```kes
say "hi".upper()                 // same as upper("hi")
say [3, 1, 2].sort().reverse()
use "tools.kes"                  // load another file (command line)
```

Any built-in or function you define can be called with a dot, with the first value in front.

### Built-in functions

**Input and conversion**

- `ask(prompt?)`: Reads one line of input as text. Gives nothing when the input has run out.
- `type(value)`: The type as text: "number", "text", "bool", "list", "map", "function", "range" or "nothing".
- `str(value)`: Turns any value into text.
- `num(text)`: Turns text like "3.5" into a number. Fails if it is not a number.
- `int(value)`: A number (or number-like text) with the decimals cut off.
- `list(value)`: Makes a list from a range, text (its characters) or a map (its keys).
- `copy(value)`: A new, independent copy of a list or map.

**Numbers**

- `abs(n)`: Distance from zero: abs(-3) is 3.
- `floor(n)`: Rounds down.
- `ceil(n)`: Rounds up.
- `sqrt(n)`: Square root.
- `round(n, digits?)`: Rounds to the nearest whole number, or to a number of decimal places.
- `min(a, b, ...)  or  min(list)`: The smallest number.
- `max(a, b, ...)  or  max(list)`: The largest number.
- `sum(list)`: Adds up a list of numbers.
- `random()`: A random decimal from 0 up to (not including) 1.
- `randomInt(low, high)`: A random whole number from low to high, both included.

**Lists, maps and ranges**

- `len(value)`: How many items (or characters, or keys).
- `push(list, value, ...)`: Adds to the end of a list. Gives the list back.
- `pop(list)`: Removes and gives back the last item.
- `remove(list, position)  or  remove(map, key)`: Removes an item and gives it back.
- `slice(value, from, to?)`: A piece of a list or text. `to` is not included. Negative positions count from the end.
- `reverse(value)`: A reversed copy of a list or text.
- `sort(list, key?)`: A sorted copy. With a function, sorts by what the function gives for each item.
- `unique(list)`: A copy with duplicates removed.
- `keys(map)`: A list of the keys.
- `values(map)`: A list of the values.
- `entries(map)`: A list of [key, value] pairs.
- `contains(thing, x)`: True if text contains the text x, a list holds x, a map has the key x, or a range includes x.
- `find(thing, x)`: Position of x in text or a list, or -1 when it is not there.

**Working with functions**

- `map(list, fn)`: A new list made by applying fn to every item. If fn takes two parameters it also gets the position.
- `filter(list, fn)`: Keeps the items for which fn gives something true.
- `reduce(list, fn, start?)`: Folds a list into one value. fn gets (total so far, next item).
- `any(list, fn)`: True if fn is true for at least one item.
- `all(list, fn)`: True if fn is true for every item.

**Text**

- `upper(text)`: UPPER CASE.
- `lower(text)`: lower case.
- `trim(text)`: Removes spaces at both ends.
- `split(text, separator?)`: Cuts text into a list. With no separator it splits on spaces; with "" it gives every character.
- `join(list, separator?)`: Glues the items of a list into one text.
- `replace(text, old, new)`: Replaces every old with new.
- `startsWith(text, start)`: True if the text begins with start.
- `endsWith(text, end)`: True if the text ends with end.
- `repeat(text, times)`: The text repeated. Same as text * times.

**Checking**

- `assert(condition, message?)`: Does nothing when the condition is true. Fails with the message when it is false.

