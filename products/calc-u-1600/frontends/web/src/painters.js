// Canvas painters: the LCD (Qt6/app/LcdWidget.cpp's algorithm). What is lit
// is the library's `display` topic ({on, frame: {width, height, dots},
// indicators: [{name, on}]}); where each indicator goes and the colours are
// the `lcd` part of the request `c1600.faceplate-map` (app/LcdLayout: the
// same table the Qt6 LCD draws with). Drawing only. The
// plotters' paper is the library's paper panel; sizeCanvas and
// calcImage are the library's (library/web/src/canvas.js).

const MONO_FONT = '"SF Mono", Menlo, Consolas, "DejaVu Sans Mono", "Liberation Mono", monospace';

const rgb = (c) => `rgb(${c[0]},${c[1]},${c[2]})`;

// The indicators lit in `display`, laid out by the model's slots
// (lcd::indicators of app/LcdLayout.cpp, kind by kind): [{text, x, scale, style}].
function lcdIndicators(slots, display) {
  const lit = (name) => ((display && display.indicators) || []).some((i) => i.name === name && i.on);
  const out = [];
  for (const s of slots || []) {
    const at = { x: s.x, scale: s.scale, style: s.style };
    if (s.kind === "flag") {
      if (lit(s.names[0])) out.push({ text: s.caption, ...at });
    } else if (s.kind === "join") {
      const text = s.names.filter(lit).join("");
      if (text) out.push({ text, ...at });
    } else if (s.kind === "first") {
      const first = s.names.find(lit);
      if (first) out.push({ text: first, ...at });
    } else if (s.kind === "always") {
      out.push({ text: s.caption, ...at });
    }
  }
  return out;
}

function paintDotMatrix(ctx, display, x, y, w, h, ink) {
  const f = display && display.frame;
  if (!f || f.width <= 0 || f.height <= 0 || !display.on) return;
  const dotSpacing = w / f.width;
  const rowSpacing = h / f.height;
  const dot = Math.min(dotSpacing, rowSpacing) * 0.9;
  ctx.fillStyle = ink;
  const dots = f.dots;
  for (let row = 0; row < f.height; row++) {
    for (let col = 0; col < f.width; col++) {
      if (!dots[row * f.width + col]) continue;
      const cx = x + (col + 0.5) * dotSpacing;
      const cy = y + (row + 0.5) * rowSpacing;
      ctx.fillRect(cx - dot / 2, cy - dot / 2, dot, dot);
    }
  }
}

function paintLcdPC1500(ctx, display, width, height, L) {
  const background = rgb(L.background), ink = rgb(L.ink);
  ctx.fillStyle = background;
  ctx.fillRect(0, 0, width, height);
  if (!display || !display.on) return;
  // 4 px horizontal padding on the whole panel.
  const left = 4, totalWidth = width - 8, totalHeight = height;
  const effectiveHeight = totalHeight * L.p15ContentScale;
  const barHeight = effectiveHeight * L.p15IndicatorBarFraction / (1 + L.p15IndicatorBarFraction);
  const canvasHeight = effectiveHeight - barHeight;
  const fontSize = effectiveHeight * L.p15IndicatorFontHeightFraction;
  const topInset = effectiveHeight * L.p15IndicatorTopInsetFraction;
  const matrixTopGap = effectiveHeight * L.p15MatrixTopGapFraction;

  ctx.font = `600 ${Math.max(1, Math.floor(fontSize))}px ${MONO_FONT}`;
  ctx.textBaseline = "top";
  ctx.fillStyle = ink;
  const y = topInset;
  for (const ind of lcdIndicators(L.indicators, display)) {
    const x = left + ind.x * totalWidth;
    if (ind.style === "dot") {
      const d = fontSize * 0.4;
      ctx.beginPath();
      ctx.ellipse(x + d / 2, y + d / 2, d / 2, d / 2, 0, 0, Math.PI * 2);
      ctx.fill();
      continue;
    }
    ctx.fillText(ind.text, x, y);
  }
  paintDotMatrix(ctx, display, left, barHeight + matrixTopGap, totalWidth, canvasHeight, ink);
}

