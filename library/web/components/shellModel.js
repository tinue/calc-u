// The shell's models in plain JS (LIB-14 design §2.3, §3.3 to §3.7, D-2):
// the same rules as library/shell/ (MenuModel, BarModel, SettingsModel,
// LoadingRule, NoticeQueue, the hit rule, PointerTracker), rule by rule, on
// the same Value forms of the topics. No React, no DOM: loadable by
// `node --test` (library/web/tests/shell_model.test.js, whose cases carry the
// C++ cases' names). A classic script of the component set's closure;
// every top-level name starts with `calcuShell` or `CALCU_SHELL` (D-17).
//
//   calcuShellMenus(inputs, product, traits) → [{id, label, items}]
//   calcuShellBar(inputs, product) → [{group, controls}]
//   calcuShellPanels(inputs) → [{kind, owner, ownerPlace}]
//   calcuShellFamilySections(rows, keysShown) → spec; calcuShellDialog(settings, presetList, spec)
//   calcuShellLoading() → {busy(runs, full, now), shown(now), due()}
//   calcuShellNotices() → {push, refusal, current, close, answer, waiting}
//   calcuShellKeyAt(map, scale, x, y); calcuShellPointer(map, scale) → {down, move, up, releaseAll}
//   calcuShellAbout(product)
//
// inputs: {controlState, busy, presetList, runState, romsMissing, settings, media}, each the
// topic's value or null. product: the JS form of ShellProduct (§3.2): {name, version, build,
// helpUrl, aboutLabel, aboutLines, aboutLinks: [[label, url]], variantLabels: {id: label},
// copyScreen: fn | null, pasteText, moduleSlotPinOuts: [...], placesWithoutControl: [...],
// items: [{group, id, label, entry, act}]}; groups as in C++: "FileOpen", "FileKinds",
// "FileMedia", "FileEnd", "Edit", "MachineEnd", "Help".

const CALCU_SHELL_ACTIONS = {
  loadPreset: "load-preset", savedMedia: "saved-media", settings: "settings", quit: "quit",
  copyScreen: "copy-screen", pasteText: "paste-text", help: "help", about: "about",
};
const CALCU_SHELL_BAR_GROUPS = ["AfterModel", "Slots", "Places", "Media", "End"];
const CALCU_SHELL_LOADING_MS = 500;
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
function calcuShellItem(id, label, extra) {
  return Object.assign({ kind: "action", id, label, checkable: false, checked: false, enabled: true, reason: "",
    role: "none", shortcut: "none", command: "", args: null, children: [] }, extra || {});
}
function calcuShellApply(item, a) {
  item.enabled = a.now;
  item.reason = a.now ? "" : a.reason;
  return item;
}
function calcuShellSeparator() { return calcuShellItem("", "", { kind: "separator" }); }
function calcuShellSubmenu(id, label, children) { return calcuShellItem(id, label, { kind: "submenu", children }); }
function calcuShellAppendGroup(into, group) {   // M1
  if (!group.length) return;
  if (into.length) into.push(calcuShellSeparator());
  into.push(...group);
}
function calcuShellActionEntry(cs, command) {
  return calcuShellList(cs, "actions").find((a) => a.command === command) || null;
}
function calcuShellStarted(inputs) { return !!(inputs && inputs.controlState); }

function calcuShellProductItems(inputs, product, group) {
  if (!calcuShellStarted(inputs) && group !== "Help") return [];   // M10
  const entries = (inputs.controlState && inputs.controlState.product) || {};
  return (product.items || []).filter((i) => i.group === group).map((i) => {
    const item = calcuShellItem(i.id, i.label);
    if (i.entry) calcuShellApply(item, entries[i.entry] ? calcuShellAvailability(entries[i.entry]) : { now: false, reason: "" });
    return item;
  });
}

function calcuShellModelItems(cs) {
  const running = calcuShellText(cs, "model");
  return calcuShellList(cs, "models").map((m) => {
    const id = calcuShellText(m, "id");
    const item = calcuShellItem(`model-${id}`, id, { checkable: true, checked: id === running });
    if (!item.checked) { item.command = "model"; item.args = { name: id }; }
    return calcuShellApply(item, calcuShellAvailability(m));
  });
}

