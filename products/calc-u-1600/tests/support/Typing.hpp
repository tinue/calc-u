#pragma once
#include <algorithm>
#include <istream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "../../app/SharpPresetRules.hpp"
#include "../../app/SharpTyping.hpp"
#include "../../core/Display/DisplayOutputText.hpp"
#include "machine/Machine.hpp"
#include "presets/ProductRules.hpp"
#include "sessions/MachineSession.hpp"
#include "typing/TypingEngine.hpp"

// The core tests' keys and waits on a machine (a built one or a machine
// class made directly): the library's typing engine with the model's
// typing profile, the machine advanced in pieces cut where the engine is
// due, as the machine session does; the waits are the product's idle rule
// (app/SharpPresetRules), as a preset's steps wait.

namespace sharptest {

using calcu::machine::Machine;
using calcu::machine::Ticks;

inline Ticks frameTicks(const Machine& m) { return m.descriptor().tickRate / 60; }
inline Ticks secondsTicks(const Machine& m, double s) {
    return static_cast<Ticks>(static_cast<double>(m.descriptor().tickRate) * s);
}

// Advances a frame at a time until `pred`; false at the cap.
template <class P> bool runUntil(Machine& m, P pred, double capSeconds) {
    const Ticks cap = secondsTicks(m, capSeconds);
    Ticks spent = 0;
    while (!pred()) {
        if (spent >= cap) return false;
        spent += m.advance(frameTicks(m));
    }
    return true;
}

// Advances a frame at a time until key readiness (as the app's start and
// Reset do); false at the cap.
inline bool runUntilKeyReady(Machine& m, double capSeconds = 30) {
    return runUntil(m, [&m] { return m.isKeyReady(); }, capSeconds);
}

inline const calcu::presets::IdleRule& idleRuleOf(const Machine& m) {
    static const calcu::presets::IdleRule pc1500 = cu1600::SharpModelRules("PC-1500A").idle();
    static const calcu::presets::IdleRule pc1600 = cu1600::SharpModelRules("PC-1600").idle();
    return m.model().value == "PC-1600" ? pc1600 : pc1500;
}

// Settled (the machine waits without a break for the rule's stretch, 20
// frames) or, with `finished`, also with its program ended; false at the
// cap.
inline bool runUntilSettled(Machine& m, double capSeconds = 30, bool finished = false) {
    const calcu::presets::IdleRule& rule = idleRuleOf(m);
    const Ticks cap = secondsTicks(m, capSeconds);
    const Ticks stretch = secondsTicks(m, rule.stretch.count());
    const Ticks piece = std::max<Ticks>(1, std::min(rule.sample, frameTicks(m)));
    Ticks spent = 0, held = 0;
    for (;;) {
        const calcu::presets::IdleState s = calcu::presets::idleState(rule, m);
        held = (finished ? s.ended : s.waits) ? held : 0;
        if (held >= stretch) return true;
        if (spent >= cap) return false;
        const Ticks ran = m.advance(piece);
        spent += ran;
        if (finished ? s.ended : s.waits) held += ran;
    }
}

// The BASIC program has ended: settled at the prompt (a preset's "until the
// program has ended").
inline bool runUntilFinished(Machine& m, double capSeconds = 60) { return runUntilSettled(m, capSeconds, true); }

// Carries out what the engine has queued: the machine advances in pieces
// cut at the engine's due ticks until nothing is down or waiting, then
// the gap after the last press; false
// when it is not done within `capSeconds` or the engine reports otherwise.
inline bool drive(Machine& m, calcu::typing::TypingEngine& e, calcu::typing::Token token, double capSeconds) {
    const Ticks end = m.tickCount() + secondsTicks(m, capSeconds);
    for (;;) {
        const Ticks now = m.tickCount();
        if (e.nextDue(now) <= now) e.act(m, now, {});
        if (e.idle()) break;
        if (now >= end) return false;
        Ticks piece = std::min(end - now, frameTicks(m));
        const Ticks due = e.nextDue(now);
        if (due != calcu::sessions::kNever && due > now) piece = std::min(piece, due - now);
        m.advance(std::max<Ticks>(1, piece));
    }
    // The gap after the last release, as before the engine's next press: a
    // key that follows (another call, or a raw key-down) keeps the cadence.
    if (e.profile()) m.advance(e.profile()->typed.gap);
    const auto r = e.takeResult(token);
    return r && r->done;
}

inline const calcu::typing::TypingProfile& profileOf(const Machine& m) { return *cu1600::typingProfile(m.model()); }

// Presses the keys in turn (key IDs), each as a typed press of the model's
// profile.
inline bool typeKeys(Machine& m, const std::vector<std::string>& ids, double capSeconds = 30) {
    calcu::typing::TypingEngine e;
    e.setMachine(&m, &profileOf(m), nullptr);
    std::vector<calcu::typing::Press> presses;
    for (const std::string& id : ids) presses.push_back(calcu::typing::Press{calcu::machine::KeyId{id}});
    const calcu::typing::Queued q = e.typeKeys(std::move(presses), calcu::typing::Origin::Typed);
    return !q.refused && drive(m, e, q.token, capSeconds);
}

inline bool tap(Machine& m, const std::string& id) { return typeKeys(m, {id}); }

// Types `text` (and ENTER) as the `type` command does; a character without
// a key refuses it all (`error` says which).
inline bool typeText(Machine& m, std::string_view text, bool enter = true, std::string* error = nullptr,
                     double capSeconds = 60) {
    calcu::typing::TypingEngine e;
    e.setMachine(&m, &profileOf(m), nullptr);
    const calcu::typing::Queued q = e.typeText(text, enter, /*refuseUnknown=*/true, calcu::typing::Origin::Typed);
    if (q.refused) {
        if (error) *error = *q.refused;
        return false;
    }
    return drive(m, e, q.token, capSeconds);
}

// Types `text` and ENTER, then waits until settled, at most `settleSeconds`
// (a line that starts a program that goes on returns at the cap).
inline bool typeLine(Machine& m, std::string_view text, double settleSeconds = 5) {
    if (!typeText(m, text, true)) return false;
    runUntilSettled(m, settleSeconds);
    return true;
}

// A cold boot, then on to key readiness (as the app's start); a PC-1500
// whose ROM asks `NEW0? :CHECK` gets CL, at most five times, as a preset's
// build does.
inline void bootToPrompt(Machine& m) {
    m.coldBoot();
    runUntilKeyReady(m);
    if (m.model().value == "PC-1600") return;
    for (int i = 0; i < 5 && displayText(m).text().find("NEW0") != std::string::npos; ++i) {
        tap(m, "cl");
        runUntilSettled(m, 5);
    }
}

// Types a listing in through the ROM's editor, a line at a time (an empty
// line left out), each settled before the next; the machine must be in
// program mode. False when a line could not be typed.
inline bool typeProgram(Machine& m, const std::string& listing) {
    std::istringstream in(listing);
    std::string line;
    while (std::getline(in, line)) {
        while (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty()) continue;
        if (!typeLine(m, line, 6)) return false;
    }
    return true;
}

} // namespace sharptest
