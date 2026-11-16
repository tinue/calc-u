#pragma once
#include <cstdint>
#include <string>
#include <vector>

#include "../Basic/BasicLoadResults.hpp"

namespace sharp {
class LoaderMemory;
}

// ── Fast BASIC program loading for the PC-1600 ─────────────────────────
//
// The PC-1600 analog of Core/PC1500/PC1500BasicLoader: takes a BASIC
// listing (tokenized in-process) or an already-tokenized file
// (basic::readBasicProgram()), pokes the payload into the BASIC program
// area, appends the 0xFF end marker, and writes BASPRG_END -- instead of
// typing the program in character by character.
//
// It does NOT type anything and never resets the machine or changes MODE
// or TITLE: this is LOAD semantics, not NEW+type. The target is the program
// area TITLE (F1D5H) selects -- S0, or the S1 / S2 program module -- and the
// caller prepares the machine first (memory cards, `INIT`, `NEW`, `TITLE`,
// mode) exactly as on real hardware.
//
// What it does, mirroring the ROM's own LOAD
// (Ref/PC-1600/PC-1600-Work-Area-Map.md §3.5 / §4.5):
//   * Lays the line records down as LOADSTORE (rom3b 7074H) does: no line
//     straddles two module banks -- the ROM leaves a 00 00 bank-end mark and
//     continues in the next bank -- except across ADTBL entry 5 -> internal
//     RAM, which is contiguous. See pc1600::planS0Placement().
//   * Erases the resident program first (start through its end mark).
//   * Finishes as LOADEND (70E1H): the $FF end mark; S0: BASPRG_END (F867,
//     stored form), the end's ADTBL index (F02CH), VARIABLE POINTER reset if
//     it isn't above the new end; S1/S2: the descriptor's end triple and the
//     module header's end offset (+5/+6); then PRGADR (FE3C-FE41), CURRENT
//     TOP (F89E) and the CURRENT bank (F1C1) as PRGRESET sets them.
// Checked byte for byte against the keystroke typer's work area
// (Core/tests/pc1600_basicloader_tests.cpp).

/// The bare tokenized payload (no header) -- the run of in-RAM line records
/// basic::readBasicProgram() yields -- into the program area TITLE selects,
/// through the loader memory access (C1600-2 AC-14). sharp::loaders::loadBasic
/// reads the file and picks the keyword table by MODE.
BasicLoadResult pc1600LoadBasicPayload(sharp::LoaderMemory& memory, const std::vector<uint8_t>& payload);
