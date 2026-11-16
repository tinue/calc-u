const { test, expect } = require('@playwright/test');

// The web client end to end through the real WASM engine, on dist/index.html
// (PlayCalculator with the physical keyboard; ?layout=wide|portrait) and
// dist/app/. URLs are relative to the project's baseURL — the server root or
// a sub-path (playwright.config.js).

test.describe.configure({ timeout: 30000 });

async function gotoReady(page, query = 'layout=wide') {
  await page.goto(`./?${query}`);
  // The key buttons appear once the worker has posted its static data.
  await expect(page.locator('.calcu-key')).toHaveCount(45, { timeout: 15000 });
  // The calculator boots in real time, as the hardware: a key pressed in its
  // first moments is lost. Give it time to reach the keyboard scan loop.
  await page.waitForTimeout(600);
}

async function pressKey(page, label) {
  await page.getByRole('button', { name: label, exact: true }).click();
  await page.waitForTimeout(200); // the 100 ms minimum hold, then the ROM's reply
}

async function pressKeys(page, labels) {
  for (const l of labels) await pressKey(page, l);
}

// The display is a canvas: compare rendered bitmaps instead of reading text.
// Digits fade in and out (Shared/DisplayPov), so wait for a settled frame.
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

async function showing(page, labels) {
  await pressKey(page, 'CLR');
  await pressKeys(page, labels);
  return settledLed(page);
}

// A point in the artwork, as fractions of the canvas, in page coordinates.
async function facePoint(page, nx, ny) {
  const box = await page.locator('canvas.calcu-keyboard').boundingBox();
  return { x: box.x + nx * box.width, y: box.y + ny * box.height };
}

async function keyRect(page, label) {
  return page.getByRole('button', { name: label, exact: true }).boundingBox();
}

async function uploadPreset(page, text, name = 'test.ti59') {
  await page.getByRole('button', { name: 'Preset' }).click();
  await page.locator('.calcu-dialog input[type=file]').setInputFiles({
    name, mimeType: 'text/plain', buffer: Buffer.from(text),
  });
  await page.waitForTimeout(600);
}

// Lbl A 1 2 3 R/S: pressing A shows 123.
const PROGRAM_123 = 'PROGRAM:\n76 11 01 02 03 91\n';

// ── Start-up ─────────────────────────────────────────────────────────────

test('both layouts load the engine with no console errors', async ({ page }) => {
  const errors = [];
  page.on('pageerror', (err) => errors.push(err.message));
  page.on('console', (msg) => { if (msg.type() === 'error') errors.push(msg.text()); });
  await gotoReady(page, 'layout=wide');
  await expect(page.locator('[data-layout=wide] [data-panel=paper]')).toBeVisible();
  await expect(page.locator('[data-panel=debug]')).toBeVisible();
  await gotoReady(page, 'layout=portrait');
  await expect(page.locator('[data-layout=portrait]')).toBeVisible();
  await expect(page.locator('[data-panel=debug]')).toHaveCount(0);
  expect(errors, `Console/page errors:\n${errors.join('\n')}`).toHaveLength(0);
});

// ── Keys ─────────────────────────────────────────────────────────────────

test('2 + 3 = computes through the real emulation core', async ({ page }) => {
  await gotoReady(page, 'layout=portrait');
  // Results against results: an entry shows no decimal point, a result does.
  const five = await showing(page, ['5', '=']);
  expect(await showing(page, ['2', '+', '3', '='])).toBe(five);
  expect(five).not.toBe(await showing(page, ['6', '=']));
});

test('a press just outside a key hits it (widened hit test)', async ({ page }) => {
  await gotoReady(page);
  const seven = await showing(page, ['7']);
  await pressKey(page, 'CLR');
  const r = await keyRect(page, '7');
  await page.mouse.click(r.x + r.width / 2, r.y - r.height * 0.2);  // within the 35 % expansion
  expect(await settledLed(page)).toBe(seven);
});

