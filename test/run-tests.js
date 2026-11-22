'use strict';
/* Runs the language tests and verifies every lesson (solutions pass, starters don't). */
const fs = require('fs');
const path = require('path');
const K = require('../src/kestral.js');
const cases = require('./language-tests.js');

let pass = 0, fail = 0;
const failures = [];
function check(name, ok, detail) {
  if (ok) pass++;
  else { fail++; failures.push(`FAIL  ${name}\n      ${detail}`); }
}

/* ---- language tests ---- */
for (const [name, src, expected] of cases) {
  const exp = typeof expected === 'string' ? { out: expected } : expected;
  const r = K.run(src, { input: exp.input, maxSteps: exp.steps, maxOutput: exp.outputMax });
  if (exp.error) {
    const msg = r.error ? r.error.message : '(no error)';
    const lineOk = exp.line === undefined || (r.error && r.error.line === exp.line);
    check(name, !!r.error && msg.includes(exp.error) && lineOk, `expected error containing: ${exp.error}${exp.line ? ' on line ' + exp.line : ''}\n      got: ${r.error ? msg + ' (line ' + r.error.line + ')' : 'no error, output: ' + JSON.stringify(r.text)}`);
  } else {
    check(name, r.ok && r.text === exp.out, r.ok ? `expected ${JSON.stringify(exp.out)}\n      got      ${JSON.stringify(r.text)}` : `unexpected error: ${r.errorText}`);
  }
}

/* ---- reference: every documented built-in must exist ---- */
const refPath = path.join(__dirname, '..', 'lessons', 'reference.json');
if (fs.existsSync(refPath)) {
  const ref = JSON.parse(fs.readFileSync(refPath, 'utf8'));
  const have = new Set(K.builtinNames());
  const documented = new Set();
  for (const group of ref.builtins) for (const f of group.items) { documented.add(f.name); check('reference built-in exists: ' + f.name, have.has(f.name), 'documented but not defined'); }
  for (const name of have) check('built-in is documented: ' + name, documented.has(name), 'defined but missing from reference.json');
}

/* ---- lessons ---- */
const lessonsPath = path.join(__dirname, '..', 'lessons', 'lessons.json');
if (fs.existsSync(lessonsPath)) {
  const lessons = JSON.parse(fs.readFileSync(lessonsPath, 'utf8'));
  for (const L of lessons) {
    if (!L.tests || !L.tests.length) continue;
    const runTests = code => L.tests.map(t => K.runTest(code, t));
    const sol = runTests(L.solution);
    sol.forEach((s, i) => check(`lesson ${L.id} solution passes test ${i + 1}`, s.ok, `expected ${JSON.stringify(s.expect)}\n      got      ${JSON.stringify(s.got)}`));
    const st = runTests(L.starter);
    check(`lesson ${L.id} starter is not already a solution`, !st.every(s => s.ok), 'the starter code passes every test');
    // every ```kes example in the lesson text must run without error (unless marked to fail)
    const re = /```kes([^\n]*)\n([\s\S]*?)```/g;
    let m, k = 0;
    const text = (L.intro || '') + '\n' + (L.challenge || '');
    while ((m = re.exec(text))) {
      k++;
      const code = m[2];
      const im = /input=(.*)/.exec(m[1]);
      const exInput = im ? im[1].split('|') : [];
      if (/^\/\/ error/m.test(code)) continue;
      const r = K.run(code, { input: exInput, maxSteps: 2e6 });
      check(`lesson ${L.id} example ${k} runs`, r.ok, r.errorText);
    }
  }
}

/* ---- example programs ---- */
const exDir = path.join(__dirname, '..', 'examples');
if (fs.existsSync(exDir)) {
  for (const f of fs.readdirSync(exDir).filter(x => x.endsWith('.kes'))) {
    const src = fs.readFileSync(path.join(exDir, f), 'utf8');
    const inputFile = path.join(exDir, f.replace(/\.kes$/, '.in'));
    const input = fs.existsSync(inputFile) ? fs.readFileSync(inputFile, 'utf8').split('\n') : [];
    const r = K.run(src, { input, maxSteps: 5e6, readFile: p => fs.readFileSync(path.join(exDir, p), 'utf8') });
    check('example runs: ' + f, r.ok, r.errorText);
    const expFile = path.join(exDir, f.replace(/\.kes$/, '.out'));
    if (fs.existsSync(expFile)) check('example output: ' + f, r.text === fs.readFileSync(expFile, 'utf8').replace(/\n$/, ''), `got ${JSON.stringify(r.text)}`);
  }
}

console.log(failures.join('\n'));
console.log(`\n${pass} passed, ${fail} failed`);
process.exit(fail ? 1 : 0);
