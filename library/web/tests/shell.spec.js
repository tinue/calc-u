const { test, expect } = require('@playwright/test');
const { open, send, request, SENTENCES } = require('./helpers');

// The React set's shell in the
// test product (src/CalcUTest.jsx with shell=1), drawn from the topics of
// library/shell/tests/TestShellPart.hpp and the library's SettingsPart, no
// code of a product; Chromium and WebKit. The GUI scenarios are those of
// the Qt6 set (library/qt6/tests/shell_scenarios.cpp); elements are
// found by their data-calcu names.

const el = (page, name) => page.locator(`[data-calcu="${name}"]`);
const sent = async (page, command) => {
  const r = await request(page, 'tsp.sent');
  return r.value.sent.filter((x) => !command || x.command === command);
};
const topic = (page, name) => page.evaluate((n) => window.calcuTestShells[0].getState().topics[n], name);
// An operation: its reply comes at its end, so nobody waits for it here.
const startOperation = (page) => page.evaluate(() => { window.calcuTestShells[0].send('tsp.op'); });

async function start(page) {
  await open(page, 'Test-A', 'shell=1&width=900&height=900');
  await expect(el(page, 'menu-bar')).toBeVisible();
  await expect(el(page, 'control-bar')).toBeVisible();
}
async function chooseItem(page, menu, ...path) {
  await el(page, `menu-${menu}`).click();
  for (const id of path) await el(page, `item-${id}`).click();
}
// A key at hit-map (x, y) on the faceplate, scale 4.
async function pointerAt(page, x, y) {
  const box = await el(page, 'test-faceplate').boundingBox();
  return [box.x + x * 4, box.y + y * 4];
}

test('the menus', async ({ page }) => {
  await start(page);
  const titles = await page.locator('[data-calcu^="menu-"]:not([data-calcu="menu-bar"])').allTextContents();
  expect(titles).toEqual(['File', 'Edit', 'Machine', 'Help']);   // Edit: the test product has Paste Text
  await el(page, 'menu-file').click();
  for (const id of ['load-preset', 'examples', 'load-program', 'settings']) await expect(el(page, `item-${id}`)).toBeVisible();
  await el(page, 'item-examples').click();
  await el(page, 'item-examples-Games').click();
  await el(page, 'item-example-examples:games/one.test').click();
  await expect.poll(async () => (await sent(page, 'load-preset')).map((x) => x.args.ref)).toEqual(['examples:games/one.test']);
  await chooseItem(page, 'machine', 'model', 'model-Model-B');
  await expect.poll(async () => (await sent(page, 'model')).length).toBe(1);
  await el(page, 'menu-machine').click();
  await el(page, 'item-model').click();
  await expect(el(page, 'item-model-Model-B')).toHaveAttribute('aria-checked', 'true');   // a model switch changes the entries
  // A product item disabled with its entry's reason; a product item acts.
  await page.mouse.click(5, 5);
  await el(page, 'menu-machine').click();
  await expect(el(page, 'item-disk-thing')).toHaveAttribute('aria-disabled', 'true');
  await expect(el(page, 'item-disk-thing')).toHaveAttribute('title', 'No disk drive.');
  await page.mouse.click(5, 5);
  await chooseItem(page, 'file', 'load-program');
  expect(await page.evaluate(() => window.calcuTestActs)).toBe(1);
});

test('no shortcut is bound', async ({ page }) => {
  await start(page);
  const prevented = await page.evaluate(() => {
    const root = document.querySelector('.calcu-client');
    return ['r', 'o', 'R'].map((key) => {
      const e = new KeyboardEvent('keydown', { key, ctrlKey: true, shiftKey: key === 'R', bubbles: true, cancelable: true });
      root.dispatchEvent(e);
      return e.defaultPrevented;
    });
  });
  expect(prevented).toEqual([false, false, false]);
  expect(await sent(page, 'reset')).toEqual([]);
  expect(await sent(page, 'cold-boot')).toEqual([]);
});

