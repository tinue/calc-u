#pragma once
#include "CueCard.hpp"
#include "../core/TIProductKinds.hpp"
#include <array>
#include <optional>
#include <string>
#include <string_view>
#include <variant>

// The TI-58C's constant memory file `ti58c.memory.yaml`, constant memory
// format 1 (docs/formats/media.md §4.1; C59-3 AC-4, EC-2, EC-3, EC-5, design
// §3.2): `format: ti58c-constant-memory`, `format-version: 1`, an optional
// stand-alone `cue-card`, and `ram` with `encoding: register-hex` and the
// raw registers R000–R063 that are not zero, H00–H03 as R060–R063, in
// today's `Rnnn:` lines (app/Bcd.hpp). Read with the library's YAML reader.

namespace cu59 {

inline constexpr const char* kConstantMemoryFileName = "ti58c.memory.yaml";

struct ConstantMemoryContents {
    std::array<RegisterNibbles, ConstantMemory::kRegisters> ram{};
    std::optional<CueCardContent> cueCard;
    bool operator==(const ConstantMemoryContents&) const = default;
};

/// Why a file is not read: an older or newer format (EC-2), or a damaged one (EC-3).
struct ConstantMemoryProblem {
    enum class Kind { Outdated, Newer, Damaged } kind = Kind::Damaged;
    std::string what;   // Damaged: "line <n>: <what>"
};

std::variant<ConstantMemoryContents, ConstantMemoryProblem> readConstantMemoryFile(std::string_view text);
// `savedAt`: ISO-8601 UTC of the write, the `saved:` key (left out when empty).
std::string writeConstantMemoryFile(const ConstantMemoryContents&, const std::string& savedAt = {});

} // namespace cu59
