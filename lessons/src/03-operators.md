==> title
Numbers and Text
==> goal
Do math, join text, and round results
==> intro
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
==> challenge
Write a program that asks for a **price** and then a **quantity**, and prints the total including 10% tax, rounded to 2 decimal places, like this:

```
Total: 22
```

(That is the output for a price of `10` and a quantity of `2`: 10 x 2 = 20, plus 10% is 22.) Note that `round` already gives you a clean number, so do not worry about trailing zeros.
==> starter
let price = num(ask())
// ask for the quantity next, then work out and say the total
==> solution
let price = num(ask())
let qty = num(ask())
let total = price * qty * 1.1
say "Total: {round(total, 2)}"
==> hints
- Adding 10% is the same as multiplying by `1.1`.
- `round(total, 2)` rounds to two decimal places.
==> tests
[
 {"input": ["10", "2"], "expect": "Total: 22"},
 {"input": ["9.99", "3"], "expect": "Total: 32.97"},
 {"input": ["5", "1"], "expect": "Total: 5.5"}
]
