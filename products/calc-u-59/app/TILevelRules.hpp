#pragma once
#include "debugger/Levels.hpp"
#include <cstdint>
#include <optional>

// The levels of the TI machines: `cpu` and `program`. The rules read InstructionEvent::marks of
// core/TIDebugging.hpp (step boundary, scan loop, weight) and never call
// the machine during an advance.
//
// - A pause of either level lands at the next instruction marked as a
//   program step boundary, or once the weights counted since it was asked
//   reach 50000 (the calculator waits for a key, or an ASM overlay runs,
//   which has no boundaries).
// - One `program` step: the same rule. One `cpu` step: one instruction,
//   carried out again up to 32 times while the processor stays at its
//   address (a HOLD).
// - Pause-on-start `program` fires at the next boundary; `cpu` fires on the
//   first instruction after which the scan-loop mark is clear, once it was
//   seen set since arming, or when the next address is 0000 while it was
//   not 0000 at arming. No start fires at a
//   restart itself: the ROM's start-up after a Reset or a Cold boot runs in
//   the advances that follow, where the 0000 rule fires.
//
// landNextPauseAtOnce(): the next pause lands where the machine stands,
// without a step (the ASM overlay's entry with CPU FREEZE ON START armed).

namespace cu59 {

class TILevelRules final : public calcu::debugger::LevelRules {
public:
    static constexpr std::uint32_t kMaxWeights = 50000;
    static constexpr int kHoldRepeats = 32;

    bool hasProgramLevel() const override { return true; }
    bool pauseLandsAtOnce(const calcu::debugger::LevelName&) const override;
    bool pauseLandsAfter(const calcu::debugger::LevelName&, const calcu::machine::InstructionEvent&) override;
    void pauseAsked(const calcu::debugger::LevelName&, calcu::machine::Machine&) override { m_count = 0; }
    bool programStepDoneAfter(const calcu::machine::InstructionEvent&) override;
    int cpuStepRepeats() const override { return kHoldRepeats; }
    void startArmed(const calcu::debugger::LevelName&, calcu::machine::Machine&) override;
    bool startFiresAfter(const calcu::debugger::LevelName&, const calcu::machine::InstructionEvent&,
                         const calcu::machine::Machine&) override;
    bool startFiresAtRestart(const calcu::debugger::LevelName&, calcu::debugger::RestartKind) const override {
        return false;
    }

    void landNextPauseAtOnce() { m_landAtOnce = true; }

private:
    bool boundaryOrCount(const calcu::machine::InstructionEvent&);

    std::uint32_t m_count = 0;
    bool m_seenLoop = false;
    std::uint64_t m_armAddress = 0;
    std::optional<calcu::debugger::LevelName> m_armed;
    mutable bool m_landAtOnce = false;
};

} // namespace cu59
