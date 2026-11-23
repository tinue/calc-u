#pragma once
#include "TIDebugPart.hpp"
#include "control/ControlRules.hpp"
#include "control/ControlState.hpp"
#include "machine/Machine.hpp"
#include "sessions/Value.hpp"
#include <string>
#include <vector>

// Calc-U-59's side of the control state: its product entries, the debug
// panel's (TIDebugPart) and the card controls. The rest of the control
// state is the library's generic entries, from the descriptors.

namespace cu59 {

// The card controls on the bar, the same in every GUI: Arm Card and Arm
// Fresh Card (both "Crd") while no card passes the reader, Eject Card while
// one does. The two Arm controls stand for the File menu's card items and
// follow the card reader's `insert`: while it cannot be used they are shown
// disabled, its reason their tooltip. Eject Card can always be pressed.
// The control state carries them as the product entry `c59.card-controls`
// while it lists the card reader (the TI-59 only).
inline constexpr const char* kCardControlsEntry = "c59.card-controls";
inline constexpr const char* kCardControlEject = "eject-card";

struct CardControl {
    std::string id;      // kItemArmCard, kItemArmFreshCard (the File menu's items), kCardControlEject
    std::string icon;    // drawn before the label: "⤓", "＋", "⏏"
    std::string label;   // the bar's label, left out where the bar fits only without: "Crd", "Eject Card"
    std::string name;    // the accessible name, and the tooltip while available: "Arm Card"
    bool available = true;
    std::string reason;  // the tooltip while not available
    bool operator==(const CardControl&) const = default;
};

// The controls while a card passes (`passing`) or not, Arm following `insert`.
std::vector<CardControl> cardControls(bool passing, const calcu::control::Availability& insert);

// The entry's value: {controls: [{id, icon, label, name, available, reason}]},
// `reason` only where not available.
calcu::sessions::Value toValue(const std::vector<CardControl>&);
// The controls of a control state; empty while it lists no card reader.
std::vector<CardControl> cardControlsOf(const calcu::control::ControlState&);

class TIControlRules final : public calcu::control::ControlRules {
public:
    explicit TIControlRules(const TIDebugPart& debug) : m_debug(debug) {}
    void productEntries(const calcu::machine::Machine&, const calcu::control::ControlState& listed,
                        std::vector<calcu::control::ProductEntry>&) const override;

private:
    const TIDebugPart& m_debug;
};

} // namespace cu59
