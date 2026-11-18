const { test, expect, chromium } = require('@playwright/test');
const { SENTENCES, open, send, request, files } = require('./helpers');

// The persistent file system in IndexedDB, one
// store per product, one writer, and the storage modes. Every test has a
// browser context of its own, so a store of its own.

const settingsPath = '/persist-test-a/settings/Test-A/Test-A.ini';

test('settings and save folder survive close and open', async ({ page, context }) => {
  await open(page);
  expect((await send(page, 'tp.write', { name: 'card.txt', text: 'hello' })).done).toBe(true);
  expect((await send(page, 'tp.set-model', { model: 'test-dot' })).done).toBe(true);
  await page.waitForTimeout(1000);
  await page.close();

  const again = await context.newPage();
  await open(again);
  expect(await files(again)).toContain('card.txt');
  expect((await request(again, 'tp.read', { name: 'card.txt' })).value.text).toBe('hello');
  const settings = (await request(again, 'tp.settings')).value;
  expect(settings.path).toBe(settingsPath);
  expect(settings.model).toBe('test-dot');
  await expect(again.locator('.calcu-client')).toHaveAttribute('data-model', 'test-dot');
  await expect(again.locator('.calcu-client')).toHaveAttribute('data-storage', 'kept');
});

test('write rename delete found after one second', async ({ page }) => {
  await open(page);
  await send(page, 'tp.write', { name: 'a.txt', text: 'a' });
  await send(page, 'tp.write', { name: 'b.txt', text: 'b' });
  await page.waitForTimeout(1000);
  await page.reload();
  await open(page);
  expect(await files(page)).toEqual(['a.txt', 'b.txt']);

  expect((await send(page, 'tp.rename', { from: 'b.txt', to: 'c.txt' })).done).toBe(true);
  expect((await send(page, 'tp.delete', { name: 'a.txt' })).done).toBe(true);
  expect((await send(page, 'tp.write', { name: 'c.txt', text: 'changed' })).done).toBe(true);
  await page.waitForTimeout(1000);
  await page.reload();
  await open(page);
  expect(await files(page)).toEqual(['c.txt']);
  expect((await request(page, 'tp.read', { name: 'c.txt' })).value.text).toBe('changed');
});

test('Test-A and Test-B on one origin see only their own', async ({ page, context }) => {
  await open(page, 'Test-A');
  await send(page, 'tp.write', { name: 'only-a.txt', text: 'a' });
  await page.waitForTimeout(1000);

  const b = await context.newPage();
  await open(b, 'Test-B');
  await expect(b.locator('.calcu-client')).toHaveAttribute('data-storage', 'kept');
  expect(await files(b)).toEqual([]);
  await send(b, 'tp.write', { name: 'only-b.txt', text: 'b' });
  await b.waitForTimeout(1000);
  await b.close();

  await page.reload();
  await open(page, 'Test-A');
  expect(await files(page)).toEqual(['only-a.txt']);
  const b2 = await context.newPage();
  await open(b2, 'Test-B');
  expect(await files(b2)).toEqual(['only-b.txt']);
});

test('second tab works in memory and says so', async ({ page, context }) => {
  await open(page);
  await expect(page.locator('.calcu-client')).toHaveAttribute('data-storage', 'kept');
  await send(page, 'tp.write', { name: 'first.txt', text: '1' });
  await page.waitForTimeout(1000);

  const second = await context.newPage();
  await open(second);
  const client = second.locator('.calcu-client');
  await expect(client).toHaveAttribute('data-storage', 'second-copy');
  await expect(second.locator('.calcu-test-banner')).toHaveText(SENTENCES.secondCopy('Test-A'));
  await expect(second.locator('.calcu-test-notices')).toContainText(SENTENCES.secondCopy('Test-A'));
  expect(await files(second)).toEqual(['first.txt']);   // read once
  await send(second, 'tp.write', { name: 'second.txt', text: '2' });
  expect(await files(second)).toEqual(['first.txt', 'second.txt']);   // in memory

  // It stays so until it is loaded again, also when the first is gone.
  await page.close();
  await second.waitForTimeout(500);
  await expect(client).toHaveAttribute('data-storage', 'second-copy');
  await second.waitForTimeout(500);
  await second.close();

  const third = await context.newPage();
  await open(third);
  await expect(third.locator('.calcu-client')).toHaveAttribute('data-storage', 'kept');
  expect(await files(third)).toEqual(['first.txt']);
});

