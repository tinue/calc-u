#include "app/SharpMachineParts.hpp"


namespace shared {

using calcu::machine::Machine;
using calcu::machine::Ticks;

// ---- SharpDebuggerGate ----

Ticks SharpDebuggerGate::advance(Machine& m, Ticks atLeast, bool& stoppedEarly) {
    stoppedEarly = false;
    if (!m_debugger) return m.advance(atLeast);
    const Ticks before = m.tickCount();
    if (!m_debugger->runDebuggerSlice(atLeast)) return m.advance(atLeast);
    const Ticks ran = m.tickCount() - before;
    stoppedEarly = ran < atLeast;   // paused or stopped at a breakpoint: the call ends
    return ran;
}

} // namespace shared
