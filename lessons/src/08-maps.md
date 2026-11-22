==> title
Maps
==> goal
Look values up by name
==> intro
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
==> challenge
Write a function `wordCount(text)` that gives back a map from each word to how many times it appears. Treat upper and lower case as the same word, so `"Go go GO"` counts `go` three times.

Hint: `split(text)` splits text into words, and `lower(text)` makes it lower case.
==> starter
fn wordCount(text) {
  let counts = {}
  // your code
  give counts
}

say wordCount("the cat and the dog")
==> solution
fn wordCount(text) {
  let counts = {}
  for word in split(lower(text)) {
    counts[word] = (counts[word] ?? 0) + 1
  }
  give counts
}
==> hints
- Loop over `split(lower(text))`.
- Inside the loop: `counts[word] = (counts[word] ?? 0) + 1`.
==> tests
[
 {"append": "let c = wordCount(\"the cat the dog\")\nsay c[\"the\"]\nsay c[\"cat\"]\nsay len(c)", "expect": "2\n1\n3"},
 {"append": "let c = wordCount(\"Go go GO\")\nsay c[\"go\"]\nsay len(c)", "expect": "3\n1"},
 {"append": "say len(wordCount(\"\"))", "expect": "0"}
]
