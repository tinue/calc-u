#include "PC1600MachineCodeLoader.hpp"

#include <algorithm>
#include <cstdio>

#include "../Machine/SharpProductKinds.hpp"

bool loadPC1600MachineCode(sharp::LoaderMemory& memory, int slot, uint32_t addr, const uint8_t* data, size_t len,
                           std::string* error, int bank) {
    if (slot == 0) {
        if (addr < 0xC000 || static_cast<uint64_t>(addr) + len > 0x10000) {
            char b[160];
            std::snprintf(b, sizeof(b), "load $%04X + %zu bytes is outside the S0 internal-RAM window ($C000-$FFFF)",
                          addr, len);
            *error = b;
            return false;
        }
        if (!memory.writeBuiltIn(static_cast<uint16_t>(addr), data, len).stored) {
            *error = "backing-store write failed for slot S0 (empty slot, or a module with no writable RAM)";
            return false;
        }
        return true;
    }
    const char* slotName = (slot == 1) ? "S1" : "S2";
    if (addr < 0x8000 || addr >= 0xC000 || static_cast<uint64_t>(addr) + len > 0x10000) {
        char b[192];
        std::snprintf(b, sizeof(b), "load $%04X + %zu bytes does not start in the %s memory-slot window ($8000-$BFFF)",
                      addr, len, slotName);
        *error = b;
        return false;
    }
    if (static_cast<int>(machinecode::pc1600BankSlot(bank)) != slot) {
        char b[96];
        std::snprintf(b, sizeof(b), "bank %d is not in the %s memory slot", bank, slotName);
        *error = b;
        return false;
    }
    // Past $BFFF the code continues in internal RAM, as BLOAD / CLOAD M
    // write it. The window's range is checked before anything is written
    // (the internal RAM is RAM throughout).
    const size_t inWindow = std::min<size_t>(len, 0xC000 - addr);
    if (!memory.slotRam(bank, static_cast<uint16_t>(addr), inWindow)) {
        char b[160];
        std::snprintf(b, sizeof(b),
                      "no RAM for $%04X + %zu bytes in slot %s, bank %d (empty slot, or a module with no writable "
                      "RAM there)",
                      addr, inWindow, slotName, bank);
        *error = b;
        return false;
    }
    if (!memory.writeSlot(bank, static_cast<uint16_t>(addr), data, inWindow).stored) {
        *error = "the slot write failed";
        return false;
    }
    if (inWindow < len && !memory.writeBuiltIn(0xC000, data + inWindow, len - inWindow).stored) {
        *error = "internal-RAM write failed";
        return false;
    }
    return true;
}

pc1600::SlotGeometry pc1600SlotGeometry(const sharp::LoaderMemory& memory, int slot) {
    pc1600::SlotGeometry g;
    const sharp::LoaderMemory::Module m = memory.module(slot);
    g.present = m.present;
    g.imageSize = m.size;
    // The BASIC program area only ever occupies bank 0 of the module (a
    // vertically banked CE-1601M rests at Port 28H = 0): the image divided
    // by the bank count for a banked card, the whole image otherwise.
    g.bankSize = m.banks > 0 ? g.imageSize / static_cast<uint32_t>(m.banks) : g.imageSize;
    return g;
}

machinecode::PC1600State pc1600LoadState(const sharp::LoaderMemory& memory) {
    machinecode::PC1600State st;
    auto peek = [&memory](uint16_t a) { return memory.peek(a); };
    st.mode1 = memory.mode() == sharp::LoaderMemory::Mode::Mode1;
    st.title = memory.programArea();
    st.basicAreas = pc1600BasicAreas(memory);
    st.ramEnd = pc1600::lh5803ToZ80(static_cast<uint16_t>(memory.peek(0xF864) << 8));
    if (st.title == 1 || st.title == 2) {
        const pc1600::SlotDescriptor d = pc1600::readSlotDescriptor(peek, st.title);
        st.titleBase = static_cast<uint32_t>(d.basePage) << 8;
        st.titleStart = d.start;
    }
    for (int bank = 0; bank < 4; bank++) {
        for (uint32_t page = 0; page < 64; page++) {
            if (memory.slotRam(bank, static_cast<uint16_t>(0x8000 + page * 0x100), 0x100))
                st.bankRamPages[static_cast<size_t>(bank)] |= uint64_t{1} << page;
        }
    }
    return st;
}

std::vector<machinecode::BasicArea> pc1600BasicAreas(const sharp::LoaderMemory& memory) {
    pc1600::PlacementInput in;
    in.peek = [&memory](uint16_t a) { return memory.peek(a); };
    in.slot1 = pc1600SlotGeometry(memory, 1);
    in.slot2 = pc1600SlotGeometry(memory, 2);
    const pc1600::PlacementResult plan = pc1600::planS0Placement(in, {});
    std::vector<machinecode::BasicArea> areas;
    if (!plan.ok) return areas;
    for (const pc1600::ProgramSegment& seg : plan.segments) {
        machinecode::BasicArea a;
        a.slot = seg.kind == pc1600::ProgramSegment::Kind::InternalRam ? 0 : seg.slot;
        a.windowBase = seg.windowBase;
        a.top = seg.top;
        a.bank = seg.adtblBank;
        areas.push_back(a);
    }
    return areas;
}
