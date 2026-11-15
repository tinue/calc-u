const { test, expect } = require('@playwright/test');

// LIB-17 AC-19 (design P59-5) for the Web frontend: a TI-59 runs at regular
// speed for one minute of wall time and its emulated time grows by one
// minute, within the bound of AC-1. Checkpoint (D-5): skipped unless
// CALCU59_WEB_PACING=1 (CTest CalcU59WebPacing:
// `CALCU59_WEB_PACING=1 run-tests.sh tests/pacing.spec.js --project root`).
// Emulated time is read in the engine's worker through the binding emulatedSeconds() (bindings.cpp; until M-34 the Web runs 2.3.0's
// batches, CooperativePacing); wall time is the test's own clock, the middle
// of each read.

test.skip(!process.env.CALCU59_WEB_PACING, 'Checkpoint: CALCU59_WEB_PACING=1');
test.describe.configure({ timeout: 120000 });

// The bound of AC-1 (one 20 ms slice, LIB-17 AC-15 for the TI-59) plus one
// period of the worker's 60 Hz tick (a read comes between two ticks).
const BOUND = 0.020 + 1 / 60;

async function read(worker) {
  const sent = Date.now();
  const seconds = await worker.evaluate(() => controller.emulatedSeconds());
  const back = Date.now();
  return { seconds, wall: (sent + back) / 2000, roundTrip: (back - sent) / 1000 };
}

test('a TI-59 keeps one minute', async ({ page }, testInfo) => {
  test.skip(testInfo.project.name !== 'root', 'one run is enough');
  const workerSeen = page.waitForEvent('worker');
  await page.goto('./?layout=wide');
  const worker = await workerSeen;
  await expect(page.locator('.calcu-key')).toHaveCount(45, { timeout: 15000 });
  await expect.poll(() => worker.evaluate(() => typeof controller === 'object' && controller !== null)).toBe(true);
  await page.waitForTimeout(1000);   // the ROM reaches its keyboard scan

  const a = await read(worker);
  await page.waitForTimeout(60000);
  const b = await read(worker);
  const emulated = b.seconds - a.seconds, wall = b.wall - a.wall;
  const bound = BOUND + (a.roundTrip + b.roundTrip) / 2;
  const note = `wall ${wall.toFixed(4)} s, emulated ${emulated.toFixed(4)} s, ` +
    `difference ${(emulated - wall).toFixed(4)} s, bound ${bound.toFixed(4)} s`;
  console.log(`[AC-19] ${note}`);
  testInfo.annotations.push({ type: 'AC-19', description: note });
  expect(Math.abs(emulated - wall), note).toBeLessThanOrEqual(bound);
});
