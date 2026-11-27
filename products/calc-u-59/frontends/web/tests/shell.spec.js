const { test, expect } = require('@playwright/test');
const { gotoReady: gotoReadyPage } = require('./helpers');

// The page put together from the React set's shell, on the real engine.
// Each case has a twin among the Qt6 window's tests (frontends/qt6/tests/,
// CalcU59QtTests). Elements are found by their data-calcu names.

test.describe.configure({ timeout: 45000 });

const el = (page, name) => page.locator(`[data-calcu="${name}"]`);

// Records the commands the page sends to the worker, by name.
async function recordCommands(page) {
  await page.addInitScript(() => {
    window.__calcuCommands = [];
    const Base = window.Worker;
    window.Worker = class extends Base {
      postMessage(m, t) {
        if (m && m.kind === 'command') window.__calcuCommands.push(m.name);
        return super.postMessage(m, t);
      }
    };
  });
}
const commands = (page) => page.evaluate(() => window.__calcuCommands.slice());

// The shell's page also shows its control bar once ready.
async function gotoReady(page, query) {
  await gotoReadyPage(page, query);
  await expect(el(page, 'control-bar')).toBeVisible();
}

const ids = (locator) => locator.evaluateAll((items) => items.map((i) => i.getAttribute('data-calcu')));
// The ids of a menu's items (data-calcu without the prefix), separators left out.
async function menuIds(page, prefix, scope) {
  const all = await ids((scope || page).locator(`[data-calcu^="${prefix}"]`));
  return all.map((n) => n.slice(prefix.length));
}

// The bar per model.
test('the bar per model', async ({ page }) => {
  await gotoReady(page);
  const model = el(page, 'bar-model');
  await expect(model.locator('option')).toHaveText(['59', '58', '58C']);
  await expect(model).toHaveValue('0');
  // The picker of slot-1: the empty slot and the 14 Solid State ROM modules.
  const slot = el(page, 'media-picker-slot-1');
  await expect(slot.locator('option')).toHaveCount(15);
  await expect(slot.locator('option:checked')).toHaveText(/Master Library/);
  // The button "PC-100C" for the place `printer`, on the paper panel,
  // pressed while plugged in.
  const printer = el(page, 'paper-panel').locator('[data-calcu="bar-place-printer"]');
  await expect(printer).toHaveAttribute('aria-label', 'PC-100C');
  await expect(el(page, 'control-bar').locator('[data-calcu="bar-place-printer"]')).toHaveCount(0);
  await expect(printer).toHaveAttribute('aria-pressed', 'false');
  await printer.click();
  await expect(printer).toHaveAttribute('aria-pressed', 'true', { timeout: 5000 });
  // The card controls on the TI-59 only.
  await expect(el(page, 'bar-media-card-reader')).toBeVisible();
  await expect(page.getByRole('button', { name: 'Arm Card', exact: true })).toBeVisible();
  await model.selectOption({ label: '58' });
  await expect(model).toHaveValue('1', { timeout: 5000 });
  await expect(el(page, 'bar-media-card-reader')).toHaveCount(0);
  await expect(page.getByRole('button', { name: /Arm .*Card/ })).toHaveCount(0);
  await expect(el(page, 'media-picker-slot-1')).toBeVisible();
  await expect(el(page, 'bar-place-printer')).toBeVisible();
});

