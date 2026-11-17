#pragma once
#include "CueCard.hpp"
#include "Keycodes.hpp"
#include "../core/TIProductKinds.hpp"
#include "machine/Machine.hpp"
#include "presets/Vocabulary.hpp"
#include "sessions/SessionPart.hpp"
#include <functional>
#include <optional>
#include <string>

// Calc-U-59's keywords of the preset format (C59-2 design §3.1; LIB-6 §11.3),
// added through the place LIB-6 gives a product (ProductRules::addKeywords,
// LIB-6 AC-29); the loader itself knows none of them:
// - the sections `partition:` (C59-2 AC-2), `program:` (AC-1, AC-3), `data:`
//   (AC-4, EC-3) and `cue-card:` (AC-5, AC-6, EC-4, EC-5), each written
//   through the product kind `c59.program-memory` (AC-17) and finished when
//   written (D-8);
// - the step `printer-trace: on|off` (LIB-6 AC-40), the PC-100C's TRACE
//   switch through its controls (`printer/controls`).
// Every check that needs no machine is made when the preset is read, with
// the line (LIB-6 AC-6, AC-30).

namespace cu59 {

struct TIKeywordSetup {
    // `cue-card:` (AC-6): fit the card as the machine's own card (the app
    // part's); unset (the harness): the section only checks its block.
    std::function<void(calcu::sessions::SessionContext&, const CueCardContent&)> fitCueCard;
};

void addTIKeywords(calcu::presets::Vocabulary&, const TIKeywordSetup&);

/// A preset by its extension: `.ti59`, `.ti58`, `.ti58c` (LIB-6 AC-35).
bool isPresetFileName(std::string_view name);

/// The model's last program step by the catalogue's model ID (959, 479).
int lastStepOf(const calcu::machine::ModelId&);
/// The number of data registers of the model (100, 60) and whether it has H00–H03.
int dataRegistersOf(const calcu::machine::ModelId&);
bool hasExtraRegisters(const calcu::machine::ModelId&);

/// The kind `c59.program-memory` of a TI machine, or nullptr.
ProgramMemory* programMemoryOf(calcu::machine::Machine&);
/// The kind `c59.constant-memory` of a TI-58C, or nullptr.
ConstantMemory* constantMemoryOf(calcu::machine::Machine&);

/// `partition:`'s value (AC-2): the number of program steps (a multiple of
/// 80, at most 960), or the sentence why not.
std::variant<int, std::string> partitionSteps(std::string_view value, int lastStep);

/// A `data:` key (AC-4, D-6): the register (0–99) or H register (0–3), or the sentence why not.
struct DataKey {
    bool extra = false;
    int index = 0;
};
std::variant<DataKey, std::string> dataKey(std::string_view key, const calcu::machine::ModelId&);

/// A register as `data:` writes it (`c59 data-registers`, AC-9): 13 digits and the exponent (`-1.234567890123e-05`).
std::string registerText(const RegisterNibbles&);

} // namespace cu59
