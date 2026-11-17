// LIB-15 AC-29 (release prep 4): the shared props of every product's
// component, named by library/web/src/embed.js's embedProps, under
// `node --test` (CTest LibraryWebNodeTests, Frequent). The file is run as
// the page runs it, a classic script; embedProps is its global.
const test = require("node:test");
const assert = require("node:assert/strict");
const fs = require("fs");
const path = require("path");
const vm = require("vm");

const ctx = vm.createContext({});
vm.runInContext(fs.readFileSync(path.join(__dirname, "..", "src", "embed.js"), "utf8"), ctx);
const { embedProps } = ctx;

test("lib15_ac29_shared_props_in_their_order", () => {
  const e = embedProps({ height: 2, workerUrl: "w.js", model: "TI-59", preset: "sum.ti59", layout: "wide",
    keyboard: false, width: 1, settings: false });
  assert.deepEqual(Object.keys(e), ["width", "keyboard", "layout", "preset", "height", "model", "workerUrl", "rest"]);
  assert.equal(e.width, 1);
  assert.equal(e.keyboard, false);
  assert.equal(e.layout, "wide");
  assert.equal(e.preset, "sum.ti59");
  assert.equal(e.height, 2);
  assert.equal(e.model, "TI-59");
  assert.equal(e.workerUrl, "w.js");
  // A product's own props stay in rest; layout is a shared one now.
  assert.deepEqual(Object.keys(e.rest), ["settings"]);
});

test("lib15_ac29_layout_has_no_library_default", () => {
  // Each product defaults it (Calc-U-59 "portrait", Calc-U-1600 "auto");
  // keyboard stays on by default (AC-16).
  const e = embedProps({});
  assert.equal(e.layout, undefined);
  assert.equal(e.keyboard, true);
  assert.deepEqual(Object.keys(e.rest), []);
});
