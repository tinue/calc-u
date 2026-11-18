// CalcuPaperPanel -- the paper panel of the React component set. It draws
// any paper from its marks and names no printer; the shell places it in its
// area for panels.
//
//   <CalcuPaperPanel shell={shell} owner={peripheralId | null}
//                    controls={[{id, form: "key" | "switch", on}]} send={fn?} />
//
// owner: the printer plugged in, from the control state; controls:
// that printer's controls; send(command, args) defaults to
// shell.send. Copy and Cut arm the clipboard inside the click (armClipboard,
// hostServices.js) and send `paper.copy` / `paper.cut {copy: true}`.
// Elements carry data-calcu names for the tests: paper-panel, paper-owner,
// paper-view, paper-copy, paper-cut, paper-toast, paper-text,
// paper-control-<id>, paper-scroll, paper-canvas.

const CALCU_PAPER_FOLLOW = 64;      // points from the end within which the view follows
const CALCU_PAPER_BLANK = 8;        // blank above and below the paper
const CALCU_PAPER_TOAST_MS = 1800;  // how long "Copied to clipboard" shows

function useCalcuPaper(shell) {
  const store = calcuPaperStore(shell);
  const none = React.useRef({ papers: [], paper: () => calcuEmptyPaper(), copy: null, version: 0 }).current;
  return React.useSyncExternalStore(
    store ? store.subscribe : () => () => {},
    store ? store.getState : () => none,
    () => none);
}

function CalcuPaperControl({ control, onSet }) {
  const [held, setHeld] = React.useState(false);
  if (control.form === "key") {
    const down = held || !!control.on;
    const release = () => {
      if (!held) return;
      setHeld(false);
      onSet(control.id, false);
    };
    return (
      <button type="button" className={"calcu-paper-control" + (down ? " calcu-paper-down" : "")}
        data-calcu={`paper-control-${control.id}`} aria-pressed={down}
        onPointerDown={(e) => {
          if (e.currentTarget.setPointerCapture && e.pointerId !== undefined) {
            try { e.currentTarget.setPointerCapture(e.pointerId); } catch (_) { /* synthetic events */ }
          }
          setHeld(true);
          onSet(control.id, true);
        }}
        onPointerUp={release} onPointerCancel={release} onLostPointerCapture={release}>
        {control.id}
      </button>
    );
  }
  return (
    <button type="button" className={"calcu-paper-control" + (control.on ? " calcu-paper-down" : "")}
      data-calcu={`paper-control-${control.id}`} aria-pressed={!!control.on}
      onClick={() => onSet(control.id, !control.on)}>
      {control.id}
    </button>
  );
}

