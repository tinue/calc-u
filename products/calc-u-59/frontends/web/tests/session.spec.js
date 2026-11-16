const { test, expect } = require('@playwright/test');

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

async function gotoReady(page, query = 'layout=wide') {
  await page.goto(`./?${query}`);
  await expect(page.locator('.calcu-key')).toHaveCount(45, { timeout: 15000 });
  await page.waitForTimeout(600);
}

const request = (page, method, ...args) =>
  page.evaluate(([m, a]) => document.querySelector('.calcu-client').calcuEngine.request(m, ...a), [method, args]);
const engineState = (page, pick) =>
  page.evaluate((k) => document.querySelector('.calcu-client').calcuEngine.getState()[k], pick);

test('ac19 PlayCalculator props, layout and settings, no module', async ({ page }) => {
  // model by its name (AC-16).
  await gotoReady(page, 'layout=wide&model=TI-58C');
  await expect(page.getByLabel('Calculator model')).toHaveValue('2');
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
  await expect(second.getByRole('button', { name: 'Settings' })).toHaveCount(0);   // settings={false}
  await expect.poll(() => second.evaluate((e) => (e.calcuEngine.getState().model || {}).moduleID)).toBe('ML');
});

// LIB-15 AC-16 (LIB-6 §11.6 P11): the `preset` prop is a path of
// bundle.yaml, which the page hands LIB-18's start as its bundle path.
test('lib15_ac16 preset prop takes a path of bundle.yaml', async ({ page }) => {
  await gotoReady(page, 'layout=wide&preset=sum.ti59');
  await expect(page.locator('.cuecard')).toHaveAttribute('aria-label', /Sum 1\.\.N/, { timeout: 10000 });
});

test('ec4 an unknown model or preset gives a notice and starts without it', async ({ page }) => {
  await gotoReady(page, 'layout=wide&model=TI-99');
  await expect(page.locator('.calcu-status')).toContainText('There is no model "TI-99"');
  await expect(page.getByLabel('Calculator model')).toHaveValue('0');
  await gotoReady(page, 'layout=wide&preset=nothing.ti59');
  await expect(page.locator('.calcu-status')).toContainText('could not be loaded');
});

test('ac22 no message of the old protocol', async ({ page }) => {
  await recordMessages(page);
  await gotoReady(page);
  await page.getByRole('button', { name: '5', exact: true }).click();
  await request(page, 'mobileRules', 400, 800);
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
  expect(await engineState(page, 'status')).toBe('');
});
