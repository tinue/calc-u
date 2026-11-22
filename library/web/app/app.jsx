// The installed app page: the product's component (../calc.js) filling the
// viewport less the safe-area padding, with an install button where the
// browser allows it and a hint on iOS. library/web/build/dist.js compileApp()
// puts in the product's component, its own props and its install sentence.

const { useState: useAppState, useEffect: useAppEffect, useRef: useAppRef } = React;

// In the service worker's scope, so the engine starts offline (engine-worker.js).
const ENGINE_WORKER_URL = new URL("engine-worker.js", document.currentScript.src).href;

// Installed, the page covers the screen and keeps clear of its edges with
// the safe-area padding (app.css). In a browser tab its bars may float over
// the page (Safari on iOS), so there the page keeps to the area the browser
// leaves it.
const APP_REGISTRATION = "serviceWorker" in navigator
  ? navigator.serviceWorker.register(new URL("sw.js", document.currentScript.src)) : null;
if (APP_REGISTRATION) APP_REGISTRATION.catch(() => {});

const APP_STANDALONE = window.matchMedia("(display-mode: standalone)").matches || window.navigator.standalone === true;
if (!APP_STANDALONE) {
  const viewport = document.querySelector('meta[name="viewport"]');
  if (viewport) viewport.content = viewport.content.replace(/,\s*viewport-fit=cover/, "");
}

function rootSize(root) {
  const cs = getComputedStyle(root);
  const w = root.clientWidth - parseFloat(cs.paddingLeft) - parseFloat(cs.paddingRight);
  const h = root.clientHeight - parseFloat(cs.paddingTop) - parseFloat(cs.paddingBottom);
  return { width: Math.floor(w), height: Math.floor(h) };
}

// Install affordance: Chrome/Edge's `beforeinstallprompt` gets a real
// button; iOS/iPadOS has no programmatic install, so it gets a hint. iPadOS
// reports a Mac user agent; touch points tell it apart from a real Mac.
function useInstallPrompt() {
  const [mode, setMode] = useAppState("none");
  const deferredRef = useAppRef(null);

  useAppEffect(() => {
    if (APP_STANDALONE) return undefined;
    const isIOS = /iPad|iPhone|iPod/.test(navigator.userAgent) ||
      (navigator.platform === "MacIntel" && navigator.maxTouchPoints > 1);
    if (isIOS) { setMode("ios"); return undefined; }
    function onPrompt(e) {
      e.preventDefault();
      deferredRef.current = e;
      setMode("installable");
    }
    function onInstalled() { setMode("none"); }
    window.addEventListener("beforeinstallprompt", onPrompt);
    window.addEventListener("appinstalled", onInstalled);
    return () => {
      window.removeEventListener("beforeinstallprompt", onPrompt);
      window.removeEventListener("appinstalled", onInstalled);
    };
  }, []);

  async function install() {
    const evt = deferredRef.current;
    if (!evt) return;
    evt.prompt();
    await evt.userChoice;
    deferredRef.current = null;
    setMode("none");
  }
  return [mode, install];
}

function SymbolShare({ size, color }) {
  return (
    <svg width={size} height={size} viewBox="0 0 24 24" fill="none" stroke={color} strokeWidth="1.6" strokeLinecap="round" strokeLinejoin="round">
      <rect x="3.5" y="4" width="17" height="16" rx="3" />
      <path d="M12 14 V 7 M9 10 L 12 7 L 15 10" />
    </svg>
  );
}

function InstallBanner({ mode, onInstall, onDismiss }) {
  if (mode === "none") return null;
  return (
    <div className="install-banner">
      {mode === "ios" && <SymbolShare size={16} color="#0a84ff" />}
      <span className="install-banner-text">
        {mode === "installable"
          ? __CALCU_INSTALL_SENTENCE__
          : "Add to Home Screen: tap Share, then “Add to Home Screen”."}
      </span>
      {mode === "installable" && <button type="button" className="install-banner-cta" onClick={onInstall}>Install</button>}
      <button type="button" className="install-banner-close" aria-label="Dismiss" onClick={onDismiss}>×</button>
    </div>
  );
}