test('dragging moves the press to the key under the pointer', async ({ page }) => {
  await gotoReady(page);
  const eight = await showing(page, ['8']);
  const sevenEight = await showing(page, ['7', '8']);
  const seven = await showing(page, ['7']);
  await pressKey(page, 'CLR');
  await settledLed(page);
  const a = await keyRect(page, '7'), b = await keyRect(page, '8');
  await page.mouse.move(a.x + a.width / 2, a.y + a.height / 2);
  await page.mouse.down();
  await page.waitForTimeout(30);
  await page.mouse.move(b.x + b.width / 2, b.y + b.height / 2, { steps: 4 });
  // Hold 8 until the ROM has taken it, not for a fixed time: on a slow runner
  // the scan after 7 can take longer than any fixed hold (CI, 2026-11-15).
  // A third outcome: the ROM took 7 while 8 was already down (7 stays down
  // for its minimum hold of 100 ms), so its key-release gate never saw no key
  // and 8 is not taken while it is held -- the display stays at 7. That is a
  // race of the hold rule, not of the drag; LIB-5 (M-36) owns it.
  const led = page.locator('canvas.calcu-led');
  await expect.poll(async () => [eight, sevenEight].includes(await led.evaluate((c) => c.toDataURL())),
    { timeout: 3000 }).toBe(true).catch(() => {});
  await page.mouse.up();
  // The press moved to 8: 7 is up, and when 8 comes up nothing stays pressed.
  await expect(page.getByRole('button', { name: '7', exact: true })).toHaveAttribute('aria-pressed', 'false');
  await expect(page.getByRole('button', { name: '8', exact: true })).toHaveAttribute('aria-pressed', 'false');
  const shown = await settledLed(page);
  expect([eight, sevenEight, seven]).toContain(shown);
});

test('holding the display runs at full speed', async ({ page }) => {
  await gotoReady(page);
  const p = await facePoint(page, 0.5, 0.075);
  await page.mouse.move(p.x, p.y);
  await page.mouse.down();
  await expect(page.locator('.calcu-face')).toHaveAttribute('data-full-speed', 'true');
  await page.mouse.up();
  await expect(page.locator('.calcu-face')).toHaveAttribute('data-full-speed', 'false');
});

test('the pressed key is shown pressed', async ({ page }) => {
  await gotoReady(page);
  const r = await keyRect(page, '5');
  await page.mouse.move(r.x + r.width / 2, r.y + r.height / 2);
  await page.mouse.down();
  await expect(page.getByRole('button', { name: '5', exact: true })).toHaveAttribute('aria-pressed', 'true');
  await page.mouse.up();
  await expect(page.getByRole('button', { name: '5', exact: true })).toHaveAttribute('aria-pressed', 'false');
});

// ── Physical keyboard (the library's typing with app/TITyping's map, in the worker) ──

async function typeKeys(page, keys) {
  for (const k of keys) {
    await page.keyboard.press(k);
    await page.waitForTimeout(400); // a two-code binding is two taps of 100 ms + the scan-loop wait
  }
}

test('typing 2 + 3 = matches clicking', async ({ page }) => {
  await gotoReady(page);
  const five = await showing(page, ['5', '=']);
  await pressKey(page, 'CLR');
  await page.locator('.calcu-device').focus();
  await typeKeys(page, ['2', '+', '3', '=']);
  expect(await settledLed(page)).toBe(five);
});

test('keystrokes are ignored until the calculator has focus', async ({ page }) => {
  await gotoReady(page);
  const before = await settledLed(page);
  await typeKeys(page, ['2', '+', '2', '=']);
  expect(await settledLed(page)).toBe(before);
});

test('Shift+A sends 2nd before A', async ({ page }) => {
  await gotoReady(page);
  await page.locator('.calcu-device').focus();
  await page.keyboard.press('Shift+A');
  await expect(page.getByRole('button', { name: '2nd', exact: true })).toHaveAttribute('aria-pressed', 'true', { timeout: 300 });
});

