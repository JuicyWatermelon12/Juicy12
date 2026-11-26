/* Packs the game into one file with everything inlined: dist/drift-tycoon.html.
   Run: node tools/build.js
   The output has no <html>/<head>/<body> of its own (the page host adds them); it also opens fine as a file. */
const fs = require('fs');
const path = require('path');
const rootDir = path.join(__dirname, '..');
const read = (f) => fs.readFileSync(path.join(rootDir, f), 'utf8');

const index = read('index.html');
const app = index.split('<!-- app:start -->')[1].split('<!-- app:end -->')[0].trim();
const fonts = index.match(/<link rel="stylesheet" href="https:\/\/fonts[^>]+>/)[0];
const three = index.match(/<script src="https:\/\/cdnjs[^>]+><\/script>/)[0];
const scripts = [...index.matchAll(/<script src="(src\/[^"]+)"><\/script>/g)].map((m) => m[1]);

const out = [
  '<title>Drift Tycoon</title>',
  '<meta name="claude:viewport-fit-sides" content="cover">',
  fonts,
  '<style>\n' + read('src/style.css') + '</style>',
  app,
  three,
  ...scripts.map((s) => '<script>\n' + read(s) + '</script>'),
  '',
].join('\n');

fs.mkdirSync(path.join(rootDir, 'dist'), { recursive: true });
fs.writeFileSync(path.join(rootDir, 'dist', 'drift-tycoon.html'), out);
console.log('dist/drift-tycoon.html', (out.length / 1024).toFixed(1) + ' KB');
