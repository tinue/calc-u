const { test, expect } = require('@playwright/test');
const { open, send } = require('./helpers');

// The paper panel of the React component set,
// CalcuPaperPanel, in the test product with the test printers TD-20 (dots
// and a text layer, unit 1/2 mm, 40 units wide) and TP-4 (pen strokes, unit
// 1/10 mm, 600 units wide); Chromium and WebKit.

const UNIT = 10000;   // Coord per unit
const panel = (page) => page.locator('[data-calcu="paper-panel"]');
const el = (page, name) => page.locator(`[data-calcu="${name}"]`);

async function start(page, context, browserName) {
  if (browserName === 'chromium') await context.grantPermissions(['clipboard-read', 'clipboard-write']);
  await open(page, 'Test-A', 'width=480&height=900');
  await expect(panel(page)).toBeVisible();
}

const attachTd20 = (page, online = true) => send(page, 'tp.attach', { place: 'printer', peripheral: 'test-dotter', online });
const attachTp4 = (page) => send(page, 'tp.attach', { place: 'aux', peripheral: 'test-plotter' });
const press = (page, key = '1') => send(page, 'tp.press', { key });
const printerTopic = (page) => page.evaluate(() => window.calcuTestShells[0].getState().topics['tp.printer']);

// The pixel of the paper canvas at a paper position (Coord), as [r, g, b],
// for a paper that starts at y = 0 and is not scrolled.
async function pixelAt(page, width, x, y) {
  return page.evaluate(([w, px, py]) => {
    const c = document.querySelector('[data-calcu="paper-canvas"]');
    const scale = c.clientWidth / w;
    const dpr = c.width / c.clientWidth;
    const d = c.getContext('2d').getImageData(Math.round(px * scale * dpr), Math.round((8 + py * scale) * dpr), 1, 1).data;
    return [d[0], d[1], d[2], d[3]];
  }, [width, x, y]);
}

// The canvas alpha: 0 is nothing drawn.
const dark = (p) => p[3] > 128 && p[0] < 128;

test('owner name and marks at their position', async ({ page, context, browserName }) => {
  await start(page, context, browserName);
  expect((await attachTd20(page)).done).toBe(true);
  expect((await press(page)).done).toBe(true);
  await expect(el(page, 'paper-owner')).toHaveText('TD-20');
  // "KEY 1": K = 0x4B, row 0 has dots in columns 0, 1, 3 of the first character.
  await expect.poll(async () => dark(await pixelAt(page, 40 * UNIT, 2.5 * UNIT, 0.5 * UNIT))).toBe(true);
  expect(dark(await pixelAt(page, 40 * UNIT, 3.5 * UNIT, 0.5 * UNIT))).toBe(true);
  expect(dark(await pixelAt(page, 40 * UNIT, 4.5 * UNIT, 0.5 * UNIT))).toBe(false);
  expect(dark(await pixelAt(page, 40 * UNIT, 5.5 * UNIT, 0.5 * UNIT))).toBe(true);

  // The plotter: key 1 (index 1) draws a blue stroke from (150, 0).
  expect((await send(page, 'tp.detach', { place: 'printer' })).done).toBe(true);
  expect((await attachTp4(page)).done).toBe(true);
  await expect(el(page, 'paper-owner')).toHaveText('TP-4');
  expect((await press(page)).done).toBe(true);
  await expect.poll(async () => {
    const p = await pixelAt(page, 600 * UNIT, 150 * UNIT, 0);
    return p[3] > 128 && p[2] > 150 && p[0] < 100;
  }).toBe(true);
});

test('no owner no name copy and cut disabled', async ({ page, context, browserName }) => {
  await start(page, context, browserName);
  await expect(el(page, 'paper-owner')).toHaveText('');
  await expect(el(page, 'paper-copy')).toBeDisabled();
  await expect(el(page, 'paper-cut')).toBeDisabled();
  // A printer printed and was detached: its paper is kept, not shown.
  await attachTd20(page);
  await press(page);
  await expect(el(page, 'paper-copy')).toBeEnabled();
  await send(page, 'tp.detach', { place: 'printer' });
  await expect(el(page, 'paper-owner')).toHaveText('');
  await expect(el(page, 'paper-copy')).toBeDisabled();
  await expect(el(page, 'paper-cut')).toBeDisabled();
});

