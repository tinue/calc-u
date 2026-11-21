const { test, expect } = require('@playwright/test');
const fs = require('fs');
const path = require('path');
const h = require('./helpers');
const { c, gotoReady, screen, waitForScreen, pickModel, menu, notBusy, shown } = h;

// The web client end to end through the real WASM engine, on dist/index.html
// (CalcU1600 with the physical keyboard), put together from the React set's
// shell: elements by the shell's names. URLs are relative to the
// project's baseURL -- the server root or a sub-path (playwright.config.js).
// The display is read as text through the session (`c1600.display-text`), not
// from pixels. The shell's own cases: shell.spec.js.

const FIXTURES = path.join(__dirname, 'fixtures');
// The PC-1500 tape is the C++ tests' (tests/fixtures/tape/), one copy.
const TAPE = path.join(__dirname, '..', '..', '..', 'tests', 'fixtures', 'tape', 'pc1500_tape.wav');

// File ▸ Examples ▸ <group> ▸ the example of that bundle path.
async function example(page, ref) {
  const entries = ((await h.ask(page, 'preset-list')).value || {}).entries || [];
  const group = (entries.find((e) => e.bundlePath === ref) || {}).group;
  await page.locator(c('menu-file')).click();
  await page.locator(c('item-examples')).hover();
  if (group) await page.locator(c(`item-examples-${group}`)).hover();
  await page.locator(c(`item-example-${ref}`)).click();
}

// A window tall enough that the panels fit below the full-width faceplate.
const TALL = { width: 1280, height: 1000 };

test('boots, and each model comes up', async ({ page }) => {
  await gotoReady(page);
  await expect(page.locator('.calcu-face[data-model="PC-1500A"]')).toHaveCount(1);  // the default
  await waitForScreen(page, /NEW0\?/);
  await pickModel(page, 'PC-1600');
  await waitForScreen(page, />/);
  await pickModel(page, 'PC-1500');
  await expect(page.locator('.calcu-key')).not.toHaveCount(0);
  await expect(page.locator(c('notice'))).toHaveCount(0);
});

// ?model= by the model's name only (an
// index is no model); ?preset= a path of the bundle with or without `examples:`.
test('?model= and ?preset= start with them', async ({ page }) => {
  await gotoReady(page, '?model=PC-1600');
  await expect(page.locator('.calcu-face[data-model="PC-1600"]')).toHaveCount(1, { timeout: 30000 });
  await expect(page.locator(c('notice'))).toHaveCount(0);
  await gotoReady(page, '?model=PC-1500');
  await expect(page.locator('.calcu-face[data-model="PC-1500"]')).toHaveCount(1, { timeout: 30000 });
  await expect(page.locator(c('notice'))).toHaveCount(0);
  await gotoReady(page, '?model=2');
  await expect(page.locator(c('notice'))).toContainText('There is no model "2"');
  // started as without it: the last used model
  await expect(page.locator('.calcu-face[data-model="PC-1500"]')).toHaveCount(1, { timeout: 30000 });
  await page.setViewportSize(TALL);
  for (const preset of ['plotter/rectangles-1600.pc1600', 'examples:plotter/rectangles-1600.pc1600']) {
    await gotoReady(page, `?preset=${preset}`);
    await expect(page.locator('[data-panel=paper][data-empty=false]')).toHaveCount(1, { timeout: 45000 });
    await expect(page.locator(c('notice'))).toHaveCount(0);
  }
});

// Settings > Startup presets (the startup presets of
// examples/bundle.yaml), each shown with its name and description; the
// Examples ▸ menu does not offer them.
test('a startup preset is picked by its name and description', async ({ page }) => {
  await gotoReady(page);
  await menu(page, 'file', 'settings');
  const row = page.locator(c('setting-preset-PC-1500A'));
  expect(await shown(row)).toBe('None');
  await row.selectOption({ label: 'CE-150 + CE-163F — PC-1500A with the CE-150 plotter and a CE-163F' });
  await expect.poll(async () => shown(row)).toContain('CE-150 + CE-163F');
  await page.locator(c('settings-close')).click();
  const examples = ((await h.ask(page, 'preset-list')).value.entries || []).filter((e) => e.kind === 'example');
  expect(examples.some((e) => e.bundlePath.startsWith('examples:startup/'))).toBe(false);
  await example(page, 'examples:basic/hanoi.pc1600');   // the menu's item for it
  await expect(page.locator('.calcu-face[data-model="PC-1600"]')).toHaveCount(1, { timeout: 30000 });
});