test('a letter counts by the letter the key carries, not its position (LIB-5 AC-21, OQ-2)', async ({ page }) => {
  await gotoReady(page);
  await page.locator('.calcu-device').focus();
  // AZERTY: the key at QWERTY's Q carries A.
  await page.locator('.calcu-device').evaluate((el) => {
    el.dispatchEvent(new KeyboardEvent('keydown', { key: 'a', code: 'KeyQ', bubbles: true, cancelable: true }));
  });
  await expect(page.getByRole('button', { name: 'A', exact: true })).toHaveAttribute('aria-pressed', 'true', { timeout: 500 });
  await page.locator('.calcu-device').evaluate((el) => {
    el.dispatchEvent(new KeyboardEvent('keyup', { key: 'a', code: 'KeyQ', bubbles: true, cancelable: true }));
  });
  await expect(page.getByRole('button', { name: 'A', exact: true })).toHaveAttribute('aria-pressed', 'false', { timeout: 500 });
});

test('a held host key stays down until let go; two keys held together are both down (LIB-5 AC-4, AC-23)', async ({ page }) => {
  await gotoReady(page);
  await page.locator('.calcu-device').focus();
  await page.keyboard.down('7');
  await page.waitForTimeout(400);
  await expect(page.getByRole('button', { name: '7', exact: true })).toHaveAttribute('aria-pressed', 'true');
  await page.keyboard.down('1');   // the last one held is drawn pressed
  await expect(page.getByRole('button', { name: '1', exact: true })).toHaveAttribute('aria-pressed', 'true', { timeout: 300 });
  await page.keyboard.up('1');
  await expect(page.getByRole('button', { name: '7', exact: true })).toHaveAttribute('aria-pressed', 'true', { timeout: 300 });
  await page.keyboard.up('7');
  await expect(page.getByRole('button', { name: '7', exact: true })).toHaveAttribute('aria-pressed', 'false', { timeout: 300 });
});

test('a click shorter than the debounce still types the key (LIB-5 AC-24)', async ({ page }) => {
  await gotoReady(page);
  const five = await showing(page, ['5']);
  await pressKey(page, 'CLR');
  const button = page.getByRole('button', { name: '5', exact: true });
  const box = await button.boundingBox();
  await page.mouse.move(box.x + box.width / 2, box.y + box.height / 2);
  await page.mouse.down();
  await page.mouse.up();   // at once: the engine holds it its 100 ms of emulated time
  expect(await settledLed(page)).toBe(five);
});

// ── Models, modules, cue cards ───────────────────────────────────────────

test('2nd Pgm 01 shows the Master Library diagnostic cue card', async ({ page }) => {
  await gotoReady(page);
  await pressKeys(page, ['2nd', 'LRN', '0', '1']);  // LRN's 2nd function is Pgm
  await expect(page.locator('.cuecard')).toHaveAttribute('aria-label', /MASTER LIBRARY DIAGNOSTIC/);
});

test('switching models: TI-58C has no card reader and keeps its constant memory', async ({ page }) => {
  await gotoReady(page);
  await expect(page.getByRole('button', { name: 'Crd' }).first()).toBeVisible();
  await page.getByLabel('Calculator model').selectOption({ label: 'TI-58C' });
  await expect(page.getByLabel('Calculator model')).toHaveValue('2');
  await expect(page.getByRole('button', { name: 'Crd' })).toHaveCount(0);
  await page.waitForTimeout(1500);  // the TI-58C boots
  const seven = await showing(page, ['7', '=']);
  await pressKeys(page, ['STO', '1', '2', 'CLR']);
  await page.waitForTimeout(1000);  // the 0.5 s persist debounce

  // The startup model is the last used; the constant memory survives.
  await gotoReady(page);
  await expect(page.getByLabel('Calculator model')).toHaveValue('2');
  await page.waitForTimeout(1500);
  expect(await showing(page, ['RCL', '1', '2'])).toBe(seven);
});

test('Settings: the startup model is kept', async ({ page }) => {
  await gotoReady(page);
  await page.getByRole('button', { name: 'Settings' }).click();
  await page.getByLabel('Startup Calculator').selectOption({ label: 'TI-58' });
  await page.getByRole('button', { name: 'Done' }).click();
  await gotoReady(page);
  await expect(page.getByLabel('Calculator model')).toHaveValue('1');
});

