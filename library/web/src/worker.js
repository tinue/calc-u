// The library's Web Worker: loads
// the product's WASM, writes every file of files.json into the file system,
// opens the persistent file system (storage.js), builds the product's
// WorkerHost and calls it about 60 times a second. Everything it passes is a
// session message in its JS form (glue/MessageCodec.hpp); the start failure
// is the state topic `web.start` in the same shape.
//
// A product's calc-engine-worker.js names the product, then loads this file:
//
//   self.CALCU_PRODUCT = { name, wasm, factory, host, mount, lock };
//   importScripts(`${self.CALC_ROOT || ""}library-worker.js`);
//
// File system: /bundle and /presets (files.json), /uploads (picked files),
// <mount> (the persistent file system: settings/<Product>/<Product>.ini and
// the save folder <Product>/).

(function () {
  const CONFIG = self.CALCU_PRODUCT;
  const ROOT = self.CALC_ROOT || "";
  const WASM_DIR = `${ROOT}wasm/`;
  const FRAME_MS = 1000 / 60;

  let Module = null;
  let host = null;
  let loop = null;
  let failed = false;
  let suspended = false;
  const pending = [];

  // `detail`: what only the app log gets (the stack, what was handled).
  const postStart = (state, failure, detail) =>
    postMessage({ kind: "state", topic: "web.start", value: { state, failure: failure || "", detail: detail || "" } });

  function errorText(e) {
    if (Module && typeof WebAssembly !== "undefined" && WebAssembly.Exception && e instanceof WebAssembly.Exception &&
        Module.getExceptionMessage) {
      try {
        const [type, message] = Module.getExceptionMessage(e);
        return message || type;
      } catch (_) { /* fall through */ }
    }
    return String((e && e.message) || e);
  }

  // The app log's part of a failure: where it happened and the stack.
  function errorDetail(where, e) {
    const stack = e && typeof e.stack === "string" ? e.stack : "";
    return stack ? `${where}\n${stack}` : where;
  }

  function stopLoop() {
    if (loop === null) return;
    clearInterval(loop);
    loop = null;
  }

  function fail(why, detail) {
    if (failed) return;
    failed = true;
    stopLoop();
    host = null;
    postStart("failed", why, detail);
  }

  let lastTick = null;
  // The pace of the calls, once a second, once the page asks for it (its
  // `?pace=1` line, shell.js): ticks, the time spent in them, the longest gap.
  let measurePace = false;
  const pace = { since: null, ticks: 0, busy: 0, longest: 0 };
  function tick() {
    if (!measurePace) {
      try {
        host.call();
      } catch (e) {
        fail(errorText(e), errorDetail("in the host's call", e));
      }
      return;
    }
    const start = performance.now();
    if (lastTick !== null) pace.longest = Math.max(pace.longest, start - lastTick);
    lastTick = start;
    try {
      host.call();
    } catch (e) {
      fail(errorText(e), errorDetail("in the host's call", e));
    }
    const end = performance.now();
    pace.ticks += 1;
    pace.busy += end - start;
    if (pace.since === null) pace.since = start;
    if (end - pace.since >= 1000) {
      const secs = (end - pace.since) / 1000;
      postMessage({ kind: "state", topic: "web.pace", value: {
        ticksPerSecond: Math.round(pace.ticks / secs),
        busyPercent: Math.round(pace.busy / (end - pace.since) * 100), longestGapMs: Math.round(pace.longest) } });
      Object.assign(pace, { since: end, ticks: 0, busy: 0, longest: 0 });
    }
  }

  function startLoop() {
    if (loop === null && host && !failed && !suspended) loop = setInterval(tick, FRAME_MS);
  }

  // A message from the page: to the host; suspend and continue also stop and
  // start the calls, the command first.
  function handle(m) {
    if (m.kind === "pace") { measurePace = true; return; }
    if (!host) return;
    try {
      host.receive(m);
    } catch (e) {
      const what = [m.kind, m.name || m.topic, m.call ? `call ${m.call}` : ""].filter(Boolean).join(" ");
      fail(errorText(e), errorDetail(`handling the message ${what}`, e));
      return;
    }
    if (m.kind === "command" && m.name === "suspend") {
      suspended = true;
      stopLoop();
    } else if (m.kind === "command" && m.name === "continue") {
      suspended = false;
      startLoop();
    }
  }

  async function fetchBytes(path) {
    let r;
    try {
      r = await fetch(`${ROOT}${path}`);
    } catch (e) {
      throw new Error(`${path} could not be fetched (${errorText(e)})`);
    }
    if (!r.ok) throw new Error(`${path} could not be fetched (${r.status})`);
    return new Uint8Array(await r.arrayBuffer());
  }

  function writeTree(FS, root, files) {
    for (const [rel, bytes] of files) {
      const parts = `${root}/${rel}`.split("/").filter(Boolean);
      let dir = "";
      for (const part of parts.slice(0, -1)) {
        dir += `/${part}`;
        try { FS.mkdir(dir); } catch (_) { /* exists */ }
      }
      FS.writeFile(`/${parts.join("/")}`, bytes);
    }
  }

  async function init() {
    try {
      importScripts(`${WASM_DIR}${CONFIG.wasm}.js`);
    } catch (e) {
      throw new Error(`wasm/${CONFIG.wasm}.js could not be loaded (${errorText(e)})`);
    }
    const list = JSON.parse(new TextDecoder().decode(await fetchBytes("files.json")));
    const fetchAll = (dir, names) => Promise.all((names || []).map(async (n) => [n, await fetchBytes(`${dir}/${n}`)]));
    let aborted = null;
    const modulePromise = self[CONFIG.factory]({
      locateFile: (path) => `${WASM_DIR}${path}`,
      onAbort: (what) => {
        aborted = Module ? `the program stopped (${what})` : `wasm/${CONFIG.wasm}.wasm did not start (${what})`;
        fail(aborted);
      },
    }).catch((e) => { throw new Error(`wasm/${CONFIG.wasm}.wasm did not start (${errorText(e)})`); });
    const [mod, bundle, presets] = await Promise.all([
      modulePromise, fetchAll("bundle", list.bundle), fetchAll("presets", list.presets),
    ]);
    if (aborted) return;
    Module = mod;
    const FS = Module.FS;
    writeTree(FS, "/bundle", bundle);
    writeTree(FS, "/presets", presets);
    FS.mkdir("/uploads");

    const storage = await calcuOpenStorage(Module, CONFIG, (state) => {
      postMessage({ kind: "state", topic: "web.storage", value: state });
      postMessage({ kind: "stream", topic: "notices", reset: false, value: [{ sentence: state.sentence }] });
    });
    calcuAppLog("debug", `storage: ${CONFIG.mount} opened as ${storage.mode}`);
    host = new Module[CONFIG.host]({
      product: CONFIG.name,
      dirs: { settingsBase: `${CONFIG.mount}/settings`, saveFolder: `${CONFIG.mount}/${CONFIG.name}`, home: "" },
      storage,
      // `?bare=1` on the worker's URL: a fresh install starts bare (the demo and test pages).
      bare: /[?&]bare=1\b/.test(self.location.search),
      // `?app-log-level=` (the page's, shell.js): the app log's level for this run.
      appLogLevel: (/[?&]app-log-level=([a-z]+)/.exec(self.location.search) || [])[1] || "",
      post: (message, transfer) => postMessage(message, transfer),
    });
    const why = host.failure();
    if (why) {
      fail(why);
      return;
    }
    while (pending.length && host) handle(pending.shift());
    startLoop();
  }

  onmessage = (e) => {
    if (failed) return;
    if (!host) pending.push(e.data);
    else handle(e.data);
  };

  if (!CONFIG) fail("The worker was started without a product.");
  else init().catch((e) => fail(errorText(e), errorDetail("while starting", e)));
})();
