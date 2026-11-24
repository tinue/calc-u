// The parts of a Web frontend's build that every product shares, used by
// each product's build.js
// after the engine and the product's files are in dist/. Node, no npm
// dependencies: the JSX is compiled with the vendored Babel run in Node.
//
//   buildProduct({dist, name, version, css, sources, replace, banner, component,
//                 props, installSentence, bundle, swId, core})
//                                every step below in its order: what a product's
//                                build.js and the test product's call
//   releaseVersion(productDir)   the version release.yaml names (ReleaseIt's rule)
//   compileCalcJs({vendor, librarySources, productSources, replace, out})
//                                one closure, one global
//   compileApp({vendor, component, props, installSentence, out})
//                                dist/app/app.js from library/web/app/app.jsx
//   copyAppFiles(dist)           app/sw.js, app/app.css, app/engine-worker.js, audio-worklet.js
//   writeLibraryWorker(dist)     src/sentences.js + storage.js + worker.js →
//                                dist/library-worker.js
//   writeProductWorker(dist, config)   dist/calc-engine-worker.js: self.CALCU_PRODUCT
//                                (name, wasm, factory, host, mount, lock), then
//                                library-worker.js
//   writeFilesList(dist, {bundle, presets})   dist/files.json
//   stampServiceWorker(dist, {id, core})       app/sw.js: id, list, hash
//   writeManifest(dist, {name, version, commit})   MANIFEST.json
//   checkScopedCss(cssText) → [selectors outside .calcu-client]
//   VENDOR                       library/web/vendor/: React 18, ReactDOM (shipped in
//                                a dist/vendor/), Babel (build only), ReactDOMServer
//                                (tests only); one copy for every product
//   LIBRARY_SOURCES              the page side, in load order
//   COMPONENT_SOURCES            the React component set (library/web/components/),
//                                in load order after LIBRARY_SOURCES; not in
//                                compileCalcJs's default: a product takes it in its
//                                column step (librarySources:
//                                [...LIBRARY_SOURCES, ...COMPONENT_SOURCES])
//   COMPONENT_CSS                the component set's stylesheets (scoped under
//                                .calcu-client), appended to a product's calc.css with it

const fs = require("fs");
const path = require("path");
const vm = require("vm");
const crypto = require("crypto");
const { execFileSync } = require("child_process");

const WEB = path.join(__dirname, "..");   // library/web
const VENDOR = path.join(WEB, "vendor");
const LIBRARY_SOURCES = ["base.js", "canvas.js", "sentences.js", "audio.js", "hostServices.js", "appLog.js", "shell.js", "embed.js"]
  .map((n) => path.join(WEB, "src", n));
const COMPONENT_SOURCES = ["paper.js", "PaperPanel.jsx", "RunControlButtons.jsx", "MediaPicker.jsx", "NameAndSaveDialog.jsx", "SavedMediaDialog.jsx"].map((n) => path.join(WEB, "components", n));
const COMPONENT_CSS = ["paper.css", "media.css"].map((n) => path.join(WEB, "components", n));
// The shell, its models first.
COMPONENT_SOURCES.push(...["shellModel.js", "useShell.jsx", "ShellModal.jsx", "ShellMenuBar.jsx", "ShellControlBar.jsx",
  "ShellSettings.jsx", "ShellAbout.jsx", "ShellNotice.jsx", "ShellLoading.jsx", "ShellPanelArea.jsx",
  "useShellFaceplate.jsx", "ShellRomsMissing.jsx", "ShellMoreMenu.jsx", "ShellStorageBanner.jsx"].map((n) => path.join(WEB, "components", n)));
COMPONENT_CSS.push(path.join(WEB, "components", "shell.css"));
// Host keys to `host-key-down` and the keys held down.
COMPONENT_SOURCES.push(path.join(WEB, "components", "hostKeys.js"));
// The product glue of a page and the library's media in
// the shell (the shell product, the faceplate maps, open and drop, Saved
// Media, the slot picker).
COMPONENT_SOURCES.push(...["useShellProduct.jsx", "ShellMedia.jsx"].map((n) => path.join(WEB, "components", n)));
const WORKER_SOURCES = ["appLog.js", "sentences.js", "storage.js", "worker.js"].map((n) => path.join(WEB, "src", n));

const read = (p) => fs.readFileSync(p, "utf8");
const sha256 = (buf) => crypto.createHash("sha256").update(buf).digest("hex");

