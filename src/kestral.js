/*  Kestral 1.0.0 — a small, friendly, general-purpose programming language.
    Source files use the .kes extension.
    This single file runs in Node.js and in the browser. */
(function (root, factory) {
  if (typeof module === 'object' && module.exports) module.exports = factory();
  else root.Kestral = factory();
})(typeof self !== 'undefined' ? self : this, function () {
'use strict';

const VERSION = '1.0.0';

/* ------------------------------------------------------------------ *
 *  Errors
 * ------------------------------------------------------------------ */
class KestralError extends Error {
  constructor(message, line, col, opts) {
    super(message);
    this.name = 'KestralError';
    this.line = line || 0;
    this.col = col || 0;
    this.fatal = !!(opts && opts.fatal);          // cannot be caught by try/catch
    this.phase = (opts && opts.phase) || 'runtime';
    this.incomplete = !!(opts && opts.incomplete); // syntax error because input ended early
  }
}

/* ------------------------------------------------------------------ *
 *  Lexer
 * ------------------------------------------------------------------ */
const KEYWORDS = new Set([
  'let', 'fn', 'if', 'else', 'while', 'for', 'in', 'loop', 'times', 'stop', 'skip',
  'give', 'try', 'catch', 'fail', 'match', 'and', 'or', 'not', 'true', 'false',
  'nothing', 'say', 'use'
]);
const TWO_CHAR = new Set(['==', '!=', '<=', '>=', '+=', '-=', '*=', '/=', '%=', '..', '=>', '|>', '??', '**']);
const ONE_CHAR = new Set(['+', '-', '*', '/', '%', '<', '>', '=', '(', ')', '[', ']', '{', '}', ',', '.', ':']);

const isDigit = c => c >= '0' && c <= '9';
const isIdStart = c => (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c === '_';
const isIdPart = c => isIdStart(c) || isDigit(c);

function lex(src, line0) {
  const out = [];
  let i = 0, line = line0 || 1, ls = 0;
  const n = src.length;
  const tok = (type, v, s, extra) => Object.assign({ type, v, line, col: s - ls + 1 }, extra);
  const synErr = (msg, s) => { throw new KestralError(msg, line, s - ls + 1, { phase: 'syntax' }); };

  while (i < n) {
    const c = src[i];
    if (c === ' ' || c === '\t' || c === '\r') { i++; continue; }
    if (c === '\n') { out.push(tok('nl', '\n', i)); i++; line++; ls = i; continue; }
    if (c === ';') { out.push(tok('nl', ';', i)); i++; continue; }
    if (c === '/' && src[i + 1] === '/') { while (i < n && src[i] !== '\n') i++; continue; }

    if (isDigit(c)) {
      const s = i;
      while (i < n && isDigit(src[i])) i++;
      if (src[i] === '.' && isDigit(src[i + 1] || '')) { i++; while (i < n && isDigit(src[i])) i++; }
      if (i < n && isIdStart(src[i])) synErr(`A number can't be followed by letters ("${src.slice(s, i + 1)}...")`, s);
      out.push(tok('num', parseFloat(src.slice(s, i)), s));
      continue;
    }

    if (isIdStart(c)) {
      const s = i;
      while (i < n && isIdPart(src[i])) i++;
      const word = src.slice(s, i);
      out.push(tok(KEYWORDS.has(word) ? 'kw' : 'id', word, s));
      continue;
    }

    if (c === '"' || c === "'") {
      const s = i, q = c, startLine = line, startCol = s - ls + 1;
      i++;
      const parts = [];
      let cur = '';
      for (;;) {
        if (i >= n || src[i] === '\n') {
          throw new KestralError('This text never closes. Add a closing ' + q + ' at the end.', startLine, startCol, { phase: 'syntax' });
        }
        const ch = src[i];
        if (ch === q) { i++; break; }
        if (ch === '\\') {
          const e = src[i + 1];
          const map = { n: '\n', t: '\t', '\\': '\\', '"': '"', "'": "'", '{': '{', '}': '}' };
          if (e !== undefined && Object.prototype.hasOwnProperty.call(map, e)) { cur += map[e]; i += 2; }
          else { cur += '\\'; i++; }
          continue;
        }
        if (ch === '{') {
          let depth = 1, j = i + 1;
          while (j < n && depth > 0) {
            const d = src[j];
            if (d === '\n') break;
            if (d === '{') depth++;
            else if (d === '}') depth--;
            else if (d === '"' || d === "'") {
              const qq = d; j++;
              while (j < n && src[j] !== qq && src[j] !== '\n') { if (src[j] === '\\') j++; j++; }
            }
            j++;
          }
          if (depth !== 0) {
            throw new KestralError('This { inside the text never closes. Use \\{ if you want a literal brace.', line, i - ls + 1, { phase: 'syntax' });
          }
          const exprSrc = src.slice(i + 1, j - 1);
          if (!exprSrc.trim()) {
            throw new KestralError('Empty { } inside text. Put a value inside, or write \\{ \\} for literal braces.', line, i - ls + 1, { phase: 'syntax' });
          }
          if (cur) parts.push(cur);
          cur = '';
          parts.push({ expr: exprSrc, line, col: i - ls + 2 });
          i = j;
          continue;
        }
        cur += ch; i++;
      }
      if (cur || parts.length === 0) parts.push(cur);
      out.push(tok('str', null, s, { parts, line: startLine, col: startCol }));
      continue;
    }

    const two = src.substr(i, 2);
    if (TWO_CHAR.has(two)) { out.push(tok('op', two, i)); i += 2; continue; }
    if (ONE_CHAR.has(c)) { out.push(tok('op', c, i)); i++; continue; }
    if (c === '&' && src[i + 1] === '&') synErr('Use the word and instead of &&.', i);
    if (c === '|' && src[i + 1] === '|') synErr('Use the word or instead of ||.', i);
    if (c === '!') synErr('Use the word not instead of !  (for "not equal" write != ).', i);
    synErr(`I don't understand the character "${c}".`, i);
  }
  out.push({ type: 'eof', v: null, line, col: i - ls + 1 });
  return out;
}

/* ------------------------------------------------------------------ *
 *  Parser
 * ------------------------------------------------------------------ */
function describe(t) {
  if (t.type === 'nl') return 'the end of the line';
  if (t.type === 'eof') return 'the end of the program';
  if (t.type === 'str') return 'some text';
  if (t.type === 'num') return `the number ${t.v}`;
  return `"${t.v}"`;
}

class Parser {
  constructor(tokens) { this.t = tokens; this.p = 0; }
  get cur() { return this.t[this.p]; }
  peek(k) { return this.t[Math.min(this.p + (k || 1), this.t.length - 1)]; }
  err(msg, tok) {
    tok = tok || this.cur;
    throw new KestralError(msg, tok.line, tok.col, { phase: 'syntax', incomplete: tok.type === 'eof' });
  }
  isOp(v) { const c = this.cur; return c.type === 'op' && c.v === v; }
  isKw(v) { const c = this.cur; return c.type === 'kw' && c.v === v; }
  eatOp(v) { if (this.isOp(v)) { this.p++; return true; } return false; }
  eatKw(v) { if (this.isKw(v)) { this.p++; return true; } return false; }
  expectOp(v, hint) {
    if (!this.eatOp(v)) this.err(hint || `I expected "${v}" here but found ${describe(this.cur)}.`);
  }
  expectKw(v, hint) {
    if (!this.eatKw(v)) this.err(hint || `I expected "${v}" here but found ${describe(this.cur)}.`);
  }
  skipNL() { while (this.cur.type === 'nl') this.p++; }
  atEnd() { const c = this.cur; return c.type === 'nl' || c.type === 'eof' || (c.type === 'op' && c.v === '}'); }

  // If a newline is followed by the operator `v`, step over the newlines (for chains).
  nlThenOp(v) {
    if (this.cur.type !== 'nl') return false;
    let k = this.p;
    while (this.t[k].type === 'nl') k++;
    if (this.t[k].type === 'op' && this.t[k].v === v) { this.p = k; return true; }
    return false;
  }

  parseProgram() {
    const body = [];
    this.skipNL();
    while (this.cur.type !== 'eof') {
      body.push(this.parseStatement());
      this.endStatement();
      this.skipNL();
    }
    return { type: 'Block', body, line: 1 };
  }

  endStatement() {
    const c = this.cur;
    if (c.type === 'nl') { this.p++; return; }
    if (c.type === 'eof' || (c.type === 'op' && c.v === '}')) return;
    this.err(`I didn't expect ${describe(c)} here. Put each statement on its own line.`);
  }

  parseBlock() {
    const open = this.cur;
    if (!this.isOp('{')) this.err(`I expected a { to start a block but found ${describe(this.cur)}.`);
    this.p++;
    this.skipNL();
    const body = [];
    while (!this.isOp('}')) {
      if (this.cur.type === 'eof') throw new KestralError('This block was opened with { but never closed with }.', open.line, open.col, { phase: 'syntax', incomplete: true });
      body.push(this.parseStatement());
      this.endStatement();
      this.skipNL();
    }
    this.p++;
    return { type: 'Block', body, line: open.line };
  }

  parseStatement() {
    const tk = this.cur, line = tk.line, col = tk.col;
    if (tk.type === 'kw') {
      switch (tk.v) {
        case 'let': {
          this.p++;
          const name = this.cur;
          if (name.type !== 'id') this.err(`After "let" I expected a name, like  let score = 10`);
          this.p++;
          this.expectOp('=', `After "let ${name.v}" I expected "=" , like  let ${name.v} = 10`);
          this.skipNL();
          const value = this.parseExpr();
          return { type: 'Let', name: name.v, value, line, col };
        }
        case 'say': {
          this.p++;
          const args = [];
          if (!this.atEnd()) {
            args.push(this.parseExpr());
            while (this.eatOp(',')) { this.skipNL(); args.push(this.parseExpr()); }
          }
          return { type: 'Say', args, line, col };
        }
        case 'fn': {
          if (this.peek().type === 'id') {
            this.p++;
            const name = this.cur.v; this.p++;
            const params = this.parseParams();
            const body = this.parseFnBody();
            return { type: 'FnDecl', name, params, body, line, col };
          }
          break;
        }
        case 'if': return this.parseIf();
        case 'while': {
          this.p++;
          const cond = this.parseExpr();
          const body = this.parseBlock();
          return { type: 'While', cond, body, line, col };
        }
        case 'for': {
          this.p++;
          const names = [];
          if (this.cur.type !== 'id') this.err('After "for" I expected a name, like  for item in list { ... }');
          names.push(this.cur.v); this.p++;
          if (this.eatOp(',')) {
            if (this.cur.type !== 'id') this.err('After the comma I expected a second name, like  for key, value in map { ... }');
            names.push(this.cur.v); this.p++;
          }
          this.expectKw('in', `I expected the word "in" here, like  for ${names[0]} in list { ... }`);
          const iter = this.parseExpr();
          const body = this.parseBlock();
          return { type: 'For', names, iter, body, line, col };
        }
        case 'loop': {
          this.p++;
          const count = this.parseExpr();
          this.expectKw('times', 'I expected the word "times", like  loop 3 times { ... }');
          const body = this.parseBlock();
          return { type: 'Loop', count, body, line, col };
        }
        case 'stop': this.p++; return { type: 'Stop', line, col };
        case 'skip': this.p++; return { type: 'Skip', line, col };
        case 'give': {
          this.p++;
          const value = this.atEnd() ? null : this.parseExpr();
          return { type: 'Give', value, line, col };
        }
        case 'try': {
          this.p++;
          const body = this.parseBlock();
          this.nlThenKw('catch');
          this.expectKw('catch', 'A try block needs a catch block after it, like  try { ... } catch e { ... }');
          let name = null;
          if (this.cur.type === 'id') { name = this.cur.v; this.p++; }
          const handler = this.parseBlock();
          return { type: 'Try', body, name, handler, line, col };
        }
        case 'fail': {
          this.p++;
          const value = this.parseExpr();
          return { type: 'Fail', value, line, col };
        }
        case 'use': {
          this.p++;
          const s = this.cur;
          if (s.type !== 'str' || s.parts.length !== 1 || typeof s.parts[0] !== 'string') this.err('After "use" put the file name in quotes, like  use "tools.kes"');
          this.p++;
          return { type: 'Use', path: s.parts[0], line, col };
        }
        default: break;
      }
    }
    // expression or assignment
    const expr = this.parseExpr();
    const c = this.cur;
    if (expr.type === 'Ident' && !this.atEnd() && !(c.type === 'op' && /^(=|\+=|-=|\*=|\/=|%=|\||\?\?|\.\.|,)$/.test(c.v))) {
      const sug = suggest(expr.name, STATEMENT_WORDS);
      if (sug) this.err(`"${expr.name}" isn't a command I know. Did you mean "${sug}"?`, tk);
    }
    if (c.type === 'op' && (c.v === '=' || c.v === '+=' || c.v === '-=' || c.v === '*=' || c.v === '/=' || c.v === '%=')) {
      if (expr.type !== 'Ident' && expr.type !== 'Index' && expr.type !== 'Member') {
        this.err("You can only assign to a name, a list position like  items[0] , or a map key like  person.name", c);
      }
      this.p++;
      this.skipNL();
      const value = this.parseExpr();
      return { type: 'Assign', op: c.v, target: expr, value, line, col };
    }
    return expr;
  }

  nlThenKw(kw) {
    if (this.cur.type !== 'nl') return;
    let k = this.p;
    while (this.t[k].type === 'nl') k++;
    if (this.t[k].type === 'kw' && this.t[k].v === kw) this.p = k;
  }

  parseIf() {
    const tk = this.cur;
    this.p++;
    const cond = this.parseExpr();
    if (!this.isOp('{')) this.err(`I expected a { after the condition but found ${describe(this.cur)}.`);
    const then = this.parseBlock();
    let els = null;
    this.nlThenKw('else');
    if (this.eatKw('else')) {
      if (this.isKw('if')) els = this.parseIf();
      else els = this.parseBlock();
    }
    return { type: 'If', cond, then, else: els, line: tk.line, col: tk.col };
  }

  parseParams() {
    this.expectOp('(', 'I expected ( to start the parameter list, like  fn add(a, b) { ... }');
    const ps = [];
    this.skipNL();
    while (!this.isOp(')')) {
      const id = this.cur;
      if (id.type !== 'id') this.err(`I expected a parameter name but found ${describe(id)}.`);
      this.p++;
      let def = null;
      if (this.eatOp('=')) def = this.parseExpr();
      ps.push({ name: id.v, default: def });
      this.skipNL();
      if (!this.eatOp(',')) break;
      this.skipNL();
    }
    this.skipNL();
    this.expectOp(')', 'I expected ) to close the parameter list.');
    return ps;
  }

  parseFnBody() {
    const tk = this.cur;
    if (this.eatOp('=>')) {
      this.skipNL();
      const e = this.parseExpr();
      return { type: 'Block', body: [{ type: 'Give', value: e, line: tk.line, col: tk.col }], line: tk.line };
    }
    if (this.isOp('{')) return this.parseBlock();
    this.err(`After the parameters I expected { or => but found ${describe(this.cur)}.`);
  }

  parseArgs() {
    this.expectOp('(');
    const args = [];
    this.skipNL();
    while (!this.isOp(')')) {
      args.push(this.parseExpr());
      this.skipNL();
      if (!this.eatOp(',')) break;
      this.skipNL();
    }
    this.skipNL();
    this.expectOp(')', 'I expected ) to close this list of values.');
    return args;
  }

  /* ---- expressions, lowest precedence first ---- */
  parseExpr() { return this.parsePipe(); }

  parsePipe() {
    let left = this.parseCoalesce();
    for (;;) {
      this.nlThenOp('|>');
      if (this.isOp('|>')) {
        const tk = this.cur; this.p++; this.skipNL();
        const right = this.parsePostfix();
        left = { type: 'Pipe', left, right, line: tk.line, col: tk.col };
      } else break;
    }
    return left;
  }

  parseCoalesce() {
    let left = this.parseOr();
    while (this.isOp('??')) {
      const tk = this.cur; this.p++; this.skipNL();
      left = { type: 'Coalesce', left, right: this.parseOr(), line: tk.line, col: tk.col };
    }
    return left;
  }

  parseOr() {
    let left = this.parseAnd();
    while (this.isKw('or')) {
      const tk = this.cur; this.p++; this.skipNL();
      left = { type: 'Logic', op: 'or', left, right: this.parseAnd(), line: tk.line, col: tk.col };
    }
    return left;
  }

  parseAnd() {
    let left = this.parseNot();
    while (this.isKw('and')) {
      const tk = this.cur; this.p++; this.skipNL();
      left = { type: 'Logic', op: 'and', left, right: this.parseNot(), line: tk.line, col: tk.col };
    }
    return left;
  }

  parseNot() {
    if (this.isKw('not')) {
      const tk = this.cur; this.p++;
      return { type: 'Unary', op: 'not', arg: this.parseNot(), line: tk.line, col: tk.col };
    }
    return this.parseCompare();
  }

  parseCompare() {
    let left = this.parseRange();
    for (;;) {
      const c = this.cur;
      if (c.type === 'op' && (c.v === '==' || c.v === '!=' || c.v === '<' || c.v === '<=' || c.v === '>' || c.v === '>=')) {
        this.p++; this.skipNL();
        left = { type: 'Binary', op: c.v, left, right: this.parseRange(), line: c.line, col: c.col };
      } else break;
    }
    return left;
  }

  parseRange() {
    const left = this.parseAdd();
    if (this.isOp('..')) {
      const tk = this.cur; this.p++; this.skipNL();
      return { type: 'Range', from: left, to: this.parseAdd(), line: tk.line, col: tk.col };
    }
    return left;
  }

  parseAdd() {
    let left = this.parseMul();
    for (;;) {
      const c = this.cur;
      if (c.type === 'op' && (c.v === '+' || c.v === '-')) {
        this.p++; this.skipNL();
        left = { type: 'Binary', op: c.v, left, right: this.parseMul(), line: c.line, col: c.col };
      } else break;
    }
    return left;
  }

  parseMul() {
    let left = this.parseUnary();
    for (;;) {
      const c = this.cur;
      if (c.type === 'op' && (c.v === '*' || c.v === '/' || c.v === '%')) {
        this.p++; this.skipNL();
        left = { type: 'Binary', op: c.v, left, right: this.parseUnary(), line: c.line, col: c.col };
      } else break;
    }
    return left;
  }

  parseUnary() {
    if (this.isOp('-')) {
      const tk = this.cur; this.p++;
      return { type: 'Unary', op: 'neg', arg: this.parseUnary(), line: tk.line, col: tk.col };
    }
    return this.parsePower();
  }

  parsePower() {
    const base = this.parsePostfix();
    if (this.isOp('**')) {
      const tk = this.cur; this.p++; this.skipNL();
      return { type: 'Binary', op: '**', left: base, right: this.parseUnary(), line: tk.line, col: tk.col };
    }
    return base;
  }

  parsePostfix() {
    let e = this.parsePrimary();
    for (;;) {
      const c = this.cur;
      if (c.type === 'op' && c.v === '(') {
        const args = this.parseArgs();
        e = { type: 'Call', callee: e, args, line: c.line, col: c.col };
      } else if (c.type === 'op' && c.v === '[') {
        this.p++; this.skipNL();
        const index = this.parseExpr();
        this.skipNL();
        this.expectOp(']', 'I expected ] to close this position, like  items[0]');
        e = { type: 'Index', obj: e, index, line: c.line, col: c.col };
      } else if ((c.type === 'op' && c.v === '.') || this.nlThenOp('.')) {
        const dot = this.cur; this.p++;
        const nm = this.cur;
        if (nm.type !== 'id' && nm.type !== 'kw') this.err(`After "." I expected a name but found ${describe(nm)}.`);
        this.p++;
        e = { type: 'Member', obj: e, name: nm.v, line: dot.line, col: dot.col };
      } else break;
    }
    return e;
  }

  subParse(exprSrc, line, col) {
    let toks;
    try { toks = lex(exprSrc, line); }
    catch (e) { if (e instanceof KestralError) e.col += col - 1; throw e; }
    const p = new Parser(toks);
    p.skipNL();
    const node = p.parseExpr();
    p.skipNL();
    if (p.cur.type !== 'eof') p.err(`I didn't expect ${describe(p.cur)} inside { } in this text.`);
    return node;
  }

  parsePrimary() {
    const tk = this.cur;
    switch (tk.type) {
      case 'num': this.p++; return { type: 'Num', v: tk.v, line: tk.line, col: tk.col };
      case 'str': {
        this.p++;
        const parts = tk.parts.map(pt => typeof pt === 'string' ? pt : this.subParse(pt.expr, pt.line, pt.col));
        return { type: 'Str', parts, line: tk.line, col: tk.col };
      }
      case 'id': this.p++; return { type: 'Ident', name: tk.v, line: tk.line, col: tk.col };
      case 'kw':
        switch (tk.v) {
          case 'true': this.p++; return { type: 'Bool', v: true, line: tk.line, col: tk.col };
          case 'false': this.p++; return { type: 'Bool', v: false, line: tk.line, col: tk.col };
          case 'nothing': this.p++; return { type: 'Nothing', line: tk.line, col: tk.col };
          case 'fn': {
            this.p++;
            if (this.cur.type === 'id') this.err(`Named functions are declared on their own line, like  fn ${this.cur.v}(...) { ... }`);
            const params = this.parseParams();
            const body = this.parseFnBody();
            return { type: 'Lambda', params, body, line: tk.line, col: tk.col };
          }
          case 'if': return this.parseIf();
          case 'match': return this.parseMatch();
          default: break;
        }
        break;
      case 'op':
        if (tk.v === '(') {
          this.p++; this.skipNL();
          const e = this.parseExpr();
          this.skipNL();
          this.expectOp(')', 'I expected ) to close this bracket.');
          return e;
        }
        if (tk.v === '[') {
          this.p++; this.skipNL();
          const items = [];
          while (!this.isOp(']')) {
            items.push(this.parseExpr());
            this.skipNL();
            if (!this.eatOp(',')) break;
            this.skipNL();
          }
          this.skipNL();
          this.expectOp(']', 'I expected ] to close this list.');
          return { type: 'List', items, line: tk.line, col: tk.col };
        }
        if (tk.v === '{') {
          this.p++; this.skipNL();
          const entries = [];
          while (!this.isOp('}')) {
            const k = this.cur;
            let key;
            if (k.type === 'id' || k.type === 'kw') key = k.v;
            else if (k.type === 'num') key = k.v;
            else if (k.type === 'str' && k.parts.every(x => typeof x === 'string')) key = k.parts.join('');
            else this.err(`A map key should be a name or some text, but I found ${describe(k)}.`);
            this.p++;
            this.expectOp(':', `After the key I expected ":" , like  {name: "Ada"}`);
            this.skipNL();
            entries.push({ key, value: this.parseExpr() });
            this.skipNL();
            if (!this.eatOp(',')) break;
            this.skipNL();
          }
          this.skipNL();
          this.expectOp('}', 'I expected } to close this map.');
          return { type: 'MapLit', entries, line: tk.line, col: tk.col };
        }
        break;
      default: break;
    }
    if (tk.type === 'eof') this.err('The program ended but I was still expecting a value.');
    if (tk.type === 'nl') this.err('I expected a value here but the line ended.');
    this.err(`I expected a value here but found ${describe(tk)}.`);
  }

  parseMatch() {
    const tk = this.cur;
    this.p++;
    const subject = this.parseExpr();
    this.expectOp('{', 'I expected a { after the value being matched, like  match x { ... }');
    this.skipNL();
    const arms = [];
    while (!this.isOp('}')) {
      if (this.cur.type === 'eof') throw new KestralError('This match was opened with { but never closed with }.', tk.line, tk.col, { phase: 'syntax', incomplete: true });
      let pats = null;
      if (this.isKw('else')) this.p++;
      else {
        pats = [this.parseExpr()];
        while (this.eatOp(',')) { this.skipNL(); pats.push(this.parseExpr()); }
      }
      this.expectOp('=>', 'Each match line needs =>  after its value(s), like  1 => "one"');
      this.skipNL();
      const body = this.isOp('{') ? this.parseBlock() : this.parseStatement();
      arms.push({ pats, body });
      if (!this.atEnd()) this.err(`I didn't expect ${describe(this.cur)} here. Put each match line on its own line.`);
      this.skipNL();
    }
    this.p++;
    return { type: 'Match', subject, arms, line: tk.line, col: tk.col };
  }
}

function parse(src, line0) {
  return new Parser(lex(src, line0)).parseProgram();
}

/* ------------------------------------------------------------------ *
 *  Runtime values
 * ------------------------------------------------------------------ */
class Env {
  constructor(parent) { this.vars = new Map(); this.parent = parent || null; }
  lookup(name) {
    let e = this;
    while (e) { if (e.vars.has(name)) return e; e = e.parent; }
    return null;
  }
  define(name, v) { this.vars.set(name, v); }
  names() {
    const s = new Set();
    for (let e = this; e; e = e.parent) for (const k of e.vars.keys()) s.add(k);
    return [...s];
  }
}
class KFn {
  constructor(name, params, body, closure) { this.name = name; this.params = params; this.body = body; this.closure = closure; }
}
class Builtin {
  constructor(name, min, max, fn) { this.name = name; this.min = min; this.max = max; this.fn = fn; }
}
class Bound {
  constructor(fn, self) { this.fn = fn; this.self = self; }
}
class KRange {
  constructor(from, to) { this.from = from; this.to = to; }
  get length() { return Math.abs(this.to - this.from) + 1; }
  at(i) { return this.from <= this.to ? this.from + i : this.from - i; }
  toArray() { const a = new Array(this.length); for (let i = 0; i < a.length; i++) a[i] = this.at(i); return a; }
  contains(x) { return typeof x === 'number' && x >= Math.min(this.from, this.to) && x <= Math.max(this.from, this.to); }
}

const STATEMENT_WORDS = ['say', 'let', 'fn', 'if', 'else', 'while', 'for', 'loop', 'give', 'stop', 'skip', 'try', 'catch', 'fail', 'match', 'use'];
const isFn = v => v instanceof KFn || v instanceof Builtin || v instanceof Bound;

function typeName(v) {
  if (v === null || v === undefined) return 'nothing';
  switch (typeof v) {
    case 'number': return 'number';
    case 'string': return 'text';
    case 'boolean': return 'bool';
    default: break;
  }
  if (Array.isArray(v)) return 'list';
  if (v instanceof Map) return 'map';
  if (v instanceof KRange) return 'range';
  if (isFn(v)) return 'function';
  return 'value';
}

function fmtNumber(n) {
  if (Number.isNaN(n)) return 'NaN';
  if (!Number.isFinite(n)) return n > 0 ? 'Infinity' : '-Infinity';
  if (Number.isInteger(n)) return Math.abs(n) < 1e21 ? String(n) : n.toExponential();
  return String(parseFloat(n.toPrecision(12)));
}

function quote(s) {
  return '"' + s.replace(/\\/g, '\\\\').replace(/"/g, '\\"').replace(/\n/g, '\\n').replace(/\t/g, '\\t') + '"';
}

function format(v, nested, seen) {
  if (v === null || v === undefined) return 'nothing';
  switch (typeof v) {
    case 'number': return fmtNumber(v);
    case 'string': return nested ? quote(v) : v;
    case 'boolean': return v ? 'true' : 'false';
    default: break;
  }
  seen = seen || new Set();
  if (Array.isArray(v)) {
    if (seen.has(v)) return '[...]';
    seen.add(v);
    const s = '[' + v.map(x => format(x, true, seen)).join(', ') + ']';
    seen.delete(v);
    return s;
  }
  if (v instanceof Map) {
    if (seen.has(v)) return '{...}';
    seen.add(v);
    const parts = [];
    for (const [k, val] of v) {
      const ks = typeof k === 'string' ? (/^[A-Za-z_][A-Za-z0-9_]*$/.test(k) ? k : quote(k)) : format(k, true);
      parts.push(ks + ': ' + format(val, true, seen));
    }
    seen.delete(v);
    return '{' + parts.join(', ') + '}';
  }
  if (v instanceof KRange) return v.from + '..' + v.to;
  if (v instanceof KFn) return '<function ' + (v.name || 'anonymous') + '>';
  if (v instanceof Builtin) return '<built-in ' + v.name + '>';
  if (v instanceof Bound) return '<function ' + (v.fn.name || 'anonymous') + '>';
  return String(v);
}

function eq(a, b) {
  if (a === b) return true;
  if (a === null || b === null || a === undefined || b === undefined) return false;
  if (Array.isArray(a)) {
    if (!Array.isArray(b) || a.length !== b.length) return false;
    for (let i = 0; i < a.length; i++) if (!eq(a[i], b[i])) return false;
    return true;
  }
  if (a instanceof Map) {
    if (!(b instanceof Map) || a.size !== b.size) return false;
    for (const [k, v] of a) { if (!b.has(k) || !eq(v, b.get(k))) return false; }
    return true;
  }
  if (a instanceof KRange) return b instanceof KRange && a.from === b.from && a.to === b.to;
  return false;
}

const truthy = v => v !== null && v !== undefined && v !== false && v !== 0 && v !== '';

/* Edit distance where swapping two neighbouring letters ("sya" -> "say") counts as one edit. */
function editDistance(a, b) {
  const m = a.length, n = b.length;
  const d = [];
  for (let i = 0; i <= m; i++) { d.push(new Array(n + 1).fill(0)); d[i][0] = i; }
  for (let j = 0; j <= n; j++) d[0][j] = j;
  for (let i = 1; i <= m; i++) {
    for (let j = 1; j <= n; j++) {
      const cost = a[i - 1] === b[j - 1] ? 0 : 1;
      d[i][j] = Math.min(d[i - 1][j] + 1, d[i][j - 1] + 1, d[i - 1][j - 1] + cost);
      if (i > 1 && j > 1 && a[i - 1] === b[j - 2] && a[i - 2] === b[j - 1]) d[i][j] = Math.min(d[i][j], d[i - 2][j - 2] + 1);
    }
  }
  return d[m][n];
}

function suggest(name, names) {
  let best = null, bd = Infinity;
  const lname = name.toLowerCase();
  for (const c of names) {
    const d = editDistance(lname, c.toLowerCase());
    if (d < bd) { bd = d; best = c; }
  }
  const limit = name.length <= 4 ? 1 : 2;
  return best !== null && bd <= limit ? best : null;
}

/* ------------------------------------------------------------------ *
 *  Interpreter
 * ------------------------------------------------------------------ */
class Interp {
  constructor(opts) {
    this.opts = opts || {};
    this.global = new Env(null);
    this.steps = 0;
    this.maxSteps = this.opts.maxSteps || 5e6;
    this.maxOutput = this.opts.maxOutput || 500000;
    this.maxDepth = this.opts.maxDepth || 800;
    this.outLen = 0;
    this.depth = 0;
    this.sig = null;       // 'break' | 'continue' | 'return'
    this.retVal = null;
    this.curNode = null;
    this.loaded = new Set();
    installBuiltins(this);
  }

  /* ----- helpers ----- */
  fail(msg, node, fatal) {
    node = node || this.curNode || {};
    throw new KestralError(msg, node.line || 0, node.col || 0, { fatal: !!fatal });
  }
  str(v) { return format(v, false); }
  print(s) {
    this.outLen += s.length + 1;
    if (this.outLen > this.maxOutput) this.fail('This program printed too much text and was stopped.', this.curNode, true);
    if (this.opts.print) this.opts.print(s);
  }
  define(name, v) { this.global.define(name, v); }

  /* ----- running ----- */
  run(source, runOpts) {
    runOpts = runOpts || {};
    const env = runOpts.env || this.global;
    const ast = parse(source, 1);
    this.steps = 0; this.outLen = 0; this.depth = 0; this.sig = null;
    let value = null;
    try {
      value = this.execBlock(ast, env);
    } catch (e) {
      this.sig = null;
      if (e instanceof RangeError && /call stack/i.test(e.message)) {
        throw new KestralError('This went too deep. Does a function keep calling itself without stopping?', 0, 0, { fatal: true });
      }
      throw e;
    }
    if (this.sig) {
      const s = this.sig; this.sig = null;
      const where = s === 'return' ? '"give" only works inside a function.' : `"${s === 'break' ? 'stop' : 'skip'}" only works inside a loop.`;
      throw new KestralError(where, 0, 0);
    }
    const last = ast.body[ast.body.length - 1];
    this.lastWasExpr = !!last && !STATEMENT_TYPES.has(last.type);
    return value;
  }

  execBlock(block, env) {
    let last = null;
    const body = block.body;
    // functions declared with `fn name(...)` can be used anywhere in their block
    if (block.fnDecls === undefined) block.fnDecls = body.filter(s => s.type === 'FnDecl');
    for (let i = 0; i < block.fnDecls.length; i++) {
      const d = block.fnDecls[i];
      const f = new KFn(d.name, d.params, d.body, env);
      f.decl = d;
      env.define(d.name, f);
    }
    for (let i = 0; i < body.length; i++) {
      last = this.ev(body[i], env);
      if (this.sig) break;
    }
    return last;
  }
  scoped(block, env) { return this.execBlock(block, new Env(env)); }

  /* ----- calling ----- */
  callValue(f, args, node, calleeName) {
    if (f instanceof KFn) return this.callFn(f, args, node);
    if (f instanceof Builtin) {
      if (args.length < f.min || args.length > f.max) {
        const need = f.min === f.max ? `${f.min}` : (f.max === Infinity ? `at least ${f.min}` : `${f.min} to ${f.max}`);
        this.fail(`${f.name}() needs ${need} value${need === '1' ? '' : 's'} but got ${args.length}.`, node);
      }
      const prev = this.curNode;
      this.curNode = node;
      try { return f.fn.apply(this, args); }
      finally { this.curNode = prev; }
    }
    if (f instanceof Bound) return this.callValue(f.fn, [f.self].concat(args), node);
    this.fail(calleeName
      ? `"${calleeName}" is ${typeName(f) === 'nothing' ? 'nothing' : 'a ' + typeName(f)}, not a function, so it can't be called with ( ).`
      : `This value is ${typeName(f) === 'nothing' ? 'nothing' : 'a ' + typeName(f)}, not a function, so it can't be called with ( ).`, node);
  }

  callFn(f, args, node) {
    const name = f.name || 'this function';
    const required = f.params.filter(p => !p.default).length;
    if (args.length > f.params.length || args.length < required) {
      const want = required === f.params.length ? `${required}` : `${required} to ${f.params.length}`;
      this.fail(`${name}() needs ${want} value${want === '1' ? '' : 's'} but got ${args.length}.`, node);
    }
    if (++this.depth > this.maxDepth) {
      this.depth = 0;
      this.fail('This went too deep. Does a function keep calling itself without stopping?', node, true);
    }
    const env = new Env(f.closure);
    for (let i = 0; i < f.params.length; i++) {
      const p = f.params[i];
      env.define(p.name, i < args.length ? args[i] : this.ev(p.default, env));
    }
    try { this.execBlock(f.body, env); }
    finally { this.depth--; }
    let result = null;
    if (this.sig === 'return') { result = this.retVal; this.retVal = null; }
    this.sig = null;
    return result;
  }

  /* ----- operators ----- */
  needNum(v, op, node, side) {
    if (typeof v !== 'number') this.fail(`The ${op} operator needs numbers, but the ${side} side is ${typeName(v) === 'nothing' ? 'nothing' : 'a ' + typeName(v)}${typeof v === 'string' ? ' ("' + v + '")' : ''}.`, node);
  }

  binop(op, a, b, node) {
    switch (op) {
      case '+':
        if (typeof a === 'number' && typeof b === 'number') return a + b;
        if (typeof a === 'string' || typeof b === 'string') return this.str(a) + this.str(b);
        if (Array.isArray(a) && Array.isArray(b)) return a.concat(b);
        this.fail(`I can't add ${typeName(a)} and ${typeName(b)}.`, node);
        break;
      case '-': this.needNum(a, '-', node, 'left'); this.needNum(b, '-', node, 'right'); return a - b;
      case '*':
        if (typeof a === 'string' && typeof b === 'number') return this.repeatText(a, b, node);
        if (typeof a === 'number' && typeof b === 'string') return this.repeatText(b, a, node);
        this.needNum(a, '*', node, 'left'); this.needNum(b, '*', node, 'right'); return a * b;
      case '/':
        this.needNum(a, '/', node, 'left'); this.needNum(b, '/', node, 'right');
        if (b === 0) this.fail('Cannot divide by zero.', node);
        return a / b;
      case '%':
        this.needNum(a, '%', node, 'left'); this.needNum(b, '%', node, 'right');
        if (b === 0) this.fail('Cannot divide by zero (in %).', node);
        return ((a % b) + b) % b;
      case '**': this.needNum(a, '**', node, 'left'); this.needNum(b, '**', node, 'right'); return Math.pow(a, b);
      case '==': return eq(a, b);
      case '!=': return !eq(a, b);
      case '<': case '<=': case '>': case '>=': {
        const ok = (typeof a === 'number' && typeof b === 'number') || (typeof a === 'string' && typeof b === 'string');
        if (!ok) this.fail(`I can't compare ${typeName(a)} with ${typeName(b)} using ${op}.`, node);
        if (op === '<') return a < b;
        if (op === '<=') return a <= b;
        if (op === '>') return a > b;
        return a >= b;
      }
      default: break;
    }
    this.fail('Unknown operator ' + op, node);
  }

  repeatText(s, n, node) {
    if (!Number.isInteger(n) || n < 0) this.fail('To repeat text, multiply by a whole number that is 0 or more.', node);
    if (s.length * n > 1e6) this.fail('That repeated text would be too long.', node);
    return s.repeat(n);
  }

  getIndex(o, i, node) {
    if (Array.isArray(o) || typeof o === 'string' || o instanceof KRange) {
      const len = o instanceof KRange ? o.length : o.length;
      if (typeof i !== 'number' || !Number.isInteger(i)) this.fail(`A position must be a whole number, but I got ${typeName(i)}.`, node);
      let k = i < 0 ? i + len : i;
      if (k < 0 || k >= len) this.fail(`Position ${i} is outside this ${typeName(o)} (it has ${len} item${len === 1 ? '' : 's'}; positions go from 0 to ${len - 1}).`, node);
      return o instanceof KRange ? o.at(k) : o[k];
    }
    if (o instanceof Map) { const v = o.get(i); return v === undefined ? null : v; }
    this.fail(`I can't use [ ] on ${typeName(o) === 'nothing' ? 'nothing' : 'a ' + typeName(o)}.`, node);
  }

  checkKey(k, node) {
    if (typeof k !== 'string' && typeof k !== 'number' && typeof k !== 'boolean') this.fail('Map keys must be text, numbers or true/false.', node);
  }

  setIndex(o, i, v, node) {
    if (Array.isArray(o)) {
      const len = o.length;
      if (typeof i !== 'number' || !Number.isInteger(i)) this.fail(`A position must be a whole number, but I got ${typeName(i)}.`, node);
      const k = i < 0 ? i + len : i;
      if (k < 0 || k >= len) this.fail(`Position ${i} is outside this list (it has ${len} item${len === 1 ? '' : 's'}). Use push(list, value) to add to the end.`, node);
      o[k] = v; return;
    }
    if (o instanceof Map) { this.checkKey(i, node); o.set(i, v); return; }
    if (typeof o === 'string') this.fail("Text can't be changed in place. Build a new text instead.", node);
    this.fail(`I can't assign into ${typeName(o) === 'nothing' ? 'nothing' : 'a ' + typeName(o)}.`, node);
  }

  getMember(o, name, node) {
    if (o instanceof Map && o.has(name)) return o.get(name);
    if (o === null || o === undefined) this.fail(`I can't read ".${name}" because the value is nothing.`, node);
    const g = this.global.vars.get(name);
    if (isFn(g)) return new Bound(g, o);
    if (o instanceof Map) return null;
    this.fail(`A ${typeName(o)} has no "${name}". (Methods like  value.upper()  work for built-in functions such as upper, len, push, ...)`, node);
  }

  /* ----- evaluation ----- */
  ev(n, env) {
    if (++this.steps > this.maxSteps) {
      this.fail(`This program ran for too long (over ${this.maxSteps.toLocaleString ? this.maxSteps.toLocaleString('en-US') : this.maxSteps} steps). Is there a loop that never stops?`, n, true);
    }
    switch (n.type) {
      case 'Num': return n.v;
      case 'Bool': return n.v;
      case 'Nothing': return null;
      case 'Str': {
        const ps = n.parts;
        if (ps.length === 1 && typeof ps[0] === 'string') return ps[0];
        let s = '';
        for (let i = 0; i < ps.length; i++) s += typeof ps[i] === 'string' ? ps[i] : this.str(this.ev(ps[i], env));
        return s;
      }
      case 'Ident': {
        const e = env.lookup(n.name);
        if (!e) {
          const sug = suggest(n.name, env.names());
          this.fail(`I don't know "${n.name}".${sug ? ` Did you mean "${sug}"?` : ' Create it first with  let ' + n.name + ' = ...'}`, n);
        }
        return e.vars.get(n.name);
      }
      case 'List': { const a = new Array(n.items.length); for (let i = 0; i < a.length; i++) a[i] = this.ev(n.items[i], env); return a; }
      case 'MapLit': {
        const m = new Map();
        for (const en of n.entries) m.set(en.key, this.ev(en.value, env));
        return m;
      }
      case 'Unary': {
        const v = this.ev(n.arg, env);
        if (n.op === 'not') return !truthy(v);
        if (typeof v !== 'number') this.fail(`I can't put a minus sign on ${typeName(v) === 'nothing' ? 'nothing' : 'a ' + typeName(v)}.`, n);
        return -v;
      }
      case 'Binary': return this.binop(n.op, this.ev(n.left, env), this.ev(n.right, env), n);
      case 'Logic': {
        const l = this.ev(n.left, env);
        if (n.op === 'and') return truthy(l) ? this.ev(n.right, env) : l;
        return truthy(l) ? l : this.ev(n.right, env);
      }
      case 'Coalesce': { const l = this.ev(n.left, env); return l === null ? this.ev(n.right, env) : l; }
      case 'Range': {
        const a = this.ev(n.from, env), b = this.ev(n.to, env);
        if (!Number.isInteger(a) || !Number.isInteger(b)) this.fail('A range like 1..5 needs whole numbers on both sides.', n);
        return new KRange(a, b);
      }
      case 'Pipe': {
        const left = this.ev(n.left, env);
        const r = n.right;
        if (r.type === 'Call') {
          const f = this.ev(r.callee, env);
          const args = [left];
          for (const a of r.args) args.push(this.ev(a, env));
          return this.callValue(f, args, r, r.callee.type === 'Ident' ? r.callee.name : null);
        }
        const f = this.ev(r, env);
        return this.callValue(f, [left], n, r.type === 'Ident' ? r.name : null);
      }
      case 'Call': {
        const f = this.ev(n.callee, env);
        const args = new Array(n.args.length);
        for (let i = 0; i < args.length; i++) args[i] = this.ev(n.args[i], env);
        return this.callValue(f, args, n, n.callee.type === 'Ident' ? n.callee.name : (n.callee.type === 'Member' ? n.callee.name : null));
      }
      case 'Index': return this.getIndex(this.ev(n.obj, env), this.ev(n.index, env), n);
      case 'Member': return this.getMember(this.ev(n.obj, env), n.name, n);
      case 'Lambda': return new KFn(null, n.params, n.body, env);
      case 'If':
        if (truthy(this.ev(n.cond, env))) return this.scoped(n.then, env);
        if (n.else) return n.else.type === 'If' ? this.ev(n.else, env) : this.scoped(n.else, env);
        return null;
      case 'Match': {
        const subj = this.ev(n.subject, env);
        for (const arm of n.arms) {
          let hit = arm.pats === null;
          if (!hit) {
            for (const p of arm.pats) {
              const pv = this.ev(p, env);
              if (pv instanceof KRange ? pv.contains(subj) : eq(subj, pv)) { hit = true; break; }
            }
          }
          if (hit) return arm.body.type === 'Block' ? this.scoped(arm.body, env) : this.ev(arm.body, env);
        }
        return null;
      }

      /* ----- statements ----- */
      case 'Let': {
        const v = this.ev(n.value, env);
        if (v instanceof KFn && !v.name) v.name = n.name;
        env.define(n.name, v);
        return null;
      }
      case 'Assign': return this.doAssign(n, env);
      case 'Say': {
        const parts = new Array(n.args.length);
        for (let i = 0; i < parts.length; i++) parts[i] = this.str(this.ev(n.args[i], env));
        this.curNode = n;
        this.print(parts.join(' '));
        return null;
      }
      case 'FnDecl': {
        const cur = env.vars.get(n.name);
        if (!(cur instanceof KFn && cur.decl === n)) {
          const f = new KFn(n.name, n.params, n.body, env);
          f.decl = n;
          env.define(n.name, f);
        }
        return null;
      }
      case 'While': {
        while (truthy(this.ev(n.cond, env))) {
          this.scoped(n.body, env);
          if (this.sig) { if (this.sig === 'break') { this.sig = null; break; } if (this.sig === 'continue') { this.sig = null; continue; } break; }
        }
        return null;
      }
      case 'Loop': {
        const c = this.ev(n.count, env);
        if (typeof c !== 'number' || !Number.isInteger(c) || c < 0) this.fail('"loop" needs a whole number of times, like  loop 3 times { ... }', n);
        for (let i = 0; i < c; i++) {
          this.scoped(n.body, env);
          if (this.sig) { if (this.sig === 'break') { this.sig = null; break; } if (this.sig === 'continue') { this.sig = null; continue; } break; }
        }
        return null;
      }
      case 'For': return this.doFor(n, env);
      case 'Stop': this.sig = 'break'; return null;
      case 'Skip': this.sig = 'continue'; return null;
      case 'Give': {
        const v = n.value ? this.ev(n.value, env) : null;
        this.retVal = v; this.sig = 'return';
        return null;
      }
      case 'Try': {
        try {
          return this.scoped(n.body, env);
        } catch (e) {
          if (e instanceof KestralError && !e.fatal) {
            const env2 = new Env(env);
            if (n.name) env2.define(n.name, e.message);
            return this.execBlock(n.handler, env2);
          }
          throw e;
        }
      }
      case 'Fail': {
        const v = this.ev(n.value, env);
        this.fail(this.str(v), n);
        break;
      }
      case 'Use': return this.doUse(n);
      default: this.fail('Internal error: unknown node ' + n.type, n);
    }
    return null;
  }

  doAssign(n, env) {
    const t = n.target;
    const compound = n.op !== '=';
    const apply = cur => this.binop(n.op[0], cur, this.ev(n.value, env), n);
    if (t.type === 'Ident') {
      const e = env.lookup(t.name);
      if (!e) {
        const sug = suggest(t.name, env.names());
        this.fail(`"${t.name}" doesn't exist yet${sug ? ` (did you mean "${sug}"?)` : ''}. Create it first with  let ${t.name} = ...`, n);
      }
      e.vars.set(t.name, compound ? apply(e.vars.get(t.name)) : this.ev(n.value, env));
      return null;
    }
    if (t.type === 'Index') {
      const o = this.ev(t.obj, env), i = this.ev(t.index, env);
      const v = compound ? apply(this.getIndex(o, i, t)) : this.ev(n.value, env);
      this.setIndex(o, i, v, n);
      return null;
    }
    const o = this.ev(t.obj, env);
    if (!(o instanceof Map)) this.fail(`I can only set ".${t.name}" on a map, not on ${typeName(o) === 'nothing' ? 'nothing' : 'a ' + typeName(o)}.`, n);
    const cur = o.has(t.name) ? o.get(t.name) : null;
    o.set(t.name, compound ? apply(cur) : this.ev(n.value, env));
    return null;
  }

  doFor(n, env) {
    const it = this.ev(n.iter, env);
    const [a, b] = n.names;
    const body = n.body;
    const step = (x, y) => {
      const e = new Env(env);
      e.define(a, x);
      if (b) e.define(b, y);
      this.execBlock(body, e);
      if (this.sig) {
        if (this.sig === 'break') { this.sig = null; return true; }
        if (this.sig === 'continue') { this.sig = null; return false; }
        return true;
      }
      return false;
    };
    if (Array.isArray(it)) {
      for (let i = 0; i < it.length; i++) if (b ? step(i, it[i]) : step(it[i])) break;
    } else if (typeof it === 'string') {
      for (let i = 0; i < it.length; i++) if (b ? step(i, it[i]) : step(it[i])) break;
    } else if (it instanceof KRange) {
      const len = it.length;
      for (let i = 0; i < len; i++) if (b ? step(i, it.at(i)) : step(it.at(i))) break;
    } else if (it instanceof Map) {
      for (const [k, v] of [...it]) if (b ? step(k, v) : step(k)) break;
    } else {
      this.fail(`I can't loop over ${typeName(it) === 'nothing' ? 'nothing' : 'a ' + typeName(it)}.${typeof it === 'number' ? ` Did you mean  loop ${it} times { ... }  or  for i in 1..${it} { ... } ?` : ''}`, n);
    }
    return null;
  }

  doUse(n) {
    if (!this.opts.readFile) this.fail('"use" works when running .kes files from the command line.', n);
    let p = n.path;
    if (!/\.kes$/.test(p)) p += '.kes';
    if (this.loaded.has(p)) return null;
    this.loaded.add(p);
    let text;
    try { text = this.opts.readFile(p); }
    catch (e) { this.fail(`I couldn't read the file "${p}".`, n); }
    const ast = parse(text, 1);
    const saved = [this.sig, this.retVal];
    this.execBlock(ast, this.global);
    this.sig = saved[0]; this.retVal = saved[1];
    return null;
  }
}

const STATEMENT_TYPES = new Set(['Let', 'Assign', 'Say', 'FnDecl', 'While', 'Loop', 'For', 'Stop', 'Skip', 'Give', 'Try', 'Fail', 'Use']);

/* ------------------------------------------------------------------ *
 *  Built-in functions
 * ------------------------------------------------------------------ */
function installBuiltins(I) {
  const def = (name, min, max, fn) => I.define(name, new Builtin(name, min, max, fn));

  const needNumber = (v, fname, what) => {
    if (typeof v !== 'number') I.fail(`${fname}() needs ${what || 'a number'}, but got ${typeName(v) === 'nothing' ? 'nothing' : 'a ' + typeName(v)}.`);
    return v;
  };
  const needText = (v, fname, what) => {
    if (typeof v !== 'string') I.fail(`${fname}() needs ${what || 'text'}, but got ${typeName(v) === 'nothing' ? 'nothing' : 'a ' + typeName(v)}.`);
    return v;
  };
  const needList = (v, fname, what) => {
    if (!Array.isArray(v)) I.fail(`${fname}() needs ${what || 'a list'}, but got ${typeName(v) === 'nothing' ? 'nothing' : 'a ' + typeName(v)}.`);
    return v;
  };
  const needFn = (v, fname) => {
    if (!isFn(v)) I.fail(`${fname}() needs a function, but got ${typeName(v) === 'nothing' ? 'nothing' : 'a ' + typeName(v)}.`);
    return v;
  };
  const items = (v, fname) => {
    if (Array.isArray(v)) return v;
    if (v instanceof KRange) { if (v.length > 5e6) I.fail('That range is too large.'); return v.toArray(); }
    if (typeof v === 'string') return v.split('');
    return I.fail(`${fname}() needs a list, but got ${typeName(v) === 'nothing' ? 'nothing' : 'a ' + typeName(v)}.`);
  };
  const call = (f, args) => I.callValue(f, args, I.curNode);
  const arity = f => (f instanceof KFn ? f.params.length : 1);
  const callItem = (f, item, i) => (arity(f) >= 2 ? call(f, [item, i]) : call(f, [item]));
  const toIndex = (v, len, fname) => {
    if (typeof v !== 'number' || !Number.isInteger(v)) I.fail(`${fname}() needs whole-number positions.`);
    return v < 0 ? Math.max(0, v + len) : Math.min(v, len);
  };
  const compare = (a, b) => {
    if (typeof a === 'number' && typeof b === 'number') return a - b;
    if (typeof a === 'string' && typeof b === 'string') return a < b ? -1 : a > b ? 1 : 0;
    return I.fail(`sort() can only order numbers or text, and can't compare ${typeName(a)} with ${typeName(b)}.`);
  };

  /* input / types */
  def('ask', 0, 1, function (prompt) {
    const p = prompt === undefined ? '' : this.str(prompt);
    const r = this.opts.ask ? this.opts.ask(p) : null;
    return r === null || r === undefined ? null : String(r);
  });
  def('type', 1, 1, v => typeName(v));
  def('str', 1, 1, v => I.str(v));
  def('num', 1, 1, v => {
    if (typeof v === 'number') return v;
    if (typeof v === 'string') {
      const t = v.trim();
      if (/^[-+]?(\d+\.?\d*|\.\d+)$/.test(t)) return parseFloat(t);
      I.fail(`I can't turn ${quote(v)} into a number.`);
    }
    return I.fail(`num() needs text or a number, but got ${typeName(v) === 'nothing' ? 'nothing' : 'a ' + typeName(v)}.`);
  });
  def('int', 1, 1, v => {
    if (typeof v === 'string') { const t = v.trim(); if (!/^[-+]?(\d+\.?\d*|\.\d+)$/.test(t)) I.fail(`I can't turn ${quote(v)} into a number.`); v = parseFloat(t); }
    needNumber(v, 'int');
    return Math.trunc(v);
  });
  def('list', 1, 1, v => {
    if (v instanceof Map) return [...v.keys()];
    return items(v, 'list').slice();
  });
  def('copy', 1, 1, v => (Array.isArray(v) ? v.slice() : v instanceof Map ? new Map(v) : v));

  /* numbers */
  def('abs', 1, 1, v => Math.abs(needNumber(v, 'abs')));
  def('floor', 1, 1, v => Math.floor(needNumber(v, 'floor')));
  def('ceil', 1, 1, v => Math.ceil(needNumber(v, 'ceil')));
  def('sqrt', 1, 1, v => {
    needNumber(v, 'sqrt');
    if (v < 0) I.fail("I can't take the square root of a negative number.");
    return Math.sqrt(v);
  });
  def('round', 1, 2, (v, d) => {
    needNumber(v, 'round');
    d = d === undefined ? 0 : needNumber(d, 'round', 'a number of digits');
    const m = Math.pow(10, d);
    const r = Math.sign(v) * Math.round((Math.abs(v) + Number.EPSILON * Math.abs(v)) * m) / m;
    return r === 0 ? 0 : r;
  });
  const extreme = (name, pick) => def(name, 1, Infinity, function (...a) {
    const xs = a.length === 1 && (Array.isArray(a[0]) || a[0] instanceof KRange) ? items(a[0], name) : a;
    if (xs.length === 0) I.fail(`${name}() needs at least one number.`);
    let best = needNumber(xs[0], name);
    for (let i = 1; i < xs.length; i++) { needNumber(xs[i], name); if (pick(xs[i], best)) best = xs[i]; }
    return best;
  });
  extreme('min', (x, b) => x < b);
  extreme('max', (x, b) => x > b);
  def('sum', 1, 1, v => {
    let t = 0;
    for (const x of items(v, 'sum')) t += needNumber(x, 'sum', 'a list of numbers');
    return t;
  });
  def('random', 0, 0, () => (I.opts.random ? I.opts.random() : Math.random()));
  def('randomInt', 2, 2, (lo, hi) => {
    needNumber(lo, 'randomInt'); needNumber(hi, 'randomInt');
    if (!Number.isInteger(lo) || !Number.isInteger(hi) || hi < lo) I.fail('randomInt(low, high) needs whole numbers with low <= high.');
    const r = I.opts.random ? I.opts.random() : Math.random();
    return lo + Math.floor(r * (hi - lo + 1));
  });

  /* collections */
  def('len', 1, 1, v => {
    if (typeof v === 'string' || Array.isArray(v)) return v.length;
    if (v instanceof Map) return v.size;
    if (v instanceof KRange) return v.length;
    return I.fail(`len() needs text, a list or a map, but got ${typeName(v) === 'nothing' ? 'nothing' : 'a ' + typeName(v)}.`);
  });
  def('push', 2, Infinity, (l, ...vs) => { needList(l, 'push'); for (const v of vs) l.push(v); return l; });
  def('pop', 1, 1, l => {
    needList(l, 'pop');
    if (l.length === 0) I.fail("I can't pop from an empty list.");
    return l.pop();
  });
  def('remove', 2, 2, (c, k) => {
    if (Array.isArray(c)) {
      if (typeof k !== 'number' || !Number.isInteger(k)) I.fail('remove() needs a whole-number position for a list.');
      const i = k < 0 ? k + c.length : k;
      if (i < 0 || i >= c.length) I.fail(`Position ${k} is outside this list (it has ${c.length} item${c.length === 1 ? '' : 's'}).`);
      return c.splice(i, 1)[0];
    }
    if (c instanceof Map) { const v = c.get(k); c.delete(k); return v === undefined ? null : v; }
    return I.fail('remove() needs a list or a map.');
  });
  def('slice', 2, 3, (x, a, b) => {
    if (typeof x !== 'string' && !Array.isArray(x) && !(x instanceof KRange)) I.fail('slice() needs text or a list.');
    const arr = x instanceof KRange ? x.toArray() : x;
    const from = toIndex(a, arr.length, 'slice');
    const to = b === undefined ? arr.length : toIndex(b, arr.length, 'slice');
    return arr.slice(from, Math.max(from, to));
  });
  def('reverse', 1, 1, x => {
    if (typeof x === 'string') return x.split('').reverse().join('');
    return items(x, 'reverse').slice().reverse();
  });
  def('sort', 1, 2, (l, key) => {
    const a = items(l, 'sort').slice();
    if (key === undefined) return a.sort(compare);
    needFn(key, 'sort');
    const keyed = a.map((v, i) => ({ v, k: call(key, [v]) }));
    keyed.sort((x, y) => compare(x.k, y.k));
    return keyed.map(o => o.v);
  });
  def('unique', 1, 1, l => {
    const out = [];
    for (const v of items(l, 'unique')) if (!out.some(o => eq(o, v))) out.push(v);
    return out;
  });
  def('keys', 1, 1, m => { if (!(m instanceof Map)) I.fail('keys() needs a map.'); return [...m.keys()]; });
  def('values', 1, 1, m => { if (!(m instanceof Map)) I.fail('values() needs a map.'); return [...m.values()]; });
  def('entries', 1, 1, m => { if (!(m instanceof Map)) I.fail('entries() needs a map.'); return [...m].map(([k, v]) => [k, v]); });
  def('contains', 2, 2, (c, x) => {
    if (typeof c === 'string') return c.includes(needText(x, 'contains', 'text to look for'));
    if (Array.isArray(c)) return c.some(v => eq(v, x));
    if (c instanceof Map) return c.has(x);
    if (c instanceof KRange) return c.contains(x);
    return I.fail('contains() needs text, a list or a map.');
  });
  def('find', 2, 2, (c, x) => {
    if (typeof c === 'string') return c.indexOf(needText(x, 'find', 'text to look for'));
    if (Array.isArray(c)) { for (let i = 0; i < c.length; i++) if (eq(c[i], x)) return i; return -1; }
    return I.fail('find() needs text or a list.');
  });

  /* higher-order */
  def('map', 2, 2, (l, f) => {
    needFn(f, 'map');
    const a = items(l, 'map'), out = new Array(a.length);
    for (let i = 0; i < a.length; i++) out[i] = callItem(f, a[i], i);
    return out;
  });
  def('filter', 2, 2, (l, f) => {
    needFn(f, 'filter');
    const a = items(l, 'filter'), out = [];
    for (let i = 0; i < a.length; i++) if (truthy(callItem(f, a[i], i))) out.push(a[i]);
    return out;
  });
  def('reduce', 2, 3, (l, f, start) => {
    needFn(f, 'reduce');
    const a = items(l, 'reduce');
    let i = 0, acc;
    if (start === undefined) {
      if (a.length === 0) I.fail('reduce() on an empty list needs a starting value: reduce(list, fn, start).');
      acc = a[0]; i = 1;
    } else acc = start;
    for (; i < a.length; i++) acc = call(f, [acc, a[i]]);
    return acc;
  });
  def('any', 2, 2, (l, f) => { needFn(f, 'any'); return items(l, 'any').some(v => truthy(call(f, [v]))); });
  def('all', 2, 2, (l, f) => { needFn(f, 'all'); return items(l, 'all').every(v => truthy(call(f, [v]))); });

  /* text */
  def('upper', 1, 1, s => needText(s, 'upper').toUpperCase());
  def('lower', 1, 1, s => needText(s, 'lower').toLowerCase());
  def('trim', 1, 1, s => needText(s, 'trim').trim());
  def('split', 1, 2, (s, sep) => {
    needText(s, 'split');
    if (sep === undefined) { const t = s.trim(); return t === '' ? [] : t.split(/\s+/); }
    needText(sep, 'split', 'text to split on');
    return s.split(sep);
  });
  def('join', 1, 2, (l, sep) => {
    const a = items(l, 'join');
    return a.map(v => I.str(v)).join(sep === undefined ? '' : needText(sep, 'join', 'text to put between items'));
  });
  def('replace', 3, 3, (s, a, b) => {
    needText(s, 'replace'); needText(a, 'replace', 'text to look for'); needText(b, 'replace', 'replacement text');
    return a === '' ? s : s.split(a).join(b);
  });
  def('startsWith', 2, 2, (s, p) => needText(s, 'startsWith').startsWith(needText(p, 'startsWith')));
  def('endsWith', 2, 2, (s, p) => needText(s, 'endsWith').endsWith(needText(p, 'endsWith')));
  def('repeat', 2, 2, (s, n) => I.repeatText(needText(s, 'repeat'), needNumber(n, 'repeat'), I.curNode));

  /* misc */
  def('assert', 1, 2, (c, msg) => {
    if (!truthy(c)) I.fail(msg === undefined ? 'Assertion failed.' : I.str(msg));
    return null;
  });
}

/* ------------------------------------------------------------------ *
 *  Public API
 * ------------------------------------------------------------------ */
function formatError(err, source) {
  if (!(err instanceof KestralError)) return 'Internal error: ' + (err && err.message ? err.message : err);
  const label = err.phase === 'syntax' ? 'Syntax error' : 'Error';
  let s = err.line ? `${label} on line ${err.line}: ${err.message}` : `${label}: ${err.message}`;
  if (source && err.line) {
    const lines = source.split('\n');
    const text = lines[err.line - 1];
    if (text !== undefined) {
      const num = String(err.line);
      s += `\n  ${num} | ${text}`;
      if (err.col > 0) s += `\n  ${' '.repeat(num.length)} | ${' '.repeat(Math.max(0, err.col - 1))}^`;
    }
  }
  return s;
}

/* Run a program once and collect everything. options: { input: [lines], echoPrompts, maxSteps, random } */
function runProgram(source, options) {
  options = options || {};
  const lines = (options.input || []).slice();
  const out = [];
  const interp = new Interp({
    print: s => out.push(s),
    ask: p => {
      const v = lines.length ? lines.shift() : null;
      if (options.echoPrompts && v !== null) out.push(p + v);
      else if (options.echoPrompts) out.push(p);
      return v;
    },
    maxSteps: options.maxSteps,
    maxOutput: options.maxOutput,
    random: options.random,
    readFile: options.readFile
  });
  let error = null;
  try { interp.run(source); }
  catch (e) { error = e; }
  return {
    ok: !error,
    output: out,
    text: out.join('\n'),
    error,
    errorText: error ? formatError(error, source) : ''
  };
}

/* Check a learner's code against one lesson test.
   test = { input: [lines], append: "code run after theirs", expect: "output" }
   With `append`, only the output produced by the appended code is compared, so the learner's
   own practice prints never cause a failure. */
function runTest(code, test) {
  const input = test.input || [];
  const base = (test.prefix ? test.prefix + '\n' : '') + code;
  const first = runProgram(base, { input, maxSteps: 2e6 });
  if (!first.ok) return { ok: false, got: first.errorText, crashed: true, expect: test.expect };
  if (!test.append) return { ok: first.text === test.expect, got: first.text, expect: test.expect };
  const full = runProgram(base + '\n' + test.append, { input, maxSteps: 2e6 });
  if (!full.ok) return { ok: false, got: full.errorText, crashed: true, expect: test.expect };
  const tail = full.output.slice(first.output.length).join('\n');
  return { ok: tail === test.expect, got: tail, expect: test.expect };
}

function builtinNames() {
  return new Interp({}).global.names().sort();
}

return {
  version: VERSION,
  KestralError,
  Interp,
  create: opts => new Interp(opts),
  parse,
  run: runProgram,
  runTest,
  format: v => format(v, true),
  formatError,
  keywords: [...KEYWORDS],
  builtinNames,
  typeName
};
});
