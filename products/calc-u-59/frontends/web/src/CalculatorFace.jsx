// The calculator face: the artwork of the current model, the LED display
// and the cue card on it, the pressed-key effect (key rect blacked out, the
// artwork shifted 3 px), as Qt6/app/KeyboardWidget and App/Views/
// KeyboardView. The pointer is the React set's (useShellFaceplate, LIB-14
// AC-24, AC-43; M-57) on the product's hit map `c59.faceplate-map` (the
// widened key rects of app/KeyboardLayout, in pixels of the 2064 × 4075 base
// image, distance 0, the display's full-speed hold): a key is fixed at
// pointer down (LIB-14 D-9). A press on a corner of the fitted magnetic card
// is the product's first (C59-7 D-12): `c59.tap-card`. The key drawn pressed
// is the last one live input holds (`held-keys`, LIB-5 AC-19).
//
// Transparent buttons over the keys, named by the key's face label, make
// the keys reachable for assistive technology (and tests); pointer events on
// them bubble to the face's own handler.

const KEY_LABELS = [
  "A", "B", "C", "D", "E",
  "2nd", "INV", "lnx", "CE", "CLR",
  "LRN", "x⇄t", "x²", "√x", "1/x",
  "SST", "STO", "RCL", "SUM", "yˣ",
  "BST", "EE", "(", ")", "÷",
  "GTO", "7", "8", "9", "×",
  "SBR", "4", "5", "6", "−",
  "RST", "1", "2", "3", "+",
  "R/S", "0", ".", "+/-", "=",
];

// app/KeyboardLayout kCanvasAspect (width / height) and the base image's
// width (the hit map's units), known before the session is: the layout and
// the prerender need them.
const CANVAS_ASPECT = 2064 / 4075;
const CANVAS_WIDTH = 2064;
// Before `c59.faceplate-map` answered: where the display and the card sit
// (fractions of the canvas, KeyboardLayout's kDisplayRect, kCardRect).
const DISPLAY_RECT = { x: 0, y: 0.052, w: 1, h: 0.05 };
const CARD_RECT = { x: 0, y: 0.11, w: 1, h: 0.108 };
const KEY_BUTTON_POINTER = 1002;   // the key buttons' pointer (not the faceplate's)

const artworkFor = (model) => (model === "TI-59" ? "ti59_base" : "ti58_base");
const PRESS_SHIFT = 3;

// The page's hit map form (useShellFaceplate) from the map's Value form.
function hitMapOf(faceplate) {
  if (!faceplate) return null;
  const m = faceplate.map;
  return { keys: m.keys.map((k) => [k.name, { x: k.x, y: k.y, w: k.w, h: k.h }]), display: m.display || null,
    distance: m.distance || 0, width: CANVAS_WIDTH };
}

// The LED cells of `c59.web.display` (float32 bytes), repainted once per
// animation frame without a React render.
function useDisplayFrames(shell, onFrame) {
  React.useEffect(() => {
    if (!shell) return undefined;
    let last = null, pending = false;
    const look = () => {
      const bytes = shell.getState().topics["c59.web.display"];
      if (!bytes || bytes === last) return;
      last = bytes;
      if (pending || typeof requestAnimationFrame === "undefined") return;
      pending = true;
      requestAnimationFrame(() => {
        pending = false;
        onFrame.current(new Float32Array(last.buffer, last.byteOffset, last.byteLength / 4));
      });
    };
    look();
    return shell.subscribe(look);
  }, [shell]);
}

function LedCanvas({ shell, width, height }) {
  const ref = React.useRef(null);
  const [geometry, setGeometry] = React.useState(null);
  const cells = React.useRef(null);
  const draw = React.useRef(() => {});

  React.useEffect(() => {
    if (!shell) return;
    shell.request("c59.led-geometry", { w: width / 12, h: height }).then((r) => { if (r.done) setGeometry(r.value); });
  }, [shell, width, height]);
  const paths = React.useMemo(() => (geometry ? ledPaths(width, height, geometry) : null), [geometry, width, height]);

  React.useEffect(() => {
    const ctx = sizeCanvas(ref.current, width, height);
    draw.current = (c) => {
      if (c) cells.current = c;
      if (paths) paintLed(ctx, width, height, cells.current, geometry, paths);
    };
    draw.current();
  }, [paths, width, height]);
  useDisplayFrames(shell, draw);

  return <canvas ref={ref} className="calcu-led" style={{ width, height }} aria-hidden="true" />;
}

function CueCardCanvas({ shell, width, height }) {
  const ref = React.useRef(null);
  const card = useTopic(shell, "c59.web.cue-card");
  const [loaded, setLoaded] = React.useState(0);

  React.useEffect(() => {
    if (shell) shell.send("c59.web.cue-card-size", { width, height });
  }, [shell, width, height]);

  React.useEffect(() => {
    const ctx = sizeCanvas(ref.current, width, height);
    ctx.clearRect(0, 0, width, height);
    if (card && Math.abs(card.width - width) < 0.5) paintCueCard(ctx, card, () => setLoaded((n) => n + 1));
  }, [card, width, height, loaded]);

  return (
    <canvas
      ref={ref}
      className="cuecard"
      role="img"
      aria-label={card ? card.label : "No cue card"}
      style={{ width, height, visibility: card ? "visible" : "hidden" }}
    />
  );
}

