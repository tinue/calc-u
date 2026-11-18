const { test, expect } = require('@playwright/test');
const h = require('./helpers');
const { gotoReady, screen, pickModel, busy, c } = h;

// Typing in the Web client: the page sends the
// library's typing commands (host-key-down/up, screen-key-down/up, paste,
// focus-lost); the product's keyboard map and profiles in the engine decide.

// Shift tapped alone within 400 ms is one press of the machine's
// SHIFT: the next key is its second legend (SHIFT + 1 is ' on the PC-1600).
test('a SHIFT tap is one SHIFT press', async ({ page }) => {
  await gotoReady(page, '?model=PC-1600');
  expect(await page.evaluate(() => !!document.querySelector('.calcu-client').calcuShell)).toBe(true);
  await pickModel(page, 'PC-1600');
  await expect.poll(() => screen(page), { timeout: 20000 }).toMatch(/>/);
  await page.keyboard.down('Shift');
  await page.keyboard.up('Shift');
  await page.waitForTimeout(300);
  await page.keyboard.press('1');
  await expect.poll(() => screen(page), { timeout: 10000 }).toMatch(/'/);
});

// A host key the engine maps: typed through the keyboard map.
test('host keys type through the product keyboard map', async ({ page }) => {
  await gotoReady(page, '?model=PC-1600');
  await pickModel(page, 'PC-1600');
  await page.keyboard.type('PRINT 6*7', { delay: 60 });
  await page.keyboard.press('Enter');
  await expect.poll(() => screen(page), { timeout: 20000 }).toMatch(/42/);
});

// Keys during an operation are dropped and not pressed
// after it.
test('keys during a load are dropped', async ({ page }) => {
  await gotoReady(page, '?model=PC-1600');
  await pickModel(page, 'PC-1600');
  // The keys go in at the moment the session says busy (a poll from the
  // test can miss a short operation): host-key-down/up of '7' three times.
  await page.evaluate(() => {
    const shell = document.querySelector('.calcu-client').calcuShell;
    window.calcuTypedWhileBusy = false;
    const off = shell.subscribe(() => {
      const b = shell.getState().topics.busy;
      if (!b || !b.operationRuns || window.calcuTypedWhileBusy) return;
      window.calcuTypedWhileBusy = true;
      off();
      const key = { physical: 'Digit7', named: '', produced: 55, carried: 0, shift: false, alt: false,
        'mac-host': false, command: false, control: false, meta: false };
      for (let i = 0; i < 3; i++) {
        shell.send('host-key-down', key);
        shell.send('host-key-up', { physical: 'Digit7' });
      }
    });
  });
  await page.locator(c('bar-model')).selectOption({ label: 'PC-1500A' });   // a model switch: an operation
  await expect.poll(() => page.evaluate(() => window.calcuTypedWhileBusy), { timeout: 10000 }).toBe(true);
  await expect.poll(() => busy(page), { timeout: 30000 }).toBe(false);
  await page.waitForTimeout(1000);
  expect(await screen(page)).not.toMatch(/777/);
});
