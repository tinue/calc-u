#pragma once
#include <cstdint>
#include <string>
#include <vector>

#include "Tape/TapeDeck.hpp"
#include "app/ControlState.hpp"
#include "roms/Lookup.hpp"
#include "sessions/LibraryTopics.hpp"
#include "sessions/Messages.hpp"
#include "sessions/Payload.hpp"

// Calc-U-1600's topics, commands and requests on the library's app session
// (LIB-3 design §4, §5: the interim product topics; spec AC-38). Each is
// carried by the library unread (AC-12) and re-pointed at its owner's topic
// in the owner's step: `c1600.control-state` LIB-4 1600 (M-49);
// `c1600.machine` LIB-4 / LIB-18 1600 (M-49 / M-46; since M-47 with the
// tape counter and the floppy lamp, LIB-7 1600: `c1600.tapes` went);
// `c1600.serial-links` LIB-2 1600. `roms-missing` is
// LIB-12's topic (LIB-12 design §3.5); its payload lives here until a
// second product sends it.

namespace shared {

namespace topics {
inline const calcu::sessions::TopicName kControlState{"c1600.control-state"};
inline const calcu::sessions::TopicName kMachine{"c1600.machine"};
inline const calcu::sessions::TopicName kSerialLinks{"c1600.serial-links"};
inline const calcu::sessions::TopicName kRomsMissing{"roms-missing"};
} // namespace topics

// The commands. Those that start an operation are marked (op). Arguments in
// braces. The start, the model and variant switches, attach, detach, Reset
// and Cold boot are LIB-18's (`start`, `model`, `variant`, `attach`,
// `detach`, `reset`, `cold-boot`; startup/StartupPart.hpp), presets LIB-6's
// (`load-preset`, `preset-list`; presets/PresetsPart.hpp). These need the
// product's data (LIB-18 D-22), each one plug change:
namespace commands {
inline const calcu::sessions::CommandName& kStart = calcu::sessions::kStartCommand;                       // (op) {model?, preset?} LIB-15 N3
inline const calcu::sessions::CommandName kCE1600PRom{"c1600.ce1600p-rom"};      // (op) {version: old|new}
inline const calcu::sessions::CommandName kMountHostDrive{"c1600.mount-host-drive"};      // (op) {dir}
// C1600-2 (app/SharpLoaderPart, design §3.7): stateless, each carries the
// file's bytes; `file` picks a file of a cassette WAV (from 1; the first
// without it); `address` is the text the user typed (C000, &C000, $C000,
// 0xC000); `cpu` "lh5803" or "z80" (headerless only).
inline const calcu::sessions::CommandName kLoadBasic{"c1600.load-basic"};        // {bytes, name, file?}
inline const calcu::sessions::CommandName kTypeBasic{"c1600.type-basic"};        // (op) {bytes, name}
inline const calcu::sessions::CommandName kLoadCode{"c1600.load-code"};          // {bytes, name, file?, address?, length?, cpu?, run?} (op with run)
} // namespace commands

namespace requests {
inline const calcu::sessions::RequestName kFileKind{"c1600.file-kind"};          // {bytes} -> FileKindReply
inline const calcu::sessions::RequestName kPlanCode{"c1600.plan-code"};          // {bytes, file?, address?, length?, cpu?} -> CodePlanReply
} // namespace requests

// C1600-2 AC-1 to AC-3: what a file is.
struct FileKindReply final : calcu::sessions::PayloadOf<FileKindReply> {
    static constexpr calcu::sessions::PayloadKind kKind{"c1600.file-kind"};
    std::string kind;          // empty, basic-listing, basic-pc1500, basic-pc1600, code-lh5801, code-z80, headerless, other
    std::string words;         // other: what it holds
    std::string looksLike;     // headerless: "lh5803", "z80" or ""
    bool damaged = false;
    bool hasHeader = false;
    std::string headerName;
    std::uint32_t loadAddress = 0;
    int bank = 0;
    std::uint64_t length = 0;
    std::uint32_t start = 0;   // 0: none
    std::vector<std::string> tapeFiles;
    bool same(const FileKindReply& o) const { return toValue() == o.toValue(); }
    calcu::sessions::Value toValue() const override;
};

// C1600-2 AC-8, AC-18: the plan, for the address dialog.
struct CodePlanReply final : calcu::sessions::PayloadOf<CodePlanReply> {
    static constexpr calcu::sessions::PayloadKind kKind{"c1600.code-plan"};
    std::string refusal;       // empty: the plan holds
    bool needsAddress = false;
    std::string proposed;      // "&C0C5": the start of the selected program area (PC-1600)
    std::string placeholder;   // "hex, e.g. &C0C5"
    std::uint64_t length = 0;
    std::string cpu;           // "Z-80", "LH5803"; "" on a PC-1500
    std::string cpuNote;
    std::string where;         // the place the code would go, or its warning
    bool same(const CodePlanReply& o) const { return toValue() == o.toValue(); }
    calcu::sessions::Value toValue() const override;
};

// C1600-2 AC-11, AC-12, AC-13: what was loaded and how to keep and start it.
struct CodeLoadedReply final : calcu::sessions::PayloadOf<CodeLoadedReply> {
    static constexpr calcu::sessions::PayloadKind kKind{"c1600.code-loaded"};
    std::string summary;
    std::string range;
    std::string area;          // PC-1600: S0, S1, S2
    int bank = 0;
    std::string newCommand, newNote, callCommand, callNote;
    std::vector<std::string> warnings;
    bool started = false;      // `run`: the starting command was typed
    bool same(const CodeLoadedReply& o) const { return toValue() == o.toValue(); }
    calcu::sessions::Value toValue() const override;
};

// What the pickers, menus and panels offer now (ControlState).
struct ControlStatePayload final : calcu::sessions::PayloadOf<ControlStatePayload> {
    static constexpr calcu::sessions::PayloadKind kKind{"c1600.control-state"};
    explicit ControlStatePayload(ControlState s) : state(std::move(s)) {}
    ControlState state;
    bool same(const ControlStatePayload& o) const { return state == o.state; }
    calcu::sessions::Value toValue() const override;
};

// The machine: its model and how often it was rebuilt (0.8.1's
// machineReplaced, modelSwitched); the tape counter (0.8.1's tapeStatus)
// and the CE-1600F's lamp (LIB-7 AC-39, AC-42). The media themselves are
// LIB-7's topic `media`.
struct MachinePayload final : calcu::sessions::PayloadOf<MachinePayload> {
    static constexpr calcu::sessions::PayloadKind kKind{"c1600.machine"};
    Model model = Model::PC1500A;
    std::uint64_t builds = 0;          // grows with every machine built
    std::uint64_t modelSwitches = 0;   // grows with every model switch and preset load
    TapeDeck::Status tape;             // the counter: position while playing, length while recording
    bool floppyMotor = false;          // the drive's motor runs: the green lamp
    bool same(const MachinePayload& o) const {
        return model == o.model && builds == o.builds && modelSwitches == o.modelSwitches &&
               tape.mode == o.tape.mode && tape.motor == o.tape.motor && tape.position == o.tape.position &&
               tape.length == o.tape.length && floppyMotor == o.floppyMotor;
    }
    calcu::sessions::Value toValue() const override;
};

// The serial port files (0.8.1's serialLinksRelinked; LIB-2 AC-33).
struct SerialLinksPayload final : calcu::sessions::PayloadOf<SerialLinksPayload> {
    static constexpr calcu::sessions::PayloadKind kKind{"c1600.serial-links"};
    std::string rs232c, ce158;   // the path a client opens, empty while there is none
    std::uint64_t moves = 0;
    bool same(const SerialLinksPayload& o) const { return rs232c == o.rs232c && ce158 == o.ce158 && moves == o.moves; }
    calcu::sessions::Value toValue() const override;
};

// A file of the ROM full set is missing or bad: no machine (LIB-12 AC-5).
struct RomsMissingPayload final : calcu::sessions::PayloadOf<RomsMissingPayload> {
    static constexpr calcu::sessions::PayloadKind kKind{"roms-missing"};
    explicit RomsMissingPayload(calcu::roms::RomsMissing m) : missing(std::move(m)) {}
    calcu::roms::RomsMissing missing;
    bool same(const RomsMissingPayload& o) const { return calcu::roms::describe(missing) == calcu::roms::describe(o.missing); }
    calcu::sessions::Value toValue() const override;
};

} // namespace shared