test('the panels take only the room a full-width faceplate leaves', async ({ page }) => {
  // A phone in landscape: the calculator alone, as large as it fits.
  await page.setViewportSize({ width: 844, height: 390 });
  await gotoReady(page);
  await expect(page.locator(c('panel-area'))).toHaveCount(0);
  const face = await page.locator('.calcu-face').boundingBox();
  expect(face.height).toBeGreaterThan(250);
  // In portrait the faceplate spans the width and the inspector fits below.
  await page.setViewportSize({ width: 390, height: 844 });
  await expect(page.locator('[data-panel=inspector]')).toBeVisible();
  // A tall desktop window too; a wide, short one shows the calculator alone.
  await page.setViewportSize(TALL);
  await expect(page.locator('[data-panel=inspector]')).toBeVisible();
  await page.setViewportSize({ width: 1600, height: 700 });
  await expect(page.locator(c('panel-area'))).toHaveCount(0);
});

test('the client has the keyboard from the start, no click needed', async ({ page }) => {
  await gotoReady(page);
  await expect(page.locator('.calcu-client')).toBeFocused();
});

test('the physical keyboard types, ENTER runs it', async ({ page }) => {
  await gotoReady(page);
  await pickModel(page, 'PC-1600');
  await page.locator('.calcu-client').focus();  // from the Model picker
  await page.keyboard.type('PRINT 6*7', { delay: 60 });
  await page.keyboard.press('Enter');
  await waitForScreen(page, /42/);
});

test('the keyboard comes back to the calculator after a picker or a click', async ({ page }) => {
  await gotoReady(page);
  // A picker change hands the keyboard back.
  await pickModel(page, 'PC-1600');
  await page.keyboard.type('PRINT 6*7', { delay: 60 });
  await page.keyboard.press('Enter');
  await waitForScreen(page, /42/);
  // A button that had the focus doesn't get ENTER after a click on the
  // faceplate (off any key): the calculator does.
  await page.locator(c('menu-help')).focus();
  const face = await page.locator('.calcu-face').boundingBox();
  await page.mouse.click(face.x + 4, face.y + 4);
  await page.keyboard.type('PRINT 7*8', { delay: 60 });
  await page.keyboard.press('Enter');
  await waitForScreen(page, /56/);
  await expect(page.locator('.calcu-shell-menu-items')).toHaveCount(0);
  // iOS moves the focus to a menu title on its first letter typed on a
  // hardware keyboard: the client takes it straight back, and the next
  // keys still reach the calculator.
  await page.keyboard.press('p');
  await page.locator(c('menu-file')).focus();
  await expect(page.locator('.calcu-client')).toBeFocused();
  await page.keyboard.type('RINT 9*9', { delay: 60 });
  await page.keyboard.press('Enter');
  await waitForScreen(page, /81/);
});

test('a faceplate key types while its pointer is down', async ({ page }) => {
  await gotoReady(page);
  await pickModel(page, 'PC-1600');
  const key = await page.locator('.calcu-key[data-key="5"]').boundingBox();
  await page.mouse.move(key.x + key.width / 2, key.y + key.height / 2);
  await page.mouse.down();
  await expect(page.locator('.calcu-key[data-key="5"]')).toHaveAttribute('aria-pressed', 'true');
  await page.waitForTimeout(150);
  await page.mouse.up();
  await expect(page.locator('.calcu-key[data-key="5"]')).toHaveAttribute('aria-pressed', 'false');
  await waitForScreen(page, /5/);
});

test.describe('on a touchscreen', () => {
  test.use({ hasTouch: true });

  // An instant tap (down and up at once) in the gap just below a key: the
  // shell's rule takes the nearest key, the engine's minimum hold lets the
  // ROM see it.
  test('a quick tap next to a key types it', async ({ page }) => {
    await gotoReady(page);
    await pickModel(page, 'PC-1600');
    const key = await page.locator('.calcu-key[data-key="5"]').boundingBox();
    await page.touchscreen.tap(key.x + key.width / 2, key.y + key.height + 3);
    await waitForScreen(page, /5/);
  });

  // The taller touch menu bar is counted: the control bar ends inside the
  // window (a phone in landscape, the calculator alone).
  test('the client fits the window with the touch menu bar', async ({ page }) => {
    await page.setViewportSize({ width: 932, height: 400 });
    await gotoReady(page, '?height=400');
    expect(await page.evaluate(() => matchMedia('(pointer: coarse)').matches)).toBe(true);
    const bar = await page.locator('.calcu-controlbar').boundingBox();
    const client = await page.locator('.calcu-client').boundingBox();
    expect(bar.y + bar.height).toBeLessThanOrEqual(client.y + client.height + 0.5);
  });
});

