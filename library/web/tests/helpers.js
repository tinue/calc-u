// What the shell's Playwright tests share: opening the test product's host
// page, its shells (window.calcuTestShells, src/CalcUTest.jsx), and a server
// of a dist/ of its own for the offline tests.
const { expect } = require('@playwright/test');
const fs = require('fs');
const http = require('http');
const path = require('path');

const DIST = process.env.CALCU_TEST_DIST ||
  path.join(__dirname, '..', '..', '..', 'build', 'web', 'library', 'web', 'test-dist');

const SENTENCES = {
  secondCopy: (p) => `${p} is open in another tab or window: what you save here is not kept.`,
  unreadable: (p) => `${p} could not read its saved files: they are left as they are, and what you save here is not kept.`,
  unavailable: (p) => `This browser keeps no files for ${p} here: what you save is not kept.`,
};

// The host page of a product, started: `query` is the page's search string.
async function open(page, product = 'Test-A', query = '', state = 'running') {
  await page.goto(`${product}/index.html${query ? `?${query}` : ''}`);
  await expect(page.locator('.calcu-client').first()).toHaveAttribute('data-start', state, { timeout: 30000 });
  if (state === 'running') await expect(page.locator('.calcu-client').first()).not.toHaveAttribute('data-model', '');
}

const send = (page, name, args, i = 0) =>
  page.evaluate(([n, a, k]) => window.calcuTestShells[k].send(n, a), [name, args === undefined ? null : args, i]);
const request = (page, name, args, i = 0) =>
  page.evaluate(([n, a, k]) => window.calcuTestShells[k].request(n, a), [name, args === undefined ? null : args, i]);

async function files(page, i = 0) {
  const reply = await page.evaluate((k) => window.calcuTestShells[k].getState().topics['tp.files'], i);
  return (reply && reply.files) || [];
}

const TYPES = {
  '.html': 'text/html', '.js': 'text/javascript', '.css': 'text/css', '.json': 'application/json',
  '.wasm': 'application/wasm', '.png': 'image/png', '.txt': 'text/plain',
};

// A server of one dist/ below `prefix`, no-store (with setOffline() the HTTP
// cache would still answer). server.dir can be changed while it runs (AC-11).
function serveDist(dir, prefix = '/calc/') {
  const server = http.createServer((req, res) => {
    let url = decodeURIComponent(req.url.split('?')[0]);
    if (url.endsWith('/')) url += 'index.html';
    const file = path.join(server.dir, url.slice(prefix.length));
    if (!url.startsWith(prefix) || !fs.existsSync(file) || fs.statSync(file).isDirectory()) {
      res.writeHead(404);
      res.end();
      return;
    }
    res.writeHead(200, { 'content-type': TYPES[path.extname(file)] || 'application/octet-stream', 'cache-control': 'no-store' });
    fs.createReadStream(file).pipe(res);
  });
  server.dir = dir;
  return new Promise((resolve) => server.listen(0, () => resolve(server)));
}

function stop(server) {
  server.closeAllConnections();
  return new Promise((resolve) => server.close(resolve));
}

module.exports = { DIST, SENTENCES, open, send, request, files, serveDist, stop };
