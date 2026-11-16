const { test, expect } = require('@playwright/test');
const h = require('./helpers');
const { c } = h;

// LIB-14 Calc-U-1600 (design Part D, D.8: Q3 to Q10, Q15; LIB-4 T16-6): the
// Web page put together from the React set's shell, found by the shell's
// names (library/shell/ObjectNames.hpp). Chromium and WebKit
// (playwright.config.js: shell-chromium, shell-webkit). The scenario IDs of
// AC-25 are those of the Qt6 cases (CalcU1600QtTests).

test.describe.configure({ timeout: 90000 });

const TALL = { width: 1280, height: 1000 };

async function items(page, menuId) {
  await page.locator(c(`menu-${menuId}`)).click();
  const ids = await page.locator(`${c(`menu-${menuId}`)} + ul > li[data-calcu]`)
    .evaluateAll((ls) => ls.map((l) => l.getAttribute('data-calcu').replace(/^item-/, '')));
  await page.locator(c(`menu-${menuId}`)).click();
  return ids;
}

test('lib14_ac35 the web menus', async ({ page }) => {
  await h.gotoReady(page);
  const menus = await page.locator(`${c('menu-bar')} > div > button`).allTextContents();
  expect(menus).toEqual(['File', 'Edit', 'Machine', 'Help']);
  // Load Preset… and Examples ▸ are two items; the program files (C1600-2);
  // the media items of LIB-7; Settings…; no Mount Directory…, Unmount, Quit.
  expect(await items(page, 'file')).toEqual(['load-preset', 'examples', 'load-basic-program', 'load-machine-code',
    'import-media', 'saved-media', 'settings']);
  expect(await items(page, 'edit')).toEqual(['copy-screen', 'paste-text']);
  expect(await items(page, 'machine')).toEqual(['model', 'peripherals', 'reset', 'cold-boot', 'resume']);
  expect(await items(page, 'help')).toEqual(['help', 'about']);
  // The model's variants on the PC-1500 and the PC-1600 (AC-34's labels).
  await h.pickModel(page, 'PC-1600');
  await page.locator(c('menu-machine')).click();
  await expect(page.locator(c('item-variants-model'))).toContainText('ROM Version');
  await page.locator(c('menu-machine')).click();
  await h.pickModel(page, 'PC-1500');
  await page.locator(c('menu-machine')).click();
  await expect(page.locator(c('item-variants-model'))).toContainText('ROM Revision');
});

// AC-4, D-30 (ledger 1600-35): the page binds no shortcut to a menu item; the
// keys stay the browser's.
test('lib14_ac35 no shortcut is bound', async ({ page }) => {
  await h.gotoReady(page);
  await h.notBusy(page);
  await page.evaluate(() => {
    const shell = document.querySelector('.calcu-client').calcuShell;
    window.__busySeen = false;
    shell.subscribe(() => { const b = shell.getState().topics.busy; if (b && b.operationRuns) window.__busySeen = true; });
  });
  const prevented = await page.evaluate(() => {
    const mac = /Mac|iPhone|iPad|iPod/.test(navigator.platform || navigator.userAgent);
    const root = document.querySelector('.calcu-client');
    return [['r', 'KeyR', false], ['r', 'KeyR', true], [',', 'Comma', false], ['c', 'KeyC', false]].map(([key, code, shift]) => {
      const e = new KeyboardEvent('keydown', { key, code, shiftKey: shift, ctrlKey: !mac, metaKey: mac, bubbles: true, cancelable: true });
      root.dispatchEvent(e);
      return e.defaultPrevented;
    });
  });
  expect(prevented).toEqual([false, false, false, false]);
  await page.waitForTimeout(500);
  expect(await page.evaluate(() => window.__busySeen)).toBe(false);
  await expect(page.locator(c('settings-dialog'))).toHaveCount(0);
});

