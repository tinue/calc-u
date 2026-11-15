const { test, expect } = require('@playwright/test');

// LIB-5 in the Web client (design §6, T16-4, T16-6): the page sends the
// library's typing commands (host-key-down/up, screen-key-down/up, paste,
// focus-lost); the product's keyboard map and profiles in the engine decide.

async function gotoReady(page, query = '') {
  await page.goto(`./${query}`);
  await expect(page.locator('.calcu-client[data-ready=true]')).toHaveCount(1, { timeout: 30000 });
}

const engineOf = (page) => page.evaluate(() => !!document.querySelector('.calcu-client').calcuEngine);
const request = (page, method, ...args) =>
  page.evaluate(([m, a]) => document.querySelector('.calcu-client').calcuEngine.request(m, ...a), [method, args]);
const screen = (page) => request(page, 'lcdText');
const busy = (page) => page.evaluate(() => document.querySelector('.calcu-client').calcuEngine.getState().busy);

async function pickModel(page, index) {
  await page.getByLabel('Model', { exact: true }).selectOption(String(index));
  await expect(page.locator(`.calcu-face[data-model="${index}"]`)).toHaveCount(1, { timeout: 30000 });
  await expect.poll(() => busy(page), { timeout: 30000 }).toBe(false);
}

// AC-33: Shift tapped alone within 400 ms is one press of the machine's
// SHIFT: the next key is its second legend (SHIFT + 1 is ' on the PC-1600).
test('a SHIFT tap is one SHIFT press', async ({ page }) => {
  await gotoReady(page, '?model=2');
  expect(await engineOf(page)).toBe(true);
  await pickModel(page, 2);
  await expect.poll(() => screen(page), { timeout: 20000 }).toMatch(/>/);
  await page.keyboard.down('Shift');
  await page.keyboard.up('Shift');
  await page.waitForTimeout(300);
  await page.keyboard.press('1');
  await expect.poll(() => screen(page), { timeout: 10000 }).toMatch(/'/);
});

// A host key the engine maps: typed through the keyboard map (AC-29).
test('host keys type through the product keyboard map', async ({ page }) => {
  await gotoReady(page, '?model=2');
  await pickModel(page, 2);
  await page.keyboard.type('PRINT 6*7', { delay: 60 });
  await page.keyboard.press('Enter');
  await expect.poll(() => screen(page), { timeout: 20000 }).toMatch(/42/);
});

// AC-36 (row 1600-9): keys during an operation are dropped and not pressed
// after it.
test('keys during a load are dropped', async ({ page }) => {
  await gotoReady(page, '?model=2');
  await pickModel(page, 2);
  // The keys go in at the moment the session says busy (a poll from the
  // test can miss a short operation): host-key-down/up of '7' three times.
  await page.evaluate(() => {
    const e = document.querySelector('.calcu-client').calcuEngine;
    window.calcuTypedWhileBusy = false;
    const off = e.subscribe(() => {
      if (!e.getState().busy || window.calcuTypedWhileBusy) return;
      window.calcuTypedWhileBusy = true;
      off();
      for (let i = 0; i < 3; i++) {
        e.call('hostKeyDown', 'Digit7', '7', false, false, false, false, false);
        e.call('hostKeyUp', 'Digit7');
      }
    });
  });
  await page.getByLabel('Model', { exact: true }).selectOption('1');   // a model switch: an operation
  await expect.poll(() => page.evaluate(() => window.calcuTypedWhileBusy), { timeout: 10000 }).toBe(true);
  await expect.poll(() => busy(page), { timeout: 30000 }).toBe(false);
  await page.waitForTimeout(1000);
  expect(await screen(page)).not.toMatch(/777/);
});
