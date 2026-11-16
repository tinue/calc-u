// The components' view of the library's Web shell (LIB-15 1600, M-31):
// createShell() starts the library's worker, which hosts the app session
// (frontends/web/bindings.cpp, app/SharpAppPart, app/SharpWebPart); this file
// only translates -- a component's call or request to a LIB-3 command or
// request of the session, and the topics the session sends to the slices
// the components read (spec AC-27). It decides nothing. Until LIB-14 1600
// (M-49) rewrites the components on the family's component set, they keep
// the names they had (engine.call("pickModel", 2)).
//
// The LCD canvas repaints from `frames` (the `c1600.lcd` topic), once per
// animation frame, without a React render. Created in an effect only (a host
// page may prerender without a Worker).

const MODEL_IDS = ["PC-1500", "PC-1500A", "PC-1600"];
const PERIPHERALS = ["CE-150", "CE-1600P", "CE-158"];
const ROM_VERSIONS = ["new", "old"];

function downloadBytes(name, bytes, type) { calcuDownload(name, bytes, type); }

// A picked/dropped File's bytes.
function fileBytes(file) {
  return file.arrayBuffer().then((b) => new Uint8Array(b));
}

// KeyboardEvent.code of the keys a keyboard map may name (calcu::typing::NamedKey).
const NAMED_KEYS = {
  Escape: "escape", Backspace: "backspace", Enter: "return", NumpadEnter: "enter", Tab: "tab", Delete: "delete",
  ArrowUp: "up", ArrowDown: "down", ArrowLeft: "left", ArrowRight: "right",
  F1: "f1", F2: "f2", F3: "f3", F4: "f4", F5: "f5", F6: "f6",
  ScrollLock: "scroll-lock", Insert: "insert", Home: "home", End: "end", PageUp: "page-up", PageDown: "page-down",
};

// The command `host-key-down` of a browser key event: which key, which
// modifiers, the one character it produced (0 for none or more).
function hostKeyEvent(code, key, shift, control, alt, meta, macHost) {
  const chars = [...(key || "")];
  const produced = chars.length === 1 ? chars[0].codePointAt(0) : 0;
  const letter = chars.length === 1 && chars[0].toLowerCase() !== chars[0].toUpperCase()
    ? chars[0].toLowerCase().codePointAt(0) : 0;
  return {
    physical: code, named: NAMED_KEYS[code] || "", produced, carried: letter,
    shift: !!shift, alt: !!alt, "mac-host": !!macHost,
    // By what the user pressed: on a Mac the browser's metaKey is Command.
    command: macHost ? !!meta : false, control: !!control, meta: macHost ? false : !!meta,
  };
}

const errorText = (r) => (r.done ? "" : r.reason || "Refused.");
const value = (r) => r.value;

