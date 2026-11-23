const { test, expect } = require('@playwright/test');
const fs = require('fs');
const { open, send, request, files } = require('./helpers');

// The media components of the React set drawn in the test product Test-M
// (src/CalcUTest.jsx with shell=1), whose session hosts the library's MediaPart
// with the test kinds of library/media/tests/TestKinds.hpp on the page's
// save folder; Chromium and WebKit. test-dot's holding place is `disk`
// (test-disk, `.testdisk.yaml`).

const el = (page, name) => page.locator(`[data-calcu="${name}"]`);
const topic = (page, name) => page.evaluate((n) => window.calcuTestShells[0].getState().topics[n], name);
const disk = (name, content, extra = '') =>
  `format: test-disk\nformat-version: ${extra.includes('version-1') ? 1 : 2}\nname: ${name}\n` +
  `${extra.includes('template') ? 'template: true\n' : ''}content: ${content}\n`;

// Whether the save folder has the file now (tp.files is listed only by tp.* commands).
const inSaveFolder = async (page, name) => (await request(page, 'tp.read', { name })).done;

async function start(page, model = 'test-dot') {
  await open(page, 'Test-M', `shell=1&width=900&height=900&model=${model}`);
  await expect(el(page, 'control-bar')).toBeVisible();
  await expect.poll(async () => ((await topic(page, 'media')) || {}).kinds?.length || 0).toBeGreaterThan(0);
}
async function names(page, kind = 'test-disk') {
  const m = await topic(page, 'media');
  const k = ((m && m.kinds) || []).find((x) => x.kind === kind);
  return k ? [...k.templates, ...k.instances].map((i) => i.name) : [];
}
async function saved(page, file, text) {
  expect((await send(page, 'tp.write', { name: file, text })).done).toBe(true);
  expect((await send(page, 'media.refresh', {})).done).toBe(true);
}
async function importByMenu(page, name, text) {
  await el(page, 'menu-file').click();
  const [chooser] = await Promise.all([page.waitForEvent('filechooser'), el(page, 'item-import-media').click()]);
  await chooser.setFiles({ name, mimeType: 'application/x-yaml', buffer: Buffer.from(text) });
}

test('file menu has import and saved media', async ({ page }) => {
  await start(page);
  await el(page, 'menu-file').click();
  await expect(el(page, 'item-import-media')).toHaveText(/Import Media…/);   // the product has kinds of medium
  await expect(el(page, 'item-saved-media')).toHaveText(/Saved Media…/);
  await page.mouse.click(5, 5);
  await importByMenu(page, 'Work.testdisk.yaml', disk('Work', 'w'));
  await expect.poll(() => names(page)).toContain('Work');
  await expect.poll(() => inSaveFolder(page, 'Work.testdisk.yaml')).toBe(true);   // in the save folder
  await el(page, 'menu-file').click();
  await el(page, 'item-saved-media').click();
  await expect(el(page, 'media-saved-test-disk-Work')).toBeVisible();
});

test('media in persistent file system after reload', async ({ page }) => {
  await start(page);
  await importByMenu(page, 'Kept.testdisk.yaml', disk('Kept', 'k'));
  await expect.poll(() => names(page)).toContain('Kept');
  await page.waitForTimeout(1000);
  await page.reload();
  await start(page);
  await expect.poll(() => names(page)).toContain('Kept');
  expect(await files(page)).toContain('Kept.testdisk.yaml');
});

test('picker lists the saved instance', async ({ page }) => {
  await start(page);
  await saved(page, 'Mine.testdisk.yaml', disk('Mine', 'm'));
  const picker = el(page, 'media-picker-disk');
  await expect(picker.locator('option', { hasText: 'Mine' })).toHaveCount(1);
  await expect(picker.locator('option').first()).toHaveText('(empty)');
  await picker.selectOption({ label: 'Mine' });
  await expect.poll(async () => {
    const m = await topic(page, 'media');
    const p = (m.places || []).find((x) => x.place === 'disk');
    return p && p.holds ? p.holds.name : null;
  }).toBe('Mine');
});

test('file of another version not listed', async ({ page }) => {
  await start(page);
  await saved(page, 'Old.testdisk.yaml', disk('Old', 'o', 'version-1'));
  await saved(page, 'Mine.testdisk.yaml', disk('Mine', 'm'));   // listed after the other one was read
  const picker = el(page, 'media-picker-disk');
  await expect(picker.locator('option', { hasText: 'Mine' })).toHaveCount(1);
  await expect(picker.locator('option', { hasText: 'Old' })).toHaveCount(0);
});

test('name and save asks the name', async ({ page }) => {
  await start(page);
  await saved(page, 'Blank.testdisk.yaml', disk('Blank', 'b', 'template'));
  await el(page, 'media-picker-disk').selectOption({ label: 'Blank' });
  await expect(el(page, 'media-name-and-save-disk')).toBeVisible();   // a template that can change
  await el(page, 'media-name-and-save-disk').click();
  // The shell's dialog with the product's prompt.
  await expect(el(page, 'name-and-save')).toContainText('Instance name:');
  await expect(el(page, 'media-save')).toBeDisabled();   // no name yet
  await el(page, 'media-name').fill('Named');
  await el(page, 'media-save').click();
  await expect(el(page, 'name-and-save')).toHaveCount(0);   // closed once saved
  await expect.poll(() => inSaveFolder(page, 'Named.testdisk.yaml')).toBe(true);
  await expect.poll(async () => {
    const m = await topic(page, 'media');
    const p = (m.places || []).find((x) => x.place === 'disk');
    return p && p.holds ? p.holds.name : null;
  }).toBe('Named');
});

test('saved media lists downloads deletes after asking', async ({ page }) => {
  await start(page);
  await saved(page, 'Mine.testdisk.yaml', disk('Mine', 'm'));
  await saved(page, 'Blank.testdisk.yaml', disk('Blank', 'b', 'template'));
  await el(page, 'menu-file').click();
  await el(page, 'item-saved-media').click();
  const mine = el(page, 'media-saved-test-disk-Mine');
  await expect(mine).toBeVisible();
  await expect(el(page, 'media-saved-test-disk-Blank')).toContainText('template');   // a template marked
  const [download] = await Promise.all([page.waitForEvent('download'), mine.locator('[data-calcu="media-download"]').click()]);
  expect(download.suggestedFilename()).toBe('Mine.testdisk.yaml');
  expect(fs.readFileSync(await download.path(), 'utf8')).toContain('content: m');
  let asked = '';
  page.once('dialog', (d) => { asked = d.message(); d.accept(); });   // asked first
  await mine.locator('[data-calcu="media-delete"]').click();
  await expect(mine).toHaveCount(0);
  expect(asked).toContain('Mine');
  await expect.poll(() => inSaveFolder(page, 'Mine.testdisk.yaml')).toBe(false);
});
