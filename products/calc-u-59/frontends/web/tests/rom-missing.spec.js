const { test, expect } = require('@playwright/test');

// LIB-12 on the web client (design §6, P5, P6): the library's worker fetches
// the bundle (dist/files.json: the full set of ROM files) at init and the
// engine looks it up. A bad or missing file shows the lookup's text and no
// calculator (AC-10; D-8: the text in the page until LIB-14/C59-8, since M-34
// the failed start of LIB-15 AC-3); a file the worker cannot fetch at all is
// a failed start too (EC-3).

test.describe.configure({ timeout: 30000 });

const status = (page) => page.locator('.calcu-status');

test('lib12_ac10_bad_rom_file_shows_the_text_and_no_calculator', async ({ page }) => {
  // A truncated response: the file is there but too short.
  await page.route('**/bundle/TMC0583.txt', async (route) => {
    const response = await route.fetch();
    const text = await response.text();
    await route.fulfill({ response, body: text.slice(0, text.length / 2) });
  });
  await page.goto('./?layout=wide');
  await expect(status(page)).toContainText('TMC0583.txt: wrong size', { timeout: 15000 });
  await expect(status(page)).toContainText('The full set of ROM files is needed');
  await expect(page.locator('.calcu-key')).toHaveCount(0);
});

test('lib12_ac10_missing_rom_file_shows_the_text_and_no_calculator', async ({ page }) => {
  await page.route('**/files.json', async (route) => {
    const response = await route.fetch();
    const files = await response.json();
    files.bundle = files.bundle.filter((f) => f !== 'ML.rom.yaml');
    await route.fulfill({ response, json: files });
  });
  await page.goto('./?layout=wide');
  await expect(status(page)).toContainText('ML.rom.yaml: missing; needed by ML', { timeout: 15000 });
  await expect(page.locator('.calcu-key')).toHaveCount(0);
});

test('lib12_ec3_rom_file_that_cannot_be_fetched_is_a_failed_start', async ({ page }) => {
  await page.route('**/bundle/CD2401.txt', (route) => route.abort('failed'));
  await page.goto('./?layout=wide');
  await expect(status(page)).not.toBeEmpty({ timeout: 15000 });
  await expect(page.locator('.calcu-key')).toHaveCount(0);
});

// tests/rom_missing_build.sh (CTest CalcU59RomMissingBuild, Longer): a dist/
// built from a ROM folder without one file; CALCU59_ROM_TAKEN_AWAY names it.
test('lib12_build_with_one_rom_file_taken_away_names_it', async ({ page }) => {
  const missing = process.env.CALCU59_ROM_TAKEN_AWAY;
  test.skip(!missing, 'only from tests/rom_missing_build.sh');
  await page.goto('./?layout=wide');
  await expect(status(page)).toContainText(`${missing}: missing`, { timeout: 15000 });
  await expect(page.locator('.calcu-key')).toHaveCount(0);
});