function walk(dir) {
  // Dotfiles (macOS .DS_Store, editor droppings) never ship: a host that
  // ignores them would 404 on a listed file.
  if (!fs.existsSync(dir)) return [];
  return fs.readdirSync(dir, { withFileTypes: true }).filter((e) => !e.name.startsWith(".")).flatMap((e) => {
    const p = path.join(dir, e.name);
    return e.isDirectory() ? walk(p) : [p];
  });
}
const rel = (root, f) => path.relative(root, f).split(path.sep).join("/");

const babels = new Map();
function babelFrom(vendor) {
  if (!babels.has(vendor)) {
    const sandbox = vm.createContext({ console });
    vm.runInContext(read(path.join(vendor, "babel.min.js")), sandbox, { filename: "babel.min.js" });
    if (!sandbox.Babel) throw new Error(`${vendor}/babel.min.js did not expose Babel`);
    babels.set(vendor, sandbox.Babel);
  }
  return babels.get(vendor);
}

function compileSource(vendor, file, text) {
  try {
    return babelFrom(vendor).transform(text === undefined ? read(file) : text,
      { presets: ["react"], sourceType: "script", compact: false, comments: true }).code;
  } catch (e) {
    throw new Error(`Babel failed on ${file}: ${e.message}`);
  }
}

function applyReplace(text, replace) {
  let out = text;
  for (const [from, to] of Object.entries(replace || {})) out = out.split(from).join(to);
  return out;
}

// One closure: nothing of the library or the product reaches the host
// page's globals but the component the product sets on window.
function compileCalcJs({ vendor, librarySources = LIBRARY_SOURCES, productSources, replace, banner, out }) {
  const parts = [...librarySources, ...productSources].map((f) =>
    `/* === ${path.basename(f)} === */\n` + compileSource(vendor, f, applyReplace(read(f), replace)));
  const text = `// ${banner || "Generated by library/web/build/dist.js."} Do not edit.\n` +
    "(function () {\n" + parts.join("\n") + "\n})();\n";
  if (out) fs.writeFileSync(out, text);
  return text;
}

function compileApp({ vendor, component, props, installSentence, out }) {
  const source = applyReplace(read(path.join(WEB, "app", "app.jsx")), {
    __CALCU_APP_COMPONENT__: component,
    __CALCU_APP_PROPS__: JSON.stringify(props || {}),
    __CALCU_INSTALL_SENTENCE__: JSON.stringify(installSentence),
  });
  const text = "// Generated from library/web/app/app.jsx by library/web/build/dist.js. Do not edit.\n" +
    compileSource(vendor, "app/app.jsx", source);
  if (out) fs.writeFileSync(out, text);
  return text;
}

function copyAppFiles(dist) {
  fs.mkdirSync(path.join(dist, "app"), { recursive: true });
  fs.copyFileSync(path.join(WEB, "app", "sw.js"), path.join(dist, "app", "sw.js"));
  fs.copyFileSync(path.join(WEB, "app", "app.css"), path.join(dist, "app", "app.css"));
  fs.copyFileSync(path.join(WEB, "app", "engine-worker.js"), path.join(dist, "app", "engine-worker.js"));
  fs.copyFileSync(path.join(WEB, "src", "audio-worklet.js"), path.join(dist, "audio-worklet.js"));
}

function writeLibraryWorker(dist) {
  const text = "// Generated from library/web/src/ by library/web/build/dist.js. Do not edit.\n" +
    WORKER_SOURCES.map((f) => `/* === ${path.basename(f)} === */\n${read(f)}`).join("\n");
  fs.writeFileSync(path.join(dist, "library-worker.js"), text);
  return text;
}

// The worker entry: the product's configuration of the library's worker
// (library/web/src/worker.js, dist/library-worker.js).
function writeProductWorker(out, worker) {
  fs.writeFileSync(path.join(out, "calc-engine-worker.js"),
    `// ${worker.name}'s worker entry, generated by library/web/build/dist.js. Do not edit.\n` +
    `self.CALCU_PRODUCT = ${JSON.stringify(worker, null, 2)};\n` +
    'importScripts(`${self.CALC_ROOT || ""}library-worker.js`);\n');
}

