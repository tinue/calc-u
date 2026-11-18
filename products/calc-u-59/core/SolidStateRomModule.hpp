#pragma once
#include "ModuleSlotPinOut.hpp"
#include <cstddef>
#include <cstdint>

// A Solid State ROM module as a peripheral in the place `slot-1`. A core object like the PC-100C: TIMachine's
// plug() and unplug() own it. Its ROM (at most 5000 bytes, the rest zero)
// comes in with the attach, decoded from the module file the request
// carries; it reads no file. It
// holds the image, the address register and its digit read position.

class SolidStateRomModule final : public ModuleSlotPinOut {
public:
    static constexpr std::size_t kBytes = 5000;
    SolidStateRomModule(const std::uint8_t* rom, std::size_t count);

    std::uint16_t word(ModuleWord, std::uint16_t kr) override;
    std::uint16_t address() const override { return m_addr; }
    std::uint8_t addressReadPos() const override { return m_readPos; }

    /// A reset, power cycle or cold boot of the machine: the
    /// address register and its read position clear.
    void operation();

    /// The address latches; the image comes from the ROM set.
    template <class Io>
    void stateIo(Io& io) {
        io.begin("MOD ");
        io(m_addr)(m_readPos)(m_wasWriting);
        io.end();
    }

private:
    // LIB.PC.OUT can shift in an address up to 9999; past the image the module reads 0.
    std::uint8_t byteAt(std::uint16_t a) const { return a < kBytes ? m_data[a] : 0; }
    std::uint8_t m_data[kBytes]{};
    std::uint16_t m_addr = 0;
    std::uint8_t m_readPos = 0;      // digit read position of LIB.PC.IN (0-3, cycles)
    bool m_wasWriting = false;       // direction: true after LIB.PC.OUT, false after LIB.PC.IN
};
