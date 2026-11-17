// LIB-14 L1 to L6 (design §6): library/web/components/shellModel.js under
// `node --test` (CTest LibraryWebNodeTests, Frequent), the mirror of the C++
// model tests in library/shell/tests/ case by case, under the same names.
// The inputs are those the C++ tests build from LIB-4's and LIB-6's payloads
// (library/shell/tests/inputs.txt), and the hit cases are the C++ ones
// (library/shell/tests/hit_cases.txt).
const test = require("node:test");
const assert = require("node:assert/strict");
const fs = require("fs");
const path = require("path");
const m = require("../components/shellModel.js");

const FIXTURES = path.join(__dirname, "..", "..", "shell", "tests");
const INPUTS = Object.fromEntries(fs.readFileSync(path.join(FIXTURES, "inputs.txt"), "utf8").split("\n")
  .filter((l) => l && !l.startsWith("#")).map((l) => [l.slice(0, l.indexOf(" ")), JSON.parse(l.slice(l.indexOf(" ") + 1))]));
const clone = (v) => JSON.parse(JSON.stringify(v));
const QT = { hasQuit: true, shortcuts: true, savedMedia: false };
const WEB = { hasQuit: false, shortcuts: false, savedMedia: true };

// The JS form of library/shell/tests/TestShellPart.hpp's product().
function product() {
  return {
    name: "Test-Product", version: "1.2.3", build: "abc123", helpUrl: "https://calc-u.org/test/",
    aboutLabel: "", aboutLines: ["A product made for the tests.", "No code of a product."],
    aboutLinks: [["Source", "https://example.org/source"], ["Privacy", "https://example.org/privacy"]],
    variantLabels: { "Model-A": "Revision", "test-plotter": "Pen" }, presetFilter: ".test",
    copyScreen: null, pasteText: false, moduleSlotPinOuts: ["module"], placesWithoutControl: ["host-drive"],
    items: [
      { group: "FileKinds", id: "load-program", label: "Load Program…", entry: "" },
      { group: "FileMedia", id: "mount-drive", label: "Mount Directory…", entry: "" },
      { group: "MachineEnd", id: "disk-thing", label: "Disk Thing", entry: "tp.disk" },
    ],
  };
}
function inputs(cs) {
  return { controlState: cs === undefined ? clone(INPUTS.modelA) : cs, busy: null, presetList: clone(INPUTS.presetList),
    runState: null, romsMissing: null, settings: null, media: null };
}
const ids = (items) => items.map((i) => (i.kind === "separator" ? "-" : i.id)).join(" ");
const menu = (ms, id) => ms.find((x) => x.id === id);
const find = m.calcuShellFindItem;

// L1: the menus.
test("ac1_menus_and_items_in_order", () => {
  const p = product();
  p.copyScreen = () => {};
  p.pasteText = true;
  const ms = m.calcuShellMenus(inputs(), p, QT);
  assert.equal(ms.map((x) => x.id).join(" "), "file edit machine help");
  assert.equal(ids(menu(ms, "file").items), "load-preset examples - load-program - mount-drive - settings quit");
  assert.equal(ids(menu(ms, "edit").items), "copy-screen paste-text");
  assert.equal(ids(menu(ms, "machine").items), "model variants-model peripherals - reset cold-boot resume disk-thing");
  assert.equal(ids(menu(ms, "help").items), "help about");
  assert.equal(find(ms, "load-preset").label, "Load Preset…");
  assert.equal(find(ms, "cold-boot").label, "Cold boot");
  assert.equal(find(ms, "help").label, "Test-Product Help");
  assert.equal(ids(find(ms, "examples").children), "examples-Games examples-Math");
  const one = find(ms, "example-examples:games/one.test");
  assert.equal(one.command, "load-preset");
  assert.deepEqual(one.args, { ref: "examples:games/one.test" });
});

test("ac1_item_without_function_and_empty_menu_left_out", () => {
  const i = inputs(clone(INPUTS.noColdBoot));
  i.presetList = null;
  const p = product();
  p.items = [];
  const ms = m.calcuShellMenus(i, p, QT);
  assert.equal(ms.map((x) => x.id).join(" "), "file machine help");
  assert.equal(find(ms, "examples"), null);
  assert.equal(find(ms, "cold-boot"), null);
  assert.ok(find(ms, "reset"));
  assert.equal(ids(menu(ms, "file").items), "load-preset - settings quit");
  p.helpUrl = "";
  assert.equal(ids(menu(m.calcuShellMenus(i, p, QT), "help").items), "about");
});

