#pragma once
#include "sessions/Messages.hpp"
#include "sessions/Value.hpp"
#include <functional>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

// What a product gives the shell: its name and texts, the labels of
// its variant menus, the marks of module slots and of places without a
// control, and its own menu items. The shell names no model and no
// product; all of that comes from here.

namespace calcu::shell {

// The menu groups a product item goes into.
enum class MenuGroup { FileOpen, FileKinds, FileMedia, FileEnd, Edit, MachineEnd, Help };
// The groups of the control bar, left to right.
enum class BarGroup { AfterModel, Slots, Places, Media, End };

struct ProductItem {
    MenuGroup group = MenuGroup::FileKinds;
    std::string id, label;   // id: stable, for the object name `item-<id>`
    // A product entry of the control state ({available, reason}) that says
    // whether the item can be used now; empty: always.
    std::string entry;
    std::function<void()> act;
    // The item exists while the control state lists this media place, and
    // is enabled exactly when its `insert` is. Empty: none.
    std::string mediaPlace = {};
    // A command of the session in place of `act`: sent when chosen, a
    // refusal shown as a notice. The model carries it, so the item works on
    // every frontend that draws the model.
    sessions::CommandName command = {};
    sessions::Value args = {};
    // A toggle or a choice that follows a state topic of the session:
    // checkable, checked while the topic's member `key` equals `equals`.
    // Chosen while checked, it sends `checkedArgs` (a toggle turned off);
    // without them nothing (a choice: it is what runs).
    struct Check {
        sessions::TopicName topic;
        std::string key;
        sessions::Value equals;
        sessions::Value checkedArgs = {};
    };
    std::optional<Check> checkedBy = {};
    // A submenu of these items in place of an action.
    std::vector<ProductItem> children = {};
};

struct ShellProduct {
    std::string name, version, build;               // "Calc-U-1600"
    std::string helpUrl;                            // "https://calc-u.org/1600/"
    std::string aboutLabel;                         // empty: "About <name>"
    std::vector<std::string> aboutLines;            // the texts in order
    std::vector<std::pair<std::string, std::string>> aboutLinks;   // label, address
    std::map<std::string, std::string> variantLabels;   // model or peripheral ID → submenu label
    // Model ID → its name on the bar's model picker, short so the bar fits
    // beside the faceplate; a model not listed shows its ID. The menus
    // keep the ID.
    std::map<std::string, std::string> barModelLabels;
    // The bar as the iOS app draws it: the media group in the middle (else
    // packed left after the places; End is at the right edge on every bar), and each picker
    // showing its short text when closed (the model's bar name, a module's
    // ID) instead of the chosen entry's label. The bar model carries both
    // (BarGroupModel::pushed, BarControl::shortShown).
    bool barSpread = false;
    bool barShortPickers = false;
    std::string presetFilter;                       // the file dialog's filter for Load Preset…
    // The Web's Upload Preset…: an accept list (".ti59,.yaml") of the presets
    // and every file a preset can name. A page has no folder to look in, so
    // the preset is chosen together with its files.
    std::string presetUploads;
    // Edit items: Copy Screen is the product's, so it
    // is its function; Paste Text is the shell's, switched on here.
    std::function<void()> copyScreen;
    bool pasteText = false;
    std::set<std::string> moduleSlotPinOuts;        // the pin-outs of module slots
    std::set<std::string> placesWithoutControl;     // "host-drive"
    // Places whose control the paper panel shows, at the left of its
    // header, instead of the bar: Calc-U-59's PC-100C over its paper.
    std::set<std::string> placesOnPaper;
    // The media kinds a mobile bar keeps ("magnetic-card"); every other
    // medium is chosen through More there.
    std::set<std::string> mobileBarMedia;
    // Name & Save… of a media place, which the shell carries out with the
    // media components' dialog: from the place's submenu (mobile), from the
    // button after each module slot's picker on the bar (`slotButtons`) and
    // from a product's own media control (the frontend shell's
    // nameAndSave(place)). The dialog asks for the name with the place's
    // prompt in `prompts` (by the place of the topic `media`), else with `prompt`.
    struct NameAndSave {
        bool slotButtons = false;
        std::string prompt = "Name:";
        std::map<std::string, std::string> prompts;   // place → prompt ("floppy" → "Disk name:")
    };
    NameAndSave nameAndSave;
    std::vector<ProductItem> items;
};

// The Name & Save dialog's prompt for a place.
std::string nameAndSavePrompt(const ShellProduct&, const std::string& place);

// The About dialog: the product's name, "Version <v> (<build>)",
// its texts and links in order, one button to close it.
struct AboutModel {
    std::string title;     // "About <name>"
    std::string name;
    std::string version;   // "Version 1.2.3 (abc123)"
    std::vector<std::string> lines;
    std::vector<std::pair<std::string, std::string>> links;
};
AboutModel about(const ShellProduct&);

// The state topics the product's items follow (their checkedBy), each once:
// a frontend keeps their last values in ShellInputs::productTopics.
std::vector<sessions::TopicName> productTopics(const ShellProduct&);

// The problems of a product's shell data, for its tests: a model or
// peripheral with variants in `catalogueControlState` and no label, an empty
// name or help address, duplicate item ids, an item that follows a topic and
// sends no command, an item's `mediaPlace` that the
// control state does not list (pass each model's control state). Empty when
// sound.
std::vector<std::string> checkShellProduct(const ShellProduct&, const sessions::Value& catalogueControlState);
// The same over every model's control state, and an item's `mediaPlace`
// that none of them lists.
std::vector<std::string> checkShellProduct(const ShellProduct&, const std::vector<sessions::Value>& controlStates);

} // namespace calcu::shell
