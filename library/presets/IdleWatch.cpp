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

IdleState idleState(const IdleRule& rule, const machine::Machine& m) {
    IdleState s;
    s.takesKeys = rule.takesKeys ? rule.takesKeys(m) : m.isKeyReady();
    if (rule.busy) s.busy = rule.busy(m);
    const bool awaits = rule.awaitsMedium && rule.awaitsMedium(m);
    s.waits = (s.takesKeys || awaits) && s.busy.empty();
    s.ended = s.waits && (!rule.programEnded || rule.programEnded(m));
    return s;
}

sessions::Task<IdleOutcome> IdleWatch::wait(sessions::OperationContext& op, std::optional<Ticks> bound) {
    return watch(op, bound, false);
}

sessions::Task<IdleOutcome> IdleWatch::waitFinished(sessions::OperationContext& op, std::optional<Ticks> bound) {
    return watch(op, bound, true);
}

sessions::Task<IdleOutcome> IdleWatch::watch(sessions::OperationContext& op, std::optional<Ticks> bound, bool finished) {
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
        const IdleState st = idleState(rule_, *m);
        const bool ready = finished ? st.ended : st.waits;
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
