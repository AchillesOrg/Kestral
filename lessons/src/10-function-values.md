==> title
Functions as Values
==> goal
Pass functions around, and chain them into pipelines
==> intro
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
==> challenge
Write two functions:

- `makeAdder(n)` gives back a new function that adds `n` to whatever it is given. After `let add5 = makeAdder(5)`, `add5(3)` is `8`.
- `evensSquared(numbers)` gives back a list of the squares of the even numbers only. `evensSquared([1, 2, 3, 4])` is `[4, 16]`.

Try to write `evensSquared` with `filter` and `map`.
==> starter
fn makeAdder(n) {
  // give back a function
}

fn evensSquared(numbers) {
  // your code
}

let add5 = makeAdder(5)
say add5(3)
say evensSquared([1, 2, 3, 4])
==> solution
fn makeAdder(n) {
  give fn(x) => x + n
}

fn evensSquared(numbers) {
  give numbers
    |> filter(fn(x) => x % 2 == 0)
    |> map(fn(x) => x * x)
}
==> hints
- `makeAdder` is one line: `give fn(x) => x + n`.
- Filter first (keep the even numbers), then map (square them).
==> tests
[
 {"append": "let add5 = makeAdder(5)\nsay add5(3)\nsay makeAdder(10)(1)\nsay makeAdder(0)(7)", "expect": "8\n11\n7"},
 {"append": "say evensSquared([1, 2, 3, 4])\nsay evensSquared([1, 3])\nsay evensSquared([6])", "expect": "[4, 16]\n[]\n[36]"}
]
