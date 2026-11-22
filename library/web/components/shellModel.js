// The shell's rules that run on the page's own clock and measurements, in
// plain JS: the same rules as library/shell/ (barFit, view, LoadingRule,
// NoticeQueue, the hit rule, PointerTracker, about), rule by rule. The
// menus, the bar, the panels and the settings dialog are the C++ models',
// which the worker serves (topic `web.shell`, shell::ModelHost). No React,
// no DOM: loadable by `node --test` (library/web/tests/shell_model.test.js,
// whose cases carry the C++ cases' names). A classic script of the component
// set's closure; every top-level name starts with `calcuShell` or `CALCU_SHELL`.
//
//   calcuShellBarFit(faceplate, withoutLabels, withLabels, room?) → {width, labels}
//   calcuShellView(inputs) → {side, panelsFit, panelsAlone, faceplateWidth, faceplateHeight, room}
//   calcuShellLoading() → {busy(runs, full, now), shown(now), due()}
//   calcuShellNotices() → {push, refusal, current, close, answer, waiting}
//   calcuShellKeyAt(map, scale, x, y); calcuShellPointer(map, scale) → {down, move, up, releaseAll}
//   calcuShellAbout(product)
//   calcuShellList, calcuShellText, calcuShellAvailability: reading the topics' Values

const CALCU_SHELL_ACTIONS = {
  loadPreset: "load-preset", savedMedia: "saved-media", settings: "settings", quit: "quit",
  copyScreen: "copy-screen", pasteText: "paste-text", help: "help", about: "about",
};
const CALCU_SHELL_LOADING_MS = 500;
// The notice queue's question rule, as shell::kAsksPrefix (Notices.cpp): "asks:"
// and the spaces after it dropped. Every sender writes CALCU_ASKS ("asks: ",
// MediaPicker.jsx; C++ media::kAsks), which this reads the same.
const CALCU_SHELL_ASKS = "asks:";
const CALCU_SHELL_STOPPED = "Stopped.";

function calcuShellList(v, key) {
  const x = v && v[key];
  return Array.isArray(x) ? x : [];
}
function calcuShellText(v, key) {
  const x = v && v[key];
  return typeof x === "string" ? x : "";
}
function calcuShellAvailability(entry, key) {
  let holder = entry;
  if (key && key !== "available") holder = entry ? entry[key] : null;
  if (!holder) return { now: true, reason: "" };
  const now = holder.available !== false;
  return { now, reason: now ? "" : calcuShellText(holder, "reason") };
}
function calcuShellBarFit(faceplate, withoutLabels, withLabels, room = Infinity) {
  const labels = withLabels <= faceplate;
  return { labels, width: Math.min(room, Math.max(faceplate, labels ? withLabels : withoutLabels)) };
}

// The view rule (C++ shell::view): the faceplate as large as the window
// allows, its proportions kept; the room left over lies "beside" it (the
// window relatively wider) or "below" it, and picks the view. The panels go
// there only when that room is at least their minimum on that side (0: no
// panels there); else the product's fallback. On mobile with the room below,
// panelsAloneOnMobileBelow gives the panels alone.
// inputs: {width, height, faceplateAspect, fixedWidth, fixedHeight,
// panelsMinWidth, panelsMinHeight, mobile, panelsAloneOnMobileBelow}.
function calcuShellView(inputs) {
  const i = inputs;
  const boxWidth = Math.max(0, i.width - (i.fixedWidth || 0));
  const boxHeight = Math.max(0, i.height - (i.fixedHeight || 0));
  const aspect = i.faceplateAspect > 0 ? i.faceplateAspect : 1;
  const faceplateWidth = Math.min(boxWidth, boxHeight * aspect);
  const faceplateHeight = faceplateWidth / aspect;
  const side = boxWidth > boxHeight * aspect ? "beside" : "below";
  const room = side === "beside" ? boxWidth - faceplateWidth : boxHeight - faceplateHeight;
  const least = (side === "beside" ? i.panelsMinWidth : i.panelsMinHeight) || 0;
  return {
    side, panelsFit: least > 0 && room >= least,
    // Only where the room below takes the panels: a sliver of it (a phone in
    // landscape whose safe area narrows the window) keeps the calculator.
    panelsAlone: !!i.panelsAloneOnMobileBelow && !!i.mobile && side === "below" && least > 0 && room >= least,
    faceplateWidth, faceplateHeight, room,
  };
}

// "Loading…": shown when an operation has run at full speed for 500 ms
// without a break; the next full-speed stretch counts from its start.
function calcuShellLoading() {
  let since = null;
  return {
    busy(operationRuns, full, now) {
      if (operationRuns && full) { if (since === null) since = now; } else since = null;
    },
    shown(now) { return since !== null && now - since >= CALCU_SHELL_LOADING_MS; },
    due() { return since === null ? null : since + CALCU_SHELL_LOADING_MS; },
  };
}

