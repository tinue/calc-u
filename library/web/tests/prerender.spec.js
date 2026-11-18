const { test, expect } = require('@playwright/test');
const path = require('path');
const { DIST } = require('./helpers');
const { prerender } = require('./dist-helpers');

// A host
// site may render the component on a server, in a sandbox whose fetch()
// and Worker throw. Rendering has no side effects -- those belong in
// effects, which a static render never runs (dist-helpers.js prerender).

test('renders on a server without fetch or worker', async () => {
  const ctx = prerender(path.join(DIST, 'Test-A', 'calc.js'));

  expect(Object.keys(ctx).filter((k) => /^(createShell|embedProps|useShell|useTopic|CALC_BASE|CALCU_SENTENCES)$/.test(k)),
    'calc.js leaks its internals into the global scope').toEqual([]);
  const html = ctx.ReactDOMServer.renderToStaticMarkup(
    ctx.React.createElement(ctx.CalcUTest, { width: 400, height: 300, model: 'test-dot' }));
  expect(html).toContain('calcu-client');
  expect(html).toContain('data-start="starting"');
  expect(html).toContain('Starting Test-A…');
});