test('no quit', async ({ page }) => {
  await start(page);
  await el(page, 'menu-file').click();
  await expect(el(page, 'item-settings')).toBeVisible();
  await expect(el(page, 'item-quit')).toHaveCount(0);
});

test('the bar', async ({ page }) => {
  await start(page);
  const names = await el(page, 'control-bar').locator('[data-calcu]').evaluateAll((xs) => xs.map((x) => x.dataset.calcu));
  const order = ['bar-model', 'bar-slot-slot-1', 'bar-slot-slot-2', 'bar-place-printer', 'bar-place-aux', 'tp-disk', 'bar-media-card']
    .map((n) => names.indexOf(n));
  expect(order.every((i) => i >= 0)).toBe(true);
  expect([...order].sort((a, b) => a - b)).toEqual(order);   // left to right; the product's control in its group
  expect(names).not.toContain('bar-place-host-drive');      // a place needing more has no control
  await expect(el(page, 'bar-slot-slot-1')).toContainText('1:');
  await expect(el(page, 'bar-place-printer')).toHaveText('TD-20');
  await el(page, 'bar-place-printer').click();   // the button attaches
  await expect.poll(async () => (await sent(page, 'attach')).map((x) => x.args.peripheral)).toEqual(['test-dotter']);
  await el(page, 'bar-place-aux').selectOption({ label: 'TD-21' });
  await expect.poll(async () => (await sent(page, 'attach')).length).toBe(2);
  // Model-B has no slots and no media place: no gap.
  await el(page, 'bar-model').selectOption({ label: 'Model-B' });
  await expect(el(page, 'bar-slot-slot-1')).toHaveCount(0);
  await expect(el(page, 'bar-group-Slots')).toHaveCount(0);
});

test('the settings dialog', async ({ page }) => {
  await start(page);
  await chooseItem(page, 'file', 'settings');
  const d = el(page, 'settings-dialog');
  await expect(d).toBeVisible();
  await expect(d).toHaveAttribute('aria-label', 'Settings');
  await expect(d.locator('legend')).toHaveText(['General', 'Startup presets', 'Tracing']);
  await expect(d.locator('button')).toHaveText(['Close']);   // Close only
  await el(page, 'setting-startup-model').selectOption('test-dot');
  await expect.poll(async () => {
    const s = await topic(page, 'settings');
    return s.rows.find((r) => r.key === 'model' && r.section === 'startup').value;
  }).toBe('test-dot');   // applied and written at once
  await expect(el(page, 'setting-preset-test-seg').locator('option')).toHaveText(['None']);
  await el(page, 'settings-close').click();
  await expect(d).toHaveCount(0);
});

test('loading shows after 500 ms and goes on reset', async ({ page }) => {
  await start(page);
  await startOperation(page);
  await expect(el(page, 'loading')).toHaveCount(0);   // not before 500 ms
  await expect(el(page, 'loading')).toBeVisible({ timeout: 5000 });
  await expect(el(page, 'loading')).toContainText('Loading…');
  await expect(el(page, 'loading').locator('button')).toHaveCount(0);
  await send(page, 'reset');
  await expect(el(page, 'loading')).toHaveCount(0);
});

test('the bar works while loading', async ({ page }) => {
  await start(page);
  await startOperation(page);
  await expect(el(page, 'loading')).toBeVisible({ timeout: 5000 });
  await el(page, 'bar-place-printer').click();   // the bar can be used
  await expect.poll(async () => (await sent(page, 'attach')).length).toBe(1);
  await chooseItem(page, 'machine', 'reset');   // the menus and Reset can be used
  await expect.poll(async () => (await sent(page, 'reset')).length).toBe(1);
  await expect(el(page, 'loading')).toHaveCount(0);
  await expect(el(page, 'notice')).toHaveCount(0);   // the stopped operation is no notice
});

