==> title
Lists
==> goal
Keep many values in one place
==> intro
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
==> challenge
Write a function `average(numbers)` that gives back the average of a list of numbers. Use a loop (or `sum` if you like). You can assume the list is not empty.
==> starter
fn average(numbers) {
  // your code
}

say average([2, 4, 6])
==> solution
fn average(numbers) {
  let total = 0
  for n in numbers {
    total += n
  }
  give total / len(numbers)
}
==> hints
- Add everything up first, then divide by `len(numbers)`.
- Make sure the `give` is **after** the loop, not inside it.
==> tests
[
 {"append": "say average([2, 4, 6])\nsay average([1, 2])\nsay average([10])", "expect": "4\n1.5\n10"}
]
