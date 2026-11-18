// CalcU59 — the web client, and the one global calc.js exposes
// (reference/WebClient.md has the props). Put together from the React set's
// shell (LIB-14 59, M-57): in the wide layout the shell's menu bar (File,
// Machine, Help; no Edit, AC-26), in the portrait layout no menu bar and the
// bar's Reset, Preset, Settings and More (AC-29, B8); in every calculator
// column the shell's control bar (the model picker, LIB-7's module picker of
// `slot-1`, the button "PC-100C", C59-7's card controls; AC-27); the shell's
// settings, About, notices, "Loading…", the storage banner and "ROMs
// missing". The product's data come from `c59.shell-product` and
// `c59.faceplate-map` (app/TIShellProduct, TIWebPart). Two layouts, and
// layout="auto" (the PWA) picks between them by the window's size, with the
// iOS rules of app/ (`c59.mobile-rules`):
//   wide     — calculator | printer | debugger, as the macOS window and the
//              Qt MainWindow (resizable dividers between the columns);
//   portrait — one page at a time, calculator or printer, switched with the
//              corner chevrons, as the iPhone layout (no debugger).
// It owns the physical keyboard while focused (host keys through the
// library's calcuHostKeys, LIB-5), the drop (LIB-13's `open`), and keyboard
// feedback (portrait).

const BAR_HEIGHT = 50;
const FACE_PADDING = 8;
const PRINTER_MIN = 220, PRINTER_MAX = 320, DEBUG_MIN = 220;
// The narrowest artwork whose control bar still fits (labelled in wide, icons
// in portrait). Below it the client keeps its size and the page scrolls, as
// the apps' windows have a minimum size.
const FACE_MIN = { wide: 400, portrait: 264 };
const FACE_MIN_AUTO = 120;  // layout="auto": whatever a landscape phone's height leaves
const SPLITTERS = 1 + 5;  // the line after the calculator, the divider before the debugger
const MENU_HEIGHT = 26;
const CALC_VERSION = "__CALC_VERSION__";

/// The artwork's width that `height` leaves room for, and the column height for an artwork width.
const faceWidthFor = (height) => (height - BAR_HEIGHT - 2 * FACE_PADDING) * CANVAS_ASPECT;
const columnHeight = (faceWidth, height) => Math.max(height || 0, faceWidth / CANVAS_ASPECT + 2 * FACE_PADDING + BAR_HEIGHT);

/// The client's minimum width for a layout.
function minimumWidth(portrait) {
  return portrait ? FACE_MIN.portrait + 2 * FACE_PADDING
    : FACE_MIN.wide + 2 * FACE_PADDING + PRINTER_MIN + DEBUG_MIN + SPLITTERS;
}

// The shell's traits per layout (design §10.5): the Web has no Quit and
// binds no shortcut; portrait has no menu bar (M13).
const traitsFor = (portrait, settings) => ({ hasQuit: false, shortcuts: false, savedMedia: true,
  menuBar: !portrait, settings: settings !== false, about: true });

// Keyboard feedback (Settings, portrait): a short click synthesised with
// WebAudio. No vibration: iOS Safari has no Vibration API, and Android's
// did nothing on the phones tried.
let clickAudio = null;
function playClick() {
  try {
    clickAudio = clickAudio || new (window.AudioContext || window.webkitAudioContext)();
    const t = clickAudio.currentTime;
    const osc = clickAudio.createOscillator(), gain = clickAudio.createGain();
    osc.frequency.value = 2000;
    gain.gain.setValueAtTime(0.25, t);
    gain.gain.exponentialRampToValueAtTime(0.001, t + 0.03);
    osc.connect(gain).connect(clickAudio.destination);
    osc.start(t);
    osc.stop(t + 0.03);
  } catch (e) { /* no audio */ }
}

