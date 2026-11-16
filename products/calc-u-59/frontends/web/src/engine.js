// The components' view of the library's Web shell (LIB-15 59, M-34):
// createShell() starts the library's worker, which hosts the app session
// (frontends/web/bindings.cpp, app/TIAppPart, app/TIWebPart); this file only
// translates -- a component's call or request to a LIB-3 command or request
// of the session, and the topics the session sends to the slices the
// components read (spec AC-22). It decides nothing. Until LIB-14 59 (M-57)
// rewrites the components on the family's component set, they keep the
// names they had (engine.call("startMachine", 1)), as Calc-U-1600's do.
//
// Since M-54 the start, the model switch, Reset, Cold boot, the module and
// the printer are LIB-18's commands (`start`, `model`, `reset`, `cold-boot`,
// `attach`, `detach`) and a preset is LIB-6's `load-preset` (a bundled
// example by its bundle path `examples:<path>`, an uploaded one by its
// content); the preset list is LIB-6's request `preset-list`.
//
// The LED canvas repaints from `frames` (the `c59.web.display` topic), once
// per animation frame, without a React render. Created in an effect only (a
// host page may prerender without a Worker). No entry of the page's storage (LIB-9 AC-23):
// the settings, the cards and ti58c.memory.yaml are files of the persistent file
// system the worker keeps (LIB-15 AC-5, AC-20).

const PRODUCT = "Calc-U-59";

const flagOf = (r) => !!(r.done && r.value);
// The models by their index in 2.3.0's picker (0 TI-59, 1 TI-58, 2 TI-58C).
const MODEL_NAMES = ["TI-59", "TI-58", "TI-58C"];
const valueOf = (r) => r.value;

// Each method of 2.3.0's engine: [kind, LIB-3 name, args → object, reply → result].
const METHODS = {
  // LIB-18 (M-54): the library's machine control.
  startMachine: ["command", "model", (model) => ({ name: MODEL_NAMES[model] || String(model) })],
  resetMachine: ["command", "reset", () => null],
  cleanResetMachine: ["command", "cold-boot", () => null],
  changeModule: ["command", (id) => (id ? "attach" : "detach"), (id) => (id ? { place: "slot-1", peripheral: id } : { place: "slot-1" })],
  setStartupModel: ["command", "c59.web.set-startup-model", (model) => ({ model })],
  setTraceMaxMB: ["command", "c59.web.set-trace-max-mb", (mb) => ({ mb })],
  setKeyboardFeedback: ["command", "c59.web.set-keyboard-feedback", (mode) => ({ mode })],
  pointerDown: ["command", "c59.web.pointer-down", (x, y) => ({ x, y })],
  pointerMove: ["command", "c59.web.pointer-move", (x, y) => ({ x, y })],
  pointerUp: ["command", "c59.web.pointer-up", () => null],
  keyButtonDown: ["command", "c59.web.key-button-down", (row, col) => ({ row, col })],
  keyButtonUp: ["command", "c59.web.key-button-up", () => null],
  // LIB-5: the host key as the browser reports it; the worker's keyboard
  // map (app/TITyping) decides what it presses.
  keyDown: ["command", "host-key-down", (event) => event],
  keyUp: ["command", "host-key-up", (physical) => ({ physical })],
  blur: ["command", "c59.web.blur", () => null],
  fitCard: ["command", "c59.web.fit-card", (name, bytes) => (bytes ? { name, bytes } : { name }), flagOf],
  fitBlankCard: ["command", "c59.web.fit-blank-card", (name) => ({ name }), flagOf],
  freshCardFileName: ["request", "c59.web.fresh-card-name", (name) => ({ name }), valueOf],
  feedSide: ["command", "c59.feed-side", (side) => ({ side })],
  ejectCard: ["command", "c59.eject-card", () => null],
  // LIB-18: the PC-100C attached or detached, as the printer topic says it is now.
  togglePrinterConnection: ["command", (connected) => (connected ? "detach" : "attach"),
    (connected) => (connected ? { place: "printer" } : { place: "printer", peripheral: "PC-100C" })],
  // LIB-6 (M-54): the bundled presets (the Startup and Example entries of bundle.yaml).
  presetList: ["request", "preset-list", () => null, (r) => (r.value && r.value.entries) || []],
  setLiveDebugEnabled: ["command", "c59.live-debug", (on) => ({ on: !!on })],
  setCpuDebugEnabled: ["command", "c59.cpu-debug", (on) => ({ on: !!on })],
  // LIB-10 (M-53): FREEZE ON START and ARMED as the run control's pause-on-start
  // ("program" for the CALCULATOR tab, "cpu" for the CPU tab, null: off).
  pauseOnStart: ["command", "pause-on-start", (level) => (level ? { level } : { off: true })],
  clearHeatmap: ["command", "c59.clear-heatmap", () => null],
  cycleDebugLevel: ["command", "c59.cycle-debug-level", () => null],
  clearLog: ["command", "c59.clear-log", () => null],
  dumpVars: ["command", "c59.dump-vars", () => null],
  dumpScom: ["command", "c59.dump-scom", () => null],
  dumpProg: ["command", "c59.dump-prog", () => null],
  dumpMemory: ["command", "c59.dump-memory", () => null],
  // LIB-10: the TRACE button is the trace session's button (AC-23); the trace
  // is handed over with keep-file when it ends (AC-26).
  setTrace: ["command", "trace", (on) => (on ? { button: true } : { off: true })],
  loadAsm: ["command", "c59.load-asm", (text, name) => ({ text, name })],
  runAsm: ["command", "c59.run-asm", () => null],
  clearAsm: ["command", "c59.clear-asm", () => null],
  setCueCardSize: ["command", "c59.web.cue-card-size", (width, height) => ({ width, height })],
  setDebugWidth: ["command", "c59.web.debug-width", (width) => ({ width })],
  mobileRules: ["request", "c59.mobile-rules", (w, h) => ({ w, h }), valueOf],
  ticks: ["request", "ticks", () => null, valueOf],   // {ticks, seconds}: the machine's emulated time (LIB-3 AC-39)
};