test('the control bar follows the model: peripherals, floppy, tape', async ({ page }) => {
  await page.setViewportSize(TALL);
  await gotoReady(page);
  // PC-1500A: CE-150 and CE-158 on the bar, no disk.
  await expect(page.locator(c('bar-place-printer'))).toHaveText('CE-150');
  await expect(page.locator(c('bar-place-interface'))).toHaveText('CE-158');
  await expect(page.locator(c('bar-disk'))).toHaveCount(0);
  await page.locator(c('bar-place-printer')).click();
  await expect(page.locator(c('bar-place-printer'))).toHaveAttribute('aria-pressed', 'true', { timeout: 30000 });
  await expect(page.locator(c('media-picker-cassette'))).toBeVisible();
  await expect(page.locator('[data-panel=paper]')).toBeVisible();

  await pickModel(page, 'PC-1600');
  await expect(page.locator(c('bar-disk'))).toHaveCount(0);
  await page.locator(c('bar-place-printer')).selectOption({ label: 'CE-1600P' });
  await expect.poll(() => page.locator(c('bar-disk')).evaluate((e) => e.disabled), { timeout: 30000 }).toBe(false);
  // Machine ▸ Peripherals: the CE-158 is ruled out while the CE-1600P is on.
  await page.locator(c('menu-machine')).click();
  await page.locator(c('item-peripherals')).hover();
  await expect(page.locator(c('item-peripheral-interface-CE-158'))).toHaveAttribute('aria-disabled', 'true');
  await expect(page.locator(c('item-peripheral-printer-CE-1600P'))).toHaveAttribute('aria-checked', 'true');
});

// A plotter example is loaded, and the paper panel
// appears under the plotter's name with marks on it.
test('plotter example shows the paper panel with marks', async ({ page }) => {
  await page.setViewportSize(TALL);
  await gotoReady(page);
  await example(page, 'examples:plotter/rectangles-1600.pc1600');
  await expect(page.locator('[data-panel=paper][data-empty=false]')).toHaveCount(1, { timeout: 45000 });
  await expect(page.locator('[data-panel=paper] [data-calcu=paper-owner]')).toHaveText('CE-1600P');
});

// A bundled example is loaded by its bundle path,
// and its result shows.
test('bundled example loads and its result shows', async ({ page }) => {
  await page.setViewportSize(TALL);
  await gotoReady(page);
  await example(page, 'examples:basic/hanoi.pc1600');
  await expect(page.locator('.calcu-face[data-model="PC-1600"]')).toHaveCount(1, { timeout: 30000 });
  await waitForScreen(page, /Hanoy Towers|Bricks/, 45000);
  await expect(page.locator(c('notice'))).toHaveCount(0);
});

// Machine ▸ Cold boot.
test('cold boot menu item', async ({ page }) => {
  await gotoReady(page);
  await waitForScreen(page, /NEW0\?/);
  await page.keyboard.press('Escape');
  await menu(page, 'machine', 'cold-boot');
  await notBusy(page);
  await waitForScreen(page, /NEW0\?/);
});

// The paper panel is always shown, named while a plotter is
// plugged in; a detach keeps the paper.
test('paper panel always shown, named while a plotter is plugged in', async ({ page }) => {
  await page.setViewportSize(TALL);
  await gotoReady(page);
  const panel = page.locator('[data-panel=paper]');
  await expect(panel).toBeVisible();
  await expect(panel.locator('[data-calcu=paper-owner]')).toHaveText('');
  await page.locator(c('bar-place-printer')).click();
  await expect(page.locator(c('bar-place-printer'))).toHaveAttribute('aria-pressed', 'true', { timeout: 30000 });
  await expect(panel.locator('[data-calcu=paper-owner]')).toHaveText('CE-150');
  await page.locator(c('bar-place-printer')).click();
  await expect(page.locator(c('bar-place-printer'))).toHaveAttribute('aria-pressed', 'false', { timeout: 30000 });
  await expect(panel).toBeVisible();
  await expect(panel.locator('[data-calcu=paper-owner]')).toHaveText('');
});

