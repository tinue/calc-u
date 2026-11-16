// Headless PC-1600 + CE-1600P plotter probe. Boots a `plotter: CE-1600P`
// preset, runs its script, and dumps every pen/motor/colour event the
// AlpsPlotterMechanism logged plus the final turret colour -- so the
// colour-turret behaviour (a `COLOR n` command, the power-on home spin)
// can be inspected without the GUI.
//
// The machine is built through the machine interface (C1600-1 design §7);
// the paper is the CE-1600P's paper-output, the preset goes through the
// library's loader (adapter CliPresetRun, until M-51). The events are the
// mechanism's log once the load has ended.
//
// Usage: pc1600_plotter_probe <preset.pc1600> [maxSettleCycles]
//
// ROM set is read from roms/ (same repo-root convention as tools/run_tests.sh).

#include <cstdio>
#include <string>
#include <vector>

#include "../core/Connector/CE1600PCard.hpp"
#include "../core/Machine/LegacyCalls.hpp"
#include "CliCommon.hpp"
#include "CliPresetRun.hpp"

int main(int argc, char** argv) {
    if (argc < 2) {
        std::fprintf(stderr, "usage: %s <preset.pc1600>\n", argv[0]);
        return 1;
    }
    std::string err;
    std::unique_ptr<cli::CliPresetRun> run = cli::CliPresetRun::load(argv[1], &err);
    if (!run || !run->machine()) {
        std::fprintf(stderr, "preset apply failed: %s\n", err.c_str());
        return 1;
    }
    cli::Machine& machine = *run->machine();

    CE1600PCard* card = sharp::legacy::coreParts(machine).ce1600p;
    if (!card || !cli::ofPeripheral<calcu::paper::PaperOutput>(machine, sharp::kCE1600P)) {
        std::fprintf(stderr, "no CE-1600P attached by this preset\n");
        return 1;
    }
    auto events = card->mechanism().drainEvents();
    std::printf("CE-1600P events (%zu):\n", events.size());
    for (const auto& e : events) std::printf("  %s\n", e.c_str());
    auto pts = cli::plotPointsOf(machine, sharp::kCE1600P);
    std::printf("plot points=%zu\n", pts.size());
    if (!pts.empty()) {
        std::printf("first point colour index=%d, last point colour index=%d\n",
                    (int)pts.front().color, (int)pts.back().color);
    }
    return 0;
}