function paintLcdPC1600(ctx, display, width, height, L) {
  const background = rgb(L.background), ink = rgb(L.ink);
  ctx.fillStyle = background;
  ctx.fillRect(0, 0, width, height);
  if (!display || !display.on) return;
  const fontSize = height * L.p16LabelFontHeightFraction;
  const rowHeight = height * L.p16RowHeightFraction;
  const rowTop = height * L.p16IndicatorRowTop;
  ctx.textBaseline = "middle";
  for (const ind of lcdIndicators(L.indicators, display)) {
    const x = ind.x * width;
    const size = Math.max(1, Math.floor(fontSize * (ind.scale || 1)));
    ctx.font = `500 ${size}px ${MONO_FONT}`;
    const cy = rowTop + rowHeight / 2;
    if (ind.style === "inverted") {
      // A filled rounded box in ink, the text in the background colour.
      const w = ctx.measureText(ind.text).width;
      const h = size;
      ctx.fillStyle = ink;
      ctx.beginPath();
      if (ctx.roundRect) ctx.roundRect(x - 2, cy - h / 2 - 2, w + 4, h + 4, 2);
      else ctx.rect(x - 2, cy - h / 2 - 2, w + 4, h + 4);
      ctx.fill();
      ctx.fillStyle = background;
    } else {
      ctx.fillStyle = ink;
    }
    ctx.fillText(ind.text, x, cy);
  }
  paintDotMatrix(ctx, display, L.p16MatrixLeft * width, L.p16MatrixTop * height,
    (L.p16MatrixRight - L.p16MatrixLeft) * width, (L.p16MatrixBottom - L.p16MatrixTop) * height, ink);
}

// The LCD of the model whose faceplate map's `lcd` is `L`; `display` the
// `display` topic's value (null: a blank panel).
function paintLcd(ctx, L, display, width, height) {
  if (!L) return;
  if (L.pc1600) paintLcdPC1600(ctx, display, width, height, L);
  else paintLcdPC1500(ctx, display, width, height, L);
}
// A PNG Blob of a canvas.
function canvasPng(canvas) {
  return new Promise((resolve) => canvas.toBlob(resolve, "image/png"));
}

// Puts `items` ({mime: Promise of a Blob or string}) on the clipboard.
// Called straight from the click (Safari allows a write only inside the
// gesture), with the data still on its way from the worker -- ClipboardItem
// takes promises. Resolves false if the browser refuses, or an item's
// promise rejects (its data did not come), with an app log line that names
// `what` and the browser's error, or the reason the data did not come.
async function writeClipboard(what, items) {
  const asBlob = (mime) => Promise.resolve(items[mime]).then((v) => (typeof v === "string" ? new Blob([v], { type: mime }) : v));
  try {
    if (navigator.clipboard && window.ClipboardItem) {
      const entries = {};
      for (const mime of Object.keys(items)) {
        entries[mime] = asBlob(mime);
        entries[mime].catch(() => {});   // told once, by the write below
      }
      await navigator.clipboard.write([new ClipboardItem(entries)]);
      return true;
    }
    if (navigator.clipboard && items["text/plain"]) {
      await navigator.clipboard.writeText(await items["text/plain"]);
      return true;
    }
  } catch (e) {
    // Data that did not come is the cause, whatever the browser says then.
    const missing = (await Promise.allSettled(Object.values(items).map((v) => Promise.resolve(v))))
      .find((r) => r.status === "rejected");
    calcuAppLog("warn", missing ? `${what}: ${(missing.reason && missing.reason.message) || missing.reason}`
      : `${what}: not copied: ${calcuErrorText(e)}`);
  }
  return false;
}

// Copy Screen: `items` on the clipboard, else (the
// browser refuses) its "image/png" downloaded as `fileName`; nothing when
// the picture itself did not come.
async function copyImageOrDownload(what, fileName, items) {
  if (await writeClipboard(what, items)) return;
  const png = await Promise.resolve(items["image/png"]).catch(() => null);
  if (png) {
    calcuAppLog("warn", `${what}: offered as the download ${fileName}.`);
    downloadBytes(fileName, png, "image/png");
  }
}

// A file or blob for the user to keep (the library's download).
function downloadBytes(name, bytes, type) { calcuDownload(name, bytes, type); }

// A picked/dropped File's bytes.
function fileBytes(file) {
  return file.arrayBuffer().then((b) => new Uint8Array(b));
}
