#pragma once
#include "sessions/Value.hpp"
#include "shell/HitMap.hpp"
#include "shell/SettingsModel.hpp"
#include "shell/ShellProduct.hpp"

// The shell's data in the Value forms the Web page reads
// (library/web/components/shellModel.js): written once in C++, so a product's
// Web part sends its ShellProduct, its settings dialog and its faceplate's
// hit map as they are (Calc-U-1600's `c1600.shell-product`,
// `c1600.faceplate-map`). Objects keep their members in the order below.

namespace calcu::shell {

// {name, helpUrl, aboutLabel, aboutLines: [...], aboutLinks: [[label, url]]
//  (only when there is one), variantLabels: {id: label}, presetFilter,
//  copyScreen, pasteText, moduleSlotPinOuts: [...], placesWithoutControl:
//  [...], items: [{group, id, label, entry, onBar?, mediaPlace?}]}.
// Not in it: `version`, `build` (the page has its own). `copyScreen` says
// whether the product has the item; the page adds its own act.
// A dialog row's `choices` ([[value, label]]) only when set.
sessions::Value toValue(const ShellProduct&);

// [{title, rows: [{widget, section, key, label}]}]; widgets
// "startup-model", "startup-preset", "folder", "integer", "choice", "text",
// "note".
sessions::Value toValue(const DialogSpec&);

// {distance, display: {x, y, w, h}, keys: [{name, x, y, w, h}]}, in the
// map's own units; `display` only when the map has one.
sessions::Value toValue(const HitMap&);

// {x, y, w, h}.
sessions::Value toValue(const Rect&);

} // namespace calcu::shell
