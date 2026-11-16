#include "PC1500MachineCodeLoader.hpp"

#include <cstdio>

#include "../Machine/SharpProductKinds.hpp"

bool loadPC1500MachineCode(sharp::LoaderMemory& memory, uint32_t addr, const uint8_t* data, size_t len,
                           std::string* error) {
    if (static_cast<uint64_t>(addr) + len > 0x10000) {
        char b[96];
        std::snprintf(b, sizeof(b), "&%X + %zu bytes runs past &FFFF", addr, len);
        *error = b;
        return false;
    }
    const sharp::LoaderMemory::Write w = memory.writeBuiltIn(static_cast<uint16_t>(addr), data, len);
    if (w.stored) return true;
    char b[160];
    std::snprintf(b, sizeof(b), "&%X is not RAM (ROM or no memory there) -- the code at &%X-&%X did not load",
                  static_cast<unsigned>(w.firstMiss), addr, static_cast<unsigned>(addr + len - 1));
    *error = b;
    return false;
}

void pc1500UserRam(const sharp::LoaderMemory& memory, uint32_t* start, uint32_t* end) {
    *start = static_cast<uint32_t>(memory.peek(kPc1500RamStPage)) << 8;
    *end = static_cast<uint32_t>(memory.peek(kPc1500RamEndPage)) << 8;
}
