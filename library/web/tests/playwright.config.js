const { defineConfig, devices } = require('@playwright/test');
const { webTests } = require('../tools/playwright-config.js');

// Against build/web/library/web/test-dist/: every page test at the root and
// under the sub-path in Chromium, and at the root also in WebKit. The
// offline tests bring their own server, in both engines.
const PAGES = /(hosting|storage|embed)\.spec\.js/;
const downloads = { acceptDownloads: true };

module.exports = defineConfig(webTests(devices, {
  testDir: '.',
  testMatch: /\.spec\.js$/,
  projects: [
    { name: 'root', testMatch: PAGES, engine: 'chromium', at: 'root' },
    { name: 'subpath', testMatch: PAGES, engine: 'chromium', at: 'subpath' },
    { name: 'webkit', testMatch: PAGES, engine: 'webkit', at: 'root' },
    // The paper panel, at the root, in both engines.
    { name: 'paper-chromium', testMatch: /paper\.spec\.js/, engine: 'chromium', at: 'root' },
    { name: 'paper-webkit', testMatch: /paper\.spec\.js/, engine: 'webkit', at: 'root' },
    // The run-control buttons and the trace download, at the root, in both engines.
    { name: 'run-control-chromium', testMatch: /run-control\.spec\.js/, engine: 'chromium', at: 'root', use: downloads },
    { name: 'run-control-webkit', testMatch: /run-control\.spec\.js/, engine: 'webkit', at: 'root', use: downloads },
    // The shell of the React set, at the root, in both engines.
    { name: 'shell-chromium', testMatch: /shell\.spec\.js/, engine: 'chromium', at: 'root' },
    { name: 'shell-webkit', testMatch: /shell\.spec\.js/, engine: 'webkit', at: 'root' },
    // The media components on the test product Test-M, at the root, in both engines.
    { name: 'media-chromium', testMatch: /media\.spec\.js/, engine: 'chromium', at: 'root', use: downloads },
    { name: 'media-webkit', testMatch: /media\.spec\.js/, engine: 'webkit', at: 'root', use: downloads },
    { name: 'prerender', testMatch: /prerender\.spec\.js/ },
    { name: 'offline-chromium', testMatch: /offline\.spec\.js/, engine: 'chromium' },
    { name: 'offline-webkit', testMatch: /offline\.spec\.js/, engine: 'webkit' },
  ],
}));
