#pragma once
#include "sessions/Value.hpp"
#include "shell/BarModel.hpp"
#include "shell/HitMap.hpp"
#include "shell/MenuModel.hpp"
#include "shell/SettingsModel.hpp"
#include "shell/ShellProduct.hpp"

// The shell's data in the Value forms the Web page reads: written once in
// C++, so the worker sends the models (ModelHost) and a product's Web part
// its data and its faceplate's hit map as they are (Calc-U-1600's
// `c1600.shell-product`, `c1600.faceplate-map`). Objects keep their members
// in the order below.

namespace calcu::shell {

// What the page needs of the product beside the models: {name, helpUrl,
// presetUploads, nameAndSave: {slotButtons}, items: [{id, label}]} (the page
// acts on its items by id). Not in it: `version`, `build` (the page has its
// own), About and the Name & Save prompts (ModelHost serves them).
sessions::Value toValue(const ShellProduct&);

// About as the page draws it: {title, name, version, lines: [...], links:
// [[label, url]], supportHeading, support: [[label, value]], text} (`text`:
// aboutText, what Copy puts on the clipboard).
sessions::Value toValue(const AboutModel&);

// {distance, display: {x, y, w, h}, keys: [{name, x, y, w, h}]}, in the
// map's own units; `display` only when the map has one.
sessions::Value toValue(const HitMap&);

// {x, y, w, h}.
sessions::Value toValue(const Rect&);

// The models the Web page draws (ModelHost): an item {kind: "action" |
// "submenu" | "separator" | "heading", id, label, checkable, checked, enabled, reason,
// role: "none" | "preferences" | "about" | "quit", shortcut: "none" |
// "load-preset" | "reset" | "cold-boot" | "preferences" | "quit" | "copy" |
// "paste", command, args, children, confirm, destructive}; a menu {id,
// label, items}; a bar group {group: "AfterModel" | "Slots" | "Places" |
// "Media" | "End", controls, pushed}, a control {kind: "model" | "slot" |
// "button" | "picker" | "media" | "more", id, label, icon, shown, shortShown,
// plugged, choices, entry, mediumKind}; a panel {kind, owner, ownerPlace, hint,
// places}; a dialog section {title, rows: [{row, value, shown, choices:
// [[value, label]], resettable, min, max}]} (min, max null when unset).
sessions::Value toValue(const MenuItem&);
sessions::Value toValue(const std::vector<Menu>&);
sessions::Value toValue(const std::vector<BarGroupModel>&);
sessions::Value toValue(const std::vector<Panel>&);
sessions::Value toValue(const std::vector<ShownSection>&);

} // namespace calcu::shell
