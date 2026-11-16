#pragma once
#include "../app/KeyIds.hpp"
#include "../app/ProductRoms.hpp"
#include "../core/PC100C.hpp"
#include "../core/PC100CPaper.hpp"
#include "../core/TIAdapters.hpp"
#include "../core/TIMachineTesting.hpp"
#include "paper/Paper.hpp"
#include "../core/TICatalogue.hpp"
#include "../core/TISegments.hpp"
#include "machine/Machine.hpp"
#include "peripherals/Controls.hpp"
#include "roms/Bundle.hpp"
#include "TempFolder.hpp"
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <map>
#include <functional>
#include <memory>
#include <sstream>

// Test fixtures: look up roms/ in the working directory (the product directory
// under CTest) and build a machine the way the apps do, through the machine
// interface (C59-1). Instruction counts stay exact: one advance(1) runs one
// instruction.

namespace testroms {

using calcu::machine::Machine;

inline std::optional<std::string> readFile(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return std::nullopt;
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

/// A fresh folder of the test, removed with the object (library/testing/).
using TempDir = calcu::testing::TempFolder;

/// The ROM set of roms/ (fetch_roms.sh), through the lookup as the apps get
/// it (LIB-12): looked up once per test program. A missing or bad file ends
/// the program with the lookup's message.
inline std::shared_ptr<const calcu::roms::RomSet> romSet() {
    static const std::shared_ptr<const calcu::roms::RomSet> set = [] {
        auto r = cu59::lookUpRoms(calcu::roms::DirectoryBundle("roms"));
        if (!r.ok()) {
            std::fprintf(stderr, "%s\n", calcu::roms::describe(r.missing).c_str());
            std::exit(1);
        }
        return r.set;
    }();
    return set;
}

// The adapters of a TI machine (C59-1 design §5).
inline TIMemoryAdapter& mem(Machine& m) { return *memoryOf(m); }
/// The panel's reads of the core (LIB-10 L-E3; the debug target adapter went in M-53).
inline TIDebugPanelAdapter& dbg(Machine& m) { return *debugPanelOf(m); }
/// The core under the machine, for what only a test inside the core reads.
inline TI59Machine& hw(Machine& m) { return TIMachineTesting::hardware(static_cast<TIMachine&>(m)); }
/// The core's card reader (insertCard, cardEject, isCardPresent, cardMode, …).
inline TI59Machine& card(Machine& m) { return hw(m); }
/// The TI-59's card reader as a media place (C59-7); nullptr on a TI-58/58C.
inline calcu::media::MediaPlace* cardPlace(Machine& m) {
    return calcu::machine::capabilityAs<calcu::media::MediaPlace>(m, calcu::machine::CapabilityId{"card-reader/media-place"});
}
inline TIDebugPanelAdapter& panel(Machine& m) { return *debugPanelOf(m); }

/// Instructions per second in RUN mode: tick rate / 32.
inline double instructionHz(cu59::Model model) {
    return tiCatalogue().model(cu59::modelId(model))->tickRate / 32.0;
}

inline void freshMachine(Machine& m);

/// Build a machine with ROM and constants, the Master Library and the
/// PC-100C attached at the build, as 2.3.0's apps started (C59-6).
inline std::unique_ptr<Machine> makeMachine(cu59::Model model, bool printer = true) {
    auto m = cu59::buildMachine(model, *romSet(), cu59::MachinePeripherals{std::string("ML"), printer, false});
    freshMachine(*m);
    return m;
}

// The PC-100C and the module through their places and capabilities (C59-6).
inline calcu::paper::PaperOutput* paper(Machine& m) {
    return calcu::machine::capabilityAs<calcu::paper::PaperOutput>(m, calcu::machine::CapabilityId{"printer/paper-output"});
}
/// A paper of the PC-100C's description per machine, bound to its paper
/// output whenever a new one is there (LIB-8 AC-24): what the tests read the
/// printout from, as the PaperPart does in the app session.
struct PaperTap {
    calcu::paper::Paper paper{pc100cpaper::description()};
    std::size_t linesTaken = 0;
};
inline std::map<const Machine*, PaperTap>& paperTaps() {
    static std::map<const Machine*, PaperTap> taps;
    return taps;
}
inline PaperTap& paperTap(Machine& m) {
    PaperTap& t = paperTaps()[&m];
    auto* out = paper(m);
    if (!out) t.paper.release();
    else if (out->connected() != &t.paper) {
        t.paper.release();
        t.paper.bind(out);
    }
    return t;
}
/// A machine just built: a fresh paper tap (a machine at the address of one
/// gone gets none of its paper), and the PC-100C's code lines kept (P-E3).
inline void freshMachine(Machine& m) {
    paperTaps().erase(&m);
    TIMachineTesting::printedCodeLines(static_cast<TIMachine&>(m));
}
inline calcu::peripherals::Controls* printerControls(Machine& m) {
    return calcu::machine::capabilityAs<calcu::peripherals::Controls>(m, calcu::machine::CapabilityId{"printer/controls"});
}
/// The lines printed since the last call (none without a PC-100C).
inline std::vector<std::string> printedLines(Machine& m) {
    PaperTap& t = paperTap(m);
    std::vector<std::string> out;
    for (std::size_t i = t.linesTaken; i < t.paper.text().size(); ++i) out.push_back(t.paper.text()[i].text);
    t.linesTaken = t.paper.text().size();
    return out;
}
/// The code lines the PC-100C ended since the last call (P-E3).
inline std::vector<PC100C::CodeLine> printedCodeLines(Machine& m) {
    return TIMachineTesting::printedCodeLines(static_cast<TIMachine&>(m));
}
/// PRINT or ADV down/up, TRACE on/off.
inline void printerControl(Machine& m, const calcu::machine::ControlId& id, bool on) {
    auto* c = printerControls(m);
    if (!c) return;
    if (id == PC100C::kTrace) c->set(id, on);
    else if (on) c->press(id);
    else c->release(id);
}
inline const calcu::machine::PlacePath kPrinterPlace{{kTIPrinterPlace}};
inline const calcu::machine::PlacePath kSlotPlace{{kTIModulePlace}};
/// The PC-100C attached (TRACE at `trace`) or detached; what follows is
/// returned, not run.
inline calcu::machine::ChangeResult setPrinter(Machine& m, bool on, bool trace = false) {
    if (on == (paper(m) != nullptr)) return {};
    return on ? m.attach(cu59::printerRequest(trace), tiCatalogue()) : m.detach(kPrinterPlace, kPC100C, tiCatalogue());
}
/// The ID of the module in `slot-1`, or "".
inline std::string moduleIn(const Machine& m) {
    for (const auto& e : m.plugged().byPlace)
        if (e.place == kSlotPlace) return e.peripheral.value;
    return {};
}

/// Called after every 256 instructions of run() and at its end, when set
/// (a trace drained while a script runs, LIB-10 AC-33).
inline std::function<void(Machine&)>& afterRun() {
    static std::function<void(Machine&)> hook;
    return hook;
}

/// `n` instructions.
inline void run(Machine& m, uint32_t n) {
    for (uint32_t i = 0; i < n; ++i) {
        m.advance(1);
        if ((i & 255) == 255 && afterRun()) afterRun()(m);
    }
    if (afterRun()) afterRun()(m);
}

/// Run until the machine reports key readiness (or give up).
inline bool runToScanLoop(Machine& m, cu59::Model, uint32_t maxSteps = 2'000'000) {
    for (uint32_t i = 0; i < maxSteps; i += 100) {
        run(m, 100);
        if (m.isKeyReady()) return true;
    }
    return false;
}

/// The A–G bits of one position.
inline uint8_t segmentBits(const calcu::machine::SegmentPosition& p) {
    uint8_t bits = 0;
    for (int s = 0; s < 7; ++s)
        if (p.lit[static_cast<size_t>(s)]) bits |= static_cast<uint8_t>(1u << s);
    return bits;
}

inline const std::vector<calcu::machine::SegmentPosition>& positions(const calcu::machine::DisplayOutput& d) {
    return std::get<calcu::machine::SegmentFrame>(d.frame).positions;
}

/// One position as text: a digit, " ' o (degree) -, or nothing for a blank.
inline std::string glyphText(uint8_t bits) {
    for (int i = 0; i < 10; ++i)
        if (bits == kDigitSegments[i]) return std::string(1, static_cast<char>('0' + i));
    if (bits == kSegmentsQuote) return "\"";
    if (bits == kSegmentsApostrophe) return "'";
    if (bits == kSegmentsDegree) return "o";
    if (bits == kSegmentsMinus) return "-";
    return "";
}

/// Display as text, leftmost digit first. Positions not driven since the last
/// read (suppressed, blank, or the CPU computing) are skipped.
inline std::string displayText(Machine& m) {
    const auto out = m.readDisplay();
    std::string s;
    for (int i = 11; i >= 0; --i) {
        const auto& p = positions(out)[static_cast<size_t>(i)];
        if (p.brightness.value_or(0) == 0) continue;
        s += glyphText(segmentBits(p));
    }
    return s;
}

inline void press(Machine& m, int code) { m.keyDown(*cu59::keyIdAt(static_cast<uint8_t>(code))); }
inline void release(Machine& m, int code) { m.keyUp(*cu59::keyIdAt(static_cast<uint8_t>(code))); }

/// Press a matrix key, hold ~100 ms of emulated time, release, settle.
inline void tap(Machine& m, cu59::Model model, int code) {
    press(m, code);
    run(m, static_cast<uint32_t>(instructionHz(model) * 0.1));
    release(m, code);
    run(m, static_cast<uint32_t>(instructionHz(model) * 0.1));
    runToScanLoop(m, model);
}

} // namespace testroms
