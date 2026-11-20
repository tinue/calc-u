const { test, expect } = require('@playwright/test');
const path = require('path');
const { prerender } = require('../../../../../library/web/tests/dist-helpers');

// The website prerenders CalcU59 server-side, in a sandbox whose
// fetch() and Worker throw: rendering must have no side effects — those
// belong in effects, which a static render never runs (docs/developer/Web-Client.md).
// This renders dist/calc.js the same way, in Node; no browser involved.

test('CalcU59 renders server-side without fetch or Worker', async ({}, testInfo) => {
  test.skip(testInfo.project.name !== 'root', 'independent of the base URL');
  const ctx = prerender(path.join(__dirname, '..', 'dist', 'calc.js'));

  expect(Object.keys(ctx).filter((k) => /^(createEngine|useEngine|CalculatorFace|ControlBar|PaperPane|Debugger|paintLed|PlayCalculator)$/.test(k)),
         'calc.js leaks its internals into the global scope').toEqual([]);

  for (const props of [{ layout: 'wide', keyboard: true }, { layout: 'portrait' }]) {
    const html = ctx.ReactDOMServer.renderToStaticMarkup(ctx.React.createElement(ctx.CalcU59, props));
    expect(html).toContain('calcu-device');
    expect(html).toContain(`data-layout="${props.layout}"`);
    expect(html).toContain('calcu-keyboard');
  }
});
