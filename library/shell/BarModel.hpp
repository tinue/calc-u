#pragma once
#include "sessions/Value.hpp"
#include "shell/Inputs.hpp"
#include "shell/MenuModel.hpp"
#include "shell/ShellProduct.hpp"
#include <string>
#include <vector>

// The control bar (LIB-14 design §3.4, rules B1 to B7; spec AC-8 to AC-10,
// AC-42): groups left to right, inside a group the descriptor order. The
// shell reads no product entry: a product's own control goes into the group
// it names (Qt6 Shell::addBarControl, React `extra`). Mirrored in
// shellModel.js (calcuShellBar).

namespace calcu::shell {

struct BarControl {
    enum class Kind { ModelPicker, SlotPicker, PlaceButton, PlacePicker, Media };
    Kind kind = Kind::ModelPicker;
    std::string id;           // "model", a place, a media place
    std::string label;        // "1:" before a slot when the model has more than one; the peripheral's name on a button
    bool plugged = false;     // a button: shown as pressed
    std::vector<MenuItem> choices;   // a picker: "None" (D-11) and what fits; a button: its one choice
    sessions::Value entry;    // the place or media place entry, for LIB-7's and C59-7's factories
    std::string mediumKind;   // Media: the kind, which picks the factory
};

struct BarGroupModel {
    BarGroup group;
    std::vector<BarControl> controls;
};

std::vector<BarGroupModel> bar(const ShellInputs&, const ShellProduct&);

// B6, AC-23: the panels the control state lists (the paper panel always),
// each with its owner's peripheral ID or none.
struct Panel {
    std::string kind;    // "paper"
    std::string owner;   // the peripheral ID; empty: none
    std::string ownerPlace;
};
std::vector<Panel> panels(const ShellInputs&);

} // namespace calcu::shell
