// CalcU59 — the web client, and the one global calc.js exposes
// (docs/developer/Web-Client.md has the props). Put together from the React set's
// shell: on a desktop the shell's menu bar (File, Machine, Help; no Edit),
// on mobile none (CALCU_SHELL_MOBILE, useShell.jsx), in either layout; in
// every calculator column the shell's control bar (the model picker, the
// library's module picker of `slot-1`, the card controls, More at its
// right edge with Load Preset… and Settings… in it); the button "PC-100C"
// on the paper pane; the shell's
// settings, About, notices, "Loading…", the storage banner and "ROMs
// missing". The product's data come from `c59.shell-product` and
// `c59.faceplate-map` (app/TIShellProduct, TIWebPart). Two layouts, and
// layout="auto" (the PWA) picks between them by the window's size with the
// library's view rule (calcuShellView, on app/'s chrome):
//   wide     — calculator | printer | debugger, as the Qt MainWindow
//              (resizable dividers between the columns): the room beside
//              the faceplate takes the printer (and the debugger, where it
//              takes both and the window is tall enough);
//   portrait — one page at a time, calculator or printer, switched with the
//              corner chevrons, as the iPhone layout (no debugger): the
//              fallback where the room beside is too small, or below.
// It owns the physical keyboard while focused (host keys through the
// library's calcuHostKeys), the drop (the session's `open`), and keyboard
// feedback (portrait).

const FACE_PADDING = 8;
// The bar's height and the printer column's width range are app/'s
// (TIChrome, the chrome of `c59.shell-product`); these numbers stand in only
// until the engine answered (layout="auto" waits for the chrome). One
// product per page, so one set.
const CHROME_SIZES = { barHeight: 50, printerMinWidth: 240, printerMaxWidth: 400, debuggerMinWidth: 220 };
const takeChromeSizes = (chrome) => { if (chrome && chrome.sizes) Object.assign(CHROME_SIZES, chrome.sizes); };
// The narrowest artwork whose control bar still fits (labelled in wide, icons
// in portrait). Below it the client keeps its size and the page scrolls, as
// the apps' windows have a minimum size.
const FACE_MIN = { wide: 400, portrait: 264 };
const FACE_MIN_AUTO = 120;  // layout="auto": whatever a landscape phone's height leaves
const SPLITTERS = 1 + 5;  // the line after the calculator, the divider before the debugger
const MENU_HEIGHT = 26;
const CALC_VERSION = "__CALC_VERSION__";
const CALC_BUILD = "__CALC_BUILD__";   // tools/build-id.cmake

/// The artwork's width that `height` leaves room for, and the column height for an artwork width.
const faceWidthFor = (height) => (height - CHROME_SIZES.barHeight - 2 * FACE_PADDING) * CANVAS_ASPECT;
const columnHeight = (faceWidth, height) => Math.max(height || 0, faceWidth / CANVAS_ASPECT + 2 * FACE_PADDING + CHROME_SIZES.barHeight);

/// The client's minimum width for a layout.
function minimumWidth(portrait) {
  return portrait ? FACE_MIN.portrait + 2 * FACE_PADDING
    : FACE_MIN.wide + 2 * FACE_PADDING + CHROME_SIZES.printerMinWidth + CHROME_SIZES.debuggerMinWidth + SPLITTERS;
}

// The shell's traits: the Web has no Quit and binds no shortcut; the
// page being mobile decides the menu bar (useShell.jsx).
const traitsFor = (settings) => Object.assign({}, CALCU_SHELL_WEB_TRAITS, { settings: settings !== false });

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

// The library's module picker of `slot-1` (ShellSlotPicker).
const SlotPicker = (props) => <ShellSlotPicker {...props} label="Solid State Module" />;

function BarButton({ icon, label, compact, title, name, className = "", ...rest }) {
  return (
    <button type="button" className={`calcu-barbutton ${className}`} title={title || name || label} aria-label={name || label} {...rest}>
      <span className="calcu-icon" aria-hidden="true">{icon}</span>
      <span className="calcu-bar-label">{label}</span>
    </button>
  );
}

// The card controls, the media controls of the kind
// `magnetic-card` on the shell's bar, as the control state declares them
// (`c59.card-controls`): Arm Card and Arm Fresh Card (both "Crd") while no
// card passes, Eject Card while one does; disabled with the reason as their
// tooltip while they cannot be used.
const C59_BAR = React.createContext(null);
function CardControls({ shell }) {
  const bar = React.useContext(C59_BAR);
  const cs = shell.inputs.controlState;
  const entry = cs && cs.product ? cs.product["c59.card-controls"] : null;
  const acts = {
    "arm-card": () => bar.cardDialog("arm"),
    "arm-fresh-card": () => bar.cardDialog("fresh"),
    "eject-card": () => shell.shell.send("media.eject", { place: "card-reader" }),
  };
  return (
    <React.Fragment>
      {calcuShellList(entry, "controls").map((c) => {
        const ok = calcuShellAvailability(c);
        const name = calcuShellText(c, "name");
        return <BarButton key={c.id} icon={calcuShellText(c, "icon")} label={calcuShellText(c, "label")} name={name}
          compact={bar.compact} disabled={!ok.now} title={ok.now ? name : ok.reason} onClick={acts[c.id]} />;
      })}
    </React.Fragment>
  );
}
const C59_MEDIA_CONTROLS = { "magnetic-card": CardControls };