function calcuShellPlaceKind(place, product) {   // D-23, B3
  if ((product.moduleSlotPinOuts || []).includes(calcuShellText(place, "pinOut"))) return "slot";
  if ((product.placesWithoutControl || []).includes(calcuShellText(place, "place"))) return "none";
  const n = calcuShellList(place, "fitting").length;
  if (n === 0) return "none";
  return n === 1 ? "button" : "picker";
}

function calcuShellPlaceChoices(place, withNone) {
  const where = calcuShellText(place, "place");
  const fitting = calcuShellList(place, "fitting");
  const anyPlugged = fitting.some((f) => f.plugged === true);
  const out = [];
  if (withNone) {   // D-11
    const none = calcuShellItem(`peripheral-${where}-none`, "None", { checkable: true, checked: !anyPlugged });
    if (anyPlugged) { none.command = "detach"; none.args = { place: where }; }
    out.push(calcuShellApply(none, calcuShellAvailability(place, "empty")));
  }
  for (const f of fitting) {
    const id = calcuShellText(f, "id");
    const item = calcuShellItem(`peripheral-${where}-${id}`, calcuShellText(f, "name"), { checkable: true, checked: f.plugged === true });
    if (item.checked && withNone) { /* a picker's plugged choice: what runs */ }
    else if (item.checked) { item.command = "detach"; item.args = { place: where }; }
    else { item.command = "attach"; item.args = { place: where, peripheral: id }; }
    out.push(calcuShellApply(item, calcuShellAvailability(f)));
  }
  return out;
}

function calcuShellVariantItems(variants, running, owner, command) {
  return variants.map((v) => {
    const id = calcuShellText(v, "id");
    const item = calcuShellItem(`variant-${owner}-${id}`, id, { checkable: true, checked: id === running });
    if (!item.checked) command(item, id);
    return calcuShellApply(item, calcuShellAvailability(v));
  });
}
function calcuShellVariantLabel(product, id) {
  return (product.variantLabels && product.variantLabels[id]) || "Variant";
}

function calcuShellFileMenu(inputs, product, traits) {
  const items = [];
  const open = [];
  const lp = calcuShellStarted(inputs) ? calcuShellActionEntry(inputs.controlState, "load-preset") : null;
  if (lp) {   // M2
    const a = calcuShellAvailability(lp);
    open.push(calcuShellApply(calcuShellItem("load-preset", "Load Preset…", { shortcut: traits.shortcuts ? "load-preset" : "none" }), a));
    const groups = [];
    for (const e of calcuShellList(inputs.presetList, "entries")) {
      if (e.kind !== "example") continue;
      const group = calcuShellText(e, "group");
      const ref = calcuShellText(e, "bundlePath");
      const item = calcuShellApply(calcuShellItem(`example-${ref}`, calcuShellText(e, "name"), { command: "load-preset", args: { ref } }), a);
      let g = groups.find((x) => x.group === group);
      if (!g) { g = { group, list: [] }; groups.push(g); }
      g.list.push(item);
    }
    if (groups.length) {
      const sub = [];
      for (const g of groups) {
        if (!g.group) sub.push(...g.list);
        else sub.push(calcuShellSubmenu(`examples-${g.group}`, g.group, g.list));
      }
      open.push(calcuShellSubmenu("examples", "Examples", sub));
    }
  }
  open.push(...calcuShellProductItems(inputs, product, "FileOpen"));
  calcuShellAppendGroup(items, open);
  calcuShellAppendGroup(items, calcuShellProductItems(inputs, product, "FileKinds"));
  const media = [];
  if (traits.savedMedia && calcuShellList(inputs.media, "kinds").length) {   // M2; LIB-7 AC-29
    media.push(calcuShellItem("import-media", "Import Media…", { command: "media.import" }));
    media.push(calcuShellItem("saved-media", "Saved Media…"));
  }
  media.push(...calcuShellProductItems(inputs, product, "FileMedia"));
  calcuShellAppendGroup(items, media);
  const end = calcuShellProductItems(inputs, product, "FileEnd");
  end.push(calcuShellItem("settings", "Settings…", { role: "preferences", shortcut: traits.shortcuts ? "preferences" : "none" }));
  if (traits.hasQuit) end.push(calcuShellItem("quit", "Quit", { role: "quit", shortcut: traits.shortcuts ? "quit" : "none" }));
  calcuShellAppendGroup(items, end);
  return items;
}

