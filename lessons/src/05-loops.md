==> title
Loops
==> goal
Repeat work without repeating yourself
==> intro
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
==> challenge
Print the numbers from 1 to 15, one per line, but:

- for multiples of 3, print `Fizz` instead of the number
- for multiples of 5, print `Buzz` instead
- for multiples of both 3 and 5, print `FizzBuzz`

This is the famous FizzBuzz puzzle. The first lines are `1`, `2`, `Fizz`, `4`, `Buzz`.
==> starter
for i in 1..15 {
  // decide what to say
}
==> solution
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
==> hints
- `i % 3 == 0` is true when `i` is a multiple of 3.
- Check the "both" case (multiple of 15) **first**. Otherwise a 15 would print just `Fizz`.
==> tests
[
 {"expect": "1\n2\nFizz\n4\nBuzz\nFizz\n7\n8\nFizz\nBuzz\n11\nFizz\n13\n14\nFizzBuzz"}
]
