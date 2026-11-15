#include "presets/IdleWatch.hpp"
#include "sessions/MachineSession.hpp"
#include "sessions/SessionPart.hpp"
#include <algorithm>
#include <chrono>

namespace calcu::presets {

using machine::Ticks;

sessions::Task<bool> untilRunning(sessions::OperationContext& op) {
    while (op.session().machineSession().paused()) {
        const bool waited = co_await op.hostWait(std::chrono::milliseconds(1));
        if (!waited) co_return false;
    }
    co_return true;
}

sessions::Task<IdleOutcome> IdleWatch::wait(sessions::OperationContext& op, std::optional<Ticks> bound) {
    IdleOutcome out;
    const machine::Machine* m = op.session().machine();
    if (!m) {
        out.noMachine = true;
        co_return out;
    }
    const machine::TickRate rate = m->descriptor().tickRate;
    const Ticks stretch = machine::EmulatedTime::fromSeconds(rule_.stretch, rate).ticks();
    const Ticks piece = std::max<Ticks>(1, std::min(rule_.sample, sessions::OperationContext::runPiece(rate)));
    const Ticks start = m->tickCount();
    std::optional<Ticks> readySince;
    for (;;) {
        if (op.session().machineSession().paused()) {
            const bool runs = co_await untilRunning(op);
            if (!runs) {
                out.paused = true;
                break;
            }
        }
        m = op.session().machine();
        if (!m) {
            out.noMachine = true;
            break;
        }
        const Ticks now = m->tickCount();
        out.waited = now - start;
        const bool ready = rule_.readyNow ? rule_.readyNow(*m) : m->isKeyReady();
        if (!ready) readySince.reset();
        else if (!readySince) readySince = now;
        if (readySince && now - *readySince >= stretch) {
            out.idle = true;
            break;
        }
        if (bound && out.waited >= *bound) {
            out.boundReached = true;
            break;
        }
        co_await op.run(piece);
    }
    co_return out;
}

} // namespace calcu::presets
