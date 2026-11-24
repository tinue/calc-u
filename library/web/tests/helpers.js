// What the shell's Playwright tests share: opening the test product's host
// page, its shells (window.calcuTestShells, src/CalcUTest.jsx), and (from
// dist-helpers.js, which the products' tests use too) a server of a dist/ of
// its own for the offline tests.
const { expect } = require('@playwright/test');
const path = require('path');
const { serveDist, stop } = require('./dist-helpers');

const DIST = process.env.CALCU_TEST_DIST ||
  path.join(__dirname, '..', '..', '..', 'build', 'web', 'library', 'web', 'test-dist');

const SENTENCES = {
  secondCopy: (p) => `${p} is open in another tab or window: what you save here is not kept.`,
  unreadable: (p) => `${p} could not read its saved files: they are left as they are, and what you save here is not kept.`,
  unavailable: (p) => `This browser keeps no files for ${p} here: what you save is not kept.`,
};

// The host page of a product, started: `query` is the page's search string.
async function open(page, product = 'Test-A', query = '', state = 'running') {
  await page.goto(`${product}/index.html${query ? `?${query}` : ''}`);
  await expect(page.locator('.calcu-client').first()).toHaveAttribute('data-start', state, { timeout: 30000 });
  if (state === 'running') await expect(page.locator('.calcu-client').first()).not.toHaveAttribute('data-model', '');
}

// The app log's lines on the page's console from now on, as [{type, text}].
function consoleLines(page) {
  const lines = [];
  page.on('console', (m) => { if (m.text().startsWith('[Calc-U] ')) lines.push({ type: m.type(), text: m.text() }); });
  return lines;
}

const send = (page, name, args, i = 0) =>
  page.evaluate(([n, a, k]) => window.calcuTestShells[k].send(n, a), [name, args === undefined ? null : args, i]);
const request = (page, name, args, i = 0) =>
  page.evaluate(([n, a, k]) => window.calcuTestShells[k].request(n, a), [name, args === undefined ? null : args, i]);

async function files(page, i = 0) {
  const reply = await page.evaluate((k) => window.calcuTestShells[k].getState().topics['tp.files'], i);
  return (reply && reply.files) || [];
}

module.exports = { DIST, SENTENCES, consoleLines, open, send, request, files, serveDist, stop };