// ── The media of the topic `media` on the control bar ────────────────────

const PRODUCT = path.join(__dirname, '..', '..', '..');
const CARDS = path.join(PRODUCT, 'frontends', 'qt6', 'resources', 'cards');
const nameAndSave = (page) => page.locator('[data-dialog=name-and-save]');
const slot1 = (page) => page.locator(c('media-picker-slot-1'));
const saveSlot1 = (page) => page.locator(c('name-and-save-slot-1'));

// A bundled template's file as an instance of another name (an import).
function instanceOf(file, key, name) {
  return fs.readFileSync(path.join(CARDS, file), 'utf8')
    .replace(new RegExp(`^${key}: .*$`, 'm'), `${key}: "${name}"`)
    .replace(/^template: true.*\n/m, '');
}

// A module template saved under `name` (Name & Save of slot 1).
async function saveModule(page, template, name) {
  await slot1(page).selectOption({ label: template });
  await expect(saveSlot1(page)).toBeEnabled({ timeout: 30000 });
  await saveSlot1(page).click();
  await nameAndSave(page).locator('[data-calcu=media-name]').fill(name);
  await nameAndSave(page).locator('[data-calcu=media-save]').click();
  await expect.poll(() => shown(slot1(page)), { timeout: 30000 }).toBe(name);
}

test('Name & Save can write-protect a CE-161, and only a card with the switch', async ({ page }) => {
  await gotoReady(page);
  await slot1(page).selectOption({ label: 'CE-161' });
  await expect(saveSlot1(page)).toBeEnabled({ timeout: 30000 });
  await saveSlot1(page).click();
  await expect(nameAndSave(page)).toContainText('Instance name:');
  const box = nameAndSave(page).locator('[data-calcu=media-protect]');
  await expect(box).toHaveCount(1);
  await expect(nameAndSave(page)).toContainText("Write-protect (switch to read-only, can't be undone)");
  const name = `Rom ${Date.now()}`;
  await nameAndSave(page).locator('[data-calcu=media-name]').fill(name);
  await box.check();
  await nameAndSave(page).locator('[data-calcu=media-save]').click();
  await expect.poll(() => shown(slot1(page)), { timeout: 30000 }).toBe(name);
  await notBusy(page);

  await slot1(page).selectOption({ label: 'CE-1638' });  // battery-backed, no switch
  await expect(saveSlot1(page)).toBeEnabled({ timeout: 30000 });
  await saveSlot1(page).click();
  await expect(nameAndSave(page).locator('[data-calcu=media-protect]')).toHaveCount(0);
  await nameAndSave(page).getByRole('button', { name: 'Cancel' }).click();
  await expect(nameAndSave(page)).toHaveCount(0);
});

// A module template is saved under a name and is an
// instance afterwards (it offers no Name & Save) ...
test('module template saved under a name is an instance', async ({ page }) => {
  await gotoReady(page);
  await pickModel(page, 'PC-1600');
  await saveModule(page, 'CE-1600M', `Work ${Date.now()}`);
  await expect(saveSlot1(page)).toBeDisabled();
});

// ... and the picker of that slot then lists it among the instances.
test('the slot picker lists a saved module among the instances', async ({ page }) => {
  await gotoReady(page);
  await pickModel(page, 'PC-1600');
  const name = `Listed ${Date.now()}`;
  await saveModule(page, 'CE-1600M', name);
  // The picker's order: the empty slot, the templates, the instances, the
  // modules that are not media.
  const labels = await slot1(page).locator('option').allTextContents();
  expect(labels[0]).toBe('(empty)');
  expect(labels.indexOf(name)).toBeGreaterThan(labels.indexOf('CE-1600M'));
  expect(labels.indexOf(name)).toBeLessThan(labels.indexOf('CE-151'));
  await expect(page.locator(c('media-picker-slot-2')).locator('option', { hasText: name })).toHaveCount(1);
});

test('Name & Save keeps a card across a reload', async ({ page }) => {
  await gotoReady(page);
  await pickModel(page, 'PC-1600');
  const name = `Work ${Date.now()}`;
  await saveModule(page, 'CE-1600M', name);
  await page.waitForTimeout(500);  // IndexedDB write

  await page.reload();
  await expect(page.locator('.calcu-client[data-ready=true]')).toHaveCount(1, { timeout: 30000 });
  await expect(page.locator('.calcu-face[data-model="PC-1600"]')).toHaveCount(1);  // last used model
  await expect(slot1(page).locator('option', { hasText: name })).toHaveCount(1, { timeout: 30000 });
});

