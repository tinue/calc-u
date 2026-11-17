// CalcU1600 -- the web client, and the one global calc.js exposes
// (docs/developer/Web-Client.md has the props). Put together from the React
// set's shell (LIB-14 Part D, D.4): the shell's menu bar, the faceplate
// scaled to the width with the shell's pointer (useShellFaceplate), the
// shell's control bar with the product's disk controls (product entry
// `c1600.disk-controls`, AC-42) and the cassette controls (LIB-7), and the
// shell's area for panels with the paper panel, the INSPECTOR and the
// CE-158's printer panel; the shell's settings, About, notices and
// "Loading…". The session is LIB-15's (useShell); everything sent is a
// library command or one of the product's (`c1600.*`), nothing decided here.
// The page owns the physical keyboard while focused, the drop, and the
// dialog-led flows of C1600-2 (Load BASIC Program…, Load Machine Code…).
// It binds no shortcut to a menu item (AC-4, D-30): those keys are the
// browser's; the paste event still types (LIB-5).
//
// Test hook: the root element's `calcuShell` is the shell (createShell).

// calc.css: the menu bar is taller on a touch screen (pointer: coarse).
const MENU_HEIGHT = typeof matchMedia !== "undefined" && matchMedia("(pointer: coarse)").matches ? 40 : 26;
const BAR_HEIGHT = 40;
const PANEL_MIN = 220;          // the least room below the faceplate worth a panels' row
const INSPECTOR_MIN_WIDTH = 700; // narrower: the inspector only when no other panel is shown
const SHIFT_TAP_MS = 400;
const SHIFT_TAP_POINTER = 1000;   // the screen press a SHIFT tap is sent as (no pointer of the faceplate)
const IS_MAC = CALCU_HOST_IS_MAC;
const MODEL_IDS = ["PC-1500", "PC-1500A", "PC-1600"];
const DEFAULT_MODEL = "PC-1500A";

// A grey image (Copy Screen) as a PNG Blob.
function grayPng(image) {
  const canvas = document.createElement("canvas");
  canvas.width = image.width;
  canvas.height = image.height;
  const ctx = canvas.getContext("2d");
  const data = ctx.createImageData(image.width, image.height);
  for (let i = 0; i < image.pixels.length; i++) {
    const v = image.pixels[i];
    data.data[4 * i] = v;
    data.data[4 * i + 1] = v;
    data.data[4 * i + 2] = v;
    data.data[4 * i + 3] = 255;
  }
  ctx.putImageData(data, 0, 0);
  return canvasPng(canvas);
}

// The standard "save" glyph: a diskette.
function SaveIcon() {
  return (
    <svg className="calcu-save-icon" viewBox="0 0 16 16" aria-hidden="true">
      <path d="M2.5 1.5h9l2.5 2.5v9.5a1 1 0 0 1-1 1h-10.5a1 1 0 0 1-1-1v-11a1 1 0 0 1 1-1z" fill="#3d6fb6" />
      <rect x="4" y="1.5" width="7" height="4.5" fill="#e9eef6" />
      <rect x="8.5" y="2.3" width="1.6" height="2.9" fill="#3d6fb6" />
      <rect x="3.5" y="8.5" width="9" height="6" rx="0.5" fill="#fff" />
    </svg>
  );
}

// The product's own controls and the media controls it hands the shell's
// bar get the page's actions through this context.
const C1600Bar = React.createContext(null);

// LIB-7's module picker of a slot (the library's ShellSlotPicker), with its
// Name & Save button.
function SlotControls(props) {
  const bar = React.useContext(C1600Bar);
  return <ShellSlotPicker {...props} nameAndSave={bar.nameAndSave} icon={<SaveIcon />} />;
}

function tapeCounter(lamps) {
  if (!lamps || !lamps.tapeMode) return "–";
  const s = lamps.tapeSeconds;
  return `${lamps.tapeMode === "play" ? "▶" : "●"} ${Math.floor(s / 60)}:${String(s % 60).padStart(2, "0")}`;
}