// A desktop page: the menus, and More at the end of the bar.
test('the desktop page has the menus and more at the end of the bar', async ({ page }) => {
  await recordCommands(page);
  await gotoReady(page);
  const titles = await page.locator('[data-calcu^="menu-"]:not([data-calcu="menu-bar"])').allTextContents();
  expect(titles).toEqual(['File', 'Machine', 'Help']);   // no Edit
  await el(page, 'menu-file').click();
  await expect.poll(() => menuIds(page, 'item-', el(page, 'menu-bar'))).toEqual(
    ['load-preset', 'examples', 'import-media', 'saved-media', 'arm-card', 'arm-fresh-card', 'settings']);   // no Quit
  await expect(el(page, 'item-load-preset')).toHaveText(/Load Preset…/);
  await el(page, 'menu-machine').click();
  await expect.poll(() => menuIds(page, 'item-', el(page, 'menu-bar'))).toEqual(['model', 'peripherals', 'reset', 'cold-boot', 'resume']);
  await expect(el(page, 'item-resume')).toHaveAttribute('aria-disabled', 'true');
  // The bar has More at its end, and no button of Reset, Preset or Settings; More holds every menu's items.
  await page.keyboard.press('Escape');
  await expect(el(page, 'bar-more')).toBeVisible();
  await expect(el(page, 'control-bar').locator('[data-calcu^="bar-action-"]')).toHaveCount(0);
  await el(page, 'bar-more').click();
  await expect.poll(() => menuIds(page, 'more-item-', el(page, 'more-menu'))).toEqual(
    ['heading-file', 'load-preset', 'examples', 'import-media', 'saved-media', 'arm-card', 'arm-fresh-card', 'settings',
      'heading-machine', 'model', 'peripherals', 'reset', 'cold-boot', 'resume', 'heading-help', 'help', 'show-app-log', 'about']);
  await el(page, 'bar-more').click();
  await el(page, 'menu-machine').click();
  await el(page, 'item-cold-boot').click();
  await expect(el(page, 'more-confirm')).toHaveCount(0);   // the menu item asks nothing (the More menu's asks first)
  await expect.poll(() => commands(page)).toContain('cold-boot');
  await el(page, 'menu-help').click();
  await expect.poll(() => menuIds(page, 'item-', el(page, 'menu-bar'))).toEqual(['help', 'show-app-log', 'about']);
  await expect(el(page, 'item-help')).toHaveText(/Calc-U-59 Help/);
  await expect(el(page, 'item-about')).toHaveText(/About Calc-U-59/);
});

// The mobile page: no menu bar, in either layout; the bar keeps the model,
// the module and the card controls, More at its right edge with Load Preset…
// and Settings… in it.
test('the mobile page has no menu bar and more at the right edge', async ({ page }) => {
  await recordCommands(page);
  for (const layout of ['portrait', 'wide']) {
    await gotoReady(page, `layout=${layout}&mobile=1`);
    await expect(el(page, 'menu-bar')).toHaveCount(0);
    const bar = el(page, 'control-bar');
    expect(await ids(bar.locator('[data-calcu^="bar-"]:not([data-calcu^="bar-group-"])'))).toEqual(
      ['bar-model', 'bar-slot-slot-1', 'bar-media-card-reader', 'bar-more']);
    const more = await el(page, 'bar-more').boundingBox();
    const box = await bar.first().boundingBox();
    expect(box.x + box.width - (more.x + more.width)).toBeLessThan(16);   // at the right edge
  }
  await el(page, 'bar-more').click();
  await el(page, 'more-item-reset').click();
  await expect.poll(() => commands(page)).toContain('reset');
  expect(await commands(page)).not.toContain('cold-boot');
  await el(page, 'bar-more').click();
  await el(page, 'more-item-settings').click();
  await expect(el(page, 'settings-dialog')).toBeVisible();
});

// A desktop page in the portrait layout keeps its menu bar.
test('the desktop page has the menu bar in portrait too', async ({ page }) => {
  await gotoReady(page, 'layout=portrait&mobile=0');
  await expect(el(page, 'menu-bar')).toBeVisible();
  await expect(el(page, 'bar-more')).toBeVisible();
});

test('more holds every menu item under its heading', async ({ page }) => {
  await page.setViewportSize({ width: 280, height: 700 });
  await gotoReady(page, 'layout=portrait&mobile=1');
  await el(page, 'bar-more').click();
  const menu = el(page, 'more-menu');
  await expect(menu).toBeVisible();
  await expect.poll(() => menuIds(page, 'more-item-', menu)).toEqual(
    ['heading-file', 'load-preset', 'examples', 'import-media', 'saved-media', 'arm-card', 'arm-fresh-card', 'settings',
      'heading-machine', 'model', 'peripherals', 'reset', 'cold-boot', 'resume', 'heading-help', 'help', 'show-app-log', 'about']);
  await expect(el(page, 'more-item-resume')).toHaveAttribute('aria-disabled', 'true');
  // Inside the client at the narrowest portrait.
  const client = await page.locator('.calcu-client').boundingBox();
  const box = await menu.boundingBox();
  expect(box.x).toBeGreaterThanOrEqual(client.x - 0.5);
  expect(box.x + box.width).toBeLessThanOrEqual(client.x + client.width + 0.5);
  // Examples ▸ opens inline and loads one.
  await el(page, 'more-item-examples').click();
  await el(page, 'more-item-examples-Programs').click();
  await el(page, 'more-item-example-examples:sum.ti59').click();
  await expect(page.locator('.cuecard')).toHaveAttribute('aria-label', /Sum 1\.\.N/, { timeout: 5000 });
});