test("ac2_model_menu_marks_running_and_sends_model", () => {
  const ms = m.calcuShellMenus(inputs(), product(), QT);
  assert.equal(ids(find(ms, "model").children), "model-Model-A model-Model-B");
  const a = find(ms, "model-Model-A");
  const b = find(ms, "model-Model-B");
  assert.ok(a.checkable && a.checked);
  assert.equal(a.command, "");
  assert.equal(b.command, "model");
  assert.deepEqual(b.args, { name: "Model-B" });
});

test("ac2_variant_submenus_for_model_and_peripheral", () => {
  const ms = m.calcuShellMenus(inputs(clone(INPUTS.auxPlotter)), product(), QT);
  const vm = find(ms, "variants-model");
  assert.equal(vm.label, "Revision");
  assert.equal(ids(vm.children), "variant-model-v1 variant-model-v2");
  assert.ok(vm.children[0].checked);
  assert.equal(vm.children[1].command, "variant");
  assert.deepEqual(vm.children[1].args, { name: "v2" });
  const vp = find(ms, "variants-aux-test-plotter");
  assert.equal(vp.label, "Pen");
  assert.ok(vp.children[0].checked);
  assert.deepEqual(vp.children[1].args, { place: "aux", peripheral: "test-plotter", variant: "pen-2" });
  assert.equal(find(m.calcuShellMenus(inputs(), product(), QT), "variants-aux-test-plotter"), null);
  assert.equal(find(m.calcuShellMenus(inputs(clone(INPUTS.modelB)), product(), QT), "variants-model"), null);
});

test("ac3_peripherals_attach_replace_and_detach", () => {
  const ms = m.calcuShellMenus(inputs(clone(INPUTS.auxPlotter)), product(), QT);
  assert.equal(ids(find(ms, "peripherals").children),
    "peripheral-printer-test-dotter - peripheral-aux-test-plotter peripheral-aux-test-dotter-2");
  const td20 = find(ms, "peripheral-printer-test-dotter");
  assert.equal(td20.command, "attach");
  assert.deepEqual(td20.args, { place: "printer", peripheral: "test-dotter" });
  const tp4 = find(ms, "peripheral-aux-test-plotter");
  assert.ok(tp4.checked);
  assert.equal(tp4.command, "detach");
  assert.deepEqual(tp4.args, { place: "aux" });
  assert.deepEqual(find(ms, "peripheral-aux-test-dotter-2").args, { place: "aux", peripheral: "test-dotter-2" });
});

test("ac4_shortcuts_qt_and_none_on_web", () => {
  const p = product();
  p.copyScreen = () => {};
  p.pasteText = true;
  const qt = m.calcuShellMenus(inputs(), p, QT);
  assert.equal(find(qt, "load-preset").shortcut, "load-preset");
  assert.equal(find(qt, "reset").shortcut, "reset");
  assert.equal(find(qt, "cold-boot").shortcut, "cold-boot");
  assert.equal(find(qt, "settings").shortcut, "preferences");
  assert.equal(find(qt, "quit").shortcut, "quit");
  assert.equal(find(qt, "copy-screen").shortcut, "copy");
  assert.equal(find(qt, "paste-text").shortcut, "paste");
  assert.equal(find(qt, "resume").shortcut, "none");
  const web = m.calcuShellMenus(inputs(), p, WEB);
  for (const id of ["load-preset", "reset", "cold-boot", "settings", "copy-screen", "paste-text"])
    assert.equal(find(web, id).shortcut, "none");
});

test("ac5_unavailable_disabled_with_reason", () => {
  const i = inputs(clone(INPUTS.unavailable));
  const ms = m.calcuShellMenus(i, product(), QT);
  assert.equal(find(ms, "model-Model-B").enabled, false);
  assert.equal(find(ms, "model-Model-B").reason, "Model-B needs a module.");
  assert.equal(find(ms, "reset").reason, "Reset is not possible now.");
  assert.equal(find(ms, "peripheral-printer-test-dotter").reason, "TD-20 does not fit now.");
  assert.ok(find(ms, "cold-boot").enabled);
  assert.equal(find(ms, "resume").enabled, false);
  i.runState = { paused: true };
  assert.ok(find(m.calcuShellMenus(i, product(), QT), "resume").enabled);
});

