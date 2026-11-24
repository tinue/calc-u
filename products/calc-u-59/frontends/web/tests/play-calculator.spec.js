const { test, expect } = require('@playwright/test');
const { gotoReady } = require('./helpers');

// The web client end to end through the real WASM engine, on dist/index.html
// (CalcU59 with the physical keyboard; ?layout=wide|portrait) and
// dist/app/. URLs are relative to the project's baseURL — the server root or
// a sub-path (playwright.config.js).

test.describe.configure({ timeout: 30000 });

async function pressKey(page, label) {
  await page.getByRole('button', { name: label, exact: true }).click();
  await page.waitForTimeout(200); // the 100 ms minimum hold, then the ROM's reply
}

async function pressKeys(page, labels) {
  for (const l of labels) await pressKey(page, l);
}

// The display is a canvas: compare rendered bitmaps instead of reading text.
// Digits fade in and out (DisplayPov in app/FrameSampler), so wait for a settled frame.
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

// The largest difference of any colour channel of two LED pictures; 255 for
// pictures of different sizes. The glow is drawn with a blur, which may
// differ by a step or two between two drawings of the same display.
async function ledDistance(page, a, b) {
  return page.evaluate(async ([a, b]) => {
    const load = (url) => new Promise((done) => { const i = new Image(); i.onload = () => done(i); i.src = url; });
    const [ia, ib] = await Promise.all([load(a), load(b)]);
    if (ia.width !== ib.width || ia.height !== ib.height) return 255;
    const c = document.createElement('canvas');
    c.width = ia.width;
    c.height = ia.height;
    const ctx = c.getContext('2d');
    const pixels = (img) => { ctx.clearRect(0, 0, c.width, c.height); ctx.drawImage(img, 0, 0); return ctx.getImageData(0, 0, c.width, c.height).data; };
    const da = pixels(ia), db = pixels(ib);
    let most = 0;
    for (let k = 0; k < da.length; k++) most = Math.max(most, Math.abs(da[k] - db[k]));
    return most;
  }, [a, b]);
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

// The shell's elements by their data-calcu names.
const el = (page, name) => page.locator(`[data-calcu="${name}"]`);

/// A File menu item (wide: the menu bar; portrait: behind the More button).
async function fileItem(page, ...path) {
  if (await el(page, 'menu-bar').count()) {
    await el(page, 'menu-file').click();
    for (const id of path) await el(page, `item-${id}`).click();
  } else {
    await el(page, 'bar-more').click();
    for (const id of path) await el(page, `more-item-${id}`).click();
  }
}

/// File ▸ Load Preset… (More ▸ Load Preset… without a menu bar) with an uploaded file.
async function uploadPreset(page, text, name = 'test.ti59') {
  const chooser = page.waitForEvent('filechooser');
  if (await el(page, 'menu-bar').count()) await fileItem(page, 'load-preset');
  else { await el(page, 'bar-more').click(); await el(page, 'more-item-load-preset').click(); }
  await (await chooser).setFiles({ name, mimeType: 'text/plain', buffer: Buffer.from(text) });
  await page.waitForTimeout(600);
}

/// The shell's settings dialog, a row changed, closed.
async function setSetting(page, section, key, label) {
  await fileItem(page, 'settings');
  await el(page, `setting-${section}-${key}`).selectOption({ label });
  await el(page, 'settings-close').click();
}

const modelPicker = (page) => el(page, 'bar-model');

// A preset of format 2 (docs/formats/preset.md) on a TI-59 with the Master
// Library module and the PC-100C, with `body` after its header.
const preset = (body) => 'format-version: 2\nmodel: TI-59\nslot-1: { name: ML }\nprinter: PC-100C\n' + body;
// Lbl A 1 2 3 R/S: pressing A shows 123.
const PROGRAM_123 = preset('program:\n  keycodes: |\n    76 11 01 02 03 91\n');
// The same, then CLR 1 2nd Write at regular speed: the machine waits for a card.
const PROGRAM_123_WRITE = PROGRAM_123 + 'keys:\n  - regular-speed:\n  - key: CLR 1 Write\n';
// A fresh machine: no program.
const FRESH = preset('partition: 479\n');

// The PC-100C attached (the built-in default has no printer) by
// its button "PC-100C" on the paper panel.
async function connectPrinter(page) {
  const toggle = el(page, 'bar-place-printer');
  if ((await toggle.getAttribute('aria-pressed')) !== 'true') await toggle.click();
  await expect(toggle).toHaveAttribute('aria-pressed', 'true', { timeout: 5000 });
}

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
  expect(await ledDistance(page, five, await showing(page, ['2', '+', '3', '=']))).toBeLessThanOrEqual(4);
  expect(await ledDistance(page, five, await showing(page, ['6', '=']))).toBeGreaterThan(64);
});

