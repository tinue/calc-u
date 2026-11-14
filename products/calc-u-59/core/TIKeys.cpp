#include "TIKeys.hpp"
#include <array>

namespace {

using S = std::string_view;
// The "Also" column of docs/formats/preset.md §8.1.
constexpr S k2nd[] = {"2ND"};
constexpr S kLnx[] = {"LNX"};
constexpr S kSwap[] = {"X⇄T", "x<>t"};
constexpr S kSquare[] = {"X²", "x^2"};
constexpr S kRoot[] = {"√X", "sqrt"};
constexpr S kRecip[] = {"1/X"};
constexpr S kPower[] = {"Yˣ", "y^x"};
constexpr S kDivide[] = {"/"};
constexpr S kTimes[] = {"x", "*"};
constexpr std::span<const S> kNone{};

constexpr std::array<TIKey, 45> kKeys{{
    {"A", kNone, 1, 1}, {"B", kNone, 1, 2}, {"C", kNone, 1, 3}, {"D", kNone, 1, 4}, {"E", kNone, 1, 5},
    {"2nd", k2nd, 2, 1}, {"INV", kNone, 2, 2}, {"lnx", kLnx, 2, 3}, {"CE", kNone, 2, 4}, {"CLR", kNone, 2, 5},
    {"LRN", kNone, 3, 1}, {"x⇄t", kSwap, 3, 2}, {"x²", kSquare, 3, 3}, {"√x", kRoot, 3, 4}, {"1/x", kRecip, 3, 5},
    {"SST", kNone, 4, 1}, {"STO", kNone, 4, 2}, {"RCL", kNone, 4, 3}, {"SUM", kNone, 4, 4}, {"yˣ", kPower, 4, 5},
    {"BST", kNone, 5, 1}, {"EE", kNone, 5, 2}, {"(", kNone, 5, 3}, {")", kNone, 5, 4}, {"÷", kDivide, 5, 5},
    {"GTO", kNone, 6, 1}, {"7", kNone, 6, 2}, {"8", kNone, 6, 3}, {"9", kNone, 6, 4}, {"×", kTimes, 6, 5},
    {"SBR", kNone, 7, 1}, {"4", kNone, 7, 2}, {"5", kNone, 7, 3}, {"6", kNone, 7, 4}, {"-", kNone, 7, 5},
    {"RST", kNone, 8, 1}, {"1", kNone, 8, 2}, {"2", kNone, 8, 3}, {"3", kNone, 8, 4}, {"+", kNone, 8, 5},
    {"R/S", kNone, 9, 1}, {"0", kNone, 9, 2}, {".", kNone, 9, 3}, {"+/-", kNone, 9, 4}, {"=", kNone, 9, 5},
}};

} // namespace

std::span<const TIKey> tiKeys() { return kKeys; }

const TIKey* tiKeyById(std::string_view id) {
    for (const auto& k : kKeys)
        if (k.id == id) return &k;
    return nullptr;
}