test("ac6_roles_and_no_quit_on_web", () => {
  const qt = m.calcuShellMenus(inputs(), product(), QT);
  assert.equal(find(qt, "settings").role, "preferences");
  assert.equal(find(qt, "about").role, "about");
  assert.equal(find(qt, "quit").role, "quit");
  for (const id of ["load-preset", "examples", "load-program", "model", "reset", "cold-boot", "resume", "help"])
    assert.equal(find(qt, id).role, "none");
  const web = m.calcuShellMenus(inputs(), product(), WEB);
  assert.equal(find(web, "quit"), null);
  assert.ok(find(web, "settings"));
});

test("ac7_product_item_in_its_group", () => {
  const p = product();
  p.items.push({ group: "FileOpen", id: "open-thing", label: "Open Thing…", entry: "" });
  p.items.push({ group: "FileEnd", id: "end-thing", label: "End Thing", entry: "" });
  p.items.push({ group: "Edit", id: "copy-log", label: "Copy Log", entry: "" });
  p.items.push({ group: "Help", id: "tips", label: "Tips", entry: "" });
  const ms = m.calcuShellMenus(inputs(), p, QT);
  assert.equal(ids(menu(ms, "file").items), "load-preset examples open-thing - load-program - mount-drive - end-thing settings quit");
  assert.equal(ids(menu(ms, "edit").items), "copy-log");
  assert.equal(ids(menu(ms, "help").items), "help tips about");
  assert.equal(find(ms, "disk-thing").enabled, false);
  assert.equal(find(ms, "disk-thing").reason, "No disk drive.");
});

test("media_items_where_saved_media_and_kinds", () => {
  const i = inputs();
  i.media = clone(INPUTS.media);
  const web = m.calcuShellMenus(i, product(), WEB);
  assert.equal(ids(menu(web, "file").items), "load-preset examples - load-program - import-media saved-media mount-drive - settings");
  assert.equal(find(web, "import-media").command, "media.import");
  assert.equal(find(web, "saved-media").label, "Saved Media…");
  assert.equal(find(m.calcuShellMenus(i, product(), QT), "import-media"), null);
  i.media = { kinds: [] };
  assert.equal(find(m.calcuShellMenus(i, product(), WEB), "import-media"), null);
});

test("ac19_no_machine_menu_without_control_state", () => {
  const i = inputs(null);
  i.romsMissing = { text: "ROM files are missing." };
  const ms = m.calcuShellMenus(i, product(), QT);
  assert.equal(ms.map((x) => x.id).join(" "), "file help");
  assert.equal(ids(menu(ms, "file").items), "settings quit");
  assert.equal(ids(menu(ms, "help").items), "help about");
});

test("ec4_model_switch_changes_entries", () => {
  const a = m.calcuShellMenus(inputs(clone(INPUTS.modelA)), product(), QT);
  const b = m.calcuShellMenus(inputs(clone(INPUTS.modelB)), product(), QT);
  assert.ok(find(a, "model-Model-A").checked);
  assert.ok(find(b, "model-Model-B").checked);
  assert.ok(find(a, "peripheral-aux-test-plotter"));
  assert.equal(find(b, "peripheral-aux-test-plotter"), null);
  assert.equal(ids(find(b, "peripherals").children), "peripheral-printer-test-dotter");
});

// L2: the bar and the panels.
const describeBar = (groups) => groups.map((g) => `[${g.controls.map((c) => c.id).join(" ")}]`).join("");
const control = (groups, id) => groups.flatMap((g) => g.controls).find((c) => c.id === id) || null;

test("ac8_groups_and_order", () => {
  const b = m.calcuShellBar(inputs(), product());
  assert.equal(describeBar(b), "[model][slot-1 slot-2][printer aux][card][]");
  assert.deepEqual(b.map((g) => g.group), ["AfterModel", "Slots", "Places", "Media", "End"]);
  assert.equal(control(b, "model").choices.length, 2);
  assert.equal(control(b, "card").mediumKind, "test-card");
});

test("ac8_slot_numbers_with_two_slots", () => {
  const b = m.calcuShellBar(inputs(), product());
  assert.equal(control(b, "slot-1").label, "1:");
  assert.equal(control(b, "slot-2").label, "2:");
  assert.equal(control(m.calcuShellBar(inputs(clone(INPUTS.oneSlot)), product()), "slot-1").label, "");
});

