#pragma once
#include <string>

#include "Basic/BasicLoadResults.hpp"
#include "machine/Machine.hpp"
#include "sessions/Operation.hpp"
#include "typing/TypingEngine.hpp"

// ── Typing a BASIC listing in ────────────────────────────────────────────
//
// Inside an operation, through the typing engine: each
// line of the listing, an empty line left out, is typed through the ROM's
// editor with ENTER; then the loader waits until the machine reports prompt
// readiness (at most 12 s of emulated time) and until the end of the
// program has not moved for 30 frames (at most 6 s). A line is stored when
// the end of the program or the line's own record changed
// (basic::LineStoreCheck, read through the loader memory access). On the
// PC-1600 a line of more than 79 characters is not typed and counts as not
// stored. The whole listing is checked for keys first: a character without
// a key refuses the command before anything is typed. The loader never
// presses MODE: the machine must be in program mode. It reads no machine
// class.

namespace sharp::loaders {

inline constexpr int kMaxTypedLineLength = 79;   // PC-1600, characters

calcu::sessions::Task<BasicTypeResult> typeProgram(calcu::sessions::OperationContext& ctx,
                                                   calcu::typing::TypingEngine& engine,
                                                   calcu::machine::Machine& machine, std::string listing);

} // namespace sharp::loaders
