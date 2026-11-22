const { test, expect } = require('@playwright/test');
const { gotoReady } = require('./helpers');

// The Calc-U-59 Web frontend on the library's shell and app session: the
// props, the messages, unknown names, and suspending a hidden page.

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

// The page talks to the library's shell itself: the root element's
// `calcuShell`.
const request = (page, name, args) =>
  page.evaluate(([n, a]) => document.querySelector('.calcu-client').calcuShell.request(n, a).then((r) => r.value), [name, args || null]);
const el = (page, name) => page.locator(`[data-calcu="${name}"]`);

test('CalcU59 props, layout and settings', async ({ page }) => {
  // model by its name.
  await gotoReady(page, 'layout=wide&model=TI-58C');
  await expect(el(page, 'bar-model')).toHaveValue('2');
  // A second CalcU59 without keyboard and with settings off: keyboard is on
  // by default.
  await page.evaluate(() => {
    const div = document.createElement('div');
    div.id = 'second';
    div.style.width = '400px';
    document.body.appendChild(div);
    ReactDOM.createRoot(div).render(React.createElement(CalcU59,
      { layout: 'portrait', width: 360, height: 640, settings: false }));
  });
  const second = page.locator('#second .calcu-client');
  await expect(second.locator('.calcu-key')).toHaveCount(45, { timeout: 15000 });
  await expect(second).toHaveAttribute('tabindex', '0');           // keyboard on
  await expect(second).toHaveAttribute('data-layout', 'portrait');  // layout
  // settings={false}: no Settings… behind More.
  // The second copy on the page keeps nothing it saves: its notice first.
  await expect(second.locator('[data-calcu="notice"]')).toBeVisible();
  await second.locator('[data-calcu="notice-ok"]').click();
  await second.locator('[data-calcu="bar-more"]').click();
  await expect(second.locator('[data-calcu="more-item-settings"]')).toHaveCount(0);
  await expect(second.locator('[data-calcu="more-item-help"]')).toBeVisible();
});

// The component is CalcU59, the one global of calc.js, with its
// `portraitWidth`; the shared props have layout among them.
test('CalcU59 is the component', async ({ page }) => {
  await gotoReady(page, 'layout=portrait');
  expect(await page.evaluate(() => [typeof window.CalcU59,
    typeof (window.CalcU59 && window.CalcU59.portraitWidth)])).toEqual(['function', 'function']);
  await expect(page.locator('.calcu-client')).toHaveAttribute('data-layout', 'portrait');
});

// The `preset` prop is a path of bundle.yaml, with or without `examples:`,
// which the page hands the session's start as its bundle path.
test('preset prop takes a path of bundle.yaml', async ({ page }) => {
  for (const preset of ['sum.ti59', 'examples:sum.ti59']) {
    await gotoReady(page, `layout=wide&preset=${preset}`);
    await expect(page.locator('.cuecard')).toHaveAttribute('aria-label', /Sum 1\.\.N/, { timeout: 10000 });
  }
});

// A layout that is not one of Calc-U-59's
// is ignored with a console warning, and the default, portrait, is drawn.
test('an unknown layout is ignored with a warning', async ({ page }) => {
  const warnings = [];
  page.on('console', (m) => { if (m.type() === 'warning') warnings.push(m.text()); });
  await gotoReady(page, 'layout=wide');
  await page.evaluate(() => {
    const div = document.createElement('div');
    div.id = 'layout-tall';
    document.body.appendChild(div);
    ReactDOM.createRoot(div).render(React.createElement(CalcU59, { layout: 'tall', width: 400, height: 700 }));
  });
  await expect(page.locator('#layout-tall .calcu-client')).toHaveAttribute('data-layout', 'portrait', { timeout: 10000 });
  await expect.poll(() => warnings.filter((w) => /layout/.test(w))).toEqual(
    ['CalcU59: layout "tall" is not one of its layouts ("portrait", "wide", "auto"); it is ignored.']);
});

test('an unknown model or preset gives a notice and starts without it', async ({ page }) => {
  await gotoReady(page, 'layout=wide&model=TI-99');
  await expect(el(page, 'notice')).toContainText('There is no model "TI-99"');   // the shell's notice
  await expect(el(page, 'bar-model')).toHaveValue('0');
  await gotoReady(page, 'layout=wide&preset=nothing.ti59');
  await expect(el(page, 'notice')).toContainText('could not be loaded');
});

test('only the messages of the session protocol', async ({ page }) => {
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
});

test('hidden page suspends, shown continues', async ({ page }) => {
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