test('cold boot asks first', async ({ page }) => {
  await recordCommands(page);
  await gotoReady(page, 'layout=portrait&mobile=1');
  await el(page, 'bar-more').click();
  await el(page, 'more-item-cold-boot').click();
  const ask = el(page, 'more-confirm');
  await expect(ask).toContainText('Cold boot the machine? What it holds in memory is lost.');
  await expect(el(page, 'more-confirm-yes')).toHaveText('Cold boot');
  await el(page, 'more-confirm-no').click();   // Cancel sends nothing
  await expect(ask).toHaveCount(0);
  await page.waitForTimeout(300);
  expect(await commands(page)).not.toContain('cold-boot');
  await el(page, 'bar-more').click();
  await el(page, 'more-item-cold-boot').click();
  await el(page, 'more-confirm-yes').click();
  await expect.poll(() => commands(page)).toContain('cold-boot');
});

test('settings prop off', async ({ page }) => {
  await page.goto('./?layout=wide');
  await page.evaluate(() => {
    document.getElementById('root').remove();
    const div = document.createElement('div');
    div.id = 'second';
    document.body.appendChild(div);
    ReactDOM.createRoot(div).render(React.createElement(CalcU59, { layout: 'portrait', width: 360, height: 640, settings: false }));
  });
  const second = page.locator('#second .calcu-client');
  await expect(second.locator('.calcu-key')).toHaveCount(45, { timeout: 15000 });
  await page.waitForTimeout(500);
  if (await second.locator('[data-calcu="notice"]').count()) await second.locator('[data-calcu="notice-ok"]').click();   // a second copy's storage notice
  // No Settings… in More or the menus; Load Preset… stays.
  await second.locator('[data-calcu="bar-more"]').click();
  await expect(second.locator('[data-calcu="more-item-load-preset"]')).toBeVisible();
  await expect(second.locator('[data-calcu="more-item-settings"]')).toHaveCount(0);
  await expect(second.locator('[data-calcu="more-item-about"]')).toBeVisible();
});

// The settings per layout.
async function sections(page) {
  return page.locator('[data-calcu="settings-dialog"] legend').allTextContents();
}
test('the web settings per layout', async ({ page }) => {
  await gotoReady(page);
  await el(page, 'menu-file').click();
  await el(page, 'item-settings').click();
  const dialog = el(page, 'settings-dialog');
  await expect(dialog).toContainText('Settings');
  expect(await sections(page)).toEqual(['General', 'Startup presets', 'Tracing', 'App log']);
  await expect(dialog).toContainText('Startup model');
  await expect(el(page, 'setting-startup-model').locator('option').first()).toHaveText('Last used');
  await expect(el(page, 'setting-general-version')).toHaveText(/^Version \d+\.\d+/);
  for (const m of ['TI-59', 'TI-58', 'TI-58C']) {
    await expect(el(page, `setting-preset-${m}`)).toBeVisible();
    await expect(el(page, `setting-preset-${m}`).locator('option').first()).toHaveText('None');
  }
  await expect(el(page, 'setting-trace-max-file-size-mb')).toBeVisible();
  await expect(dialog).not.toContainText('Solid State Module');
  await expect(dialog).not.toContainText('Keyboard Feedback');    // not in the wide layout
  await expect(dialog.getByRole('button')).toHaveText(['Close']);   // the only button
  // A change is written at once (no Done).
  await el(page, 'setting-startup-model').selectOption({ label: 'TI-58C' });
  await expect(el(page, 'setting-startup-model')).toHaveValue('TI-58C');
  await el(page, 'settings-close').click();

  await gotoReady(page, 'layout=portrait');
  await el(page, 'bar-more').click();
  await el(page, 'more-item-settings').click();
  expect(await sections(page)).toEqual(['General', 'Startup presets', 'Keyboard', 'App log']);
  await expect(el(page, 'setting-keyboard-feedback').locator('option')).toHaveText(['Off', 'Click Sound']);
  await expect(el(page, 'setting-startup-model')).toHaveValue('TI-58C');   // kept
  await expect(el(page, 'setting-trace-max-file-size-mb')).toHaveCount(0);
});

