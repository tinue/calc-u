const { test, expect } = require('@playwright/test');

// LIB-12 on the Calc-U-1600 web client (design §6, Q4, Q5): the worker
// fetches the bundle (dist/files.json) at init and the engine looks the ROMs
// up there. A bad or missing file of the full set stops the start: the page
// shows the lookup's text and no calculator runs (AC-13; D-8: the text in
// the page until LIB-14).

test.describe.configure({ timeout: 45000 });

const fatal = (page) => page.locator('.calcu-modal-text');
const ready = (page) => page.locator('.calcu-client[data-ready=true]');

test('lib12_ac13_bad_rom_file_shows_the_text_and_no_calculator', async ({ page }) => {
  // A truncated response: the file is there but too short.
  await page.route('**/bundle/PC1600-P1-B4-CE1600P-old.bin', async (route) => {
    const response = await route.fetch();
    const body = await response.body();
    await route.fulfill({ response, body: body.subarray(0, body.length - 1) });
  });
  await page.goto('./');
  await expect(fatal(page)).toContainText('PC1600-P1-B4-CE1600P-old.bin: wrong size', { timeout: 30000 });
  await expect(fatal(page)).toContainText('The full set of ROM files is needed');
  await expect(ready(page)).toHaveCount(0);
});

test('lib12_ac13_missing_rom_file_shows_the_text_and_no_calculator', async ({ page }) => {
  await page.route('**/files.json', async (route) => {
    const response = await route.fetch();
    const files = await response.json();
    files.bundle = files.bundle.filter((f) => f !== 'CE-502B.card.yaml');
    await route.fulfill({ response, json: files });
  });
  await page.goto('./');
  await expect(fatal(page)).toContainText('CE-502B.card.yaml: missing; needed by CE-502B', { timeout: 30000 });
  await expect(ready(page)).toHaveCount(0);
});

// tests/rom_missing_build.sh (CTest CalcU1600RomMissingBuild, Longer): a
// dist/ built from a ROM folder without one file; CALCU1600_ROM_TAKEN_AWAY
// names it.
test('lib12_build_with_one_rom_file_taken_away_names_it', async ({ page }) => {
  const missing = process.env.CALCU1600_ROM_TAKEN_AWAY;
  test.skip(!missing, 'only from tests/rom_missing_build.sh');
  await page.goto('./');
  await expect(fatal(page)).toContainText(`${missing}: missing`, { timeout: 30000 });
  await expect(ready(page)).toHaveCount(0);
});
