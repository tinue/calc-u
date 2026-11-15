#include "TIRomReader.hpp"
#include "ModuleFile.hpp"
#include "TICatalogue.hpp"
#include "TIRomText.hpp"
#include "roms/RomSet.hpp"
#include <algorithm>
#include <string_view>

using namespace calcu::machine;
using calcu::roms::FileProblem;
using calcu::roms::Problem;

namespace {

std::string_view textOf(std::span<const std::uint8_t> bytes) {
    return {reinterpret_cast<const char*>(bytes.data()), bytes.size()};
}

std::string hex4(unsigned v) {
    static const char* d = "0123456789ABCDEF";
    std::string s = "0x";
    for (int sh = 12; sh >= 0; sh -= 4) s += d[(v >> sh) & 0xF];
    return s;
}

constexpr std::string_view kModuleSuffix = ".rom.yaml";

bool isModuleFile(const std::string& name) {
    return name.size() > kModuleSuffix.size() && name.compare(name.size() - kModuleSuffix.size(), kModuleSuffix.size(), kModuleSuffix) == 0;
}

} // namespace

TIRomReader::Decoded TIRomReader::decode(const RomEntry& entry, std::span<const std::uint8_t> bytes) const {
    Decoded d;
    if (isModuleFile(entry.fileName)) {
        TIModuleFile m = readModuleFile(textOf(bytes), d.error);
        if (!d.error.empty()) return d;
        const std::string id = entry.fileName.substr(0, entry.fileName.size() - kModuleSuffix.size());
        if (m.moduleName != id) {
            d.error = "module-name is '" + m.moduleName + "', not '" + id + "'";
            return d;
        }
        d.amount = m.rom.size();
        return d;
    }
    if (entry.unit == ContentUnit::Rows) d.amount = decodeConstantsText(textOf(bytes), d.error).size();
    else d.amount = decodeRomText(textOf(bytes), d.error).size();
    if (!d.error.empty()) d.amount = 0;
    return d;
}

std::vector<FileProblem> TIRomReader::checkSet(const calcu::roms::RomSet& set) const {
    std::vector<FileProblem> problems;
    std::vector<const std::vector<RomEntry>*> checked;
    for (const auto& model : tiCatalogue().models()) {
        const auto& entries = model.romsFor(std::nullopt);
        // The TI-58 has the TI-59's files: checked once, as the TI-59's.
        bool same = false;
        for (const auto* c : checked)
            if (c->size() == entries.size() && std::equal(c->begin(), c->end(), entries.begin(),
                                                          [](const RomEntry& a, const RomEntry& b) { return a.fileName == b.fileName; }))
                same = true;
        if (same) continue;
        checked.push_back(&entries);
        std::vector<std::uint16_t> words;
        std::size_t rows = 0;
        std::string error;
        for (const auto& e : entries) {
            const auto* b = set.bytes(e.fileName);
            if (!b) continue;
            if (e.unit == ContentUnit::Rows) {
                rows += decodeConstantsText(textOf(*b), error).size();
            } else {
                auto w = decodeRomText(textOf(*b), error);
                words.insert(words.end(), w.begin(), w.end());
            }
        }
        const std::string& first = entries.front().fileName;
        if (words.size() != 6144)
            problems.push_back({first, Problem::ProductCheck,
                                model.model.value + "'s instruction files give " + std::to_string(words.size()) +
                                    " words, not 6144", {}});
        if (rows != 64)
            problems.push_back({entries.back().fileName, Problem::ProductCheck,
                                model.model.value + "'s constant files give " + std::to_string(rows) + " rows, not 64", {}});
        if (model.model.value != "TI-59" || words.size() != 6144) continue;
        // The known-good TI-59 dump: word 0 = 0x0A01 (CLR IDL), word 6143 = 0x1987.
        std::string lastFile;
        for (const auto& e : entries)
            if (e.unit == ContentUnit::Words) lastFile = e.fileName;
        if (words.front() != 0x0A01)
            problems.push_back({first, Problem::ProductCheck,
                                "first word of the TI-59 instruction ROM is " + hex4(words.front()) + ", expected 0x0A01", {}});
        if (words.back() != 0x1987)
            problems.push_back({lastFile, Problem::ProductCheck,
                                "last word of the TI-59 instruction ROM is " + hex4(words.back()) + ", expected 0x1987", {}});
    }
    return problems;
}