// Settings ▸ Startup presets ▸ Choose…: checked first, then kept in the
// browser's storage (the file cache) and set; over a reload it is the start.
test('a startup preset of my own', async ({ page }) => {
  await gotoReady(page);
  const choose = async (file) => {
    await el(page, 'menu-file').click();
    await el(page, 'item-settings').click();
    const chooser = page.waitForEvent('filechooser');
    await el(page, 'setting-preset-TI-59').selectOption({ label: 'Choose…' });
    await (await chooser).setFiles(file);
  };
  // A preset for another model: refused, the row as it was.
  await choose({ name: 'other.ti58', mimeType: 'text/plain', buffer: Buffer.from('format-version: 2\nmodel: TI-58\n') });
  await expect(el(page, 'notice')).toContainText('cannot be the startup preset of the TI-59');
  await el(page, 'notice-ok').click();
  await expect(el(page, 'setting-preset-TI-59').locator('option:checked')).not.toHaveText('other.ti58');
  await el(page, 'settings-close').click();
  // One for the TI-59: set, shown by its name.
  await choose({ name: 'mine.ti59', mimeType: 'text/plain',
    buffer: Buffer.from('format-version: 2\nmodel: TI-59\n') });
  await expect(el(page, 'setting-preset-TI-59').locator('option:checked')).toHaveText('mine.ti59', { timeout: 30000 });
  await expect(el(page, 'notice')).toHaveCount(0);
  await el(page, 'setting-startup-model').selectOption({ label: 'TI-59' });
  await expect(el(page, 'setting-startup-model')).toHaveValue('TI-59');
  // Calc-U-59.ini reaches the browser's storage (IndexedDB "/persist-calc-u-59") a moment later.
  await expect.poll(() => page.evaluate(() => new Promise((resolve) => {
    const open = indexedDB.open('/persist-calc-u-59');
    open.onsuccess = () => {
      const get = open.result.transaction('FILE_DATA').objectStore('FILE_DATA')
        .get('/persist-calc-u-59/settings/Calc-U-59/Calc-U-59.ini');
      get.onsuccess = () => resolve(get.result && get.result.contents
        ? new TextDecoder().decode(get.result.contents) : '');
      get.onerror = () => resolve('');
    };
    open.onerror = () => resolve('');
  })), { timeout: 10000 }).toMatch(/^model\s*=\s*TI-59$[\s\S]*mine\.ti59|mine\.ti59[\s\S]*^model\s*=\s*TI-59$/m);
  await gotoReady(page);
  await el(page, 'menu-file').click();
  await el(page, 'item-settings').click();
  await expect(el(page, 'setting-preset-TI-59').locator('option:checked')).toHaveText('mine.ti59');
  await expect(el(page, 'notice')).toHaveCount(0);
});

