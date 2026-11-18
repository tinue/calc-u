const { test, expect } = require('@playwright/test');
const fs = require('fs');
const path = require('path');
const { open, send } = require('./helpers');

// The run-control buttons of the React
// component set, CalcuRunControlButtons, on the test product's session with
// the library's DebuggerPart and the test processor; an ended trace
// handed over as a download. Chromium and WebKit.

const CASES = JSON.parse(fs.readFileSync(path.join(__dirname, '..', '..', 'debugger', 'tests', 'button_cases.json'), 'utf8'));
const el = (page, name) => page.locator(`[data-calcu="${name}"]`);
const runState = (page) => page.evaluate(() => window.calcuTestShells[0].getState().topics['run-state']);

test('react buttons follow the run state', async ({ page }) => {
  await open(page, 'Test-A', 'width=480&height=900');
  // The rule's table, the same as the C++ rule's and the Qt6 widget's.
  for (const c of CASES.cases) {
    const use = await page.evaluate(([level, state]) => window.CalcUTest.buttonUse(level, state), [c.level, c.state]);
    expect(use).toEqual({ pause: c.pause, step: c.step, resume: c.resume });
  }
  // On the session: the labels are the product's.
  await expect(el(page, 'run-control-pause')).toHaveText('FREEZE');
  await expect(el(page, 'run-control-pause')).toBeEnabled();
  await expect(el(page, 'run-control-step')).toBeDisabled();
  await expect(el(page, 'run-control-resume')).toBeDisabled();
  await el(page, 'run-control-pause').click();
  await expect.poll(async () => (await runState(page)).paused).toBe(true);
  await expect(el(page, 'run-control-pause')).toBeDisabled();
  await expect(el(page, 'run-control-step')).toBeEnabled();
  await expect(el(page, 'run-control-resume')).toBeEnabled();

  // Step: once on press, then four times a second while held.
  await page.evaluate(() => {
    const shell = window.calcuTestShells[0];
    window.calcuSteps = 0;
    const send = shell.send.bind(shell);
    shell.send = (n, a) => {
      if (n === 'step') window.calcuSteps += 1;
      return send(n, a);
    };
  });
  const box = await el(page, 'run-control-step').boundingBox();
  await page.mouse.move(box.x + box.width / 2, box.y + box.height / 2);
  await page.mouse.down();
  await page.waitForTimeout(620);
  await page.mouse.up();
  const held = await page.evaluate(() => window.calcuSteps);
  expect(held).toBeGreaterThanOrEqual(3);
  expect(held).toBeLessThanOrEqual(4);
  await page.waitForTimeout(400);
  expect(await page.evaluate(() => window.calcuSteps)).toBe(held);   // released: no more
  await expect.poll(async () => (await runState(page)).reason).toBe('step');

  await el(page, 'run-control-resume').click();
  await expect.poll(async () => (await runState(page)).paused).toBe(false);
  await expect(el(page, 'run-control-pause')).toBeEnabled();
});

test('an ended trace is a download', async ({ page }) => {
  await open(page, 'Test-A', 'width=480&height=900');
  expect((await send(page, 'trace', { file: 'web.bin' })).done).toBe(true);
  await expect.poll(async () => {
    const t = await page.evaluate(() => window.calcuTestShells[0].getState().topics.trace);
    return t && t.running;
  }).toBe(true);
  await page.waitForTimeout(200);
  const download = page.waitForEvent('download');
  expect((await send(page, 'trace', { off: true })).done).toBe(true);
  const d = await download;
  expect(d.suggestedFilename()).toBe('web.bin');
  const file = await d.path();
  const bytes = fs.readFileSync(file);
  expect(bytes.subarray(0, 4).toString()).toBe('TTRC');
  expect(bytes[bytes.length - 1]).toBe(3);
  expect((bytes.length - 6) % 3).toBe(0);
  expect(bytes.length).toBeGreaterThan(6);
});
