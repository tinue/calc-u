#include "sessions/Operation.hpp"
#include "sessions/LibraryTopics.hpp"
#include "sessions/MachineSession.hpp"
#include "sessions/SessionPart.hpp"
#include <algorithm>
#include <stdexcept>

namespace calcu::sessions {

using machine::Ticks;

Ticks OperationContext::runPiece(machine::TickRate rate) { return std::max<Ticks>(1, rate / 60); }


Task<bool> OperationContext::hostWait(pacing::WallTime d) {
    if (session_->virtualTime()) co_return false;
    const pacing::WallTime end = session_->wallTime() + d;
    do {   // at least one host call: a loop of short waits never holds the host
        co_await yieldToHost();
    } while (session_->wallTime() < end);
    co_return true;
}

Task<Ticks> OperationContext::run(Ticks ticks) {
    MachineSession& ms = session_->machineSession();
    Ticks total = 0;
    while (total < ticks) {
        const machine::Machine* m = ms.machine();
        if (!m) throw std::runtime_error(kNoMachine);
        if (ms.paused()) {   // a paused machine holds the operation: see below
            co_await yieldToHost();
            co_return total;
        }
        const Ticks piece = std::min<Ticks>(ticks - total, runPiece(m->descriptor().tickRate));
        const Ticks ran = co_await request([&ms, piece] {
            bool stopped = false;
            return ms.advance(piece, stopped);
        });
        if (ran == 0 && ms.machine() && ms.paused()) {
            // The debugger's run control paused the machine (a
            // pause-on-start, a pause-at-start). The run gives the host its
            // call and ends with what ran, so that the caller looks at its
            // own state again (a key that came up on the paused machine); what needs the machine then waits until it runs.
            // In virtual time no call comes: the operation does not finish
            // (its reply is "The operation did not finish.").
            co_await yieldToHost();
            co_return total;
        }
        if (ran == 0) throw std::runtime_error(kNoMachine);
        total += ran;
    }
    co_return total;
}

} // namespace calcu::sessions
