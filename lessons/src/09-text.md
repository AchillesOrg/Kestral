==> title
Working with Text
==> goal
Slice, search and reshape words
==> intro
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
==> challenge
Write two functions:

- `initials(name)` gives back the first letter of every word, capitalized, each followed by a dot. `initials("ada lovelace")` is `"A.L."`.
- `isPalindrome(text)` gives back `true` if the text reads the same forwards and backwards, ignoring upper and lower case. `"Racecar"` is a palindrome and `"Hello"` is not.
==> starter
fn initials(name) {
  // your code
}

fn isPalindrome(text) {
  // your code
}

say initials("ada lovelace")
say isPalindrome("Racecar")
==> solution
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
==> hints
- For `initials`: loop over `split(name)`, and use `word[0]` for the first letter.
- Build the answer by adding to a text value that starts as `""`.
- For `isPalindrome`: compare `lower(text)` with `reverse(lower(text))`.
==> tests
[
 {"append": "say initials(\"ada lovelace\")\nsay initials(\"grace brewster hopper\")\nsay initials(\"cher\")", "expect": "A.L.\nG.B.H.\nC."},
 {"append": "say isPalindrome(\"Racecar\")\nsay isPalindrome(\"Hello\")\nsay isPalindrome(\"a\")", "expect": "true\nfalse\ntrue"}
]
