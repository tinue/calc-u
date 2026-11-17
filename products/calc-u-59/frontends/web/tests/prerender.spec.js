const { test, expect } = require('@playwright/test');
const path = require('path');
const { prerender } = require('../../../../../library/web/tests/dist-helpers');

// The website prerenders PlayCalculator server-side, in a sandbox whose
// fetch() and Worker throw: rendering must have no side effects — those
// belong in effects, which a static render never runs (reference/WebClient.md).
// This renders dist/calc.js the same way, in Node; no browser involved.

test('PlayCalculator renders server-side without fetch or Worker', async ({}, testInfo) => {
  test.skip(testInfo.project.name !== 'root', 'independent of the base URL');
  const ctx = prerender(path.join(__dirname, '..', 'dist', 'calc.js'));

  expect(Object.keys(ctx).filter((k) => /^(createEngine|useEngine|CalculatorFace|ControlBar|PaperPane|Debugger|paintLed)$/.test(k)),
         'calc.js leaks its internals into the global scope').toEqual([]);

  for (const props of [{ layout: 'wide', keyboard: true }, { layout: 'portrait' }]) {
    const html = ctx.ReactDOMServer.renderToStaticMarkup(ctx.React.createElement(ctx.PlayCalculator, props));
    expect(html).toContain('calcu-device');
    expect(html).toContain(`data-layout="${props.layout}"`);
    expect(html).toContain('calcu-keyboard');
  }
});