// The notices one after the other, none lost; a refusal whose reason starts
// with "asks:" is a question (Yes sends the command again confirmed).
function calcuShellNotices() {
  const queue = [];
  let last = "";
  const push = (sentence) => { last = sentence; queue.push({ text: sentence, question: false, command: null }); };
  return {
    push,
    refusal(reason, sent) {
      const r = reason || "";
      if (r.startsWith(CALCU_SHELL_ASKS)) {
        queue.push({ text: r.slice(CALCU_SHELL_ASKS.length).replace(/^ +/, ""), question: true, command: sent || null });
        return;
      }
      if (!r || r === last || r === CALCU_SHELL_STOPPED) return;
      push(r);
    },
    current() { return queue.length ? queue[0] : null; },
    close() { queue.shift(); },
    answer(yes) {
      const s = queue.shift();
      if (!s || !yes || !s.question || !s.command) return null;
      return { name: s.command.name, args: Object.assign({}, s.command.args || {}, { confirmed: true }) };
    },
    waiting() { return queue.length; },
  };
}

// The hit rule: inside an area (edges included) the first such key; else the
// nearest key within `distance`; else none. A hit map {keys: [[key, {x, y, w,
// h}]], display: {x, y, w, h} | null, distance}, in its own units times scale.
function calcuShellKeyAt(map, scale, px, py) {
  const scaled = (r) => ({ x: r.x * scale, y: r.y * scale, w: r.w * scale, h: r.h * scale });
  const inside = (r) => px >= r.x && px <= r.x + r.w && py >= r.y && py <= r.y + r.h;
  for (const [key, r] of map.keys) if (inside(scaled(r))) return key;
  let best = null;
  let bestDistance = 0;
  for (const [key, r0] of map.keys) {
    const r = scaled(r0);
    const dx = Math.max(r.x - px, 0, px - (r.x + r.w));
    const dy = Math.max(r.y - py, 0, py - (r.y + r.h));
    const d = Math.sqrt(dx * dx + dy * dy);
    if (d <= (map.distance || 0) && (best === null || d < bestDistance)) { best = key; bestDistance = d; }
  }
  return best;
}

function calcuShellOnDisplay(map, scale, px, py) {
  const d = map.display;
  return !!d && px >= d.x * scale && px <= (d.x + d.w) * scale && py >= d.y * scale && py <= (d.y + d.h) * scale;
}

// The pointers: {down, move, up, releaseAll}, each giving [{name, args}].
// A key is fixed at down; down on the display holds full speed until the
// last pointer on it goes up or leaves.
function calcuShellPointer(map, scale) {
  const held = new Map();
  const s = () => (typeof scale === "function" ? scale() : scale || 1);
  const onDisplayCount = () => [...held.values()].filter((h) => h.display).length;
  const endDisplay = (h, out) => {
    if (!h.display) return;
    h.display = false;
    if (onDisplayCount() === 0) out.push({ name: "speed", args: { full: false } });
  };
  const up = (pointer) => {
    const out = [];
    const h = held.get(pointer);
    if (!h) return out;
    if (h.key !== null) out.push({ name: "screen-key-up", args: { key: h.key, pointer } });
    endDisplay(h, out);
    held.delete(pointer);
    return out;
  };
  return {
    down(pointer, px, py) {
      const out = held.has(pointer) ? up(pointer) : [];
      const h = { key: null, display: false };
      if (calcuShellOnDisplay(map, s(), px, py)) {
        if (onDisplayCount() === 0) out.push({ name: "speed", args: { full: true } });
        h.display = true;
      } else {
        const k = calcuShellKeyAt(map, s(), px, py);
        if (k !== null) { h.key = k; out.push({ name: "screen-key-down", args: { key: k, pointer } }); }
      }
      held.set(pointer, h);   // no key: nothing held, also when it moves onto one
      return out;
    },
    move(pointer, px, py) {   // a move changes no key
      const out = [];
      const h = held.get(pointer);
      if (h && h.display && !calcuShellOnDisplay(map, s(), px, py)) endDisplay(h, out);
      return out;
    },
    up,
    releaseAll() {
      const out = [];
      for (const id of [...held.keys()].sort((a, b) => a - b)) out.push(...up(id));
      return out;
    },
  };
}

function calcuShellAbout(product) {
  let title = product.aboutLabel || `About ${product.name}`;
  if (title.endsWith("…")) title = title.slice(0, -1);
  return {
    title, name: product.name,
    version: `Version ${product.version}${product.build ? ` (${product.build})` : ""}`,
    lines: product.aboutLines || [], links: product.aboutLinks || [],
  };
}

if (typeof module !== "undefined" && module.exports) {
  module.exports = {
    calcuShellBarFit, calcuShellView, calcuShellLoading, calcuShellNotices, calcuShellKeyAt,
    calcuShellPointer, calcuShellAbout, calcuShellAvailability, CALCU_SHELL_LOADING_MS,
  };
}
