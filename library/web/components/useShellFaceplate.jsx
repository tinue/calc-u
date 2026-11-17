// useShellFaceplate -- the pointer on a product's faceplate element (LIB-14
// design §3.7, §3.10; spec AC-24, AC-43, EC-8): pointerdown, move, up and
// cancel through calcuShellPointer, with pointer capture, sending
// screen-key-down/up {key, pointer} (LIB-5) and speed {full} (LIB-3). The
// hit map is the product's, in its own units; `scale` gives the logical
// pixels per unit as shown now (default: the element's width over the hit
// map's `width`). The product draws pressed keys from live input (H6).
//
//   useShellFaceplate(shell, hitMap, faceplateRef, scale?)
//   useShellFaceplate(shell, hitMap, faceplateRef, {scale, origin, productFirst})
//
// shell: LIB-15's shell (createShell); hitMap: {keys: [[key, {x, y, w, h}]],
// display, distance, width}. Part E (design §10.3.4, D-44): `origin` (a
// point {x, y} or a function giving it) is the hit map's (0, 0) in the
// element, in logical pixels; `productFirst(x, y)` gets a press in hit-map
// units first, and a press it takes (true) is not the faceplate's: its
// moves and release pass by as well. A number or a function as the fourth
// argument is `scale`, as before.

function useShellFaceplate(shell, hitMap, ref, options) {
  const opts = options !== null && typeof options === "object" ? options : { scale: options };
  const tracker = React.useRef(null);
  const scaleRef = React.useRef(opts.scale);
  scaleRef.current = opts.scale;
  const originRef = React.useRef(opts.origin);
  originRef.current = opts.origin;
  const firstRef = React.useRef(opts.productFirst);
  firstRef.current = opts.productFirst;
  React.useEffect(() => {
    const el = ref.current;
    if (!el || !shell || !hitMap) return undefined;
    const currentScale = () => {
      if (typeof scaleRef.current === "function") return scaleRef.current();
      if (typeof scaleRef.current === "number") return scaleRef.current;
      return hitMap.width ? el.clientWidth / hitMap.width : 1;
    };
    const t = calcuShellPointer(hitMap, currentScale);
    tracker.current = t;
    const send = (out) => { for (const c of out) shell.send(c.name, c.args); };
    const origin = () => {
      const o = typeof originRef.current === "function" ? originRef.current() : originRef.current;
      return o || { x: 0, y: 0 };
    };
    const at = (e) => {
      const r = el.getBoundingClientRect();
      const o = origin();
      return [e.clientX - r.left - o.x, e.clientY - r.top - o.y];
    };
    const passing = new Set();   // pointers whose press the product took
    const down = (e) => {
      if (e.button !== undefined && e.button !== 0) return;
      const [x, y] = at(e);
      const sc = currentScale();
      if (firstRef.current && firstRef.current(x / sc, y / sc)) { passing.add(e.pointerId); return; }
      try { el.setPointerCapture(e.pointerId); } catch (_) { /* synthetic events */ }
      send(t.down(e.pointerId, x, y));
      e.preventDefault();
    };
    const move = (e) => { if (!passing.has(e.pointerId)) send(t.move(e.pointerId, ...at(e))); };
    const up = (e) => {
      if (passing.delete(e.pointerId)) return;
      send(t.up(e.pointerId));
    };
    const blur = () => send(t.releaseAll());
    el.addEventListener("pointerdown", down);
    el.addEventListener("pointermove", move);
    el.addEventListener("pointerup", up);
    el.addEventListener("pointercancel", up);
    window.addEventListener("blur", blur);
    return () => {
      send(t.releaseAll());
      el.removeEventListener("pointerdown", down);
      el.removeEventListener("pointermove", move);
      el.removeEventListener("pointerup", up);
      el.removeEventListener("pointercancel", up);
      window.removeEventListener("blur", blur);
    };
  }, [shell, hitMap, ref]);
  return tracker;
}