test("ac9_button_for_one_picker_for_several", () => {
  const b = m.calcuShellBar(inputs(), product());
  assert.equal(control(b, "printer").kind, "button");
  assert.equal(control(b, "printer").label, "TD-20");
  assert.equal(control(b, "aux").kind, "picker");
  assert.deepEqual(control(b, "aux").choices.map((c) => c.label), ["None", "TP-4", "TD-21"]);
});

test("ac9_button_attaches_and_detaches", () => {
  const off = control(m.calcuShellBar(inputs(), product()), "printer");
  assert.equal(off.choices[0].command, "attach");
  const on = control(m.calcuShellBar(inputs(clone(INPUTS.printerDotter)), product()), "printer");
  assert.ok(on.plugged);
  assert.equal(on.choices[0].command, "detach");
  const aux = control(m.calcuShellBar(inputs(clone(INPUTS.auxPlotter)), product()), "aux");
  assert.equal(aux.choices[0].command, "detach");
  assert.ok(aux.choices[1].checked);
});

test("ac9_place_needing_more_has_no_control", () => {
  assert.equal(control(m.calcuShellBar(inputs(), product()), "host-drive"), null);
  const p = product();
  p.placesWithoutControl = [];
  assert.ok(control(m.calcuShellBar(inputs(), p), "host-drive"));
});

test("ac10_no_entry_no_gap", () => {
  assert.equal(describeBar(m.calcuShellBar(inputs(clone(INPUTS.modelB)), product())), "[model][][printer][][]");
});

test("ac23_paper_panel_always_with_or_without_owner", () => {
  assert.deepEqual(m.calcuShellPanels(inputs()), [{ kind: "paper", owner: "", ownerPlace: "" }]);
  assert.deepEqual(m.calcuShellPanels(inputs(clone(INPUTS.printerDotter))), [{ kind: "paper", owner: "test-dotter", ownerPlace: "printer" }]);
});

test("ac42_shell_reads_no_product_entry", () => {
  const cs = clone(INPUTS.modelA);
  const before = describeBar(m.calcuShellBar(inputs(cs), product()));
  cs.product = { "tp.extra": { show: true, available: false }, "tp.disk": { available: true } };
  assert.equal(describeBar(m.calcuShellBar(inputs(cs), product())), before);
});

test("ec5_printer_change_moves_paper_owner", () => {
  assert.equal(m.calcuShellPanels(inputs(clone(INPUTS.printerDotter)))[0].owner, "test-dotter");
  assert.equal(m.calcuShellPanels(inputs(clone(INPUTS.modelA)))[0].owner, "");
});

test("b7_no_bar_without_control_state", () => {
  assert.deepEqual(m.calcuShellBar(inputs(null), product()), []);
  assert.deepEqual(m.calcuShellPanels(inputs(null)), []);
});

// L3: the dialog model, on a `settings` topic as SettingsPart writes it.
function settings(over) {
  const row = (section, key, kind, value, def, extra) => Object.assign({ section, key, kind, value, default: def, choices: [],
    inUse: value, isDefault: value === def }, extra || {});
  const rows = [
    row("startup", "model", "model-or-last", "last", "last", { choices: ["last", "Model-A", "Model-B"] }),
    row("preset", "Model-A", "text", "", ""),
    row("preset", "Model-B", "text", "", ""),
    row("storage", "save-folder", "path", "", "", { inUse: "/home/u/Test-Product" }),
    row("storage", "tape-folder", "path", "", "", { inUse: "/home/u/Test-Product/Tapes" }),
    row("trace", "folder", "path", "", "", { inUse: "/home/u/Test-Product" }),
    row("trace", "max-file-size-mb", "integer", "50", "50", { min: 1, max: 1000 }),
    row("folders", "presets", "folder", "", "", { inUse: "/home/u" }),
  ];
  for (const [k, v] of Object.entries(over || {})) Object.assign(rows.find((r) => `${r.section}.${r.key}` === k), v);
  return { home: "/home/u", rows };
}
const shownRow = (ss, key) => ss.flatMap((s) => s.rows).find((r) => `${r.row.section}.${r.row.key}` === key) || null;

