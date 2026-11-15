#pragma once
#include "../app/KeyIds.hpp"
#include "../app/ProductRoms.hpp"
#include "../core/PC100C.hpp"
#include "../core/PrinterOutput.hpp"
#include "../core/TIAdapters.hpp"
#include "../core/TICatalogue.hpp"
#include "../core/TISegments.hpp"
#include "machine/Machine.hpp"
#include "peripherals/Controls.hpp"
#include "roms/Bundle.hpp"
#include <cstdio>
#include <cstdlib>
#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
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

/// A fresh folder under the system's temporary folder, removed with the object.
struct TempDir {
    std::filesystem::path path;
    TempDir() {
        static std::atomic<int> n{0};
        path = std::filesystem::temp_directory_path() /
               ("calcu59-test-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + "-" +
                std::to_string(n++));
        std::filesystem::create_directories(path);
    }
    ~TempDir() {
        std::error_code ec;
        std::filesystem::remove_all(path, ec);
    }
    TempDir(const TempDir&) = delete;
    TempDir& operator=(const TempDir&) = delete;
    /// Every file of roms/ (no subfolders) copied in.
    void copyRoms() const {
        for (const auto& e : std::filesystem::directory_iterator("roms"))
            if (e.is_regular_file()) std::filesystem::copy_file(e.path(), path / e.path().filename());
    }
};

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
inline TIDebugTargetAdapter& dbg(Machine& m) { return *debugTargetOf(m); }
inline TICardReaderAdapter& card(Machine& m) { return *cardReaderOf(m); }
inline TIDebugPanelAdapter& panel(Machine& m) { return *debugPanelOf(m); }

/// Instructions per second in RUN mode: tick rate / 32.
inline double instructionHz(cu59::Model model) {
    return tiCatalogue().model(cu59::modelId(model))->tickRate / 32.0;
}

/// Build a machine with ROM and constants, the Master Library and the
/// PC-100C attached at the build, as 2.3.0's apps started (C59-6).
inline std::unique_ptr<Machine> makeMachine(cu59::Model model, bool printer = true) {
    return cu59::buildMachine(model, *romSet(), cu59::MachinePeripherals{std::string("ML"), printer, false});
}

// The PC-100C and the module through their places and capabilities (C59-6).
inline PrinterOutput* paper(Machine& m) {
    return calcu::machine::capabilityAs<PrinterOutput>(m, calcu::machine::CapabilityId{"printer/c59.printer-output"});
}
inline calcu::peripherals::Controls* printerControls(Machine& m) {
    return calcu::machine::capabilityAs<calcu::peripherals::Controls>(m, calcu::machine::CapabilityId{"printer/controls"});
}
/// The lines printed since the last call (none without a PC-100C).
inline std::vector<std::string> printedLines(Machine& m) {
    auto* p = paper(m);
    return p ? p->takeLines() : std::vector<std::string>{};
}
inline std::vector<PrinterCodeLine> printedCodeLines(Machine& m) {
    auto* p = paper(m);
    return p ? p->takeCodeLines() : std::vector<PrinterCodeLine>{};
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

/// `n` instructions.
inline void run(Machine& m, uint32_t n) {
    for (uint32_t i = 0; i < n; ++i) m.advance(1);
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
