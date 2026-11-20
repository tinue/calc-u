#pragma once
#include "machine/EmulatedTime.hpp"
#include "machine/Ids.hpp"
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

// The typing profile of a model: holds, gaps, when a press may start, the keys
// of a character and the names `key` takes. Every duration is in the
// machine's ticks: the product builds a profile per model at its tick rate.

namespace calcu::typing {

class TypingSupport;

struct Press {
    machine::KeyId key;
    bool mustFinish = false;   // typed even when live input drops what waits (the closing KBII)
    bool operator==(const Press&) const = default;
};

// A press of typed text or of a sequence: held `hold`, then `gap` before
// the next press may start.
struct Cadence {
    machine::Ticks hold = 0;
    machine::Ticks gap = 0;
};

// How the first press of a step starts: as any press that waits
// (by queuedWaitsForReadiness), at once, or at key readiness (with the
// profile's bound).
enum class StartRule { AsQueued, AtOnce, KeyReadiness };

// A TI press stays down until the ROM's key scan has found it `samplesNeeded`
// more times, at most `maxHold`, as a user holds R/S until it is taken.
struct SampleRule {
    std::uint32_t samplesNeeded = 2;
    machine::Ticks maxHold = 0;
};

struct TypingProfile {
    machine::Ticks minHold = 0;   // every press of the engine; an earlier key-up waits for it
    Cadence typed;                // typed text, sequences, `key`
    // PC-1600: 6 frames more after `shift` and `kbii`; none when unset.
    std::function<machine::Ticks(const machine::KeyId&)> extraGapAfter;
    bool queuedWaitsForReadiness = false;         // a queued press waits for key readiness
    bool waitsForReadinessAfterRelease = false;   // not idle until the machine is ready again
    StartRule firstPressOfStep = StartRule::AsQueued;
    std::optional<machine::Ticks> firstPressBound;      // PC-1600 15 s; none: waits without limit
    machine::Ticks readinessPoll = 1;                   // how often a waiting press looks again
    std::optional<SampleRule> holdUntilSampled;         // TI only
    // Keys the ROM reads outside its key wait (BREAK, the TI's R/S): once at
    // the front, such a press of a command (`key`, not a preset's item) waits
    // for key readiness at most doesNotWaitAfter, then goes down all the same.
    std::function<bool(const machine::KeyId&)> doesNotWait;
    machine::Ticks doesNotWaitAfter = 0;

    // Typed text: a character → its presses, decided
    // when its first press is due, with the machine's typing support
    // (latches). nullopt: the model has no key for it. With no typing
    // support (nullptr) it answers as for every latch off, and the answer
    // to "has a key" never depends on the latches (a preset is refused at read for exactly
    // the characters that cannot be typed). Unset: the model
    // types no characters.
    std::function<std::optional<std::vector<Press>>(char32_t, const TypingSupport*)> charKeys;
    std::optional<machine::KeyId> enter;   // `type` presses it; none: no `type`
    // `key` names: a name → its presses; nullopt:
    // refused. Unset: the key IDs of the model as written.
    std::function<std::optional<std::vector<Press>>(std::string_view)> name;
    // A `key` of several names, when the names act on each other (a
    // `2nd` written before a second function is pressed once): the presses
    // of them all; nullopt: refused. Unset: name by name.
    std::function<std::optional<std::vector<Press>>(const std::vector<std::string>&)> names;

    // The gap after a press of `key`: typed.gap and the extra one.
    machine::Ticks gapAfter(const machine::KeyId& key) const {
        return typed.gap + (extraGapAfter ? extraGapAfter(key) : 0);
    }
};

// Seconds at `rate`, to the nearest tick.
machine::Ticks ticksOf(double seconds, machine::TickRate rate);

} // namespace calcu::typing
