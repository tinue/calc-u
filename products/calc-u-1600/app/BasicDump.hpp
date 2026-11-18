#pragma once
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "Machine/SharpProductKinds.hpp"

// `c1600 dump-basic`: the
// pointers of BASIC and the bytes of the program area, read through the
// loader memory kind (the PC-1500's ME0, the PC-1600's Z-80 view). The
// lines are those the old CLIs' `--dump-basic` printed, without their
// `---` headings.

namespace cu1600 {

struct BasicPointer {
    std::string name;                   // "BASPRG_END"
    std::uint16_t address = 0;          // where it is kept ($7867)
    std::uint16_t value = 0;
    std::optional<std::uint16_t> z80;   // PC-1600: the Z-80's address for it
    std::optional<int> length;          // BASPRG_END: end - start
};

struct BasicRow {
    std::uint16_t address = 0;
    std::vector<std::uint8_t> bytes;   // 16
};

struct BasicDump {
    std::vector<std::string> lines;   // the reply
    std::vector<BasicPointer> pointers;
    std::vector<BasicRow> rows;       // empty when the start does not lie before the end
};

BasicDump basicDump(const sharp::LoaderMemory&);

} // namespace cu1600
