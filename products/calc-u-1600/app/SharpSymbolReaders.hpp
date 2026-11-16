#pragma once
#include <vector>

#include "debugger/Symbols.hpp"

// The symbol readers of Calc-U-1600 (LIB-10 design §10.3, D-5; spec AC-51):
// the labels and equates of an sdas listing (.lst, .rst), of a zasm
// listing (.lst), or of a `.SYMBOLS:` table, read with core/Debug/Listing.
// The harness's `symbols <file> [<target>]` and the DebuggerPart take them.

namespace shared {

std::vector<const calcu::debugger::SymbolReader*> sharpSymbolReaders();

} // namespace shared
