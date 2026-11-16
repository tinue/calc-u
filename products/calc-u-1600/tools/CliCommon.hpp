#pragma once
// Small helpers shared by the headless CLIs (pc1500_cli, pc1600_cli) and
// their peers (Ce158CliPeer.hpp): building a machine through the machine
// interface (C1600-1 design §7), whole-file I/O, the CE-150 summary, the
// cassette options and memory dumps. The machine is a
// calcu::machine::Machine; what the interface lacks goes through the
// adapter kinds and sharp::legacy (core/Machine/LegacyCalls.hpp) until the
// tools go (M-51).
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "../core/Connector/Ce150Card.hpp"
#include "../core/Connector/PlotterPaperOutput.hpp"
#include "../core/Debug/Inspect/Inspector.hpp"
#include "../core/FileIO.hpp"
#include "../core/Job.hpp"
#include "../core/Machine/LegacyCalls.hpp"
#include "../core/Machine/SharpBuilder.hpp"
#include "../core/Machine/SharpCatalogue.hpp"
#include "../core/Machine/SharpProductKinds.hpp"
#include "../core/Machine/SharpRoms.hpp"
#include "machine/Build.hpp"
#include "machine/Machine.hpp"

namespace cli {

using calcu::machine::Machine;

/// The ROM set of the folder `romFolder` (the tools' `roms`, relative to
/// the working directory), through the library's lookup (LIB-12; LIB-11
/// AC-20): the folder is the tool's, the looking is the library's. Null
/// with `error` the lookup's message (every file missing or bad) when a
/// file of the full set is missing or bad. Looked up once per folder.
inline std::shared_ptr<const calcu::roms::RomSet> romSet(const std::string& romFolder, std::string* error) {
    static std::string folder;
    static calcu::roms::LookupResult found;
    if (!found.ok() || folder != romFolder) {
        found = sharp::lookUpRoms(calcu::roms::DirectoryBundle(romFolder));
        folder = romFolder;
    }
    if (!found.ok() && error) *error = calcu::roms::describe(found.missing);
    return found.set;
}

/// A machine of `model` ("PC-1500", "PC-1500A", "PC-1600") and `variant`
/// (std::nullopt: the default), built from the catalogue with the images of
/// the ROM set of `romFolder` (romSet()), nothing plugged in, not booted.
/// Null with `error` set when the lookup fails or the build is refused.
inline std::unique_ptr<Machine> buildMachine(const std::string& model, const std::optional<std::string>& variant,
                                             const std::string& romFolder, std::string* error) {
    calcu::machine::BuildRequest req;
    req.model = calcu::machine::ModelId{model};
    if (variant) req.variant = calcu::machine::VariantId{*variant};
    req.soundRate = sharp::kSoundRate;
    const auto roms = romSet(romFolder, error);
    if (!roms) return nullptr;
    const calcu::machine::MachineDescriptor* d = sharp::sharpCatalogue().model(req.model);
    if (!d) {
        *error = "unknown model " + model;
        return nullptr;
    }
    req.roms = roms->imagesFor(*d, req.variant);
    sharp::SharpMachineBuilder builder;
    calcu::machine::BuildResult built = calcu::machine::build(sharp::sharpCatalogue(), builder, req);
    if (!built.machine) *error = built.refusal;
    return std::move(built.machine);
}

/// Runs `task` to its end, `slice` ticks at a time, calling `between` after
/// each slice (a serial peer's pump). A Job never splits a run request, so
/// the machine runs as in one runToCompletion().
template <class T>
T runSliced(job::Task<T> task, uint64_t slice, const std::function<void()>& between) {
    job::Job<T> j(std::move(task));
    while (!j.step(slice))
        if (between) between();
    if (between) between();
    return j.result();
}

/// Advances `ticks` (at least), `slice` at a time, calling `between` after each.
inline void advanceSliced(Machine& m, uint64_t ticks, uint64_t slice, const std::function<void()>& between) {
    uint64_t ran = 0;
    while (ran < ticks) {
        ran += m.advance(std::min(slice, ticks - ran));
        if (between) between();
    }
}

/// The capability of kind K the peripheral `id` owns now, or nullptr.
template <class K>
K* ofPeripheral(Machine& m, const char* id) {
    for (const auto& e : m.capabilities(K::kKind))
        if (const auto* o = std::get_if<calcu::machine::PeripheralOwner>(&e.owner); o && o->peripheral.value == id)
            return calcu::machine::capabilityAs<K>(m, e.id);
    return nullptr;
}

/// Whether the peripheral `id` is plugged in now.
inline bool plugged(const Machine& m, const char* id) {
    for (const auto& e : m.plugged().byPlace)
        if (e.peripheral.value == id) return true;
    return false;
}

/// A byte through the banks selected now (c1600.loader-memory).
inline uint8_t peek(Machine& machine, uint16_t addr) { return sharp::firstOf<sharp::LoaderMemory>(machine)->peek(addr); }

/// Reads the whole file. False if it can't be opened.
inline bool readFile(const std::string& path, std::vector<uint8_t>* out) {
    return readWholeFile(path, out);
}

/// Reads the whole file and requires exactly `size` bytes (ROM images).
inline bool readFileExact(const std::string& path, size_t size, std::vector<uint8_t>* out) {
    return readFile(path, out) && out->size() == size;
}

/// Writes `text` to `path`, replacing it. False on any open/write/close error.
inline bool writeFile(const std::string& path, const void* data, size_t size) {
    std::FILE* f = std::fopen(path.c_str(), "wb");
    if (!f) return false;
    const bool ok = std::fwrite(data, 1, size, f) == size;
    return std::fclose(f) == 0 && ok;
}
inline bool writeFile(const std::string& path, const std::string& text) {
    return writeFile(path, text.data(), text.size());
}
inline bool writeFile(const std::string& path, const std::vector<uint8_t>& bytes) {
    return writeFile(path, bytes.data(), bytes.size());
}

/// The plotter's paper in raw steps (sharp::rawPlotPoints): a paper is
/// connected to its paper-output capability once and reads what the plotter
/// kept until then (LIB-8 §10.3). Empty when that plotter is not plugged in.
inline std::vector<sharp::RawPlotPoint> plotPointsOf(Machine& machine, const char* plotter) {
    return sharp::takeRawPlotPoints(ofPeripheral<calcu::paper::PaperOutput>(machine, plotter));
}

/// Prints the CE-150 plot summary (points, first 60 events) when a CE-150 is
/// attached. Works on either model.
inline void printCe150Report(Machine& machine) {
    Ce150Card* card = sharp::legacy::coreParts(machine).ce150;
    if (!card) return;
    std::printf("CE-150: attached, plot points=%zu\n", plotPointsOf(machine, sharp::kCE150).size());
    const auto events = card->mechanism().drainEvents();
    std::printf("CE-150 events (%zu):\n", events.size());
    for (size_t i = 0; i < events.size() && i < 60; ++i) std::printf("  %s\n", events[i].c_str());
}

/// --tape-in <in.wav> / --tape-out <out.wav>: a cassette for the recorder
/// behind the tape interface (c1600.tape-deck), put in before the run and
/// taken out after it.
struct TapeOptions {
    std::string in, out;