// What the worker writes into its file system: every file below the two
// folders, by its path inside the folder.
function writeFilesList(dist, { bundle = "bundle", presets = "presets" } = {}) {
  const list = {
    bundle: walk(path.join(dist, bundle)).map((f) => rel(path.join(dist, bundle), f)).sort(),
    presets: walk(path.join(dist, presets)).map((f) => rel(path.join(dist, presets), f)).sort(),
  };
  fs.writeFileSync(path.join(dist, "files.json"), JSON.stringify(list, null, 2) + "\n");
  return list;
}

// Every precached path must exist, or the service worker's install fails in
// the browser -- caught here instead. The hash makes a new build a new cache.
function stampServiceWorker(dist, { id, core }) {
  const swFile = path.join(dist, "app", "sw.js");
  const list = JSON.parse(read(path.join(dist, "files.json")));
  const assets = [
    "./", "index.html", "app.js", "app.css", "engine-worker.js", "manifest.json", "icon.png",
    ...core.map((f) => `../${f}`),
    "../calc.js", "../calc.css", "../calc-engine-worker.js", "../library-worker.js", "../audio-worklet.js",
    "../files.json",
    ...list.bundle.map((f) => `../bundle/${f}`),
    ...list.presets.map((f) => `../presets/${f}`),
  ];
  const unique = [...new Set(assets)];
  const hash = crypto.createHash("sha256");
  for (const a of unique) {
    const file = path.join(dist, "app", a === "./" ? "index.html" : a);
    if (!fs.existsSync(file)) throw new Error(`app/sw.js precaches ${a}, which is not in dist/`);
    hash.update(a).update(fs.readFileSync(file));
  }
  const sw = applyReplace(read(swFile), {
    "/*__ASSETS__*/": unique.map((a) => JSON.stringify(a)).join(",\n  "),
    __CACHE_PREFIX_ID__: id,
    __CACHE_VERSION__: hash.digest("hex").slice(0, 16),
  });
  fs.writeFileSync(swFile, sw);
  return unique;
}

// The build ID of a product (tools/build-id.cmake): the commit, pre- while
// its CHANGELOG entry is open, -dirty with changes not committed; the About
// and the Settings' version line show it in parentheses.
function buildId(productDir) {
  try {
    return execFileSync("cmake", [`-DPRODUCT=${productDir}`, "-P", path.join(WEB, "..", "..", "tools", "build-id.cmake")],
                        { encoding: "utf8" }).trim() || "unknown";
  } catch (_) {
    return "unknown";
  }
}

function gitCommit(dir) {
  try {
    const git = (...args) => execFileSync("git", ["-C", dir, ...args], { encoding: "utf8" }).trim();
    return git("rev-parse", "HEAD") + (git("status", "--porcelain") ? "-dirty" : "");
  } catch (_) {
    return "unknown";
  }
}

// The same fields in every product: name, version, commit, and the
// sha256 of every file of dist/ by its path.
function writeManifest(dist, { name, version, commit }) {
  const files = {};
  for (const f of walk(dist).sort()) {
    const r = rel(dist, f);
    if (r !== "MANIFEST.json") files[r] = sha256(fs.readFileSync(f));
  }
  const manifest = { name, version, commit: commit || gitCommit(WEB), files };
  fs.writeFileSync(path.join(dist, "MANIFEST.json"), JSON.stringify(manifest, null, 2) + "\n");
  return manifest;
}

