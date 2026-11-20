// The Calc-U-1600 Web Worker entry: the product's
// configuration for the library's worker (library/web/src/worker.js, built
// into dist/library-worker.js), which hosts the app session of the engine
// (frontends/web/bindings.cpp) and passes the session's messages to the page
// (docs/formats/session-protocol.md).
//
// File system: /bundle (ROMs, bundled cards and disks) and /presets (the
// bundled examples) from files.json, /uploads (files the user opened), and
// /persist, the persistent file system in IndexedDB: the settings file
// (settings/Calc-U-1600/Calc-U-1600.ini) and the save folder (Calc-U-1600/).
// One tab at a time writes it (Web Locks, `calcu1600-storage`).
self.CALCU_PRODUCT = {
  name: "Calc-U-1600",
  wasm: "calcu1600-core",
  factory: "createCalcU1600Module",
  host: "CalcU1600Host",
  mount: "/persist",
  lock: "calcu1600-storage",
};
importScripts(`${self.CALC_ROOT || ""}library-worker.js`);