test('a notice', async ({ page }) => {
  await start(page);
  await send(page, 'tsp.notice', { text: 'The first went wrong.' });
  await send(page, 'tsp.notice', { text: 'The second went wrong.' });
  await expect(el(page, 'notice')).toContainText('The first went wrong.');
  await expect(el(page, 'notice').locator('.calcu-shell-modal-title')).toHaveCount(0);   // no title
  await el(page, 'notice-ok').click();
  await expect(el(page, 'notice')).toContainText('The second went wrong.');
  await el(page, 'notice-ok').click();
  await expect(el(page, 'notice')).toHaveCount(0);
  // A refusal of a command the shell sent is a notice; `asks:` is a question,
  // and Yes sends the command again with confirmed.
  await page.evaluate(() => window.calcuTestShellModel.sendChecked('tsp.refuse', { reason: 'That cannot be done now.' }));
  await expect(el(page, 'notice')).toContainText('That cannot be done now.');
  await el(page, 'notice-ok').click();
  await page.evaluate(() => window.calcuTestShellModel.sendChecked('tsp.confirm', { name: 'A' }));
  await expect(el(page, 'notice')).toContainText('Replace the card?');
  await el(page, 'notice-yes').click();
  await expect.poll(async () => (await sent(page, 'tsp.confirm')).map((x) => x.args)).toEqual([{ name: 'A' }, { name: 'A', confirmed: true }]);
  await expect(el(page, 'notice')).toHaveCount(0);
});

test('roms missing', async ({ page }) => {
  await start(page);
  await send(page, 'tsp.publish', { topic: 'roms-missing', value: { text: 'The ROM files of Model-A are missing.' } });
  await send(page, 'tsp.publish', { topic: 'control-state', value: null });
  await expect(el(page, 'roms-missing')).toHaveText('The ROM files of Model-A are missing.');
  await expect(el(page, 'control-bar')).toHaveCount(0);
  await expect(el(page, 'menu-machine')).toHaveCount(0);
  await expect(el(page, 'test-faceplate')).toBeHidden();
  await chooseItem(page, 'help', 'about');   // Help and About still work
  await expect(el(page, 'about-dialog')).toBeVisible();
  await expect(el(page, 'about-version')).toHaveText('Version 1.2.3 (abc123)');
});

test('help opens', async ({ page }) => {
  await start(page);
  await page.evaluate(() => { window.calcuOpened = []; window.open = (url) => { window.calcuOpened.push(url); return null; }; });
  await chooseItem(page, 'help', 'help');
  await expect.poll(() => page.evaluate(() => window.calcuOpened)).toEqual(['https://calc-u.org/test/']);
});

test('the shell without product code', async ({ page }) => {
  await start(page);
  for (const name of ['menu-bar', 'control-bar', 'panel-area', 'paper-panel']) await expect(el(page, name)).toHaveCount(1);
  // The component set's part of calc.js (from its first file to the product's) names no product.
  const text = await page.evaluate(() => fetch('calc.js').then((r) => r.text()));
  const set = text.slice(text.indexOf('/* === paper.js === */'), text.indexOf('/* === CalcUTest.jsx === */'));
  expect(set).toContain('function ShellMenuBar');
  for (const name of ['TI-59', 'TI-58', 'PC-1500', 'PC-1600', 'Calc-U-59', 'Calc-U-1600']) expect(set).not.toContain(name);
});

test('a key under the pointer', async ({ page }) => {
  await start(page);
  const [x, y] = await pointerAt(page, 25, 25);
  await page.mouse.move(x, y);
  await page.mouse.down();
  await expect.poll(async () => (await sent(page, 'screen-key-down')).map((s) => s.args.key)).toEqual(['2']);
  await page.mouse.move(x + 80, y);   // a drag changes no key
  await page.mouse.up();
  await expect.poll(async () => (await sent(page, 'screen-key-up')).map((s) => s.args.key)).toEqual(['2']);
  // Down where no key is, then onto a key: nothing.
  const [nx, ny] = await pointerAt(page, 15, 40);
  await page.mouse.move(nx, ny);
  await page.mouse.down();
  await page.mouse.move(x, y);
  await page.mouse.up();
  await page.waitForTimeout(200);
  expect((await sent(page, 'screen-key-down')).length).toBe(1);
});

