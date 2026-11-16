#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "CpuRegisters.hpp"
#include "SourceMap.hpp"

namespace calcu::machine {
class Machine;
}

// ── Build & Load ─────────────────────────────────────────────────────────
//
// Puts a freshly assembled program into the machine through the one plan
// and writer of "Load Machine Code…" (no dialog: the address
// comes from the request or the file's header) and binds its listing to
// exactly the loaded range, replacing whatever listing an earlier load of
// that range brought. That is how the debugger knows which listing
// belongs to which bytes.

namespace debug {

struct LoadRequest {
    std::string bin;                  ///< the program (.bin, headerless or with a CE-158 / PC-1600 header)
    std::string listing;              ///< its listing (.lst / .rst); empty = none
    std::string source;               ///< the listing's main source, if not derivable
    std::vector<std::string> symbols; ///< extra .SYMBOLS: files
    int thread = 1;                   ///< the CPU it's for (PC-1600: 1 = Z-80, 2 = LH5803)
    BankKey key;
    bool hasAddress = false;          ///< else the header's, else (headerless) the listing's lowest address
    uint32_t address = 0;             ///< in `thread`'s address space
    bool hasEntry = false;            ///< else entrySymbol, else the header's autorun address, else the
    uint16_t entry = 0;               ///< listing's ENTRY (if in the loaded range), else the load address
    std::string entrySymbol;          ///< a symbol of the listing, or an address as text (&, 0x or $ hex, else decimal)
};

struct LoadResult {
    bool ok = false;
    std::string error;
    uint16_t lo = 0, hi = 0;          ///< the loaded range, in `thread`'s address space
    uint16_t entry = 0;
    std::string callCommand;          ///< the BASIC command that runs it, e.g. "CALL &40C5"
    int binding = 0;                  ///< the SourceMap binding of its listing (0 = no listing)
    std::vector<std::string> warnings;
};

/// Plans and writes through the loaders of C1600-2 (sharp::loaders::planCode,
/// loadCode; design §3.10). Verifies the bound listing against memory right
/// away (`match` and `peek`: the debug targets' bank state and
/// side-effect-free reads, by thread).
LoadResult loadProgram(calcu::machine::Machine& machine, const LoadRequest& request, SourceMap& map,
                       const BankMatch& match, const CodePeek& peek);

} // namespace debug
