// A product's whole Web build, the same steps for every product; its
// frontends/web/build.js names only what is its own and calls
// buildWebProduct, and its frontends/web/build.sh runs that build.js:
//
//   1. the engine: CMake target `target` of the repository's root project,
//      configured with emcmake in build/web (Release), with the option
//      CALCU_WEB_TEST_AIDS on, or off with --release (a release Web build:
//      without the engine's test-only commands; switching compiles only the
//      files the option changes again);
//   2. dist/ (CALCU_WEB_DIST: another folder) made afresh: wasm/<engine>.js
//      and .wasm, the product's `files`, its index.html (the demo page),
//      app/index.html, app.css and manifest.json, React 18 in vendor/ (for
//      the app and the demo page; a host page brings its own) and
//      calc-engine-worker.js from `worker`;
//   3. the library's product build (dist.js buildProduct): calc.css, calc.js
//      from src/<sources>, the app, the presets of examples/bundle.yaml,
//      files.json, the service worker (it precaches vendor/, wasm/, img/
//      and fonts/ besides what it always does) and MANIFEST.json
//      (`<name>-web`, the version of the product's release.yaml).
//
//   buildWebProduct({web, name, target, engine, worker, files, sources,
//                    component, props, installSentence, swId})
//   filesIn(dir, pattern)   the files directly in `dir` whose name matches;
//                           none is an error that names the folder

const fs = require("fs");
const path = require("path");
const { execFileSync } = require("child_process");
const dist = require("./dist.js");

const ROOT = path.join(__dirname, "..", "..", "..");   // the repository's root CMake project
const BUILD = path.join(ROOT, "build", "web");

function fail(message) {
  console.error(`error: ${message}`);
  process.exit(1);
}

function filesIn(dir, pattern) {
  const found = fs.existsSync(dir) ? fs.readdirSync(dir, { withFileTypes: true })
    .filter((e) => e.isFile() && !e.name.startsWith(".") && pattern.test(e.name))
    .map((e) => path.join(dir, e.name)).sort() : [];
  if (!found.length) fail(`no file matching ${pattern} in ${dir}`);
  return found;
}

function releaseBuild(argv) {
  let release = false;
  for (const arg of argv) {
    if (arg === "--release") release = true;
    else {
      console.error("usage: frontends/web/build.sh [--release]");
      process.exit(2);
    }
  }
  return release;
}

function buildEngine(target, aids) {
  for (const tool of ["emcmake", "cmake", "ninja"]) {
    try {
      execFileSync("sh", ["-c", `command -v ${tool}`], { stdio: "ignore" });
    } catch (_) {
      fail(`${tool} not found on PATH.`);
    }
  }
  const option = `-DCALCU_WEB_TEST_AIDS=${aids}`;
  const cache = path.join(BUILD, "CMakeCache.txt");
  if (!fs.existsSync(cache))
    execFileSync("emcmake", ["cmake", "-S", ROOT, "-B", BUILD, "-G", "Ninja", "-DCMAKE_BUILD_TYPE=Release", option], { stdio: ["ignore", "ignore", "inherit"] });
  else if (!fs.readFileSync(cache, "utf8").split("\n").includes(`CALCU_WEB_TEST_AIDS:BOOL=${aids}`))
    execFileSync("emcmake", ["cmake", "-S", ROOT, "-B", BUILD, option], { stdio: ["ignore", "ignore", "inherit"] });
  execFileSync("cmake", ["--build", BUILD, "--target", target], { stdio: "inherit" });
}

// `files`: [to, sources] pairs; `to` ends in "/" for a folder of dist/,
// else it is the one file's path in dist/.
function copyFiles(out, files) {
  for (const [to, sources] of files) {
    const list = [].concat(sources);
    const folder = to.endsWith("/");
    if (!folder && list.length !== 1) fail(`${to}: one source, not ${list.length}`);
    fs.mkdirSync(path.join(out, folder ? to : path.dirname(to)), { recursive: true });
    for (const from of list) {
      if (!fs.existsSync(from)) fail(`${from} not found`);
      fs.copyFileSync(from, path.join(out, folder ? path.join(to, path.basename(from)) : to));
    }
  }
}

function buildWebProduct({ web, name, target, engine, worker, files, sources, component, props, installSentence, swId }) {
  const product = path.join(web, "..", "..");
  const out = path.resolve(process.env.CALCU_WEB_DIST || path.join(web, "dist"));
  buildEngine(target, releaseBuild(process.argv.slice(2)) ? "OFF" : "ON");

  fs.rmSync(out, { recursive: true, force: true });
  const engineDir = path.join(BUILD, path.relative(ROOT, product));
  copyFiles(out, [
    ["wasm/", [`${engine}.js`, `${engine}.wasm`].map((f) => path.join(engineDir, f))],
    ["vendor/", ["react.production.min.js", "react-dom.production.min.js"].map((f) => path.join(dist.VENDOR, f))],
    ["index.html", path.join(web, "index.html")],
    ["app/", ["index.html", "manifest.json"].map((f) => path.join(web, "app", f))],
    ...files,
  ]);
  dist.writeProductWorker(out, { name, wasm: engine, ...worker });

  const precached = (dir) => dist.walk(path.join(out, dir)).map((f) => path.relative(out, f).split(path.sep).join("/"));
  dist.buildProduct({
    dist: out,
    name: `${name}-web`,
    version: dist.releaseVersion(product),
    build: dist.buildId(product),
    buildTime: dist.buildTime(),
    css: path.join(web, "calc.css"),
    sources: sources.map((n) => path.join(web, "src", n)),
    banner: `${name} web client -- generated by frontends/web/build.sh from frontends/web/src/ and library/web/src/.`,
    component, props, installSentence,
    bundle: path.join(product, "examples", "bundle.yaml"),
    swId,
    core: ["vendor", "wasm", "img", "fonts"].flatMap(precached),
  });
  console.log(`wrote ${out}`);
}

module.exports = { buildWebProduct, filesIn };
