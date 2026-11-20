#include "ConstantMemoryFile.hpp"
#include "Bcd.hpp"
#include "CueCardYaml.hpp"
#include "Keycodes.hpp"
#include "formats/Flow.hpp"
#include "formats/Yaml.hpp"
#include "media/Header.hpp"
#include <cstdio>
#include <sstream>

namespace cu59 {

using calcu::formats::YamlNode;

namespace {

ConstantMemoryProblem damaged(int line, const std::string& what) {
    return {ConstantMemoryProblem::Kind::Damaged, (line > 0 ? "line " + std::to_string(line) + ": " : std::string()) + what};
}

bool allDigits(std::string_view s) {
    if (s.empty()) return false;
    for (char c : s)
        if (c < '0' || c > '9') return false;
    return true;
}

} // namespace

std::variant<ConstantMemoryContents, ConstantMemoryProblem> readConstantMemoryFile(std::string_view text) {
    YamlNode root;
    std::string error;
    if (!calcu::formats::parseYaml(std::string(text), &root, &error)) return damaged(0, error);
    calcu::media::Header header;
    if (!calcu::media::readHeader(root, {"ti58c-constant-memory", "constant memory", 1, ""}, &header, &error)) {
        if (header.formatVersion > 1) return ConstantMemoryProblem{ConstantMemoryProblem::Kind::Newer, {}};
        return damaged(0, error);
    }
    ConstantMemoryContents out;
    for (const auto& [k, n] : root.map)
        if (k != "format" && k != "format-version" && k != "memory-name" && k != "description" && k != "saved" && k != "cue-card" && k != "ram")
            return damaged(n.line, "unknown key " + k);
    if (const YamlNode* card = root.find("cue-card")) {
        std::string why;
        int at = 0;
        auto c = readStandAloneCueCard(*card, [&](int line, const std::string& m) {
            if (why.empty()) why = m, at = line;
        });
        if (!c) return damaged(at, why);
        out.cueCard = *c;
    }
    const YamlNode* ram = root.find("ram");
    if (!ram || !ram->isMap()) return damaged(ram ? ram->line : 0, "ram: is required, with encoding: register-hex and bytes: |");
    for (const auto& [k, n] : ram->map)
        if (k != "encoding" && k != "bytes") return damaged(n.line, "ram: has no key " + k);
    const YamlNode* encoding = ram->find("encoding");
    if (!encoding || calcu::formats::scalarText(*encoding) != "register-hex")
        return damaged(encoding ? encoding->line : ram->line, "ram: encoding: must be register-hex");
    const YamlNode* bytes = ram->find("bytes");
    if (!bytes) return out;   // no register lines
    if (!bytes->isScalar()) return damaged(bytes->line, "ram: bytes: is a block (bytes: |)");
    std::istringstream in(bytes->scalar);
    const std::vector<int> lines = blockLineNumbers(text, bytes->line, 10'000);
    std::size_t index = 0;
    for (std::string l; std::getline(in, l); ++index) {
        const int lineNo = index < lines.size() ? lines[index] : bytes->line;
        const std::size_t first = l.find_first_not_of(' ');
        if (first == std::string::npos) continue;
        const std::size_t colon = l.find(':');
        const std::string name = colon == std::string::npos ? std::string() : l.substr(first, colon - first);
        if (name.size() != 4 || name[0] != 'R' || !allDigits(name.substr(1)))
            return damaged(lineNo, "a register line is Rnnn: and 8 hex bytes");
        const int r = std::stoi(name.substr(1));
        if (r >= ConstantMemory::kRegisters) return damaged(lineNo, name + " is above R063");
        const auto b = parseRegisterLineHex(l.substr(colon + 1));
        if (!b) return damaged(lineNo, "a register line is Rnnn: and 8 hex bytes");
        out.ram[static_cast<std::size_t>(r)] = decodeRegisterLine(*b);
    }
    return out;
}

std::string writeConstantMemoryFile(const ConstantMemoryContents& c, const std::string& savedAt) {
    calcu::media::Header header;
    header.comment = "Calc-U-59 TI-58C constant memory";
    header.format = "ti58c-constant-memory";
    header.savedAt = savedAt;
    std::string out = calcu::media::headerText(header);
    if (c.cueCard) out += "cue-card:\n" + writeStandAloneCueCard(*c.cueCard, 2);
    out += "ram:\n  encoding: register-hex\n";
    std::string rows;
    for (int r = 0; r < ConstantMemory::kRegisters; ++r) {
        const RegisterNibbles& n = c.ram[static_cast<std::size_t>(r)];
        bool zero = true;
        for (auto x : n) zero = zero && x == 0;
        if (zero) continue;
        char name[8];
        std::snprintf(name, sizeof name, "R%03d", r);
        rows += std::string("    ") + name + ": " + registerLineHex(n) + "\n";
    }
    if (!rows.empty()) out += "  bytes: |\n" + rows;
    return out;
}

} // namespace cu59
