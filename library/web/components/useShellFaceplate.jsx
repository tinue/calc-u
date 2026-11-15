// useShellFaceplate -- the pointer on a product's faceplate element (LIB-14
// design §3.7, §3.10; spec AC-24, AC-43, EC-8): pointerdown, move, up and
// cancel through calcuShellPointer, with pointer capture, sending
// screen-key-down/up {key, pointer} (LIB-5) and speed {full} (LIB-3). The
// hit map is the product's, in its own units; `scale` gives the logical
// pixels per unit as shown now (default: the element's width over the hit
// map's `width`). The product draws pressed keys from live input (H6).
//
//   useShellFaceplate(shell, hitMap, faceplateRef, scale?)
//
// shell: LIB-15's shell (createShell); hitMap: {keys: [[key, {x, y, w, h}]],
// display, distance, width}.

function useShellFaceplate(shell, hitMap, ref, scale) {
  const tracker = React.useRef(null);
  const scaleRef = React.useRef(scale);
  scaleRef.current = scale;
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
    const at = (e) => {
      const r = el.getBoundingClientRect();
      return [e.clientX - r.left, e.clientY - r.top];
    };
    const down = (e) => {
      if (e.button !== undefined && e.button !== 0) return;
      try { el.setPointerCapture(e.pointerId); } catch (_) { /* synthetic events */ }
      send(t.down(e.pointerId, ...at(e)));
      e.preventDefault();
    };
    const move = (e) => send(t.move(e.pointerId, ...at(e)));
    const up = (e) => send(t.up(e.pointerId));
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
