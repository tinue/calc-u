#pragma once
#include <optional>
#include <string>
#include <vector>

#include "machine/Ids.hpp"
#include "typing/KeyboardMap.hpp"
#include "typing/TypingPart.hpp"
#include "typing/TypingProfile.hpp"

// Typing on the Sharp machines (LIB-5 design §4; spec AC-14, AC-28 to AC-31,
// AC-34, AC-41): the typing profile and the keyboard map per model, which
// the library's typing engine (calcu::typing) reads. The character tables
// stay in the core (SharpShiftedSymbols.hpp, PC1500TypedInput.hpp,
// PC1600TypedInput.hpp); this reads them.
//
//                     PC-1500, PC-1500A           PC-1600
//   minimum, typed    4 frames; 4 down, 4 up      the same, + 6 after shift and kbii
//   a press that      waits for key readiness     follows the cadence (D-6)
//   waits
//   first of a step   as one that waits           at key readiness, 15 s at most (AC-14)
//   characters        AC-30                       AC-30 and KBII by the latches (AC-34)
//   names             key IDs, `break` for `on`, any case; `ent` refused (AC-41)
//
// A frame is 1/60 s of emulated time (n × rate / 60 ticks).

namespace cu1600 {

// The profile of a model ("PC-1500", "PC-1500A", "PC-1600"), or nullptr.
const calcu::typing::TypingProfile* typingProfile(const calcu::machine::ModelId&);

// The keyboard map of a model (AC-29; the rules of 0.8.1's resolveHostKey):
// the named keys with their Shift forms on the PC-1600, the characters of
// AC-30; with the host's command modifiers nothing; Option or AltGr type.
// A single key is held (AC-31); a key that needs shift or KBII is a sequence.
class SharpKeyboardMap final : public calcu::typing::KeyboardMap {
public:
    explicit SharpKeyboardMap(bool pc1600) : m_pc1600(pc1600) {}
    std::optional<calcu::typing::Mapped> map(const calcu::typing::HostKeyEvent&) const override;

private:
    bool m_pc1600;
};

const SharpKeyboardMap* keyboardMapOf(const calcu::machine::ModelId&);

// What TypingPart gets (LIB-5 §3.5): the profiles and maps of the three models.
const calcu::typing::TypingProduct& typingProduct();

// The presses of a typed character on the model (AC-30), as the profile
// gives them with every latch off; nullopt: no key (tests, the Qt input method).
std::optional<std::vector<calcu::typing::Press>> charPresses(const calcu::machine::ModelId&, char32_t);

} // namespace cu1600