// LIB-9 AC-21, AC-25 (design T-28): the settings are Calc-U-59.ini, since
// M-34 a file of the persistent file system (LIB-9 AC-23; storage.spec.js
// reads it there); 2.3.0's entries in the page's storage are not read, and a reload
// keeps a change.
test('lib9_ac21_settings_in_the_ini_file_survive_a_reload', async ({ page }) => {
  await page.goto('./?layout=wide');
  await page.evaluate(() => {
    window.localStorage.clear();
    window.localStorage.setItem('calcu59/startupModel', '2');   // 2.3.0: TI-58C
    window.localStorage.setItem('calcu59/lastUsedModel', '2');
  });
  await gotoReady(page);
  await expect(page.getByLabel('Calculator model')).toHaveValue('0');   // the defaults: TI-59
  await page.getByRole('button', { name: 'Settings' }).click();
  await page.getByLabel('Startup Calculator').selectOption({ label: 'TI-58C' });
  await page.getByRole('button', { name: 'Done' }).click();
  await page.waitForTimeout(1000);   // kept within a second (LIB-15 AC-6)
  await gotoReady(page);
  await expect(page.getByLabel('Calculator model')).toHaveValue('2');
});

// ── Presets ──────────────────────────────────────────────────────────────

test('a bundled preset shows its own cue card', async ({ page }) => {
  await gotoReady(page);
  await page.getByRole('button', { name: 'Preset' }).click();
  await page.getByRole('button', { name: /Sum of N numbers/ }).click();
  await expect(page.locator('.cuecard')).toHaveAttribute('aria-label', /Sum 1\.\.N/, { timeout: 5000 });
});

test('an uploaded preset loads its program', async ({ page }) => {
  await gotoReady(page);
  const expected = await showing(page, ['1', '2', '3']);
  await uploadPreset(page, PROGRAM_123);
  await pressKey(page, 'A');
  expect(await settledLed(page)).toBe(expected);
});

test('a preset dropped on the calculator loads', async ({ page }) => {
  await gotoReady(page);
  const expected = await showing(page, ['1', '2', '3']);
  await pressKey(page, 'CLR');
  await dropFile(page, 'drop.ti59', PROGRAM_123);
  await page.waitForTimeout(800);
  await pressKey(page, 'A');
  expect(await settledLed(page)).toBe(expected);
});

// ── Magnetic cards (session folder) ──────────────────────────────────────

async function dropFile(page, name, text) {
  const dt = await page.evaluateHandle(([n, t]) => {
    const d = new DataTransfer();
    d.items.add(new File([t], n));
    return d;
  }, [name, text]);
  await page.locator('.calcu-face').dispatchEvent('dragover', { dataTransfer: dt });
  await page.locator('.calcu-face').dispatchEvent('drop', { dataTransfer: dt });
}

/// Writes PROGRAM_123 to a blank session card and returns the card's text.
async function writtenCardText(page) {
  await uploadPreset(page, PROGRAM_123 + 'KEYSTROKES:\n25 82 21 91\n');
  await page.waitForTimeout(1000);
  await page.getByRole('button', { name: 'Crd' }).nth(1).click();
  await page.getByLabel('Card name').fill('prog');
  await page.getByRole('button', { name: 'Fit', exact: true }).click();
  await page.getByRole('button', { name: 'Feed card side 1' }).click();
  await expect(page.getByRole('button', { name: 'Eject Card' })).toHaveCount(0, { timeout: 15000 });
  await page.getByRole('button', { name: 'Crd' }).first().click();
  const download = page.waitForEvent('download');
  await page.getByRole('button', { name: 'Download prog.U59' }).click();
  const stream = await (await download).createReadStream();
  const chunks = [];
  for await (const c of stream) chunks.push(c);
  await page.getByRole('button', { name: 'Cancel' }).click();
  return Buffer.concat(chunks).toString();
}