// `faceplate`: the reply of `c59.faceplate-map` for `model` (null until it came).
function CalculatorFace({ shell, faceplate, model, width }) {
  const held = useTopic(shell, "held-keys");
  const fullSpeed = useTopic(shell, "busy", (b) => !!b && b.speed === "full");   // LIB-3's speed now
  const media = useTopic(shell, "media");
  const reader = ((media && media.places) || []).find((p) => p.place === "card-reader") || null;
  const cardFitted = !!(reader && reader.holds);
  const height = width / CANVAS_ASPECT;
  const scale = width / CANVAS_WIDTH;
  const baseRef = React.useRef(null);
  const faceRef = React.useRef(null);
  const [loaded, setLoaded] = React.useState(0);
  const heldKey = held && held.live && held.live.length ? held.live[held.live.length - 1] : null;
  const keyRects = faceplate ? faceplate.keyRects : null;
  const pressed = keyRects ? keyRects.find((k) => k.name === heldKey) || null : null;

  // Artwork and the pressed key.
  React.useEffect(() => {
    const ctx = sizeCanvas(baseRef.current, width, height);
    ctx.clearRect(0, 0, width, height);
    const img = calcImage(artworkFor(model), () => setLoaded((n) => n + 1));
    if (!img) return;
    ctx.imageSmoothingQuality = "high";
    ctx.drawImage(img, 0, 0, width, height);
    if (pressed) {
      const sx = img.naturalWidth / width, sy = img.naturalHeight / height;
      const r = { x: pressed.x * scale, y: pressed.y * scale, w: pressed.w * scale, h: pressed.h * scale };
      ctx.fillStyle = "#000";
      ctx.fillRect(r.x, r.y, r.w, r.h);
      ctx.save();
      ctx.beginPath();
      ctx.rect(r.x, r.y, r.w, r.h);
      ctx.clip();
      ctx.drawImage(img, r.x * sx, r.y * sy, r.w * sx, r.h * sy, r.x + PRESS_SHIFT, r.y + PRESS_SHIFT, r.w, r.h);
      ctx.restore();
    }
  }, [model, pressed, width, height, loaded]);

  // The pointer (LIB-14 AC-24, AC-43): the shell's hit rule; a press on a
  // corner of the fitted card is the card's (C59-7 D-12, design §10.3.4).
  const hitMap = React.useMemo(() => hitMapOf(faceplate), [faceplate]);
  const scaleRef = React.useRef(scale);
  scaleRef.current = scale;
  const fittedRef = React.useRef(cardFitted);
  fittedRef.current = cardFitted;
  const onCorner = (x, y) => {
    if (!faceplate || !fittedRef.current) return false;
    const c = faceplate.card;
    if (x < c.x || x >= c.x + c.w || y < c.y || y >= c.y + c.h) return false;
    const fx = (x - c.x) / c.w, fy = (y - c.y) / c.h;
    return faceplate.cardCorners.some(([cx, cy, cw, ch]) => fx >= cx && fx < cx + cw && fy >= cy && fy < cy + ch);
  };
  useShellFaceplate(shell, hitMap, faceRef, {
    scale: () => scaleRef.current,
    productFirst: (x, y) => {
      if (!onCorner(x, y)) return false;
      shell.send("c59.tap-card", { x: x / faceplate.canvas.w, y: y / faceplate.canvas.h });
      return true;
    },
  });

  // A key button activated without a pointer (keyboard, screen reader).
  const onKeyClick = (name) => (e) => {
    if (e.detail !== 0 || !shell) return;
    shell.send("screen-key-down", { key: name, pointer: KEY_BUTTON_POINTER });
    setTimeout(() => shell.send("screen-key-up", { key: name, pointer: KEY_BUTTON_POINTER }), 150);
  };

  const canvas = faceplate ? faceplate.canvas : { w: CANVAS_WIDTH, h: CANVAS_WIDTH / CANVAS_ASPECT };
  const frac = (r) => ({ x: r.x / canvas.w, y: r.y / canvas.h, w: r.w / canvas.w, h: r.h / canvas.h });
  const d = faceplate && faceplate.map.display ? frac(faceplate.map.display) : DISPLAY_RECT;
  const c = faceplate ? frac(faceplate.card) : CARD_RECT;
  return (
    <div
      className="calcu-face"
      ref={faceRef}
      data-calcu="faceplate"
      data-full-speed={fullSpeed}
      style={{ width, height }}
      onMouseDown={(e) => e.preventDefault()}
      onContextMenu={(e) => e.preventDefault()}
    >
      <canvas ref={baseRef} className="calcu-keyboard" style={{ width, height }} aria-hidden="true" />
      <div className="calcu-overlay" style={{ left: d.x * width, top: d.y * height }}>
        <LedCanvas shell={shell} width={d.w * width} height={d.h * height} />
      </div>
      <div className="calcu-overlay" style={{ left: c.x * width, top: c.y * height }}>
        <CueCardCanvas shell={shell} width={c.w * width} height={c.h * height} />
      </div>
      {keyRects && keyRects.map((k, i) => (
        <button
          key={k.name}
          type="button"
          tabIndex={-1}
          className="calcu-key"
          aria-label={KEY_LABELS[i]}
          aria-pressed={heldKey === k.name}
          onClick={onKeyClick(k.name)}
          style={{ left: k.x * scale, top: k.y * scale, width: k.w * scale, height: k.h * scale }}
        />
      ))}
      {faceplate && cardFitted && [0, 1].map((side) => {
        const [cx, cy, cw, ch] = faceplate.cardCorners[side];
        return (
          <button
            key={`side${side}`}
            type="button"
            tabIndex={-1}
            className="calcu-key"
            aria-label={`Feed card side ${side + 1}`}
            // A pointer press is the product's first (productFirst); this is for assistive technology.
            onClick={(e) => { if (e.detail === 0 && shell) shell.send("media.feed", { place: "card-reader", side }); }}
            style={{ left: (c.x + c.w * cx) * width, top: (c.y + c.h * cy) * height,
                     width: c.w * cw * width, height: c.h * ch * height }}
          />
        );
      })}
    </div>
  );
}