test('follows near the end stays when scrolled back', async ({ page, context, browserName }) => {
  await start(page, context, browserName);
  await attachTd20(page);
  const scroller = el(page, 'paper-scroll');
  const metrics = () => scroller.evaluate((s) => ({ top: s.scrollTop, height: s.scrollHeight, view: s.clientHeight }));
  for (let i = 0; i < 8; i++) await press(page);
  await expect.poll(async () => (await metrics()).height).toBeGreaterThan(8 * 7 * 10);
  await expect.poll(async () => { const m = await metrics(); return m.height - (m.top + m.view); }).toBeLessThanOrEqual(1);
  // No scroll bar is drawn.
  expect(await scroller.evaluate((s) => s.offsetWidth - s.clientWidth)).toBe(0);
  // Scrolled back (by the wheel): the view stays where it is.
  await scroller.hover();
  await page.mouse.wheel(0, -100000);
  await expect.poll(async () => (await metrics()).top).toBe(0);
  const before = (await metrics()).height;
  for (let i = 0; i < 3; i++) await press(page);
  await expect.poll(async () => (await metrics()).height).toBeGreaterThan(before);
  expect((await metrics()).top).toBe(0);
  // Back near the end (within 64 points): it follows again.
  await scroller.evaluate((s) => { s.scrollTop = s.scrollHeight - s.clientHeight - 40; s.dispatchEvent(new Event('scroll')); });
  const grown = (await metrics()).height;
  await press(page);
  await expect.poll(async () => (await metrics()).height).toBeGreaterThan(grown);
  await expect.poll(async () => { const m = await metrics(); return m.height - (m.top + m.view); }).toBeLessThanOrEqual(1);
});

test('view control shows the view that is on', async ({ page, context, browserName }) => {
  await start(page, context, browserName);
  await attachTd20(page);
  await press(page);
  await press(page, '2');
  const view = el(page, 'paper-view');
  await expect(view).toHaveText('Marks');
  await expect(view).toHaveAttribute('data-view', 'marks');
  await expect(el(page, 'paper-canvas')).toBeVisible();
  await view.click();
  await expect(view).toHaveText('Text');
  await expect(view).toHaveAttribute('data-view', 'text');
  const text = el(page, 'paper-text');
  await expect(text).toHaveText('KEY 1   \nKEY 2   ', { useInnerText: false });
  const style = await text.evaluate((t) => ({ font: getComputedStyle(t).fontFamily, select: getComputedStyle(t).userSelect || getComputedStyle(t).webkitUserSelect }));
  expect(style.font).toMatch(/mono|Menlo|Consolas|Courier/i);
  expect(style.select).not.toBe('none');
  // The text can be selected.
  const selected = await text.evaluate((t) => {
    const r = document.createRange();
    r.selectNodeContents(t);
    const s = window.getSelection();
    s.removeAllRanges();
    s.addRange(r);
    return s.toString();
  });
  expect(selected).toContain('KEY 2');
  // TP-4 has no text layer: no view control.
  await send(page, 'tp.detach', { place: 'printer' });
  await attachTp4(page);
  await expect(el(page, 'paper-owner')).toHaveText('TP-4');
  await expect(view).toHaveCount(0);
});

test('printer controls held and switched', async ({ page, context, browserName }) => {
  await start(page, context, browserName);
  await attachTd20(page);
  const online = el(page, 'paper-control-online');
  const feed = el(page, 'paper-control-feed');
  await expect(online).toHaveAttribute('aria-pressed', 'true');
  await expect(feed).toHaveAttribute('aria-pressed', 'false');
  // The switch shows its position.
  await online.click();
  await expect(online).toHaveAttribute('aria-pressed', 'false');
  await expect.poll(async () => (await printerTopic(page)).controls.find((c) => c.id === 'online').on).toBe(false);
  await online.click();
  await expect(online).toHaveAttribute('aria-pressed', 'true');
  // The key is down while it is held.
  await feed.hover();
  await page.mouse.down();
  await expect(feed).toHaveAttribute('aria-pressed', 'true');
  await expect.poll(async () => (await printerTopic(page)).controls.find((c) => c.id === 'feed').on).toBe(true);
  await page.mouse.up();
  await expect(feed).toHaveAttribute('aria-pressed', 'false');
  await expect.poll(async () => (await printerTopic(page)).controls.find((c) => c.id === 'feed').on).toBe(false);
  // The feed made the paper longer: Copy can be used.
  await expect(el(page, 'paper-copy')).toBeEnabled();
});

// The row: the control state's labels (the ID where none is declared), the
// short ones below the chrome's compact width (calcuShellPaperRowCompact),
// a switch with the dot of its position, Copy and Cut as named icons; one
// row that never wraps, Copy and Cut where they were with or without a
// printer.
test('the row: labels, compact labels, dot, icons', async ({ page, context, browserName }) => {
  await start(page, context, browserName);
  const cut = el(page, 'paper-cut');
  const cutRight = async () => { const b = await cut.boundingBox(); return b.x + b.width; };
  const before = await cutRight();
  await attachTd20(page);
  const online = el(page, 'paper-control-online');
  const feed = el(page, 'paper-control-feed');
  await expect(feed).toHaveText('FEED');
  await expect(online).toHaveText('online');
  await expect(online.locator('.calcu-paper-dot.calcu-paper-dot-on')).toHaveCount(1);
  await online.click();
  await expect(online.locator('.calcu-paper-dot')).toHaveCount(1);
  await expect(online.locator('.calcu-paper-dot-on')).toHaveCount(0);
  for (const [name, label] of [['paper-copy', 'Copy'], ['paper-cut', 'Cut']]) {
    await expect(el(page, name)).toHaveAttribute('aria-label', label);
    await expect(el(page, name)).toHaveAttribute('title', label);
    await expect(el(page, name).locator('svg')).toHaveCount(1);
    await expect(el(page, name)).toHaveText('');
  }
  expect(await cutRight()).toBe(before);
  // Narrow, with a chrome's compact width: the short labels.
  await expect(panel(page)).toHaveAttribute('data-compact', 'false');
  await page.evaluate(() => {
    const p = document.querySelector('.calcu-test-paper');
    p.style.setProperty('--calcu-paper-row-compact-width', '280');
    p.style.width = '240px';
  });
  await expect(panel(page)).toHaveAttribute('data-compact', 'true');
  await expect(feed).toHaveText('FD');
  await expect(online).toHaveText('online');   // no short label: the label
  const row = page.locator('.calcu-paper-footer');
  const box = await row.boundingBox();
  for (const b of [feed, online, cut]) expect(Math.abs((await b.boundingBox()).y - box.y) < box.height).toBe(true);
  await page.evaluate(() => { document.querySelector('.calcu-test-paper').style.width = ''; });
  await expect(feed).toHaveText('FEED');
});

