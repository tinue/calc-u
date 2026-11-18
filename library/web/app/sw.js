// The service worker of a product's installed app, scope: dist/app/. Precaches the app page and everything the
// product needs offline -- its code and WASM, the files of files.json (the
// ROMs, the bundled presets and media) and its images -- so the installed
// app starts and runs every model without a network.
//
// library/web/build/dist.js stampServiceWorker() fills in the product's id,
// the list (every path relative to this script, each checked to exist) and
// a hash of the files. The scope is part of the cache's name: the origin
// may be shared with the host site or another product.
//
// A new release: the first install takes over at once;
// a later one waits until no window of the app is open, so a running app
// never mixes the files of two releases. Its activate removes the older
// caches of this app; the fetch handler answers from this release's cache
// only. The persistent file system is IndexedDB and is never touched here.
const CACHE_PREFIX = `__CACHE_PREFIX_ID__-app ${self.registration.scope} `;
const CACHE_NAME = CACHE_PREFIX + "__CACHE_VERSION__";

const ASSETS = [
  /*__ASSETS__*/
];

let firstInstall = false;

self.addEventListener("install", (event) => {
  firstInstall = !self.registration.active;
  event.waitUntil(
    caches.open(CACHE_NAME)
      .then((cache) => cache.addAll(ASSETS))
      .then(() => (firstInstall ? self.skipWaiting() : undefined))
  );
});

self.addEventListener("activate", (event) => {
  event.waitUntil(
    caches.keys()
      .then((keys) => Promise.all(
        keys.filter((key) => key.startsWith(CACHE_PREFIX) && key !== CACHE_NAME).map((key) => caches.delete(key))
      ))
      .then(() => (firstInstall ? self.clients.claim() : undefined))
  );
});

// Cache-first from this release's cache: the files are versioned by
// CACHE_NAME, not by URL.
self.addEventListener("fetch", (event) => {
  if (event.request.method !== "GET") return;
  event.respondWith(
    caches.open(CACHE_NAME)
      .then((cache) => cache.match(event.request, { ignoreSearch: true }))
      .then((cached) => cached || fetch(event.request))
  );
});
