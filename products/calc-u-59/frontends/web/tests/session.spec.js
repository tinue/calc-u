const { test, expect } = require('@playwright/test');
const { gotoReady } = require('./helpers');

// The Calc-U-59 Web frontend on the library's shell and app session (M-34):
// LIB-15 AC-19, AC-22, EC-4 for this product; LIB-3 AC-31 (the Web part).

test.describe.configure({ timeout: 30000 });

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

// Since M-57 the page talks to the library's shell itself (src/engine.js
// went): the root element's `calcuShell`.
const request = (page, name, args) =>
  page.evaluate(([n, a]) => document.querySelector('.calcu-client').calcuShell.request(n, a).then((r) => r.value), [name, args || null]);
const el = (page, name) => page.locator(`[data-calcu="${name}"]`);

test('ac19 PlayCalculator props, layout and settings, no module', async ({ page }) => {
  // model by its name (AC-16).
  await gotoReady(page, 'layout=wide&model=TI-58C');
  await expect(el(page, 'bar-model')).toHaveValue('2');
  // A second PlayCalculator without keyboard, with 2.3.0's module prop and
  // settings off: keyboard is on by default, module is not read.
  await page.evaluate(() => {
    const div = document.createElement('div');
    div.id = 'second';
    div.style.width = '400px';
    document.body.appendChild(div);
    ReactDOM.createRoot(div).render(React.createElement(PlayCalculator,
      { layout: 'portrait', width: 360, height: 640, module: 'ST', settings: false }));
  });
  const second = page.locator('#second .calcu-client');
  await expect(second.locator('.calcu-key')).toHaveCount(45, { timeout: 15000 });
  await expect(second).toHaveAttribute('tabindex', '0');           // keyboard on
  await expect(second).toHaveAttribute('data-layout', 'portrait');  // layout
  // settings={false}: no Settings button and no Settings… behind More (LIB-14 AC-29, M18).
  await expect(second.locator('[data-calcu="bar-action-settings"]')).toHaveCount(0);
  // The second copy on the page keeps nothing it saves: its notice first (LIB-15 AC-8).
  await expect(second.locator('[data-calcu="notice"]')).toBeVisible();
  await second.locator('[data-calcu="notice-ok"]').click();
  await second.locator('[data-calcu="bar-more"]').click();
  await expect(second.locator('[data-calcu="more-item-settings"]')).toHaveCount(0);
  await expect(second.locator('[data-calcu="more-item-help"]')).toBeVisible();
  // 2.3.0's module prop is not read: the Master Library, as the control state says.
  await expect.poll(() => second.evaluate((e) => {
    const cs = e.calcuShell.getState().topics['control-state'];
    const slot = cs && cs.places.find((p) => p.place === 'slot-1');
    const plugged = slot && slot.fitting.find((f) => f.plugged);
    return plugged ? plugged.id : '';
  })).toBe('ML');
});

// LIB-15 AC-16 (LIB-6 §11.6 P11): the `preset` prop is a path of
// bundle.yaml, which the page hands LIB-18's start as its bundle path.
test('lib15_ac16 preset prop takes a path of bundle.yaml', async ({ page }) => {
  await gotoReady(page, 'layout=wide&preset=sum.ti59');
  await expect(page.locator('.cuecard')).toHaveAttribute('aria-label', /Sum 1\.\.N/, { timeout: 10000 });
});

test('ec4 an unknown model or preset gives a notice and starts without it', async ({ page }) => {
  await gotoReady(page, 'layout=wide&model=TI-99');
  await expect(el(page, 'notice')).toContainText('There is no model "TI-99"');   // the shell's notice (LIB-14 AC-31)
  await expect(el(page, 'bar-model')).toHaveValue('0');
  await gotoReady(page, 'layout=wide&preset=nothing.ti59');
  await expect(el(page, 'notice')).toContainText('could not be loaded');
});

test('ac22 no message of the old protocol', async ({ page }) => {
  await recordMessages(page);
  await gotoReady(page);
  await page.getByRole('button', { name: '5', exact: true }).click();
  await request(page, 'c59.mobile-rules', { w: 400, h: 800 });
  const { sent, received } = await page.evaluate(() => ({
    sent: window.__calcuSent.map((m) => m && m.kind),
    received: window.__calcuReceived.map((m) => m && m.kind),
  }));
  for (const k of sent) expect(['listen', 'command', 'request', 'answer']).toContain(k);
  for (const k of received) expect(['state', 'stream', 'reply', 'ask', 'sound-start', 'sound', 'sound-stop']).toContain(k);
  expect(sent).toContain('listen');
  // 2.3.0's own types are gone (calc-engine-worker.js at the pin).
  const types = await page.evaluate(() => [...window.__calcuSent, ...window.__calcuReceived].map((m) => m && m.type).filter(Boolean));
  expect(types).toEqual([]);
});

test('lib3_ac31 hidden page suspends, shown continues', async ({ page }) => {
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
  await expect(el(page, 'notice')).toHaveCount(0);
});
