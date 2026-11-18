#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "../MachineCodeFile.hpp"
#include "PC1600ProgramPlacement.hpp"

namespace sharp {
class LoaderMemory;
}

// Writes a machine-language block linearly into one PC-1600 load target
// through the loader memory access: `slot` 0 = S0,
// the internal RAM ($C000-$FFFF); 1 / 2 = the memory slots' $8000-$BFFF
// window, in global `bank` (machinecode::LoadPlan::bank; ignored for S0) --
// a block that runs on past $BFFF continues in internal RAM, as BLOAD /
// CLOAD M write it. `addr` is the Z-80 (SC7852) address. Whichever bank is
// switched in does not matter. All or nothing: both ranges of a block that
// crosses $BFFF are checked before either is written; false, writing
// nothing, with `error` set.
bool loadPC1600MachineCode(sharp::LoaderMemory& memory, int slot, uint32_t addr, const uint8_t* data, size_t len,
                           std::string* error, int bank);

// Geometry of the module in `slot` (1 or 2) as the program-placement logic
// needs it (PC1600ProgramPlacement.hpp). Shared with the fast BASIC loader.
pc1600::SlotGeometry pc1600SlotGeometry(const sharp::LoaderMemory& memory, int slot);

// The live BASIC program area ("S0") as machinecode::BasicArea runs, in
// ADTBL order -- the first is where `NEW "S0:",<size>` reserves from. Read
// from the work area (S0MTb / ADTBL / BASPRG_ST) via the placement logic;
// empty if that can't be read (e.g. a machine that hasn't booted).
std::vector<machinecode::BasicArea> pc1600BasicAreas(const sharp::LoaderMemory& memory);

// What the machine-code placement rules read (machinecode::PC1600State):
// MODE, TITLE, the S0 areas, for TITLE S1/S2 that module's window base
// and program start from its slot descriptor, and which pages of banks
// 0-3 are RAM.
machinecode::PC1600State pc1600LoadState(const sharp::LoaderMemory& memory);