// The saved media list: by kind and name, a template
// and a medium in a place marked; one in a place cannot be deleted.
test('saved media list', async ({ page }) => {
  await gotoReady(page);
  await pickModel(page, 'PC-1600');
  const name = `Kept ${Date.now()}`;
  await saveModule(page, 'CE-1600M', name);
  await notBusy(page);
  await menu(page, 'file', 'saved-media');
  const row = page.locator(`[data-calcu="media-saved-battery-backed-module-${name}"]`);
  await expect(row).toContainText('memory module');
  await expect(row).toContainText('in slot-1');
  await expect(row.getByRole('button', { name: 'Delete' })).toBeDisabled();
  const download = page.waitForEvent('download');
  await row.getByRole('button', { name: 'Download' }).click();
  expect((await download).suggestedFilename()).toBe(`${name}.module.yaml`);
  await page.locator(c('saved-media')).getByRole('button', { name: 'Close' }).click();
  await expect(page.locator(c('saved-media'))).toHaveCount(0);
});

// A delete asks first; no keeps it.
test('saved media delete asks first', async ({ page }) => {
  await gotoReady(page);
  await pickModel(page, 'PC-1600');
  const name = `Gone ${Date.now()}`;
  await saveModule(page, 'CE-1600M', name);
  await slot1(page).selectOption({ label: '(empty)' });
  await notBusy(page);
  await menu(page, 'file', 'saved-media');
  const row = page.locator(`[data-calcu="media-saved-battery-backed-module-${name}"]`);
  await expect(row.getByRole('button', { name: 'Delete' })).toBeEnabled();
  let asked = '';
  page.once('dialog', (d) => { asked = d.message(); d.dismiss(); });
  await row.getByRole('button', { name: 'Delete' }).click();
  await expect.poll(() => asked).toContain('Delete');
  await page.waitForTimeout(300);
  await expect(row).toHaveCount(1);
  page.once('dialog', (d) => d.accept());
  await row.getByRole('button', { name: 'Delete' }).click();
  await expect(row).toHaveCount(0);
});

// Import Media… (the library's `media.import`: the host's file
// picker) takes module files, disk files and a cassette definition with its
// WAV, chosen together.
test('import card disk and tape with wav', async ({ page }) => {
  await gotoReady(page);
  const stamp = Date.now();
  const files = [
    { name: `Card ${stamp}.module.yaml`, mimeType: 'text/yaml', buffer: Buffer.from(instanceOf('ce1600m.module.yaml', 'module-name', `Card ${stamp}`)) },
    { name: `Disk ${stamp}.floppy.yaml`, mimeType: 'text/yaml', buffer: Buffer.from(instanceOf('formatted.floppy.yaml', 'disk-name', `Disk ${stamp}`)) },
    { name: `Tape ${stamp}.cassette.yaml`, mimeType: 'text/yaml',
      buffer: Buffer.from(`format: cassette\nformat-version: 1\ncassette-name: "Tape ${stamp}"\naudio: { file: "side${stamp}.wav" }\n`) },
    { name: `side${stamp}.wav`, mimeType: 'audio/wav', buffer: fs.readFileSync(TAPE) },
  ];
  const chooser = page.waitForEvent('filechooser');
  await menu(page, 'file', 'import-media');
  await (await chooser).setFiles(files);
  await notBusy(page);
  await menu(page, 'file', 'saved-media');
  await expect(page.locator(`[data-calcu="media-saved-battery-backed-module-Card ${stamp}"]`)).toHaveCount(1, { timeout: 30000 });
  await expect(page.locator(`[data-calcu="media-saved-floppy-disk-Disk ${stamp}"]`)).toHaveCount(1);
  await expect(page.locator(`[data-calcu="media-saved-cassette-Tape ${stamp}"]`)).toHaveCount(1);
  await page.locator(c('saved-media')).getByRole('button', { name: 'Close' }).click();
  // The tape plays from the picker of the CE-150's cassette.
  await page.locator(c('bar-place-printer')).click();
  await expect(page.locator(c('media-picker-cassette')).locator('option', { hasText: `Tape ${stamp}` })).toHaveCount(1, { timeout: 30000 });
});

