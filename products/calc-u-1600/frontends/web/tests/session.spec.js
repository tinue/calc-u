const { test, expect } = require('@playwright/test');

// The Calc-U-1600 Web frontend on the library's shell and app session: the
// component's props, settings and media storage, sound, the protocol, a
// hidden tab, and pacing (the last only with CALCU1600_WEB_PACING=1: a
// minute of wall time, Checkpoint).

// Records what crosses postMessage between the page and the worker.
async function recordMessages(page) {
  await page.addInitScript(() => {
    window.__calcuSent = [];
    window.__calcuReceived = [];
    const Base = window.Worker;
    window.Worker = class extends Base {
      constructor(...args) {
        super(...args);
        this.addEventListener('message', (e) => window.__calcuReceived.push(e.data));
      }
      postMessage(m, t) {
        window.__calcuSent.push(m);
        return super.postMessage(m, t);
      }
    };
  });
}

const h = require('./helpers');
const { gotoReady, notBusy } = h;

const send = h.send;
const log = (page) => page.evaluate(() => (document.querySelector('.calcu-client').calcuShell.getState().streams.log || []).map((l) => l.text));
const ticks = async (page) => (await h.ask(page, 'ticks')).value;
const setting = async (page, section, key) => {
  const rows = ((await h.topic(page, 'settings')) || {}).rows || [];
  return (rows.find((r) => r.section === section && r.key === key) || {}).value;
};
// ENTER after the typed line, once the paste is through.
async function typedEnter(page) {
  await page.waitForTimeout(1500);
  await page.locator('.calcu-client').focus();
  await page.keyboard.press('Enter');
}

test('CalcU1600 takes the shared component props, the model by its name', async ({ page }) => {
  await gotoReady(page, '?model=PC-1500');
  await expect.poll(async () => ((await h.topic(page, 'control-state')) || {}).model).toBe('PC-1500');
  await expect(page.locator('.calcu-face[data-model="PC-1500"]')).toHaveCount(1);
});

// `layout` is a shared prop; Calc-U-1600 has one layout and takes "auto"
// only (its default); another value is ignored with a console warning.
test('layout auto only, another value ignored with a warning', async ({ page }) => {
  const warnings = [];
  page.on('console', (m) => { if (m.type() === 'warning') warnings.push(m.text()); });
  await gotoReady(page);
  await page.evaluate(() => {
    for (const layout of ['auto', 'wide']) {
      const div = document.createElement('div');
      div.id = `layout-${layout}`;
      document.body.appendChild(div);
      ReactDOM.createRoot(div).render(React.createElement(CalcU1600, { layout, width: 900, height: 600 }));
    }
  });
  await expect(page.locator('#layout-wide .calcu-client')).toHaveCount(1);
  await expect(page.locator('#layout-auto .calcu-client')).toHaveCount(1);
  await expect.poll(() => warnings.filter((w) => /layout/.test(w))).toEqual(
    ['CalcU1600: layout "wide" is not one of its layouts ("auto"); it is ignored.']);
});

test('settings in Calc-U-1600.ini, media in the save folder', async ({ page }) => {
  await gotoReady(page, '?model=PC-1600');
  expect((await send(page, 'set-setting', { section: 'startup', key: 'model', value: 'PC-1600' })).done).toBe(true);
  await notBusy(page);
  expect((await send(page, 'media.choose', { place: 'slot-1', entry: 'bundled:ce1600m.module.yaml' })).done).toBe(true);
  await notBusy(page);
  expect((await send(page, 'media.name-and-save', { place: 'slot-1', name: 'Kept' })).done).toBe(true);
  // The persistent file system is IndexedDB "/persist".
  await expect.poll(() => page.evaluate(() => new Promise((resolve) => {
    const open = indexedDB.open('/persist');
    open.onsuccess = () => {
      const db = open.result;
      const keys = db.transaction('FILE_DATA').objectStore('FILE_DATA').getAllKeys();
      keys.onsuccess = () => resolve(keys.result.map(String));
      keys.onerror = () => resolve([]);
    };
    open.onerror = () => resolve([]);
  })), { timeout: 10000 }).toEqual(expect.arrayContaining([
    '/persist/settings/Calc-U-1600/Calc-U-1600.ini',
    '/persist/Calc-U-1600/Kept.module.yaml',
  ]));
  await gotoReady(page);
  await expect.poll(async () => ((await h.topic(page, 'media')) || { kinds: [] }).kinds
    .flatMap((k) => k.instances).map((i) => i.fileName), { timeout: 10000 }).toContain('Kept.module.yaml');
  expect(await setting(page, 'startup', 'model')).toBe('PC-1600');
});

