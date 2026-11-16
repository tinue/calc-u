#pragma once
#include <cstdint>

#include "machine/Machine.hpp"
#include "sessions/Operation.hpp"

// ── The tests' machine jobs (was core/Job.hpp, the adapter JobForward) ──
//
// The old typers (PC1500BasicTyper, PC1600BasicTyper, BootRun.hpp) are the
// tests' support, not the core's: the golden runs' and C1600-2 AC-7's
// reference typer. `namespace job` names calcu::sessions' Task, Job and
// runToCompletion, so a job of theirs runs as the library's run requests --
// the same thread-local hand-off, never split.
//
// Rules for writing one (the library's, `sessions/Operation.hpp`):
//  - Every function that (transitively) runs the machine is a coroutine
//    returning Task<T>, and awaits the ones it calls.
//  - Coroutine parameters are taken by value, except the machine reference
//    and out-pointers: a Task starts lazily, after the full expression that
//    created it -- a reference to a temporary argument would dangle.
//  - Single-threaded: one job runs at a time per thread.
namespace job {

using calcu::sessions::Job;
using calcu::sessions::Task;
using calcu::sessions::runToCompletion;

// The awaitable form of `machine.runCycles(cycles)` on an old machine class.
template <class Machine>
calcu::sessions::detail::RunAwaiter run(Machine& machine, std::uint64_t cycles) {
    return calcu::sessions::request([&machine, cycles] { return machine.runCycles(cycles); });
}

// The same on the machine interface (C1600-1 design §2): `machine.advance(ticks)`.
inline calcu::sessions::detail::RunAwaiter run(calcu::machine::Machine& machine, std::uint64_t ticks) {
    return calcu::sessions::request([&machine, ticks] { return machine.advance(ticks); });
}

} // namespace job