// "Loading…" and the notice dialog.
test('loading and the notice dialog', async ({ page }) => {
  await gotoReady(page);
  const chooser = page.waitForEvent('filechooser');
  await el(page, 'menu-file').click();
  await el(page, 'item-load-preset').click();
  // A preset that runs at full speed far longer than 500 ms of wall time.
  await (await chooser).setFiles({ name: 'long.ti59', mimeType: 'text/plain',
    buffer: Buffer.from('format-version: 2\nmodel: TI-59\nkeys:\n  - wait: 36000s\n') });
  await page.waitForTimeout(250);
  await expect(el(page, 'loading')).toHaveCount(0);   // not before 500 ms
  await expect(el(page, 'loading')).toBeVisible({ timeout: 3000 });
  // It blocks nothing: the model picker and the menus can be used,
  // and Cold boot stops what runs.
  await expect(el(page, 'bar-model')).toBeEnabled();
  await el(page, 'menu-machine').click();
  await expect(el(page, 'item-reset')).toBeVisible();
  await expect(el(page, 'item-cold-boot')).not.toHaveAttribute('aria-disabled', 'true');
  await el(page, 'item-cold-boot').click();
  await expect(el(page, 'loading')).toHaveCount(0, { timeout: 30000 });
  if (await el(page, 'notice').count()) await el(page, 'notice-ok').click();   // the stopped preset's notice, if any
  // A notice is a dialog with OK, its sentence as it is (no title).
  const dt = await page.evaluateHandle(() => { const d = new DataTransfer(); d.items.add(new File(['hello'], 'notes.txt')); return d; });
  await page.locator('.calcu-face').dispatchEvent('drop', { dataTransfer: dt });
  await expect(el(page, 'notice')).toContainText('Calc-U-59 cannot open notes.txt.');
  await expect(el(page, 'notice').locator('.calcu-shell-modal-title')).toHaveCount(0);
  await el(page, 'notice-ok').click();
  await expect(el(page, 'notice')).toHaveCount(0);
  await expect(page.locator('.calcu-status')).toHaveCount(0);   // no status line
});

// Help and About.
test('help and about in the help menu and not in the settings', async ({ page }) => {
  await page.addInitScript(() => {
    window.__opened = [];
    window.open = (url) => { window.__opened.push(url); return null; };
  });
  await gotoReady(page);
  await el(page, 'menu-help').click();
  await el(page, 'item-help').click();
  await expect.poll(() => page.evaluate(() => window.__opened)).toEqual(['https://calc-u.org/59/']);
  await el(page, 'menu-help').click();
  await el(page, 'item-about').click();
  const about = el(page, 'about-dialog');
  await expect(el(page, 'about-name')).toHaveText('Calc-U-59');
  await expect(el(page, 'about-version')).toHaveText(/^Version \d+\.\d+/);
  await expect(about).toContainText('Calc-U-59 Calculator Emulation');
  await expect(about).toContainText('presets, and debugger tools.');
  expect(await about.locator('a').evaluateAll((as) => as.map((a) => a.href))).toEqual([
    'https://github.com/tinue/Calc-U', 'https://github.com/tinue/Calc-U/issues',
    'https://github.com/tinue/Calc-U/blob/main/products/calc-u-59/PRIVACY.md']);
  await el(page, 'about-close').click();
  await el(page, 'menu-file').click();
  await el(page, 'item-settings').click();
  await expect(el(page, 'settings-dialog')).not.toContainText('Calc-U-59 Help');
  await expect(el(page, 'settings-dialog')).not.toContainText('About');
});

// The layouts, one picture each.
test('the layouts portrait wide and auto', async ({ page }, testInfo) => {
  await gotoReady(page, 'layout=wide');
  await expect(page.locator('[data-layout=wide] .calcu-calc')).toBeVisible();
  await expect(page.locator('[data-panel=paper]')).toBeVisible();
  await expect(page.locator('[data-panel=debug]')).toBeVisible();
  await expect(page.locator('.calcu-splitter')).toHaveCount(1);
  await page.locator('.calcu-client').screenshot({ path: testInfo.outputPath('layout-wide.png') });
  await gotoReady(page, 'layout=portrait');
  await expect(page.locator('[data-layout=portrait]')).toBeVisible();
  await expect(page.locator('[data-panel=debug]')).toHaveCount(0);
  await page.locator('.calcu-client').screenshot({ path: testInfo.outputPath('layout-portrait.png') });
  await page.setViewportSize({ width: 440, height: 956 });
  await page.goto('app/');
  await expect(page.locator('.calcu-key')).toHaveCount(45, { timeout: 15000 });
  await expect(page.locator('[data-layout=portrait]')).toBeVisible();   // auto: paged on a phone
  await page.screenshot({ path: testInfo.outputPath('layout-auto.png') });
});

