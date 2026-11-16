#pragma once
#include "../core/Job.hpp"
#include "../core/Machine/SharpCores.hpp"
#include "../core/PC1500/PC1500BasicTyper.hpp"
#include "../core/PC1600/PC1600BasicTyper.hpp"
#include "machine/Machine.hpp"

// 0.8.1's boot run, kept for the tests (LIB-18 design §10.8, D-24): a power
// cycle under way runs to its end, a phase at a time, then the model's boot
// run to the prompt (runBootToPromptJob of both typers). The app and the
// harness run to key readiness instead (LIB-18 AC-13); the golden runs keep
// this run so that they measure the cores and not LIB-18's readiness wait.
// Was sharp::legacy::runToPrompt until M-46.

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