test('twice on one page', async ({ page }) => {
  await page.goto('Test-A/index.html?twice=1');
  const clients = page.locator('.calcu-client');
  await expect(clients).toHaveCount(2);
  await expect(clients.nth(0)).toHaveAttribute('data-start', 'running', { timeout: 30000 });
  await expect(clients.nth(1)).toHaveAttribute('data-start', 'running', { timeout: 30000 });
  await expect(page.locator('.calcu-client[data-storage=kept]')).toHaveCount(1);
  await expect(page.locator('.calcu-client[data-storage=second-copy]')).toHaveCount(1);
  await expect(page.locator('.calcu-client[data-storage=second-copy] .calcu-test-banner'))
    .toHaveText(SENTENCES.secondCopy('Test-A'));
});

test('plain http every copy writes', async ({ baseURL }, testInfo) => {
  test.skip(testInfo.project.name !== 'root', 'Chromium at the root only: a host name mapped to the test server');
  const port = new URL(baseURL).port;
  const browser = await chromium.launch({ args: ['--host-resolver-rules=MAP calcu.test 127.0.0.1'] });
  try {
    const context = await browser.newContext();
    const url = `http://calcu.test:${port}/Test-A/index.html`;
    const one = await context.newPage();
    await one.goto(url);
    await expect(one.locator('.calcu-client')).toHaveAttribute('data-start', 'running', { timeout: 30000 });
    expect(await one.evaluate(() => window.isSecureContext)).toBe(false);
    const two = await context.newPage();
    await two.goto(url);
    await expect(two.locator('.calcu-client')).toHaveAttribute('data-start', 'running', { timeout: 30000 });
    await expect(one.locator('.calcu-client')).toHaveAttribute('data-storage', 'kept');
    await expect(two.locator('.calcu-client')).toHaveAttribute('data-storage', 'kept');
    await expect(two.locator('.calcu-test-banner')).toHaveCount(0);
    // The app page cannot be installed: no service worker over plain HTTP.
    await one.goto(`http://calcu.test:${port}/Test-A/app/`);
    expect(await one.evaluate(() => 'serviceWorker' in navigator)).toBe(false);
  } finally {
    await browser.close();
  }
});

test('unreadable store is not written over', async ({ page, context }) => {
  await open(page);
  await send(page, 'tp.write', { name: 'keep.txt', text: 'kept' });
  await page.waitForTimeout(1000);
  await page.close();

  const broken = await context.newPage();
  await open(broken, 'Test-A', 'worker=tests/unreadable-worker.js');
  await expect(broken.locator('.calcu-client')).toHaveAttribute('data-storage', 'unreadable');
  await expect(broken.locator('.calcu-test-banner')).toHaveText(SENTENCES.unreadable('Test-A'));
  await send(broken, 'tp.write', { name: 'new.txt', text: 'new' });
  await send(broken, 'tp.set-model', { model: 'test-dot' });
  expect(await files(broken)).toContain('new.txt');
  await broken.waitForTimeout(1000);
  await broken.close();

  const again = await context.newPage();
  await open(again);
  expect(await files(again)).toEqual(['keep.txt']);
  expect((await request(again, 'tp.settings')).value.model).not.toBe('test-dot');
});

test('no store works in memory and says so', async ({ page }) => {
  await open(page, 'Test-A', 'worker=tests/unavailable-worker.js');
  await expect(page.locator('.calcu-client')).toHaveAttribute('data-storage', 'unavailable');
  await expect(page.locator('.calcu-test-banner')).toHaveText(SENTENCES.unavailable('Test-A'));
  await expect(page.locator('.calcu-test-notices')).toContainText(SENTENCES.unavailable('Test-A'));
  expect((await send(page, 'tp.write', { name: 'mem.txt', text: 'm' })).done).toBe(true);
  expect(await files(page)).toEqual(['mem.txt']);
});
