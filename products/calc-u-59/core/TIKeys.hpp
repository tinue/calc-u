#pragma once
#include <cstdint>
#include <span>
#include <string_view>

// The 45 keys of the TI-58/58C/59 (C59-1 AC-9, AC-10): key ID, the other
// spellings of docs/formats/preset.md §8.1, and the key's row (1-9, top to
// bottom) and column (1-5, left to right). Row r, column c is the key that
// the matrix code r × 10 + c names in 2.3.0's state files. Core-internal: above the
// core a key is a calcu::machine::KeyId from the descriptor.

struct TIKey {
    std::string_view id;
    std::span<const std::string_view> otherSpellings;
    int row, col;
};

/// The keys in row order, left to right (the descriptor's order).
std::span<const TIKey> tiKeys();

/// The key whose ID is `id` exactly (no other spelling), or nullptr.
const TIKey* tiKeyById(std::string_view id);

/// K-line bit of each column (index 0 unused): the core's key matrix holds a
/// key as (K-line bit, digit slot = row).
inline constexpr int kTIKeyLineBits[6] = {0, 1, 2, 3, 5, 6};
