const { test, expect } = require('@playwright/test');
const path = require('path');
const { DIST, send, request, files, serveDist, stop } = require('./helpers');

// The installed app page, in Chromium and in
// WebKit (an iPad's installed app). Each test brings its own server of one
// dist/ below /calc/, no-store, so that only the service worker can answer
// once it is gone.

const running = (page) =>
  expect(page.locator('.calcu-client')).toHaveAttribute('data-start', 'running', { timeout: 30000 });

async function installed(page, server) {
  const appUrl = `http://localhost:${server.address().port}/calc/app/`;
  await page.goto(appUrl);
  await running(page);
  await page.evaluate(() => navigator.serviceWorker.ready);
  await expect.poll(() => page.evaluate(() => !!navigator.serviceWorker.controller)).toBe(true);
  return appUrl;
}

test('installed app starts and runs with the server gone', async ({ page, context }) => {
  test.setTimeout(120000);
  const server = await serveDist(path.join(DIST, 'Test-A'));
  const appUrl = await installed(page, server);
  await page.close();
  await stop(server);

  const app = await context.newPage();
  await app.goto(appUrl);
  await running(app);
  const client = app.locator('.calcu-client');
  for (const model of ['test-dot', 'test-seg']) {   // every model
    expect((await send(app, 'start', { model })).done).toBe(true);
    await expect(client).toHaveAttribute('data-model', model);
    const before = (await request(app, 'tp.calls')).value.calls;
    await app.waitForTimeout(300);
    expect((await request(app, 'tp.calls')).value.calls).toBeGreaterThan(before);
  }
  // A bundled preset and a bundled file, from the cache.
  expect((await send(app, 'start', { preset: 'demo/seg.test' })).done).toBe(true);
  await expect(client).toHaveAttribute('data-preset', 'demo/seg.test');
  expect((await request(app, 'tp.bundle', { name: 'demo.txt' })).value.text).toBe('a bundled file\n');
  await expect(app.locator('.calcu-test-failed')).toHaveCount(0);
});

test('the new release from the next start, all files, old cache gone, store untouched', async ({ page, context }) => {
  test.setTimeout(120000);
  const server = await serveDist(path.join(DIST, 'Test-A'));
  try {
    const appUrl = await installed(page, server);
    await send(page, 'tp.write', { name: 'kept.txt', text: 'from 1.0.0' });
    await page.waitForTimeout(1000);
    const oldCaches = await page.evaluate(() => caches.keys());
    expect(oldCaches.length).toBe(1);

    // A newer release is served; the running app stays on its own.
    server.dir = path.join(DIST, 'Test-A-2');
    await page.evaluate(() => navigator.serviceWorker.getRegistration().then((r) => r.update()));
    await expect.poll(() => page.evaluate(() => navigator.serviceWorker.getRegistration().then((r) => !!r.waiting)),
      { timeout: 30000 }).toBe(true);
    await page.reload();
    await running(page);
    await expect(page.locator('.calcu-test-title')).toHaveText('Test-A 1.0.0');
    expect((await request(page, 'tp.bundle', { name: 'version.txt' })).value.text).toBe('1.0.0');
    await page.close();

    // The next start: every file of the new release, the old cache gone.
    const next = await context.newPage();
    await next.goto(appUrl);
    await running(next);
    await expect(next.locator('.calcu-test-title')).toHaveText('Test-A 2.0.0');
    expect((await request(next, 'tp.bundle', { name: 'version.txt' })).value.text).toBe('2.0.0');
    const caches = await next.evaluate(() => caches.keys());
    expect(caches.length).toBe(1);
    expect(caches[0]).not.toBe(oldCaches[0]);
    expect(await files(next)).toEqual(['kept.txt']);
    expect((await request(next, 'tp.read', { name: 'kept.txt' })).value.text).toBe('from 1.0.0');
  } finally {
    await stop(server);
  }
});

test('install button, iOS hint', async ({ page, browser, browserName }) => {
  const server = await serveDist(path.join(DIST, 'Test-A'));
  try {
    const appUrl = `http://localhost:${server.address().port}/calc/app/`;
    await page.goto(appUrl);
    await running(page);
    if (browserName === 'chromium') {
      await page.evaluate(() => {
        const e = new Event('beforeinstallprompt', { cancelable: true });
        e.prompt = () => { window.__prompted = true; };
        e.userChoice = Promise.resolve({ outcome: 'accepted' });
        window.dispatchEvent(e);
      });
      await expect(page.locator('.install-banner-text')).toHaveText('Install Test-A to run it offline in its own window.');
      await page.locator('.install-banner-cta').click();
      expect(await page.evaluate(() => window.__prompted)).toBe(true);
      await expect(page.locator('.install-banner')).toHaveCount(0);
    } else {
      // No install banner without the browser's offer; an iPad gets the hint.
      await expect(page.locator('.install-banner')).toHaveCount(0);
      const ipad = await browser.newContext({
        userAgent: 'Mozilla/5.0 (iPad; CPU OS 18_0 like Mac OS X) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/18.0 Mobile/15E148 Safari/604.1',
      });
      const p = await ipad.newPage();
      await p.goto(appUrl);
      await expect(p.locator('.install-banner-text')).toContainText('Add to Home Screen');
      await ipad.close();
    }
  } finally {
    await stop(server);
  }
});