test('a card dropped on the TI-59 is fitted, and read when a corner is pressed', async ({ page }) => {
  await gotoReady(page);
  const expected = await showing(page, ['1', '2', '3']);
  await pressKey(page, 'CLR');
  const card = await writtenCardText(page);
  await uploadPreset(page, 'PARTITION: 479\n');  // a fresh machine: no program
  await dropFile(page, 'dropped.U59', card);
  await page.getByRole('button', { name: 'Feed card side 1' }).click();
  await expect(page.getByRole('button', { name: 'Eject Card' })).toHaveCount(0, { timeout: 15000 });
  await page.waitForTimeout(3000);
  await pressKey(page, 'CLR');
  await pressKey(page, 'A');
  expect(await settledLed(page)).toBe(expected);
});

test('a card dropped on the TI-58 is ignored', async ({ page }) => {
  await gotoReady(page);
  const card = await writtenCardText(page);
  await page.getByLabel('Calculator model').selectOption({ label: 'TI-58' });
  await page.waitForTimeout(600);
  await dropFile(page, 'dropped.U59', card);
  await page.waitForTimeout(600);
  await expect(page.locator('.calcu-status')).toHaveCount(0);
  await expect(page.getByRole('button', { name: 'Eject Card' })).toHaveCount(0);
});

test('a dropped file that is neither preset nor card is refused', async ({ page }) => {
  await gotoReady(page);
  await dropFile(page, 'notes.txt', 'hello');
  await expect(page.locator('.calcu-status')).toContainText('notes.txt: not a');
});

test('a blank card is written, listed, and read back', async ({ page }) => {
  await gotoReady(page);
  await uploadPreset(page, PROGRAM_123);
  const expected = await (async () => { await pressKey(page, 'A'); return settledLed(page); })();

  // The same program again, then CLR 1 2nd Write: waits for a card.
  await uploadPreset(page, PROGRAM_123 + 'KEYSTROKES:\n25 82 21 91\n');
  await page.waitForTimeout(1000);
  await page.getByRole('button', { name: 'Crd' }).nth(1).click();  // a blank card passes: written
  await page.getByLabel('Card name').fill('prog');
  await page.getByRole('button', { name: 'Fit', exact: true }).click();
  await expect(page.getByRole('button', { name: 'Eject Card' })).toHaveCount(0);  // fitted, not fed
  await page.getByRole('button', { name: 'Feed card side 1' }).click();
  // In the reader until the write is done, then ejected (and saved).
  await expect(page.getByRole('button', { name: 'Eject Card' })).toBeVisible();
  await expect(page.getByRole('button', { name: 'Eject Card' })).toHaveCount(0, { timeout: 15000 });

  await uploadPreset(page, 'PARTITION: 479\n');  // a fresh machine: no program
  await page.getByRole('button', { name: 'Crd' }).first().click();  // read
  await page.getByRole('button', { name: 'prog.U59', exact: true }).click();
  await page.getByRole('button', { name: 'Fit', exact: true }).click();
  await page.getByRole('button', { name: 'Feed card side 1' }).click();
  await page.waitForTimeout(3000);
  await pressKey(page, 'CLR');
  await pressKey(page, 'A');
  expect(await settledLed(page)).toBe(expected);
});

test('the fresh card has its file name as title, and an existing name asks first', async ({ page }) => {
  await gotoReady(page);
  await writtenCardText(page);  // prog.U59 exists now
  await page.getByRole('button', { name: 'Crd' }).nth(1).click();
  await page.getByLabel('Card name').fill('prog');
  await page.getByRole('button', { name: 'Fit', exact: true }).click();
  await expect(page.getByRole('alert')).toContainText('Replace card prog.U59?');
  await page.getByRole('button', { name: 'Cancel' }).click();
  await page.getByRole('button', { name: 'Crd' }).nth(1).click();
  await page.getByLabel('Card name').fill('Other');
  await page.getByRole('button', { name: 'Fit', exact: true }).click();
  await expect(page.locator('.cuecard')).toHaveAttribute('aria-label', /^Other/);
});

// ── Printer ──────────────────────────────────────────────────────────────