// The cassette's controls (LIB-7 AC-42; the shell's media control of the
// kind `cassette`, B1): the tape picker, a new tape to record onto, Name &
// Save, the counter with the motor (`c1600.lamps`).
function TapeControls({ shell, entry }) {
  const bar = React.useContext(C1600Bar);
  const place = calcuMediaPlace(shell.inputs.media, "cassette");
  const ok = calcuShellAvailability(entry, "insert");
  const lamps = bar.lamps;
  return (
    <fieldset className="calcu-bar-fieldset" disabled={!ok.now} title={ok.now ? undefined : ok.reason}>
      <span className="calcu-bar-label">Cassette:</span>
      <CalcuMediaPicker place={place} send={bar.send} />
      <button type="button" className="calcu-iconbutton" title="A new cassette to record onto (CSAVE)"
        data-calcu="tape-new" aria-label="New cassette" onClick={() => bar.recordTape()}>
        <SaveIcon />
      </button>
      <button type="button" className="calcu-iconbutton" disabled={!place || !place.nameAndSave} title="Name & Save"
        data-calcu="name-and-save-cassette" aria-label="Name & Save cassette" onClick={() => bar.nameAndSave("cassette")}>
        <SaveIcon />
      </button>
      <span className={`calcu-tape-counter${lamps && lamps.tapeMotor ? " on" : ""}`} data-lamp="tape"
        title={lamps && lamps.tapeMode === "play" ? `Playing ${lamps.tapeFile}` : lamps && lamps.tapeMode === "record" ? `Recording onto ${lamps.tapeFile}` : "No cassette"}>
        {tapeCounter(lamps)}
      </span>
    </fieldset>
  );
}

// The CE-1600F's disk controls (LIB-14 AC-36, AC-42; LIB-4 AC-17, D-29):
// shown on every PC-1600 by the product entry `c1600.disk-controls`, usable
// only while the CE-1600P is plugged in (its reason the tooltip) and no
// operation runs (the media place's insert, LIB-4 AC-20).
function DiskControls({ s }) {
  const bar = React.useContext(C1600Bar);
  const cs = s.inputs.controlState;
  const entry = cs && cs.product ? cs.product["c1600.disk-controls"] : null;
  if (!entry || !entry.shown) return null;
  const floppy = calcuShellList(cs, "mediaPlaces").find((m) => calcuShellText(m, "id") === "floppy");
  const insert = floppy ? calcuShellAvailability(floppy, "insert") : { now: true, reason: "" };
  const ok = entry.available ? insert : { now: false, reason: entry.reason || "" };
  const place = ok.now ? calcuMediaPlace(s.inputs.media, "floppy") : null;
  const side = place && place.holds && place.holds.side ? 1 : 0;
  return (
    <fieldset className="calcu-bar-fieldset" data-calcu="bar-disk" disabled={!ok.now} title={ok.now ? undefined : ok.reason}>
      <span className="calcu-bar-label">Disk:</span>
      {place ? <CalcuMediaPicker place={place} send={bar.send} />
        : <select className="calcu-media-picker" data-calcu="media-picker-floppy" disabled><option>(empty)</option></select>}
      <button type="button" className="calcu-button calcu-compact" disabled={!place || !place.holds}
        title="Eject and turn the disk over" data-calcu="disk-side" aria-label="Disk side"
        onClick={() => bar.send("media.turn", { place: "floppy" })}>
        {String(side)}
      </button>
      <button type="button" className="calcu-iconbutton" disabled={!place || !place.nameAndSave} title="Name & Save"
        data-calcu="name-and-save-floppy" aria-label="Name & Save disk" onClick={() => bar.nameAndSave("floppy")}>
        <SaveIcon />
      </button>
      <span className={`calcu-lamp${bar.lamps && bar.lamps.floppyMotor ? " on" : ""}`} data-lamp="floppy"
        title="Drive active -- wait for this to go dark before turning the disk over">●</span>
    </fieldset>
  );
}

const C1600_MEDIA_CONTROLS = { cassette: TapeControls };

// The paper panel is placed by the product's layout (AC-40: the INSPECTOR,
// the paper, the CE-158's printer, in this order; only with room), so the
// shell's area draws none of its own.
const C1600_PANELS = { paper: () => null };

// Before `c1600.shell-product` answered: the shell's frame with nothing of
// the product's yet.
const C1600_NO_PRODUCT = { name: "Calc-U-1600", items: [], variantLabels: {}, moduleSlotPinOuts: [], placesWithoutControl: [] };

