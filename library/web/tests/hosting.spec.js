const { test, expect } = require('@playwright/test');
const fs = require('fs');
const { open, send, request } = require('./helpers');

// The session in a Web Worker, its host calls,
// suspend and continue, a start that fails, the host services.

const PAGE_TO_WORKER = ['listen', 'command', 'request', 'answer'];
const WORKER_TO_PAGE = ['state', 'stream', 'reply', 'ask', 'sound-start', 'sound', 'sound-stop'];

// Every message both ways, recorded on the page.
function recordMessages() {
  window.__sent = [];
  window.__received = [];
  const Original = window.Worker;
  window.Worker = class extends Original {
    constructor(...args) {
      super(...args);
      this.addEventListener('message', (e) => window.__received.push(e.data && e.data.kind));
    }
    postMessage(m, t) {
      window.__sent.push(m && m.kind);
      return super.postMessage(m, t);
    }
  };
}

const callsNow = async (page) => (await request(page, 'tp.calls')).value.calls;

function setHidden(page, hidden) {
  return page.evaluate((h) => {
    Object.defineProperty(document, 'hidden', { configurable: true, get: () => h });
    Object.defineProperty(document, 'visibilityState', { configurable: true, get: () => (h ? 'hidden' : 'visible') });
    document.dispatchEvent(new Event('visibilitychange'));
  }, hidden);
}

test('the session runs in a worker and the page gets only session-protocol messages', async ({ page }) => {
  await page.addInitScript(recordMessages);
  await open(page);
  expect(page.workers().length).toBeGreaterThanOrEqual(1);
  expect((await send(page, 'tp.write', { name: 'a.txt', text: 'x' })).done).toBe(true);
  expect((await request(page, 'tp.calls')).done).toBe(true);
  expect((await send(page, 'no-such-command')).done).toBe(false);
  const { sent, received } = await page.evaluate(() => ({ sent: window.__sent, received: window.__received }));
  expect(sent.length).toBeGreaterThan(0);
  expect(received.length).toBeGreaterThan(0);
  expect(sent.filter((k) => !PAGE_TO_WORKER.includes(k))).toEqual([]);
  expect(received.filter((k) => !WORKER_TO_PAGE.includes(k))).toEqual([]);
});

test('about 60 host calls a second', async ({ page }) => {
  await open(page);
  const t0 = Date.now();
  const c0 = await callsNow(page);
  await page.waitForTimeout(2000);
  const c1 = await callsNow(page);
  const rate = (c1 - c0) / ((Date.now() - t0) / 1000);
  expect(rate).toBeGreaterThan(40);
  expect(rate).toBeLessThan(75);
});

test('hidden stops the calls, shown continues them', async ({ page }) => {
  await open(page);
  await setHidden(page, true);
  await page.waitForTimeout(200);
  const c0 = await callsNow(page);
  await page.waitForTimeout(600);
  expect(await callsNow(page)).toBe(c0);
  await setHidden(page, false);
  await page.waitForTimeout(600);
  expect(await callsNow(page)).toBeGreaterThan(c0 + 10);
});

test('a missing bundled file', async ({ page, context }) => {
  await context.route('**/bundle/demo.txt', (route) => route.fulfill({ status: 404, body: '' }));
  await open(page, 'Test-A', '', 'failed');
  const alert = page.locator('.calcu-test-failed');
  await expect(alert).toContainText('Test-A could not start');
  await expect(alert).toContainText('bundle/demo.txt');
  await expect(page.locator('.calcu-test-status')).toHaveCount(0);
});

test('a broken wasm', async ({ page, context }) => {
  await context.route('**/wasm/calcutest-core.wasm',
    (route) => route.fulfill({ status: 200, contentType: 'application/wasm', body: 'not a wasm module' }));
  await open(page, 'Test-A', '', 'failed');
  await expect(page.locator('.calcu-test-failed')).toContainText('calcutest-core.wasm did not start');
});

test('a worker script error', async ({ page }) => {
  await open(page, 'Test-A', 'worker=tests/broken-worker.js', 'failed');
  await expect(page.locator('.calcu-test-failed')).toContainText('Test-A could not start');
  await expect(page.locator('.calcu-test-status')).toHaveCount(0);
});

test('states its services and no folder picker', async ({ page }) => {
  await open(page);
  const reply = await request(page, 'tp.services');
  expect(reply.value.services.sort()).toEqual(
    ['audio-sink', 'clipboard-image', 'clipboard-text', 'file-picker', 'keep-file', 'open-url'].sort());
  expect(reply.value.services).not.toContain('folder-picker');
});

test('a handed file is a download', async ({ page }) => {
  await open(page);
  const [download] = await Promise.all([
    page.waitForEvent('download'),
    page.evaluate(() => window.calcuTestShells[0].send('tp.ask', {
      service: 'keep-file',
      args: { name: 'trace.txt', bytes: new TextEncoder().encode('a trace\n'), mediaType: 'text/plain' },
    })),
  ]);
  expect(download.suggestedFilename()).toBe('trace.txt');
  expect(fs.readFileSync(await download.path(), 'utf8')).toBe('a trace\n');
  await expect(page.locator('.calcu-test-answers')).toContainText('"service":"keep-file","given":true');
});

test('file picker, clipboard text, open url answered once', async ({ page, context, browserName }) => {
  await open(page);
  const answers = () => page.evaluate(() => window.calcuTestShells[0].getState().streams['tp.answers'] || []);

  const [chooser] = await Promise.all([page.waitForEvent('filechooser'), page.locator('.calcu-test-pick').click()]);
  await chooser.setFiles({ name: 'pick.txt', mimeType: 'text/plain', buffer: Buffer.from('picked') });
  await expect.poll(async () => (await answers()).length).toBe(1);
  expect((await answers())[0]).toMatchObject({ service: 'file-picker', given: true, value: ['/uploads/pick.txt'] });

  const [popup] = await Promise.all([page.waitForEvent('popup'), page.locator('.calcu-test-open').click()]);
  expect(popup.url()).toContain('about:blank');
  await expect.poll(async () => (await answers()).length).toBe(2);
  expect((await answers())[1]).toMatchObject({ service: 'open-url', given: true });

  if (browserName === 'chromium') {
    await context.grantPermissions(['clipboard-read', 'clipboard-write']);
    await page.locator('.calcu-test-copy').click();
    await expect.poll(async () => (await answers()).length).toBe(3);
    expect((await answers())[2]).toMatchObject({ service: 'clipboard-text', given: true });
    expect(await page.evaluate(() => navigator.clipboard.readText())).toBe('calcu copied');
  }
  await page.waitForTimeout(300);
  expect((await answers()).length).toBe(browserName === 'chromium' ? 3 : 2);
});
