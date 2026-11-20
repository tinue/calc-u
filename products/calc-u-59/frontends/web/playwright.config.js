const { defineConfig, devices } = require('@playwright/test');
const { webTests } = require('../../../../library/web/tools/playwright-config.js');

// Every test but the offline start runs at the server root and under the
// sub-path (reference/WebClient.md).
module.exports = defineConfig(webTests(devices, {
  projects: [
    { name: 'root', testIgnore: 'offline.spec.js', engine: 'chromium', at: 'root' },
    { name: 'subpath', testIgnore: 'offline.spec.js', engine: 'chromium', at: 'subpath' },
    // The offline start brings its own server, and runs in both engines: the
    // iPhone's installed app is WebKit.
    { name: 'offline-chromium', testMatch: 'offline.spec.js', engine: 'chromium' },
    { name: 'offline-webkit', testMatch: 'offline.spec.js', engine: 'webkit' },
  ],
}));
