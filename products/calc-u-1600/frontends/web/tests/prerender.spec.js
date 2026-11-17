const { test, expect } = require('@playwright/test');
const path = require('path');
const { prerender } = require('../../../../../library/web/tests/dist-helpers');

// A host site may prerender CalcU1600 server-side, in a sandbox whose
// fetch() and Worker throw: rendering must have no side effects -- those
// belong in effects, which a static render never runs. This renders
// dist/calc.js the same way, in Node.

test('CalcU1600 renders server-side without fetch or Worker', async () => {
  const ctx = prerender(path.join(__dirname, '..', 'dist', 'calc.js'));

  expect(Object.keys(ctx).filter((k) => /^(Faceplate|Inspector|paintLcd|DiskControls|TapeControls|useDialogs)$/.test(k)),
         'calc.js leaks its internals into the global scope').toEqual([]);
  const html = ctx.ReactDOMServer.renderToStaticMarkup(ctx.React.createElement(ctx.CalcU1600, { width: 1000, height: 700 }));
  expect(html).toContain('calcu-client');
  expect(html).toContain('calcu-shell-menubar');
  expect(html).toContain('calcu-face');
});
