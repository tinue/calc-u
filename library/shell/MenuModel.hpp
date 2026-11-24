#pragma once
#include "sessions/Messages.hpp"
#include "sessions/Value.hpp"
#include "shell/Inputs.hpp"
#include "shell/ShellProduct.hpp"
#include <string>
#include <vector>

// The menus: computed afresh from the inputs at each change; a component
// set replaces what changed in place.

namespace calcu::shell {

struct MenuItem {
    enum class Kind { Action, Submenu, Separator, Heading };   // Heading: a menu's name in More
    enum class Role { None, Preferences, About, Quit };
    enum class Shortcut { None, LoadPreset, Reset, ColdBoot, Preferences, Quit, Copy, Paste };

    Kind kind = Kind::Action;
    std::string id, label;
    bool checkable = false, checked = false, enabled = true;
    std::string reason;          // the tooltip of a disabled item
    Role role = Role::None;
    Shortcut shortcut = Shortcut::None;
    sessions::CommandName command;   // sent when chosen; empty: a shell action (by id) or a product item
    sessions::Value args;
    std::vector<MenuItem> children;
    // A question asked before the item is carried out, with the buttons of
    // the item's label and Cancel; empty: none. `destructive`: shown as
    // such where the platform has it (iOS).
    std::string confirm;
    bool destructive = false;

    bool operator==(const MenuItem&) const = default;
};

struct Menu {
    std::string id, label;   // "file", "File"
    std::vector<MenuItem> items;

    bool operator==(const Menu&) const = default;
};

// What a window offers. Qt6: {true, true, false}; Web: {false, false,
// true}. `mobile`: the window is mobile (the iOS app, the installed Web
// app, a browser that asks for the mobile site), else desktop (Qt, a Web
// page in a desktop browser's tab); a desktop window has a menu bar, a
// mobile one none, and its bar keeps only what is used while the machine
// runs. The menus are computed on both, for the More button. `settings`
// false: no Settings… (the Web prop `settings` off); `about` false: no
// About (iOS).
struct Traits {
    bool hasQuit = true;
    bool shortcuts = true;
    bool savedMedia = false;
    bool mobile = false;
    bool settings = true;
    bool about = true;
};

// The ids of the shell's own actions (an item without a command).
namespace actions {
inline constexpr const char* kLoadPreset = "load-preset";   // a file dialog, then `load-preset {ref}`
inline constexpr const char* kSavedMedia = "saved-media";   // the media components' saved media dialog
inline constexpr const char* kSettings = "settings";
inline constexpr const char* kQuit = "quit";
inline constexpr const char* kCopyScreen = "copy-screen";   // each frontend brings its act
inline constexpr const char* kPasteText = "paste-text";     // `paste {text}` with the clipboard's text
inline constexpr const char* kHelp = "help";
inline constexpr const char* kShowAppLog = "show-app-log";   // the app log: its file, or (Web) a download
inline constexpr const char* kAbout = "about";
// Mobile, a media place's submenu: `<prefix><place>`, args {place}, the
// media components' Name & Save dialog for that place.
inline constexpr const char* kNameAndSavePrefix = "media-name-and-save-";
} // namespace actions

std::vector<Menu> menus(const ShellInputs&, const ShellProduct&, Traits);

// The items of the More button (on every bar): every menu's items in menu
// order, each menu after its heading (its name in capitals, id
// `heading-<menu>`); submenus stay submenus. Cold boot asks first.
std::vector<MenuItem> moreItems(const std::vector<Menu>&);
inline constexpr const char* kColdBootQuestion = "Cold boot the machine? What it holds in memory is lost.";

// Shared with the bar model: the models as items, the running one
// checked, each sending `model {name}` but the running one.
std::vector<MenuItem> modelItems(const sessions::Value& controlState);
// The kind of a place for the shell.
enum class PlaceKind { ModuleSlot, Button, Picker, NoControl };
PlaceKind placeKind(const sessions::Value& place, const ShellProduct&);
// The choices of a place: for a picker "None" first, then what fits;
// each sends `attach {place, peripheral}` when not plugged, `detach {place}`
// for the plugged one (a button) or for "None" (a picker).
std::vector<MenuItem> placeChoices(const sessions::Value& place, bool withNone);

// A menu item by id, depth first, or nullptr (the tests, the component sets).
const MenuItem* findItem(const std::vector<Menu>&, const std::string& id);
const Menu* findMenu(const std::vector<Menu>&, const std::string& id);

} // namespace calcu::shell
