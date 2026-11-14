#include "Pacing.hpp"

namespace cu59 {

BatchResult runBatch(calcu::machine::Machine& m, int32_t& carry, int32_t targetTicks, FreezeController& freeze) {
    BatchResult r;
    TIDebugTargetAdapter& debug = *debugTargetOf(m);
    TIPacingHints& hints = *pacingHintsOf(m);
    hints.takePolledHardware();   // this batch's instructions only
    if (carry < targetTicks) carry += static_cast<int32_t>(m.advance(static_cast<calcu::machine::Ticks>(targetTicks - carry)));
    r.polledHardware = hints.takePolledHardware();
    switch (debug.takeStop()) {
    case TIStop::Breakpoint:
        r.stop = BatchResult::Stop::Breakpoint;
        return r;
    case TIStop::Observer:
        r.stop = BatchResult::Stop::Freeze;
        r.freezeOwner = freeze.takeFired().value_or(FreezeOwner::Cpu);
        return r;
    case TIStop::None:
        break;
    }
    // Subtract rather than reset: the overshoot carries into the next batch.
    carry -= targetTicks;
    return r;
}

bool shouldSleep(bool fullSpeed, bool inIdleAndKeyDetect, bool polledHardware) {
    if (!fullSpeed) return true;
    return inIdleAndKeyDetect || polledHardware;
}

} // namespace cu59
