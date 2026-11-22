#pragma once
#include <cstdint>
#include <string>
#include <vector>

#include "Tape/TapeDeck.hpp"
#include "roms/Lookup.hpp"
#include "sessions/LibraryTopics.hpp"
#include "sessions/Messages.hpp"
#include "sessions/Payload.hpp"

// Calc-U-1600's topics, commands and requests on the library's app session.
// The control state, the model and the builds are the library's topics.
// What is the product's own: `c1600.lamps` (the tape counter and the
// floppy lamp), `c1600.serial-links` (the serial
// link files). `roms-missing` is the ROM lookup's topic; its
// payload lives here until a second product sends it.

namespace cu1600 {

namespace topics {
inline const calcu::sessions::TopicName kLamps{"c1600.lamps"};
inline const calcu::sessions::TopicName kSerialLinks{"c1600.serial-links"};
} // namespace topics

// The commands. Those that start an operation are marked (op). Arguments in
// braces. The start, the model and variant switches, attach, detach, Reset
// and Cold boot are the library's machine control's (`start`, `model`, `variant`, `attach`,
// `detach`, `reset`, `cold-boot`; startup/StartupPart.hpp), presets the preset loader's
// (`load-preset`, `preset-list`; presets/PresetsPart.hpp). These need the
// product's data, each one plug change:
namespace commands {
inline const calcu::sessions::CommandName& kStart = calcu::sessions::kStartCommand;                       // (op) {model?, preset?}
inline const calcu::sessions::CommandName kCE1600PVariant{"c1600.ce1600p-variant"};      // (op) {version: old|new}
inline const calcu::sessions::CommandName kMountHostDrive{"c1600.mount-host-drive"};      // (op) {dir}
// The loaders (app/SharpLoaderPart): stateless, each carries the
// file's bytes; `file` picks a file of a cassette WAV (from 1; the first
// without it); `address` is the text the user typed (C000, &C000, $C000,
// 0xC000); `cpu` "lh5803" or "z80" (headerless only).
inline const calcu::sessions::CommandName kLoadBasic{"c1600.load-basic"};        // {bytes, name, file?}
inline const calcu::sessions::CommandName kTypeBasic{"c1600.type-basic"};        // (op) {bytes, name}
inline const calcu::sessions::CommandName kLoadCode{"c1600.load-code"};          // {bytes, name, file?, address?, length?, cpu?, run?} (op with run)
} // namespace commands

namespace requests {
inline const calcu::sessions::RequestName kFileKind{"c1600.file-kind"};          // {bytes} -> FileKindReply
inline const calcu::sessions::RequestName kCodePlan{"c1600.code-plan"};          // {bytes, file?, address?, length?, cpu?} -> CodePlanReply
} // namespace requests

// What a file is.
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
    std::vector<std::string> tapeKinds;   // the kind of each, as `kind`
    bool same(const FileKindReply& o) const { return toValue() == o.toValue(); }
    calcu::sessions::Value toValue() const override;
};

// The plan, for the address dialog.
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

// What was loaded and how to keep and start it.
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

// The tape counter with the name of the tape in, and
// the CE-1600F's lamp. The media themselves are the media part's
// topic `media`. toValue: {floppyMotor, tapeMode ("play", "record", ""),
// tapeMotor, tapeSeconds, tapeFile}, as the Web page draws it.
struct LampsPayload final : calcu::sessions::PayloadOf<LampsPayload> {
    static constexpr calcu::sessions::PayloadKind kKind{"c1600.lamps"};
    TapeDeck::Status tape;   // the counter: position while playing, length while recording
    std::string tapeName;    // the tape in the cassette, "" none
    bool floppyMotor = false;   // the drive's motor runs: the green lamp
    bool same(const LampsPayload& o) const {
        return tape.mode == o.tape.mode && tape.motor == o.tape.motor && tape.position == o.tape.position &&
               tape.length == o.tape.length && tapeName == o.tapeName && floppyMotor == o.floppyMotor;
    }
    calcu::sessions::Value toValue() const override;
    // The cassette's counter as every GUI shows it: "0:00" without a
    // cassette (dimmed), "▶ m:ss" playing (the position), "● m:ss"
    // recording (the length); its tooltip.
    bool tapeEmpty() const { return tape.mode == TapeDeck::Mode::Empty; }
    std::string tapeCounter() const;
    std::string tapeTip() const;
};

// The serial port files.
struct SerialLinksPayload final : calcu::sessions::PayloadOf<SerialLinksPayload> {
    static constexpr calcu::sessions::PayloadKind kKind{"c1600.serial-links"};
    std::string rs232c, ce158;   // the path a client opens, empty while there is none
    std::uint64_t moves = 0;
    bool same(const SerialLinksPayload& o) const { return rs232c == o.rs232c && ce158 == o.ce158 && moves == o.moves; }
    calcu::sessions::Value toValue() const override;
};

} // namespace cu1600