test("ac11_sections_shown_for_keys_present", () => {
  const s = settings();
  const spec = m.calcuShellFamilySections(s.rows);
  const all = m.calcuShellDialog(s, INPUTS.presetList, spec);
  assert.deepEqual(all.map((x) => x.title), ["General", "Startup presets", "Storage", "Folders", "Tracing"]);
  assert.equal(shownRow(all, "startup.model").row.label, "Startup model");
  assert.equal(shownRow(all, "storage.save-folder").row.label, "Save folder");
  assert.equal(shownRow(all, "trace.max-file-size-mb").row.label, "Maximum file size (MB)");
  assert.equal(shownRow(all, "folders.presets").row.label, "Presets folder");
  assert.deepEqual(shownRow(all, "startup.model").choices[0], ["last", "Last used"]);
  const web = m.calcuShellDialog(s, INPUTS.presetList,
    m.calcuShellFamilySections(s.rows, ["startup.model", "preset.Model-A", "preset.Model-B", "trace.max-file-size-mb"]));
  assert.deepEqual(web.map((x) => x.title), ["General", "Startup presets", "Tracing"]);
});

test("ac12_startup_preset_choices", () => {
  const s = settings();
  const a = shownRow(m.calcuShellDialog(s, INPUTS.presetList, m.calcuShellFamilySections(s.rows)), "preset.Model-A");
  assert.deepEqual(a.choices, [["", "None"], ["examples:startup/a.test", "Start A — The A start"], ["examples:startup/a2.test", "Start A2"]]);
  assert.equal(a.row.label, "Model-A");
});

test("ac13_folder_row_home_change_reset", () => {
  let s = settings();
  let save = shownRow(m.calcuShellDialog(s, null, m.calcuShellFamilySections(s.rows)), "storage.save-folder");
  assert.equal(save.shown, "~/Test-Product");
  assert.equal(save.resettable, false);
  s = settings({ "storage.save-folder": { value: "/home/u/Elsewhere", inUse: "/home/u/Elsewhere", isDefault: false } });
  save = shownRow(m.calcuShellDialog(s, null, m.calcuShellFamilySections(s.rows)), "storage.save-folder");
  assert.equal(save.shown, "~/Elsewhere");
  assert.equal(save.resettable, true);
});

test("ec3_missing_startup_preset_still_shown", () => {
  const s = settings({ "preset.Model-A": { value: "/home/u/gone/start.test", isDefault: false } });
  const a = shownRow(m.calcuShellDialog(s, INPUTS.presetList, m.calcuShellFamilySections(s.rows)), "preset.Model-A");
  assert.deepEqual(a.choices[a.choices.length - 1], ["/home/u/gone/start.test", "~/gone/start.test"]);
});

test("with_tilde_only_for_home_and_below", () => {
  assert.equal(m.calcuShellWithTilde("/Users/x", "/Users/x"), "~");
  assert.equal(m.calcuShellWithTilde("/Users/x/a", "/Users/x"), "~/a");
  assert.equal(m.calcuShellWithTilde("/Users/xy/a", "/Users/x"), "/Users/xy/a");
  assert.equal(m.calcuShellWithTilde("/tmp/a", ""), "/tmp/a");
});

// L4: "Loading…" and the notices.
test("ac15_loading_after_500_ms_at_full_speed", () => {
  const r = m.calcuShellLoading();
  r.busy(true, true, 1000);
  assert.equal(r.shown(1000), false);
  assert.equal(r.due(), 1500);
  r.busy(true, true, 1499);
  assert.equal(r.shown(1499), false);
  assert.equal(r.shown(1500), true);
  r.busy(true, false, 1500);
  assert.equal(r.shown(1500), false);
  assert.equal(r.due(), null);
  r.busy(true, true, 1500);
  assert.equal(r.shown(2100), true);
  r.busy(false, false, 2100);
  assert.equal(r.shown(2100), false);
  r.busy(false, true, 2100);
  assert.equal(r.shown(3100), false);
});

test("ec1_short_operation_shows_nothing", () => {
  const r = m.calcuShellLoading();
  r.busy(true, true, 0);
  assert.equal(r.shown(450), false);
  r.busy(false, false, 450);
  assert.equal(r.shown(550), false);
});

test("ec7_back_to_full_speed_counts_again", () => {
  const r = m.calcuShellLoading();
  r.busy(true, true, 0);
  r.busy(true, false, 400);
  r.busy(true, true, 450);
  assert.equal(r.shown(850), false);
  assert.equal(r.shown(950), true);
});

