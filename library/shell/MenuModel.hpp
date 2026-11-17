#pragma once
#include "sessions/Messages.hpp"
#include "sessions/Value.hpp"
#include "shell/Inputs.hpp"
#include "shell/ShellProduct.hpp"
#include <string>
#include <vector>

// The menus (LIB-14 design §3.3, rules M1 to M12; spec AC-1 to AC-7, AC-19,
// EC-4): computed afresh from the inputs at each change; a component set
// replaces what changed in place (M11). Mirrored rule by rule in
// library/web/components/shellModel.js (calcuShellMenus).

namespace calcu::shell {

struct MenuItem {
    enum class Kind { Action, Submenu, Separator };
    enum class Role { None, Preferences, About, Quit };   // M2, M8, M12 (AC-6)
    enum class Shortcut { None, LoadPreset, Reset, ColdBoot, Preferences, Quit, Copy, Paste };   // AC-4, D-14

    Kind kind = Kind::Action;
    std::string id, label;
    bool checkable = false, checked = false, enabled = true;
    std::string reason;          // the tooltip of a disabled item (AC-5)
    Role role = Role::None;
    Shortcut shortcut = Shortcut::None;
    sessions::CommandName command;   // sent when chosen; empty: a shell action (by id) or a product item
    sessions::Value args;
    std::vector<MenuItem> children;
    // A question asked before the item is carried out, with the buttons of
    // the item's label and Cancel; empty: none (M15). `destructive`: shown as
    // such where the platform has it (iOS; C59-8 AC-5).
    std::string confirm;
    bool destructive = false;
};

struct Menu {
    std::string id, label;   // "file", "File"
    std::vector<MenuItem> items;
};

// Qt6: {true, true, false}; Web: {false, false, true} (AC-4, AC-6; LIB-7 OQ-4).
// Part E (design §10.3.1): `menuBar` false for a window without a menu bar
// (the Web's portrait layout, iOS; M13): the menus are still computed, for
// the More button; `settings` false: no Settings… (the Web prop `settings`
// off, LIB-15 AC-19; M18); `about` false: no About (iOS, C59-8 AC-11).
struct Traits {
    bool hasQuit = true;
    bool shortcuts = true;
    bool savedMedia = false;
    bool menuBar = true;
    bool settings = true;
    bool about = true;
};

// The ids of the shell's own actions (an item without a command).
namespace actions {
inline constexpr const char* kLoadPreset = "load-preset";   // a file dialog, then `load-preset {ref}`
inline constexpr const char* kSavedMedia = "saved-media";   // LIB-7's saved media dialog
inline constexpr const char* kSettings = "settings";
inline constexpr const char* kQuit = "quit";
inline constexpr const char* kCopyScreen = "copy-screen";   // the product's function
inline constexpr const char* kPasteText = "paste-text";     // `paste {text}` with the clipboard's text
inline constexpr const char* kHelp = "help";
inline constexpr const char* kAbout = "about";
} // namespace actions

std::vector<Menu> menus(const ShellInputs&, const ShellProduct&, Traits);

// M14: the items of the More button (a window without a menu bar), in menu
// order: File, Edit, Machine, Help without what has a control on such a bar
// (Load Preset…, Settings…, Quit, Reset, Model ▸, Peripherals ▸, a product
// item with `onBar`); one separator between the groups that remain, never
// first, last or twice. Cold boot asks first (M15).
std::vector<MenuItem> moreItems(const std::vector<Menu>&, const ShellProduct&);
inline constexpr const char* kColdBootQuestion = "Cold boot the machine? What it holds in memory is lost.";

// Shared with the bar model (B5): the models as items, the running one
// checked, each sending `model {name}` but the running one (AC-2).
std::vector<MenuItem> modelItems(const sessions::Value& controlState);
// The kind of a place for the shell (D-23, B3).
enum class PlaceKind { ModuleSlot, Button, Picker, NoControl };
PlaceKind placeKind(const sessions::Value& place, const ShellProduct&);
// The choices of a place: for a picker "None" first (D-11), then what fits;
// each sends `attach {place, peripheral}` when not plugged, `detach {place}`
// for the plugged one (a button) or for "None" (a picker).
std::vector<MenuItem> placeChoices(const sessions::Value& place, bool withNone);

// A menu item by id, depth first, or nullptr (the tests, the component sets).
const MenuItem* findItem(const std::vector<Menu>&, const std::string& id);
const Menu* findMenu(const std::vector<Menu>&, const std::string& id);

} // namespace calcu::shell
