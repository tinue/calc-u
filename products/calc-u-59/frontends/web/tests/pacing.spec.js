const { test, expect } = require('@playwright/test');

// LIB-17 AC-19 (design P59-5) for the Web frontend: a TI-59 runs at regular
// speed for one minute of wall time and its emulated time grows by one
// minute, within the bound of AC-1. Checkpoint (D-5): skipped unless
// CALCU59_WEB_PACING=1 (CTest CalcU59WebPacing:
// `CALCU59_WEB_PACING=1 run-tests.sh tests/pacing.spec.js --project root`).
// Emulated time is the session's `ticks` request (LIB-3 AC-39) through the
// page's engine (since M-34 the Web runs on the library's session and pacer);
// wall time is the test's own clock, the middle of each read, and half the
// round trip of each read is added to the bound.

test.skip(!process.env.CALCU59_WEB_PACING, 'Checkpoint: CALCU59_WEB_PACING=1');
test.describe.configure({ timeout: 120000 });

// The bound of AC-1 (one 20 ms slice, LIB-17 AC-15 for the TI-59) plus one
// period of the worker's 60 Hz tick (a read comes between two ticks).
const BOUND = 0.020 + 1 / 60;

async function read(page) {
  const sent = Date.now();
  const { seconds } = await page.evaluate(() => document.querySelector('.calcu-client').calcuShell.request('ticks').then((r) => r.value));
  const back = Date.now();
  return { seconds, wall: (sent + back) / 2000, roundTrip: (back - sent) / 1000 };
}

test('a TI-59 keeps one minute', async ({ page }, testInfo) => {
  test.skip(testInfo.project.name !== 'root', 'one run is enough');
  await page.goto('./?layout=wide');
  await expect(page.locator('.calcu-key')).toHaveCount(45, { timeout: 15000 });
  await page.waitForTimeout(1000);   // the ROM reaches its keyboard scan

  const a = await read(page);
  await page.waitForTimeout(60000);
  const b = await read(page);
  const emulated = b.seconds - a.seconds, wall = b.wall - a.wall;
  const bound = BOUND + (a.roundTrip + b.roundTrip) / 2;
  const note = `wall ${wall.toFixed(4)} s, emulated ${emulated.toFixed(4)} s, ` +
    `difference ${(emulated - wall).toFixed(4)} s, bound ${bound.toFixed(4)} s`;
  console.log(`[AC-19] ${note}`);
  testInfo.annotations.push({ type: 'AC-19', description: note });
  expect(Math.abs(emulated - wall), note).toBeLessThanOrEqual(bound);
});
