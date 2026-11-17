// The calculator face, as Qt6/app/FaceplateWidget + LcdWidget: the model's
// artwork scaled to `width` (aspect kept), the LCD painted over its
// rectangle from the library's `display` topic, a translucent white flash on
// each key live input holds (the topic `held-keys`, LIB-5 AC-19; its
// button, calc.css). The pointer is the React set's (useShellFaceplate,
// LIB-14 AC-24, AC-43): the key under it by the shell's one rule with the
// hit map of `c1600.faceplate-map` (24 px around a key, D-8), several
// pointers, holding the LCD runs at full speed.
//
// Key rectangles and the LCD rectangle are app/KeyLayout's, in art pixels.
// Transparent buttons over the keys, named by key, serve assistive
// technology and tests; they take no pointer of their own.

const FACE_ART = { "PC-1500": "pc1500.png", "PC-1500A": "pc1500A.png", "PC-1600": "pc1600.png" };
// Width / height of each artwork, known before the session is (layout and
// prerender need it): app/KeyLayout's imageWidth / imageHeight.
const FACE_ASPECT = { "PC-1500": 4000 / 1760, "PC-1500A": 4000 / 1760, "PC-1600": 4000 / 1777 };

// The faceplate map's hit map in the form useShellFaceplate takes.
function hitMapOf(map) {
  if (!map) return null;
  return {
    keys: map.keys.map((k) => [k.name, { x: k.x, y: k.y, w: k.w, h: k.h }]),
    display: map.display,
    distance: map.distance,
    width: map.width,
  };
}

function LcdCanvas({ shell, lcd, rect }) {
  const ref = React.useRef(null);
  React.useEffect(() => {
    const canvas = ref.current;
    if (!canvas) return undefined;
    const ctx = sizeCanvas(canvas, rect.w, rect.h);
    const draw = () => paintLcd(ctx, lcd, shell ? shell.frames.display : null, rect.w, rect.h);
    draw();
    if (!shell) return undefined;
    shell.frames.onFrame = draw;
    return () => { if (shell.frames.onFrame === draw) shell.frames.onFrame = null; };
  }, [shell, lcd, rect.w, rect.h]);
  return (
    <canvas ref={ref} className="calcu-lcd" data-calcu="lcd"
      style={{ left: rect.x, top: rect.y, width: rect.w, height: rect.h }} aria-label="Display" role="img" />
  );
}

function Faceplate({ shell, model, map, width, shiftFlash }) {
  const height = width / (FACE_ASPECT[model] || FACE_ASPECT["PC-1500A"]);
  const layout = map && map.model === model ? map : null;
  const scale = layout ? width / layout.width : 0;
  const artRef = React.useRef(null);
  const faceRef = React.useRef(null);
  const [loaded, setLoaded] = React.useState(0);
  const held = useTopic(shell, "held-keys");
  // A host SHIFT tap (LIB-5 AC-33) is down and up at once, too short for the
  // held keys: `shiftFlash` counts the taps, each drawn pressed for a moment.
  const [flashing, setFlashing] = React.useState(false);
  React.useEffect(() => {
    if (!shiftFlash) return undefined;
    setFlashing(true);
    const t = setTimeout(() => setFlashing(false), 150);
    return () => clearTimeout(t);
  }, [shiftFlash]);
  const heldLive = (held && held.live) || [];
  const live = flashing && !heldLive.includes("shift") ? [...heldLive, "shift"] : heldLive;
  const hitMap = React.useMemo(() => hitMapOf(layout), [layout]);
  // The shell's productFirst hook (LIB-14 D-44) notes where the pointer went
  // down, in art pixels, and never takes the press: a held key with several
  // rectangles (the PC-1600's two SHIFT keys, one machine key) is drawn on
  // the one pressed, else on the first (app/KeyLayout's heldKeyIndex).
  const [press, setPress] = React.useState(null);
  const notePress = React.useCallback((x, y) => { setPress({ x, y }); return false; }, []);
  React.useEffect(() => setPress(null), [model]);
  useShellFaceplate(shell, hitMap, faceRef, { productFirst: notePress });
  const heldIndex = (name) => {
    const keys = layout ? layout.keys : [];
    const hit = press && keys.findIndex((k) => k.name === name && press.x >= k.x && press.x < k.x + k.w
      && press.y >= k.y && press.y < k.y + k.h);
    return hit !== null && hit >= 0 ? hit : keys.findIndex((k) => k.name === name);
  };

  const scaled = (r) => ({ x: r.x * scale, y: r.y * scale, w: r.w * scale, h: r.h * scale });

  React.useEffect(() => {
    const ctx = sizeCanvas(artRef.current, width, height);
    ctx.clearRect(0, 0, width, height);
    const img = calcImage(FACE_ART[model] || FACE_ART["PC-1500A"], () => setLoaded((n) => n + 1));
    if (img) {
      ctx.imageSmoothingQuality = "high";
      ctx.drawImage(img, 0, 0, width, height);
    }
  }, [model, width, height, loaded]);

  return (
    <div className="calcu-face" data-calcu="faceplate" data-model={model} ref={faceRef} style={{ width, height }}
      onContextMenu={(e) => e.preventDefault()}>
      <canvas ref={artRef} className="calcu-face-art" style={{ width, height }} aria-hidden="true" />
      {layout && <LcdCanvas shell={shell} lcd={layout.lcd} rect={scaled(layout.display)} />}
      {layout && layout.keys.map((k, i) => {
        const r = scaled(k);
        return (
          <button key={`${k.name}-${i}`} type="button" tabIndex={-1} className="calcu-key" data-key={k.name}
            aria-label={k.name} aria-pressed={live.includes(k.name) && heldIndex(k.name) === i}
            style={{ left: r.x, top: r.y, width: r.w, height: r.h }} />
        );
      })}
    </div>
  );
}
