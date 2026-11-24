// The shared props of every product's
// component, named by library/web/src/embed.js's embedProps, under
// `node --test` (CTest LibraryWebNodeTests, Frequent). The file is run as
// the page runs it, a classic script; embedProps is its global.
const test = require("node:test");
const assert = require("node:assert/strict");
const fs = require("fs");
const path = require("path");
const vm = require("vm");

// The bundle's app log (appLog.js, before embed.js): its warnings recorded.
let warnings = null;
const ctx = vm.createContext({ calcuAppLog: (level, text) => { if (warnings && level === "warn") warnings.push(text); } });
vm.runInContext(fs.readFileSync(path.join(__dirname, "..", "src", "embed.js"), "utf8"), ctx);
const { embedProps } = ctx;

// The product's layouts: embedProps checks
// `layout` against them; another value is ignored with an app log warning
// that names the component, and the product's default is used.
const C59 = { name: "CalcU59", layouts: ["portrait", "wide", "auto"], layout: "portrait" };
const C1600 = { name: "CalcU1600", layouts: ["auto"], layout: "auto" };
function withWarnings(run) {
  const kept = [];
  warnings = kept;
  try { return { result: run(), warnings: kept }; } finally { warnings = null; }
}

test("shared_props_in_their_order", () => {
  const e = embedProps({ height: 2, workerUrl: "w.js", model: "TI-59", preset: "sum.ti59", layout: "wide",
    keyboard: false, width: 1, settings: false, paperWindow: true }, C59);
  assert.deepEqual(Object.keys(e),
    ["width", "keyboard", "layout", "preset", "height", "model", "workerUrl", "paperWindow", "rest"]);
  assert.equal(e.width, 1);
  assert.equal(e.keyboard, false);
  assert.equal(e.layout, "wide");
  assert.equal(e.preset, "sum.ti59");
  assert.equal(e.height, 2);
  assert.equal(e.model, "TI-59");
  assert.equal(e.workerUrl, "w.js");
  assert.equal(e.paperWindow, true);
  // A product's own props stay in rest; layout is a shared one.
  assert.deepEqual(Object.keys(e.rest), ["settings"]);
});

test("no_layout_is_the_products_default", () => {
  // Each product defaults it (Calc-U-59 "portrait", Calc-U-1600 "auto");
  // keyboard stays on by default, the paper's own window off.
  const { result: e, warnings } = withWarnings(() => embedProps({}, C59));
  assert.equal(e.layout, "portrait");
  assert.equal(e.keyboard, true);
  assert.equal(e.paperWindow, false);
  assert.deepEqual(Object.keys(e.rest), []);
  assert.equal(withWarnings(() => embedProps({}, C1600)).result.layout, "auto");
  assert.deepEqual(warnings, []);
});

test("layout_of_the_products_layouts_is_taken", () => {
  const { result, warnings } = withWarnings(() => embedProps({ layout: "auto" }, C59));
  assert.equal(result.layout, "auto");
  assert.deepEqual(warnings, []);
});

test("unknown_layout_warns_and_is_ignored", () => {
  const c1600 = withWarnings(() => embedProps({ layout: "wide" }, C1600));
  assert.equal(c1600.result.layout, "auto");
  assert.deepEqual(c1600.warnings, ['CalcU1600: layout "wide" is not one of its layouts ("auto"); it is ignored.']);
  const c59 = withWarnings(() => embedProps({ layout: "tall" }, C59));
  assert.equal(c59.result.layout, "portrait");
  assert.deepEqual(c59.warnings,
    ['CalcU59: layout "tall" is not one of its layouts ("portrait", "wide", "auto"); it is ignored.']);
});

// `preset` and ?preset= take a path of the bundle with or
// without the prefix `examples:`; the session's start gets the bundle path.
test("preset_with_or_without_examples_prefix", () => {
  assert.equal(ctx.bundlePathOf("sum.ti59"), "examples:sum.ti59");
  assert.equal(ctx.bundlePathOf("examples:plotter/globus.pc1600"), "examples:plotter/globus.pc1600");
  assert.equal(ctx.bundlePathOf(""), "");
  assert.equal(ctx.bundlePathOf(undefined), "");
});
