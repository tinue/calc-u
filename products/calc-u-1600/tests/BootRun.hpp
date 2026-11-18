#pragma once
#include "support/Job.hpp"
#include "../core/Machine/SharpCores.hpp"
#include "support/PC1500BasicTyper.hpp"
#include "support/PC1600BasicTyper.hpp"
#include "machine/Machine.hpp"

// The old typers' boot run, kept for the tests: a power cycle under way
// runs to its end, a phase at a time, then the model's boot run to the
// prompt (runBootToPromptJob of both typers). The app and the harness run
// to key readiness instead; the golden runs keep this run so that they
// measure the cores and not the readiness wait.

namespace sharptest {

inline job::Task<void> bootRunJob(calcu::machine::Machine& m) {
    sharp::SharpMachine* s = sharp::sharpOf(m);
    if (!s) co_return;
    // One advance per phase: each phase is one run of the core, as 0.8.1.
    while (s->powerCycleActive()) co_await job::run(m, s->powerCyclePhaseTicks());
    if (PC1600Machine* p = sharp::pc1600Of(m)) co_await runBootToPromptJob(*p);
    else if (PC1500Machine* p = sharp::pc1500Of(m)) co_await runBootToPromptJob(*p);
}

inline void bootRun(calcu::machine::Machine& m) { job::runToCompletion(bootRunJob(m)); }

} // namespace sharptest
