#pragma once
#include <cstdint>
#include <map>
#include <string>
#include <string_view>
#include <vector>

#include "opening/FileKind.hpp"
#include "opening/OpenRequest.hpp"
#include "sessions/Payload.hpp"
#include "sessions/Topics.hpp"

// ── Calc-U-1600's file kinds for opening a file (LIB-13 Part D) ─────────
//
// The library's open part (library/opening/, LIB-13) tells presets by their
// extensions and model line and media by their suffixes; what is left goes
// to the product's file kinds, asked in this order (AC-17, D4.1): the BASIC
// program, then the machine-code program, each told by C1600-2's one
// classifier (programfile::classify, core/ProgramFile.hpp):
//   - a BASIC listing or tokenized BASIC: basic-program;
//   - machine code behind a CE-158 or PC-1600 header: machine-code-program;
//   - headerless bytes the classifier takes for code, named `.bin` (any
//     case; `.rom` is no name of a machine-code file, AC-17): machine code;
//   - a cassette WAV: the kind of its first readable file; a WAV without
//     one goes to BASIC, whose loader says why;
//   - empty, other: neither (the open part's test 5 or 6 follows).
// What follows (AC-18, D4.2, D-11): in the harness the kind loads the file
// at once (a WAV's first file; headerless code is refused, it needs an
// address); by every way with a window it sends the stream topic
// `c1600.program-opened`, and the frontend runs the flow of File ▸ Load
// BASIC Program… or Load Machine Code… on it, without their file dialog.

namespace cu1600 {

// AC-17: the product's preset extensions, lower case.
std::vector<std::string> presetExtensions();
// AC-20, D-15: 1 MiB for a drop, Open With and the start argument; none for
// the dialog and the harness.
inline constexpr std::uint64_t kMaxOpenBytes = 1u << 20;
std::map<calcu::opening::Way, std::uint64_t> openLimits();

enum class ProgramKind { None, Basic, MachineCode };
// AC-17's rows: what kind of program a file of this name is.
ProgramKind programKindOf(const std::vector<std::uint8_t>& bytes, std::string_view name);
// The kind's name in the open reply and the topic: "basic-program",
// "machine-code-program".
std::string_view programKindName(ProgramKind);

namespace topics {
inline const calcu::sessions::TopicName kProgramOpened{"c1600.program-opened"};
}

// D4.2: a program file opened by a way with a window, for the frontend's
// program flow: its name, bytes and kind, and its path where the host has it.
struct ProgramOpenedPayload final : calcu::sessions::PayloadOf<ProgramOpenedPayload> {
    static constexpr calcu::sessions::PayloadKind kKind{"c1600.program-opened"};
    std::string name;
    std::string kind;   // programKindName
    std::vector<std::uint8_t> bytes;
    std::string path;   // empty: none
    bool same(const ProgramOpenedPayload& o) const {
        return name == o.name && kind == o.kind && bytes == o.bytes && path == o.path;
    }
    calcu::sessions::Value toValue() const override;
};

class ProgramFileKind : public calcu::opening::FileKind {
public:
    std::string_view name() const override { return programKindName(m_which); }
    // D4.3, D-13: C1600-2 has the content decide, never the name.
    bool takesName(std::string_view) const override { return true; }
    bool takes(const calcu::opening::OpenedFile&) const override;
    calcu::sessions::CommandOutcome open(const calcu::opening::OpenedFile&, calcu::opening::Way,
                                         calcu::sessions::SessionContext&) override;
    // The stream topic of D4.2 (nullptr in the harness: the kind loads).
    void setTopic(calcu::sessions::StreamTopic* topic) { m_topic = topic; }

protected:
    explicit ProgramFileKind(ProgramKind which) : m_which(which) {}
    ~ProgramFileKind() = default;

private:
    ProgramKind m_which;
    calcu::sessions::StreamTopic* m_topic = nullptr;
};

class BasicProgramKind final : public ProgramFileKind {
public:
    BasicProgramKind() : ProgramFileKind(ProgramKind::Basic) {}
};

class MachineCodeKind final : public ProgramFileKind {
public:
    MachineCodeKind() : ProgramFileKind(ProgramKind::MachineCode) {}
};

} // namespace cu1600