// Keyboard feedback on each key press (portrait, the PWA): `[keyboard]
// feedback` of the `settings` topic, the key from `held-keys`. A leaf of its
// own, so a key press re-renders nothing else.
function KeyFeedback({ shell, enabled }) {
  const mode = useTopic(shell, "settings", (st) => {
    const r = ((st && st.rows) || []).find((x) => x.section === "keyboard" && x.key === "feedback");
    return r ? r.value : "off";
  });
  const held = useTopic(shell, "held-keys", (h) => (h && h.live && h.live.length ? h.live[h.live.length - 1] : ""));
  React.useEffect(() => { if (enabled && held && mode === "click") playClick(); }, [held]);
  return null;
}

// LIB-7's module picker of `slot-1` (LIB-7 AC-47; the library's
// ShellSlotPicker).
const SlotPicker = (props) => <ShellSlotPicker {...props} label="Solid State Module" />;

function BarButton({ icon, label, compact, title, name, className = "", ...rest }) {
  return (
    <button type="button" className={`calcu-barbutton ${className}`} title={title || name || label} aria-label={name || label} {...rest}>
      <span className="calcu-icon" aria-hidden="true">{icon}</span>
      {!compact && <span>{label}</span>}
    </button>
  );
}

// C59-7's card controls (AC-13), the media controls of the kind
// `magnetic-card` on the shell's bar (B1): Arm Card and Arm Fresh Card (both
// "Crd") while no card passes, Eject Card while one does; disabled with the
// media place's reason while it cannot be used (`insert`).
const C59_BAR = React.createContext(null);
function CardControls({ shell, entry }) {
  const bar = React.useContext(C59_BAR);
  const reader = cardReaderOf(shell.inputs.media);
  const swiping = !!reader && reader.passing !== undefined && reader.passing !== null;
  const ok = calcuShellAvailability(entry, "insert");
  if (swiping)
    return <BarButton icon="⏏" label="Eject Card" compact={bar.compact}
      onClick={() => shell.shell.send("media.eject", { place: "card-reader" })} />;
  return (
    <React.Fragment>
      <BarButton icon="⤓" label="Crd" name="Arm Card" compact={bar.compact} disabled={!ok.now}
        title={ok.now ? "Arm Card" : ok.reason} onClick={() => bar.cardDialog("arm")} />
      <BarButton icon="＋" label="Crd" name="Arm Fresh Card" compact={bar.compact} disabled={!ok.now}
        title={ok.now ? "Arm Fresh Card" : ok.reason} onClick={() => bar.cardDialog("fresh")} />
    </React.Fragment>
  );
}
const C59_MEDIA_CONTROLS = { "magnetic-card": CardControls };

function CalculatorColumn({ page, faceWidth, compact, width }) {
  const { shell, s, faceplate, model } = page;
  return (
    <div className="calcu-calc" style={{ width: width || faceWidth + 2 * FACE_PADDING }}>
      <div className="calcu-face-pad"><CalculatorFace shell={shell} faceplate={faceplate} model={model} width={faceWidth} /></div>
      <div className={"calcu-bar" + (compact ? " calcu-compact" : "")}>
        <C59_BAR.Provider value={{ compact, cardDialog: page.cardDialog }}>
          <ShellControlBar shell={s} compact={compact} slotPicker={SlotPicker} mediaControls={C59_MEDIA_CONTROLS} />
        </C59_BAR.Provider>
      </div>
    </div>
  );
}

/// A draggable divider that changes the width before it by the drag.
function Divider({ onDrag }) {
  const start = React.useRef(null);
  return (
    <div className="calcu-splitter" role="separator" aria-orientation="vertical"
      onPointerDown={(e) => { start.current = e.clientX; e.currentTarget.setPointerCapture(e.pointerId); }}
      onPointerMove={(e) => {
        if (start.current === null) return;
        onDrag(e.clientX - start.current);
        start.current = e.clientX;
      }}
      onPointerUp={() => { start.current = null; }} />
  );
}

