// Headless PC-1600 + CE-1600P plotter probe. Boots a `plotter: CE-1600P`
// preset, runs its script, and dumps every pen/motor/colour event the
// AlpsPlotterMechanism logged plus the final turret colour -- so the
// colour-turret behaviour (a `COLOR n` command, the power-on home spin)
// can be inspected without the GUI.
//
// The machine is built through the machine interface (C1600-1 design §7);
// the paper is the CE-1600P's c1600.plotter-paper, the preset goes through
// sharp::legacy::applyPresetJob until M-46.
//
// Usage: pc1600_plotter_probe <preset.pc1600> [maxSettleCycles]
//
// ROM set is read from roms/ (same repo-root convention as tools/run_tests.sh).

#include <cstdio>
#include <string>
#include <vector>

#include "../core/Connector/CE1600PCard.hpp"
#include "../core/Machine/LegacyCalls.hpp"
#include "../core/Preset/PresetFile.hpp"
#include "CliCommon.hpp"

int main(int argc, char** argv) {
    if (argc < 2) {
        std::fprintf(stderr, "usage: %s <preset.pc1600>\n", argv[0]);
        return 1;
    }
    PresetFile preset;
    std::string err;
    if (!parsePresetFile(argv[1], &preset, &err)) {
        std::fprintf(stderr, "parse failed: %s\n", err.c_str());
        return 1;
    }
    std::string romErr;
    std::unique_ptr<cli::Machine> built = cli::buildMachine(sharp::kPC1600, "New", {"roms"}, &romErr);
    if (!built) {
        std::fprintf(stderr, "could not load roms/ ROM set (run from repo root): %s\n", romErr.c_str());
        return 1;
    }
    cli::Machine& machine = *built;
    auto paper = [&machine] { return cli::ofPeripheral<sharp::PlotterPaper>(machine, sharp::kCE1600P); };
    PresetLoadResult res = job::runToCompletion(sharp::legacy::applyPresetJob(
        machine, preset,
        [&machine](const std::string& l) {
            std::fprintf(stderr, "[preset] %s\n", l.c_str());
            // During the load the preset's plotter is on the core only (the
            // interface lists it when the load ends): its card directly.
            if (CE1600PCard* card = sharp::legacy::coreParts(machine).ce1600p)
                for (const auto& e : card->mechanism().drainEvents()) std::fprintf(stderr, "    <evt> %s\n", e.c_str());
        },
        ".", ".", {}, {"roms"}, {}, {}, {}));
    if (!res.ok) {
        std::fprintf(stderr, "preset apply failed: %s\n", res.error.c_str());
        return 1;
    }

    sharp::PlotterPaper* p = paper();
    if (!p) {
        std::fprintf(stderr, "no CE-1600P attached by this preset\n");
        return 1;
    }
    auto events = p->drainEvents();
    std::printf("CE-1600P events (%zu):\n", events.size());
    for (const auto& e : events) std::printf("  %s\n", e.c_str());
    auto pts = p->points();
    std::printf("plot points=%zu\n", pts.size());
    if (!pts.empty()) {
        std::printf("first point colour index=%d, last point colour index=%d\n",
                    (int)pts.front().color, (int)pts.back().color);
    }
    return 0;
}
