==> title
Matching
==> goal
Choose between many cases cleanly
==> intro
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
==> challenge
Write a function `dayType(n)` for day numbers (1 = Monday ... 7 = Sunday):

- 1 to 5 gives `"weekday"`
- 6 or 7 gives `"weekend"`
- anything else gives `"invalid"`

Use `match`.
==> starter
fn dayType(n) {
  give match n {
    // your lines here
  }
}

say dayType(2)
say dayType(7)
==> solution
fn dayType(n) {
  give match n {
    1..5 => "weekday"
    6, 7 => "weekend"
    else => "invalid"
  }
}
==> hints
- `1..5 => "weekday"` covers the weekdays in one line.
- Do not forget `else => "invalid"` for numbers like 0 or 8.
==> tests
[
 {"append": "say dayType(1)\nsay dayType(5)\nsay dayType(6)\nsay dayType(7)\nsay dayType(0)\nsay dayType(8)", "expect": "weekday\nweekday\nweekend\nweekend\ninvalid\ninvalid"}
]
