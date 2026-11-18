#pragma once
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "MachineCodeFile.hpp"
#include "ProgramFile.hpp"
#include "machine/Machine.hpp"

namespace sharp {
class LoaderMemory;
}

// ── The loaders of Calc-U-1600 ────────────────────────────────────────────
//
// The entry points of every caller -- the menu and the harness (through
// app/SharpLoaderPart), a preset's `program:` (app/SharpPresetRules),
// the debugger's Build & Load (core/Debug/ProgramLoader): what a file is,
// loading BASIC, the one plan for machine code and its write. They reach
// memory only through the loader memory access (sharp::LoaderMemory, the
// kind `c1600.loader-memory`) and name no machine class; the model follows
// from the kind (Mode::None: a PC-1500 or PC-1500A). Each comes on the kind
// and on a calcu::machine::Machine, whose kind it uses. Typing a program in
// is core/BasicTyping.hpp. Single-threaded; nothing thrown.

namespace sharp::loaders {

// The machine's loader memory, or nullptr (not a Sharp machine).
LoaderMemory* memoryOf(calcu::machine::Machine&);

// ---- What a file is ----

struct Header {
    std::string name;                 // a CE-158 header's name; empty for a PC-1600 header
    std::uint32_t loadAddress = 0;    // without the bank
    int bank = 0;                     // a PC-1600 header's bank (bits 16-23)
    std::size_t length = 0;           // the length the header says
    std::optional<std::uint32_t> start;   // the address to start at; none for 0 or FFFF
};

struct FileKind {
    programfile::Kind kind = programfile::Kind::Empty;
    std::string words;                // Other: what it holds ("plain text", ...); WAV without a file: why
    std::optional<machinecode::Cpu> looksLike;   // headerless: LH5801 (LH5803) or Z-80 code by SDE's guess
    std::optional<Header> header;
    bool damaged = false;
    std::vector<std::string> tapeFiles;   // a WAV's readable files in tape order ("HELLO", "(no name)")
    std::vector<programfile::Kind> tapeKinds;   // the kind of each of them (each loaded by its own loader)
};
FileKind fileKind(const std::vector<std::uint8_t>& bytes);

// The image of file `n` (from 1) of a cassette WAV; n = 0, or a
// file that is no WAV: the first file, or the bytes themselves. nullopt,
// with `why`, for a number the WAV does not have.
std::optional<std::vector<std::uint8_t>> image(const std::vector<std::uint8_t>& bytes, int n, std::string* why);

// "<name>: <reason>".
std::string refusal(const std::string& name, const std::string& reason);

// ---- BASIC ----

struct BasicLoaded {
    bool ok = false;
    std::uint16_t start = 0, end = 0;   // the main CPU's addresses
    std::string refusal;                // the reason, without the file's name
};
// Not an operation: writes at once. Refused, writing nothing.
BasicLoaded loadBasic(LoaderMemory&, const std::vector<std::uint8_t>& image);
BasicLoaded loadBasic(calcu::machine::Machine&, const std::vector<std::uint8_t>& image);

// ---- Machine code ----

struct CodeRequest {                       // what a caller gives by hand
    std::optional<std::uint32_t> address;  // in the processor's space
    std::optional<std::size_t> length;
    std::optional<machinecode::Cpu> cpu;   // headerless only: the caller names it (the debugger, `cpu=`)
};

struct CodePlan {
    machinecode::Target target = machinecode::Target::PC1500;
    machinecode::File file;
    machinecode::LoadOptions options;
    machinecode::PC1600State state;   // PC-1600 only
    machinecode::LoadPlan plan;
    std::string refusal;              // the plan's one sentence; empty: it holds
    bool holds() const { return refusal.empty(); }
    bool needsAddress() const { return plan.error == machinecode::LoadError::NeedsAddress; }
};
// The one plan: machinecode::planLoad with the state
// read from the kind.
CodePlan planCode(LoaderMemory&, const std::vector<std::uint8_t>& image, const CodeRequest&);
CodePlan planCode(calcu::machine::Machine&, const std::vector<std::uint8_t>& image, const CodeRequest&);

// For the address dialog: the processor that is assumed, in words
// (empty on a PC-1500), and where a plan that holds puts the code (or its
// warning; empty: nothing to say).
std::string cpuNote(const CodePlan&);
std::string where(const CodePlan&);

struct CodeLoaded {
    std::string range;                     // "&C0C5-&C0EE", in the processor's space
    std::uint32_t address = 0;             // the first byte, in the processor's space
    std::size_t length = 0;
    bool header = false;                   // the file has a header
    std::string area;                      // PC-1600: "S0", "S1", "S2"; empty on the PC-1500
    int bank = 0;
    machinecode::Cpu cpu = machinecode::Cpu::Z80;
    std::uint32_t entry = 0;               // what the starting command calls
    bool headerStart = false;              // the header names the address to start at
    machinecode::Advice advice;            // NEW and its note, the starting command and its note
    std::vector<std::string> warnings;     // the work area; BASIC can overwrite it
    std::string summary;                   // "Loaded 42 bytes at &C0C5-&C0EE (S0)"
};
// Writes what the plan says; refuses with the plan's sentence (or the
// writer's), writing nothing then. Not an operation.
std::variant<CodeLoaded, std::string> loadCode(LoaderMemory&, const std::vector<std::uint8_t>& image,
                                               const CodeRequest&);
std::variant<CodeLoaded, std::string> loadCode(calcu::machine::Machine&, const std::vector<std::uint8_t>& image,
                                               const CodeRequest&);

} // namespace sharp::loaders
