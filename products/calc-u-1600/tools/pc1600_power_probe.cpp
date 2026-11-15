// Throw-away probe: trace the ROM path the PC-1600 takes when the OFF key
// is pressed.
//
// Boots the full dual-CPU PC-1600 on the confirmed ROM set, built through
// the machine interface (C1600-1 design §7), runs to the post-boot idle
// loop, presses OFF, then streams:
//   * every OUT (nn),v and sub-CPU command byte (via PC1600PowerProbe.hpp)
//   * every *newly reached* PC (a novelty filter drops loop spam)
//   * where/if either CPU HALTs
// A step is advance(1): one instruction of the bus owner (or one idle
// slice), counted in the T-states it took. The CPUs are read through
// sharp::legacy::coreParts until the probes go (M-51).
//
// Build: tools/build_pc1600_power_probe.sh
// Run:   ./pc1600_power_probe [roms-dir]   (default ./roms)

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <string>
#include <unordered_map>
#include <vector>

#include "../core/CPU/LH5803/LH5803.hpp"
#include "../core/CPU/SC7852/SC7852.hpp"
#include "../core/Machine/LegacyCalls.hpp"
#include "../core/PC1600/PC1600BusArbiter.hpp"
#include "../core/PC1600/PC1600PowerProbe.hpp"
#include "CliCommon.hpp"
#include "peripherals/RealTimeClock.hpp"

namespace {

uint32_t key(const sharp::legacy::CoreParts& c) {
    const bool z80 = c.arbiter->sc7852Owns();
    const uint16_t pc = z80 ? c.sc7852->pc() : static_cast<uint16_t>(c.lh5803->pc());
    return (z80 ? 0u : 0x10000u) | pc;
}

void printKey(uint32_t k) {
    std::fprintf(stderr, "%s %04X", (k & 0x10000) ? "5803" : "Z80 ", k & 0xFFFF);
}

} // namespace