const INITIAL_STATE = {
  data: null,          // c59.static-data: models, modules, layout, heatmap
  status: CALCU_SENTENCES.starting(PRODUCT),
  persistent: true,    // the persistent file system keeps what is saved (web.storage)
  model: null,         // {model, moduleID}
  cueCard: null,       // draw list
  runState: { frozen: false, owner: null, pcArmed: false, scanArmed: false, fullSpeed: false, heldKey: -1, asmActive: false },
  printer: { connected: false, trace: false },   // the PC-100C's place and TRACE (the paper: CalcuPaperPanel)
  card: { swiping: false, fileName: "card.U59", cards: [] },
  live: null,
  cpu: [],
  heatmap: null,
  log: { text: "", level: 0 },
  trace: { running: false, fileName: "" },        // the library's `trace` topic
  asm: { fileName: "", words: 0, status: "", active: false },
  traces: [],          // [{name, size}]: the traces handed over (keep-file), newest last (LIB-10 D-25)
  settings: { startupModel: -1, traceMaxMB: 50, keyboardFeedback: 0 },
  debugFont: 11,
  debugPanel: { panel: 0, n: 0 },  // the debug-panel: step, the tab asked for, and how often
  geometry: null,
};

const float32 = (bytes) => (bytes ? new Float32Array(bytes.buffer, bytes.byteOffset, bytes.byteLength / 4) : null);