test('lib14_ac36 the bar per model', async ({ page }) => {
  await page.setViewportSize(TALL);
  await h.gotoReady(page);
  // PC-1500A: the model picker, slot-1 without a number, the buttons CE-150
  // and CE-158; no disk; the tape controls once a CE-150 is plugged in.
  await expect(page.locator(c('bar-model'))).toBeVisible();
  await expect(page.locator(c('bar-slot-slot-1'))).toBeVisible();
  await expect(page.locator(`${c('bar-slot-slot-1')} .calcu-shell-slot-number`)).toHaveCount(0);
  await expect(page.locator(c('bar-slot-slot-2'))).toHaveCount(0);
  await expect(page.locator(c('bar-place-printer'))).toHaveText('CE-150');
  await expect(page.locator(c('bar-place-interface'))).toHaveText('CE-158');
  for (const none of ['bar-place-host-drive', 'bar-place-bus-rom', 'bar-disk', 'bar-media-cassette'])
    await expect(page.locator(c(none))).toHaveCount(0);
  await page.locator(c('bar-place-printer')).click();
  await expect(page.locator(c('bar-place-printer'))).toHaveAttribute('aria-pressed', 'true', { timeout: 30000 });
  await expect(page.locator(c('bar-media-cassette'))).toBeVisible();
  // PC-1600: "1:" and "2:", a printer picker with None, CE-150, CE-1600P,
  // the CE-158's button, the disk controls disabled with the reason.
  await h.pickModel(page, 'PC-1600');
  await expect(page.locator(`${c('bar-slot-slot-1')} .calcu-shell-slot-number`)).toHaveText('1:');
  await expect(page.locator(`${c('bar-slot-slot-2')} .calcu-shell-slot-number`)).toHaveText('2:');
  await expect(page.locator(`${c('bar-place-printer')} option`)).toHaveText(['None', 'CE-150', 'CE-1600P']);
  await expect(page.locator(c('bar-place-interface'))).toHaveText('CE-158');
  const fieldsetDisabled = (n) => page.locator(c(n)).evaluate((e) => e.disabled);
  expect(await fieldsetDisabled('bar-disk')).toBe(true);
  await expect(page.locator(c('bar-disk'))).toHaveAttribute('title', 'The disk drive is in the CE-1600P.');
  await expect(page.locator(c('bar-media-cassette'))).toHaveCount(0);
  await page.locator(c('bar-place-printer')).selectOption({ label: 'CE-1600P' });
  await expect.poll(() => fieldsetDisabled('bar-disk'), { timeout: 30000 }).toBe(false);
  await expect(page.locator(c('bar-media-cassette'))).toBeVisible();
  // The CE-158 beside the CE-1600P: refused, with the machine's reason (AC-5).
  await expect(page.locator(c('bar-place-interface'))).toHaveAttribute('aria-disabled', 'true');
});

test('lib14_ac37 the web settings', async ({ page }) => {
  await h.gotoReady(page);
  await h.menu(page, 'file', 'settings');
  const dialog = page.locator(c('settings-dialog'));
  await expect(dialog).toBeVisible();
  await expect(dialog.locator('.calcu-shell-section-title, legend, h3').first()).toBeVisible();
  const text = await dialog.innerText();
  for (const t of ['General', 'Startup model', 'Startup presets', 'PC-1500', 'PC-1500A', 'PC-1600', 'Storage'])
    expect(text).toContain(t);
  await expect(dialog.locator(c('setting-storage-saved-media'))).toContainText("kept in this browser's storage");
  for (const none of ['Trace folder', 'Save folder', 'Serial ports', 'Debugger', 'folder']) expect(text).not.toContain(none);
  // A startup preset by its description; the model by its name (ledger 1600-35).
  const row = dialog.locator(c('setting-preset-PC-1500A'));
  await row.selectOption({ label: 'CE-150 + CE-163F — PC-1500A with the CE-150 plotter and a CE-163F' }).catch(async () => {
    const labels = await row.locator('option').allTextContents();
    throw new Error(`options: ${labels.join(' | ')}`);
  });
  await dialog.locator(c('setting-startup-model')).selectOption('PC-1600');
  await expect.poll(async () => {
    const rows = ((await h.topic(page, 'settings')) || {}).rows || [];
    const v = (s, k) => (rows.find((r) => r.section === s && r.key === k) || {}).value;
    return [v('startup', 'model'), v('preset', 'PC-1500A')];
  }).toEqual(['PC-1600', 'examples:startup/default-pc1500a.pc1500a']);
  // LIB-9 AC-32: kept in Calc-U-1600.ini over a reload.
  await page.reload();
  await expect(page.locator('.calcu-client[data-ready=true]')).toHaveCount(1, { timeout: 30000 });
  await expect(page.locator('.calcu-face[data-model="PC-1600"]')).toHaveCount(1, { timeout: 30000 });
  const keys = await page.evaluate(() => Object.keys(window.localStorage));
  expect(keys.filter((k) => k.startsWith('calcu1600/'))).toEqual([]);
});

test('lib14_ac39 help and about', async ({ page }) => {
  await page.addInitScript(() => { window.__opened = []; window.open = (u) => { window.__opened.push(u); return null; }; });
  await h.gotoReady(page);
  await h.menu(page, 'help', 'help');
  await expect.poll(() => page.evaluate(() => window.__opened)).toEqual(['https://calc-u.org/1600/']);
  await h.menu(page, 'help', 'about');
  const about = page.locator(c('about-dialog'));
  await expect(about.locator(c('about-name'))).toHaveText('Calc-U-1600');
  await expect(about.locator(c('about-version'))).toContainText('Version 0.');
  await expect(about).toContainText('A Sharp PC-1500/1500A/1600 pocket-computer emulator.');
  await about.locator(c('about-close')).click();
  await expect(about).toHaveCount(0);
});