// The PC-100C's paper in the component set's paper panel (M-53; ADR 0032).
test('LIB-8 AC-31 the PC-100C prints a number, the text view shows it, Cut empties the paper and is disabled', async ({ page }) => {
  await gotoReady(page);
  const printer = page.locator('[data-panel=paper]');
  // Connected from the start, as in the apps.
  await expect(printer.getByRole('button', { name: /PC-100C/ })).toHaveAttribute('aria-pressed', 'true');
  await expect(printer.locator('[data-calcu=paper-owner]')).toHaveText('PC-100C');
  await pressKeys(page, ['4', '2', '2nd', '+/-']);  // 42 Prt
  const cut = printer.locator('[data-calcu=paper-cut]');
  await expect(cut).toBeEnabled({ timeout: 5000 });
  await printer.locator('[data-calcu=paper-view]').click();
  await expect(printer.locator('[data-calcu=paper-text]')).toContainText('42.');
  await page.context().grantPermissions(['clipboard-read', 'clipboard-write']).catch(() => {});
  await cut.click();
  await expect(cut).toBeDisabled();
  await expect(printer.locator('[data-calcu=paper-text]')).toHaveText('');
});

test('LIB-8 AC-28 PRINT and ADV are held, TRACE is a switch, the view control shows the view that is on', async ({ page }) => {
  await gotoReady(page);
  const printer = page.locator('[data-panel=paper]');
  const control = (id) => printer.locator(`[data-calcu=paper-control-${id}]`);
  await expect(control('PRINT')).toBeVisible();
  // PRINT: down while held, up on release; it prints the display.
  await control('PRINT').dispatchEvent('pointerdown', { pointerId: 1 });
  await expect(control('PRINT')).toHaveAttribute('aria-pressed', 'true');
  await page.waitForTimeout(150);
  await control('PRINT').dispatchEvent('pointerup', { pointerId: 1 });
  await expect(control('PRINT')).toHaveAttribute('aria-pressed', 'false');
  await expect(printer.locator('[data-calcu=paper-cut]')).toBeEnabled({ timeout: 5000 });
  // ADV is held too.
  await control('ADV').dispatchEvent('pointerdown', { pointerId: 2 });
  await expect(control('ADV')).toHaveAttribute('aria-pressed', 'true');
  await control('ADV').dispatchEvent('pointerup', { pointerId: 2 });
  await expect(control('ADV')).toHaveAttribute('aria-pressed', 'false');
  // TRACE latches: on, then off.
  await expect(control('TRACE')).toHaveAttribute('aria-pressed', 'false');
  await control('TRACE').click();
  await expect(control('TRACE')).toHaveAttribute('aria-pressed', 'true');
  await control('TRACE').click();
  await expect(control('TRACE')).toHaveAttribute('aria-pressed', 'false');
  // The view control names the view that is on (row 59-46).
  const view = printer.locator('[data-calcu=paper-view]');
  await expect(view).toHaveText('Marks');
  await view.click();
  await expect(view).toHaveText('Text');
  await expect(printer.locator('[data-calcu=paper-text]')).toBeVisible();
  // Detached: the panel shows an empty paper without a name (AC-29).
  await printer.getByRole('button', { name: /PC-100C/ }).click();
  await expect(printer.locator('[data-calcu=paper-owner]')).toHaveText('');
  await expect(control('PRINT')).toHaveCount(0);
});

test('the focus outline shows for the keyboard only, not after a tap', async ({ page }) => {
  await gotoReady(page, 'layout=portrait');
  const device = page.locator('.calcu-device');
  const outline = () => device.evaluate((e) => getComputedStyle(e).outlineStyle);

  // Reached by the keyboard (the demo page's links come first): outlined.
  for (let i = 0; i < 4; i++) await page.keyboard.press('Tab');
  await expect(device).toBeFocused();
  expect(await outline()).toBe('solid');
  await device.evaluate((e) => e.blur());

  // A tap after a key press focuses it from script, which WebKit (and Chromium,
  // after a key press) count as focus-visible: not outlined. A synthetic event
  // keeps the browser's input modality at "keyboard".
  await page.keyboard.press('Shift');
  await page.locator('.calcu-face').evaluate((e) => {
    e.dispatchEvent(new PointerEvent('pointerdown', { bubbles: true, pointerId: 1, pointerType: 'touch' }));
  });
  await expect(device).toBeFocused();
  expect(await outline()).toBe('none');
});

