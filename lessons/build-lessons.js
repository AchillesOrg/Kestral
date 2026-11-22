'use strict';
/* Reads lessons/src/*.md and writes lessons/lessons.json and TUTORIAL.md.
   Lesson file format: sections introduced by a line like  ==> name
   Sections: title, goal, intro, challenge, starter, solution, hints (lines starting "- "), tests (JSON array) */
const fs = require('fs');
const path = require('path');

const root = path.join(__dirname, '..');
const srcDir = path.join(__dirname, 'src');

function parseLesson(file, id) {
  const text = fs.readFileSync(path.join(srcDir, file), 'utf8').replace(/\r\n/g, '\n');
  const sections = {};
  let cur = null;
  for (const line of text.split('\n')) {
    const m = /^==> (\w+)\s*$/.exec(line);
    if (m) { cur = m[1]; sections[cur] = []; continue; }
    if (cur) sections[cur].push(line);
  }
  const get = k => (sections[k] || []).join('\n').replace(/^\n+|\n+$/g, '');
  const hints = get('hints').split('\n').filter(l => l.startsWith('- ')).map(l => l.slice(2).trim());
  let tests = [];
  try { tests = JSON.parse(get('tests') || '[]'); }
  catch (e) { throw new Error(`${file}: tests are not valid JSON: ${e.message}`); }
  return {
    id,
    title: get('title'),
    goal: get('goal'),
    intro: get('intro'),
    challenge: get('challenge'),
    starter: get('starter') + '\n',
    solution: get('solution') + '\n',
    hints,
    tests
  };
}

const files = fs.readdirSync(srcDir).filter(f => /^\d+.*\.md$/.test(f)).sort();
const lessons = files.map((f, i) => parseLesson(f, i + 1));
const stripInfo = t => t.replace(/```kes input=[^\n]*/g, '```kes');
fs.writeFileSync(path.join(__dirname, 'lessons.json'), JSON.stringify(lessons, null, 1));

/* ---- TUTORIAL.md ---- */
const ref = JSON.parse(fs.readFileSync(path.join(__dirname, 'reference.json'), 'utf8'));
const head = fs.readFileSync(path.join(__dirname, 'tutorial-intro.md'), 'utf8').replace(/\n+$/, '');
const tail = fs.readFileSync(path.join(__dirname, 'tutorial-outro.md'), 'utf8').replace(/\n+$/, '');

let md = head + '\n\n---\n\n## Contents\n\n';
for (const L of lessons) md += `${L.id}. [${L.title}](#lesson-${L.id}-${L.title.toLowerCase().replace(/[^a-z0-9]+/g, '-').replace(/^-|-$/g, '')})${L.goal ? ' - ' + L.goal : ''}\n`;
md += '\n';
for (const L of lessons) {
  md += `\n---\n\n## Lesson ${L.id}: ${L.title}\n\n${stripInfo(L.intro)}\n\n### Your turn\n\n${stripInfo(L.challenge)}\n\n`;
  md += 'Starter code:\n\n```kes\n' + L.starter.replace(/\n+$/, '') + '\n```\n\n';
  if (L.hints.length) md += '<details><summary>Hints</summary>\n\n' + L.hints.map(h => '- ' + h).join('\n') + '\n\n</details>\n\n';
  md += '<details><summary>Solution (try first!)</summary>\n\n```kes\n' + L.solution.replace(/\n+$/, '') + '\n```\n\n</details>\n';
}
md += '\n---\n\n' + tail + '\n\n---\n\n## Quick reference\n\n';
for (const sec of ref.syntax) {
  md += `### ${sec.title}\n\n\`\`\`kes\n${sec.code}\n\`\`\`\n\n${sec.note ? sec.note + '\n\n' : ''}`;
}
md += '### Built-in functions\n\n';
for (const g of ref.builtins) {
  md += `**${g.group}**\n\n`;
  for (const f of g.items) md += `- \`${f.sig}\`: ${f.desc}\n`;
  md += '\n';
}
fs.writeFileSync(path.join(root, 'TUTORIAL.md'), md);
console.log(`Built ${lessons.length} lessons and TUTORIAL.md`);
