// The shell's models in plain JS:
// the same rules as library/shell/ (MenuModel, BarModel, SettingsModel,
// LoadingRule, NoticeQueue, the hit rule, PointerTracker), rule by rule, on
// the same Value forms of the topics. No React, no DOM: loadable by
// `node --test` (library/web/tests/shell_model.test.js, whose cases carry the
// C++ cases' names). A classic script of the component set's closure;
// every top-level name starts with `calcuShell` or `CALCU_SHELL`.
//
//   calcuShellMenus(inputs, product, traits) → [{id, label, items}]
//   calcuShellBar(inputs, product, traits) → [{group, controls, pushed}]
//   calcuShellBarFit(faceplate, withoutLabels, withLabels, room?) → {width, labels}
//   calcuShellMoreItems(menus, product) → the items of the More button
//   calcuShellPanels(inputs, product) → [{kind, owner, ownerPlace, places}]
//   calcuShellFamilySections(rows, keysShown) → spec; calcuShellDialog(settings, presetList, spec)
//   calcuShellLoading() → {busy(runs, full, now), shown(now), due()}
//   calcuShellNotices() → {push, refusal, current, close, answer, waiting}
//   calcuShellKeyAt(map, scale, x, y); calcuShellPointer(map, scale) → {down, move, up, releaseAll}
//   calcuShellAbout(product)
//
// inputs: {controlState, busy, presetList, runState, romsMissing, settings, media}, each the
// topic's value or null. product: the JS form of ShellProduct: {name, version, build,
// helpUrl, aboutLabel, aboutLines, aboutLinks: [[label, url]], variantLabels: {id: label},
// barModelLabels: {id: label}, barSpread, barShortPickers,
// copyScreen: fn | null, pasteText, moduleSlotPinOuts: [...], placesWithoutControl: [...], placesOnPaper: [...],
// items: [{group, id, label, entry, act, onBar, mediaPlace}]}; groups as in C++: "FileOpen", "FileKinds",
// "FileMedia", "FileEnd", "Edit", "MachineEnd", "Help".
// traits: {hasQuit, shortcuts, savedMedia, menuBar, settings, about} (menuBar
// false is a window without a menu bar: the menus are still computed, for the
// More button, but not drawn; settings false: no Settings…; about false: no
// About).

const CALCU_SHELL_ACTIONS = {
  loadPreset: "load-preset", savedMedia: "saved-media", settings: "settings", quit: "quit",
  copyScreen: "copy-screen", pasteText: "paste-text", help: "help", about: "about",
};
const CALCU_SHELL_BAR_GROUPS = ["AfterModel", "Slots", "Places", "Media", "End"];
const CALCU_SHELL_COLD_BOOT_QUESTION = "Cold boot the machine? What it holds in memory is lost.";   // asked in the More menu
const CALCU_SHELL_TRAITS = { hasQuit: false, shortcuts: false, savedMedia: true, menuBar: true, settings: true, about: true };
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
function calcuShellAppendGroup(into, group) {   // a separator between groups that have items
  if (!group.length) return;
  if (into.length) into.push(calcuShellSeparator());
  into.push(...group);
}
function calcuShellActionEntry(cs, command) {
  return calcuShellList(cs, "actions").find((a) => a.command === command) || null;
}
function calcuShellStarted(inputs) { return !!(inputs && inputs.controlState); }