function CalcuPaperPanel({ shell, owner, controls, send }) {
  const state = useCalcuPaper(shell);
  const entry = calcuPaperOf(state.papers, owner);
  const name = entry ? entry.owner : "";
  const description = entry ? entry.description : null;
  const paper = entry ? state.paper(name) : calcuEmptyPaper();
  const textLayer = !!(description && description.textLayer);
  const [view, setView] = React.useState("marks");
  const showText = textLayer && view === "text";
  const usable = !!entry && !paper.empty;
  const sendTo = send || ((n, a) => (shell ? shell.send(n, a) : Promise.resolve({ done: false })));

  const scrollRef = React.useRef(null);
  const canvasRef = React.useRef(null);
  const lastHeight = React.useRef(null);   // the scroll height the last follow saw
  const [size, setSize] = React.useState({ width: 0, height: 0 });
  const [scrollTop, setScrollTop] = React.useState(0);

  React.useEffect(() => {
    const el = scrollRef.current;
    if (!el) return undefined;
    const update = () => setSize((old) => (old.width === el.clientWidth && old.height === el.clientHeight
      ? old : { width: el.clientWidth, height: el.clientHeight }));
    update();
    if (typeof ResizeObserver === "undefined") return undefined;
    const o = new ResizeObserver(update);
    o.observe(el);
    return () => o.disconnect();
  }, [showText]);

  // The whole width of the paper scaled to the panel.
  const scale = description && description.width > 0 && size.width > 0 ? size.width / description.width : 0;
  const reach = paper.reach || [0, 0];
  const contentHeight = scale > 0 && !paper.empty
    ? Math.ceil((reach[1] - reach[0]) * scale) + 2 * CALCU_PAPER_BLANK : 0;

  // Follow the newest marks when the view was at the end of the paper
  // as it was before it grew, or within 64 points of it; stay otherwise.
  // Measured here from the live scroll position, not from the last scroll
  // event, which may still be on its way.
  React.useLayoutEffect(() => {
    const el = scrollRef.current;
    if (!el || showText) return;
    // A view shown anew (after the text view) starts at the end.
    const before = lastHeight.current === null || lastHeight.el !== el ? el.scrollHeight : lastHeight.current;
    lastHeight.el = el;
    if (before - (el.scrollTop + el.clientHeight) <= CALCU_PAPER_FOLLOW) el.scrollTop = el.scrollHeight;
    lastHeight.current = el.scrollHeight;
    setScrollTop(el.scrollTop);
  }, [paper.version, contentHeight, showText]);

  const onScroll = () => {
    const el = scrollRef.current;
    if (el) setScrollTop(el.scrollTop);
  };

  // Only the marks in view are drawn.
  React.useEffect(() => {
    const canvas = canvasRef.current;
    if (!canvas || showText) return;
    const dpr = (typeof window !== "undefined" && window.devicePixelRatio) || 1;
    const w = Math.max(1, size.width), h = Math.max(1, size.height);
    if (canvas.width !== Math.round(w * dpr) || canvas.height !== Math.round(h * dpr)) {
      canvas.width = Math.round(w * dpr);
      canvas.height = Math.round(h * dpr);
    }
    const ctx = canvas.getContext("2d");
    if (!ctx) return;
    ctx.setTransform(1, 0, 0, 1, 0, 0);
    ctx.clearRect(0, 0, canvas.width, canvas.height);
    if (!scale || paper.empty) return;
    ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
    const top = reach[0] + (scrollTop - CALCU_PAPER_BLANK) / scale;
    calcuPaintMarks(ctx, description, paper.marks, scale, top, top + h / scale);
  });

  // The outcome of a Copy or Cut, in the panel, for 1.8 s.
  const [toast, setToast] = React.useState(null);
  const copy = state.copy;
  React.useEffect(() => {
    if (!copy || !name || copy.owner !== name) return undefined;
    setToast(copy.outcome === "failed" ? "Copy failed" : "Copied to clipboard");
    const t = setTimeout(() => setToast(null), CALCU_PAPER_TOAST_MS);
    return () => clearTimeout(t);
  }, [copy && copy.seq]);

  const copyOrCut = (command, args) => {
    armClipboard({ text: textLayer });   // in the gesture (hostServices.js armClipboard)
    Promise.resolve(sendTo(command, args)).then((r) => {
      if (!r || !r.done) calcuDisarmClipboard();
    });
  };
  const setControl = (id, on) => sendTo("set-control", { peripheral: owner, control: id, on });

  return (
    <div className="calcu-paper-panel" data-calcu="paper-panel">
      <div className="calcu-paper-header">
        <span className="calcu-paper-owner" data-calcu="paper-owner">{name}</span>
        {textLayer && (
          <button type="button" className="calcu-paper-view" data-calcu="paper-view" data-view={view}
            aria-pressed={view === "text"} onClick={() => setView(view === "text" ? "marks" : "text")}>
            {view === "text" ? "Text" : "Marks"}
          </button>
        )}
        {entry && (controls || []).map((c) => <CalcuPaperControl key={c.id} control={c} onSet={setControl} />)}
        <span className="calcu-paper-actions">
          <button type="button" data-calcu="paper-copy" disabled={!usable}
            onClick={() => copyOrCut("paper.copy", { owner: name })}>Copy</button>
          <button type="button" data-calcu="paper-cut" disabled={!usable}
            onClick={() => copyOrCut("paper.cut", { owner: name, copy: true })}>Cut</button>
        </span>
      </div>
      {showText ? (
        <pre className="calcu-paper-text" data-calcu="paper-text">{paper.lines.map((l) => l.text).join("\n")}</pre>
      ) : (
        <div className="calcu-paper-scroll" data-calcu="paper-scroll" ref={scrollRef} onScroll={onScroll}>
          <div className="calcu-paper-sheet" style={{ height: contentHeight }}>
            <canvas className="calcu-paper-canvas" data-calcu="paper-canvas" ref={canvasRef}
              style={{ top: scrollTop, width: size.width, height: size.height }} />
          </div>
        </div>
      )}
      {toast && <div className="calcu-paper-toast" data-calcu="paper-toast" role="status">{toast}</div>}
    </div>
  );
}
