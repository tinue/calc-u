#!/usr/bin/env node
//
// What Calc-U-1600's Web build has of its own; frontends/web/build.sh runs
// it, the steps are the library's (library/web/build/product.js). The engine
// is core/ + app/ + frontends/web/bindings.cpp. `--release` leaves out the
// test-only commands (app/SharpWebPart.cpp), as the release's zip is built.
// docs/developer/Web-Client.md is the contract for what dist/ contains.

const path = require("path");
const { buildWebProduct, filesIn } = require("../../../../library/web/build/product.js");

const WEB = __dirname;
const PRODUCT = path.join(WEB, "..", "..");
// CALCU1600_ROMS_DIR: another ROM folder (tests/rom_missing_build.sh).
const ROMS = process.env.CALCU1600_ROMS_DIR || path.join(PRODUCT, "roms");
const QT = path.join(PRODUCT, "frontends", "qt6", "resources");

filesIn(ROMS, /\.bin$/i);   // no dump: roms/fetch_roms.sh was not run, stop before the engine's build
buildWebProduct({
  web: WEB,
  name: "Calc-U-1600",
  target: "calcu1600_web",
  engine: "calcu1600-core",
  // File system: /bundle (ROMs, bundled cards and disks) and /presets (the
  // bundled examples) from files.json, /uploads (files the user opened), and
  // /persist, the persistent file system in IndexedDB: the settings file
  // (settings/Calc-U-1600/Calc-U-1600.ini) and the save folder
  // (Calc-U-1600/). One tab at a time
  // writes it (Web Locks).
  worker: { factory: "createCalcU1600Module", host: "CalcU1600Host", mount: "/persist", lock: "calcu1600-storage" },
  files: [
    // What the engine reads as bundled resources (AppPaths::bundledResourcesDir):
    // the ROM full set (the dumps and the CE-502B's module definition) and the
    // bundled card and disk templates, the empty tape's sound too. The
    // host-drive ROM stays out: the web has no host drive.
    ["bundle/", [...filesIn(ROMS, /\.(bin|rom)$|\.module\.yaml$/i),
      ...filesIn(path.join(QT, "cards"), /\.(yaml|wav)$/)]],
    ["img/", filesIn(path.join(QT, "faceplates"), /\.png$/)],
    ["img/icon.png", path.join(QT, "linux", "calc-u-1600.png")],
    ["app/icon.png", path.join(QT, "linux", "calc-u-1600.png")],
  ],
  // In load order; one closure, so nothing but CalcU1600 reaches the page.
  sources: ["painters.js", "Faceplate.jsx", "Panels.jsx", "Dialogs.jsx", "CalcU1600.jsx"],
  component: "CalcU1600", props: { layout: "auto", paperWindow: true },
  installSentence: "Install Calc-U-1600 to run it offline in its own window.",
  swId: "calc-u-1600",
});