test('portrait: the chevron switches to the printer page and back', async ({ page }) => {
  await gotoReady(page, 'layout=portrait');
  await expect(page.locator('[data-panel=paper]')).toHaveCount(0);
  await page.getByRole('button', { name: 'Printer' }).click();
  await expect(page.locator('[data-panel=paper]')).toBeVisible();
  await page.getByRole('button', { name: 'Calculator' }).click();
  await expect(page.locator('canvas.calcu-keyboard')).toBeVisible();
});

// ── Debugger (wide) ──────────────────────────────────────────────────────

test('FREEZE, STEP and RESUME on the CALCULATOR tab', async ({ page }) => {
  await gotoReady(page);
  const debug = page.locator('[data-panel=debug]');
  await debug.getByRole('button', { name: 'FREEZE', exact: true }).click();
  await expect(debug.getByRole('button', { name: 'RESUME' })).toBeEnabled();
  await expect(debug.getByText(/← next/)).toBeVisible();
  await debug.getByRole('button', { name: 'RESUME' }).click();
  await expect(debug.getByRole('button', { name: 'RESUME' })).toBeDisabled();
});

test('LIB-10 AC-34 FREEZE STEP RESUME of both tabs follow the run state', async ({ page }) => {
  await gotoReady(page);
  const debug = page.locator('[data-panel=debug]');
  const tab = (name) => debug.getByRole('tab', { name, exact: true });
  const button = (name) => debug.locator(`[data-calcu=run-control-${name}]`);
  for (const [name, level, other] of [['CALCULATOR', 'program', 'CPU'], ['CPU', 'cpu', 'CALCULATOR']]) {
    await tab(name).click();
    await expect(debug.locator(`[data-calcu=run-control][data-level=${level}]`)).toBeVisible();
    // Running: only FREEZE can be used.
    await expect(button('pause')).toBeEnabled();
    await expect(button('step')).toBeDisabled();
    await expect(button('resume')).toBeDisabled();
    await button('pause').click();
    // Paused by this tab's level: STEP and RESUME, no FREEZE.
    await expect(button('resume')).toBeEnabled({ timeout: 5000 });
    await expect(button('step')).toBeEnabled();
    await expect(button('pause')).toBeDisabled();
    await button('step').dispatchEvent('pointerdown', { pointerId: 3 });
    await button('step').dispatchEvent('pointerup', { pointerId: 3 });
    await expect(button('resume')).toBeEnabled({ timeout: 5000 });
    // The other tab's buttons: the pause is not its level's.
    await tab(other).click();
    await expect(button('step')).toBeDisabled();
    await expect(button('resume')).toBeDisabled();
    await expect(button('pause')).toBeDisabled();
    await tab(name).click();
    await button('resume').click();
    await expect(button('pause')).toBeEnabled();
    await expect(button('resume')).toBeDisabled();
  }
});

test('LIB-10 AC-26 a trace is downloaded when switched off', async ({ page }) => {
  await gotoReady(page);
  const debug = page.locator('[data-panel=debug]');
  await debug.getByRole('tab', { name: 'LOG' }).click();
  await debug.getByRole('button', { name: /^TRACE/ }).click();
  await pressKeys(page, ['1', '+', '1', '=']);
  const download = page.waitForEvent('download');
  await debug.getByRole('button', { name: /^TRACE/ }).click();
  const d = await download;
  expect(d.suggestedFilename()).toBe('CALCU59_TRACE.bin');
  expect((await (await d.createReadStream()).toArray()).reduce((n, b) => n + b.length, 0)).toBeGreaterThan(1000);
});

// ── The standalone app (app/) ────────────────────────────────────────────

