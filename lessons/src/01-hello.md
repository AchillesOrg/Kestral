==> title
Hello, Kestral
==> goal
Print your first lines with say
==> intro
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
==> challenge
Print exactly these two lines, in this order:

```
Kestral flies!
Lesson 1 done
```

Press **Check answer** when your output matches.
==> starter
// Write your two say lines below
==> solution
say "Kestral flies!"
say "Lesson 1 done"
==> hints
- Each line you want to print needs its own `say`.
- Text must be inside double quotes, and match exactly, including the capital letters and the exclamation mark.
==> tests
[
 {"expect": "Kestral flies!\nLesson 1 done"}
]
