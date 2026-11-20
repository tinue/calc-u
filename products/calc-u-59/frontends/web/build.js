#!/usr/bin/env node
//
// What Calc-U-59's Web build has of its own; frontends/web/build.sh runs it,
// the steps are the library's (library/web/build/product.js). The engine
// is core/ + app/ + bindings.cpp (the product's CMakeLists.txt has its
// Emscripten flags). It has no test-only command, so `--release` builds the
// same dist/ as the Playwright tests drive. docs/developer/Web-Client.md is the
// contract for what dist/ contains.

const path = require("path");
const { buildWebProduct, filesIn } = require("../../../../library/web/build/product.js");

const WEB = __dirname;
const PRODUCT = path.join(WEB, "..", "..");
// CALCU59_ROMS_DIR: another ROM folder, as the harness reads it
// (tests/rom_missing_build.sh builds a dist/ without one ROM file).
const ROMS = process.env.CALCU59_ROMS_DIR || path.join(PRODUCT, "roms");
const ASSETS = path.join(PRODUCT, "frontends", "swift", "App", "Assets.xcassets");

buildWebProduct({
  web: WEB,
  name: "Calc-U-59",
  target: "calcu59_web",
  engine: "ti59-core",
  // File system: /bundle (the ROM full set) and /presets (the bundled
  // presets) from files.json, /uploads (files the user opened), and
  // /persist-calc-u-59, the persistent file system in IndexedDB: the settings
  // file (settings/Calc-U-59/Calc-U-59.ini) and the save folder (Calc-U-59/:
  // the magnetic cards and ti58c.memory.yaml). A mount of its own, so that it
  // never shares a store with Calc-U-1600 on one origin. One tab at a time
  // writes it (Web Locks).
  worker: { factory: "createTI59CoreModule", host: "CalcU59Host", mount: "/persist-calc-u-59", lock: "calcu59-storage" },
  files: [
    // The full set of ROM files (the ten calculator dumps and the 14 module
    // files with the module list and cue cards, as roms/fetch_roms.sh
    // fetched them). A missing file is not copied, and the started client
    // names it (tests/rom_missing_build.sh).
    ["bundle/", filesIn(ROMS, /\.txt$|\.rom\.yaml$/)],
    // The calculator artwork and the cue-card templates (one source: the
    // Xcode asset catalogue, as for the Qt app).
    ["img/", ["ti59_base", "ti58_base", "CueCard", "MagnetCard", "SolidStateCard"]
      .map((n) => path.join(ASSETS, `${n}.imageset`, `${n}.png`))],
    ["fonts/", filesIn(path.join(WEB, "fonts"), /./)],
    // The app icon, derived from App/AppIcon.icon by tools/gen_qt_icons.sh.
    ["app/icon.png", path.join(PRODUCT, "frontends", "qt6", "resources", "linux", "calc-u-59.png")],
  ],
  // In load order: helpers before the components that use them. One closure,
  // so nothing but CalcU59 reaches the host page's globals.
  sources: ["page.jsx", "painters.js", "CalculatorFace.jsx", "Dialogs.jsx", "Paper.jsx", "Debugger.jsx", "CalcU59.jsx"],
  component: "CalcU59", props: { layout: "auto" },
  installSentence: "Install Calc-U 59 for one-tap, full-screen launches.",
  swId: "calcu59",   // 2.3.0's cache names: its caches go at the first activation
});
