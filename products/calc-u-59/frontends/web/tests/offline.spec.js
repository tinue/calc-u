const { test, expect } = require('@playwright/test');
const path = require('path');
const { serveDist, stop } = require('../../../../../library/web/tests/dist-helpers');

// The installed app started from nothing without a connection: a new page
// after the server is gone, so the worker, its scripts, the WASM and the ROMs
// can only come from the service worker. This test has its own server: with
// context.setOffline() the browser's HTTP cache still answers what the service
// worker doesn't, and hides a file it fails to serve. no-store keeps that
// cache out of it. Also runs in WebKit, the engine of the iPhone's installed
// app (playwright.config.js).

test.describe.configure({ timeout: 60000 });

const DIST = path.join(__dirname, '..', 'dist');
const PREFIX = '/anything/calc/';
test('the installed app starts without a connection and runs every model', async ({ page, context }) => {
  const server = await serveDist(DIST, PREFIX);
  const appUrl = `http://localhost:${server.address().port}${PREFIX}app/`;

  await page.goto(appUrl);
  await expect(page.locator('.calcu-key')).toHaveCount(45, { timeout: 30000 });
  await page.evaluate(() => navigator.serviceWorker.ready);
  await page.close();
  // The closed page's worker lets go of the save folder's lock a little
  // later (slow WebKit runners: seconds). A page started before that is a
  // second copy (row 59-8) and says so, which is right but not this test.
  // The probe is a file of the origin that starts no calculator: the demo
  // page (PREFIX itself) starts a worker of its own, which takes the lock
  // when the closed page's worker let go of it first, and the wait never
  // ends (a race; Linux WebKit lost it about every other run).
  const probe = await context.newPage();
  await probe.goto(`http://localhost:${server.address().port}${PREFIX}MANIFEST.json`);
  await expect.poll(() => probe.evaluate(async () =>
    navigator.locks ? (await navigator.locks.query()).held.length : 0), { timeout: 30000 }).toBe(0);
  expect(probe.workers()).toEqual([]);
  await probe.close();
  await stop(server);

  const app = await context.newPage();
  await app.goto(appUrl);
  await expect(app.locator('.calcu-key')).toHaveCount(45, { timeout: 30000 });

  // A model or module not started with is read later, by a synchronous request.
  const model = app.locator('[data-calcu="bar-model"]');   // the shell's model picker (M-57)
  for (const name of ['TI-58C', 'TI-58', 'TI-59']) {
    await model.selectOption({ label: name });
    await expect(model).toHaveValue(await model.locator(`option:text-is("${name}")`).getAttribute('value'));
    await app.waitForTimeout(500);
    await expect(app.locator('.calcu-key')).toHaveCount(45);
  }
  // A switch shows "Starting Calc-U-59…" until the new model runs (row 59-38):
  // on a slow runner that outlasts the 500 ms above. Nothing may stay; a failure
  // names the text that did.
  const status = app.locator('[data-calcu="notice"], [role=status]');
  await expect.poll(async () => (await status.count()) ? await status.allTextContents() : [],
    { timeout: 30000 }).toEqual([]);

  // The module picker of the bar (row 59-9, M-57).
  const modules = app.locator('[data-calcu="media-picker-slot-1"]');
  await modules.selectOption({ label: 'Applied Statistics' });
  await app.waitForTimeout(1000);
  // A module whose image can't be read is not selected: the picker would still show ML.
  await expect(modules.locator('option:checked')).toHaveText('Applied Statistics');
});
