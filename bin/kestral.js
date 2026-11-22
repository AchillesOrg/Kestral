#!/usr/bin/env node
'use strict';
/* Kestral command line:  kestral file.kes | kestral (REPL) | kestral -e "say 1 + 2" */
const fs = require('fs');
const path = require('path');
const readline = require('readline');
const K = require('../src/kestral.js');

const HELP = `Kestral ${K.version}: a small, friendly programming language

Usage:
  kestral                  start the interactive REPL
  kestral file.kes         run a program
  kestral -e "say 1 + 2"   run a snippet of code
  kestral --help           show this help
  kestral --version        show the version

In the REPL:  .help  .clear  .exit`;

// Read one line from standard input, synchronously (used by ask()).
function readLineSync() {
  const buf = Buffer.alloc(1);
  let line = '';
  let got = false;
  for (;;) {
    let n = 0;
    try { n = fs.readSync(0, buf, 0, 1, null); }
    catch (e) {
      if (e.code === 'EAGAIN') continue;
      if (e.code === 'EOF') break;
      throw e;
    }
    if (n === 0) break;
    got = true;
    const ch = buf.toString('utf8');
    if (ch === '\n') return line.replace(/\r$/, '');
    line += ch;
  }
  return got ? line : null;
}

function makeInterp(baseDir) {
  return K.create({
    maxSteps: 5e9,
    maxDepth: 600,
    print: s => process.stdout.write(s + '\n'),
    ask: prompt => { if (prompt) process.stdout.write(prompt); return readLineSync(); },
    readFile: p => fs.readFileSync(path.resolve(baseDir, p), 'utf8')
  });
}

function runSource(src, baseDir) {
  const interp = makeInterp(baseDir);
  try {
    interp.run(src);
    return 0;
  } catch (e) {
    process.stderr.write(K.formatError(e, src) + '\n');
    return 1;
  }
}

function repl() {
  const interp = makeInterp(process.cwd());
  console.log(`Kestral ${K.version}  (type .help for help, .exit to quit)`);
  const rl = readline.createInterface({ input: process.stdin, output: process.stdout, prompt: 'kes> ' });
  let buffer = '';
  rl.prompt();
  rl.on('line', line => {
    const trimmed = line.trim();
    if (!buffer) {
      if (trimmed === '.exit') { rl.close(); return; }
      if (trimmed === '.help') { console.log(HELP); rl.prompt(); return; }
      if (trimmed === '.clear') { interp.global = new (interp.global.constructor)(null); console.log('Cleared. (Built-ins are gone too, restart to get them back.)'); rl.prompt(); return; }
    }
    buffer += (buffer ? '\n' : '') + line;
    try {
      const value = interp.run(buffer);
      if (interp.lastWasExpr && value !== null) console.log(K.format(value));
      buffer = '';
    } catch (e) {
      if (e instanceof K.KestralError && e.incomplete) {
        rl.setPrompt('... ');
        rl.prompt();
        return;
      }
      console.error(K.formatError(e, buffer));
      buffer = '';
    }
    rl.setPrompt('kes> ');
    rl.prompt();
  });
  rl.on('close', () => { console.log(''); process.exit(0); });
}

function main(argv) {
  const args = argv.slice(2);
  if (args.length === 0) return repl();
  if (args[0] === '--help' || args[0] === '-h') { console.log(HELP); return; }
  if (args[0] === '--version' || args[0] === '-v') { console.log(K.version); return; }
  if (args[0] === '-e') {
    if (!args[1]) { console.error('Give some code after -e'); process.exit(2); }
    process.exit(runSource(args[1], process.cwd()));
  }
  const file = args[0];
  let src;
  try { src = fs.readFileSync(file, 'utf8'); }
  catch (e) { console.error(`I couldn't open "${file}".`); process.exit(2); }
  process.exit(runSource(src, path.dirname(path.resolve(file))));
}

main(process.argv);
