#include "PacingProbe.hpp"
#include "qt6/PacingProbe.hpp"
#include "../MainWindow.hpp"

#include <QApplication>
#include <cmath>
#include <cstdio>

int runPacingProbe(QApplication& app, MainWindow& window, std::uint16_t attachPort, double seconds) {
    return calcu::qt6::runPacingProbe(app, attachPort, seconds, [&window](const calcu::qt6::PacingSample& s) {
        const double w = s.wall;
        const double e = s.emulated();
        const int result = s.secondsBefore >= 0 && std::fabs(e - w) <= 0.025 ? 0 : 1;
        MachineController* c = window.controller();
        std::printf("pacing-probe: wall %.3f s, emulated %.3f s, difference %+.4f s, %llu host calls, "
                    "longest gap %.1f ms: %s\n",
                    w, e, e - w, static_cast<unsigned long long>(c->calls()), c->longestGapMs(),
                    result == 0 ? "ok" : "beyond the bound");
        return result;
    });
}
