#pragma once
#include "sessions/Value.hpp"
#include <functional>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

// What a product gives the shell (LIB-14 design §3.2; spec AC-2, AC-7, AC-20,
// AC-21, AC-42; ADR 0023, frontend.md I-4): its name and texts, the labels of
// its variant menus, the marks of module slots and of places without a
// control (D-23), and its own menu items. The shell names no model and no
// product; all of that comes from here.

namespace calcu::shell {

// The groups of AC-1 a product item goes into (M1).
enum class MenuGroup { FileOpen, FileKinds, FileMedia, FileEnd, Edit, MachineEnd, Help };
// The groups of the control bar (B1, B2), left to right.
// Start: before the model picker, only in a window without a menu bar (B8).
enum class BarGroup { Start, AfterModel, Slots, Places, Media, End };

struct ProductItem {   // AC-7
    MenuGroup group = MenuGroup::FileKinds;
    std::string id, label;   // id: stable, for the object name `item-<id>`
    // A product entry of the control state ({available, reason}) that says
    // whether the item can be used now; empty: always (M9).
    std::string entry;
    std::function<void()> act;
    // The product draws its own control for it on the bar, so a window
    // without a menu bar leaves it out of the More menu (M14).
    bool onBar = false;
    // The item exists while the control state lists this media place, and
    // is enabled exactly when its `insert` is (M17; LIB-4 AC-12). Empty: none.
    std::string mediaPlace;
};

struct ShellProduct {
    std::string name, version, build;               // "Calc-U-1600"
    std::string helpUrl;                            // AC-20; "https://calc-u.org/1600/"
    std::string aboutLabel;                         // empty: "About <name>"
    std::vector<std::string> aboutLines;            // AC-21, the texts in order
    std::vector<std::pair<std::string, std::string>> aboutLinks;   // label, address
    std::map<std::string, std::string> variantLabels;   // model or peripheral ID → submenu label (AC-2)
    std::string presetFilter;                       // the file dialog's filter for Load Preset…
    // Edit items (AC-1; D-12): Copy Screen is the product's (AC-41), so it
    // is its function; Paste Text is the shell's, switched on here.
    std::function<void()> copyScreen;
    bool pasteText = false;
    std::set<std::string> moduleSlotPinOuts;        // the pin-outs of module slots (D-23)
    std::set<std::string> placesWithoutControl;     // AC-9: "host-drive" (D-23)
    std::vector<ProductItem> items;
};

// The About dialog (AC-21): the product's name, "Version <v> (<build>)",
// its texts and links in order, one button to close it.
struct AboutModel {
    std::string title;     // "About <name>"
    std::string name;
    std::string version;   // "Version 1.2.3 (abc123)"
    std::vector<std::string> lines;
    std::vector<std::pair<std::string, std::string>> links;
};
AboutModel about(const ShellProduct&);

// The problems of a product's shell data, for its tests (D-13): a model or
// peripheral with variants in `catalogueControlState` and no label, an empty
// name or help address, duplicate item ids, an item's `mediaPlace` that the
// control state does not list (pass each model's control state). Empty when
// sound.
std::vector<std::string> checkShellProduct(const ShellProduct&, const sessions::Value& catalogueControlState);
// The same over every model's control state, and an item's `mediaPlace`
// that none of them lists.
std::vector<std::string> checkShellProduct(const ShellProduct&, const std::vector<sessions::Value>& controlStates);

} // namespace calcu::shell