function calcuShellEditMenu(inputs, product, traits) {   // M3
  if (!calcuShellStarted(inputs)) return [];
  const items = [];
  if (product.copyScreen) items.push(calcuShellItem("copy-screen", "Copy Screen", { shortcut: traits.shortcuts ? "copy" : "none" }));
  if (product.pasteText) items.push(calcuShellItem("paste-text", "Paste Text", { shortcut: traits.shortcuts ? "paste" : "none" }));
  items.push(...calcuShellProductItems(inputs, product, "Edit"));
  return items;
}

function calcuShellMachineMenu(inputs, product, traits) {
  const cs = inputs.controlState;
  const items = [];
  const choose = [];
  const models = calcuShellModelItems(cs);   // M4
  if (models.length) choose.push(calcuShellSubmenu("model", "Model", models));
  const model = calcuShellText(cs, "model");
  const variants = calcuShellList(cs, "variants");   // M5
  if (variants.length > 1) {
    choose.push(calcuShellSubmenu("variants-model", calcuShellVariantLabel(product, model),
      calcuShellVariantItems(variants, cs.variant, "model", (i, id) => { i.command = "variant"; i.args = { name: id }; })));
  }
  for (const place of calcuShellList(cs, "places")) {
    const where = calcuShellText(place, "place");
    for (const f of calcuShellList(place, "fitting")) {
      if (f.plugged !== true) continue;
      const vs = calcuShellList(f, "variants");
      if (vs.length < 2) continue;
      const id = calcuShellText(f, "id");
      const owner = `${where}-${id}`;
      choose.push(calcuShellSubmenu(`variants-${owner}`, calcuShellVariantLabel(product, id),
        calcuShellVariantItems(vs, f.variant, owner, (i, v) => { i.command = "attach"; i.args = { place: where, peripheral: id, variant: v }; })));
    }
  }
  const peripherals = [];   // M6, D-10
  for (const place of calcuShellList(cs, "places")) {
    const k = calcuShellPlaceKind(place, product);
    if (k === "slot" || k === "none") continue;
    const list = calcuShellPlaceChoices(place, false);
    if (!list.length) continue;
    if (peripherals.length) peripherals.push(calcuShellSeparator());
    peripherals.push(...list);
  }
  if (peripherals.length) choose.push(calcuShellSubmenu("peripherals", "Peripherals", peripherals));
  calcuShellAppendGroup(items, choose);
  const run = [];   // M7
  const commandItem = (command, label, shortcut) => {
    const e = calcuShellActionEntry(cs, command);
    if (!e) return;
    run.push(calcuShellApply(calcuShellItem(command, label, { command, shortcut: traits.shortcuts ? shortcut : "none" }), calcuShellAvailability(e)));
  };
  commandItem("reset", "Reset", "reset");
  commandItem("cold-boot", "Cold boot", "cold-boot");
  const resume = calcuShellItem("resume", "Resume", { command: "resume" });
  if (!(inputs.runState && inputs.runState.paused === true)) calcuShellApply(resume, { now: false, reason: "The machine is not paused." });
  run.push(resume);
  run.push(...calcuShellProductItems(inputs, product, "MachineEnd"));
  calcuShellAppendGroup(items, run);
  return items;
}

function calcuShellHelpMenu(inputs, product) {   // M8
  const items = [];
  if (product.helpUrl) items.push(calcuShellItem("help", `${product.name} Help`));
  items.push(...calcuShellProductItems(inputs, product, "Help"));
  items.push(calcuShellItem("about", product.aboutLabel || `About ${product.name}`, { role: "about" }));
  return items;
}

