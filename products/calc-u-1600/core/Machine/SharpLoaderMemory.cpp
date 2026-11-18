#include "Machine/SharpLoaderMemory.hpp"

#include "../PC1500/PC1500Machine.hpp"
#include "../PC1600/PC1600Machine.hpp"

namespace sharp {

// ---- PC-1500, PC-1500A ----

std::uint8_t PC1500LoaderMemory::peek(std::uint16_t a, Processor) const { return m_.memory().peek(a); }

LoaderMemory::Write PC1500LoaderMemory::writeBuiltIn(std::uint16_t address, const std::uint8_t* data,
                                                     std::size_t size) {
    if (static_cast<std::uint64_t>(address) + size > 0x10000) return {false, 0x10000};
    // The range first: each byte is written back with the
    // value it holds, which stores nothing new; a byte that is not stored
    // (ROM, open bus, a module that drops it) refuses the whole write.
    for (std::size_t i = 0; i < size; ++i) {
        const auto at = static_cast<std::uint16_t>(address + i);
        if (!m_.memory().poke(at, m_.memory().peek(at))) return {false, at};
    }
    for (std::size_t i = 0; i < size; ++i) m_.memory().poke(static_cast<std::uint16_t>(address + i), data[i]);
    return {true, 0};
}

// ---- PC-1600 ----

std::uint8_t PC1600LoaderMemory::peek(std::uint16_t a, Processor p) const {
    if (p == Processor::Main) return m_.debugPeek(a);
    bool readable = true;
    return m_.lh5803Memory().debugPeek(a, /*me1=*/false, &readable);
}

LoaderMemory::Mode PC1600LoaderMemory::mode() const { return m_.mode1() ? Mode::Mode1 : Mode::Mode0; }

int PC1600LoaderMemory::programArea() const { return m_.programAreaTitle(); }

LoaderMemory::Write PC1600LoaderMemory::writeBuiltIn(std::uint16_t address, const std::uint8_t* data,
                                                     std::size_t size) {
    if (address < 0xC000) return {false, address};
    if (static_cast<std::uint64_t>(address) + size > 0x10000) return {false, 0x10000};
    if (!m_.debugWriteInternalRam(static_cast<std::size_t>(address - 0xC000), data, size)) return {false, address};
    return {true, 0};
}

LoaderMemory::Write PC1600LoaderMemory::writeSlot(int bank, std::uint16_t address, const std::uint8_t* data,
                                                  std::size_t size) {
    if (bank < 0 || bank > 3 || address < 0x8000) return {false, address};
    if (static_cast<std::uint64_t>(address) + size > 0xC000) return {false, 0xC000};
    if (!m_.debugSlotBusWritable(bank, address, size)) {
        std::uint32_t miss = address;
        for (std::size_t i = 0; i < size; ++i)
            if (!m_.debugSlotBusWritable(bank, static_cast<std::uint16_t>(address + i), 1)) {
                miss = address + static_cast<std::uint32_t>(i);
                break;
            }
        return {false, miss};
    }
    if (!m_.debugWriteSlotBus(bank, address, data, size)) return {false, address};
    return {true, 0};
}

bool PC1600LoaderMemory::slotRam(int bank, std::uint16_t address, std::size_t size) const {
    if (bank < 0 || bank > 3 || address < 0x8000 || static_cast<std::uint64_t>(address) + size > 0xC000) return false;
    return m_.debugSlotBusWritable(bank, address, size);
}

LoaderMemory::Module PC1600LoaderMemory::module(int slot) const {
    Module out;
    if (slot != 1 && slot != 2) return out;
    const std::vector<std::uint8_t> image = m_.debugSlotImage(slot);
    out.present = !image.empty();
    out.size = static_cast<std::uint32_t>(image.size());
    const PC1600Machine::DebugBankState bs = m_.debugBankState();
    const int count = slot == 2 ? bs.slot2CardBankCount : bs.slot1CardBankCount;
    out.banks = count > 0 ? count : 0;
    return out;
}

std::vector<std::uint8_t> PC1600LoaderMemory::internalRam() const {
    std::vector<std::uint8_t> ram(PC1600Machine::kInternalRamSize);
    m_.debugCopyInternalRam(ram.data());
    return ram;
}

} // namespace sharp
