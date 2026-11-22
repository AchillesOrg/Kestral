==> title
Capstone: Number Cruncher
==> goal
Put everything together in a real program
==> intro
You now know the whole language. Time to build something with it. A good way to tackle any program is in small steps:

1. Say in plain words what it should do.
2. Get the simplest version working.
3. Add one feature at a time, running after each.

## Reading input until it ends

Here is the pattern for "keep asking until the user types `done`". `ask()` gives back `nothing` when there is no more input, so we stop on that too.

```kes input=Ada|Grace|Linus|done
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
==> challenge
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
==> starter
let numbers = []

while true {
  let line = ask()
  if line == nothing or line == "done" {
    stop
  }
  // try to turn the line into a number and remember it
}

// print the results
==> solution
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
==> hints
- Put `num(line)` inside `try`. If it fails, `catch` prints the `Skipping:` message.
- After the loop, check `len(numbers) == 0` first.
- `sum(numbers)` and `max(numbers)` save you from writing more loops.
==> tests
[
 {"input": ["4", "8", "15", "16", "23", "42", "done"], "expect": "Count: 6\nTotal: 108\nAverage: 18\nLargest: 42"},
 {"input": ["2.5", "3", "done"], "expect": "Count: 2\nTotal: 5.5\nAverage: 2.75\nLargest: 3"},
 {"input": ["x", "5", "done"], "expect": "Skipping: x\nCount: 1\nTotal: 5\nAverage: 5\nLargest: 5"},
 {"input": ["done"], "expect": "No numbers entered."},
 {"input": ["7", "1"], "expect": "Count: 2\nTotal: 8\nAverage: 4\nLargest: 7"}
]
