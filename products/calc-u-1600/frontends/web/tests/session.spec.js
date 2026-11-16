const { test, expect } = require('@playwright/test');

// The Calc-U-1600 Web frontend on the library's shell and app session
// (M-31): LIB-15 AC-24 to AC-27, LIB-3 AC-36, LIB-17 AC-22 (the last only
// with CALCU1600_WEB_PACING=1: a minute of wall time, Checkpoint).

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

async function gotoReady(page, query = '') {
  await page.goto(`./${query}`);
  await expect(page.locator('.calcu-client[data-ready=true]')).toHaveCount(1, { timeout: 30000 });
}

const request = (page, method, ...args) =>
  page.evaluate(([m, a]) => document.querySelector('.calcu-client').calcuEngine.request(m, ...a), [method, args]);
const engineState = (page) => page.evaluate(() => {
  const s = document.querySelector('.calcu-client').calcuEngine.getState();
  return { busy: s.busy, ui: s.ui, media: s.media, log: s.log };
});
// ENTER after the typed line, once the paste is through.
async function typedEnter(page) {
  await page.waitForTimeout(1500);
  await page.locator('.calcu-client').focus();
  await page.keyboard.press('Enter');
}
const notBusy = (page) => expect.poll(async () => (await engineState(page)).busy, { timeout: 30000 }).toBe(false);

test('ac24 CalcU1600 takes the AC-16 props, the model by its name', async ({ page }) => {
  await gotoReady(page, '?model=PC-1500');
  await expect.poll(async () => ((await engineState(page)).ui || {}).model).toBe(0);
  await expect(page.locator('.calcu-face[data-model="0"]')).toHaveCount(1);
});

test('ac25 settings in Calc-U-1600.ini, media in the save folder of 0.8.x', async ({ page }) => {
  await gotoReady(page, '?model=PC-1600');
  await request(page, 'setStartupModel', 'PC-1600');
  await notBusy(page);
  const send = (c, a) => page.evaluate(([cc, aa]) => document.querySelector('.calcu-client').calcuEngine.send(cc, aa), [c, a]);
  expect((await send('media.choose', { place: 'slot-1', entry: 'bundled:ce1600m.card.yaml' })).done).toBe(true);   // LIB-7 (M-47)
  await notBusy(page);
  expect((await send('media.name-and-save', { place: 'slot-1', name: 'Kept' })).done).toBe(true);
  // The persistent file system is IndexedDB "/persist", as 0.8.x's.
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
    '/persist/Calc-U-1600/Kept.card.yaml',
  ]));
  await gotoReady(page);
  await expect.poll(async () => ((await engineState(page)).media || { kinds: [] }).kinds
    .flatMap((k) => k.instances).map((i) => i.fileName), { timeout: 10000 }).toContain('Kept.card.yaml');
  expect((await request(page, 'settingsState')).startupModel).toBe('PC-1600');
});

test('ac26 buzzer samples at regular speed, none at full speed or in an operation', async ({ page }) => {
  await recordMessages(page);
  await gotoReady(page, '?model=PC-1600');
  const sounds = () => page.evaluate(() => window.__calcuReceived.filter((m) => m && m.kind === 'sound').length);
  await page.evaluate(() => document.querySelector('.calcu-client').calcuEngine.call('pasteText', 'BEEP 3'));
  await typedEnter(page);

  await expect.poll(sounds, { timeout: 20000 }).toBeGreaterThan(0);
  // Full speed: nothing.
  await request(page, 'setTurbo', true);
  await page.waitForTimeout(300);
  await page.evaluate(() => document.querySelector('.calcu-client').calcuEngine.call('pasteText', 'BEEP 3'));
  await typedEnter(page);
  await page.waitForTimeout(300);
  const before = await sounds();
  await page.waitForTimeout(1500);
  expect(await sounds()).toBe(before);
  await request(page, 'setTurbo', false);
  // An operation: nothing while it runs.
  await page.evaluate(() => { window.__calcuReceived.length = 0; });
  const r = request(page, 'reset', true);
  await r;
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

// LIB-6 AC-49: a preset's log lines are in the engine's log stream.
test('lib6_ac49 preset log in the log stream', async ({ page }) => {
  await gotoReady(page);
  await request(page, 'loadPreset', 'examples:basic/hanoi.pc1600');
  await expect.poll(async () => (await engineState(page)).log.join('\n'), { timeout: 45000 })
    .toContain('Preset examples:basic/hanoi.pc1600 carried out to its end.');
});

// A preset handed over as a file the page uploaded (LIB-6 AC-54: the files
// it names go beside it).
async function uploadPreset(page, name, text) {
  return page.evaluate(([n, t]) => document.querySelector('.calcu-client').calcuEngine
    .request('upload', n, new TextEncoder().encode(t)), [name, text]);
}

// LIB-6 AC-54: an uploaded preset is loaded; its result shows.
test('lib6_ac54 uploaded preset is loaded', async ({ page }) => {
  await gotoReady(page);
  await notBusy(page);
  const path = await uploadPreset(page, 'mine.pc1600',
    'format-version: 2\nmodel: PC-1600\nkeys:\n  - type: PRINT 6*7\n  - expect: 42\n');
  const r = await request(page, 'loadPreset', path);
  expect(r.reason || '').toBe('');
  expect(r.done).toBe(true);
  await notBusy(page);
});

// LIB-10 AC-40 (EC-8; the format 2 clause, M-46): `trace:` fails in the Web frontend.
test('lib10_ac40 trace step fails on the web', async ({ page }) => {
  await gotoReady(page);
  await notBusy(page);
  const path = await uploadPreset(page, 'web.pc1600',
    'format-version: 2\nmodel: PC-1600\nkeys:\n  - trace: t.bin\n  - wait: 1s\n');
  const r = await request(page, 'loadPreset', path);
  expect(r.done).toBe(false);
  expect(r.reason).toMatch(/line 4: .*trace/i);
});

test('ac27 no message of the old protocol', async ({ page }) => {
  await recordMessages(page);
  await gotoReady(page);
  await request(page, 'presetExamples');
  await request(page, 'lcdText');
  const { sent, received } = await page.evaluate(() => ({
    sent: window.__calcuSent.map((m) => m && m.kind),
    received: window.__calcuReceived.map((m) => m && m.kind),
  }));
  for (const k of sent) expect(['listen', 'command', 'request', 'answer']).toContain(k);
  for (const k of received) expect(['state', 'stream', 'reply', 'ask', 'sound-start', 'sound', 'sound-stop']).toContain(k);
  expect(sent).toContain('listen');
});

test('lib3_ac36 hidden tab suspends, shown continues', async ({ page }) => {
  await recordMessages(page);
  await gotoReady(page);
  const seconds = async () => (await request(page, 'ticks')).seconds;
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

test('lib17_ac22 a PC-1600 keeps one minute', async ({ page }) => {
  test.skip(!process.env.CALCU1600_WEB_PACING, 'Checkpoint: set CALCU1600_WEB_PACING=1');
  test.setTimeout(150000);
  await gotoReady(page, '?model=PC-1600');
  await notBusy(page);
  const t0 = await request(page, 'ticks');
  const w0 = Date.now();
  await page.waitForTimeout(60000);
  const t1 = await request(page, 'ticks');
  const wall = (Date.now() - w0) / 1000;
  const emulated = t1.seconds - t0.seconds;
  console.log(`  pacing (Web): wall ${wall.toFixed(3)} s, emulated ${emulated.toFixed(3)} s`);
  expect(Math.abs(emulated - wall)).toBeLessThan(0.05);   // the bound of AC-1 plus the page's round trip
});
