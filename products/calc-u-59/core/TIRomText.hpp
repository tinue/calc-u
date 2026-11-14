#pragma once
#include "MachineVariant.hpp"
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

// Decoding of the plaintext ROM dumps (Datamath museum format: header block,
// a "---" separator line, then address-prefixed data rows), moved into the
// core from app/RomLoader.cpp: the library hands raw ROM bytes, the core
// decodes (LIB-1 design D-4). Failures are return values (LIB-1 §2a).

/// The decoded ROM set of one model: the 6144-word instruction image and the
/// 64 constant rows as a flat 64×16-byte buffer.
struct TIRoms {
    std::vector<uint16_t> words;
    std::vector<uint8_t> constants;
};

/// A ROM .txt file: "AAAA: WWWW WWWW ..." rows after the separator. Rows
/// without a 4-hex-digit address are skipped; words are masked to 13 bits.
/// `error` is set on an invalid hex word.
std::vector<uint16_t> decodeRomText(std::string_view text, std::string& error);

/// A CONST-K .txt file: the 16-nibble rows of the "CONSTANT ROM (NUMBER)"
/// section, nibbles reversed into hardware register order. `error` is set on
/// an invalid hex nibble.
std::vector<std::vector<uint8_t>> decodeConstantsText(std::string_view text, std::string& error);

/// The model's three instruction files (in order) and two constant files
/// decoded into one set; checks the word and row counts and, on the TI-59,
/// the known-good sentinels (word 0 = 0x0A01 CLR IDL, word 6143 = 0x1987).
/// `error` names what is wrong; empty on success.
TIRoms decodeRomSet(MachineVariant v, const std::vector<std::string_view>& romTexts,
                    const std::vector<std::string_view>& constantTexts, std::string& error);
