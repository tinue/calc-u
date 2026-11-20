#pragma once
#include <string>

#include "support/PC1500BasicTyper.hpp"
#include "../core/PC1500/PC1500Machine.hpp"
#include "support/PC1600BasicTyper.hpp"
#include "../core/PC1600/PC1600Machine.hpp"
#include "../core/PC1500/PC1500LcdText.hpp"
#include "TestHarness.hpp"

// The steps a version 1 preset took on an old machine class, for the core
// tests that set a card up this way (the version 1 loaders are gone; these
// tests are about the card, not the preset): the boot to the prompt,
// `NEW0? :CHECK` answered with CL (as the loader did), typed lines, a typed
// program through the old typer (the reference typer of the BASIC loader
// tests), and the wait for the interpreter.

namespace scripted {

// After allReset(): the boot run; a fresh memory map's NEW0? answered.
inline void bootPC1500(PC1500Machine& m) {
    m.coldBoot();
    runBootToPrompt(m);
    for (int i = 0; i < 5; ++i) {
        LcdText t = pc1500LcdText(m);
        if (t.text().find("NEW0") == std::string::npos) break;
        tapKey(m, "cl");
        waitIdle(m, PC1500Machine::kCpuHz * 5);
    }
}

inline bool type(PC1500Machine& m, const std::string& line) {
    std::string error;
    const bool ok = typeLine(m, line, true, &error);
    if (!ok) std::fprintf(stderr, "  typeLine %s: %s\n", line.c_str(), error.c_str());
    return ok;
}

inline bool program(PC1500Machine& m, const std::string& listing) { return typeBasicProgramText(m, listing).ok; }

inline void untilIdle(PC1500Machine& m, double seconds) {
    waitUntilBasicIdle(m, static_cast<uint64_t>(PC1500Machine::kCpuHz * seconds));
}

inline void bootPC1600(PC1600Machine& m) {
    m.coldBoot();
    runBootToPrompt(m);
}

inline bool type(PC1600Machine& m, const std::string& line) {
    std::string error;
    const bool ok = typeLine(m, line, true, &error);
    if (!ok) std::fprintf(stderr, "  typeLine %s: %s\n", line.c_str(), error.c_str());
    return ok;
}

inline bool program(PC1600Machine& m, const std::string& listing) { return typeBasicProgramText(m, listing).ok; }

inline void untilIdle(PC1600Machine& m, double seconds) {
    waitUntilBasicIdle(m, static_cast<uint64_t>(PC1600Machine::kTStateHz * seconds));
}

} // namespace scripted
