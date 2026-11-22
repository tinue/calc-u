const { test, expect } = require('@playwright/test');
const { gotoReady } = require('./helpers');

// The persistent file system of the Calc-U-59 Web frontend: its settings
// part, the TI-58C's constant memory as a file of the save folder
// (ti58c.memory.yaml), the magnetic cards, and one writer at a time. The
// store is IndexedDB "/persist-calc-u-59".

test.describe.configure({ timeout: 30000 });

const STORE = '/persist-calc-u-59';
const INI = `${STORE}/settings/Calc-U-59/Calc-U-59.ini`;
const MEM = `${STORE}/Calc-U-59/ti58c.memory.yaml`;

// The shell's elements by their data-calcu names.
const el = (page, name) => page.locator(`[data-calcu="${name}"]`);
const modelPicker = (page) => el(page, 'bar-model');
/// File ▸ Settings… (the shell's dialog), the startup model set, closed.
async function setStartupModel(page, label) {
  await el(page, 'menu-file').click();
  await el(page, 'item-settings').click();
  await el(page, 'setting-startup-model').selectOption({ label });
  await el(page, 'settings-close').click();
}

// The files IndexedDB holds for the product: {path: text} (IDBFS's store).
const storedFiles = (page) => page.evaluate((store) => new Promise((resolve) => {
  const open = indexedDB.open(store);
  open.onsuccess = () => {
    const db = open.result;
    if (!db.objectStoreNames.contains('FILE_DATA')) { resolve({}); return; }
    const out = {};
    const cursor = db.transaction('FILE_DATA').objectStore('FILE_DATA').openCursor();
    cursor.onsuccess = () => {
      const c = cursor.result;
      if (!c) { resolve(out); return; }
      const v = c.value;
      if (v && v.contents) out[String(c.key)] = new TextDecoder().decode(v.contents);
      c.continue();
    };
    cursor.onerror = () => resolve(out);
  };
  open.onerror = () => resolve({});
}), STORE);

test('settings survive a page load, in Calc-U-59.ini', async ({ page }) => {
  await gotoReady(page);
  await setStartupModel(page, 'TI-58');
  await expect.poll(async () => (await storedFiles(page))[INI] || '', { timeout: 5000 }).toContain('model = TI-58');
  await gotoReady(page);
  await expect(modelPicker(page)).toHaveValue('1');
});

async function press(page, keys) {
  for (const k of keys) {
    await page.getByRole('button', { name: k, exact: true }).click();
    await page.waitForTimeout(200);
  }
}

// The display is a canvas: a settled frame as a picture.
async function settledLed(page) {
  const led = page.locator('canvas.calcu-led');
  let previous = await led.evaluate((c) => c.toDataURL());
  for (let i = 0; i < 20; i++) {
    await page.waitForTimeout(250);
    const current = await led.evaluate((c) => c.toDataURL());
    if (current === previous) return current;
    previous = current;
  }
  throw new Error('LED display never settled');
}

test('the constant memory is ti58c.memory.yaml in the save folder', async ({ page }) => {
  await gotoReady(page, 'layout=wide&model=TI-58C');
  await page.waitForTimeout(1500);   // the TI-58C boots
  await press(page, ['4', '2', 'STO', '0', '7', 'CLR']);
  await expect.poll(async () => Object.keys(await storedFiles(page)), { timeout: 5000 }).toContain(MEM);
  const text = (await storedFiles(page))[MEM];
  expect(text).toContain('format: ti58c-constant-memory');
  expect(text).toContain('format-version: 1');
  expect(Object.keys(await storedFiles(page)).filter((k) => k.endsWith('/ti58c.mem'))).toEqual([]);
});

// A model switch to the
// TI-58C, a value stored, a page load: the last model and the value come back.
test('the constant memory and the last model are kept across a page load', async ({ page }) => {
  await gotoReady(page);
  await modelPicker(page).selectOption({ label: '58C' });
  await expect(modelPicker(page)).toHaveValue('2');
  await page.waitForTimeout(1500);   // the TI-58C boots
  await press(page, ['CLR', '7', '=']);
  const seven = await settledLed(page);
  await press(page, ['STO', '1', '2', 'CLR']);
  await page.waitForTimeout(1000);   // the half-second write
  await gotoReady(page);
  await expect(modelPicker(page)).toHaveValue('2');   // [startup] last-model
  await page.waitForTimeout(1500);
  await press(page, ['CLR', 'RCL', '1', '2']);
  expect(await settledLed(page)).toBe(seven);
});