test('holding the display', async ({ page }) => {
  await start(page);
  const [x, y] = await pointerAt(page, 10, 5);
  await page.mouse.move(x, y);
  await page.mouse.down();
  await expect.poll(async () => (await topic(page, 'busy')).speed).toBe('full');
  await expect(el(page, 'loading')).toHaveCount(0);   // no operation: no "Loading…"
  await page.mouse.up();
  await expect.poll(async () => (await topic(page, 'busy')).speed).toBe('regular');
});

// The hit map's origin inside the
// faceplate element, and a press the product takes first passes by.
test('faceplate origin and a press the product takes', async ({ page }) => {
  await open(page, 'Test-A', 'shell=1&width=900&height=900&origin=40');
  await expect(el(page, 'control-bar')).toBeVisible();
  const box = await el(page, 'test-faceplate').boundingBox();
  const at = (x, y) => [box.x + 40 + x * 4, box.y + 40 + y * 4];
  // Key 2's centre, measured from the origin.
  let [x, y] = at(25, 25);
  await page.mouse.move(x, y);
  await page.mouse.down();
  await page.mouse.up();
  await expect.poll(async () => (await sent(page, 'screen-key-down')).map((s) => s.args.key)).toEqual(['2']);
  // The same point without the origin would be key 1's area: no key 1.
  [x, y] = at(5, 25);
  await page.mouse.move(x, y);
  await page.mouse.down();
  await page.mouse.up();
  await expect.poll(async () => (await sent(page, 'screen-key-down')).map((s) => s.args.key)).toEqual(['2', '1']);
  // A press at x ≥ 60 is the product's: no key, no speed, its release passes too.
  [x, y] = at(65, 25);
  await page.mouse.move(x, y);
  await page.mouse.down();
  await page.mouse.up();
  await page.waitForTimeout(200);
  expect(await page.evaluate(() => window.calcuTestProductPresses)).toBe(1);
  expect((await sent(page, 'screen-key-down')).length).toBe(2);
  expect((await sent(page, 'screen-key-up')).length).toBe(2);
});

// The storage banner is
// the library's sentence while what is saved is not kept.
test("storage banner shows the library's sentence while not kept", async ({ page, context }) => {
  await open(page, 'Test-A', 'shell=1&width=900&height=900');
  await expect(el(page, 'storage-banner')).toHaveCount(0);   // the first copy keeps what it saves
  const second = await context.newPage();
  await open(second, 'Test-A', 'shell=1&width=900&height=900');
  await expect(second.locator('[data-calcu="storage-banner"]')).toHaveText(SENTENCES.secondCopy('Test-A'));
  await expect(second.locator('[data-calcu="storage-banner"]')).toHaveAttribute('role', 'status');
  await second.close();
});

test('file menu has import and saved media', async ({ page }) => {
  await start(page);
  await el(page, 'menu-file').click();
  await expect(el(page, 'item-import-media')).toHaveCount(0);   // no kinds of medium
  await page.mouse.click(5, 5);
  await send(page, 'tsp.publish', { topic: 'media', value: { kinds: [{ kind: 'test-card', label: 'Card' }], places: [] } });
  await el(page, 'menu-file').click();
  await expect(el(page, 'item-import-media')).toHaveText(/Import Media…/);
  await expect(el(page, 'item-saved-media')).toHaveText(/Saved Media…/);
  await el(page, 'item-import-media').click();
  await expect.poll(async () => (await sent(page, 'media.import')).length).toBe(1);
});