function CalculatorColumn({ page, faceWidth, width }) {
  const { shell, s, faceplate, model } = page;
  const barRef = React.useRef(null);
  const body = faceWidth + 2 * FACE_PADDING;   // the calculator's body, the bar's width where it fits
  const fit = useShellBarFit(barRef, body);   // the labels ("Crd", "Preset") and the width
  const compact = !fit.labels;
  return (
    <div className="calcu-calc" style={{ width: Math.max(width || body, fit.width) }}>
      <div className="calcu-face-pad"><CalculatorFace shell={shell} faceplate={faceplate} model={model} width={faceWidth} /></div>
      <div ref={barRef} className={"calcu-bar" + (compact ? " calcu-compact" : "")}>
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
const embeddedWide = () => ({
  embedded: true, printerMin: CHROME_SIZES.printerMinWidth, printerMax: CHROME_SIZES.printerMaxWidth, printerStart: 360,
  debugger: true,
});

/// The view of layout="auto" for the client's room (the library's rule on
/// app/'s chrome): beside the faceplate with the panels fitting, the wide
/// layout; else the pages.
function autoView(width, height, menuHeight) {
  const sizes = CHROME_SIZES;
  const view = calcuShellView({
    width, height, faceplateAspect: CANVAS_ASPECT, fixedWidth: 2 * FACE_PADDING,
    fixedHeight: menuHeight + sizes.barHeight + 2 * FACE_PADDING,
    panelsMinWidth: sizes.panelsMinWidth, panelsMinHeight: sizes.panelsMinHeight,
    debuggerMinWidth: sizes.debuggerMinWidth, debuggerMinHeight: sizes.debuggerMinHeight, mobile: CALCU_SHELL_MOBILE,
  });
  const wide = view.side === "beside" && view.panelsFit;
  return { wide, printerMin: sizes.printerMinWidth, printerMax: sizes.printerMaxWidth, debugger: wide && view.debuggerFits };
}

/// `rules` is embeddedWide() or (layout="auto") autoView(): the artwork
/// fills the height, whatever its size, and the debugger may be left out.
/// A bar wider than the artwork widens the column.
function WideLayout({ page, width, height, rules }) {
  const room = height ? height - page.menuHeight : 0;
  const faceWidth = rules.embedded
    ? Math.max(FACE_MIN.wide, Math.min(room ? faceWidthFor(room) : 420, width * 0.4))
    : Math.max(FACE_MIN_AUTO, faceWidthFor(room));
  // At least as wide as the icons-only bar needs (the portrait minimum).
  const columnWidth = Math.max(faceWidth + 2 * FACE_PADDING, minimumWidth(true));
  const total = columnHeight(faceWidth, room);
  const { printerMin, printerMax, debugger: hasDebugger } = rules;
  const [printerWidth, setPrinterWidth] = React.useState(rules.printerStart ?? printerMax);
  const free = width - columnWidth - 2;
  const pw = Math.max(printerMin, Math.min(printerMax, printerWidth, free - (hasDebugger ? CHROME_SIZES.debuggerMinWidth : 0)));
  return (
    <div className="calcu-wide" style={{ height: total }}>
      <CalculatorColumn page={page} faceWidth={faceWidth} width={columnWidth} />
      <div className="calcu-vline" />
      <div style={{ width: pw, flex: "none", display: "flex" }}><PaperPane shell={page.shell} owner={page.paperOwner} places={page.paperPlaces} onChoose={page.s.choose} /></div>
      {hasDebugger && <Divider onDrag={(dx) => setPrinterWidth(Math.max(printerMin, Math.min(printerMax, pw + dx)))} />}
      {hasDebugger && <Debugger />}
    </div>
  );
}

function PortraitLayout({ page, width, height: whole }) {
  const height = whole ? whole - page.menuHeight : whole;
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
          <CalculatorColumn page={page} faceWidth={faceWidth} />
        </div>
        {view === "printer" && <div className="calcu-page calcu-page-printer"><PaperPane shell={page.shell} owner={page.paperOwner} places={page.paperPlaces} onChoose={page.s.choose} /></div>}
        {view === "calc" ? chevron("right") : chevron("left")}
      </div>
    </div>
  );
}

// Before `c59.shell-product` answered: the shell's frame with nothing of
// the product's yet.
const C59_NO_PRODUCT = { name: "Calc-U-59", acts: {} };

// The props (docs/developer/Web-Client.md): the embedding interface's width,
// keyboard (on by default), layout ("portrait", the default, "wide" or
// "auto"; embedProps ignores another value with a console warning), preset
// (a bundled preset's path, with or without `examples:`), height, model (by
// its name) and workerUrl, and the product's own settings. model and preset
// are read once, at mount.
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
  const menuBar = !CALCU_SHELL_MOBILE;
  const hostKeys = React.useRef(null);
  if (!hostKeys.current) hostKeys.current = calcuHostKeys();

  // The Playwright tests' hook: the shell on the client's element.
  React.useEffect(() => { if (rootRef.current && shell) rootRef.current.calcuShell = shell; }, [shell]);
  const start = useShellState(shell, (st) => st.start);
  const failure = useShellState(shell, (st) => st.failure);
  const log = useLogView(shell);

  // Load Preset… and a drop are the session's `open` (an upload):
  // it decides what the files are, several are one folder, a refusal
  // is its notice. The picker offers the preset extensions and all files.
  const openFiles = (files, way) => calcuOpenFiles((name, args) => shell.send(name, args), files, way);

  // layout="auto": the view for this size, once app/'s chrome is here
  // (the answer of the last render; the first answer's layout is portrait).
  const [chromeHere, setChromeHere] = React.useState(false);
  const rules = auto && chromeHere ? autoView(width || measured || 0, height || 0, menuBar ? MENU_HEIGHT : 0) : null;
  const portrait = auto ? !rules || !rules.wide : layout !== "wide";

  // The product's shell data per layout (the settings differ).
  // No Copy Screen.
  const { product, chrome } = useShellProduct(shell, {
    request: "c59.shell-product", args: { layout: portrait ? "portrait" : "wide", version: CALC_VERSION, build: CALC_BUILD },
    fallback: C59_NO_PRODUCT, version: CALC_VERSION, build: CALC_BUILD, copyScreen: null, openFiles,
    acts: { "arm-card": () => setCardDialog("arm"), "arm-fresh-card": () => setCardDialog("fresh") },
  });
  // The product's chrome (app/TIChrome) as the root's custom properties.
  takeChromeSizes(chrome);
  React.useEffect(() => { if (chrome) setChromeHere(true); }, [chrome]);
  const dark = useColourScheme();
  const chromeStyle = React.useMemo(() => calcuChromeStyle(chrome, dark), [chrome, dark]);
  const traits = React.useMemo(() => traitsFor(settings), [settings]);
  const s = useCalcuShell(shell, product, traits, { layout: portrait ? "portrait" : "wide", version: CALC_VERSION, build: CALC_BUILD });
  const cs = s.inputs.controlState;
  const model = (cs && cs.model) || modelProp || "TI-59";

  // The faceplate map, one request per model.
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
  // the worker (app/TITyping's TIKeyboardMap) decides what the others press.
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
    if (e.repeat) return;   // the host's repeat; the engine ignores it
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
      shell.send("focus-lost");   // every held key comes up
    }
  };
  // Marked, as this focus is for the keys and no reason to outline the client:
  // WebKit counts any focus from script as focus-visible.
  const focusForKeys = (e) => {
    if (!keyboard || !e.target.closest || !e.target.closest(".calcu-face")) return;
    rootRef.current.dataset.pointerFocus = "";
    rootRef.current.focus({ preventScroll: true });
  };

  // ── Drag and drop any file: the session's `open {way: drop, upload}` ─
  const drop = calcuFileDrop(shell ? openFiles : null);

  const paper = s.panels.find((p) => p.kind === "paper") || {};
  const paperOwner = paper.owner || "";
  const page = { shell, s, faceplate: maps[model] || null, model, paperOwner, paperPlaces: paper.places || [], cardDialog: setCardDialog,
    menuHeight: menuBar ? MENU_HEIGHT : 0 };
  const romsMissing = !!s.romsMissing && !s.started;
  const failed = start === "failed";
  const Layout = portrait ? PortraitLayout : WideLayout;
  return (
    <div
      ref={rootRef}
      className="calcu-client calcu-device"
      data-layout={portrait ? "portrait" : "wide"}
      data-start={start}
      style={{ ...chromeStyle, width: w }}
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
        {/* Without a machine there is no bar and no More button, so mobile
            shows the menu bar for Help and About too. */}
        {(menuBar || romsMissing) && !failed && <ShellMenuBar shell={s} />}
        {romsMissing && <ShellRomsMissing shell={s} />}
        {failed && !romsMissing && <div className="calcu-shell-roms-missing" data-calcu="start-failed" role="alert">{failure}</div>}
        {!romsMissing && !failed && (!auto || rules) && <Layout page={page} width={w} height={height} rules={rules || embeddedWide()} />}
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
