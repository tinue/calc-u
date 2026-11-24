// useCalcuPaperWindow -- the paper panel in a window of its own on a
// desktop page (Qt6: qt6/PaperWindow): a browser window without the
// browser's bars (window.open with `popup`), its document made here with the
// page's stylesheets, the panel drawn into it through a React portal, so it
// shares the page's shell and paper store; no second worker.
//
//   const pw = useCalcuPaperWindow(shell, {offered, title, notify});
//   pw.own                   the paper is in its own window
//   pw.toggle()              into its own window (inside the click: a
//                            browser opens a window only within a gesture),
//                            or back
//   pw.portal(element, style) the element drawn into the window, on a root
//                            with the client's chrome custom properties
//                            (null while the paper is in the page)
//
// offered: the page offers the window (a desktop page of the app, not an
// embed, not mobile); title: the window's title; notify(text): a page
// notice.
// Closing the window puts the paper back; the page's own end closes the
// window and keeps [window] paper-own-window, so that the next start opens
// it again. A browser refuses a window outside a gesture, so that start
// may be refused: the paper stays in the page and a notice says so.
// The window's place and inner size are [window] paper-x, paper-y,
// paper-width, paper-height, written through the session's
// `remember-window` (the settings file in the worker, as every setting).

const CALCU_PAPER_WINDOW_POLL_MS = 1000;   // a browser tells no move: its place is read this often
const CALCU_PAPER_WINDOW_BLOCKED =
  "The browser did not open the printer's window; the button on the paper opens it.";

function calcuPaperWindowNumber(win, key, fallback) {
  const v = parseInt(win && win[key], 10);
  return Number.isFinite(v) ? v : fallback;
}

function useCalcuPaperWindow(shell, { offered, title, notify }) {
  const win = useTopic(shell, "settings", (st) => (st && st.window) || null);
  const [popup, setPopup] = React.useState(null);   // {view, root}
  const popupRef = React.useRef(null);
  popupRef.current = popup;
  const ending = React.useRef(false);                // the page itself goes
  const restored = React.useRef(false);
  const winRef = React.useRef(win);
  winRef.current = win;

  const remember = React.useCallback((values) => {
    if (shell) Promise.resolve(shell.send("remember-window", { values })).catch(() => {});
  }, [shell]);

  const placeOf = (view) => ({
    "paper-x": Math.round(view.screenX), "paper-y": Math.round(view.screenY),
    "paper-width": Math.round(view.innerWidth), "paper-height": Math.round(view.innerHeight),
  });

  const back = React.useCallback((closeIt) => {
    const p = popupRef.current;
    if (!p) return;
    popupRef.current = null;
    setPopup(null);
    remember(p.view.closed ? { "paper-own-window": false } : { ...placeOf(p.view), "paper-own-window": false });
    p.stop();
    if (closeIt && !p.view.closed) p.view.close();
  }, [remember]);

  // The window, made now; null where the browser refuses it.
  const open = React.useCallback(() => {
    if (popupRef.current) { popupRef.current.view.focus(); return true; }
    const w = winRef.current;
    const width = calcuPaperWindowNumber(w, "paper-width", 400), height = calcuPaperWindowNumber(w, "paper-height", 640);
    let features = `popup=yes,width=${width},height=${height}`;
    if (w && w["paper-x"] !== "" && w["paper-y"] !== "" && w["paper-x"] !== undefined)
      features += `,left=${calcuPaperWindowNumber(w, "paper-x", 0)},top=${calcuPaperWindowNumber(w, "paper-y", 0)}`;
    let view = null;
    try { view = window.open("", "calcu-paper-window", features); } catch (_) { view = null; }
    if (!view) return false;
    const d = view.document;
    d.title = title || "Printer";
    d.documentElement.style.height = "100%";
    d.body.style.margin = "0";
    d.body.style.height = "100%";
    // The page's stylesheets, at their absolute addresses (the window's
    // blank document has no base of its own to resolve them against).
    for (const el of document.querySelectorAll('link[rel="stylesheet"], style')) {
      const copy = d.importNode(el, true);
      if (el.tagName === "LINK") copy.href = el.href;
      d.head.appendChild(copy);
    }
    const root = d.createElement("div");
    d.body.appendChild(root);
    // Its place as it changes (a browser tells a resize, never a move).
    let last = JSON.stringify(placeOf(view));
    const check = () => {
      if (view.closed) return;
      const now = JSON.stringify(placeOf(view));
      if (now !== last) { last = now; remember(placeOf(view)); }
    };
    const poll = setInterval(check, CALCU_PAPER_WINDOW_POLL_MS);
    const closed = () => { if (!ending.current) back(false); };
    view.addEventListener("resize", check);
    view.addEventListener("pagehide", closed);
    const stop = () => {
      clearInterval(poll);
      try { view.removeEventListener("pagehide", closed); view.removeEventListener("resize", check); } catch (_) { /* gone */ }
    };
    const p = { view, root, stop };
    popupRef.current = p;
    setPopup(p);
    remember({ "paper-own-window": true });
    view.focus();
    return true;
  }, [title, remember, back]);

  const toggle = React.useCallback(() => {
    if (popupRef.current) back(true);
    else if (!open() && notify) notify(CALCU_PAPER_WINDOW_BLOCKED);
  }, [open, back, notify]);

  // The page's end takes the window with it; the setting stays.
  React.useEffect(() => {
    const end = () => {
      ending.current = true;
      const p = popupRef.current;
      if (p && !p.view.closed) p.view.close();
    };
    window.addEventListener("pagehide", end);
    return () => window.removeEventListener("pagehide", end);
  }, []);

  // At the start, the window again where the last run left it.
  React.useEffect(() => {
    if (!offered || !win || restored.current) return;
    restored.current = true;
    if (win["paper-own-window"] === "true" && !open() && notify) notify(CALCU_PAPER_WINDOW_BLOCKED);
  }, [offered, win, open, notify]);

  const portal = (element, style) => {
    if (!popup) return null;
    return ReactDOM.createPortal(
      <div className="calcu-client calcu-paper-window" data-calcu="paper-window-root" style={style}>{element}</div>,
      popup.root);
  };
  return { own: !!popup, toggle, portal, view: popup ? popup.view : null };
}
