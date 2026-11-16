#pragma once
#include <cstddef>
#include <cstdint>
#include <string>

namespace sharp {
class LoaderMemory;
}

// Writes a machine-language block into the PC-1500/1500A at `addr` through
// the loader memory access (C1600-2 AC-10, AC-14): all or nothing -- a byte
// of the range that is not RAM (ROM, open bus, a module that drops it)
// writes nothing and `error` says where.
bool loadPC1500MachineCode(sharp::LoaderMemory& memory, uint32_t addr, const uint8_t* data, size_t len,
                           std::string* error);

// The ROM's user-RAM bounds: RAM_ST / RAM_END hold page numbers (the high
// byte of the first valid / first invalid page).
constexpr uint16_t kPc1500RamStPage = 0x7863;
constexpr uint16_t kPc1500RamEndPage = 0x7864;

// User RAM as the ROM reports it: `start` = RAM_ST * 256, `end` = RAM_END *
// 256 (one past the last byte).
void pc1500UserRam(const sharp::LoaderMemory& memory, uint32_t* start, uint32_t* end);