test("ac17_notices_one_after_the_other", () => {
  const q = m.calcuShellNotices();
  assert.equal(q.current(), null);
  q.push("The first went wrong.");
  q.push("The second went wrong.");
  q.push("The first went wrong.");
  assert.equal(q.current().text, "The first went wrong.");
  q.close();
  assert.equal(q.current().text, "The second went wrong.");
  q.close();
  assert.equal(q.current().text, "The first went wrong.");
  q.close();
  assert.equal(q.current(), null);
});

test("ec2_notice_over_loading", () => {
  const r = m.calcuShellLoading();
  const q = m.calcuShellNotices();
  r.busy(true, true, 0);
  q.push("A file could not be read.");
  assert.ok(r.shown(600) && q.current());
  q.close();
  assert.ok(r.shown(600));
});

test("refusal_equal_to_last_notice_not_repeated", () => {
  const q = m.calcuShellNotices();
  q.push("The disk is full.");
  q.refusal("The disk is full.");
  assert.equal(q.waiting(), 1);
  q.refusal("Another operation is running.");
  q.refusal("");
  q.refusal("Stopped.");
  assert.equal(q.waiting(), 2);
});

test("asks_refusal_is_a_question_yes_resends_confirmed", () => {
  const q = m.calcuShellNotices();
  const sent = { name: "media.delete", args: { kind: "card", name: "A" } };
  q.refusal("asks: Delete the card A?", sent);
  assert.ok(q.current().question);
  assert.equal(q.current().text, "Delete the card A?");
  assert.deepEqual(q.answer(true), { name: "media.delete", args: { kind: "card", name: "A", confirmed: true } });
  q.refusal("asks: Replace it?", sent);
  assert.equal(q.answer(false), null);
  assert.equal(q.current(), null);
});

// L5: the hit rule, the same cases as the C++ test.
function hitMap() {
  return { keys: [["1", { x: 0, y: 20, w: 10, h: 10 }], ["2", { x: 20, y: 20, w: 10, h: 10 }], ["3", { x: 40, y: 20, w: 10, h: 10 }]],
    display: { x: 0, y: 0, w: 50, h: 15 }, distance: 4 };
}

test("ac24_hit_cases", () => {
  let map = null;
  let cases = 0;
  for (const line of fs.readFileSync(path.join(FIXTURES, "hit_cases.txt"), "utf8").split("\n")) {
    const words = line.trim().split(/\s+/);
    if (!words[0] || words[0].startsWith("#")) continue;
    if (words[0] === "map") {
      map = { distance: Number(words[1]), display: null, keys: words.slice(2).map((k) => {
        const [key, nums] = k.split(":");
        const [x, y, w, h] = nums.split(",").map(Number);
        return [key, { x, y, w, h }];
      }) };
    } else if (words[0] === "at") {
      const got = m.calcuShellKeyAt(map, Number(words[1]), Number(words[2]), Number(words[3]));
      assert.equal(got === null ? "-" : got, words[4], line);
      cases += 1;
    }
  }
  assert.ok(cases >= 25);
});

const describeOut = (out) => out.map((c) => `${c.name} ${JSON.stringify(c.args)}`).join(" ");

test("ac24_several_pointers_hold_keys", () => {
  const t = m.calcuShellPointer(hitMap(), 1);
  assert.equal(describeOut(t.down(1, 5, 25)), 'screen-key-down {"key":"1","pointer":1}');
  assert.equal(describeOut(t.down(2, 25, 25)), 'screen-key-down {"key":"2","pointer":2}');
  assert.equal(describeOut(t.down(3, 6, 26)), 'screen-key-down {"key":"1","pointer":3}');
  assert.equal(describeOut(t.move(1, 45, 25)), "");
  assert.equal(describeOut(t.up(1)), 'screen-key-up {"key":"1","pointer":1}');
  assert.equal(describeOut(t.up(1)), "");
  assert.equal(describeOut(t.releaseAll()), 'screen-key-up {"key":"2","pointer":2} screen-key-up {"key":"1","pointer":3}');
  const big = m.calcuShellPointer(hitMap(), () => 2);
  assert.equal(describeOut(big.down(7, 50, 50)), 'screen-key-down {"key":"2","pointer":7}');
});

