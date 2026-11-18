#pragma once
#include <optional>
#include <string>
#include <vector>

#include "machine/Ids.hpp"
#include "typing/KeyboardMap.hpp"
#include "typing/TypingPart.hpp"
#include "typing/TypingProfile.hpp"

// Typing on the Sharp machines: the typing profile and the keyboard map per
// model, which the library's typing engine (calcu::typing) reads. The character tables
// stay in the core (SharpShiftedSymbols.hpp, PC1500TypedInput.hpp,
// PC1600TypedInput.hpp); this reads them.
//
//                     PC-1500, PC-1500A           PC-1600
//   minimum, typed    4 frames; 4 down, 4 up      the same, + 6 after shift and kbii
//   a press that      waits for key readiness     follows the cadence, as the PC-1600's
//   waits                                         paste feeder always did
//   first of a step   as one that waits           at key readiness, 15 s at most
//   characters        letters, digits, `+ - = * / ( ) .`, space; second legends with shift;
//                                                 PC-1600: digit-row legends with shift, accents
//                                                 with KBII by the latches
//   names             key IDs, `break` for `on`, any case; `ent` refused
//
// A frame is 1/60 s of emulated time (n × rate / 60 ticks).

namespace cu1600 {

// The profile of a model ("PC-1500", "PC-1500A", "PC-1600"), or nullptr.
const calcu::typing::TypingProfile* typingProfile(const calcu::machine::ModelId&);

// The keyboard map of a model: the named keys with their Shift forms on
// the PC-1600, the typed characters; with the host's command modifiers
// nothing; Option or AltGr type.
// A single key is held; a key that needs shift or KBII is a sequence.
class SharpKeyboardMap final : public calcu::typing::KeyboardMap {
public:
    explicit SharpKeyboardMap(bool pc1600) : m_pc1600(pc1600) {}
    std::optional<calcu::typing::Mapped> map(const calcu::typing::HostKeyEvent&) const override;

private:
    bool m_pc1600;
};

const SharpKeyboardMap* keyboardMapOf(const calcu::machine::ModelId&);

// What TypingPart gets: the profiles and maps of the three models.
const calcu::typing::TypingProduct& typingProduct();

// The presses of a typed character on the model, as the profile
// gives them with every latch off; nullopt: no key (tests, the Qt input method).
std::optional<std::vector<calcu::typing::Press>> charPresses(const calcu::machine::ModelId&, char32_t);

} // namespace cu1600
