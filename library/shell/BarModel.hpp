#pragma once
#include "sessions/Value.hpp"
#include "shell/Inputs.hpp"
#include "shell/MenuModel.hpp"
#include "shell/ShellProduct.hpp"
#include <string>
#include <vector>

// The control bar: groups left to right, inside a group the descriptor order. The
// shell reads no product entry: a product's own control goes into the group
// it names (Qt6 Shell::addBarControl, React `extra`). Mirrored in
// shellModel.js (calcuShellBar).

namespace calcu::shell {

struct BarControl {
    // Action and More only in a window without a menu bar.
    enum class Kind { ModelPicker, SlotPicker, PlaceButton, PlacePicker, Media, Action, More };
    Kind kind = Kind::ModelPicker;
    std::string id;           // "model", a place, a media place
    std::string label;        // "1:" before a slot when the model has more than one; the peripheral's name on a button
    bool plugged = false;     // a button: shown as pressed
    std::vector<MenuItem> choices;   // a picker: "None" and what fits; a button: its one choice
    sessions::Value entry;    // the place or media place entry, for the media components' and the products' factories
    std::string mediumKind;   // Media: the kind, which picks the factory
    MenuItem item;            // Action: the menu item it carries out (id, label, enabled, reason, command)
};

struct BarGroupModel {
    BarGroup group;
    std::vector<BarControl> controls;
};

// The bar's groups; with `traits.menuBar == false` Start holds the Action
// `reset` ("Reset"); End the Actions `load-preset` ("Preset") and
// `settings` ("Settings", unless `traits.settings` is off), then More, whose
// `choices` are moreItems(). With a menu bar there is no Start group.
std::vector<BarGroupModel> bar(const ShellInputs&, const ShellProduct&, Traits = {});

// The panels the control state lists (the paper panel always),
// each with its owner's peripheral ID or none.
struct Panel {
    std::string kind;    // "paper"
    std::string owner;   // the peripheral ID; empty: none
    std::string ownerPlace;
};
std::vector<Panel> panels(const ShellInputs&);

} // namespace calcu::shell