function CalcU1600(props) {
  const embedded = embedProps(props);
  const { width, height, keyboard } = embedded;
  // ?model= takes the model's index or its name; ?preset= a path under presets/.
  const modelProp = embedded.model === undefined || embedded.model === null || embedded.model === "" ? ""
    : Number.isInteger(Number(embedded.model)) ? (MODEL_IDS[Number(embedded.model)] || "") : String(embedded.model);
  const shell = useShell("Calc-U-1600", { ...embedded, model: modelProp,
    preset: embedded.preset ? `examples:${embedded.preset}` : "" });
  const rootRef = React.useRef(null);
  const hostKeys = React.useRef(null);        // the host keys held (the library's calcuHostKeys)
  if (!hostKeys.current) hostKeys.current = calcuHostKeys();
  const [shiftFlash, setShiftFlash] = React.useState(0);   // SHIFT taps, drawn as a flash
  const shiftTap = React.useRef(null);        // time the bare Shift went down
  const programFlow = React.useRef(false);    // a program flow runs (one at a time)
  const dialogs = useDialogs();
  const sRef = React.useRef(null);

  React.useEffect(() => { if (rootRef.current) rootRef.current.calcuShell = shell; }, [shell]);

  const start = useShellState(shell, (st) => st.start);
  const failure = useShellState(shell, (st) => st.failure);
  const lamps = useTopic(shell, "c1600.lamps");
  const programs = useStream(shell, "c1600.program-opened");

  const tell = (text) => dialogs.ask("notice", { text });
  const sendChecked = (name, args) => (sRef.current ? sRef.current.sendChecked(name, args) : Promise.resolve({ done: false }));
  const send = (name, args) => (shell ? shell.send(name, args || null) : Promise.resolve({ done: false, reason: "" }));

  // LIB-13: a drop and Load Preset… are the session's `open`: it decides
  // what the files are (several are one folder in /uploads, its preset
  // opened, AC-8), keeps them while busy (AC-7), refuses with a notice and
  // asks before a medium of the same name is replaced (AC-4: the shell's
  // question). A program file comes back as the stream c1600.program-opened.
  const openFiles = (files, way) => calcuOpenFiles(sendChecked, files, way);

  // Load BASIC Program / Load Machine Code on a file (C1600-2 AC-16 to
  // AC-19): the session's file kind, the WAV's choice, then the load.
  // `index`: the file of a WAV already chosen (from 1); otherwise asked when
  // it holds several.
  const openProgram = async (kind, name, bytes, index) => {
    const title = kind === "basic" ? "Load BASIC Program" : "Load Machine Code";
    if (!index) {
      const k = await shell.request("c1600.file-kind", { bytes });
      const tapeFiles = k.done && k.value ? k.value.tapeFiles : null;
      if (tapeFiles && tapeFiles.length > 1) {
        const names = tapeFiles.map((n, i) => `${i + 1}. ${n}`);
        const picked = await dialogs.ask("choice", { title, text: `${name} holds ${names.length} files. Load which one?`, options: names });
        if (picked === null) return;
        index = picked + 1;
      }
    }
    index = index || 0;
    if (kind === "basic") {
      await sendChecked("c1600.load-basic", { name, bytes, file: index });
      return;
    }
    const planned = await shell.request("c1600.plan-code", { bytes, file: index, address: "" });
    const plan = planned.done ? planned.value : { refusal: planned.reason };
    let address = "";
    if (plan.needsAddress) {
      address = await dialogs.ask("address", { shell, plan, bytes, index });
      if (address === null) return;
    } else if (plan.refusal) { tell(`${name}: ${plan.refusal}`); return; }
    const r = await sendChecked("c1600.load-code", { name, bytes, file: index, address });
    if (!r.done) return;
    await dialogs.ask("loaded", { result: r.value });
    // Type the proposed CALL, ready to run: the user presses ENTER (AC-19).
    send("paste", { text: r.value.callCommand });
  };
  const pickProgram = async (kind) => {
    const [f] = await calcuPickFiles(kind === "basic" ? ".bas,.bbin,.wav" : ".bin,.wav", false);
    if (f) openProgram(kind, f.name, await fileBytes(f));
  };

  // LIB-13 AC-18 (D4.2): a program file the session opened runs the flow
  // of Load BASIC Program… or Load Machine Code… without its file dialog.
  // A WAV with several files asks which one; the file chosen goes to the
  // flow of its own kind. One flow at a time: one that comes meanwhile is
  // dropped with a log line (design D8).
  const programsSeen = React.useRef(0);
  React.useEffect(() => {
    if (programs.length < programsSeen.current) programsSeen.current = 0;
    if (!shell || programs.length <= programsSeen.current || dialogs.open) return;
    const p = programs[programsSeen.current];
    programsSeen.current += 1;
    if (programFlow.current) { console.log(`[open] a program flow runs; dropped ${p.name}`); return; }
    programFlow.current = true;
    (async () => {
      let flow = p.kind === "machine-code-program" ? "code" : "basic";
      let index = 0;
      const k = await shell.request("c1600.file-kind", { bytes: p.bytes });
      const { tapeFiles, tapeKinds } = (k.done && k.value) || {};
      if (tapeFiles && tapeFiles.length > 1) {
        const names = tapeFiles.map((n, i) => `${i + 1}. ${n}`);
        const picked = await dialogs.ask("choice", { title: "Open", text: `${p.name} holds ${names.length} files. Load which one?`, options: names });
        if (picked === null) return;
        index = picked + 1;
        if (tapeKinds && tapeKinds[picked]) flow = tapeKinds[picked].startsWith("code-") ? "code" : "basic";
      }
      await openProgram(flow, p.name, p.bytes, index);
    })().finally(() => { programFlow.current = false; });
  }, [shell, programs, dialogs.open]);

  const copyScreen = () => {
    const image = shell.request("c1600.screen-image", null).then((r) => r.value);
    copyImageOrDownload("screen.png", { "image/png": image.then(grayPng), "text/plain": image.then((i) => i.text) });
  };

  // The product's shell data (D-32) with the page's acts. Load Preset…: the
  // picked files through LIB-13's `open` (AC-8: a preset with the files it
  // names, as one folder).
  const { product, ready: productReady } = useShellProduct(shell, {
    request: "c1600.shell-product", fallback: C1600_NO_PRODUCT, version: CalcU1600.version, build: "web",
    copyScreen, openFiles, presetAccept: ".pc1500,.pc1500a,.pc1600,.bas,.bbin,.bin,.wav,.yaml",
    acts: { "load-basic-program": () => pickProgram("basic"), "load-machine-code": () => pickProgram("code") },
  });

  const s = useCalcuShell(shell, product, CALCU_SHELL_WEB_TRAITS);
  sRef.current = s;
  const cs = s.inputs.controlState;
  const busy = !!(s.inputs.busy && s.inputs.busy.operationRuns);
  const model = (cs && cs.model) || modelProp || DEFAULT_MODEL;

  // The faceplate maps, one request per model (D-31).
  const maps = useFaceplateMaps(shell, "c1600.faceplate-map", model);

  // Ready once the start (an operation: the model, then its preset) is
  // over: keys pressed while an operation runs are dropped (ledger 1600-9).
  const [started, setStarted] = React.useState(false);
  React.useEffect(() => { if (!started && s.started && s.inputs.busy && !busy) setStarted(true); }, [s.started, busy, s.inputs.busy]);
  const ready = start === "running" && started && !!maps[model] && productReady;

  // The keyboard reaches the calculator without a click first: the client
  // takes the focus once it runs and after each dialog, unless the host
  // page has put it elsewhere.
  const modalOpen = dialogs.open || !!s.open || !!s.notice;
  React.useEffect(() => {
    const el = rootRef.current;
    if (!keyboard || !ready || modalOpen || !el) return;
    const active = document.activeElement;
    if (!active || active === document.body) el.focus({ preventScroll: true });
  }, [keyboard, ready, modalOpen]);

  // After a tap or click anywhere but a text field or picker, the keyboard
  // goes back to the calculator. A control-bar button that kept it would be
  // pressed again by ENTER or SPACE. A dialog keeps its own focus, and so
  // does a host page element outside the client.
  const reclaimFocus = () => {
    const el = rootRef.current;
    if (!keyboard || !el) return;
    setTimeout(() => {
      if (el.querySelector(".calcu-modal, .calcu-shell-modal")) return;
      const active = document.activeElement;
      if (active && active !== document.body && !el.contains(active)) return;
      if (active && active !== el && /^(INPUT|SELECT|TEXTAREA)$/.test(active.tagName)) return;
      el.focus({ preventScroll: true });
    }, 0);
  };
  // A button in the client never keeps the focus: every key, Tab included,
  // goes to the calculator. iOS moves the focus there on its own (a tapped
  // button after the tap, a menu title on its first letter typed on a
  // hardware keyboard).
  const onFocusCapture = (e) => {
    if (e.target.tagName === "BUTTON" && !e.target.closest(".calcu-modal, .calcu-shell-modal")) reclaimFocus();
  };

  // ── Media flows (LIB-7) ─────────────────────────────────────────────
  const bar = {
    send,
    lamps,
    // Name & Save of a module, a disk or a tape (LIB-7 AC-16, AC-40; D-25).
    nameAndSave: async (word) => {
      const place = calcuMediaPlace(s.inputs.media, word);
      if (!place) return;
      const prompt = word === "floppy" ? "Disk name:" : word === "cassette" ? "Cassette name:" : "Instance name:";
      await dialogs.ask("nameAndSave", { shell, place, prompt });
    },
    // The record button: a new tape under a name, on the empty tape (LIB-7
    // AC-20, D-24): the name first, asked before an instance is replaced
    // (the shell's question); its files at the first motor stop.
    recordTape: async () => {
      const place = calcuMediaPlace(s.inputs.media, "cassette");
      const empty = place && (place.picker || []).find((e) => e.group === "bundled-template" && e.label === "Empty");
      if (!empty) return;
      const name = await dialogs.ask("prompt", { title: "Save Cassette", label: "Cassette name:" });
      if (name === null) return;
      await sendChecked("media.start-new", { place: "cassette", template: empty.id, name });
    },
  };

  // ── Keyboard (MainWindow::keyPressEvent / keyReleaseEvent) ──────────
  const releaseHeldKeys = () => {
    if (hostKeys.current.focusLost() && shell) send("focus-lost");   // LIB-5 AC-10: every held key comes up
    shiftTap.current = null;
  };
  const onKeyDown = (e) => {
    if (!keyboard || !shell || !s.started || modalOpen) return;
    shell.unlockAudio();
    // A key with ⌘ (Ctrl elsewhere) is the browser's: no menu item has a
    // shortcut here (AC-4, D-30); ⌘V / Ctrl+V comes as the paste event.
    if (IS_MAC ? e.metaKey : e.ctrlKey && !e.altKey) { shiftTap.current = null; return; }
    if (e.repeat) { e.preventDefault(); return; }
    if (e.key === "Shift") { shiftTap.current = performance.now(); return; }
    shiftTap.current = null;
    if (e.key === "Dead" || e.isComposing) return;
    // Function keys past F6 and the like stay the browser's.
    if (/^F([7-9]|1\d)$/.test(e.key)) return;
    e.preventDefault();
    const down = hostKeys.current.down(e);   // LIB-5 AC-7, AC-29; none while held
    if (down) send("host-key-down", down);
  };
  const onKeyUp = (e) => {
    if (!shell) return;
    if (e.key === "Shift") {
      const tapped = shiftTap.current !== null && performance.now() - shiftTap.current <= SHIFT_TAP_MS;
      shiftTap.current = null;
      // LIB-5 AC-33: a SHIFT tap is one press of the machine's shift key
      // (the wall-time rule stays here, D-12; the engine holds it its minimum).
      if (tapped) {
        send("screen-key-down", { key: "shift", pointer: SHIFT_TAP_POINTER });
        send("screen-key-up", { key: "shift", pointer: SHIFT_TAP_POINTER });
        setShiftFlash((n) => n + 1);   // too short for the held keys to show
      }
    }
    // The engine keeps a quick tap down long enough to be scanned (LIB-5 AC-1).
    const up = hostKeys.current.up(e);
    if (up) send("host-key-up", up);
  };

  // ── Layout (AC-40) ──────────────────────────────────────────────────
  const [measured, setMeasured] = React.useState({ w: width || 0, h: height || 0 });
  React.useEffect(() => {
    if (width && height) { setMeasured({ w: width, h: height }); return undefined; }
    const el = rootRef.current && rootRef.current.parentElement;
    if (!el) return undefined;
    const ro = new ResizeObserver(() => setMeasured({ w: width || el.clientWidth, h: height || el.clientHeight }));
    ro.observe(el);
    return () => ro.disconnect();
  }, [width, height]);
  const W = measured.w || 900, H = measured.h || 600;
  // The calculator comes first: the faceplate is as large as the window
  // allows. Only when it reaches both edges and leaves PANEL_MIN or more
  // below it do the panels get that space (portrait, tall windows) -- a
  // phone in landscape shows the calculator alone.
  const aspect = FACE_ASPECT[model] || FACE_ASPECT[DEFAULT_MODEL];
  const faceRoom = H - MENU_HEIGHT - BAR_HEIGHT;
  const faceWidth = Math.max(200, Math.min(W, faceRoom * aspect));
  const panelRoom = faceRoom - W / aspect >= PANEL_MIN;
  const paperOwner = (s.panels.find((p) => p.kind === "paper") || {}).owner || "";
  const printerEntry = cs && cs.product ? cs.product["c1600.ce158-printer-panel"] : null;
  // LIB-8 AC-36: the paper panel with or without a plotter. Where only one
  // panel fits (a narrow window), it gives way to the inspector while no
  // plotter is plugged in, as the panels give way today.
  const showPaper = panelRoom && (W >= INSPECTOR_MIN_WIDTH || !!paperOwner);
  const showPrinter = panelRoom && !!(printerEntry && printerEntry.shown);
  const showInspector = panelRoom && (W >= INSPECTOR_MIN_WIDTH || !(showPaper || showPrinter));
  const anyPanel = showInspector || showPaper || showPrinter;

  const romsMissing = !!s.romsMissing && !s.started;
  const failed = start === "failed";
  return (
    <div className="calcu-client" ref={rootRef} tabIndex={0} style={{ width: W, height: H }}
      data-ready={ready ? "true" : "false"} data-model={model}
      onKeyDown={onKeyDown} onKeyUp={onKeyUp} onBlur={releaseHeldKeys}
      onPointerDown={() => { shiftTap.current = null; if (shell) shell.unlockAudio(); }}
      onPointerUp={reclaimFocus}
      onFocusCapture={onFocusCapture}
      onChange={(e) => {
        if (e.target.tagName === "SELECT" && !e.target.closest(".calcu-modal, .calcu-shell-modal")) { e.target.blur(); reclaimFocus(); }
      }}
      onPaste={(e) => {
        const text = e.clipboardData && e.clipboardData.getData("text/plain");
        if (text && shell) { e.preventDefault(); send("paste", { text }); }
      }}
      {...calcuFileDrop(shell ? openFiles : null)}>
      <ShellMenuBar shell={s} />
      {/* LIB-14 D-39 (M-57, ledger 1600-48): the library's storage sentence. */}
      <ShellStorageBanner shell={shell} />
      <div className="calcu-face-row">
        {romsMissing && <ShellRomsMissing shell={s} />}
        {failed && !romsMissing && <div className="calcu-shell-roms-missing" data-calcu="start-failed" role="alert">{failure}</div>}
        {!romsMissing && !failed && <Faceplate shell={shell} model={model} map={maps[model] || null} width={faceWidth} shiftFlash={shiftFlash} />}
      </div>
      <C1600Bar.Provider value={bar}>
        <div className="calcu-controlbar">
          <ShellControlBar shell={s} slotPicker={SlotControls} mediaControls={C1600_MEDIA_CONTROLS}
            extra={{ Media: [<DiskControls key="disk" s={s} />] }} />
        </div>
      </C1600Bar.Provider>
      {anyPanel && s.started && (
        <div className="calcu-panels">
          <ShellPanelArea shell={s} panels={C1600_PANELS}>
            {showInspector && <Inspector shell={shell} />}
            {showPaper && <PaperDock shell={shell} owner={paperOwner} />}
            {showPrinter && <Ce158Printer shell={shell} />}
          </ShellPanelArea>
        </div>
      )}
      <ShellDialogs shell={s} savedMedia={ShellSavedMedia} />
      {dialogs.element}
    </div>
  );
}

CalcU1600.version = "__CALC_VERSION__";
if (typeof window !== "undefined") window.CalcU1600 = CalcU1600;
