#pragma once
#include "MachineModelInfo.hpp"
#include "typing/KeyboardMap.hpp"
#include "typing/TypingPart.hpp"
#include "typing/TypingProfile.hpp"
#include <optional>
#include <string>
#include <string_view>
#include <vector>

// Typing on the TI models (LIB-5 design §4; spec AC-20, AC-21, AC-26): the
// typing profile per model, the keyboard map, and the product the library's
// TypingPart is given (cu59::typingProduct()). Replaces 2.3.0's KeySequencer
// (the holds) and KeyboardMap (the 26 bindings).

namespace cu59 {

/// §3.4, D-3 (task T59-0, tests/typing_tests.cpp lib5_tr_sample_rule_measurement):
/// a TI press stays down until the ROM's key scan has found it this many
/// more times, at most kTIMaxHoldSeconds. Measured on `Lbl A Prt GTO A`
/// (2026-11-15): 0 (the 100 ms alone) misses R/S in 12 of 24 phase × model
/// × speed cases, 1 stops it in all 24 with holds of 100-266 ms, 2 in all
/// 24 with holds up to 596 ms; at the idle loop 1 keeps the 100 ms
/// and Adv feeds two lines, as the 100 ms alone.
inline constexpr std::uint32_t kTISamplesNeeded = 1;
inline constexpr double kTIMaxHoldSeconds = 1.0;
inline constexpr double kTIMinHoldSeconds = 0.100;     ///< AC-20
inline constexpr double kTIReadinessPoll = 0.0015;     ///< D-7: 2.3.0's kScanPoll

/// AC-20: minimum hold 100 ms of emulated time, presses that wait at key
/// readiness (and again after a release), held until sampled; no characters,
/// no `type`; the names of docs/formats/preset.md §8.1 for `key` (AC-26).
const calcu::typing::TypingProfile& typingProfile(Model);

/// AC-26: the presses of one `key` name (§8.1, §8.1.1), matched without
/// regard to case; nullopt for any other word.
std::optional<std::vector<calcu::typing::Press>> tiKeyName(std::string_view);
/// The names of one `key` in turn; a `2nd` written before a second function
/// is pressed once.
std::optional<std::vector<calcu::typing::Press>> tiKeyNames(const std::vector<std::string>&);

/// AC-21: the 26 keys of 2.3.0's keyboard map by its rules: a letter by the
/// letter the key carries (Shift: `2nd`, then the letter's key), anything
/// else by the character the layout produced; nothing with Command,
/// Control or Option.
class TIKeyboardMap final : public calcu::typing::KeyboardMap {
public:
    std::optional<calcu::typing::Mapped> map(const calcu::typing::HostKeyEvent&) const override;
};

/// What makeAppSession and the harness hand the library's TypingPart.
const calcu::typing::TypingProduct& typingProduct();

} // namespace cu59
