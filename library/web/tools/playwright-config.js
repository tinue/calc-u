// The Playwright configuration the library's and both products' Web tests
// share; each playwright.config.js names only its projects:
//
//   webTests(devices, { projects, testDir = './tests', ...more config })
//
// A project's `engine` ('chromium', 'webkit') picks the desktop browser and
// `at` its base URL on tools/playwright.sh's servers: 'root' (the folder at a
// server root, CALCU_WEB_PORT) or 'subpath' (only under /anything/calc/ on
// the next port; the client must work unchanged wherever it is hosted).
// `devices` is the caller's: one @playwright/test, the one of its package.

function webTests(devices, { projects, testDir = './tests', ...more }) {
  const port = Number(process.env.CALCU_WEB_PORT);
  if (!port) throw new Error('CALCU_WEB_PORT is not set: run the tests through run-tests.sh');
  const urls = { root: `http://localhost:${port}/`, subpath: `http://localhost:${port + 1}/anything/calc/` };
  const engines = { chromium: devices['Desktop Chrome'], webkit: devices['Desktop Safari'] };
  return {
    testDir,
    // An engine's start in a browser takes seconds; two workers keep a
    // machine's emulation from starving the tests' timers.
    timeout: 60000,
    workers: 2,
    use: { headless: true },
    ...more,
    projects: projects.map(({ engine, at, use, ...project }) => ({
      ...project,
      use: { ...(engine && engines[engine]), ...(at && { baseURL: urls[at] }), ...use },
    })),
  };
}

module.exports = { webTests };
