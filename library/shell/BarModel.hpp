#pragma once
#include "sessions/Value.hpp"
#include "shell/Inputs.hpp"
#include "shell/MenuModel.hpp"
#include "shell/ShellProduct.hpp"
#include <limits>
#include <string>
#include <vector>

// The control bar: groups left to right, inside a group the descriptor order. The
// shell reads no product entry: a product's own control goes into the group
// it names (Qt6 Shell::addBarControl, React `extra`). Mirrored in
// shellModel.js (calcuShellBar).

namespace calcu::shell {

struct BarControl {
    enum class Kind { ModelPicker, SlotPicker, PlaceButton, PlacePicker, Media, More };
    Kind kind = Kind::ModelPicker;
    std::string id;           // "model", a place, a media place
    std::string label;        // "1:" before a slot when the model has more than one; the peripheral's name on a button
    // A picker: what it shows closed where the product wants it short
    // (shortShown): the model's bar name, the chosen entry's ID ("ML"), "—"
    // for none. A module slot's media picker shows the topic's `short`.
    std::string shown;
    bool shortShown = false;
    bool plugged = false;     // a button: shown as pressed
    // A picker: its empty entry ("No printer") and what fits; a button: its
    // one choice; More: moreItems().
    std::vector<MenuItem> choices;
    sessions::Value entry;    // the place or media place entry, for the media components' and the products' factories
    std::string mediumKind;   // Media: the kind, which picks the factory
};

struct BarGroupModel {
    BarGroup group;
    std::vector<BarControl> controls;
    bool pushed = false;      // the room before it is taken up (End always; a spread bar's Media)
};

// The bar's groups. End holds More alone, pushed to the right edge on every
// platform; Load Preset… and Settings… are in it, so the bar has the same
// controls with or without a menu bar. Mobile keeps what is used while the
// machine runs: the model picker, the module slots and the media of the
// product's mobileBarMedia; the places go to More ▸ Peripherals, the other
// media to their submenus in More. The product's placesOnPaper are not on
// the bar.
std::vector<BarGroupModel> bar(const ShellInputs&, const ShellProduct&, Traits = {});

// The bar below a faceplate: its labels ("Crd", "Preset") only where the
// whole bar fits the faceplate's width with them; then as wide as the
// faceplate, wider only where the controls need more, and never wider than
// the room. The frontend measures the bar with and without its labels, in
// its own units; the room defaults to none set.
struct BarFit {
    double width = 0;
    bool labels = false;
};
BarFit barFit(double faceplate, double withoutLabels, double withLabels,
              double room = std::numeric_limits<double>::infinity());

// The panels the control state lists (the paper panel always),
// each with its owner's peripheral ID or none.
struct Panel {
    std::string kind;    // "paper"
    std::string owner;   // the peripheral ID; empty: none
    std::string ownerPlace;
    // The paper panel: the controls of the product's placesOnPaper, as the
    // bar would have them (a place button or picker).
    std::vector<BarControl> places;
};
std::vector<Panel> panels(const ShellInputs&, const ShellProduct&);

} // namespace calcu::shell
