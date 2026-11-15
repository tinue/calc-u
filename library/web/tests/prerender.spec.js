const { test, expect } = require('@playwright/test');
const fs = require('fs');
const path = require('path');
const vm = require('vm');
const { DIST } = require('./helpers');

// LIB-15 T15 (design §6; both products' prerender.spec.js today): a host
// site may render the component on a server, in a sandbox whose fetch()
// and Worker throw. Rendering has no side effects -- those belong in
// effects, which a static render never runs.

const VENDOR = path.join(__dirname, '..', '..', '..', 'products', 'calc-u-1600', 'frontends', 'web', 'vendor');

function sandbox() {
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
  return vm.createContext(ctx);
}

test('ac18 renders on a server without fetch or worker', async () => {
  const ctx = sandbox();
  const run = (file) => vm.runInContext(fs.readFileSync(file, 'utf8'), ctx, { filename: file });
  run(path.join(VENDOR, 'react.production.min.js'));
  run(path.join(VENDOR, 'react-dom-server-legacy.browser.production.min.js'));
  run(path.join(DIST, 'Test-A', 'calc.js'));

  expect(Object.keys(ctx).filter((k) => /^(createShell|embedProps|useShell|useTopic|CALC_BASE|CALCU_SENTENCES)$/.test(k)),
    'calc.js leaks its internals into the global scope').toEqual([]);
  const html = ctx.ReactDOMServer.renderToStaticMarkup(
    ctx.React.createElement(ctx.CalcUTest, { width: 400, height: 300, model: 'test-dot' }));
  expect(html).toContain('calcu-client');
  expect(html).toContain('data-start="starting"');
  expect(html).toContain('Starting Test-A…');
});
