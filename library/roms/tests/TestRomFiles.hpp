#pragma once
#include "TestMachines.hpp"
#include "roms/Lookup.hpp"
#include "TempFolder.hpp"
#include <atomic>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <vector>

// ROM files made for the tests: a temporary folder
// with a file for every entry of the test catalogue (library/machine/tests/
// TestMachines: binary entries, and the text entry test-dot-font.txt of 64
// rows), a test text reader, and catalogues with one file that two entries
// need and with two entries of one name that differ.

namespace calcu::roms::test {

namespace fs = std::filesystem;
using namespace calcu::machine;
using namespace calcu::machine::test;

using calcu::testing::TempFolder;

// The text reader of the tests: a row is a non-empty line; a line ending
// in \n, \r\n or \r is one line. A line "bad" cannot be read.
class TestTextReader : public ProductRomReader {
public:
    Decoded decode(const RomEntry&, std::span<const std::uint8_t> bytes) const override {
        Decoded d;
        std::string line;
        auto endLine = [&] {
            if (line == "bad") d.error = "a line the reader does not know";
            if (!line.empty()) ++d.amount;
            line.clear();
        };
        for (std::uint8_t b : bytes) {
            if (b == '\n' || b == '\r') endLine();
            else line += static_cast<char>(b);
        }
        endLine();
        return d;
    }
    // A product check over the whole set: optional, off by default.
    std::vector<FileProblem> checkSet(const RomSet& set) const override {
        ++checkSetCalls;
        if (!rejectSet) return {};
        return {FileProblem{set.fileNames().front(), Problem::ProductCheck, "the product says no", {}}};
    }
    bool rejectSet = false;
    mutable int checkSetCalls = 0;
};

// The 64 rows of test-dot-font.txt, with `eol` after each.
inline std::string fontText(std::string_view eol = "\n", int rows = 64) {
    std::string s;
    for (int i = 0; i < rows; ++i) s += "row " + std::to_string(i) + std::string(eol);
    return s;
}

// Every file of `set`: binary files of their size, filled with `fill`; the
// text file as fontText().
inline void writeFullSet(const TempFolder& folder, const FullSet& set, char fill = 'x') {
    for (const auto& f : set.files) {
        if (f.entry.encoding == RomEncoding::Text) folder.write(f.entry.fileName, fontText());
        else folder.write(f.entry.fileName, std::string(static_cast<std::size_t>(f.entry.decodedAmount), fill));
    }
}

// The test catalogue plus a peripheral that needs test-module-8k.rom too.
inline Catalogue catalogueWithSharedFile() {
    Catalogue c = makeTestCatalogue();
    PeripheralDescriptor twin;
    twin.id = PeripheralId{"test-twin"};
    twin.name = "Test twin of the 8k module";
    twin.roms = c.peripheral(kModule)->romsFor(VariantId{"8k"});
    twin.fits = {PinOutId{"test.bus"}};
    c.addPeripheral(twin);
    return c;
}

// The test catalogue plus a peripheral whose entry for test-seg.rom differs.
inline Catalogue catalogueWithConflict() {
    Catalogue c = makeTestCatalogue();
    PeripheralDescriptor odd;
    odd.id = PeripheralId{"test-odd"};
    odd.name = "Test peripheral with another test-seg.rom";
    RomEntry e = c.model(kSeg)->roms.front();
    e.decodedAmount = 1024;
    odd.roms = {e};
    c.addPeripheral(odd);
    return c;
}

inline const FileProblem* problemOf(const RomsMissing& m, const std::string& name) {
    for (const auto& f : m.files)
        if (f.fileName == name) return &f;
    return nullptr;
}

} // namespace calcu::roms::test