test('empty paper copy and cut disabled', async ({ page, context, browserName }) => {
  await start(page, context, browserName);
  await attachTd20(page);
  await expect(el(page, 'paper-owner')).toHaveText('TD-20');
  await expect(el(page, 'paper-copy')).toBeDisabled();
  await expect(el(page, 'paper-cut')).toBeDisabled();
  await press(page);
  await expect(el(page, 'paper-copy')).toBeEnabled();
  await expect(el(page, 'paper-cut')).toBeEnabled();
  await el(page, 'paper-cut').click();
  await expect(el(page, 'paper-copy')).toBeDisabled();
  await expect(el(page, 'paper-cut')).toBeDisabled();
});

// Records what reaches the clipboard, passing it on.
function spyClipboard() {
  const clip = navigator.clipboard;
  const write = clip.write.bind(clip);
  window.__clip = [];
  clip.write = (items) => {
    const entry = { types: items.flatMap((i) => i.types), sizes: {}, text: null };
    window.__clip.push(entry);
    for (const t of items[0].types) {
      items[0].getType(t).then(async (b) => {
        entry.sizes[t] = b.size;
        if (t === 'text/plain') entry.text = await b.text();
      }).catch(() => {});
    }
    return write(items);
  };
}

test('copied to clipboard for 1.8 seconds', async ({ page, context, browserName }) => {
  await start(page, context, browserName);
  await page.evaluate(spyClipboard);
  await attachTd20(page);
  await press(page);
  await expect(el(page, 'paper-copy')).toBeEnabled();
  await el(page, 'paper-copy').click();
  const toast = el(page, 'paper-toast');
  await expect(toast).toHaveText('Copied to clipboard');
  const shown = Date.now();
  await expect(toast).toHaveCount(0, { timeout: 5000 });
  const lasted = Date.now() - shown;
  expect(lasted).toBeGreaterThan(1200);
  expect(lasted).toBeLessThan(3000);
  // One clipboard entry with the picture and the text.
  const clip = await page.evaluate(() => window.__clip);
  expect(clip.length).toBe(1);
  expect(clip[0].types.sort()).toEqual(['image/png', 'text/plain']);
  await expect.poll(async () => (await page.evaluate(() => window.__clip[0].sizes['image/png'])) || 0).toBeGreaterThan(50);
  await expect.poll(async () => page.evaluate(() => window.__clip[0].text)).toBe('KEY 1   ');
  // A clipboard that takes nothing: "Copy failed".
  await page.evaluate(() => {
    navigator.clipboard.write = () => Promise.reject(new DOMException('refused', 'NotAllowedError'));
    navigator.clipboard.writeText = () => Promise.reject(new DOMException('refused', 'NotAllowedError'));
  });
  const download = page.waitForEvent('download');
  await el(page, 'paper-copy').click();
  await expect(toast).toHaveText('Copy failed');
  await download;
});

test('picture refused text alone copied', async ({ page, context, browserName }) => {
  await start(page, context, browserName);
  await page.evaluate(() => {
    window.__texts = [];
    navigator.clipboard.write = () => Promise.reject(new DOMException('no pictures', 'NotAllowedError'));
    navigator.clipboard.writeText = (t) => { window.__texts.push(t); return Promise.resolve(); };
  });
  await attachTd20(page);
  await press(page);
  await press(page, '2');
  await expect(el(page, 'paper-copy')).toBeEnabled();
  await el(page, 'paper-copy').click();
  await expect(el(page, 'paper-toast')).toHaveText('Copied to clipboard');
  expect(await page.evaluate(() => window.__texts)).toEqual(['KEY 1   \nKEY 2   ']);
});

test('picture refused offered as download', async ({ page, context, browserName }) => {
  await start(page, context, browserName);
  await page.evaluate(() => {
    navigator.clipboard.write = () => Promise.reject(new DOMException('no pictures', 'NotAllowedError'));
  });
  await attachTp4(page);
  await press(page);
  await expect(el(page, 'paper-copy')).toBeEnabled();
  const download = page.waitForEvent('download');
  await el(page, 'paper-copy').click();
  const d = await download;
  expect(d.suggestedFilename()).toBe('TP-4.png');
  await expect(el(page, 'paper-toast')).toHaveText('Copy failed');
});
