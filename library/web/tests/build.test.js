// LIB-15 T13 (design §6): library/web/build/dist.js under `node --test`
// (CTest LibraryWebNodeTests, Frequent). Needs only Node: each test puts a
// small dist/ together in a temporary folder.
const test = require("node:test");
const assert = require("node:assert/strict");
const crypto = require("crypto");
const fs = require("fs");
const os = require("os");
const path = require("path");
const vm = require("vm");
const dist = require("../build/dist.js");

const VENDOR = dist.VENDOR;

function tempDist() {
  const dir = fs.mkdtempSync(path.join(os.tmpdir(), "calcu-dist-"));
  const put = (rel, text) => {
    fs.mkdirSync(path.dirname(path.join(dir, rel)), { recursive: true });
    fs.writeFileSync(path.join(dir, rel), text);
  };
  return { dir, put };
}

test("ac14 manifest fields and hashes", () => {
  const { dir, put } = tempDist();
  put("calc.js", "x");
  put("bundle/rom.bin", "rom");
  put("presets/a/b.test", "model: test-seg\n");
  put(".DS_Store", "never shipped");
  const list = dist.writeFilesList(dir);
  assert.deepEqual(list, { bundle: ["rom.bin"], presets: ["a/b.test"] });
  const m = dist.writeManifest(dir, { name: "Test-web", version: "1.2.3", commit: "abc" });
  const written = JSON.parse(fs.readFileSync(path.join(dir, "MANIFEST.json"), "utf8"));
  assert.deepEqual(written, m);
  assert.deepEqual(Object.keys(written), ["name", "version", "commit", "files"]);
  assert.equal(written.name, "Test-web");
  assert.equal(written.version, "1.2.3");
  assert.equal(written.commit, "abc");
  assert.deepEqual(Object.keys(written.files).sort(), ["bundle/rom.bin", "calc.js", "files.json", "presets/a/b.test"]);
  assert.equal(written.files["bundle/rom.bin"], crypto.createHash("sha256").update("rom").digest("hex"));
});

test("ac15 one global, nothing else", () => {
  const { dir, put } = tempDist();
  put("Product.jsx",
    "const helper = 1;\nfunction Product(props) { return <div className=\"calcu-client\">{helper}</div>; }\n" +
    "Product.version = \"__CALC_VERSION__\";\nwindow.Product = Product;\n");
  const text = dist.compileCalcJs({
    vendor: VENDOR, productSources: [path.join(dir, "Product.jsx")], replace: { __CALC_VERSION__: "9.9" },
  });
  const ctx = { URL, React: { createElement: () => ({}) }, document: { currentScript: null }, location: { href: "https://x/" } };
  ctx.window = ctx;
  vm.createContext(ctx);
  const before = new Set(Object.keys(ctx));
  vm.runInContext(text, ctx);
  const added = Object.keys(ctx).filter((k) => !before.has(k));
  assert.deepEqual(added, ["Product"]);
  assert.equal(ctx.Product.version, "9.9");
  // The library's page side is inside the closure.
  for (const name of ["createShell", "embedProps", "useTopic", "CALCU_SENTENCES", "CALC_BASE"])
    assert.ok(text.includes(name), `calc.js holds ${name}`);
});

test("ac17 scope check finds an unscoped selector", () => {
  const css = `
    /* a comment { not a rule } */
    .calcu-client { color: red; }
    .calcu-client .key, .calcu-client > button { margin: 0; }
    @font-face { font-family: "Calc-U Archivo"; src: url(x.woff2); }
    @media (max-width: 600px) { .calcu-client .key { padding: 0; } .calcu-menubar { height: 40px; } }
    body { margin: 0; }
    .calcu-clientele { color: blue; }
    @keyframes blink { from { opacity: 0; } to { opacity: 1; } }
  `;
  assert.deepEqual(dist.checkScopedCss(css), [".calcu-menubar", "body", ".calcu-clientele"]);
  assert.deepEqual(dist.checkScopedCss(fs.readFileSync(path.join(__dirname, "product", "calc.css"), "utf8")), []);
});

test("service worker stamp: every precached file exists, a hash per release", () => {
  const { dir, put } = tempDist();
  for (const f of ["app/index.html", "app/app.js", "app/app.css", "app/engine-worker.js", "app/manifest.json",
    "app/icon.png", "calc.js", "calc.css", "calc-engine-worker.js", "library-worker.js", "audio-worklet.js",
    "wasm/p.wasm", "bundle/rom.bin"]) put(f, f);
  dist.copyAppFiles(dir);
  dist.writeFilesList(dir);
  const assets = dist.stampServiceWorker(dir, { id: "test-a", core: ["wasm/p.wasm"] });
  assert.ok(assets.includes("../bundle/rom.bin"));
  assert.ok(assets.includes("../wasm/p.wasm"));
  const sw = fs.readFileSync(path.join(dir, "app", "sw.js"), "utf8");
  assert.ok(!sw.includes("__CACHE_VERSION__") && !sw.includes("__CACHE_PREFIX_ID__") && !sw.includes("/*__ASSETS__*/"));
  assert.match(sw, /const CACHE_PREFIX = `test-a-app /);

  const other = tempDist();
  other.put("wasm/q.wasm", "q");
  assert.throws(() => {
    dist.copyAppFiles(other.dir);
    dist.writeFilesList(other.dir);
    dist.stampServiceWorker(other.dir, { id: "x", core: ["wasm/q.wasm"] });
  }, /precaches .* which is not in dist/);
});
