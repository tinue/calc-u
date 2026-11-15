#pragma once
#include "opening/OpenRequest.hpp"
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

// The tests of AC-1 in their order (LIB-13 design §3.2; spec AC-1, AC-2,
// AC-6, AC-10, EC-1, EC-5). Names are compared in lower case (ASCII).

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

// AC-2 (Calc-U-1600 core/DropFile.cpp:20-37): UTF-8 without a zero byte,
// after an optional byte-order mark, with a line that starts with `model:`
// in the first column.
bool looksLikePreset(const std::vector<std::uint8_t>& bytes);

// AC-6: whether a drag of one file of that name (and size, when the
// frontend knows it) would be taken: tests 1b (against the drop's limit),
// 1c, 2 and 3, or a kind's name.
bool wouldTake(std::string_view name, std::optional<std::uint64_t> size, const OpenSetup&);

// Helpers the part shares.
std::string lowerAscii(std::string_view);
std::string fileNameOf(const OpenedFile&);   // the name, else the last part of the path
bool endsWith(std::string_view lowerName, std::string_view lowerSuffix);
std::string sizeText(std::uint64_t bytes);   // "1 MiB", "64 KiB", "100 bytes" (D-5)

} // namespace calcu::opening
