# Kestral

A small, friendly, general-purpose programming language. Programs are plain text files ending in `.kes`.

```kes
fn greet(name = "friend") => "Hello, {name}!"

for person in ["Ada", "Grace", "Linus"] {
  say greet(person)
}
```

## What is in this folder

| Path | What it is |
| --- | --- |
| `TUTORIAL.md` | The full 13-lesson course with challenges, hints, solutions and a reference. Start here. |
| `dist/kestral-studio.html` | The Studio app. Double-click to open it in any browser. No install, works offline. |
| `src/kestral.js` | The whole language (lexer, parser, interpreter, built-ins) in one readable file. |
| `bin/kestral.js` | The command line tool and interactive REPL. |
| `examples/` | Seven example programs, plus a tiny library loaded with `use`. |
| `test/` | 355 automated checks (language, error messages, every lesson solution, every example). |

## Run a program

You need [Node.js](https://nodejs.org) 16 or newer.

```
node bin/kestral.js examples/primes.kes     run a file
node bin/kestral.js                         interactive REPL
node bin/kestral.js -e 'say 6 * 7'          run one line
```

Optional: `npm link` in this folder makes the command `kestral` available everywhere.

## Rebuild and test

```
npm test          run all tests
npm run build     rebuild TUTORIAL.md data and the Studio app (run: node lessons/build-lessons.js first if you edit lessons)
```

Lessons live in `lessons/src/*.md`. Edit one, run `node lessons/build-lessons.js && node app/build.js`, and both the tutorial and the app update.
