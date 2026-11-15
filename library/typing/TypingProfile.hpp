#pragma once
#include "machine/EmulatedTime.hpp"
#include "machine/Ids.hpp"
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

// The typing profile of a model (LIB-5 design §3.2; spec AC-1, AC-3, AC-11,
// AC-14, AC-15, AC-20, AC-28): holds, gaps, when a press may start, the keys
// of a character and the names `key` takes. Every duration is in the
// machine's ticks: the product builds a profile per model at its tick rate.

namespace calcu::typing {

class TypingSupport;

struct Press {
    machine::KeyId key;
    bool mustFinish = false;   // AC-9: typed even when live input drops what waits (the closing KBII)
    bool operator==(const Press&) const = default;
};

// A press of typed text or of a sequence: held `hold`, then `gap` before
// the next press may start.
struct Cadence {
    machine::Ticks hold = 0;
    machine::Ticks gap = 0;
};

// How the first press of a step starts (AC-14): as any press that waits
// (by queuedWaitsForReadiness), at once, or at key readiness (with the
// profile's bound).
enum class StartRule { AsQueued, AtOnce, KeyReadiness };

// §3.4 (TR, D-3): a TI press stays down until the ROM has read it
// `samplesNeeded` more times, at most `maxHold`.
struct SampleRule {
    std::uint32_t samplesNeeded = 2;
    machine::Ticks maxHold = 0;
};

struct TypingProfile {
    machine::Ticks minHold = 0;   // AC-1: every press of the engine
    Cadence typed;                // AC-11, AC-28: typed text, sequences, `key`
    // PC-1600: 6 frames more after `shift` and `kbii`; none when unset.
    std::function<machine::Ticks(const machine::KeyId&)> extraGapAfter;
    bool queuedWaitsForReadiness = false;         // AC-3, AC-20, AC-31 (D-6)
    bool waitsForReadinessAfterRelease = false;   // AC-20: not idle until the machine is ready again
    StartRule firstPressOfStep = StartRule::AsQueued;   // AC-14
    std::optional<machine::Ticks> firstPressBound;      // AC-14: PC-1600 15 s; EC-3: none
    machine::Ticks readinessPoll = 1;                   // D-7: how often a waiting press looks again
    std::optional<SampleRule> holdUntilSampled;         // §3.4: TI only

    // Typed text (AC-11, AC-13, AC-30): a character → its presses, decided
    // when its first press is due, with the machine's typing support
    // (latches). nullopt: the model has no key for it. With no typing
    // support (nullptr) it answers as for every latch off, and the answer
    // to "has a key" never depends on the latches (D-14). Unset: the model
    // types no characters.
    std::function<std::optional<std::vector<Press>>(char32_t, const TypingSupport*)> charKeys;
    std::optional<machine::KeyId> enter;   // AC-16: `type` presses it; none: no `type` (D-8)
    // `key` names (AC-15, AC-26, AC-41): a name → its presses; nullopt:
    // refused. Unset: the key IDs of the model as written.
    std::function<std::optional<std::vector<Press>>(std::string_view)> name;
    // A `key` of several names, when the names act on each other (AC-26: a
    // `2nd` written before a second function is pressed once): the presses
    // of them all; nullopt: refused. Unset: name by name.
    std::function<std::optional<std::vector<Press>>(const std::vector<std::string>&)> names;

    // The gap after a press of `key`: typed.gap and the extra one.
    machine::Ticks gapAfter(const machine::KeyId& key) const {
        return typed.gap + (extraGapAfter ? extraGapAfter(key) : 0);
    }
};

// n frames (1/60 s) at `rate`: n × rate / 60 (as Calc-U-1600 counts today).
machine::Ticks frames(int n, machine::TickRate rate);
// Seconds at `rate`, to the nearest tick.
machine::Ticks ticksOf(double seconds, machine::TickRate rate);

} // namespace calcu::typing
