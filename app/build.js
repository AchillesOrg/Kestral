'use strict';
/* Builds the Kestral Studio app from app/template.html.
   dist/kestral-studio.html  complete standalone page (double-click to open in a browser)
   dist/studio-artifact.html the same page as a fragment, for publishing as an Artifact */
const fs = require('fs');
const path = require('path');

const root = path.join(__dirname, '..');
const read = p => fs.readFileSync(path.join(root, p), 'utf8');

const safeJson = v => JSON.stringify(v).replace(/</g, '\\u003c').replace(/\u2028/g, '\\u2028').replace(/\u2029/g, '\\u2029');

const titles = {
  'hello.kes': 'Hello, world',
  'primes.kes': 'Prime numbers',
  'fibonacci.kes': 'Fibonacci, two ways',
  'bank_account.kes': 'Bank account (closures)',
  'word_frequency.kes': 'Word frequency',
  'temperature.kes': 'Temperature converter',
  'uses_tools.kes': 'Using another file (use)',
  'tools.kes': 'tools.kes (helper library)'
};
const examplesDir = path.join(root, 'examples');
const examples = fs.readdirSync(examplesDir).filter(f => f.endsWith('.kes')).sort().map(f => {
  const inFile = path.join(examplesDir, f.replace(/\.kes$/, '.in'));
  return {
    file: f,
    title: titles[f] || f,
    lib: f === 'tools.kes',
    code: fs.readFileSync(path.join(examplesDir, f), 'utf8'),
    input: fs.existsSync(inFile) ? fs.readFileSync(inFile, 'utf8').replace(/\n+$/, '') : ''
  };
});

const lessons = JSON.parse(read('lessons/lessons.json')).map(L => ({
  id: L.id, title: L.title, goal: L.goal, intro: L.intro, challenge: L.challenge,
  starter: L.starter, solution: L.solution, hints: L.hints, tests: L.tests
}));
const reference = JSON.parse(read('lessons/reference.json'));
const version = JSON.parse(read('package.json')).version;

let tpl = read('app/template.html');
const fill = (key, text) => { tpl = tpl.split(key).join(text); };
fill('/*__KESTRAL_SOURCE__*/', read('src/kestral.js'));
fill('__LESSONS_JSON__', safeJson(lessons));
fill('__REFERENCE_JSON__', safeJson(reference));
fill('__EXAMPLES_JSON__', safeJson(examples));
fill('__VERSION__', version);

if (/<\/script/i.test(tpl.replace(/<\/script>/g, ''))) throw new Error('A stray </script> would break the page.');

fs.mkdirSync(path.join(root, 'dist'), { recursive: true });
fs.writeFileSync(path.join(root, 'dist', 'studio-artifact.html'), tpl);

// The standalone version is a complete document (so browsers use standards mode).
const m = /^([\s\S]*?<\/style>)\s*([\s\S]*)$/.exec(tpl);
if (!m) throw new Error('template must contain a <style> block');
const standalone = '<!doctype html>\n<html lang="en">\n<head>\n<meta charset="utf-8">\n<meta name="viewport" content="width=device-width, initial-scale=1, viewport-fit=cover">\n' +
  m[1] + '\n</head>\n<body>\n' + m[2] + '\n</body>\n</html>\n';
fs.writeFileSync(path.join(root, 'dist', 'kestral-studio.html'), standalone);
console.log(`Built dist/kestral-studio.html (${Math.round(standalone.length / 1024)} KB) and dist/studio-artifact.html`);