// The side button shows the side up, 0 or 1, and turns the disk over.
test('the disk side button turns the disk over', async ({ page }) => {
  await page.setViewportSize(TALL);
  await gotoReady(page);
  await pickModel(page, 'PC-1600');
  await page.locator(c('bar-place-printer')).selectOption({ label: 'CE-1600P' });
  await expect(page.locator(c('media-picker-floppy'))).toBeEnabled({ timeout: 30000 });
  await page.locator(c('media-picker-floppy')).selectOption({ label: 'Formatted' });
  await expect(page.locator(c('disk-side'))).toHaveText('0', { timeout: 30000 });
  await page.locator(c('disk-side')).click();
  await expect(page.locator(c('disk-side'))).toHaveText('1');
  expect(await shown(page.locator(c('media-picker-floppy')))).toBe('Formatted');
});

// The record button: a name first, a new tape on the empty template.
test('a new tape is recorded under its name', async ({ page }) => {
  await page.setViewportSize(TALL);
  await gotoReady(page);
  await page.locator(c('bar-place-printer')).click();
  await expect(page.locator(c('media-picker-cassette'))).toBeVisible({ timeout: 30000 });
  await page.locator(c('cassette-new')).click();
  const name = `Rec ${Date.now()}`;
  await page.locator('[data-dialog=prompt] input[type=text]').fill(name);
  await page.locator('[data-dialog=prompt] button', { hasText: 'OK' }).click();
  await expect.poll(() => shown(page.locator(c('media-picker-cassette'))), { timeout: 30000 }).toBe(name);
  await expect(page.locator('[data-lamp=tape]')).toContainText('●');
});

test('headerless code asks the address, then NEW and CALL', async ({ page }) => {
  await gotoReady(page);
  await pickModel(page, 'PC-1600');
  const chooser = page.waitForEvent('filechooser');
  await menu(page, 'file', 'load-machine-code');
  await (await chooser).setFiles(path.join(FIXTURES, 'rtn4.bin'));
  const dialog = page.locator('[data-dialog=machinecode]');
  await expect(dialog.locator('input')).toHaveValue('&C0C5');
  await dialog.locator('input').fill('zz');
  await expect(dialog).toContainText('Not a hex address');
  await expect(dialog.getByRole('button', { name: 'Load' })).toBeDisabled();
  await dialog.locator('input').fill('&C0C5');
  await dialog.getByRole('button', { name: 'Load' }).click();
  const loaded = page.locator('[data-dialog=loaded]');
  await expect(loaded).toContainText('Loaded 4 bytes at &C0C5');
  await expect(loaded).toContainText('CALL &C0C5');
  await loaded.getByRole('button', { name: 'OK' }).click();
  await waitForScreen(page, /CALL &C0C5/);
});

// The fixture tape twice in one WAV: its data chunk doubled.
function doubledTape() {
  const wav = fs.readFileSync(TAPE);
  let at = 12;
  while (wav.toString('ascii', at, at + 4) !== 'data') at += 8 + wav.readUInt32LE(at + 4);
  const size = wav.readUInt32LE(at + 4);
  const data = wav.subarray(at + 8, at + 8 + size);
  const doubled = Buffer.concat([wav.subarray(0, at + 8 + size), data]);
  doubled.writeUInt32LE(size * 2, at + 4);
  doubled.writeUInt32LE(doubled.length - 8, 4);
  return doubled;
}

test('a WAV with several programs asks which one', async ({ page }) => {
  await gotoReady(page);
  const chooser = page.waitForEvent('filechooser');
  await menu(page, 'file', 'load-basic-program');
  await (await chooser).setFiles({ name: 'two.wav', mimeType: 'audio/wav', buffer: doubledTape() });
  const dialog = page.locator('[data-dialog=choice]');
  await expect(dialog).toContainText('two.wav holds 2 files. Load which one?');
  await expect(dialog.locator('option')).toHaveCount(2);
  await dialog.getByRole('button', { name: 'OK' }).click();
  await expect(dialog).toHaveCount(0);
});

test('a BASIC program is loaded, a refusal is a notice', async ({ page }) => {
  await gotoReady(page);
  await pickModel(page, 'PC-1600');
  let chooser = page.waitForEvent('filechooser');
  await menu(page, 'file', 'load-basic-program');
  await (await chooser).setFiles({ name: 'six.bas', mimeType: 'text/plain', buffer: Buffer.from('10 PRINT 6*7\n') });
  await expect(page.locator(c('notice'))).toHaveCount(0);
  await page.locator('.calcu-client').focus();
  await page.keyboard.type('RUN', { delay: 60 });
  await page.keyboard.press('Enter');
  await waitForScreen(page, /42/);
  // Machine code given to the BASIC loader: refused, as a notice without a
  // title.
  chooser = page.waitForEvent('filechooser');
  await menu(page, 'file', 'load-basic-program');
  await (await chooser).setFiles(path.join(FIXTURES, 'rtn4.bin'));
  const notice = page.locator(c('notice'));
  await expect(notice).toContainText('rtn4.bin: ');
  await expect(notice.locator('.calcu-shell-modal-title')).toHaveCount(0);
});

