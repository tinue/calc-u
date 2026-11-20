const { defineConfig, devices } = require('@playwright/test');
const { webTests } = require('../../../../library/web/tools/playwright-config.js');

// The play tests run at the server root and under the sub-path
// (docs/developer/Web-Client.md).
module.exports = defineConfig(webTests(devices, {
  projects: [
    { name: 'root', testMatch: /play|prerender|rom-missing|session|typing/, engine: 'chromium', at: 'root' },
    { name: 'subpath', testMatch: /play/, engine: 'chromium', at: 'subpath' },
    // The page on the React set's shell, in both engines.
    { name: 'shell-chromium', testMatch: /shell/, engine: 'chromium', at: 'root' },
    { name: 'shell-webkit', testMatch: /shell/, engine: 'webkit', at: 'root' },
    // The offline start brings its own server, in both engines: an iPad's
    // installed app is WebKit.
    { name: 'offline-chromium', testMatch: /offline/, engine: 'chromium' },
    { name: 'offline-webkit', testMatch: /offline/, engine: 'webkit' },
  ],
}));