// A key under the pointer.
test('a key under the pointer', async ({ page }) => {
  await gotoReady(page);
  const key = (label) => page.getByRole('button', { name: label, exact: true });
  const r = await key('5').boundingBox();
  // The key's centre.
  await page.mouse.move(r.x + r.width / 2, r.y + r.height / 2);
  await page.mouse.down();
  await expect(key('5')).toHaveAttribute('aria-pressed', 'true');
  await page.mouse.up();
  await expect(key('5')).toHaveAttribute('aria-pressed', 'false');
  // A point in the widened area (above the key's rectangle).
  await page.mouse.move(r.x + r.width / 2, r.y - r.height * 0.2);
  await page.mouse.down();
  await expect(key('5')).toHaveAttribute('aria-pressed', 'true');
  await page.mouse.up();
  // A point in the gap between 5 and 6, beyond both widened areas: no key (distance 0).
  const six = await key('6').boundingBox();
  const gap = (r.x + r.width + six.x) / 2;
  await page.mouse.move(gap, r.y + r.height / 2);
  await page.mouse.down();
  await page.waitForTimeout(300);
  expect(await page.locator('.calcu-key[aria-pressed=true]').count()).toBe(0);
  await page.mouse.up();
});

// Import Media… and Saved Media… in the File menu.
test('file menu has import and saved media', async ({ page }) => {
  await gotoReady(page);
  await el(page, 'menu-file').click();
  await expect(el(page, 'item-import-media')).toHaveText(/Import Media…/);
  await expect(el(page, 'item-saved-media')).toHaveText(/Saved Media…/);
  await el(page, 'item-saved-media').click();
  await expect(el(page, 'saved-media')).toBeVisible();
  await page.keyboard.press('Escape');
  // Portrait: behind the More button.
  await gotoReady(page, 'layout=portrait');
  await el(page, 'bar-more').click();
  await expect(el(page, 'more-item-import-media')).toBeVisible();
  await expect(el(page, 'more-item-saved-media')).toBeVisible();
});

// The paper in its own window (the app page on a desktop): a popup with the
// same panel, the debugger taking the room; closing it puts the paper back,
// and the setting and the place reach the settings file through
// remember-window. Qt6's twin: paper_own_window_debugger_takes_the_room.
test('the paper in its own window and back', async ({ page }) => {
  await page.setViewportSize({ width: 1400, height: 900 });
  await page.goto('app/');
  await expect(page.locator('.calcu-key')).toHaveCount(45, { timeout: 15000 });
  await expect(page.locator('[data-layout=wide]')).toBeVisible();
  const button = page.locator('[data-panel=paper] [data-calcu=paper-window]');
  await expect(button).toHaveAttribute('aria-label', 'Open in Own Window');
  const [popup] = await Promise.all([page.waitForEvent('popup'), button.click()]);
  await expect(popup.locator('[data-calcu=paper-panel]')).toBeVisible();
  await expect(popup.locator('[data-calcu=paper-window]')).toHaveAttribute('aria-label', 'Back to Main Window');
  await expect(page.locator('[data-panel=paper]')).toHaveCount(0);
  await expect(page.locator('[data-panel=debug]')).toBeVisible();
  // The printer attached from the window (the app's start may have it
  // already), its controls there.
  const place = popup.locator('[data-calcu=bar-place-printer]');
  if ((await place.getAttribute('aria-pressed')) !== 'true') await place.click();
  await expect(place).toHaveAttribute('aria-pressed', 'true', { timeout: 5000 });
  await expect(popup.locator('[data-calcu=paper-owner]')).toHaveText('PC-100C');
  await expect(popup.locator('[data-calcu=paper-control-PRINT]')).toBeVisible();
  const setting = () => page.evaluate(() => {
    const st = document.querySelector('.calcu-device').calcuShell.getState().topics.settings;
    return st && st.window ? st.window['paper-own-window'] : null;
  });
  await expect.poll(setting).toBe('true');
  await popup.close();
  await expect(page.locator('[data-panel=paper] [data-calcu=paper-owner]')).toHaveText('PC-100C');
  await expect.poll(setting).toBe('false');
  // In the page, no button on an embed.
  await gotoReady(page, 'layout=wide');
  await expect(page.locator('[data-calcu=paper-window]')).toHaveCount(0);
});
