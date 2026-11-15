// The page side of paper output (LIB-8 design §2, §3.5, §3.7): the papers of
// a product from its topics, kept whole on the page, and the drawing of
// marks on a canvas. Classic script in the closure of a product's calc.js
// (LIB-14 D-17: every top-level name starts with calcu / Calcu); no React
// here, so that a test could load it alone.
//
//   calcuPaperStore(shell) → the store of one shell (made once per shell):
//     getState() → {papers: [{printer, owner, description}],
//                   paper(owner) → {marks, lines, reach: [from, to], empty, version},
//                   copy: {owner, outcome, reason, seq} | null, version}
//     subscribe(listener) → unsubscribe
//   calcuPaperOf(papers, printer) → the entry of a printer (peripheral ID), or null
//   calcuPaintMarks(ctx, description, marks, scale, top, bottom)
//     draws the marks whose y lies in [top, bottom] (Coord) on a 2D context
//     whose origin is the paper's left edge at y = top; `scale` is context
//     units per Coord. A dot is a disc of its diameter in the grey of its
//     darkness; a stroke a polyline of its width (at least one unit of the
//     context, D-12) with round ends and joins, in its colour.
//
// Coordinates arrive as integers in ten-thousandths of the printer's unit
// (paper/PaperTopics.hpp). A paper's marks are kept as the topic sends them:
// a run of dots as one flat array [x, y, darkness, …], a stroke as
// {colour, path: [x, y, …]}; a message whose first mark continues a stroke
// appends its positions to the last stroke (AC-6).

const CALCU_PAPER_STORES = typeof WeakMap !== "undefined" ? new WeakMap() : null;

function calcuEmptyPaper() {
  return { marks: [], lines: [], reach: [0, 0], empty: true, version: 0 };
}

function calcuPaperAppend(paper, item) {
  const marks = (item && item.marks) || [];
  marks.forEach((m, i) => {
    if (Array.isArray(m)) {
      paper.marks.push({ dots: m.slice() });
      return;
    }
    const last = paper.marks[paper.marks.length - 1];
    if (i === 0 && item.continuesStroke && last && last.path) last.path.push(...m.path);
    else paper.marks.push({ colour: m.colour, path: m.path.slice() });
  });
  for (const l of (item && item.lines) || []) paper.lines.push(l);
  if (item && item.reach) paper.reach = item.reach;
  paper.empty = false;
  paper.version += 1;
}

function calcuPaperStore(shell) {
  if (!shell) return null;
  if (CALCU_PAPER_STORES && CALCU_PAPER_STORES.has(shell)) return CALCU_PAPER_STORES.get(shell);
  const byOwner = {};
  const listeners = new Set();
  let copy = null;
  let copySeq = 0;
  let version = 0;
  let snapshot = null;
  const paperOf = (owner) => {
    if (!byOwner[owner]) byOwner[owner] = calcuEmptyPaper();
    return byOwner[owner];
  };
  const changed = () => {
    version += 1;
    snapshot = null;
    listeners.forEach((l) => l());
  };
  const take = (topic, reset, items) => {
    if (topic === "paper-copies") {
      for (const c of items) copy = { ...c, seq: ++copySeq };
      if (items.length) changed();
      return;
    }
    if (topic.indexOf("paper ") !== 0) return;
    const owner = topic.slice("paper ".length);
    const p = paperOf(owner);
    if (reset) {
      const v = p.version + 1;
      byOwner[owner] = calcuEmptyPaper();
      byOwner[owner].version = v;
    }
    for (const item of items) calcuPaperAppend(byOwner[owner], item);
    changed();
  };
  // What the shell kept before this store was made (the history since the
  // last reset; at most CALCU_STREAM_KEEP items), then every message.
  const kept = shell.getState().streams || {};
  for (const topic of Object.keys(kept)) if (topic.indexOf("paper ") === 0) take(topic, true, kept[topic]);
  if (shell.listenStream) shell.listenStream((m) => take(m.topic, m.reset, m.value || []));
  let lastPapers;
  if (shell.subscribe) shell.subscribe(() => {
    const p = shell.getState().topics.papers;
    if (p !== lastPapers) {
      lastPapers = p;
      changed();
    }
  });
  const store = {
    getState: () => {
      if (!snapshot) {
        snapshot = {
          papers: shell.getState().topics.papers || [],
          paper: (owner) => byOwner[owner] || calcuEmptyPaper(),
          copy,
          version,
        };
      }
      return snapshot;
    },
    subscribe: (l) => { listeners.add(l); return () => listeners.delete(l); },
  };
  if (CALCU_PAPER_STORES) CALCU_PAPER_STORES.set(shell, store);
  return store;
}

function calcuPaperOf(papers, printer) {
  if (!printer) return null;
  return (papers || []).find((p) => p.printer === printer) || null;
}

function calcuPaintMarks(ctx, description, marks, scale, top, bottom) {
  if (!description) return;
  const dots = description.form === "dots";
  const r = (description.dotDiameter || 0) / 2;
  const half = dots ? r : (description.strokeWidth || 0) / 2;
  const lo = top - half, hi = bottom + half;
  const greys = description.darknessGreys || [];
  const grey = (d) => {
    const g = greys.length ? greys[Math.min(d, greys.length - 1)] : 0;
    return `rgb(${g},${g},${g})`;
  };
  const radius = Math.max(r * scale, 0.5);
  for (const m of marks) {
    if (m.dots) {
      const a = m.dots;
      for (let i = 0; i + 2 < a.length; i += 3) {
        const y = a[i + 1];
        if (y < lo || y > hi) continue;
        ctx.fillStyle = grey(a[i + 2]);
        ctx.beginPath();
        ctx.arc(a[i] * scale, (y - top) * scale, radius, 0, Math.PI * 2);
        ctx.fill();
      }
      continue;
    }
    const p = m.path;
    if (!p || p.length < 2) continue;
    let inView = false;
    for (let i = 1; i < p.length && !inView; i += 2) inView = p[i] >= lo && p[i] <= hi;
    if (!inView && p.length > 2) {   // a segment may cross the view with both ends outside
      for (let i = 3; i < p.length && !inView; i += 2) inView = Math.min(p[i - 2], p[i]) <= hi && Math.max(p[i - 2], p[i]) >= lo;
    }
    if (!inView) continue;
    ctx.strokeStyle = m.colour;
    ctx.fillStyle = m.colour;
    ctx.lineWidth = Math.max(1, (description.strokeWidth || 0) * scale);
    ctx.lineCap = "round";
    ctx.lineJoin = "round";
    ctx.beginPath();
    ctx.moveTo(p[0] * scale, (p[1] - top) * scale);
    if (p.length === 2) ctx.lineTo(p[0] * scale + 0.01, (p[1] - top) * scale);
    for (let i = 2; i + 1 < p.length; i += 2) ctx.lineTo(p[i] * scale, (p[i + 1] - top) * scale);
    ctx.stroke();
  }
}
