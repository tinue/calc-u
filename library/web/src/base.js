// The directory calc.js was loaded from: the worker, its WASM, the bundled files and the
// artwork sit beside it, wherever dist/ is hosted. Captured while the
// script runs (document.currentScript is null afterwards); without one -- a
// server-side prerender -- relative to the page, which a render never uses.
const CALC_BASE = (() => {
  const script = typeof document !== "undefined" ? document.currentScript : null;
  if (script && script.src) return new URL(".", script.src).href;
  return typeof location !== "undefined" ? new URL(".", location.href).href : "";
})();
