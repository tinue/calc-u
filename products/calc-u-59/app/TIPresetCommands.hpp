#pragma once
#include "sessions/SessionPart.hpp"

// The product commands of C59-2 AC-9 (design §3.4, D-9): what the sections
// `partition:`, `program:` and `data:` do, and their reads, through the
// product kind `c59.program-memory`. A part of its own, so that the app
// session (TIAppPart's parts) and the harness entry (until M-59 it makes its
// own library parts) both have it:
//
// | command         | arguments                                   | reply (Done's result, kind `c59.lines`) |
// |-----------------|---------------------------------------------|----------------------------------------|
// | `c59.partition` | {value?: "479"}                             | without a value: ["479"] (the last step) |
// | `c59.program`   | {step?: "010", keycodes: "76 11 …"}         | —                                       |
// | `c59.program`   | {step: "010", count: n}                     | ["010 76", "011 11", …]                 |
// | `c59.registers` | {set: [["05", "1.5"], …]}                  | —                                       |
// | `c59.registers` | {get: ["05", "H01", …]}                     | ["05: 1.5e0", …]                        |
//
// A write is refused where the section would refuse the preset, with its
// sentence, and while an operation runs ("Another operation is running.");
// the reads are not. A write changes memory at once, between two host calls.

namespace cu59 {

namespace commands {
inline const calcu::sessions::CommandName kPartition{"c59.partition"};
inline const calcu::sessions::CommandName kProgram{"c59.program"};
inline const calcu::sessions::CommandName kRegisters{"c59.registers"};
} // namespace commands

class ProgramMemoryPart final : public calcu::sessions::SessionPart {
public:
    void attach(calcu::sessions::SessionSetup&) override;
};

} // namespace cu59
