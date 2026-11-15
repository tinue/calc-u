const { test, expect } = require('@playwright/test');

// The persistent file system of the Calc-U-59 Web frontend (M-34): LIB-15
// AC-20 (its settings part, and ti58c.mem as a file of the save folder; the
// cards and ti58c.memory.yaml are M-55's and M-54's), AC-21 (one writer),
// LIB-9 AC-23. The store is IndexedDB "/persist-calc-u-59" (design §3.3).

test.describe.configure({ timeout: 30000 });

const STORE = '/persist-calc-u-59';
const INI = `${STORE}/settings/Calc-U-59/Calc-U-59.ini`;
const MEM = `${STORE}/Calc-U-59/ti58c.mem`;

async function gotoReady(page, query = 'layout=wide') {
  await page.goto(`./?${query}`);
  await expect(page.locator('.calcu-key')).toHaveCount(45, { timeout: 15000 });
  await page.waitForTimeout(600);
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

test('ac20 settings survive a page load, in Calc-U-59.ini', async ({ page }) => {
  await gotoReady(page);
  await page.getByRole('button', { name: 'Settings' }).click();
  await page.getByLabel('Startup Calculator').selectOption({ label: 'TI-58' });
  await page.getByRole('button', { name: 'Done' }).click();
  await expect.poll(async () => (await storedFiles(page))[INI] || '', { timeout: 5000 }).toContain('model = TI-58');
  await gotoReady(page);
  await expect(page.getByLabel('Calculator model')).toHaveValue('1');
});

test('ac20 the TI-58C constant memory is ti58c.mem in the save folder', async ({ page }) => {
  await gotoReady(page, 'layout=wide&model=TI-58C');
  await page.waitForTimeout(1500);   // the TI-58C boots
  for (const k of ['4', '2', 'STO', '0', '7', 'CLR']) {
    await page.getByRole('button', { name: k, exact: true }).click();
    await page.waitForTimeout(200);
  }
  await expect.poll(async () => Object.keys(await storedFiles(page)), { timeout: 5000 }).toContain(MEM);
});

test('lib9_ac23 no calcu59 key read or written', async ({ page }) => {
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
  await page.getByRole('button', { name: 'Settings' }).click();
  await page.getByLabel('Startup Calculator').selectOption({ label: 'TI-58C' });
  await page.getByRole('button', { name: 'Done' }).click();
  await page.getByLabel('Calculator model').selectOption({ label: 'TI-58C' });
  await page.waitForTimeout(1500);
  const keys = await page.evaluate(() => window.__calcuKeys);
  expect(keys.filter((k) => k.startsWith('calcu59/'))).toEqual([]);
});

test('ac21 a second tab says it cannot save', async ({ page, context }, testInfo) => {
  await gotoReady(page);
  const second = await context.newPage();
  await gotoReady(second);
  await expect(second.locator('.calcu-status')).toContainText(
    'Calc-U-59 is open in another tab or window: what you save here is not kept.');
  await expect(page.locator('.calcu-status')).toHaveCount(0);   // the first keeps what it saves
  // What the second saves is not kept: its startup model stays out of the store.
  await second.getByRole('button', { name: 'Dismiss' }).click();
  await second.getByRole('button', { name: 'Settings' }).click();
  await second.getByLabel('Startup Calculator').selectOption({ label: 'TI-58C' });
  await second.getByRole('button', { name: 'Done' }).click();
  await second.waitForTimeout(1500);
  expect((await storedFiles(page))[INI] || '').not.toContain('model = TI-58C');
  testInfo.annotations.push({ type: 'exit check', description: 'a second tab says it cannot save' });
  await second.close();
});