test('the loaded dialog and the CALL in the input line, not started', async ({ page }) => {
  await gotoReady(page);
  await pickModel(page, 'PC-1600');
  // A PC-1600 header: Z-80 RET at &C100, no address to start at.
  const file = Buffer.from([0xFF, 0x10, 0x00, 0x00, 0x10, 0x01, 0x00, 0x00, 0x00, 0xC1, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x0F, 0xC9]);
  const chooser = page.waitForEvent('filechooser');
  await menu(page, 'file', 'load-machine-code');
  await (await chooser).setFiles({ name: 'ret.bin', mimeType: 'application/octet-stream', buffer: file });
  await expect(page.locator('[data-dialog=machinecode]')).toHaveCount(0);
  const loaded = page.locator('[data-dialog=loaded]');
  await expect(loaded).toContainText('Keep BASIC from overwriting it:');
  await expect(loaded).toContainText('Start it:');
  await expect(loaded).toContainText('CALL &C100');
  await loaded.getByRole('button', { name: 'OK' }).click();
  await waitForScreen(page, /CALL &C100/);
  await page.waitForTimeout(500);
  expect(await screen(page)).toMatch(/CALL &C100/);   // in the input line, not started
});

test('the inspector shows the pointers and the module dump', async ({ page }) => {
  await page.setViewportSize(TALL);
  await gotoReady(page);
  await page.locator('[data-panel=inspector] button', { hasText: 'Pointers' }).click();
  await expect(page.locator('.calcu-inspector-output')).toContainText('RAM_ST');
  await page.locator('[data-panel=inspector] button', { hasText: 'Module YAML' }).click();
  await expect(page.locator('.calcu-inspector-output')).toContainText('Module dump');
});

// The shell's storage banner with the library's sentence, not
// Calc-U-1600's own.
test('a second tab says its changes are not saved', async ({ page, context }) => {
  await gotoReady(page);
  await expect(page.locator('[data-calcu="storage-banner"]')).toHaveCount(0);
  const second = await context.newPage();
  await gotoReady(second);
  await expect(second.locator('[data-calcu="storage-banner"]')).toHaveText(
    'Calc-U-1600 is open in another tab or window: what you save here is not kept.');
});

test('the installed app shell starts', async ({ page }) => {
  await page.goto('./app/');
  await expect(page.locator('.calcu-client[data-ready=true]')).toHaveCount(1, { timeout: 30000 });
  await expect(page.locator(c('menu-bar'))).toBeVisible();
});

// The run control in the Web page
// without a debugger -- the page has no harness, so the session commands
// pause and resume go from the page.
test('run control pauses and resumes the web machine', async ({ page }) => {
  await gotoReady(page);
  const paused = async () => !!((await h.topic(page, 'run-state')) || {}).paused;
  const ticks = async () => (await h.ask(page, 'ticks')).value.ticks;
  await h.send(page, 'pause', { level: 'cpu' });
  await expect.poll(paused, { timeout: 10000 }).toBe(true);
  const t0 = await ticks();
  await page.waitForTimeout(300);
  expect(await ticks()).toBe(t0);   // paused: no emulated time passes
  await h.send(page, 'resume');
  await expect.poll(paused, { timeout: 10000 }).toBe(false);
  await expect.poll(ticks, { timeout: 10000 }).toBeGreaterThan(t0);
});

// Machine ▸ Resume runs a paused machine again, and can be
// used only while it is paused.
test('Machine Resume runs a paused machine', async ({ page }) => {
  await gotoReady(page);
  const paused = async () => !!((await h.topic(page, 'run-state')) || {}).paused;
  await page.locator(c('menu-machine')).click();
  await expect(page.locator(c('item-resume'))).toHaveAttribute('aria-disabled', 'true');
  await page.locator(c('menu-machine')).click();
  await h.send(page, 'pause', { level: 'cpu' });
  await expect.poll(paused, { timeout: 10000 }).toBe(true);
  await menu(page, 'machine', 'resume');
  await expect.poll(paused, { timeout: 10000 }).toBe(false);
});

