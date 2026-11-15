const { defineConfig, devices } = require('@playwright/test');

// run-tests.sh serves build/web/library/web/test-dist/ at a server root and,
// on the next port, only under a sub-path (AC-13): every page test runs
// against each, in Chromium, and at the root also in WebKit. The offline
// tests bring their own server, in both engines.
const PORT = Number(process.env.CALCU_LIB_WEB_PORT || 8170);
const PAGES = /(hosting|storage|embed)\.spec\.js/;

module.exports = defineConfig({
  testDir: '.',
  testMatch: /\.spec\.js$/,
  timeout: 60000,
  workers: 2,
  use: { headless: true },
  projects: [
    { name: 'root', testMatch: PAGES, use: { ...devices['Desktop Chrome'], baseURL: `http://localhost:${PORT}/` } },
    { name: 'subpath', testMatch: PAGES, use: { ...devices['Desktop Chrome'], baseURL: `http://localhost:${PORT + 1}/anything/calc/` } },
    { name: 'webkit', testMatch: PAGES, use: { ...devices['Desktop Safari'], baseURL: `http://localhost:${PORT}/` } },
    // LIB-8: the paper panel, at the root, in both engines.
    { name: 'paper-chromium', testMatch: /paper\.spec\.js/, use: { ...devices['Desktop Chrome'], baseURL: `http://localhost:${PORT}/` } },
    { name: 'paper-webkit', testMatch: /paper\.spec\.js/, use: { ...devices['Desktop Safari'], baseURL: `http://localhost:${PORT}/` } },
    // LIB-10: the run-control buttons and the trace download, at the root, in both engines.
    { name: 'run-control-chromium', testMatch: /run-control\.spec\.js/, use: { ...devices['Desktop Chrome'], baseURL: `http://localhost:${PORT}/`, acceptDownloads: true } },
    { name: 'run-control-webkit', testMatch: /run-control\.spec\.js/, use: { ...devices['Desktop Safari'], baseURL: `http://localhost:${PORT}/`, acceptDownloads: true } },
    // LIB-14: the shell of the React set, at the root, in both engines.
    { name: 'shell-chromium', testMatch: /shell\.spec\.js/, use: { ...devices['Desktop Chrome'], baseURL: `http://localhost:${PORT}/` } },
    { name: 'shell-webkit', testMatch: /shell\.spec\.js/, use: { ...devices['Desktop Safari'], baseURL: `http://localhost:${PORT}/` } },
    // LIB-7: the media components on the test product Test-M, at the root, in both engines.
    { name: 'media-chromium', testMatch: /media\.spec\.js/, use: { ...devices['Desktop Chrome'], baseURL: `http://localhost:${PORT}/`, acceptDownloads: true } },
    { name: 'media-webkit', testMatch: /media\.spec\.js/, use: { ...devices['Desktop Safari'], baseURL: `http://localhost:${PORT}/`, acceptDownloads: true } },
    { name: 'prerender', testMatch: /prerender\.spec\.js/ },
    { name: 'offline-chromium', testMatch: /offline\.spec\.js/, use: { ...devices['Desktop Chrome'] } },
    { name: 'offline-webkit', testMatch: /offline\.spec\.js/, use: { ...devices['Desktop Safari'] } },
  ],
});
