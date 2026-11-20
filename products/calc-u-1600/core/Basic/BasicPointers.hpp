#pragma once
#include <concepts>
#include <cstdint>

// Where BASIC keeps its program pointers, per model: big-endian words in
// the system work area. The PC-1600 keeps the PC-1500's trio at the same
// offset in its relocated work area; the values it stores there are
// LH5803-side addresses (pc1600::lh5803ToZ80 gives the Z-80's).

namespace sharp {

struct BasicPointerAddresses {
    std::uint16_t programStart;   // BASPRG_ST
    std::uint16_t programEnd;     // BASPRG_END, the program's FFH end mark
    std::uint16_t editHead;       // BASPRG_EDT (PC-1600: the edit / merge head)
    std::uint16_t variables;      // VAR_START / VARIABLE POINTER, the variables grow down from it
};

inline constexpr BasicPointerAddresses kPC1500BasicPointers{0x7865, 0x7867, 0x7869, 0x7899};
inline constexpr BasicPointerAddresses kPC1600BasicPointers{0xF865, 0xF867, 0xF869, 0xF899};

constexpr const BasicPointerAddresses& basicPointers(bool pc1600) {
    return pc1600 ? kPC1600BasicPointers : kPC1500BasicPointers;
}

// A big-endian word, the high byte at `a`, through a byte reader: a
// callable `peek(address)` or anything with a member `peek(address)`.
template <class Peek>
    requires std::invocable<const Peek&, std::uint16_t>
constexpr std::uint16_t readBE16(const Peek& peek, std::uint16_t a) {
    return static_cast<std::uint16_t>((peek(a) << 8) | peek(static_cast<std::uint16_t>(a + 1)));
}

template <class Memory>
    requires(!std::invocable<const Memory&, std::uint16_t>) &&
            requires(const Memory& m, std::uint16_t a) { m.peek(a); }
constexpr std::uint16_t readBE16(const Memory& m, std::uint16_t a) {
    return readBE16([&m](std::uint16_t at) { return m.peek(at); }, a);
}

} // namespace sharp