function calcuShellProductItems(inputs, product, group) {
  if (!calcuShellStarted(inputs) && group !== "Help") return [];   // no control state: only Help's items
  const entries = (inputs.controlState && inputs.controlState.product) || {};
  const out = [];
  for (const i of (product.items || []).filter((x) => x.group === group)) {
    const item = calcuShellItem(i.id, i.label);
    if (i.mediaPlace) {   // only with its media place, as its `insert`
      const place = calcuShellList(inputs.controlState, "mediaPlaces").find((m) => calcuShellText(m, "id") === i.mediaPlace);
      if (!place) continue;
      calcuShellApply(item, calcuShellAvailability(place, "insert"));
    }
    if (i.entry && item.enabled) calcuShellApply(item, entries[i.entry] ? calcuShellAvailability(entries[i.entry]) : { now: false, reason: "" });
    out.push(item);
  }
  return out;
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

// A module slot gets the slot picker; a place without a control none; else
// a button when exactly one peripheral fits, a picker when several do.
function calcuShellPlaceKind(place, product) {
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
  if (withNone) {   // the empty place reads "None"
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
  if (lp) {
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
  if (traits.savedMedia && calcuShellList(inputs.media, "kinds").length) {
    media.push(calcuShellItem("import-media", "Import Media…", { command: "media.import" }));
    media.push(calcuShellItem("saved-media", "Saved Media…"));
  }
  media.push(...calcuShellProductItems(inputs, product, "FileMedia"));
  calcuShellAppendGroup(items, media);
  const end = calcuShellProductItems(inputs, product, "FileEnd");
  if (traits.settings !== false)
    end.push(calcuShellItem("settings", "Settings…", { role: "preferences", shortcut: traits.shortcuts ? "preferences" : "none" }));
  if (traits.hasQuit) end.push(calcuShellItem("quit", "Quit", { role: "quit", shortcut: traits.shortcuts ? "quit" : "none" }));
  calcuShellAppendGroup(items, end);
  return items;
}

function calcuShellEditMenu(inputs, product, traits) {
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
  const models = calcuShellModelItems(cs);
  if (models.length) choose.push(calcuShellSubmenu("model", "Model", models));
  const model = calcuShellText(cs, "model");
  const variants = calcuShellList(cs, "variants");
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
  const peripherals = [];   // one flat list, place after place
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
  const run = [];
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

function calcuShellHelpMenu(inputs, product, traits) {
  const items = [];
  if (product.helpUrl) items.push(calcuShellItem("help", `${product.name} Help`));
  items.push(...calcuShellProductItems(inputs, product, "Help"));
  if (traits.about !== false) items.push(calcuShellItem("about", product.aboutLabel || `About ${product.name}`, { role: "about" }));
  return items;
}

function calcuShellMenus(inputs, product, traits) {
  const t = Object.assign({}, CALCU_SHELL_TRAITS, traits || {});
  const out = [];
  const add = (id, label, items) => { if (items.length) out.push({ id, label, items }); };   // a menu without items is left out
  add("file", "File", calcuShellFileMenu(inputs, product, t));
  add("edit", "Edit", calcuShellEditMenu(inputs, product, t));
  if (calcuShellStarted(inputs)) add("machine", "Machine", calcuShellMachineMenu(inputs, product, t));
  add("help", "Help", calcuShellHelpMenu(inputs, product, t));
  return out;
}

// The items of the More button, in menu order, without what has a
// control on a bar without a menu bar; one separator between the groups
// that remain, never first, last or twice; Cold boot asks first.
function calcuShellMoreItems(menus, product) {
  const own = ["load-preset", "settings", "quit", "model", "peripherals"];
  const onBar = (i) => own.includes(i.id) || (product.items || []).some((p) => p.id === i.id && p.onBar);
  const out = [];
  let pending = false;
  for (const m of menus) {
    pending = true;
    for (const i of m.items) {
      if (i.kind === "separator") { pending = true; continue; }
      if (onBar(i)) continue;
      if (pending && out.length) out.push(calcuShellSeparator());
      pending = false;
      const copy = Object.assign({}, i);
      if (copy.id === "cold-boot") Object.assign(copy, { confirm: CALCU_SHELL_COLD_BOOT_QUESTION, destructive: true });
      out.push(copy);
    }
  }
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

function calcuShellBar(inputs, product, traits) {
  if (!calcuShellStarted(inputs)) return [];
  const t = Object.assign({}, CALCU_SHELL_TRAITS, traits || {});
  const cs = inputs.controlState;
  const groups = CALCU_SHELL_BAR_GROUPS.map((group) => ({ group, controls: [], pushed: false }));
  // The bar's model names (a choice's label is the model ID); label: the running model's.
  const barNames = product.barModelLabels || {};
  const choices = calcuShellModelItems(cs).map((c) => Object.assign(c, { label: barNames[c.label] || c.label }));
  const running = choices.find((c) => c.checked);
  groups[0].controls.push({ kind: "model", id: "model", label: "", shown: running ? running.label : "",
    shortShown: !!product.barShortPickers, plugged: false, choices, entry: null, mediumKind: "" });
  // A spread bar: the room before the media group and End is taken up.
  groups[3].pushed = groups[4].pushed = !!product.barSpread;
  const places = calcuShellList(cs, "places");
  const slotCount = places.filter((p) => calcuShellPlaceKind(p, product) === "slot").length;
  let slot = 0;
  const onPaper = product.placesOnPaper || [];
  for (const place of places) {
    if (onPaper.includes(calcuShellText(place, "place"))) continue;   // on the paper panel
    const isSlot = calcuShellPlaceKind(place, product) === "slot";
    if (isSlot) slot += 1;
    const c = calcuShellPlaceControl(place, product, isSlot && slotCount > 1 ? `${slot}:` : "");
    if (c) groups[isSlot ? 1 : 2].controls.push(c);
  }
  for (const m of calcuShellList(cs, "mediaPlaces"))
    groups[3].controls.push({ kind: "media", id: calcuShellText(m, "id"), label: "", plugged: false, choices: [], entry: m, mediumKind: calcuShellText(m, "kind") });
  if (t.menuBar !== false) return groups;
  // A window without a menu bar. End holds Preset, Settings and More
  // (with Reset), each following its menu item.
  const menus = calcuShellMenus(inputs, product, t);
  const action = (id, label) => {
    const item = calcuShellFindItem(menus, id);
    return item ? { kind: "action", id, label, plugged: false, choices: [], entry: null, mediumKind: "", item } : null;
  };
  groups[4].controls.push(...[action("load-preset", "Preset"), action("settings", "Settings")].filter(Boolean));
  groups[4].controls.push({ kind: "more", id: "more", label: "More", plugged: false, choices: calcuShellMoreItems(menus, product), entry: null, mediumKind: "" });
  return groups;
}

// The bar below a faceplate (C++ shell::barFit): labels only where the whole
// bar fits the faceplate with them; as wide as the faceplate, wider where the
// controls need more, never wider than the room.
function calcuShellBarFit(faceplate, withoutLabels, withLabels, room = Infinity) {
  const labels = withLabels <= faceplate;
  return { labels, width: Math.min(room, Math.max(faceplate, labels ? withLabels : withoutLabels)) };
}

// A place's control: a module slot's picker (its label "n:" given), a
// button where one peripheral fits, a picker where several do; null for a
// place without a control.
function calcuShellPlaceControl(place, product, slotLabel) {
  const kind = calcuShellPlaceKind(place, product);
  const id = calcuShellText(place, "place");
  const c = { kind: "", id, label: "", shown: "", shortShown: !!product.barShortPickers, plugged: false, choices: [], entry: place, mediumKind: "" };
  // A picker's short text: the chosen entry's ID, "—" for none.
  const shownOf = (choices) => {
    const i = choices.find((x) => x.checked);
    if (!i) return "\u2014";
    const prefix = `peripheral-${id}-`;
    const word = i.id.startsWith(prefix) ? i.id.slice(prefix.length) : i.label;
    return word === "none" ? "\u2014" : word;
  };
  if (kind === "slot") {
    const choices = calcuShellPlaceChoices(place, true);
    return Object.assign(c, { kind: "slot", label: slotLabel, choices, shown: shownOf(choices) });
  }
  if (kind === "button") {
    const f = calcuShellList(place, "fitting")[0];
    return Object.assign(c, { kind: "button", label: calcuShellText(f, "name"), plugged: f.plugged === true, choices: calcuShellPlaceChoices(place, false) });
  }
  if (kind === "picker") {
    const choices = calcuShellPlaceChoices(place, true);
    return Object.assign(c, { kind: "picker", choices, shown: shownOf(choices) });
  }
  return null;
}

// places: on the paper panel, the controls of the product's placesOnPaper.
function calcuShellPanels(inputs, product) {
  const cs = inputs && inputs.controlState;
  const onPaper = (product && product.placesOnPaper) || [];
  return calcuShellList(cs, "panels").map((p) => {
    const kind = calcuShellText(p, "kind");
    const places = kind !== "paper" ? [] : calcuShellList(cs, "places")
      .filter((place) => onPaper.includes(calcuShellText(place, "place")))
      .map((place) => calcuShellPlaceControl(place, product, "")).filter(Boolean);
    return {
      kind,
      owner: p.owner ? calcuShellText(p.owner, "peripheral") : "",
      ownerPlace: p.owner ? calcuShellText(p.owner, "place") : "",
      places,
    };
  });
}

// The family sections for the keys the `settings` topic has (keysShown:
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
    else if (r.section === "storage" && r.key === "cassette-folder") storage.push(row("folder", "Cassettes"));
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

function calcuShellDialog(settings, presetList, spec) {
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
        if (Array.isArray(row.choices) && row.choices.length) {   // exactly these; a value not among them as written
          s.choices = row.choices.map(([v, l]) => [v, l]);
          if (!s.choices.some(([v]) => v === s.value)) s.choices.push([s.value, s.value]);
        } else s.choices = calcuShellList(r, "choices").map((c) => [c, c]);
      }
      shown.push(s);
    }
    if (shown.length) out.push({ title: section.title, rows: shown });
  }
  return out;
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
    calcuShellMenus, calcuShellFindItem, calcuShellBar, calcuShellBarFit, calcuShellPanels, calcuShellFamilySections,
    calcuShellDialog, calcuShellWithTilde, calcuShellLoading, calcuShellNotices, calcuShellKeyAt,
    calcuShellPointer, calcuShellAbout, calcuShellMoreItems, CALCU_SHELL_LOADING_MS, CALCU_SHELL_COLD_BOOT_QUESTION,
  };
}