// ---- Opening a file ----

// A file dropped on the client, as the browser hands it over.
async function dropFile(page, name, buffer) {
  await page.evaluate(([n, bytes]) => {
    const dt = new DataTransfer();
    dt.items.add(new File([new Uint8Array(bytes)], n));
    document.querySelector('.calcu-client')
      .dispatchEvent(new DragEvent('drop', { dataTransfer: dt, bubbles: true, cancelable: true }));
  }, [name, [...buffer]]);
}

test('a dropped WAV with several programs asks which one, then loads by its kind', async ({ page }) => {
  await gotoReady(page);
  await notBusy(page);
  await dropFile(page, 'two.wav', doubledTape());
  const dialog = page.locator('[data-dialog=choice]');
  await expect(dialog).toContainText('two.wav holds 2 files. Load which one?', { timeout: 30000 });
  await expect(dialog.locator('option')).toHaveCount(2);
  await dialog.getByRole('button', { name: 'OK' }).click();
  await expect(dialog).toHaveCount(0);
  // A BASIC file: the BASIC loader, which says nothing when it loads.
  await expect(page.locator('[data-dialog=machinecode]')).toHaveCount(0);
});

test('a dropped headerless .bin asks the address', async ({ page }) => {
  await gotoReady(page);
  await pickModel(page, 'PC-1600');
  // SharpDataExchange's Z-80 sample, eight times: headerless code by its look.
  const block = [0x21, 0x00, 0x80, 0x06, 0x10, 0x7E, 0xFE, 0x20, 0x28, 0x01, 0x23, 0x10, 0xF8, 0xCD, 0x00, 0x10, 0xC9];
  const code = Buffer.from(Array(8).fill(block).flat());
  await dropFile(page, 'loop.bin', code);
  const dialog = page.locator('[data-dialog=machinecode]');
  await expect(dialog.locator('input')).toHaveValue('&C0C5', { timeout: 30000 });
  await dialog.getByRole('button', { name: 'Cancel' }).click();
  // Named .rom it is nothing the product opens: a notice.
  await dropFile(page, 'loop.rom', code);
  await expect(page.locator(c('notice'))).toContainText('Calc-U-1600 cannot open loop.rom.', { timeout: 30000 });
});

test('a dropped disk file is imported and inserted', async ({ page }) => {
  await gotoReady(page);
  await pickModel(page, 'PC-1600');
  await page.locator(c('bar-place-printer')).selectOption({ label: 'CE-1600P' });
  await expect(page.locator(c('media-picker-floppy'))).toBeEnabled({ timeout: 30000 });
  const name = `Dropped ${Date.now()}`;
  await dropFile(page, `${name}.floppy.yaml`, Buffer.from(instanceOf('formatted.floppy.yaml', 'disk-name', name)));
  await expect.poll(() => shown(page.locator(c('media-picker-floppy'))), { timeout: 30000 }).toBe(name);
  await expect(page.locator(c('notice'))).toHaveCount(0);
});

test('a dropped file over 1 MiB is refused with a notice', async ({ page }) => {
  await gotoReady(page);
  await notBusy(page);
  await dropFile(page, 'big.bas', Buffer.alloc((1 << 20) + 1, 0x20));
  await expect(page.locator(c('notice'))).toContainText('big.bas is larger than 1 MiB', { timeout: 30000 });
});

// File ▸ Load Preset… takes several files; the preset among
// them is opened with the files it names beside it.
test('Load Preset opens the preset among its files', async ({ page }) => {
  await gotoReady(page);
  await notBusy(page);
  const chooser = page.waitForEvent('filechooser');
  await menu(page, 'file', 'load-preset');
  await (await chooser).setFiles([
    { name: 'six.bas', mimeType: 'text/plain', buffer: Buffer.from('10 PRINT 6*7\n') },
    { name: 'run.pc1600', mimeType: 'text/plain',
      buffer: Buffer.from('format-version: 2\nmodel: PC-1600\nprogram: { file: six.bas }\nkeys:\n  - type: RUN\n  - expect: 42\n') },
  ]);
  await expect(page.locator('.calcu-face[data-model="PC-1600"]')).toHaveCount(1, { timeout: 30000 });
  await waitForScreen(page, /42/);
  await expect(page.locator(c('notice'))).toHaveCount(0);
});