test('lib14_ac40 panels only with room', async ({ page }) => {
  // A tall window: the faceplate, the bar, then the INSPECTOR and the paper.
  await page.setViewportSize(TALL);
  await h.gotoReady(page);
  const order = await page.locator(`${c('panel-area')} > [data-panel]`).evaluateAll((ps) => ps.map((p) => p.dataset.panel));
  expect(order).toEqual(['inspector', 'paper']);
  const face = await page.locator('.calcu-face').boundingBox();
  const bar = await page.locator(c('control-bar')).boundingBox();
  const panels = await page.locator(c('panel-area')).boundingBox();
  expect(bar.y).toBeGreaterThanOrEqual(face.y + face.height - 1);
  expect(panels.y).toBeGreaterThanOrEqual(bar.y + bar.height - 1);
  // The CE-158's printer panel while a CE-158 is plugged in.
  await page.locator(c('bar-place-interface')).click();
  await expect(page.locator('[data-panel=printer]')).toBeVisible({ timeout: 30000 });
  // A wide, short window: the calculator alone.
  await page.setViewportSize({ width: 1600, height: 700 });
  await expect(page.locator(c('panel-area'))).toHaveCount(0);
});

test('lib14_ac41 copy screen', async ({ page }) => {
  await page.addInitScript(() => {
    window.__copied = null;
    const clip = navigator.clipboard || {};
    Object.defineProperty(navigator, 'clipboard', { configurable: true, value: Object.assign(Object.create(clip), {
      write: async (list) => {
        const out = {};
        for (const item of list) for (const t of item.types) out[t] = await item.getType(t).then((b) => b.size);
        window.__copied = out;
      },
    }) });
  });
  await h.gotoReady(page);
  await h.waitForScreen(page, /NEW0\?/);
  await h.menu(page, 'edit', 'copy-screen');
  await expect.poll(() => page.evaluate(() => window.__copied), { timeout: 10000 }).not.toBeNull();
  const copied = await page.evaluate(() => window.__copied);
  expect(copied['image/png']).toBeGreaterThan(100);
  expect(copied['text/plain']).toBeGreaterThan(0);
});

// AC-24 (ledger 1600-37): a mouse click in the gap below a key, within 24
// pixels, presses that key; the shell's rule, the product's hit map.
test('lib14_ac24 a key under the pointer', async ({ page }) => {
  await page.setViewportSize(TALL);
  await h.gotoReady(page, '?model=PC-1600');
  await h.notBusy(page);
  await h.waitForScreen(page, />/);
  const key = await page.locator('.calcu-key[data-key="5"]').boundingBox();
  await page.mouse.move(key.x + key.width / 2, key.y + key.height + 3);
  await page.mouse.down();
  await expect(page.locator('.calcu-key[data-key="5"]')).toHaveAttribute('aria-pressed', 'true');
  await page.waitForTimeout(150);
  await page.mouse.up();
  await expect(page.locator('.calcu-key[data-key="5"]')).toHaveAttribute('aria-pressed', 'false');
  await h.waitForScreen(page, /5/);
});

// LIB-4 AC-20 (T16-6): while a preset loads, the peripheral buttons and
// pickers, the disk picker and the tape controls are disabled; the model
// picker is not.
test('lib4_ac20 peripheral buttons, disk picker and tape controls disabled while loading', async ({ page }) => {
  await page.setViewportSize(TALL);
  await h.gotoReady(page, '?model=PC-1600');
  await h.notBusy(page);
  // The states the bar showed while the session said busy.
  await page.evaluate(() => {
    const shell = document.querySelector('.calcu-client').calcuShell;
    window.__whileBusy = [];
    const look = () => {
      const b = shell.getState().topics.busy;
      if (!b || !b.operationRuns) return;
      const q = (n) => document.querySelector(`[data-calcu="${n}"]`);
      const disabled = (n) => { const e = q(n); return e ? (e.disabled || e.getAttribute('aria-disabled') === 'true') : null; };
      window.__whileBusy.push({ printer: disabled('bar-place-printer'), iface: disabled('bar-place-interface'),
        disk: disabled('bar-disk'), tape: q('bar-media-cassette') ? !!q('bar-media-cassette').querySelector('fieldset:disabled') : null,
        model: disabled('bar-model') });
    };
    setInterval(look, 50);
  });
  await h.send(page, 'load-preset', { ref: 'examples:plotter/lissajou-1600.pc1600' });
  await h.notBusy(page);
  const seen = await page.evaluate(() => window.__whileBusy.filter((s) => s.disk !== null && s.tape !== null));
  expect(seen.length).toBeGreaterThan(0);
  for (const s of seen) {
    expect(s.disk).toBe(true);
    expect(s.tape).toBe(true);
    expect(s.iface).toBe(true);
    expect(s.model).toBe(false);
  }
});

// C1600-2 AC-15: the two items in File and the files they offer.
test('c1600_2_ac15 the file items', async ({ page }) => {
  await h.gotoReady(page);
  await page.locator(c('menu-file')).click();
  await expect(page.locator(c('item-load-basic-program'))).toHaveText(/Load BASIC Program…/);
  await expect(page.locator(c('item-load-machine-code'))).toHaveText(/Load Machine Code…/);
  await page.locator(c('menu-file')).click();
  for (const [item, accept] of [['load-basic-program', '.bas,.bbin,.wav'], ['load-machine-code', '.bin,.wav']]) {
    const chooser = page.waitForEvent('filechooser');
    await h.menu(page, 'file', item);
    const fc = await chooser;
    expect(await fc.element().evaluate((e) => e.accept)).toBe(accept);
  }
});