test('the standalone app follows the window as it rotates', async ({ page }) => {
  // iPhone: portrait pages; in landscape calculator and printer.
  await page.setViewportSize({ width: 440, height: 956 });
  await page.goto('app/');
  await expect(page.locator('.calcu-key')).toHaveCount(45, { timeout: 15000 });
  await expect(page.locator('[data-layout=portrait]')).toBeVisible();

  await page.setViewportSize({ width: 832, height: 419 });
  await expect(page.locator('[data-layout=wide] [data-panel=paper]')).toBeVisible();
  // Fits the window without clipping.
  expect((await page.locator('.calcu-device').boundingBox()).width).toBeLessThanOrEqual(832);
  await expect(page.locator('[data-panel=debug]')).toHaveCount(0);
  await expect(page.locator('.calcu-key')).toHaveCount(45);
  // Everything stays inside the short window, the bar without its labels.
  const bar = await page.locator('.calcu-bar').boundingBox();
  expect(bar.y + bar.height).toBeLessThanOrEqual(419);
  await expect(page.locator('.calcu-bar').getByText('Preset')).toHaveCount(0);

  // iPhone Duo open: the narrow tape's button bar fits it.
  await page.setViewportSize({ width: 880, height: 645 });
  await expect(page.locator('[data-panel=debug]')).toBeVisible();
  // The paper panel's header wraps in the narrow tape: nothing is cut off.
  const paperHeader = page.locator('[data-panel=paper] .calcu-paper-header');
  await expect(paperHeader.locator('[data-calcu=paper-control-PRINT]')).toBeVisible();
  expect(await paperHeader.evaluate((e) => e.scrollWidth <= e.clientWidth)).toBe(true);

  // iPad mini: all three panels, icons only; iPad Pro: with labels.
  await page.setViewportSize({ width: 1133, height: 744 });
  await expect(page.locator('[data-panel=debug]')).toBeVisible();
  await expect(page.locator('.calcu-bar').getByText('Preset')).toHaveCount(0);
  await page.setViewportSize({ width: 1366, height: 1000 });
  await expect(page.locator('[data-panel=debug]')).toBeVisible();
  await expect(page.locator('.calcu-bar').getByText('Preset')).toBeVisible();

  // Back to portrait: pages again.
  await page.setViewportSize({ width: 744, height: 1133 });
  await expect(page.locator('[data-layout=portrait]')).toBeVisible();
  await expect(page.locator('[data-panel=debug]')).toHaveCount(0);
});

test('the standalone app precaches all three models', async ({ page }) => {
  await page.goto('app/');
  await expect(page.locator('.calcu-key')).toHaveCount(45, { timeout: 15000 });

  // cache.addAll() rejects if any precached path is wrong, and the worker
  // then never activates — so an active worker plus a full cache means every
  // relative path resolved under this base URL.
  const cached = await page.evaluate(async () => {
    const reg = await navigator.serviceWorker.ready;
    const names = await caches.keys();
    const name = names.find((n) => n.startsWith(`calcu59-app ${reg.scope} `));
    return name ? (await (await caches.open(name)).keys()).map((r) => r.url) : [];
  });
  for (const f of ['../calc.js', '../wasm/ti59-core.wasm', '../img/ti58_base.png', '../bundle/CD2400.txt']) {
    expect(cached).toContain(new URL(f, page.url()).href);
  }
});

test('the installed app loads any module and any preset offline', async ({ page, context }) => {
  await page.goto('app/');
  await expect(page.locator('.calcu-key')).toHaveCount(45, { timeout: 15000 });
  await page.evaluate(() => navigator.serviceWorker.ready);
  await page.reload();  // now under the service worker
  await expect(page.locator('.calcu-key')).toHaveCount(45, { timeout: 15000 });
  await context.setOffline(true);

  await page.getByRole('button', { name: 'Settings' }).click();
  await page.getByLabel('Solid State Module').selectOption({ label: 'Applied Statistics' });
  await page.getByRole('button', { name: 'Done' }).click();
  await page.waitForTimeout(1000);
  // A module whose image can't be read is not selected: the dialog would still show ML.
  await page.getByRole('button', { name: 'Settings' }).click();
  await expect(page.getByLabel('Solid State Module')).toHaveValue('ST');
  await page.getByRole('button', { name: 'Done' }).click();

  await page.getByRole('button', { name: 'Preset' }).click();
  await page.getByRole('button', { name: /Sum of N numbers/ }).click();
  await expect(page.locator('.cuecard')).toHaveAttribute('aria-label', /Sum 1\.\.N/, { timeout: 5000 });
});
