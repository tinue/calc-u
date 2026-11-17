// What the Calc-U-1600 Web tests share (M-49: the page on the React set's
// shell). Elements are found by the shell's names (library/shell/
// ObjectNames.hpp: data-calcu); the session through the test hook, the
// root element's `calcuShell` (LIB-15's createShell): its commands,
// requests and topics.
const { expect } = require('@playwright/test');

const root = '.calcu-client';

async function gotoReady(page, query = '') {
  await page.goto(`./${query}`);
  await expect(page.locator(`${root}[data-ready=true]`)).toHaveCount(1, { timeout: 30000 });
}

// A command or request of the session: its reply {done, reason, value}.
const send = (page, name, args = null) =>
  page.evaluate(([n, a]) => document.querySelector('.calcu-client').calcuShell.send(n, a), [name, args]);
const ask = (page, name, args = null) =>
  page.evaluate(([n, a]) => document.querySelector('.calcu-client').calcuShell.request(n, a), [name, args]);
const topic = (page, name) =>
  page.evaluate((n) => document.querySelector('.calcu-client').calcuShell.getState().topics[n] || null, name);

const screen = async (page) => (await ask(page, 'c1600.display-text')).value || '';
const busy = async (page) => !!((await topic(page, 'busy')) || {}).operationRuns;
const notBusy = (page) => expect.poll(() => busy(page), { timeout: 30000 }).toBe(false);

async function waitForScreen(page, re, timeout = 20000) {
  await expect.poll(() => screen(page), { timeout }).toMatch(re);
}

const c = (name) => `[data-calcu="${name}"]`;

// A menu item by the shell's names: menu(page, 'file', 'load-basic-program')
// or with submenus between: menu(page, 'machine', 'peripherals', 'peripheral-printer-CE-150').
async function menu(page, menuId, ...items) {
  await page.locator(c(`menu-${menuId}`)).click();
  for (const id of items.slice(0, -1)) await page.locator(c(`item-${id}`)).hover();
  await page.locator(c(`item-${items[items.length - 1]}`)).click();
}

// A model from the control bar's model picker; done once the session has it.
async function pickModel(page, model) {
  const picker = page.locator(c('bar-model'));
  const index = await picker.locator('option').evaluateAll((os, m) => os.findIndex((o) => o.textContent === m), model);
  await picker.selectOption(String(index));
  await expect(page.locator(`.calcu-face[data-model="${model}"]`)).toHaveCount(1, { timeout: 30000 });
  // The model switch is an operation; keys typed while it runs are dropped
  // (ledger 1600-9).
  await expect.poll(async () => ((await topic(page, 'control-state')) || {}).model, { timeout: 30000 }).toBe(model);
  await notBusy(page);
}

// The label of the option a picker shows.
const shown = (locator) => locator.evaluate((s) => (s.selectedOptions[0] ? s.selectedOptions[0].textContent : ''));

module.exports = { gotoReady, send, ask, topic, screen, busy, notBusy, waitForScreen, c, menu, pickModel, shown };