    bool parseArg(int argc, char** argv, int& i) {
        if (i + 1 >= argc) return false;
        if (std::strcmp(argv[i], "--tape-in") == 0) { in = argv[++i]; return true; }
        if (std::strcmp(argv[i], "--tape-out") == 0) { out = argv[++i]; return true; }
        return false;
    }

    bool arm(Machine& machine) const {
        sharp::TapeDeckAccess* deck = sharp::firstOf<sharp::TapeDeckAccess>(machine);
        if (!in.empty()) {
            std::string error;
            if (!deck->play(in, error)) {
                std::fprintf(stderr, "--tape-in: %s\n", error.c_str());
                return false;
            }
        } else if (!out.empty()) {
            deck->record(out);
        }
        return true;
    }

    /// Reports the tape position and ejects it, saving a recording.
    bool finish(Machine& machine) const {
        if (in.empty() && out.empty()) return true;
        sharp::TapeDeckAccess* deck = sharp::firstOf<sharp::TapeDeckAccess>(machine);
        const auto tape = deck->status();
        std::printf("Tape: %.2f s of %.2f s, motor %s\n", tape.position, tape.length, tape.motor ? "on" : "off");
        std::string error;
        if (!deck->eject(&error)) {
            std::fprintf(stderr, "--tape-out: %s\n", error.c_str());
            return false;
        }
        if (!out.empty()) std::printf("Wrote %.2f s of tape to %s\n", tape.length, out.c_str());
        return true;
    }
};

/// --dump-mem <addr>,<len> (repeatable): memory printed as hex at the end.
using MemDumps = std::vector<std::pair<uint32_t, uint32_t>>;

// CALCU_PAPER_OUT=<file>: the plotters' paper as points, one per line
// ("x y color newStroke"), after a header per plotter -- the reference the
// migration records once (M-8 of docs/migration-plan.md, LIB-8 AC-39).
inline void appendPlotPoints(std::string* out, const char* plotter, const std::vector<sharp::RawPlotPoint>& points) {
    *out += std::string("plotter ") + plotter + " points " + std::to_string(points.size()) + "\n";
    for (const auto& p : points)
        *out += std::to_string(p.x) + " " + std::to_string(p.y) + " " + std::to_string(p.color) + " " +
                std::to_string(p.newStroke) + "\n";
}

/// The CALCU_PAPER_OUT file (see appendPlotPoints()): the CE-150's paper,
/// then the CE-1600P's, of whichever is plugged in. False when it can't be
/// written; true without the variable.
inline bool writePaperIfAsked(Machine& machine) {
    const char* paperPath = std::getenv("CALCU_PAPER_OUT");
    if (!paperPath) return true;
    std::string paper;
    if (ofPeripheral<calcu::paper::PaperOutput>(machine, sharp::kCE150))
        appendPlotPoints(&paper, "CE-150", plotPointsOf(machine, sharp::kCE150));
    if (ofPeripheral<calcu::paper::PaperOutput>(machine, sharp::kCE1600P))
        appendPlotPoints(&paper, "CE-1600P", plotPointsOf(machine, sharp::kCE1600P));
    return writeFile(paperPath, paper);
}

/// Adds `<addr>,<len>` to `dumps`; false (with a message) if malformed.
inline bool parseMemDump(const char* arg, MemDumps* dumps) {
    char* rest = nullptr;
    const auto start = static_cast<uint32_t>(std::strtoul(arg, &rest, 0));
    const uint32_t length = (rest && *rest == ',') ? static_cast<uint32_t>(std::strtoul(rest + 1, nullptr, 0)) : 0;
    if (length == 0) {
        std::fprintf(stderr, "--dump-mem wants <addr>,<len>\n");
        return false;
    }
    dumps->emplace_back(start, length);
    return true;
}

/// --inspect <view> (repeatable): an inspector view (Core/Debug/Inspect)
/// printed at the end, by its short name (inspect::viewName()).
using InspectViews = std::vector<inspect::View>;

inline bool parseInspect(const char* arg, InspectViews* views) {
    inspect::View v;
    if (!inspect::viewFromName(arg, &v)) {
        std::fprintf(stderr, "--inspect: unknown view '%s'\n", arg);
        return false;
    }
    views->push_back(v);
    return true;
}

/// Prints each view, through the machine's c1600.inspector.
inline void printInspect(Machine& machine, const InspectViews& views) {
    const sharp::InspectorViews* inspector = sharp::firstOf<sharp::InspectorViews>(machine);
    for (inspect::View v : views)
        for (const std::string& line : inspector->view(v)) std::printf("%s\n", line.c_str());
}

inline void printMemDumps(Machine& machine, const MemDumps& dumps) {
    for (const auto& [start, length] : dumps) {
        std::printf("--- memory $%04X+%u ---\n", start, length);
        for (uint32_t a = start; a < start + length && a <= 0xFFFF; a += 16) {
            std::printf("%04X:", a);
            for (uint32_t i = a; i < a + 16 && i < start + length && i <= 0xFFFF; ++i)
                std::printf(" %02X", peek(machine, static_cast<uint16_t>(i)));
            std::printf("\n");
        }
    }
}

}  // namespace cli