test('no calcu59 key read or written', async ({ page }) => {
  await page.addInitScript(() => {
    window.__calcuKeys = [];
    for (const name of ['getItem', 'setItem', 'removeItem']) {
      const original = Storage.prototype[name];
      Storage.prototype[name] = function (key, ...rest) {
        window.__calcuKeys.push(String(key));
        return original.call(this, key, ...rest);
      };
    }
  });
  await gotoReady(page);
  await setStartupModel(page, 'TI-58C');
  await modelPicker(page).selectOption({ label: '58C' });
  await page.waitForTimeout(1500);
  const keys = await page.evaluate(() => window.__calcuKeys);
  expect(keys.filter((k) => k.startsWith('calcu59/'))).toEqual([]);
});

test('a second tab says it cannot save', async ({ page, context }, testInfo) => {
  await gotoReady(page);
  const second = await context.newPage();
  await gotoReady(second);
  // The sentence stays in the shell's storage banner, besides its notice.
  await expect(el(second, 'storage-banner')).toHaveText(
    'Calc-U-59 is open in another tab or window: what you save here is not kept.');
  await expect(el(page, 'storage-banner')).toHaveCount(0);   // the first keeps what it saves
  await expect(el(second, 'notice')).toContainText('open in another tab or window');
  await el(second, 'notice-ok').click();
  await expect(el(second, 'storage-banner')).toBeVisible();   // the banner stays
  // What the second saves is not kept: its startup model stays out of the store.
  await setStartupModel(second, 'TI-58C');
  await second.waitForTimeout(1500);
  expect((await storedFiles(page))[INI] || '').not.toContain('model = TI-58C');
  testInfo.annotations.push({ type: 'exit check', description: 'a second tab says it cannot save' });
  await second.close();
});

// ── Magnetic cards ─────────────────────────────────────────────────────────

const CARDS = `${STORE}/Calc-U-59`;
const cardFile = (name) =>
  `format: ti59-magnetic-card\nformat-version: 3\ncard-name: "${name}"\ncue-card:\n  title: ${name}\nsides: {}\n`;
// Lbl A 1 2 3 R/S, then CLR 1 2nd Write at regular speed: the machine waits for a card.
const PROGRAM_WRITE = 'format-version: 2\nmodel: TI-59\nslot-1: { name: ML }\n' +
  'program:\n  keycodes: |\n    76 11 01 02 03 91\nkeys:\n  - regular-speed:\n  - key: CLR 1 Write\n';

async function listedCards(page) {
  await page.getByRole('button', { name: 'Arm Card', exact: true }).click();
  const names = await page.getByRole('list', { name: 'Saved cards' }).locator('button').allTextContents();
  await page.getByRole('button', { name: 'Cancel' }).click();
  return names;
}

// A card imported is a file of the persistent save folder, named after its name.
test('cards in the persistent save folder', async ({ page }) => {
  await gotoReady(page);
  const chooser = page.waitForEvent('filechooser');
  await el(page, 'menu-file').click();
  await el(page, 'item-import-media').click();   // File ▸ Import Media…
  await (await chooser).setFiles({ name: 'Saved.magnetcard.yaml', mimeType: 'application/yaml', buffer: Buffer.from(cardFile('Saved')) });
  await expect.poll(async () => Object.keys(await storedFiles(page)), { timeout: 5000 }).toContain(`${CARDS}/Saved.magnetcard.yaml`);
  expect((await storedFiles(page))[`${CARDS}/Saved.magnetcard.yaml`]).toContain('format: ti59-magnetic-card');
});

// A card written by a program, the page loaded again: the card dialog lists it.
test('cards survive a page load', async ({ page }) => {
  await gotoReady(page);
  const presetChooser = page.waitForEvent('filechooser');
  await el(page, 'menu-file').click();
  await el(page, 'item-load-preset').click();
  await (await presetChooser).setFiles({ name: 'w.ti59', mimeType: 'text/plain', buffer: Buffer.from(PROGRAM_WRITE) });
  await page.waitForTimeout(1600);
  await page.getByRole('button', { name: 'Arm Fresh Card', exact: true }).click();
  await page.getByLabel('Card name').fill('Written');
  await page.getByRole('button', { name: 'Arm', exact: true }).click();
  await page.getByRole('button', { name: 'Feed card side 1' }).click();
  await expect(page.getByRole('button', { name: 'Eject Card' })).toHaveCount(0, { timeout: 15000 });
  await expect.poll(async () => Object.keys(await storedFiles(page)), { timeout: 5000 }).toContain(`${CARDS}/Written.magnetcard.yaml`);
  expect((await storedFiles(page))[`${CARDS}/Written.magnetcard.yaml`]).toContain('bank: 1');
  await gotoReady(page);
  expect(await listedCards(page)).toEqual(['Written']);
});