// model (its name, "TI-58C") and preset (a path of bundle.yaml, "sum.ti59")
// start the session with them: LIB-18's `start {model, preset}`, the preset
// as its bundle path `examples:<path>` (LIB-15 N3, AC-16).
function createEngine({ model, preset, workerUrl }) {
  // The page's host services, with the traces it was handed kept by name for
  // the LOG tab's list (LIB-10 D-25; the worker keeps no copy).
  const kept = new Map();
  let keptList = [];
  const base = calcuWebHostServices();
  const services = {
    names: base.names,
    answer: (name, args) => {
      if (name === "keep-file" && args) {   // the session hands over traces only
        const bytes = args.bytes || new Uint8Array(0);
        kept.set(args.name, bytes);
        keptList = keptList.filter((t) => t.name !== args.name).concat([{ name: args.name, size: bytes.length }]);
        Promise.resolve().then(() => onShell());
      }
      return base.answer(name, args);
    },
  };
  const shell = createShell({ product: PRODUCT, workerUrl, start: { model: model || "", preset: bundlePath(preset) }, services });
  let state = INITIAL_STATE;
  const listeners = new Set();
  const frames = { cells: null, onFrame: null };
  let framePending = false;
  let statusOverride = null;   // a sentence of the page's, or "" once dismissed
  let seen = { notices: 0, panels: 0 };
  let dataAsked = false;
  let data = null;
  let debugPanel = INITIAL_STATE.debugPanel;

  const set = (patch) => {
    state = { ...state, ...patch };
    listeners.forEach((l) => l());
  };

  const download = (name, bytes) => { if (bytes) calcuDownload(name, bytes, "application/octet-stream"); };
  const requestFile = (kind, name) => {
    if (kind === "trace") {   // kept by the page (D-25)
      download(name, kept.get(name));
      return Promise.resolve();
    }
    return shell.request("c59.web.card-bytes", { name }).then((r) => download(name, r.value));
  };

  const onShell = () => {
    const s = shell.getState();
    const t = s.topics;
    const streams = s.streams;
    if (s.start === "running" && !dataAsked) {
      dataAsked = true;
      shell.request("c59.static-data", null).then((r) => { data = r.value; onShell(); });
    }
    const display = t["c59.web.display"];
    if (display && display !== frames.bytes) {
      frames.bytes = display;
      frames.cells = float32(display);
      if (!framePending && typeof requestAnimationFrame !== "undefined") {
        framePending = true;
        requestAnimationFrame(() => {
          framePending = false;
          if (frames.onFrame) frames.onFrame();
        });
      }
    }

    // The status line: a failed start (AC-3), else the newest notice (an
    // error, the storage sentence of AC-8) until it is dismissed, else
    // "Starting…" until the page can draw.
    const notices = streams.notices || [];
    if (notices.length > seen.notices) {
      statusOverride = notices[notices.length - 1].sentence;
      seen.notices = notices.length;
    }
    const status = s.start === "failed" ? s.failure
      : statusOverride !== null ? statusOverride
      : s.start === "running" && data ? "" : INITIAL_STATE.status;
    const panels = streams["c59.show-debug-panel"] || [];
    if (panels.length !== seen.panels) {
      seen.panels = panels.length;
      debugPanel = panels.length ? { panel: panels[panels.length - 1], n: panels.length } : INITIAL_STATE.debugPanel;
    }

    const storage = t["web.storage"];
    set({
      data: s.start === "running" ? data : null,
      status,
      persistent: !storage || storage.mode === "kept",
      model: t["c59.web.model"] || null,
      cueCard: t["c59.web.cue-card"] === undefined ? null : t["c59.web.cue-card"],
      runState: t["c59.web.run-state"] || INITIAL_STATE.runState,
      printer: t["c59.web.printer"] || INITIAL_STATE.printer,
      card: t["c59.web.card"] || INITIAL_STATE.card,
      live: t["c59.web.live"] || null,
      cpu: t["c59.web.cpu"] || INITIAL_STATE.cpu,
      heatmap: t["c59.web.heatmap"] || null,
      log: t["c59.web.log"] || INITIAL_STATE.log,
      trace: t.trace || INITIAL_STATE.trace,
      asm: t["c59.web.asm"] || INITIAL_STATE.asm,
      traces: keptList,
      settings: t["c59.web.settings"] || INITIAL_STATE.settings,
      debugFont: t["c59.web.debug-font"] || INITIAL_STATE.debugFont,
      debugPanel,
    });
  };
  const unsubscribe = shell.subscribe(onShell);

  const invoke = (method, args) => {
    const m = METHODS[method];
    if (!m) return Promise.reject(new Error(`unknown method ${method}`));
    const [kind, nameOf, toArgs, out] = m;
    // The printer toggle sends what the printer topic does not show now.
    if (method === "togglePrinterConnection") args = [!!(state.printer && state.printer.connected)];
    const name = typeof nameOf === "function" ? nameOf(...args) : nameOf;
    const p = kind === "command" ? shell.send(name, toArgs(...args)) : shell.request(name, toArgs(...args));
    return p.then((r) => (out ? out(r) : r));
  };
  const showRefusal = (r) => {
    if (!r.done && r.reason) {
      statusOverride = r.reason;
      onShell();
    }
    return r;
  };

  return {
    frames,
    /// The library's shell, for the component set (CalcuPaperPanel, CalcuRunControlButtons).
    shell,
    getState: () => state,
    subscribe: (l) => { listeners.add(l); return () => listeners.delete(l); },
    /// A command or request, result not wanted.
    call: (method, ...args) => { invoke(method, args).catch(() => {}); },
    /// A command or request whose result is wanted.
    request: (method, ...args) => invoke(method, args),
    /// A preset handed over by its content (LIB-6 AC-42, EC-5: an upload), an operation.
    loadPresetContent: (text, name) => shell.send("load-preset", { content: text, name: name || "the preset" }).then(showRefusal),
    /// A bundled example by its bundle path (LIB-6 AC-33), an operation.
    loadBundledPreset: (path) => shell.send("load-preset", { ref: path }).then(showRefusal),
    /// A file dropped on the calculator; the session decides what it does.
    dropFile: (file) => file.arrayBuffer()
      .then((b) => shell.send("c59.web.drop", { name: file.name, bytes: new Uint8Array(b) }))
      .then(showRefusal),
    /// The LED shapes for a w × h digit cell (W59-1).
    geometry: (w, h) => shell.request("c59.led-geometry", { w, h }).then((r) => set({ geometry: r.value })),
    /// A card file (from the save folder) or a trace the page was handed, as a download (LIB-15 AC-9).
    download: requestFile,
    setStatus: (status) => { statusOverride = status; onShell(); },
    destroy: () => {
      unsubscribe();
      shell.destroy();
    },
  };
}

// The engine's state for a component: the whole of it, or a slice.
const NO_ENGINE = { getState: () => INITIAL_STATE, subscribe: () => () => {} };
function useEngine(engine, select) {
  const e = engine || NO_ENGINE;
  return React.useSyncExternalStore(e.subscribe, () => select(e.getState()), () => select(INITIAL_STATE));
}

// "sum.ti59" → "examples:sum.ti59" (a path of bundle.yaml as its bundle path); empty stays empty.
function bundlePath(preset) {
  if (!preset) return "";
  return preset.startsWith("examples:") ? preset : `examples:${preset}`;
}
