#pragma once
#include "MachineModelInfo.hpp"
#include "typing/KeyboardMap.hpp"
#include "typing/TypingPart.hpp"
#include "typing/TypingProfile.hpp"
#include <optional>
#include <string>
#include <string_view>
#include <vector>

// Typing on the TI models: the typing profile per model, the keyboard map,
// and the product the library's TypingPart is given (cu59::typingProduct()).

namespace cu59 {

/// A TI press stays down until the ROM's key scan has found it this many
/// more times, at most kTIMaxHoldSeconds. Measured on `Lbl A Prt GTO A`
/// (sample_rule_measurement in tests/typing_tests.cpp): 0 (the 100 ms alone) misses R/S in 12 of 24 phase × model
/// × speed cases, 1 stops it in all 24 with holds of 100-266 ms, 2 in all
/// 24 with holds up to 596 ms; at the idle loop 1 keeps the 100 ms
/// and Adv feeds two lines, as the 100 ms alone.
inline constexpr std::uint32_t kTISamplesNeeded = 1;
inline constexpr double kTIMaxHoldSeconds = 1.0;
inline constexpr double kTIMinHoldSeconds = 0.100;
/// How long R/S, GTO and RST wait for key readiness before they go down all
/// the same: long beside the few milliseconds the scan loop takes to come
/// back, so a program typed in LRN mode keeps its order.
inline constexpr double kTIDoesNotWaitAfterSeconds = 0.200;
inline constexpr double kTIReadinessPoll = 0.0015;     ///< how often a wait at key readiness looks again

/// Minimum hold 100 ms of emulated time, presses that wait at key
/// readiness (and again after a release), held until sampled; no characters,
/// no `type`; the names of docs/formats/preset.md §8.1 for `key`.
const calcu::typing::TypingProfile& typingProfile(const ModelId&);

/// The presses of one `key` name (§8.1, §8.1.1), matched without
/// regard to case; nullopt for any other word.
std::optional<std::vector<calcu::typing::Press>> tiKeyName(std::string_view);
/// The names of one `key` in turn; a `2nd` written before a second function
/// is pressed once.
std::optional<std::vector<calcu::typing::Press>> tiKeyNames(const std::vector<std::string>&);

/// The keyboard map's 26 keys by these rules: a letter by the
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