// A new release waits until no window of the app is open (sw.js); the
// page offers to take it at once: Reload activates the waiting worker, and
// the page reloads when it takes over (every window of the app does).
function useNewVersion(registration) {
  const [waiting, setWaiting] = useAppState(null);
  useAppEffect(() => {
    if (!registration) return undefined;
    // A page the old release controls: when the new one takes over, reload.
    // A first install's takeover is no new version.
    const updating = !!navigator.serviceWorker.controller;
    const offer = (worker) => { if (worker && updating) setWaiting(worker); };
    const onChange = () => { if (updating) window.location.reload(); };
    registration.then((r) => {
      offer(r.waiting);
      r.addEventListener("updatefound", () => {
        const w = r.installing;
        if (w) w.addEventListener("statechange", () => { if (w.state === "installed") offer(w); });
      });
    }, () => {});
    navigator.serviceWorker.addEventListener("controllerchange", onChange);
    return () => navigator.serviceWorker.removeEventListener("controllerchange", onChange);
  }, [registration]);
  const reload = () => { if (waiting) waiting.postMessage({ type: "skip-waiting" }); };
  return [!!waiting, reload];
}

function NewVersionBanner({ onReload, onDismiss }) {
  return (
    <div className="install-banner" data-calcu="new-version">
      <span className="install-banner-text">A new version is ready.</span>
      <button type="button" className="install-banner-cta" onClick={onReload}>Reload</button>
      <button type="button" className="install-banner-close" aria-label="Dismiss" onClick={onDismiss}>×</button>
    </div>
  );
}

function StandaloneApp() {
  const [size, setSize] = useAppState(() => rootSize(document.getElementById("root")));
  const [installMode, install] = useInstallPrompt();
  const [dismissed, setDismissed] = useAppState(false);
  const [newVersion, reload] = useNewVersion(APP_REGISTRATION);
  const [laterVersion, setLaterVersion] = useAppState(false);

  // A ResizeObserver also sees the safe-area insets arrive after launch.
  useAppEffect(() => {
    const root = document.getElementById("root");
    const update = () => setSize((old) => {
      const s = rootSize(root);
      return s.width === old.width && s.height === old.height ? old : s;
    });
    const observer = new ResizeObserver(update);
    observer.observe(root);
    window.addEventListener("pageshow", update);
    return () => {
      observer.disconnect();
      window.removeEventListener("pageshow", update);
    };
  }, []);

  // WebKit in a Home Screen app can come back from a rotation scrolled by
  // the status bar's height while drawing as if it weren't: undo it.
  useAppEffect(() => {
    const unscroll = () => { if (window.scrollX || window.scrollY) window.scrollTo(0, 0); };
    const viewport = window.visualViewport;
    window.addEventListener("scroll", unscroll);
    window.addEventListener("resize", unscroll);
    if (viewport) viewport.addEventListener("scroll", unscroll);
    return () => {
      window.removeEventListener("scroll", unscroll);
      window.removeEventListener("resize", unscroll);
      if (viewport) viewport.removeEventListener("scroll", unscroll);
    };
  }, []);

  return (
    <>
      <__CALCU_APP_COMPONENT__ {...__CALCU_APP_PROPS__} width={size.width} height={size.height} keyboard={true} workerUrl={ENGINE_WORKER_URL} />
      {newVersion && !laterVersion
        ? <NewVersionBanner onReload={reload} onDismiss={() => setLaterVersion(true)} />
        : !dismissed && <InstallBanner mode={installMode} onInstall={install} onDismiss={() => setDismissed(true)} />}
    </>
  );
}


ReactDOM.createRoot(document.getElementById("root")).render(<StandaloneApp />);