/// The rules of the embedded layout="wide": the artwork's size follows the
/// window, within limits, and the debugger is always there.
const EMBEDDED_WIDE = {
  embedded: true, printerMin: PRINTER_MIN, printerMax: PRINTER_MAX, printerStart: 260,
  debugger: true, barLabels: true, compactPrinterBar: false,
};

/// `rules` is EMBEDDED_WIDE or (layout="auto") app/'s mobile rules: the
/// artwork fills the height, whatever its size, the bar and the printer follow
/// the iOS app, and the debugger may be left out. A bar wider than the artwork
/// widens the column.
function WideLayout({ page, width, height, rules }) {
  const room = height ? height - MENU_HEIGHT : 0;
  const faceWidth = rules.embedded
    ? Math.max(FACE_MIN.wide, Math.min(room ? faceWidthFor(room) : 420, width * 0.4))
    : Math.max(FACE_MIN_AUTO, faceWidthFor(room));
  const compact = !rules.barLabels || faceWidth < FACE_MIN.wide;
  const columnWidth = Math.max(faceWidth + 2 * FACE_PADDING, compact ? minimumWidth(true) : 0);
  const total = columnHeight(faceWidth, room);
  const { printerMin, printerMax, debugger: hasDebugger } = rules;
  const [printerWidth, setPrinterWidth] = React.useState(rules.printerStart ?? printerMax);
  const free = width - columnWidth - 2;
  const pw = Math.max(printerMin, Math.min(printerMax, printerWidth, free - (hasDebugger ? DEBUG_MIN : 0)));
  return (
    <div className="calcu-wide" style={{ height: total }}>
      <CalculatorColumn page={page} faceWidth={faceWidth} width={columnWidth} compact={compact} />
      <div className="calcu-vline" />
      <div style={{ width: pw, flex: "none", display: "flex" }}><PaperPane shell={page.shell} owner={page.paperOwner} /></div>
      {hasDebugger && <Divider onDrag={(dx) => setPrinterWidth(Math.max(printerMin, Math.min(printerMax, pw + dx)))} />}
      {hasDebugger && <Debugger />}
    </div>
  );
}

function PortraitLayout({ page, width, height }) {
  const byWidth = width - 2 * FACE_PADDING;
  const faceWidth = Math.max(FACE_MIN.portrait, height ? Math.min(byWidth, faceWidthFor(height)) : byWidth);
  const total = columnHeight(faceWidth, height);
  const [view, setView] = React.useState("calc");
  const other = view === "calc" ? "printer" : "calc";
  const chevron = (side) => (
    <button type="button" className={`calcu-chevron calcu-chevron-${side}`}
      aria-label={other === "printer" ? "Printer" : "Calculator"} onClick={() => setView(other)}>
      {side === "left" ? "‹" : "›"}
    </button>
  );
  return (
    <div className="calcu-portrait" style={{ height: total }}>
      {/* Pages and chevrons share the calculator's column: on a screen wider
          than the calculator (a tablet, a desktop window) the chevrons stay at
          its corners instead of the screen's. */}
      <div className="calcu-column" style={{ width: faceWidth + 2 * FACE_PADDING }}>
        <div className="calcu-page" hidden={view !== "calc"}>
          <CalculatorColumn page={page} faceWidth={faceWidth} compact={true} />
        </div>
        {view === "printer" && <div className="calcu-page calcu-page-printer"><PaperPane shell={page.shell} owner={page.paperOwner} /></div>}
        {view === "calc" ? chevron("right") : chevron("left")}
      </div>
    </div>
  );
}

// Before `c59.shell-product` answered: the shell's frame with nothing of
// the product's yet.
const C59_NO_PRODUCT = { name: "Calc-U-59", items: [], variantLabels: {}, moduleSlotPinOuts: [], placesWithoutControl: [] };

