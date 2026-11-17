#pragma once
#include <cstdint>
#include <functional>

class QApplication;

// The measurement of the products' `--pacing-probe <s>` (LIB-17 AC-19,
// AC-22; LIB-14 design §10.7; Part E simplification E-4): an attach client
// in the app's own process (AttachLineClient). Once the start is over (the
// bare `wait`), `ticks` through attach, `s` seconds of wall time, `ticks`
// again. What is printed, the bound and the exit code are the product's
// (`report`); attach not open is "pacing-probe: attach is not open on port
// <n>" and 1. It steps the machine only through attach. Only in builds with
// CALCU_ATTACH.

namespace calcu::qt6 {

struct PacingSample {
    unsigned long long ticksBefore = 0, ticksAfter = 0;   // `ticks:` of the two replies; 0: none
    double secondsBefore = -1, secondsAfter = -1;          // `seconds:`; -1: none
    double wall = 0;                                       // the wall time between them, in seconds
    double emulated() const { return secondsAfter - secondsBefore; }
};

// Runs the app's event loop until the sample is taken; the exit code is
// what `report` returns.
int runPacingProbe(QApplication& app, std::uint16_t attachPort, double seconds,
                   const std::function<int(const PacingSample&)>& report);

} // namespace calcu::qt6