// Each method of 0.8.1's engine: [kind, LIB-3 name, args → object, reply → result].
const METHODS = {
  presetExamples: ["request", "c1600.preset-examples", () => null, value],
  upload: ["command", "c1600.upload", (name, bytes) => ({ name, bytes }), value],
  loadPreset: ["command", "c1600.load-preset", (path) => ({ path }), null],
  openProgram: ["command", "c1600.open-program", (name, bytes) => ({ name, bytes }), value],
  loadBasic: ["command", "c1600.load-basic-picked", (index) => ({ index }), errorText],
  planMachineCode: ["command", "c1600.plan-machine-code", (index) => ({ index }), value],
  checkAddress: ["command", "c1600.check-address", (text) => ({ text }), value],
  loadMachineCode: ["command", "c1600.load-planned-machine-code", () => null,
    (r) => (r.done ? r.value : { error: r.reason })],
  pasteText: ["command", "paste", (text) => ({ text }), null],   // LIB-5 AC-12, AC-34
  dropTarget: ["request", "c1600.drop-target", (name, bytes) => ({ name, bytes }), value],
  importMedia: ["command", "c1600.import-media", (name, bytes) => ({ name, bytes }), errorText],
  nameAndSaveCard: ["command", "c1600.name-save-card", (slot, name, writeProtect) => ({ slot, name, writeProtect: !!writeProtect }), errorText],
  nameAndSaveDisk: ["command", "c1600.name-save-disk", (name) => ({ name }), errorText],
  selectTape: ["command", "c1600.select-tape", (name) => ({ name }), errorText],
  tapeExists: ["request", "c1600.tape-exists", (name) => ({ name }), value],
  recordTape: ["command", "c1600.record-tape", (name) => ({ name }), errorText],
  screenImage: ["request", "c1600.screen-image", () => null, value],
  lcdText: ["request", "c1600.lcd-text", () => null, value],
  ticks: ["request", "ticks", () => null, value],   // {ticks, seconds}: the machine's emulated time (LIB-3 AC-39)
  togglePeripheral: ["command", "c1600.toggle-peripheral", (p) => ({ peripheral: PERIPHERALS[p] }), null],
  pickModel: ["command", "c1600.pick-model", (m) => ({ model: MODEL_IDS[m] }), null],
  selectPC1500Rom: ["command", "c1600.pc1500-rom", (i) => ({ revision: ["A01", "A03", "A04"][i] }), null],
  selectPC1600Rom: ["command", "c1600.pc1600-rom", (i) => ({ version: ROM_VERSIONS[i] }), null],
  selectCE1600PRom: ["command", "c1600.ce1600p-rom", (i) => ({ version: ROM_VERSIONS[i] }), null],
  reset: ["command", "c1600.reset", (all) => ({ all: !!all }), null],
  // LIB-5 (AC-7, AC-29): the host key as the browser reports it; the
  // product's keyboard map in the engine decides what it types.
  hostKeyDown: ["command", "host-key-down", hostKeyEvent, null],
  hostKeyUp: ["command", "host-key-up", (code) => ({ physical: code }), null],
  focusLost: ["command", "focus-lost", () => null, null],
  selectModule: ["command", "c1600.select-module", (slot, name) => ({ slot, name }), null],
  selectDisk: ["command", "c1600.select-disk", (name) => ({ name }), null],
  toggleDiskSide: ["command", "c1600.toggle-disk-side", () => null, null],
  settingsState: ["request", "c1600.settings-state", () => null, value],
  setStartupModel: ["command", "c1600.set-startup-model", (model) => ({ model }), null],
  setDefaultPreset: ["command", "c1600.set-default-preset", (model, path) => ({ model, path }), null],
  mediaItems: ["request", "c1600.media-items", () => null, value],
  exportMedia: ["request", "c1600.export-media", (path) => ({ path }), (r) => (r.done ? r.value : null)],
  removeMedia: ["command", "c1600.remove-media", (path) => ({ path }), errorText],
  inspectorView: ["request", "c1600.inspector-view", (view) => ({ view }), value],
  inspectorMenu: ["request", "c1600.inspector-menu", (dumps) => ({ dumps: !!dumps }), value],
  cardYaml: ["request", "c1600.card-yaml", () => null, value],
  // On-screen keys (LIB-5 AC-4, EC-6): by key ID and pointer.
  faceKeyDown: ["command", "screen-key-down", (key, pointer) => ({ key, pointer: pointer || 0 }), null],
  faceKeyUp: ["command", "screen-key-up", (key, pointer) => ({ key, pointer: pointer || 0 }), null],
  setTurbo: ["command", "speed", (full) => ({ full: !!full }), null],
};

const INITIAL_STATE = {
  ready: false,
  fatal: "",
  persistent: true,
  faceplates: null,   // per model: {width, height, lcd, keys} in art pixels
  lcd: null,          // LcdLayout constants and colours
  ui: null,           // control state, pickers (c1600.ui)
  lamps: { floppyMotor: false, tapeMode: "", tapeMotor: false, tapeSeconds: 0, tapeFile: "" },
  busy: false,
  notices: [],        // [{title, text}], shown one at a time
  printer: { text: "", chunks: [] },
  log: [],            // preset step lines, for the inspector
};

