// The canvas helpers both products' painters use:
//
//   sizeCanvas(canvas, width, height) → its 2D context: a canvas of `width`
//       × `height` CSS pixels at the screen's resolution, its context scaled
//       to CSS pixels (the backing store is set only when its size changes)
//   calcImage(name, onLoad) → the image or null: the file img/<name> beside
//       calc.js (CALC_BASE), loaded once; null until it has loaded (or when
//       it cannot be), and then `onLoad` is called once it has, to repaint

function sizeCanvas(canvas, width, height) {
  const ratio = window.devicePixelRatio || 1;
  const w = Math.max(1, Math.round(width * ratio)), h = Math.max(1, Math.round(height * ratio));
  if (canvas.width !== w) canvas.width = w;
  if (canvas.height !== h) canvas.height = h;
  const ctx = canvas.getContext("2d");
  ctx.setTransform(ratio, 0, 0, ratio, 0, 0);
  return ctx;
}

const calcImages = new Map();
function calcImage(name, onLoad) {
  let entry = calcImages.get(name);
  if (!entry) {
    const img = new Image();
    entry = { img, waiting: [] };
    img.onload = () => { entry.waiting.splice(0).forEach((f) => f()); };
    img.src = `${CALC_BASE}img/${name}`;
    calcImages.set(name, entry);
  }
  if (!entry.img.complete && onLoad) entry.waiting.push(onLoad);
  return entry.img.complete && entry.img.naturalWidth ? entry.img : null;
}
