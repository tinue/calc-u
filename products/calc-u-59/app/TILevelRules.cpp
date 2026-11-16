#include "TILevelRules.hpp"
#include "../core/TIDebugging.hpp"
#include <utility>

namespace cu59 {

using calcu::debugger::kCpu;
using calcu::debugger::kProgram;
using calcu::debugger::LevelName;
using calcu::machine::InstructionEvent;

bool TILevelRules::pauseLandsAtOnce(const LevelName&) const { return std::exchange(m_landAtOnce, false); }

bool TILevelRules::boundaryOrCount(const InstructionEvent& ev) {
    m_count += timarks::markWeight(ev.marks);   // 1 active, 4 IDLE
    if ((ev.marks & timarks::kMarkBoundary) || m_count >= kMaxWeights) {
        m_count = 0;
        return true;
    }
    return false;
}

bool TILevelRules::pauseLandsAfter(const LevelName&, const InstructionEvent& ev) { return boundaryOrCount(ev); }

bool TILevelRules::programStepDoneAfter(const InstructionEvent& ev) { return boundaryOrCount(ev); }

void TILevelRules::startArmed(const LevelName& level, calcu::machine::Machine& m) {
    calcu::machine::Debugging* d = m.debugging();
    const std::uint64_t at = d ? d->target(0).nextAddress() : 0;
    // The run control arms again at a Reset or Cold boot, where the processor
    // stands at 0000: what was seen before stays, so that the start-up fires
    // a `cpu` start as 2.3.0's FreezeController did (row 59-34).
    if (m_armed && *m_armed == level && at == 0) return;
    m_armed = level;
    m_seenLoop = false;
    m_count = 0;
    m_armAddress = at;
}

bool TILevelRules::startFiresAfter(const LevelName& level, const InstructionEvent& ev, const calcu::machine::Machine&) {
    bool fires = false;
    if (level == kProgram) {
        fires = (ev.marks & timarks::kMarkBoundary) != 0;
    } else if (level == kCpu) {
        if (ev.marks & timarks::kMarkScanLoop) {
            m_seenLoop = true;
            return false;
        }
        fires = m_seenLoop || (ev.nextAddress == 0 && m_armAddress != 0);
    }
    if (fires) {
        m_count = 0;
        m_seenLoop = false;
        m_armed.reset();
    }
    return fires;
}

} // namespace cu59
