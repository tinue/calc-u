#include "SolidStateRomModule.hpp"
#include <algorithm>
#include <cstring>

SolidStateRomModule::SolidStateRomModule(const std::uint8_t* rom, std::size_t count) {
    if (rom) std::memcpy(m_data, rom, std::min(count, kBytes));
}

void SolidStateRomModule::operation() {
    m_addr = 0;
    m_readPos = 0;
    m_wasWriting = false;
}

std::uint16_t SolidStateRomModule::word(ModuleWord w, std::uint16_t kr) {
    switch (w) {
    case ModuleWord::Fetch: {   // LIB.IN: one byte; the address advances (wraps at 5000)
        const auto ext = static_cast<std::uint16_t>(byteAt(m_addr++) << 4);
        m_addr %= kBytes;
        return ext;
    }
    case ModuleWord::AddressOut:
        // LIB.PC.OUT: the address is decimal; the ROM shifts it in one digit
        // at a time, most significant first, four words for a full address:
        //   new = old / 10 + KR[7:4] * 1000
        if (!m_wasWriting) m_readPos = 0;   // switching from reading to writing
        m_addr = static_cast<std::uint16_t>((m_addr / 10) + ((kr >> 4 & 0xFu) * 1000));
        m_readPos = (m_readPos + 1) % 4;
        m_wasWriting = true;
        return 0;
    case ModuleWord::AddressIn: {   // LIB.PC.IN: the digit at the read position; the address stays
        if (m_wasWriting) m_readPos = 0;   // switching from writing to reading
        static constexpr std::uint16_t divisors[4] = {1, 10, 100, 1000};
        const auto ext = static_cast<std::uint16_t>(((m_addr / divisors[m_readPos]) % 10) << 4);
        m_readPos = (m_readPos + 1) % 4;
        m_wasWriting = false;
        return ext;
    }
    case ModuleWord::FetchHigh:   // LIB.HI: the high nibble of the current byte, no advance
        return static_cast<std::uint16_t>(byteAt(m_addr) & 0xF0u);
    }
    return 0;
}