test('buzzer samples at regular speed, none at full speed or in an operation', async ({ page }) => {
  await recordMessages(page);
  await gotoReady(page, '?model=PC-1600');
  const sounds = () => page.evaluate(() => window.__calcuReceived.filter((m) => m && m.kind === 'sound').length);
  await send(page, 'paste', { text: 'BEEP 3' });
  await typedEnter(page);

  await expect.poll(sounds, { timeout: 20000 }).toBeGreaterThan(0);
  // Full speed: nothing.
  await send(page, 'speed', { full: true });
  await page.waitForTimeout(300);
  await send(page, 'paste', { text: 'BEEP 3' });
  await typedEnter(page);
  await page.waitForTimeout(300);
  const before = await sounds();
  await page.waitForTimeout(1500);
  expect(await sounds()).toBe(before);
  await send(page, 'speed', { full: false });
  // An operation: nothing while it runs.
  await page.evaluate(() => { window.__calcuReceived.length = 0; });
  await send(page, 'reset');
  await notBusy(page);
  const during = await page.evaluate(() => {
    let busy = false, n = 0;
    for (const m of window.__calcuReceived) {
      if (m && m.kind === 'state' && m.topic === 'busy') busy = m.value.operationRuns;
      if (m && m.kind === 'sound' && busy) ++n;
    }
    return n;
  });
  expect(during).toBe(0);
});

// A preset's log lines are in the engine's log stream, at the log level info.
test('preset log in the log stream', async ({ page }) => {
  await gotoReady(page);
  await send(page, 'log-level', { level: 'info' });
  await send(page, 'load-preset', { ref: 'examples:basic/hanoi.pc1600' });
  await expect.poll(async () => (await log(page)).join('\n'), { timeout: 45000 })
    .toContain('Preset examples:basic/hanoi.pc1600 carried out to its end.');
});

// A preset handed over as a file the page uploaded (the files it names go
// beside it).
async function uploadPreset(page, name, text) {
  const r = await page.evaluate(([n, t]) => document.querySelector('.calcu-client').calcuShell
    .send('c1600.upload', { name: n, bytes: new TextEncoder().encode(t) }), [name, text]);
  return r.value;
}

// An uploaded preset is loaded; its result shows.
test('uploaded preset is loaded', async ({ page }) => {
  await gotoReady(page);
  await notBusy(page);
  const path = await uploadPreset(page, 'mine.pc1600',
    'format-version: 2\nmodel: PC-1600\nkeys:\n  - type: PRINT 6*7\n  - expect: 42\n');
  const r = await send(page, 'load-preset', { ref: path });
  expect(r.reason || '').toBe('');
  expect(r.done).toBe(true);
  await notBusy(page);
});

// `trace:` fails in the Web frontend.
test('trace step fails on the web', async ({ page }) => {
  await gotoReady(page);
  await notBusy(page);
  const path = await uploadPreset(page, 'web.pc1600',
    'format-version: 2\nmodel: PC-1600\nkeys:\n  - trace: t.bin\n  - wait: 1s\n');
  const r = await send(page, 'load-preset', { ref: path });
  expect(r.done).toBe(false);
  expect(r.reason).toMatch(/line 4: .*trace/i);
});

test('only the messages of the session protocol', async ({ page }) => {
  await recordMessages(page);
  await gotoReady(page);
  await h.ask(page, 'preset-list');
  await h.ask(page, 'c1600.display-text');
  const { sent, received } = await page.evaluate(() => ({
    sent: window.__calcuSent.map((m) => m && m.kind),
    received: window.__calcuReceived.map((m) => m && m.kind),
  }));
  for (const k of sent) expect(['listen', 'shell', 'command', 'request', 'answer']).toContain(k);
  for (const k of received) expect(['state', 'stream', 'reply', 'ask', 'sound-start', 'sound', 'sound-stop']).toContain(k);
  expect(sent).toContain('listen');
});

test('hidden tab suspends, shown continues', async ({ page }) => {
  await recordMessages(page);
  await gotoReady(page);
  const seconds = async () => (await ticks(page)).seconds;
  const setHidden = (hidden) => page.evaluate((h) => {
    Object.defineProperty(document, 'hidden', { configurable: true, get: () => h });
    document.dispatchEvent(new Event('visibilitychange'));
  }, hidden);
  await setHidden(true);
  await page.waitForTimeout(200);
  const t1 = await seconds();
  await page.waitForTimeout(1000);
  const t2 = await seconds();
  expect(t2 - t1).toBeLessThan(0.05);
  await setHidden(false);
  await page.waitForTimeout(1000);
  expect((await seconds()) - t2).toBeGreaterThan(0.5);
  const names = await page.evaluate(() => window.__calcuSent.filter((m) => m && m.kind === 'command').map((m) => m.name));
  expect(names).toEqual(expect.arrayContaining(['suspend', 'continue']));
});

test('a PC-1600 keeps one minute', async ({ page }) => {
  test.skip(!process.env.CALCU1600_WEB_PACING, 'Checkpoint: set CALCU1600_WEB_PACING=1');
  test.setTimeout(150000);
  await gotoReady(page, '?model=PC-1600');
  await notBusy(page);
  const t0 = await ticks(page);
  const w0 = Date.now();
  await page.waitForTimeout(60000);
  const t1 = await ticks(page);
  const wall = (Date.now() - w0) / 1000;
  const emulated = t1.seconds - t0.seconds;
  console.log(`  pacing (Web): wall ${wall.toFixed(3)} s, emulated ${emulated.toFixed(3)} s`);
  expect(Math.abs(emulated - wall)).toBeLessThan(0.05);   // the pacer's bound (one slice) plus the page's round trip
});
