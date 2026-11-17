// What every Web frontend's Playwright tests share about a built dist/: a
// server of its own for the offline tests, and the server-side render of
// its calc.js (LIB-15 AC-18). Node only, no @playwright/test: a product's
// tests require this file and run on their own copy of Playwright.
const fs = require('fs');
const http = require('http');
const path = require('path');
const vm = require('vm');

const VENDOR = path.join(__dirname, '..', 'vendor');

const TYPES = {
  '.html': 'text/html', '.js': 'text/javascript', '.css': 'text/css', '.json': 'application/json',
  '.wasm': 'application/wasm', '.png': 'image/png', '.woff2': 'font/woff2', '.txt': 'text/plain',
};

// A server of one dist/ below `prefix`, no-store (with setOffline() the HTTP
// cache would still answer what the service worker doesn't, and hide a file
// it fails to serve). server.dir can be changed while it runs (AC-11).
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

// A host site may render the component on a server, in a sandbox whose
// fetch() and Worker throw: rendering must have no side effects -- those
// belong in effects, which a static render never runs. The sandbox with
// React, ReactDOMServer and the dist/'s calc.js run in it.
function prerender(calcJs) {
  const noop = () => {};
  const ctx = {
    console, URL, Math, Date, JSON, Uint8Array, TextEncoder, TextDecoder, setTimeout, clearTimeout, queueMicrotask,
    location: { href: 'https://example.org/somewhere/', pathname: '/somewhere/' },
    navigator: { userAgent: 'prerender', platform: 'prerender' },
    fetch: () => { throw new Error('fetch() during render'); },
    Worker: function () { throw new Error('new Worker() during render'); },
    document: {
      currentScript: null,
      addEventListener: noop, removeEventListener: noop,
      createElement: () => ({ style: {} }), getElementById: () => null,
    },
  };
  ctx.window = ctx;
  ctx.self = ctx;
  vm.createContext(ctx);
  for (const file of [path.join(VENDOR, 'react.production.min.js'),
    path.join(VENDOR, 'react-dom-server-legacy.browser.production.min.js'), calcJs])
    vm.runInContext(fs.readFileSync(file, 'utf8'), ctx, { filename: file });
  return ctx;
}

module.exports = { serveDist, stop, prerender };
