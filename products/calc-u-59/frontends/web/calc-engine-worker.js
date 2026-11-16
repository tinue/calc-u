// The Calc-U-59 Web Worker entry (LIB-15 design §3.2; M-34): the product's
// configuration for the library's worker (library/web/src/worker.js, built
// into dist/library-worker.js), which hosts the app session of the engine
// (frontends/web/bindings.cpp) and passes LIB-3 messages to the page.
//
// File system: /bundle (the ROM full set) and /presets (the
// bundled presets) from files.json, /uploads (files the user opened), and
// /persist-calc-u-59, the persistent file system in IndexedDB: the settings
// file (settings/Calc-U-59/Calc-U-59.ini) and the save folder (Calc-U-59/:
// the magnetic cards and ti58c.memory.yaml). A mount of its own, so that it never
// shares a store with Calc-U-1600 on one origin (LIB-15 AC-7). One tab at a
// time writes it (Web Locks, `calcu59-storage`; AC-21).
self.CALCU_PRODUCT = {
  name: "Calc-U-59",
  wasm: "ti59-core",
  factory: "createTI59CoreModule",
  host: "CalcU59Host",
  mount: "/persist-calc-u-59",
  lock: "calcu59-storage",
};
importScripts(`${self.CALC_ROOT || ""}library-worker.js`);