function calcuShellMenus(inputs, product, traits) {
  const t = Object.assign({ hasQuit: false, shortcuts: false, savedMedia: true }, traits || {});
  const out = [];
  const add = (id, label, items) => { if (items.length) out.push({ id, label, items }); };   // M1
  add("file", "File", calcuShellFileMenu(inputs, product, t));
  add("edit", "Edit", calcuShellEditMenu(inputs, product, t));
  if (calcuShellStarted(inputs)) add("machine", "Machine", calcuShellMachineMenu(inputs, product, t));   // M10
  add("help", "Help", calcuShellHelpMenu(inputs, product));
  return out;
}

function calcuShellFindItem(menus, id) {
  const walk = (items) => {
    for (const i of items) {
      if (i.id === id && i.kind !== "separator") return i;
      const c = walk(i.children || []);
      if (c) return c;
    }
    return null;
  };
  for (const m of menus) {
    const i = walk(m.items);
    if (i) return i;
  }
  return null;
}

function calcuShellBar(inputs, product) {   // B1 to B7
  if (!calcuShellStarted(inputs)) return [];
  const cs = inputs.controlState;
  const groups = CALCU_SHELL_BAR_GROUPS.map((group) => ({ group, controls: [] }));
  groups[0].controls.push({ kind: "model", id: "model", label: "", plugged: false, choices: calcuShellModelItems(cs), entry: null, mediumKind: "" });
  const places = calcuShellList(cs, "places");
  const slotCount = places.filter((p) => calcuShellPlaceKind(p, product) === "slot").length;
  let slot = 0;
  for (const place of places) {
    const id = calcuShellText(place, "place");
    const kind = calcuShellPlaceKind(place, product);
    const c = { kind: "", id, label: "", plugged: false, choices: [], entry: place, mediumKind: "" };
    if (kind === "slot") {
      slot += 1;
      Object.assign(c, { kind: "slot", label: slotCount > 1 ? `${slot}:` : "", choices: calcuShellPlaceChoices(place, true) });
      groups[1].controls.push(c);
    } else if (kind === "button") {
      const f = calcuShellList(place, "fitting")[0];
      Object.assign(c, { kind: "button", label: calcuShellText(f, "name"), plugged: f.plugged === true, choices: calcuShellPlaceChoices(place, false) });
      groups[2].controls.push(c);
    } else if (kind === "picker") {
      Object.assign(c, { kind: "picker", choices: calcuShellPlaceChoices(place, true) });
      groups[2].controls.push(c);
    }
  }
  for (const m of calcuShellList(cs, "mediaPlaces"))
    groups[3].controls.push({ kind: "media", id: calcuShellText(m, "id"), label: "", plugged: false, choices: [], entry: m, mediumKind: calcuShellText(m, "kind") });
  return groups;
}

function calcuShellPanels(inputs) {   // B6
  return calcuShellList(inputs && inputs.controlState, "panels").map((p) => ({
    kind: calcuShellText(p, "kind"),
    owner: p.owner ? calcuShellText(p.owner, "peripheral") : "",
    ownerPlace: p.owner ? calcuShellText(p.owner, "place") : "",
  }));
}

// S1: the family sections for the keys the `settings` topic has (keysShown:
// "section.key"; empty: all); the product appends its own.
function calcuShellFamilySections(rows, keysShown) {
  const wanted = (r) => !keysShown || !keysShown.length || keysShown.includes(`${r.section}.${r.key}`);
  const cap = (s) => (s.charAt(0).toUpperCase() + s.slice(1)).replace(/-/g, " ");
  const general = [], presets = [], storage = [], folders = [], tracing = [];
  for (const r of rows || []) {
    if (!wanted(r)) continue;
    const row = (widget, label) => ({ widget, section: r.section, key: r.key, label });
    if (r.section === "startup" && r.key === "model") general.push(row("startup-model", "Startup model"));
    else if (r.section === "preset") presets.push(row("startup-preset", r.key));
    else if (r.section === "storage" && r.key === "save-folder") storage.push(row("folder", "Save folder"));
    else if (r.section === "storage" && r.key === "tape-folder") storage.push(row("folder", "Tapes"));
    else if (r.section === "folders") folders.push(row("folder", `${cap(r.key)} folder`));
    else if (r.section === "trace" && r.key === "folder") tracing.push(row("folder", "Trace folder"));
    else if (r.section === "trace" && r.key === "max-file-size-mb") tracing.push(row("integer", "Maximum file size (MB)"));
  }
  return [["General", general], ["Startup presets", presets], ["Storage", storage], ["Folders", folders], ["Tracing", tracing]]
    .filter(([, list]) => list.length).map(([title, list]) => ({ title, rows: list }));
}