test('a press just outside a key hits it (widened hit test)', async ({ page }) => {
  await gotoReady(page);
  const seven = await showing(page, ['7']);
  await pressKey(page, 'CLR');
  const r = await keyRect(page, '7');
  await page.mouse.click(r.x + r.width / 2, r.y - r.height * 0.2);  // within the 35 % expansion
  expect(await settledLed(page)).toBe(seven);
});

// The faceplate's pointer is the shell's: the
// key is the one under the pointer when it went down; a drag keeps it.
test('a drag keeps the key pressed where the pointer went down', async ({ page }) => {
  await gotoReady(page);
  const seven = await showing(page, ['7']);
  await pressKey(page, 'CLR');
  await settledLed(page);
  const a = await keyRect(page, '7'), b = await keyRect(page, '8');
  await page.mouse.move(a.x + a.width / 2, a.y + a.height / 2);
  await page.mouse.down();
  await page.waitForTimeout(30);
  await page.mouse.move(b.x + b.width / 2, b.y + b.height / 2, { steps: 4 });
  await expect(page.getByRole('button', { name: '7', exact: true })).toHaveAttribute('aria-pressed', 'true');
  await expect(page.getByRole('button', { name: '8', exact: true })).toHaveAttribute('aria-pressed', 'false');
  await page.mouse.up();
  await expect(page.getByRole('button', { name: '7', exact: true })).toHaveAttribute('aria-pressed', 'false');
  expect(await settledLed(page)).toBe(seven);
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

test('a letter counts by the letter the key carries, not its position', async ({ page }) => {
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

test('a held host key stays down until let go; two keys held together are both down', async ({ page }) => {
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

test('a click shorter than the debounce still types the key', async ({ page }) => {
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

test('switching models: TI-58C has no card reader', async ({ page }) => {
  await gotoReady(page);
  await expect(page.getByRole('button', { name: 'Arm Card', exact: true })).toBeVisible();
  await modelPicker(page).selectOption({ label: '58C' });
  await expect(modelPicker(page)).toHaveValue('2');
  await expect(page.getByRole('button', { name: /Arm .*Card/ })).toHaveCount(0);
});

test('Settings: the startup model is kept', async ({ page }) => {
  await gotoReady(page);
  await setSetting(page, 'startup', 'model', 'TI-58');
  await page.waitForTimeout(1000);
  await gotoReady(page);
  await expect(modelPicker(page)).toHaveValue('1');
});

// The settings are Calc-U-59.ini, a file of the persistent file system
// (storage.spec.js reads it there); a reload keeps a change.
test('settings_in_the_ini_file_survive_a_reload', async ({ page }) => {
  await gotoReady(page);
  await expect(modelPicker(page)).toHaveValue('0');   // the defaults: TI-59
  await setSetting(page, 'startup', 'model', 'TI-58C');
  await page.waitForTimeout(1000);   // kept within a second
  await gotoReady(page);
  await expect(modelPicker(page)).toHaveValue('2');
});

// ── Presets ──────────────────────────────────────────────────────────────

// A bundled example, loaded by its bundle path, fits its cue card.
test('bundled example sum shows its cue card', async ({ page }) => {
  await gotoReady(page);
  await fileItem(page, 'examples', 'examples-Programs', 'example-examples:sum.ti59');   // File ▸ Examples ▸
  await expect(page.locator('.cuecard')).toHaveAttribute('aria-label', /Sum 1\.\.N/, { timeout: 5000 });
});

test('uploaded preset fits the cue card of its content', async ({ page }) => {
  await gotoReady(page);
  const expected = await showing(page, ['1', '2', '3']);
  await uploadPreset(page, PROGRAM_123);
  await pressKey(page, 'A');
  expect(await settledLed(page)).toBe(expected);
});

test('a dropped preset loads like one from the dialog', async ({ page }) => {
  await gotoReady(page);
  const expected = await showing(page, ['1', '2', '3']);
  await pressKey(page, 'CLR');
  await dropFile(page, 'drop.ti59', PROGRAM_123);
  await page.waitForTimeout(800);
  await pressKey(page, 'A');
  expect(await settledLed(page)).toBe(expected);
});

// ── Magnetic cards (the library's media part) ───────────────────────────

async function dropFile(page, name, text) {
  await dropFiles(page, [[name, text]]);
}

// Several files in one drop: [[name, text], …].
async function dropFiles(page, files) {
  const dt = await page.evaluateHandle((fs) => {
    const d = new DataTransfer();
    for (const [n, t] of fs) d.items.add(new File([t], n));
    return d;
  }, files);
  await page.locator('.calcu-face').dispatchEvent('dragover', { dataTransfer: dt });
  await page.locator('.calcu-face').dispatchEvent('drop', { dataTransfer: dt });
}

// A card file of magnetic card format 3 without a written side (docs/formats/media.md §1).
const cardFile = (name) =>
  `format: ti59-magnetic-card\nformat-version: 3\ncard-name: "${name}"\ncue-card:\n  title: ${name}\nsides: {}\n`;

const ARM = { name: 'Arm Card', exact: true };
const ARM_FRESH = { name: 'Arm Fresh Card', exact: true };

/// Arm Fresh Card with `name`, then side 0 fed by its corner while the
/// program waits in `1 2nd Write`: the card written into the save folder.
async function writeFreshCard(page, name = 'prog') {
  await uploadPreset(page, PROGRAM_123_WRITE);
  await page.waitForTimeout(1000);
  await page.getByRole('button', ARM_FRESH).click();
  await page.getByLabel('Card name').fill(name);
  await page.getByRole('button', { name: 'Arm', exact: true }).click();
  await expect(page.getByRole('button', { name: 'Eject Card' })).toHaveCount(0);   // fitted, not fed
  await page.getByRole('button', { name: 'Feed card side 1' }).click();
  // In the reader until the write is done, then given back (and saved).
  await expect(page.getByRole('button', { name: 'Eject Card' })).toBeVisible();
  await expect(page.getByRole('button', { name: 'Eject Card' })).toHaveCount(0, { timeout: 15000 });
}

/// The saved cards Arm Card lists, by name.
async function listedCards(page) {
  await page.getByRole('button', ARM).click();
  const names = await page.getByRole('list', { name: 'Saved cards' }).locator('button').allTextContents();
  await page.getByRole('button', { name: 'Cancel' }).click();
  return names;
}

/// A card file imported through File ▸ Import Media….
async function importCard(page, fileName, text) {
  const chooser = page.waitForEvent('filechooser');
  await fileItem(page, 'import-media');
  await (await chooser).setFiles({ name: fileName, mimeType: 'application/yaml', buffer: Buffer.from(text) });
  await page.waitForTimeout(500);
}

// Two "Crd" buttons, Arm Card and Arm Fresh Card, while no card
// passes; Eject Card in their place while one does.
test('card buttons arm card arm fresh card and eject', async ({ page }) => {
  await gotoReady(page);
  await expect(page.getByRole('button', ARM)).toHaveText(/Crd/);
  await expect(page.getByRole('button', ARM_FRESH)).toHaveText(/Crd/);
  await uploadPreset(page, PROGRAM_123_WRITE);
  await page.waitForTimeout(1000);
  await page.getByRole('button', ARM_FRESH).click();
  await page.getByLabel('Card name').fill('buttons');
  await page.getByRole('button', { name: 'Arm', exact: true }).click();
  await page.getByRole('button', { name: 'Feed card side 1' }).click();
  await expect(page.getByRole('button', { name: 'Eject Card' })).toBeVisible();
  await expect(page.getByRole('button', ARM)).toHaveCount(0);
  await expect(page.getByRole('button', { name: 'Eject Card' })).toHaveCount(0, { timeout: 15000 });
  await expect(page.getByRole('button', ARM)).toBeVisible();
});

// Arm Card lists the saved cards by name; "No cards found" when none.
test('arm card lists the saved cards', async ({ page }) => {
  await gotoReady(page);
  await page.getByRole('button', ARM).click();
  await expect(page.getByRole('list', { name: 'Saved cards' })).toContainText('No cards found');
  await page.getByRole('button', { name: 'Cancel' }).click();
  await importCard(page, 'Beta.magnetcard.yaml', cardFile('Beta'));
  await importCard(page, 'alpha-file.magnetcard.yaml', cardFile('Alpha'));
  expect(await listedCards(page)).toEqual(['Alpha', 'Beta']);   // by the names inside, by name
});

// A fresh card armed, written by a program,
// listed by Arm Card; an existing name asks first; its cue card has its name.
test('a fresh card is written and listed and an existing name asks first', async ({ page }) => {
  await gotoReady(page);
  await writeFreshCard(page, 'prog');
  expect(await listedCards(page)).toEqual(['prog']);
  await page.getByRole('button', ARM_FRESH).click();
  await page.getByLabel('Card name').fill('prog');
  await page.getByRole('button', { name: 'Arm', exact: true }).click();
  await expect(page.getByRole('alert')).toContainText('Replace card “prog”? Its first write replaces both sides and the cue card.');
  await page.getByRole('button', { name: 'Cancel' }).click();
  await page.getByRole('button', ARM_FRESH).click();
  await page.getByLabel('Card name').fill('Other');
  await page.getByRole('button', { name: 'Arm', exact: true }).click();
  await expect(page.locator('.cuecard')).toHaveAttribute('aria-label', /^Other/);
  expect(await listedCards(page)).toEqual(['prog']);   // nothing written before the first write
});

// A card armed and read when a corner is pressed.
test('an armed card is read when a corner is pressed', async ({ page }) => {
  await gotoReady(page);
  await uploadPreset(page, PROGRAM_123);
  await pressKey(page, 'A');
  const expected = await settledLed(page);
  await writeFreshCard(page, 'prog');
  await uploadPreset(page, FRESH);   // a fresh machine: no program, nothing fitted
  await page.getByRole('button', ARM).click();
  await page.getByRole('button', { name: 'prog', exact: true }).click();
  await page.getByRole('button', { name: 'Arm', exact: true }).click();
  await expect(page.getByRole('button', { name: 'Eject Card' })).toHaveCount(0);   // fitted, not fed
  await page.getByRole('button', { name: 'Feed card side 1' }).click();
  await page.waitForTimeout(3000);
  await pressKey(page, 'CLR');
  await pressKey(page, 'A');
  expect(await settledLed(page)).toBe(expected);
});

// The cards are kept from one visit to the next.
test('cards kept from one visit to the next', async ({ page }) => {
  await gotoReady(page);
  await writeFreshCard(page, 'kept');
  await page.waitForTimeout(1500);   // the persistent file system is synced
  await page.reload();
  await gotoReady(page);
  expect(await listedCards(page)).toEqual(['kept']);
});

// The import of a card file (File ▸ Import Media…).
test('import a card file', async ({ page }) => {
  await gotoReady(page);
  await importCard(page, 'Imported.magnetcard.yaml', cardFile('Imported'));
  expect(await listedCards(page)).toEqual(['Imported']);
});

// Saved Media… lists the cards, downloads one, deletes one after asking.
test('saved media lists downloads deletes after asking', async ({ page }) => {
  await gotoReady(page);
  await importCard(page, 'Gone.magnetcard.yaml', cardFile('Gone'));
  await fileItem(page, 'saved-media');
  const row = page.locator('[data-calcu="media-saved-magnetic-card-Gone"]');
  await expect(row).toBeVisible();
  const download = page.waitForEvent('download');
  await row.locator('[data-calcu=media-download]').click();
  expect((await download).suggestedFilename()).toBe('Gone.magnetcard.yaml');
  page.once('dialog', (d) => d.accept());
  await row.locator('[data-calcu=media-delete]').click();
  await expect(row).toHaveCount(0, { timeout: 5000 });
  await page.getByRole('button', { name: 'Close' }).click();
  expect(await listedCards(page)).toEqual([]);
});

// No download per card and no upload in the card picker.
test('no per card download or upload in the card picker', async ({ page }) => {
  await gotoReady(page);
  await importCard(page, 'One.magnetcard.yaml', cardFile('One'));
  for (const which of [ARM, ARM_FRESH]) {
    await page.getByRole('button', which).click();
    const dialog = page.locator('[data-calcu=card-picker]');
    await expect(dialog.locator('input[type=file]')).toHaveCount(0);
    await expect(dialog.getByRole('button', { name: /Download/ })).toHaveCount(0);
    await expect(dialog).not.toContainText('Cards last for this page session');
    // The import and the saved media are File menu items.
    await expect(dialog.getByRole('button', { name: 'Import…' })).toHaveCount(0);
    await page.getByRole('button', { name: 'Cancel' }).click();
  }
});

// A dropped `.magnetcard.yaml`: imported into the save folder, fitted, read on a corner.
test('a dropped card is imported and fitted', async ({ page }) => {
  await gotoReady(page);
  await uploadPreset(page, PROGRAM_123);
  await pressKey(page, 'A');
  const expected = await settledLed(page);
  await writeFreshCard(page, 'prog');
  // The written card as a file: through Saved Media ▸ Download.
  await fileItem(page, 'saved-media');
  const download = page.waitForEvent('download');
  await page.locator('[data-calcu="media-saved-magnetic-card-prog"] [data-calcu=media-download]').click();
  const chunks = [];
  for await (const c of await (await download).createReadStream()) chunks.push(c);
  const text = Buffer.concat(chunks).toString().replace('card-name: "prog"', 'card-name: "dropped"');
  await page.getByRole('button', { name: 'Close' }).click();
  await uploadPreset(page, FRESH);   // a fresh machine: no program
  await dropFile(page, 'dropped.magnetcard.yaml', text);
  await expect(page.locator('.cuecard')).toHaveAttribute('aria-label', /^prog/);   // its own cue card
  await page.getByRole('button', { name: 'Feed card side 1' }).click();
  await expect(page.getByRole('button', { name: 'Eject Card' })).toHaveCount(0, { timeout: 15000 });
  await page.waitForTimeout(3000);
  await pressKey(page, 'CLR');
  await pressKey(page, 'A');
  expect(await settledLed(page)).toBe(expected);
  expect(await listedCards(page)).toEqual(['dropped', 'prog']);   // imported into the save folder
});

// A TI-58 has no card reader: refused with a notice.
test('a card dropped on the TI-58 is refused with a notice', async ({ page }) => {
  await gotoReady(page);
  await modelPicker(page).selectOption({ label: '58' });
  await page.waitForTimeout(600);
  await dropFile(page, 'dropped.magnetcard.yaml', cardFile('dropped'));
  await expect(el(page, 'notice')).toContainText('dropped.magnetcard.yaml');   // the notice dialog
  await expect(page.getByRole('button', { name: 'Eject Card' })).toHaveCount(0);
});

test('a dropped file that is nothing Calc-U-59 opens is refused with a notice', async ({ page }) => {
  await gotoReady(page);
  await dropFile(page, 'notes.txt', 'hello');
  await expect(el(page, 'notice')).toContainText('Calc-U-59 cannot open notes.txt.');
});

// A drop of exactly one file is opened; a drop of
// two is ignored.
test('a drop of two files is ignored', async ({ page }) => {
  await gotoReady(page);
  await dropFiles(page, [['notes.txt', 'hello'], ['other.txt', 'world']]);
  await page.waitForTimeout(800);
  await expect(el(page, 'notice')).toHaveCount(0);   // the first was not opened (it would be refused)
  await dropFile(page, 'notes.txt', 'hello');   // one file: opened, so refused with the notice
  await expect(el(page, 'notice')).toContainText('Calc-U-59 cannot open notes.txt.');
});

// ── Printer ──────────────────────────────────────────────────────────────

// The PC-100C's paper in the component set's paper panel.
test('the PC-100C prints a number, the text view shows it, Cut empties the paper and is disabled', async ({ page }) => {
  await gotoReady(page);
  const printer = page.locator('[data-panel=paper]');
  await connectPrinter(page);
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

test('PRINT and ADV are held, TRACE is a switch, the view control shows the view that is on', async ({ page }) => {
  await gotoReady(page);
  const printer = page.locator('[data-panel=paper]');
  const control = (id) => printer.locator(`[data-calcu=paper-control-${id}]`);
  await connectPrinter(page);
  await expect(control('PRINT')).toBeVisible();
  // The labels of the PC-100C's row, from the control state: PRINT, T with
  // the dot of TRACE's position, ADV; Copy and Cut as icons.
  await expect(control('PRINT')).toHaveText('PRINT');
  await expect(control('TRACE')).toHaveText('T');
  await expect(control('ADV')).toHaveText('ADV');
  await expect(control('TRACE').locator('.calcu-paper-dot')).toHaveCount(1);
  await expect(printer.locator('[data-calcu=paper-copy]')).toHaveAttribute('aria-label', 'Copy');
  await expect(printer.locator('[data-calcu=paper-cut]')).toHaveAttribute('aria-label', 'Cut');
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
  await expect(control('TRACE').locator('.calcu-paper-dot-on')).toHaveCount(1);
  await control('TRACE').click();
  await expect(control('TRACE')).toHaveAttribute('aria-pressed', 'false');
  // The view control names the view that is on.
  const view = printer.locator('[data-calcu=paper-view]');
  await expect(view).toHaveText('Marks');
  await view.click();
  await expect(view).toHaveText('Text');
  await expect(printer.locator('[data-calcu=paper-text]')).toBeVisible();
  // Detached: the panel shows an empty paper without a name.
  await el(page, 'bar-place-printer').click();
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
  // Lbl A 1 2 3 R/S, run with A: FREEZE lands on a program step.
  await uploadPreset(page, preset('program:\n  keycodes: |\n    76 11 01 02 03 85 01 95 61 00 02\n'));
  await pressKey(page, 'A');
  const debug = page.locator('[data-panel=debug]');
  await debug.getByRole('button', { name: 'FREEZE', exact: true }).click();
  await expect(debug.getByRole('button', { name: 'RESUME' })).toBeEnabled({ timeout: 5000 });
  const next = debug.getByText(/← next/);
  await expect(next).toBeVisible();
  const before = await next.textContent();
  // STEP: the next program step is marked next.
  const step = debug.locator('[data-calcu=run-control-step]');
  await step.dispatchEvent('pointerdown', { pointerId: 3 });
  await step.dispatchEvent('pointerup', { pointerId: 3 });
  await expect(debug.getByText(/← next/)).not.toHaveText(before, { timeout: 5000 });
  await debug.getByRole('button', { name: 'RESUME' }).click();
  await expect(debug.getByRole('button', { name: 'RESUME' })).toBeDisabled();
});

// The ASM overlay: chosen with Select File, run, and the
// CPU tab shows it running; the CALCULATOR tab cannot be chosen meanwhile.
const SIMPLE_COUNT = 'Calc-U-59 ASM 2.0\nPROGRAM:\n; a counter\nHEX:\n01D8 01DB 0D00 0A37 1805 1007\n';

test('an overlay is chosen and run and the CPU tab shows it', async ({ page }) => {
  await gotoReady(page);
  const debug = page.locator('[data-panel=debug]');
  await debug.getByRole('tab', { name: 'CPU', exact: true }).click();
  await debug.locator('.calcu-filebutton-inline input[type=file]').setInputFiles({
    name: 'simple_count.asm', mimeType: 'text/plain', buffer: Buffer.from(SIMPLE_COUNT),
  });
  await expect(debug.getByText('File: simple_count.asm')).toBeVisible({ timeout: 5000 });
  await expect(debug.getByText('Words: 6')).toBeVisible();
  await debug.getByRole('button', { name: 'Run', exact: true }).click();
  await expect(debug.getByText(/Status: ASM entered at 0x1800/)).toBeVisible({ timeout: 5000 });
  await expect(debug.getByRole('tab', { name: 'CPU', exact: true })).toHaveAttribute('aria-selected', 'true');
  await expect(debug.getByRole('tab', { name: 'CALCULATOR', exact: true })).toBeDisabled();
  await expect(debug.locator('.calcu-cpu-list')).toContainText(/^18/m, { timeout: 5000 });
  // An overlay without its first line is refused with a notice; the loaded one stays.
  await debug.locator('.calcu-filebutton-inline input[type=file]').setInputFiles({
    name: 'bad.asm', mimeType: 'text/plain', buffer: Buffer.from('HEX:\n0A01\n'),
  });
  await expect(el(page, 'notice')).toContainText('bad.asm cannot be read: an ASM overlay starts with the line');
  await el(page, 'notice-ok').click();
  await expect(debug.getByText('File: simple_count.asm')).toBeVisible();
  await debug.getByRole('button', { name: 'Clear', exact: true }).click();
  await expect(debug.getByRole('tab', { name: 'CALCULATOR', exact: true })).toBeEnabled({ timeout: 5000 });
});

test('the three debugger tabs, labels and short forms', async ({ page }) => {
  await gotoReady(page);
  const debug = page.locator('[data-panel=debug]');
  for (const name of ['CALCULATOR', 'CPU', 'LOG']) await expect(debug.getByRole('tab', { name, exact: true })).toBeVisible();
  await debug.getByRole('tab', { name: 'CALCULATOR', exact: true }).click();
  await expect(debug.locator('.calcu-debug-title')).toHaveText(/^(CALCULATOR DEBUG|DEBUG)$/);
  await expect(debug.getByRole('button', { name: /^(FREEZE ON START|F\.START)$/ })).toBeVisible();
  await expect(debug.getByRole('button', { name: 'ARMED' })).toBeVisible();
  // FREEZE ON START arms, ARMED disarms.
  await debug.getByRole('button', { name: /^(FREEZE ON START|F\.START)$/ }).click();
  await expect(debug.getByRole('button', { name: 'ARMED' })).toBeEnabled({ timeout: 5000 });
  await debug.getByRole('button', { name: 'ARMED' }).click();
  await expect(debug.getByRole('button', { name: 'ARMED' })).toBeDisabled({ timeout: 5000 });
  await debug.getByRole('tab', { name: 'CPU', exact: true }).click();
  await expect(debug.locator('.calcu-debug-title')).toHaveText(/^(CPU DEBUG|CPU)$/);
  await expect(debug.getByText('ROM HEATMAP')).toBeVisible();
  await expect(debug.getByText('ASM OVERLAY')).toBeVisible();
  // LOG: a dump into the log, the level button cycles, clear empties the view.
  await debug.getByRole('tab', { name: 'LOG', exact: true }).click();
  await debug.getByRole('button', { name: 'SCOM', exact: true }).click();
  await expect(debug.getByLabel('Debug log')).toHaveValue(/── SCOM ──/, { timeout: 5000 });
  const level = debug.getByRole('button', { name: /^LOG / });
  await expect(level).toHaveText('RUN LOG ○');
  await level.click();
  await expect(level).toHaveText('RUN LOG ●');
  await level.click();
  await level.click();
  await expect(level).toHaveText('RUN LOG ○');
  await debug.getByRole('button', { name: 'Clear the log' }).click();
  await expect(debug.getByLabel('Debug log')).toHaveValue('');
});

test('FREEZE STEP RESUME of both tabs follow the run state', async ({ page }) => {
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

// A row picked on the CPU tab holds only for its rows: a STEP shifts them,
// and the current instruction is selected anew (as in Qt and Swift).
test('a STEP on the CPU tab selects the current instruction anew', async ({ page }) => {
  await gotoReady(page);
  const debug = page.locator('[data-panel=debug]');
  await debug.getByRole('tab', { name: 'CPU', exact: true }).click();
  const button = (name) => debug.locator(`[data-calcu=run-control-${name}]`);
  await button('pause').click();
  await expect(button('step')).toBeEnabled({ timeout: 5000 });
  const rows = debug.locator('.calcu-cpu-list > div');
  const selected = debug.locator('.calcu-cpu-list > .calcu-cpu-selected');
  await expect(selected).toHaveAttribute('data-current', 'true', { timeout: 5000 });
  const current = await rows.evaluateAll((rs) => rs.findIndex((r) => r.dataset.current === 'true'));
  expect(current).toBeGreaterThan(0);
  await rows.nth(current - 1).click();
  await expect(rows.nth(current - 1)).toHaveClass(/calcu-cpu-selected/);
  await expect(selected).not.toHaveAttribute('data-current', 'true');
  const before = await debug.locator('.calcu-cpu-list').textContent();
  await button('step').dispatchEvent('pointerdown', { pointerId: 3 });
  await button('step').dispatchEvent('pointerup', { pointerId: 3 });
  await expect.poll(() => debug.locator('.calcu-cpu-list').textContent(), { timeout: 5000 }).not.toBe(before);
  await expect(selected).toHaveAttribute('data-current', 'true');
  await button('resume').click();
});

test('a trace is downloaded when switched off', async ({ page }) => {
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

// A preset's screenshot is a download,
// as Calc-U-1600's; the RUN LOG tab lists only traces.
test('a preset screenshot is offered as a download, not listed as a trace', async ({ page }) => {
  await gotoReady(page);
  const debug = page.locator('[data-panel=debug]');
  await debug.getByRole('tab', { name: 'LOG' }).click();
  const download = page.waitForEvent('download');
  await dropFile(page, 'shot.ti59', preset('keys:\n  - screenshot: shot.png\n'));
  const d = await download;
  expect(d.suggestedFilename()).toBe('shot.png');
  const bytes = Buffer.concat(await (await d.createReadStream()).toArray());
  expect(bytes.subarray(1, 4).toString()).toBe('PNG');
  await page.waitForTimeout(300);
  await expect(debug.locator('.calcu-traces')).toHaveCount(0);   // no trace was handed over
});

// ── The standalone app (app/) ────────────────────────────────────────────

// A fresh install starts the TI-59 with its startup preset: the Master
// Library and the PC-100C (the demo page starts bare).
test('the installed app starts with the startup preset', async ({ page }) => {
  await page.setViewportSize({ width: 1366, height: 1000 });
  await page.goto('app/');
  await expect(el(page, 'paper-owner')).toHaveText('PC-100C', { timeout: 15000 });
});

test('the standalone app follows the window as it rotates', async ({ page }) => {
  // iPhone: portrait pages; in landscape calculator and printer.
  await page.setViewportSize({ width: 440, height: 956 });
  await page.goto('app/');
  await expect(page.locator('.calcu-key')).toHaveCount(45, { timeout: 15000 });
  await expect(page.locator('[data-layout=portrait]')).toBeVisible();

  await page.setViewportSize({ width: 832, height: 419 });
  await expect(page.locator('[data-layout=wide] [data-panel=paper]')).toBeVisible();
  await connectPrinter(page);   // its controls show with a PC-100C attached
  // Fits the window without clipping.
  expect((await page.locator('.calcu-device').boundingBox()).width).toBeLessThanOrEqual(832);
  await expect(page.locator('[data-panel=debug]')).toHaveCount(0);
  await expect(page.locator('.calcu-key')).toHaveCount(45);
  // Everything stays inside the short window, the menus in the menu bar.
  const bar = await page.locator('.calcu-bar').boundingBox();
  expect(bar.y + bar.height).toBeLessThanOrEqual(419);
  await expect(el(page, 'menu-bar')).toBeVisible();

  // iPhone Duo open: the narrow tape's button bar fits it.
  await page.setViewportSize({ width: 880, height: 645 });
  await expect(page.locator('[data-panel=debug]')).toBeVisible();
  // The narrow tape's row: the short labels below the chrome's compact
  // width, in one row, nothing cut off.
  const paperRow = page.locator('[data-panel=paper] .calcu-paper-footer');
  const print = paperRow.locator('[data-calcu=paper-control-PRINT]');
  await expect(print).toBeVisible();
  const paperWidth = await page.locator('[data-panel=paper] [data-calcu=paper-panel]').evaluate((e) => e.clientWidth);
  await expect(print).toHaveText(paperWidth < 280 ? 'PRN' : 'PRINT');
  expect(await paperRow.evaluate((e) => e.scrollWidth <= e.clientWidth)).toBe(true);

  // iPad mini and iPad Pro: all three panels; the bar's labels only where
  // the whole bar fits the calculator with them, and never cut off.
  const barFits = () => page.locator('.calcu-bar .calcu-shell-bar').evaluate((e) => e.scrollWidth <= e.clientWidth);
  await page.setViewportSize({ width: 1133, height: 744 });
  await expect(page.locator('[data-panel=debug]')).toBeVisible();
  expect(await barFits()).toBe(true);
  await page.setViewportSize({ width: 1366, height: 1000 });
  await expect(page.locator('[data-panel=debug]')).toBeVisible();
  await expect(page.locator('.calcu-bar').getByText('Crd').first()).toBeVisible();
  expect(await barFits()).toBe(true);

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
  for (const f of ['../calc.js', '../wasm/calcu59-core.wasm', '../img/ti58_base.png', '../bundle/CD2400.txt']) {
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

  // The module picker of the bar.
  const modules = el(page, 'media-picker-slot-1');
  await modules.selectOption({ label: 'Applied Statistics' });
  await page.waitForTimeout(1000);
  // A module whose image can't be read is not selected: the picker would still show ML.
  await expect(modules.locator('option:checked')).toHaveText('Applied Statistics');

  await fileItem(page, 'examples', 'examples-Programs', 'example-examples:sum.ti59');
  await expect(page.locator('.cuecard')).toHaveAttribute('aria-label', /Sum 1\.\.N/, { timeout: 5000 });
});
