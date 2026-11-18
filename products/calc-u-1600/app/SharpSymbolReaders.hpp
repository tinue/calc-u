#pragma once
#include <vector>

#include "debugger/Symbols.hpp"

// The symbol readers of Calc-U-1600:
// the labels and equates of an sdas listing (.lst, .rst), of a zasm
// listing (.lst), or of a `.SYMBOLS:` table, read with core/Debug/Listing.
// The harness's `symbols <file> [<target>]` and the DebuggerPart take them.

namespace cu1600 {

std::vector<const calcu::debugger::SymbolReader*> sharpSymbolReaders();

} // namespace cu1600
