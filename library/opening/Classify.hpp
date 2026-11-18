#pragma once
#include "machine/Names.hpp"
#include "opening/OpenRequest.hpp"
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

// What a file is opened as, by tests in their order: 1a not readable,
// 1b larger than the way's limit, 1c a hidden file (all three refused);
// 2 a preset by its extension, 3 a medium by its suffix, 4 the first of the
// product's file kinds that takes it, 5 a preset by its content; 6 refused.
// Names are compared in lower case (ASCII).

namespace calcu::opening {

struct OpenSetup;
class FileKind;

struct Classified {
    enum class What { Refused, Preset, Medium, Kind };
    What what = What::Refused;
    std::string refusal;          // Refused: the sentence, which names the file
    FileKind* kind = nullptr;     // Kind
};

Classified classify(const OpenedFile&, Way, const OpenSetup&);

// Test 5: UTF-8 without a zero byte,
// after an optional byte-order mark, with a line that starts with `model:`
// in the first column.
bool looksLikePreset(const std::vector<std::uint8_t>& bytes);

// Whether a drag of one file of that name (and size, when the
// frontend knows it) would be taken: tests 1b (against the drop's limit),
// 1c, 2 and 3, or a kind's name.
bool wouldTake(std::string_view name, std::optional<std::uint64_t> size, const OpenSetup&);

// Helpers the part shares.
using machine::lowerAscii;   // the library's one ASCII lower case (machine/Names.hpp)
std::string fileNameOf(const OpenedFile&);   // the name, else the last part of the path
bool endsWith(std::string_view lowerName, std::string_view lowerSuffix);
std::string sizeText(std::uint64_t bytes);   // "1 MiB", "64 KiB", "100 bytes"

} // namespace calcu::opening
