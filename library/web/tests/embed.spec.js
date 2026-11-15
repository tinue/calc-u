const { test, expect } = require('@playwright/test');
const { open } = require('./helpers');

// LIB-15 T13, T14 (design §6): the embedding interface on a host page (the
// test product's index.html: React 18 as a global, calc.css, calc.js).

test('ac15 host page with React 18 global mounts CalcUTest', async ({ page }) => {
  await open(page);
  await expect(page.locator('.calcu-client')).toHaveCount(1);
  await expect(page.locator('.calcu-test-title')).toHaveText('Test-A 1.0.0');
  const globals = await page.evaluate(() => ({
    component: typeof window.CalcUTest,
    leaked: ['createShell', 'embedProps', 'useShell', 'useTopic', 'useFillParent', 'CALC_BASE', 'CALCU_SENTENCES',
      'calcuWebHostServices', 'calcuCreateAudio'].filter((k) => k in window),
  }));
  expect(globals).toEqual({ component: 'function', leaked: [] });
});

test('ac17 host styles unchanged', async ({ page }) => {
  await open(page);
  const styles = await page.evaluate(() => {
    const color = (sel) => getComputedStyle(document.querySelector(sel)).color;
    return {
      body: getComputedStyle(document.body).fontFamily,
      bodyColor: getComputedStyle(document.body).color,
      title: color('.host-title'),
      hostButton: color('.host-button'),
      inside: color('.calcu-client button'),
    };
  });
  expect(styles).toEqual({
    body: 'Georgia, serif', bodyColor: 'rgb(1, 2, 3)', title: 'rgb(7, 8, 9)', hostButton: 'rgb(4, 5, 6)',
    inside: 'rgb(10, 20, 30)',
  });
});

test('ac16 props and defaults', async ({ page }) => {
  await open(page, 'Test-A', 'width=300&height=200');
  const client = page.locator('.calcu-client');
  await expect(client).toHaveAttribute('data-keyboard', 'true');   // on by default
  expect(await client.boundingBox()).toMatchObject({ width: 300, height: 200 });
  await expect(client).toHaveAttribute('data-model', 'test-seg');   // the fallback model

  await open(page, 'Test-A', 'keyboard=0&model=test-dot');
  await expect(client).toHaveAttribute('data-keyboard', 'false');
  await expect(client).toHaveAttribute('data-model', 'test-dot');

  await open(page, 'Test-A', 'preset=demo/seg.test&model=test-dot');
  await expect(client).toHaveAttribute('data-model', 'test-seg');   // the preset's model
  await expect(client).toHaveAttribute('data-preset', 'demo/seg.test');

  await open(page, 'Test-A', 'preset=dot.test');
  await expect(client).toHaveAttribute('data-model', 'test-dot');
  await expect(client).toHaveAttribute('data-preset', 'dot.test');

  // workerUrl: the worker's script from a path the page names.
  await open(page, 'Test-A', 'worker=calc-engine-worker.js&model=test-dot');
  await expect(client).toHaveAttribute('data-model', 'test-dot');
});

test('ac16 fills its parent', async ({ page }) => {
  await open(page, 'Test-A', 'fill=1');
  const client = page.locator('.calcu-client');
  await expect(client).toHaveAttribute('data-width', '400');
  await expect(client).toHaveAttribute('data-height', '300');
  expect(await client.boundingBox()).toMatchObject({ width: 400, height: 300 });
});

test('ec4 unknown model and preset give a notice', async ({ page }) => {
  await open(page, 'Test-A', 'model=TI-99');
  await expect(page.locator('.calcu-test-notices')).toContainText('There is no model TI-99.');
  await expect(page.locator('.calcu-client')).toHaveAttribute('data-model', 'test-seg');

  await open(page, 'Test-A', 'preset=examples:dot.test');
  await expect(page.locator('.calcu-test-notices')).toContainText('There is no bundled preset examples:dot.test.');
  await expect(page.locator('.calcu-client')).toHaveAttribute('data-model', 'test-seg');
  await expect(page.locator('.calcu-client')).toHaveAttribute('data-preset', '');

  await open(page, 'Test-A', 'preset=../index.html');
  await expect(page.locator('.calcu-test-notices')).toContainText('There is no bundled preset ../index.html.');
});

test('ec5 changed props are not read again', async ({ page }) => {
  await open(page, 'Test-A', 'model=test-dot');
  const workers = page.workers().length;
  await page.evaluate(() => window.calcuRender({ model: 'test-seg', preset: 'demo/seg.test' }));
  await page.waitForTimeout(500);
  const client = page.locator('.calcu-client');
  await expect(client).toHaveAttribute('data-model', 'test-dot');
  await expect(client).toHaveAttribute('data-preset', '');
  expect(page.workers().length).toBe(workers);
  expect(await page.evaluate(() => window.calcuTestShells.length)).toBe(1);
});