int main(int argc, char** argv) {
    const std::string dir = (argc >= 2) ? argv[1] : "roms";

    std::string romErr;
    std::unique_ptr<cli::Machine> built = cli::buildMachine(sharp::kPC1600, "New", {dir}, &romErr);
    if (!built) {
        std::fprintf(stderr, "could not load the PC-1600 ROM set from '%s': %s\n", dir.c_str(), romErr.c_str());
        return 1;
    }
    cli::Machine& m = *built;
    const sharp::legacy::CoreParts core = sharp::legacy::coreParts(m);
    m.coldBoot();
    calcu::peripherals::setClocksFromHost(m, calcu::peripherals::HostDateTime{2026, 9, 2, 3, 12, 0, 0, 0});
    const uint64_t tickRate = m.descriptor().tickRate;

    uint64_t tstates = 0;
    uint64_t steps = 0;
    auto stepOnce = [&]() {
        const bool z80 = core.arbiter->sc7852Owns();
        pc1600probe::g_ownsSc7852 = z80;
        pc1600probe::g_pc = z80 ? core.sc7852->pc() : static_cast<uint16_t>(core.lh5803->pc());
        tstates += m.advance(1);
        steps++;
    };

    // ── Boot settle ─────────────────────────────────────────────────────
    const uint64_t kSettle = tickRate * 4;
    while (tstates < kSettle) stepOnce();
    std::fprintf(stderr, "boot settle: %llu steps, %llu T-states\n",
                 (unsigned long long)steps, (unsigned long long)tstates);

    // ── Characterise the idle loop ─────────────────────────────────────
    std::unordered_map<uint32_t, uint64_t> idle;
    for (int i = 0; i < 400000; i++) { idle[key(core)]++; stepOnce(); }
    std::fprintf(stderr, "\nidle-loop PC profile (top 20 of %zu distinct):\n", idle.size());
    {
        std::vector<std::pair<uint32_t, uint64_t>> v(idle.begin(), idle.end());
        std::sort(v.begin(), v.end(), [](auto& a, auto& b) { return a.second > b.second; });
        for (size_t i = 0; i < v.size() && i < 20; i++) {
            std::fprintf(stderr, "  ");
            printKey(v[i].first);
            std::fprintf(stderr, "  x%llu\n", (unsigned long long)v[i].second);
        }
    }

    // ── Press OFF, trace the divergence ────────────────────────────────
    std::fprintf(stderr, "\n=== OFF pressed (t=%llu) ===\n", (unsigned long long)tstates);
    pc1600probe::g_active = true;
    pc1600probe::g_verbose = true;
    m.keyDown(calcu::machine::KeyId{"off"});

    std::unordered_map<uint32_t, uint64_t> lastSeen; // key -> step last printed
    const uint64_t kBudget = tickRate * 3;
    const uint64_t tStart = tstates;
    bool released = false;
    int firstHaltReported = 0;

    while (tstates - tStart < kBudget) {
        if (!released && tstates - tStart > tickRate / 2) {
            m.keyUp(calcu::machine::KeyId{"off"});
            released = true;
            std::fprintf(stderr, "  (OFF released, t=%llu)\n", (unsigned long long)tstates);
        }
        const uint32_t k = key(core);
        auto it = lastSeen.find(k);
        if (it == lastSeen.end() || steps - it->second > 20000) {
            std::fprintf(stderr, "  path  ");
            printKey(k);
            std::fprintf(stderr, "   (t=+%llu)\n", (unsigned long long)(tstates - tStart));
        }
        lastSeen[k] = steps;

        stepOnce();

        if (!firstHaltReported && core.sc7852->halted() && (tstates - tStart) > 1000) {
            std::fprintf(stderr, "  [SC7852 re-entered HALT at PC=%04X, t=+%llu]\n",
                         core.sc7852->pc(), (unsigned long long)(tstates - tStart));
            firstHaltReported = 1;
        }
    }

    std::fprintf(stderr, "\n=== after OFF ===\n");
    std::fprintf(stderr, "bus owner: %s   SC7852 PC=%04X halted=%d   LH5803 PC=%04X halted=%d\n",
                 core.arbiter->sc7852Owns() ? "SC7852" : "LH5803",
                 core.sc7852->pc(), core.sc7852->halted() ? 1 : 0,
                 static_cast<uint16_t>(core.lh5803->pc()), core.lh5803->halted() ? 1 : 0);

    // Post-OFF steady-state PC profile (to catch a "wait for ON" spin loop
    // that never HALTs). Quiet the per-write spam but keep counting.
    pc1600probe::g_verbose = false;
    std::unordered_map<uint32_t, uint64_t> after;
    for (int i = 0; i < 400000; i++) { after[key(core)]++; stepOnce(); }
    std::fprintf(stderr, "\npost-OFF PC profile (top 20 of %zu distinct; '*' = also in idle loop):\n",
                 after.size());
    {
        std::vector<std::pair<uint32_t, uint64_t>> v(after.begin(), after.end());
        std::sort(v.begin(), v.end(), [](auto& a, auto& b) { return a.second > b.second; });
        for (size_t i = 0; i < v.size() && i < 20; i++) {
            std::fprintf(stderr, "  %s", idle.count(v[i].first) ? "*" : " ");
            printKey(v[i].first);
            std::fprintf(stderr, "  x%llu\n", (unsigned long long)v[i].second);
        }
    }

    std::fprintf(stderr, "\nI/O writes seen after OFF (port: count):\n");
    for (int p = 0; p < 256; p++)
        if (pc1600probe::g_ioWriteCount[p])
            std::fprintf(stderr, "  OUT (%02X)  x%llu\n", p,
                         (unsigned long long)pc1600probe::g_ioWriteCount[p]);
    std::fprintf(stderr, "sub-CPU commands seen after OFF (cmd: count):\n");
    for (int c = 0; c < 256; c++)
        if (pc1600probe::g_subCmdCount[c])
            std::fprintf(stderr, "  cmd %02X  x%llu\n", c,
                         (unsigned long long)pc1600probe::g_subCmdCount[c]);
    return 0;
}
