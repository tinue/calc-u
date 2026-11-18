#pragma once
#include <cstdint>

// The module slot's functional pin-out. Signals: VGG, VDD, instruction words, calculating status,
// external access, Φ2, Φ1, common. The module drives EXT on the words that
// read; the address register is the module's, as on the TMC0540
// (https://www.datamath.org/Chips/TMC0540.htm). An empty slot is no object:
// the CPU then reads EXT = 0 on every word.

/// LIB.IN, LIB.PC.OUT, LIB.PC.IN, LIB.HI.
enum class ModuleWord : std::uint8_t { Fetch, AddressOut, AddressIn, FetchHigh };

class ModuleSlotPinOut {
public:
    virtual ~ModuleSlotPinOut() = default;
    /// An instruction word with KR as data; the EXT it drives (0 for AddressOut).
    virtual std::uint16_t word(ModuleWord, std::uint16_t kr) = 0;
    /// Debug view: the address register (CpuFrame::m_libAddr) and its digit
    /// read position (CpuFrame::m_libAddrReadPos).
    virtual std::uint16_t address() const = 0;
    virtual std::uint8_t addressReadPos() const = 0;
};
