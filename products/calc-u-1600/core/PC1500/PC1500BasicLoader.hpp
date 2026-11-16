#pragma once
#include <cstdint>
#include <string>
#include <vector>

#include "../Basic/BasicLoadResults.hpp"

namespace sharp {
class LoaderMemory;
}

// ── Fast BASIC program loading for the PC-1500 / PC-1500A ───────────────
//
// The keystroke typer drives the ROM's line editor one character at a time
// -- exact, but minutes of emulated time for a real program. This path takes
// the tokenized payload (sharp::loaders::loadBasic reads the file and
// tokenizes a listing), pokes it straight into the BASIC program area through
// the loader memory access (C1600-2 AC-14), appends the 0xFF program-end
// marker, and writes BASPRG_END.
//
// It does NOT run NEW0 and never resets the machine: this is LOAD semantics,
// not NEW+type. It works off whatever BASPRG_ST/BASPRG_END are currently
// live -- validates both are plausible, erases the resident program between
// them, pokes the new payload in from BASPRG_ST, and fixes up BASPRG_END. The
// caller is responsible for having prepared the machine first (memory cards,
// `NEW`, mode) exactly as on real hardware.
//
// Layout facts (cross-checked with the CLIs' --dump-basic, now `pc1600 dump-basic`, against the
// keystroke typer): the in-RAM line records are byte-identical to the
// transfer-file payload, the program ends with one 0xFF at BASPRG_END
// ($7867, big-endian), and BASPRG_ST ($7865) is the load address (stock
// $40C5, lower with a low-window RAM module).

/// The bare tokenized payload (no header) -- the run of in-RAM line records
/// basic::readBasicProgram() yields. Nothing is written when it is refused.
BasicLoadResult pc1500LoadBasicPayload(sharp::LoaderMemory& memory, const std::vector<uint8_t>& payload);
