#pragma once
#include "../core/TICatalogue.hpp"
#include "machine/Ids.hpp"
#include <cstdint>
#include <optional>

// Matrix codes (row × 10 + column, row 1–9 top to bottom, column 1–5 left to
// right: the codes release 2.3.0's state files used in KEYSTROKES, now the positions of
// docs/formats/preset.md §8.1 "Position") and the machine's key IDs. Matrix
// codes are product data, not a core type; the key matrix itself stays in the
// core. The descriptor lists the keys in row order,
// so the key of code r×10+c is its key (r−1)×5 + (c−1).

namespace cu59 {

inline constexpr bool isValidMatrixCode(int code) {
    return code / 10 >= 1 && code / 10 <= 9 && code % 10 >= 1 && code % 10 <= 5;
}

/// Matrix code of the key at 0-based (row, col) — KeyboardLayout's key grid.
inline constexpr uint8_t matrixCode(int row, int col) { return static_cast<uint8_t>((row + 1) * 10 + (col + 1)); }

/// The key ID of a matrix code, or nullopt for an invalid code. The three
/// models share the keyboard.
inline std::optional<calcu::machine::KeyId> keyIdAt(std::uint8_t code) {
    if (!isValidMatrixCode(code)) return std::nullopt;
    const auto& keys = tiCatalogue().models().front().keys;
    return keys[static_cast<size_t>((code / 10 - 1) * 5 + (code % 10 - 1))].id;
}

inline std::optional<std::uint8_t> matrixCodeOf(const calcu::machine::KeyId& id) {
    const auto& keys = tiCatalogue().models().front().keys;
    for (size_t i = 0; i < keys.size(); ++i)
        if (keys[i].id == id) return static_cast<std::uint8_t>((i / 5 + 1) * 10 + (i % 5 + 1));
    return std::nullopt;
}

} // namespace cu59