function calcuShellWithTilde(path, home) {
  if (!home || !path.startsWith(home)) return path;
  if (path.length === home.length) return "~";
  const c = path.charAt(home.length);
  return c === "/" || c === "\\" ? `~${path.slice(home.length)}` : path;
}

function calcuShellDialog(settings, presetList, spec) {   // S1 to S5
  const home = calcuShellText(settings, "home");
  const rows = calcuShellList(settings, "rows");
  const out = [];
  for (const section of spec || []) {
    const shown = [];
    for (const row of section.rows) {
      const r = row.widget === "note" ? null : rows.find((x) => x.section === row.section && x.key === row.key);
      if (!r && row.widget !== "note") continue;
      const s = { row, value: r ? calcuShellText(r, "value") : "", shown: "", choices: [], resettable: r ? r.isDefault === false : false,
        min: r && typeof r.min === "number" ? r.min : null, max: r && typeof r.max === "number" ? r.max : null };
      s.shown = s.value;
      if (row.widget === "startup-model") {
        s.choices = calcuShellList(r, "choices").map((c) => [c, c === "last" ? "Last used" : c]);
      } else if (row.widget === "startup-preset") {
        s.choices.push(["", "None"]);
        let listed = !s.value;
        for (const e of calcuShellList(presetList, "entries")) {
          if (e.kind !== "startup" || e.model !== row.key) continue;
          const name = calcuShellText(e, "name");
          const description = calcuShellText(e, "description");
          s.choices.push([e.bundlePath, description ? `${name} — ${description}` : name]);
          listed = listed || e.bundlePath === s.value;
        }
        if (!listed) s.choices.push([s.value, calcuShellWithTilde(s.value, home)]);
      } else if (row.widget === "folder") {
        s.shown = calcuShellWithTilde(calcuShellText(r, "inUse"), home);
      } else if (row.widget === "choice") {
        s.choices = calcuShellList(r, "choices").map((c) => [c, c]);
      }
      shown.push(s);
    }
    if (shown.length) out.push({ title: section.title, rows: shown });
  }
  return out;
}

function calcuShellLoading() {   // L1
  let since = null;
  return {
    busy(operationRuns, full, now) {
      if (operationRuns && full) { if (since === null) since = now; } else since = null;
    },
    shown(now) { return since !== null && now - since >= CALCU_SHELL_LOADING_MS; },
    due() { return since === null ? null : since + CALCU_SHELL_LOADING_MS; },
  };
}

function calcuShellNotices() {   // N1 to N4, D-7
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

// H1 to H3, D-8: a hit map {keys: [[key, {x, y, w, h}]], display: {x, y, w, h} | null, distance}.
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

// H4, H5: {down, move, up, releaseAll}, each giving [{name, args}].
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
      held.set(pointer, h);   // EC-8: no key, nothing held
      return out;
    },
    move(pointer, px, py) {   // D-9
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

function calcuShellAbout(product) {   // AC-21
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
    calcuShellMenus, calcuShellFindItem, calcuShellBar, calcuShellPanels, calcuShellFamilySections,
    calcuShellDialog, calcuShellWithTilde, calcuShellLoading, calcuShellNotices, calcuShellKeyAt,
    calcuShellPointer, calcuShellAbout, CALCU_SHELL_LOADING_MS,
  };
}
