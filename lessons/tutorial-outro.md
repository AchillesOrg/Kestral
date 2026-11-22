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