test("ac43_display_hold_full_speed_until_up_or_leave", () => {
  const t = m.calcuShellPointer(hitMap(), 1);
  assert.equal(describeOut(t.down(1, 10, 5)), 'speed {"full":true}');
  assert.equal(describeOut(t.move(1, 20, 6)), "");
  assert.equal(describeOut(t.up(1)), 'speed {"full":false}');
  t.down(2, 10, 5);
  assert.equal(describeOut(t.move(2, 10, 40)), 'speed {"full":false}');
  assert.equal(describeOut(t.move(2, 10, 5)), "");
  assert.equal(describeOut(t.up(2)), "");
  assert.equal(describeOut(t.down(3, 10, 5)), 'speed {"full":true}');
  assert.equal(describeOut(t.down(4, 30, 5)), "");
  assert.equal(describeOut(t.up(3)), "");
  assert.equal(describeOut(t.up(4)), 'speed {"full":false}');
});

test("ec8_no_key_at_down_nothing_on_move", () => {
  const t = m.calcuShellPointer(hitMap(), 1);
  assert.equal(describeOut(t.down(1, 15, 40)), "");
  assert.equal(describeOut(t.move(1, 5, 25)), "");
  assert.equal(describeOut(t.up(1)), "");
});

// L6: Help and About.
test("ac20_help_opens_product_address", () => {
  const ms = m.calcuShellMenus(inputs(), product(), WEB);
  assert.equal(find(ms, "help").label, "Test-Product Help");
  assert.equal(find(ms, "help").command, "");
  assert.ok(find(m.calcuShellMenus(inputs(null), product(), WEB), "help"));
});

test("ac21_about_shows_product_texts", () => {
  const p = product();
  const a = m.calcuShellAbout(p);
  assert.equal(a.title, "About Test-Product");
  assert.equal(a.version, "Version 1.2.3 (abc123)");
  assert.equal(a.lines[0], "A product made for the tests.");
  assert.equal(a.links[1][1], "https://example.org/privacy");
  p.aboutLabel = "About Test-Product…";
  assert.equal(m.calcuShellAbout(p).title, "About Test-Product");
  p.build = "";
  assert.equal(m.calcuShellAbout(p).version, "Version 1.2.3");
});

// Part E (design §10.3.1, §10.3.2): a window without a menu bar (M13 to
// M18, B8) and labelled choices (S8); the C++ cases of the same names in
// menu_model_tests.cpp, bar_model_tests.cpp, settings_dialog_tests.cpp.
const PORTRAIT = Object.assign({}, WEB, { menuBar: false });
const withAction = (cs, command, available) => {
  for (const a of cs.actions) if (a.command === command) Object.assign(a, available);
  return cs;
};

test("more_items_are_the_items_without_a_control", () => {
  const p = product();
  const inp = Object.assign(inputs(), { media: clone(INPUTS.media) });
  const ms = m.calcuShellMenus(inp, p, PORTRAIT);
  assert.equal(ms.map((x) => x.id).join(" "), "file machine help");
  assert.equal(ids(m.calcuShellMoreItems(ms, p)),
    "examples - load-program - import-media saved-media mount-drive - variants-model - cold-boot resume disk-thing - help about");
  p.items[1].onBar = true;
  assert.equal(ids(m.calcuShellMoreItems(m.calcuShellMenus(inp, p, PORTRAIT), p)),
    "examples - load-program - import-media saved-media - variants-model - cold-boot resume disk-thing - help about");
  p.items[0].onBar = true;
  assert.equal(ids(m.calcuShellMoreItems(m.calcuShellMenus(inp, p, PORTRAIT), p)),
    "examples - import-media saved-media - variants-model - cold-boot resume disk-thing - help about");
});

test("more_cold_boot_asks_first", () => {
  const p = product();
  const ms = m.calcuShellMenus(inputs(), p, PORTRAIT);
  const more = m.calcuShellMoreItems(ms, p);
  const cold = more.find((i) => i.id === "cold-boot");
  assert.equal(cold.confirm, "Cold boot the machine? What it holds in memory is lost.");
  assert.equal(cold.destructive, true);
  assert.equal(cold.command, "cold-boot");
  assert.ok(!find(ms, "cold-boot").confirm);
  for (const i of more) if (i.id !== "cold-boot") assert.ok(!i.confirm);
});

test("more_follows_availability", () => {
  const p = product();
  const cs = withAction(clone(INPUTS.modelA), "cold-boot", { available: false, reason: "An operation runs." });
  let more = m.calcuShellMoreItems(m.calcuShellMenus(inputs(cs), p, PORTRAIT), p);
  const by = (id) => more.find((i) => i.id === id);
  assert.equal(by("cold-boot").enabled, false);
  assert.equal(by("cold-boot").reason, "An operation runs.");
  assert.equal(by("resume").enabled, false);
  assert.equal(by("disk-thing").reason, "No disk drive.");
  const inp = inputs();
  inp.runState = { paused: true };
  more = m.calcuShellMoreItems(m.calcuShellMenus(inp, p, PORTRAIT), p);
  assert.equal(by("cold-boot").enabled, true);
  assert.equal(by("resume").enabled, true);
});

