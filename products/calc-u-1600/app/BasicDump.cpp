#include "app/BasicDump.hpp"

#include <cstdio>

namespace cu1600 {

namespace {

std::string hex4(unsigned v) {
    char b[8];
    std::snprintf(b, sizeof b, "%04X", v & 0xFFFFu);
    return b;
}
std::string hex2(unsigned v) {
    char b[4];
    std::snprintf(b, sizeof b, "%02X", v & 0xFFu);
    return b;
}

} // namespace

BasicDump basicDump(const sharp::LoaderMemory& mem) {
    BasicDump d;
    auto pk = [&](std::uint16_t a) { return mem.peek(a); };
    auto be16 = [&](std::uint16_t a) {
        return static_cast<std::uint16_t>((pk(a) << 8) | pk(static_cast<std::uint16_t>(a + 1)));
    };
    auto le16 = [&](std::uint16_t a) {
        return static_cast<std::uint16_t>(pk(a) | (pk(static_cast<std::uint16_t>(a + 1)) << 8));
    };
    std::uint32_t from = 0, to = 0;   // the program area in the main processor's view
    if (mem.pc1600()) {
        const std::uint16_t st = be16(0xF865), end = be16(0xF867), edt = be16(0xF869);
        // F865/F867/F869 hold LH5803-side addresses; the LH5803's $0000-$7FFF
        // aliases the SC7852's $8000-$FFFF, so the Z-80 address is value +
        // $8000: $C0C5 stock ($40C5), $80C5 with the program in a module.
        auto toZ80 = [](std::uint16_t a) -> std::uint16_t { return a < 0x8000 ? static_cast<std::uint16_t>(a + 0x8000) : a; };
        const std::uint16_t stZ = toZ80(st), endZ = toZ80(end);
        const int len = static_cast<int>(end) - static_cast<int>(st);
        d.pointers = {{"BASPRG_ST", 0xF865, st, stZ, std::nullopt},
                      {"BASPRG_END", 0xF867, end, endZ, len},
                      {"BASPRG_EDT", 0xF869, edt, std::nullopt, std::nullopt},
                      {"F5CF", 0xF5CF, le16(0xF5CF), std::nullopt, std::nullopt},
                      {"F89D", 0xF89D, le16(0xF89D), std::nullopt, std::nullopt},
                      {"VARIABLE_PTR", 0xF899, be16(0xF899), std::nullopt, std::nullopt}};
        d.lines.push_back("BASPRG_ST  $F865 = $" + hex4(st) + "  (Z-80 $" + hex4(stZ) + ")");
        d.lines.push_back("BASPRG_END $F867 = $" + hex4(end) + "  (Z-80 $" + hex4(endZ) + ", len = " + std::to_string(len) + ")");
        d.lines.push_back("BASPRG_EDT $F869 = $" + hex4(edt));
        d.lines.push_back("F5CF (LE, RAM base) = $" + hex4(le16(0xF5CF)) + "   F89D (LE) = $" + hex4(le16(0xF89D)));
        d.lines.push_back("VARIABLE_PTR $F899 = $" + hex4(be16(0xF899)));
        if (stZ < endZ) {
            from = stZ;
            to = static_cast<std::uint32_t>(endZ) + 16;
        }
    } else {
        const std::uint16_t st = be16(0x7865), end = be16(0x7867), edt = be16(0x7869);
        const std::uint16_t varStart = be16(0x7899), dataPtr = be16(0x78BE);
        const int len = static_cast<int>(end) - static_cast<int>(st);
        d.pointers = {{"BASPRG_ST", 0x7865, st, std::nullopt, std::nullopt},
                      {"BASPRG_END", 0x7867, end, std::nullopt, len},
                      {"BASPRG_EDT", 0x7869, edt, std::nullopt, std::nullopt},
                      {"VAR_START", 0x7899, varStart, std::nullopt, std::nullopt},
                      {"DATA_PTR", 0x78BE, dataPtr, std::nullopt, std::nullopt},
                      {"RAM_END", 0x7864, pk(0x7864), std::nullopt, std::nullopt},
                      {"WARM_START", 0x7A20, pk(0x7A20), std::nullopt, std::nullopt}};
        d.lines.push_back("BASPRG_ST  $7865 = $" + hex4(st));
        d.lines.push_back("BASPRG_END $7867 = $" + hex4(end) + "   (len = " + std::to_string(len) + ")");
        d.lines.push_back("BASPRG_EDT $7869 = $" + hex4(edt));
        d.lines.push_back("VAR_START  $7899 = $" + hex4(varStart));
        d.lines.push_back("DATA_PTR   $78BE = $" + hex4(dataPtr));
        d.lines.push_back("RAM_END    $7864 = $" + hex2(pk(0x7864)) + " (page)   WARM_START $7A20 = $" + hex2(pk(0x7A20)));
        if (st < end) {
            from = st;
            to = static_cast<std::uint32_t>(end) + 16;
        }
    }
    // EC-4: the rows only when the start lies before the end.
    for (std::uint32_t a = from; from < to && a <= to && a <= 0xFFFF; a += 16) {
        BasicRow row;
        row.address = static_cast<std::uint16_t>(a);
        std::string line = hex4(a) + ": ";
        std::string chars;
        for (std::uint32_t i = 0; i < 16; ++i) {
            const std::uint8_t b = pk(static_cast<std::uint16_t>(a + i));
            row.bytes.push_back(b);
            line += hex2(b) + " ";
            chars += (b >= 0x20 && b < 0x7F) ? static_cast<char>(b) : '.';
        }
        d.lines.push_back(line + " |" + chars + "|");
        d.rows.push_back(std::move(row));
    }
    return d;
}

} // namespace cu1600
