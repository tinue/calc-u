#pragma once
#include "roms/RomReader.hpp"

// The reader Calc-U-59 supplies to the ROM lookup (LIB-12 AC-8, AC-14, EC-2;
// design §3.2): every file of its full set is text. The Datamath dumps are
// decoded by the core's TIRomText (an instruction file in words, a constant
// file in rows); a module file `<ID>.rom.yaml` by ModuleFile (its ROM in
// bytes, and its module-name must be <ID>). The checks of the set are those
// of 2.3.0: per model the instruction files give 6144 words and the
// constant files 64 rows, and the TI-59's instruction ROM starts with 0x0A01
// and ends with 0x1987 (spec OQ-1).

class TIRomReader final : public calcu::roms::ProductRomReader {
public:
    Decoded decode(const calcu::machine::RomEntry&, std::span<const std::uint8_t> bytes) const override;
    std::vector<calcu::roms::FileProblem> checkSet(const calcu::roms::RomSet&) const override;
};
