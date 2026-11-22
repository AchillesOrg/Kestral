# Learn Kestral

Kestral is a small, friendly, general-purpose programming language. Programs are plain text files that end in `.kes`. It is designed to be read easily, to give helpful error messages, and to be learned in a couple of weeks of short sessions.

This tutorial takes you from "what is a program?" to writing real programs. It has 13 lessons. Each one has:

- a short explanation with examples you can run
- a **challenge** that the Studio app can check for you automatically
- hints, and a full solution for when you are stuck

## A taste

```kes
fn greet(name = "friend") => "Hello, {name}!"

let people = ["Ada", "Grace", "Linus"]
for person in people {
  say greet(person)
}

let lengths = people |> map(fn(p) => len(p))
say "Name lengths: {lengths}"
```

## Two ways to run Kestral

**1. Kestral Studio (easiest).** A single page with an editor, a Run button, an Input box and the whole course built in. Nothing to install. Your progress and code are remembered in your browser.

**2. The command line.** You need [Node.js](https://nodejs.org) 16 or newer. In this folder:

```
node bin/kestral.js hello.kes       run a file
node bin/kestral.js                 start the interactive REPL
node bin/kestral.js -e "say 6 * 7"  run one line
```

Make a file called `hello.kes` containing `say "Hello from a file!"` and run it. That is your first real Kestral program outside the Studio. The `examples` folder has more programs to read and run.

## How to learn from this

- **Type the examples; do not paste them.** Your fingers learn syntax faster than your eyes do.
- **Change things.** After an example works, break it on purpose and see what the error says.
- **Do the challenge before you look at the hints.** Struggling for ten minutes is where the learning happens. Then use one hint at a time.
- **Little and often beats a marathon.** One lesson a day, with a short break, is a good rhythm. Thirteen lessons is about two weeks.
- **Run often.** Run your program after every small change, not after fifty lines.