// The props (LIB-15 AC-16, AC-19, AC-29): the embedding interface's width,
// keyboard (on by default), layout ("portrait", the default, "wide" or
// "auto"; embedProps ignores another value with a console warning), preset
// (a bundled preset's path, with or without `examples:`), height, model (by
// its name) and workerUrl, and the product's own settings. model and preset
// are read once, at mount (EC-5).
function CalcU59(props) {
  const { width, keyboard, layout, preset, height, model: modelProp, workerUrl, rest } =
    embedProps(props, { name: "CalcU59", layouts: ["portrait", "wide", "auto"], layout: "portrait" });
  const { settings = true } = rest;
  // The traces the session hands over (the LOG tab), kept by the page's host services.
  const traces = React.useRef(null);
  if (!traces.current) traces.current = createC59Traces();
  const shell = useShell(PRODUCT, { model: modelProp || "", preset: bundlePathOf(preset), workerUrl }, traces.current.services);
  const [cardDialog, setCardDialog] = React.useState(null);   // "arm" | "fresh" | null
  const rootRef = React.useRef(null);
  const [measured, setMeasured] = React.useState(0);
  const auto = layout === "auto";
  const [rules, setRules] = React.useState(null);
  const portrait = auto ? !rules || rules.paged : layout !== "wide";
  const hostKeys = React.useRef(null);
  if (!hostKeys.current) hostKeys.current = calcuHostKeys();

  // The Playwright tests' hook: the shell on the client's element.
  React.useEffect(() => { if (rootRef.current && shell) rootRef.current.calcuShell = shell; }, [shell]);
  const start = useShellState(shell, (st) => st.start);
  const failure = useShellState(shell, (st) => st.failure);
  const log = useLogView(shell);

  // layout="auto": the rules for this size come from the worker.
  React.useEffect(() => {
    if (!auto || !shell || start !== "running") return undefined;
    let current = true;
    shell.request("c59.mobile-rules", { w: width, h: height }).then((r) => { if (current && r.done) setRules(r.value); });
    return () => { current = false; };
  }, [auto, shell, start, width, height]);

  // LIB-13: Load Preset… and a drop are the session's `open` (an upload):
  // it decides what the files are, several are one folder (AC-8), a refusal
  // is its notice (AC-15). The preset extensions and all files (AC-5).
  const openFiles = (files, way) => calcuOpenFiles((name, args) => shell.send(name, args), files, way);

  // The product's shell data per layout (D-32; the settings differ, AC-30).
  // AC-26: no Copy Screen.
  const { product } = useShellProduct(shell, {
    request: "c59.shell-product", args: { layout: portrait ? "portrait" : "wide", version: CALC_VERSION },
    fallback: C59_NO_PRODUCT, version: CALC_VERSION, build: "", copyScreen: null, openFiles,
    acts: { "arm-card": () => setCardDialog("arm"), "arm-fresh-card": () => setCardDialog("fresh") },
  });
  const traits = React.useMemo(() => traitsFor(portrait, settings), [portrait, settings]);
  const s = useCalcuShell(shell, product, traits);
  const cs = s.inputs.controlState;
  const model = (cs && cs.model) || modelProp || "TI-59";

  // The faceplate map, one request per model (D-41).
  const maps = useFaceplateMaps(shell, "c59.faceplate-map", model);

  // Without a width the client fills its container.
  React.useEffect(() => {
    if (width) return undefined;
    const el = rootRef.current.parentElement;
    const ro = new ResizeObserver(() => setMeasured(el.clientWidth));
    ro.observe(el);
    return () => ro.disconnect();
  }, [width]);
  const w = Math.max(minimumWidth(auto || portrait), width || measured || (portrait ? 360 : 1100));

  // Keyboard feedback is for touch screens: portrait, and the PWA in any layout.
  const touch = layout !== "wide";
  const modalOpen = !!s.open || !!s.notice || !!cardDialog;

  // ── Physical keyboard ────────────────────────────────────────────────
  // Only with keyboard={true}, and only while the calculator itself has
  // focus (not a control in it): Tab keeps working as Tab. A key with
  // Control, Alt or Command is the browser's; the product's keyboard map in
  // the worker (app/TITyping's TIKeyboardMap) decides what the others press
  // (LIB-5; D-40).
  const ownsKeys = (e) => keyboard && shell && s.started && !modalOpen &&
    (e.target === rootRef.current || (e.target.classList && e.target.classList.contains("calcu-key")));
  const claimed = (e) => {
    if (!ownsKeys(e) || e.ctrlKey || e.altKey || e.metaKey) return false;
    const k = calcuHostKeyEvent(e);
    return k.produced !== 0 || !!k.named;
  };
  const onKeyDown = (e) => {
    if (!claimed(e)) return;
    e.preventDefault();
    if (e.repeat) return;   // the host's repeat; the engine ignores it (AC-4)
    const down = hostKeys.current.down(e);
    if (down) shell.send("host-key-down", down);
  };
  const onKeyUp = (e) => {
    if (!shell) return;
    const up = hostKeys.current.up(e);
    if (!up) return;
    e.preventDefault();
    shell.send("host-key-up", up);
  };
  const onBlur = (e) => {
    if (e.target === rootRef.current) delete rootRef.current.dataset.pointerFocus;
    if (shell && !rootRef.current.contains(e.relatedTarget)) {
      hostKeys.current.focusLost();
      shell.send("focus-lost");   // LIB-5 AC-10: every held key comes up
    }
  };
  // Marked, as this focus is for the keys and no reason to outline the client:
  // WebKit counts any focus from script as focus-visible.
  const focusForKeys = (e) => {
    if (!keyboard || !e.target.closest || !e.target.closest(".calcu-face")) return;
    rootRef.current.dataset.pointerFocus = "";
    rootRef.current.focus({ preventScroll: true });
  };

  // ── Drag and drop any file: LIB-13's `open {way: drop, upload}` ──────
  const drop = calcuFileDrop(shell ? openFiles : null);

  const paperOwner = (s.panels.find((p) => p.kind === "paper") || {}).owner || "";
  const page = { shell, s, faceplate: maps[model] || null, model, paperOwner, cardDialog: setCardDialog };
  const romsMissing = !!s.romsMissing && !s.started;
  const failed = start === "failed";
  const Layout = portrait ? PortraitLayout : WideLayout;
  return (
    <div
      ref={rootRef}
      className="calcu-client calcu-device"
      data-layout={portrait ? "portrait" : "wide"}
      data-start={start}
      style={{ width: w }}
      tabIndex={keyboard ? 0 : undefined}
      aria-label="Calc-U-59 calculator"
      onKeyDown={onKeyDown}
      onKeyUp={onKeyUp}
      onBlur={onBlur}
      onPointerDownCapture={focusForKeys}
      onDragOver={drop.onDragOver}
      onDrop={drop.onDrop}
    >
      <C59_PAGE.Provider value={{ shell, s, traces: traces.current, log }}>
        <ShellStorageBanner shell={shell} />
        {/* AC-19: without a machine there is no bar and no More button, so the
            portrait layout shows the menu bar for Help and About too. */}
        {(!portrait || romsMissing) && !failed && <ShellMenuBar shell={s} />}
        {romsMissing && <ShellRomsMissing shell={s} />}
        {failed && !romsMissing && <div className="calcu-shell-roms-missing" data-calcu="start-failed" role="alert">{failure}</div>}
        {!romsMissing && !failed && (!auto || rules) && <Layout page={page} width={w} height={height} rules={rules || EMBEDDED_WIDE} />}
        <KeyFeedback shell={shell} enabled={touch} />
        <ShellDialogs shell={s} savedMedia={ShellSavedMedia} />
        {cardDialog && <CardPicker shell={shell} mode={cardDialog} onClose={() => setCardDialog(null)} />}
      </C59_PAGE.Provider>
    </div>
  );
}

/// The portrait client's width for a height: no wider than the calculator
/// at that height needs (the demo page sizes itself with it).
CalcU59.portraitWidth = (height) => faceWidthFor(height) + 2 * FACE_PADDING;

Object.assign(window, { CalcU59 });