// The selectors of a stylesheet that are not inside .calcu-client.
// At-rules: @media and @supports blocks are looked into; @font-face and
// @keyframes hold no selectors. A selector list is checked per selector.
function checkScopedCss(cssText) {
  const css = cssText.replace(/\/\*[\s\S]*?\*\//g, "");
  const outside = [];
  const scoped = (sel) => /^\.calcu-client(?![\w-])/.test(sel) || /^:root\s+\.calcu-client(?![\w-])/.test(sel);
  const scan = (text) => {
    let i = 0;
    while (i < text.length) {
      const open = text.indexOf("{", i);
      if (open < 0) break;
      const prelude = text.slice(i, open).trim();
      let depth = 1;
      let j = open + 1;
      while (j < text.length && depth > 0) {
        if (text[j] === "{") depth++;
        else if (text[j] === "}") depth--;
        j++;
      }
      const body = text.slice(open + 1, j - 1);
      if (prelude.startsWith("@")) {
        if (/^@(media|supports|container|layer)\b/.test(prelude)) scan(body);
      } else if (prelude) {
        for (const sel of prelude.split(",").map((s) => s.trim()).filter(Boolean))
          if (!scoped(sel)) outside.push(sel);
      }
      i = j;
    }
  };
  scan(css);
  return outside;
}

// The product's version as ReleaseIt reads it: the `version:` block of
// <productDir>/release.yaml names a preset and a file, and the preset's
// pattern's first match is the version (the help tool's rule,
// tools/helptool/manifest.py; the presets the products use).
const VERSION_PRESETS = {
  xcode: /MARKETING_VERSION = ([^;\s]+);/,
  cmake: /^set\(APP_VERSION "([^"]+)"\)/m,
};
function releaseVersion(productDir) {
  const block = (read(path.join(productDir, "release.yaml")).match(/^version:.*\n((?:[ \t]+.*\n?)*)/m) || [])[1] || "";
  const field = (k) => ((block.match(new RegExp(`^\\s+${k}:\\s*(\\S+)`, "m")) || [])[1]);
  const pattern = VERSION_PRESETS[field("preset")];
  const m = pattern && field("file") && read(path.join(productDir, field("file"))).match(pattern);
  if (!m) throw new Error(`${productDir}/release.yaml: no version (preset ${field("preset")}, file ${field("file")})`);
  return m[1].trim();
}

// examples/bundle.yaml and exactly the files it lists, each `{ file: … }`,
// into dist/presets/ (the engine's bundled examples).
function copyPresets(dist, bundleFile) {
  const examples = path.dirname(bundleFile);
  const presets = [...new Set([...read(bundleFile).matchAll(/\{\s*file:\s*([^\s}]+)\s*\}/g)].map((m) => m[1]))];
  for (const f of presets) {
    const from = path.join(examples, f);
    if (!fs.existsSync(from)) throw new Error(`bundle.yaml: ${f} is not in ${path.basename(examples)}/`);
    fs.mkdirSync(path.join(dist, "presets", path.dirname(f)), { recursive: true });
    fs.copyFileSync(from, path.join(dist, "presets", f));
  }
  fs.copyFileSync(bundleFile, path.join(dist, "presets", "bundle.yaml"));
  return presets;
}

// A product's build, after its engine and its static files are in dist/:
// calc.css (the product's stylesheet and the component set's, every
// selector scoped), calc.js (one closure: the library, the component set and
// `sources`, with __CALC_VERSION__ and __CALC_BUILD__ stamped), app/app.js with `component`, the library's app files and
// worker, the presets of `bundle` (a bundle.yaml; none without it),
// files.json, the service worker stamped with `swId` and `core` and
// MANIFEST.json `name`. Returns what it wrote.
function buildProduct({ dist, name, version, build, css, sources, replace, banner, component, props, installSentence, bundle, swId, core }) {
  const cssText = [css, ...COMPONENT_CSS].map(read).join("\n");
  const outside = checkScopedCss(cssText);
  if (outside.length) throw new Error(`calc.css: selectors outside .calcu-client: ${outside.join(", ")}`);
  fs.writeFileSync(path.join(dist, "calc.css"), cssText);
  compileCalcJs({
    vendor: VENDOR,
    librarySources: [...LIBRARY_SOURCES, ...COMPONENT_SOURCES],
    productSources: sources,
    replace: { __CALC_VERSION__: version, __CALC_BUILD__: build || "unknown", ...replace },
    banner,
    out: path.join(dist, "calc.js"),
  });
  compileApp({ vendor: VENDOR, component, props, installSentence, out: path.join(dist, "app", "app.js") });
  copyAppFiles(dist);
  writeLibraryWorker(dist);
  const presets = bundle ? copyPresets(dist, bundle) : [];
  writeFilesList(dist);
  const precached = stampServiceWorker(dist, { id: swId, core });
  const manifest = writeManifest(dist, { name, version });
  console.log(`${name} ${version}: ${presets.length} presets, ${precached.length} precached, ` +
    `${Object.keys(manifest.files).length} files, ${manifest.commit}, in ${dist}`);
  return { presets, precached, manifest };
}

module.exports = {
  VENDOR, LIBRARY_SOURCES, COMPONENT_SOURCES, COMPONENT_CSS, WORKER_SOURCES, walk, compileCalcJs, compileApp, copyAppFiles, writeLibraryWorker, writeProductWorker,
  writeFilesList, stampServiceWorker, writeManifest, checkScopedCss, releaseVersion, buildId, buildProduct,
};