// model (an index 0-2 or a model's name) and preset (a path under presets/)
// start the session with them (the `start` command, LIB-15 N3).
function createEngine({ workerUrl, model, preset }) {
  const modelName = model === undefined || model === null || model === "" ? ""
    : Number.isInteger(Number(model)) ? (MODEL_IDS[Number(model)] || "") : String(model);
  const shell = createShell({
    product: "Calc-U-1600",
    workerUrl,
    start: { model: modelName, preset: preset ? `examples:${preset}` : "" },
  });
  let state = INITIAL_STATE;
  const listeners = new Set();
  const frames = { display: null, onFrame: null };
  let framePending = false;
  let statics = null;          // {faceplates, lcd}
  let staticsAsked = false;
  let started = false;
  let seen = { notices: 0, printer: 0 };   // what the page dismissed or cleared
  let lastShell = null;

  const derive = () => {
    const s = shell.getState();
    const t = s.topics;
    const streams = s.streams;
    const missing = t["roms-missing"];
    const fatal = s.start === "failed" ? s.failure : (missing && missing.text) || "";
    const notices = (streams.notices || []).slice(seen.notices)
      .map((n) => ({ title: "Calc-U-1600", text: n.sentence }));
    const printed = (streams["c1600.printer"] || []).slice(seen.printer);
    const busyState = t.busy;
    // Ready once the start (an operation: the model, then its preset) is
    // over: keys pressed while an operation runs are dropped (ledger 1600-9).
    if (!started && t["c1600.ui"] && busyState && !busyState.operationRuns) started = true;
    return {
      ready: s.start === "running" && !!statics && started,
      fatal,
      persistent: !t["web.storage"] || t["web.storage"].mode === "kept",
      faceplates: statics ? statics.faceplates : null,
      lcd: statics ? statics.lcd : null,
      ui: t["c1600.ui"] || null,
      lamps: t["c1600.lamps"] || INITIAL_STATE.lamps,
      busy: !!(busyState && busyState.operationRuns),
      notices,
      printer: { text: printed.map((p) => p.text).join(""), chunks: printed.map((p) => p.bytes) },
      log: (streams.log || []).map((l) => l.text),
    };
  };
  const update = () => {
    state = derive();
    listeners.forEach((l) => l());
  };

  const askStatics = () => {
    if (staticsAsked) return;
    staticsAsked = true;
    Promise.all([
      Promise.all([0, 1, 2].map((m) => shell.request("c1600.faceplate", { model: m }).then(value))),
      shell.request("c1600.lcd-constants", null).then(value),
    ]).then(([faceplates, lcd]) => {
      statics = { faceplates, lcd };
      update();
    });
  };

  const unsubscribe = shell.subscribe(() => {
    const s = shell.getState();
    if (s.start === "running") askStatics();
    const lcd = s.topics["c1600.lcd"];
    if (lcd && lcd !== frames.display) {
      frames.display = lcd;
      if (!framePending && typeof requestAnimationFrame !== "undefined") {
        framePending = true;
        requestAnimationFrame(() => {
          framePending = false;
          if (frames.onFrame) frames.onFrame();
        });
      }
    }
    // Only a change the components read renders them again.
    const topics = s.topics, streams = s.streams;
    const key = lastShell;
    if (key && key.start === s.start && key.ui === topics["c1600.ui"] && key.lamps === topics["c1600.lamps"] &&
        key.busy === topics.busy && key.notices === streams.notices &&
        key.printer === streams["c1600.printer"] && key.log === streams.log && key.missing === topics["roms-missing"] &&
        key.storage === topics["web.storage"]) return;
    lastShell = { start: s.start, ui: topics["c1600.ui"], lamps: topics["c1600.lamps"], busy: topics.busy,
      notices: streams.notices, printer: streams["c1600.printer"], log: streams.log,
      missing: topics["roms-missing"], storage: topics["web.storage"] };
    update();
  });

  const invoke = (method, args) => {
    const m = METHODS[method];
    if (!m) return Promise.reject(new Error(`unknown method ${method}`));
    const [kind, name, toArgs, out] = m;
    const p = kind === "command" ? shell.send(name, toArgs(...args)) : shell.request(name, toArgs(...args));
    return p.then((r) => (out ? out(r) : r));
  };

  return {
    frames,
    // The library's shell, for the React component set's panels (LIB-8: the
    // paper panel reads the `papers` and `paper <owner>` topics from it).
    shell,
    getState: () => state,
    subscribe: (l) => { listeners.add(l); return () => listeners.delete(l); },
    /// A command or request, result not wanted.
    call: (method, ...args) => { invoke(method, args).catch(() => {}); },
    /// A command or request whose result is wanted.
    request: (method, ...args) => invoke(method, args),
    unlockAudio: shell.unlockAudio,
    dismissNotice: () => { seen = { ...seen, notices: seen.notices + 1 }; update(); },
    clearPrinter: () => {
      seen = { ...seen, printer: (shell.getState().streams["c1600.printer"] || []).length };
      update();
    },
    destroy: () => {
      unsubscribe();
      shell.destroy();
    },
  };
}

// The engine's state for a component: a slice of it.
const NO_ENGINE = { getState: () => INITIAL_STATE, subscribe: () => () => {} };
function useEngine(engine, select) {
  const e = engine || NO_ENGINE;
  return React.useSyncExternalStore(e.subscribe, () => select(e.getState()), () => select(INITIAL_STATE));
}
