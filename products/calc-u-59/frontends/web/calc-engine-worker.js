// Background Worker of the web client (src/engine.js starts it). It owns the
// WASM WebController (Web/bindings.cpp) — the real cu59::SessionController
// from Shared/, run single-threaded — and calls its 60 Hz tick. Everything
// that isn't drawing happens in there; this file only fetches files and
// passes messages. Every URL here is relative to dist/, so the client works
// wherever it is hosted: to this script, or to CALC_ROOT when started through
// app/engine-worker.js.
//
// Protocol (postMessage), main thread -> worker:
//   {type: "init", settings, model, module}   -- first message: stored settings
//        by key (strings), the host's model/module overrides ("" for none)
//   {type: "call", method, args, id}   -- a WebController method (bindings.cpp);
//        with an id, its result comes back as reply
//   {type: "loadState", text}          -- a preset's contents -> stateLoaded
//   {type: "dropFile", file}           -- a dropped File, read only if its name is
//        accepted -> stateLoaded, dropRejected (neither a preset nor a card), or
//        nothing (a card, and this model has no reader)
//   {type: "cardBytes", name}          -- -> {type: "cardFile", name, bytes}
//   {type: "traceBytes", name}         -- -> {type: "traceFile", name, bytes}
//   {type: "geometry", w, h}           -- LED shapes for a w × h digit cell
//   {type: "pause"} / {type: "resume"} -- document.visibilitychange
//
// worker -> main thread:
//   {type: "ready", data}              -- staticData(): models, modules, layout,
//                                         printer geometry and font, heatmap
//   {type: "stateLoaded", errors}
//   {type: "dropRejected", name}
//   {type: "reply", id, value}
//   {type: "geometry", geometry}
//   {type: "cardFile" | "traceFile", name, bytes}
//   {type: "romsMissing", message}     -- a file of the full set is missing or bad
//                                         (LIB-12 AC-10): no calculator starts
//   {type: "error", message}           -- also: a ROM file could not be fetched
//                                         (LIB-12 EC-3, a failed start)
//   and what WebController posts on its own (bindings.cpp drain()):
//   model, display, cueCard, runState, printer, card, live, cpu, heatmap,
//   log, asm, traces, traceReady, settings, store, debugFont, debugPanel, error.

const WASM_DIR = `${self.CALC_ROOT || ""}wasm/`;

importScripts(`${WASM_DIR}ti59-core.js`);

const FRAME_MS = 1000 / 60;

let Module = null;
let controller = null;
let intervalId = null;
const pending = [];

function fetchText(name) {
  return fetch(`${WASM_DIR}roms/${name}`).then((r) => {
    if (!r.ok) throw new Error(`${name}: ${r.status}`);
    return r.text();
  });
}

// Every ROM file of the full set (Module.romFileNames(), LIB-12 AC-4, D-10)
// into Emscripten's /roms, as fetched; the WebController looks them up there.
// A file that cannot be fetched is a failed start (EC-3). A missing file
// (404) is written as nothing: the lookup names it.
async function fetchRoms(names) {
  const responses = await Promise.all(names.map((n) => fetch(`${WASM_DIR}roms/${n}`)));
  const bytes = await Promise.all(responses.map((r) => (r.ok ? r.arrayBuffer() : null)));
  Module.FS.mkdir("/roms");
  names.forEach((n, i) => { if (bytes[i]) Module.FS.writeFile(`/roms/${n}`, new Uint8Array(bytes[i])); });
}

function startLoop() {
  if (intervalId === null) intervalId = setInterval(() => controller.tick(), FRAME_MS);
}

function stopLoop() {
  if (intervalId === null) return;
  clearInterval(intervalId);
  intervalId = null;
}

function handle(msg) {
  switch (msg.type) {
    case "call": {
      const value = controller[msg.method](...(msg.args || []));
      if (msg.id) postMessage({ type: "reply", id: msg.id, value });
      break;
    }
    case "loadState": {
      const errors = controller.loadState(new TextEncoder().encode(msg.text));
      postMessage({ type: "stateLoaded", errors });
      break;
    }
    case "dropFile": {
      const { name } = msg.file, action = controller.dropAction(name);
      if (action === "notAccepted") {
        postMessage({ type: "dropRejected", name });
      } else if (action !== "ignore") {
        msg.file.arrayBuffer().then((b) => {
          postMessage({ type: "stateLoaded", errors: controller.openDroppedFile(name, new Uint8Array(b)) });
        });
      }
      break;
    }
    case "cardBytes":
      postMessage({ type: "cardFile", name: msg.name, bytes: controller.cardBytes(msg.name) });
      break;
    case "traceBytes":
      postMessage({ type: "traceFile", name: msg.name, bytes: controller.traceBytes(msg.name) });
      break;
    case "geometry":
      postMessage({ type: "geometry", geometry: Module.ledGeometry(msg.w, msg.h) });
      break;
    case "pause": controller.suspend(); stopLoop(); break;
    case "resume": controller.resume(); startLoop(); break;
    default: break;
  }
}

async function init({ settings, model, module }) {
  // ti59-core.js resolves its .wasm next to this worker, not next to itself
  // (wasm/): locateFile points it back.
  const [mod, cuecards] = await Promise.all([
    createTI59CoreModule({ locateFile: (path) => `${WASM_DIR}${path}` }),
    fetchText("cuecards.txt"),
  ]);
  Module = mod;
  const plan = Module.startupPlan(settings, model || "", module || "", cuecards);
  await fetchRoms(Module.romFileNames());
  const started = new Module.WebController(cuecards, settings, plan.model, plan.moduleID);
  if (!started.started()) return;   // it posted romsMissing
  postMessage({ type: "ready", data: Module.staticData(started) });
  controller = started;

  startLoop();
  while (pending.length) handle(pending.shift());
}

onmessage = (e) => {
  if (e.data.type === "init") {
    init(e.data).catch((err) => postMessage({ type: "error", message: String(err && err.message || err) }));
  } else if (!controller) {
    pending.push(e.data);
  } else {
    handle(e.data);
  }
};