test("media_place_item_only_with_its_media_place", () => {
  const p = product();
  p.items.push({ group: "FileMedia", id: "fit-card", label: "Fit Card…", entry: "", mediaPlace: "card" });
  let ms = m.calcuShellMenus(inputs(), p, QT);
  assert.equal(find(ms, "fit-card").enabled, true);
  const cs = clone(INPUTS.modelA);
  cs.mediaPlaces[0].insert = { available: false, reason: "A card passes." };
  ms = m.calcuShellMenus(inputs(cs), p, QT);
  assert.equal(find(ms, "fit-card").enabled, false);
  assert.equal(find(ms, "fit-card").reason, "A card passes.");
  ms = m.calcuShellMenus(inputs(clone(INPUTS.modelB)), p, QT);
  assert.equal(find(ms, "fit-card"), null);
});

test("no_settings_item_without_settings", () => {
  const t = Object.assign({}, WEB, { settings: false });
  let ms = m.calcuShellMenus(inputs(), product(), t);
  assert.equal(find(ms, "settings"), null);
  assert.ok(find(ms, "about"));
  t.about = false;
  ms = m.calcuShellMenus(inputs(), product(), t);
  assert.equal(find(ms, "about"), null);
  assert.equal(ids(menu(ms, "help").items), "help");
});

test("bar_without_menu_bar_keeps_reset_preset_settings_and_more", () => {
  const p = product();
  const b = m.calcuShellBar(inputs(), p, PORTRAIT);
  assert.equal(describeBar(b), "[reset][model][slot-1 slot-2][printer aux][card][load-preset settings more]");
  assert.equal(b[0].group, "Start");
  assert.equal(b[5].group, "End");
  const reset = control(b, "reset");
  assert.equal(reset.kind, "action");
  assert.equal(reset.label, "Reset");
  assert.equal(reset.item.command, "reset");
  assert.equal(control(b, "load-preset").label, "Preset");
  assert.equal(control(b, "load-preset").item.command, "");
  assert.equal(control(b, "settings").label, "Settings");
  const more = control(b, "more");
  assert.equal(more.kind, "more");
  assert.equal(more.choices.length, m.calcuShellMoreItems(m.calcuShellMenus(inputs(), p, PORTRAIT), p).length);
  const cs = withAction(clone(INPUTS.modelA), "reset", { available: false, reason: "An operation runs." });
  const b2 = m.calcuShellBar(inputs(cs), p, PORTRAIT);
  assert.equal(control(b2, "reset").item.enabled, false);
  assert.equal(control(b2, "reset").item.reason, "An operation runs.");
  assert.equal(control(m.calcuShellBar(inputs(), p, Object.assign({}, PORTRAIT, { settings: false })), "settings"), null);
  assert.equal(describeBar(m.calcuShellBar(inputs(), p, WEB)), "[model][slot-1 slot-2][printer aux][card][]");
});

const styleSpec = () => [{ title: "Look", rows: [{ widget: "choice", section: "test", key: "style", label: "Style", choices: [["one", "The first"]] }] }];
const styleSettings = (value) => ({ home: "/home/u", rows: [{ section: "test", key: "style", kind: "choice", value, default: "one",
  choices: ["one", "two"], inUse: value, isDefault: value === "one" }] });

test("choice_row_with_labels_offers_only_those", () => {
  const style = shownRow(m.calcuShellDialog(styleSettings("one"), null, styleSpec()), "test.style");
  assert.deepEqual(style.choices, [["one", "The first"]]);
  const plain = [{ title: "Look", rows: [{ widget: "choice", section: "test", key: "style", label: "Style" }] }];
  assert.deepEqual(shownRow(m.calcuShellDialog(styleSettings("one"), null, plain), "test.style").choices, [["one", "one"], ["two", "two"]]);
});

test("choice_value_not_listed_shown_as_written", () => {
  const style = shownRow(m.calcuShellDialog(styleSettings("two"), null, styleSpec()), "test.style");
  assert.equal(style.value, "two");
  assert.deepEqual(style.choices, [["one", "The first"], ["two", "two"]]);
});
