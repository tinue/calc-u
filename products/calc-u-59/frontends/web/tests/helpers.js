// What the Calc-U-59 Web tests share.
const { expect } = require('@playwright/test');

// The page with `query`, ready for keys: the key buttons appear once the
// worker has posted its static data; the calculator boots in real time, as
// the hardware, and a key pressed in its first moments is lost, so it gets
// time to reach the keyboard scan loop.
async function gotoReady(page, query = 'layout=wide') {
  await page.goto(`./?${query}`);
  await expect(page.locator('.calcu-key')).toHaveCount(45, { timeout: 15000 });
  await page.waitForTimeout(600);
}

module.exports = { gotoReady };
