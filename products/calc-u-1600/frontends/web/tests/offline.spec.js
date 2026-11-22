const { test, expect } = require('@playwright/test');
const path = require('path');
const { serveDist, stop } = require('../../../../../library/web/tests/dist-helpers');

// The installed app started without a connection: a new page after the
// server is gone, so the worker, its scripts, the WASM, the ROMs and the
// presets can only come from the service worker. Its own server (no-store),
// since with setOffline() the HTTP cache would still answer. Also in
// WebKit, the engine of an iPad's installed app (playwright.config.js).

const DIST = path.join(__dirname, '..', 'dist');
const PREFIX = '/anything/calc/';
test('the installed app starts without a connection and runs every model', async ({ page, context }) => {
  test.setTimeout(120000);
  const server = await serveDist(DIST, PREFIX);
  const appUrl = `http://localhost:${server.address().port}${PREFIX}app/`;

  await page.goto(appUrl);
  await expect(page.locator('.calcu-client[data-ready=true]')).toHaveCount(1, { timeout: 30000 });
  await page.evaluate(() => navigator.serviceWorker.ready);
  await page.close();
  await stop(server);

  const app = await context.newPage();
  await app.goto(appUrl);
  await expect(app.locator('.calcu-client[data-ready=true]')).toHaveCount(1, { timeout: 30000 });
  for (const model of ['PC-1600', 'PC-1500', 'PC-1500A']) {
    await app.locator('[data-calcu=bar-model]').selectOption({ label: model.replace(/^PC-/, '') });
    await expect(app.locator(`.calcu-face[data-model="${model}"]`)).toHaveCount(1, { timeout: 30000 });
  }
  // A bundled preset, with its program file, from the cache (File ▸ Examples ▸).
  const entries = (await app.evaluate(() => document.querySelector('.calcu-client').calcuShell.request('preset-list'))).value.entries;
  const group = entries.find((e) => e.bundlePath === 'examples:basic/hanoi.pc1600').group;
  await app.locator('[data-calcu=menu-file]').click();
  await app.locator('[data-calcu=item-examples]').hover();
  if (group) await app.locator(`[data-calcu="item-examples-${group}"]`).hover();
  await app.locator('[data-calcu="item-example-examples:basic/hanoi.pc1600"]').click();
  await expect(app.locator('.calcu-face[data-model="PC-1600"]')).toHaveCount(1, { timeout: 30000 });
  await expect(app.locator('[data-calcu=notice]')).toHaveCount(0);
});
