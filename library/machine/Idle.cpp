#include "machine/Idle.hpp"

namespace calcu::machine {

IdleState idleState(const Machine& m) {
    IdleState s;
    s.takesKeys = m.isKeyReady();
    s.busy = m.busyWith();
    s.waits = (s.takesKeys || m.awaitsMedium()) && s.busy.empty();
    s.ended = s.waits && m.programEnded();   // asked only when it waits
    return s;
}

} // namespace calcu::machine
