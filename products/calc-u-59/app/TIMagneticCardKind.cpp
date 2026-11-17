#include "TIMagneticCardKind.hpp"
#include "CueCardYaml.hpp"
#include "Bcd.hpp"
#include "Keycodes.hpp"
#include "TextUtil.hpp"
#include "formats/Flow.hpp"
#include "formats/Yaml.hpp"
#include <cstdio>
#include <sstream>

namespace cu59 {

using calcu::formats::YamlNode;
using calcu::media::MediumCodec;
using calcu::media::ReadOutcome;

namespace {

constexpr int kPowerOnProgramRegs = 60;   // the TI-59 at power-on: 479.59

std::string lower(std::string s) {
    for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}

std::string text(const YamlNode& n) { return calcu::formats::scalarText(n); }

std::string hex2(unsigned v) {
    char b[8];
    std::snprintf(b, sizeof b, "0x%02X", v & 0xFFu);
    return b;
}

std::string quoted(const std::string& v) { return v.find('"') == std::string::npos ? "\"" + v + "\"" : "'" + v + "'"; }

// A byte value written `0x1C` (or a decimal number), any case (README.md §1, §4).
std::optional<int> byteValue(const std::string& t) {
    const std::string l = lower(t);
    if (l.empty()) return std::nullopt;
    try {
        std::size_t used = 0;
        const int v = l.rfind("0x", 0) == 0 ? std::stoi(l.substr(2), &used, 16) + 0 : std::stoi(l, &used, 10);
        if (used != (l.rfind("0x", 0) == 0 ? l.size() - 2 : l.size()) || v < 0 || v > 255) return std::nullopt;
        return v;
    } catch (...) {
        return std::nullopt;
    }
}

struct Problem {
    std::string why;
};

// One side of `sides:` (media.md §1.1): every key required, exactly 30 rows.
std::optional<std::vector<std::uint8_t>> readSide(const YamlNode& n, const std::string& side, Problem& p) {
    auto fail = [&](int line, const std::string& m) {
        p.why = (line > 0 ? "line " + std::to_string(line) + ": " : std::string()) + "side " + side + ": " + m;
        return std::nullopt;
    };
    if (!n.isMap()) return fail(n.line, "is a block of keys.");
    for (const auto& [k, v] : n.map)
        if (k != "partition" && k != "data-type" && k != "bank" && k != "protection" && k != "checksum" && k != "encoding" &&
            k != "bytes")
            return fail(v.line, "has no key " + k + ".");
    for (const char* k : {"partition", "data-type", "bank", "protection", "checksum", "encoding", "bytes"})
        if (!n.find(k)) return fail(n.line, std::string("needs ") + k + ".");
    std::vector<std::uint8_t> d(kCardBankBytes, 0);
    const auto partition = byteValue(text(*n.find("partition")));
    if (!partition) return fail(n.find("partition")->line, "partition: is a byte, as 0x1C.");
    d[0] = static_cast<std::uint8_t>(*partition);
    const std::string type = lower(text(*n.find("data-type")));
    if (type != "program" && type != "data") return fail(n.find("data-type")->line, "data-type: is program or data.");
    d[1] = type == "program" ? 0x11 : 0x10;
    const std::string bank = text(*n.find("bank"));
    if (bank.size() != 1 || bank[0] < '1' || bank[0] > '4') return fail(n.find("bank")->line, "bank: is 1 to 4.");
    d[2] = pageByteOfBank(bank[0] - '0');
    const std::string protection = lower(text(*n.find("protection")));
    if (protection != "true" && protection != "false") return fail(n.find("protection")->line, "protection: is true or false.");
    d[3] = protection == "true" ? 0x10 : 0x00;
    const auto checksum = byteValue(text(*n.find("checksum")));
    if (!checksum) return fail(n.find("checksum")->line, "checksum: is a byte, as 0x02.");
    d[244] = d[245] = static_cast<std::uint8_t>(*checksum);
    if (lower(text(*n.find("encoding"))) != "register-hex") return fail(n.find("encoding")->line, "encoding: is register-hex.");
    const YamlNode& bytes = *n.find("bytes");
    if (!bytes.isScalar()) return fail(bytes.line, "bytes: is a block (bytes: |).");
    std::istringstream in(bytes.scalar);
    std::size_t rows = 0;
    for (std::string l; std::getline(in, l);) {
        const std::size_t first = l.find_first_not_of(' ');
        if (first == std::string::npos) continue;
        const std::size_t colon = l.find(':');
        const std::string label = colon == std::string::npos ? std::string() : l.substr(first, colon - first);
        bool digits = label.size() == 4 && label[0] == 'R';
        for (std::size_t i = 1; digits && i < 4; ++i) digits = label[i] >= '0' && label[i] <= '9';
        if (!digits) return fail(bytes.line, "a row is Rnnn: and 8 hex bytes.");
        const auto b = parseRegisterLineHex(l.substr(colon + 1));
        if (!b) return fail(bytes.line, "a row is Rnnn: and 8 hex bytes.");
        if (rows >= 30) return fail(bytes.line, "bytes: has more than 30 rows.");
        std::copy(b->begin(), b->end(), d.begin() + static_cast<long>(4 + rows * 8));
        ++rows;
    }
    if (rows != 30) return fail(bytes.line, "bytes: has " + std::to_string(rows) + " rows, not 30.");
    return d;
}

class MagneticCardCodec final : public MediumCodec {
public:
    ReadOutcome read(const std::string& bytes, const Companion&) const override {
        ReadOutcome out;
        auto unreadable = [&](const std::string& why) {
            out.status = ReadOutcome::Status::Unreadable;
            out.problem = why;
            return out;
        };
        // EC-5: a mark for the byte order, or bytes that are not UTF-8.
        if (bytes.rfind("\xEF\xBB\xBF", 0) == 0 || !isValidUtf8(bytes)) return unreadable("It is not a text file in UTF-8.");
        YamlNode root;
        std::string error;
        if (!calcu::formats::parseYaml(bytes, &root, &error)) return unreadable(error);
        if (!root.isMap()) return unreadable("It is no magnetic card file.");
        const YamlNode* format = root.find("format");
        if (!format || lower(text(*format)) != "ti59-magnetic-card") return unreadable("It is no magnetic card file (format: ti59-magnetic-card).");
        const YamlNode* version = root.find("format-version");
        if (!version) return unreadable("It has no format-version.");
        const std::string v = text(*version);
        int ver = 0;
        try {
            std::size_t used = 0;
            ver = std::stoi(v, &used);
            if (used != v.size()) ver = 0;
        } catch (...) {
            ver = 0;
        }
        if (ver <= 0) return unreadable("Its format-version is no number.");
        if (ver < kMagneticCardFormatVersion) {   // LIB-7 AC-9
            out.status = ReadOutcome::Status::Outdated;
            out.info.formatVersion = ver;
            if (const YamlNode* n = root.find("card-name")) out.info.name = text(*n);
            return out;
        }
        if (ver > kMagneticCardFormatVersion) return unreadable("Magnetic card format version " + v + " is not supported.");
        for (const auto& [k, n] : root.map)
            if (k != "format" && k != "format-version" && k != "card-name" && k != "description" && k != "template" &&
                k != "saved" && k != "cue-card" && k != "sides")
                return unreadable("line " + std::to_string(n.line) + ": unknown key " + k + ".");
        auto card = std::make_shared<TIMagneticCardData>();
        auto label = std::make_shared<TICardLabel>();
        const YamlNode* name = root.find("card-name");
        if (!name || text(*name).empty()) return unreadable("It has no card-name.");
        label->name = text(*name);
        if (const YamlNode* d = root.find("description")) label->description = text(*d);
        if (const YamlNode* t = root.find("template")) {
            const std::string tv = lower(text(*t));
            if (tv != "true" && tv != "false") return unreadable("line " + std::to_string(t->line) + ": template: is true or false.");
            label->isTemplate = tv == "true";
        }
        if (const YamlNode* c = root.find("cue-card")) {
            std::string why;
            int at = 0;
            auto cue = readStandAloneCueCard(*c, [&](int line, const std::string& m) {
                if (why.empty()) why = m, at = line;
            });
            if (!cue) return unreadable("line " + std::to_string(at) + ": " + why);
            label->cueCard = *cue;
        }
        const YamlNode* sides = root.find("sides");
        if (!sides) return unreadable("It has no sides.");
        if (sides->isMap()) {
            for (const auto& [k, n] : sides->map) {
                if (k != "0" && k != "1") return unreadable("line " + std::to_string(n.line) + ": sides: has 0 and 1 only.");
                Problem p;
                auto side = readSide(n, k, p);
                if (!side) return unreadable(p.why);
                card->sides[k == "1" ? 1 : 0] = std::move(*side);
            }
        } else if (!sides->isNull()) {
            return unreadable("line " + std::to_string(sides->line) + ": sides: is a block with 0 and 1.");
        }
        card->label = label;
        out.status = ReadOutcome::Status::Read;
        out.info.name = label->name;
        out.info.description = label->description;
        out.info.isTemplate = label->isTemplate;
        out.info.formatVersion = ver;
        out.info.hasWriteProtect = false;   // AC-29: a bank's protection is the calculator's
        out.info.content = card;
        return out;
    }

    std::vector<calcu::media::FileOut> write(const Write& w, std::string& problem) const override {
        const auto card = magneticCardOf(w.content);
        if (!card) {
            problem = "There is no magnetic card to write.";
            return {};
        }
        TIMagneticCardData out = *card;
        std::string description;
        std::optional<CueCardContent> cue;
        if (const TICardLabel* l = cardLabelOf(*card)) {
            description = l->description;
            cue = l->cueCard;
        }
        if (w.source) {   // AC-5, EC-3: the file as it is, only the written side replaced
            const ReadOutcome r = read(*w.source, {});
            if (r.status != ReadOutcome::Status::Read) {
                problem = "Its file cannot be read" + (r.problem.empty() ? std::string(".") : ": " + r.problem);
                return {};
            }
            const auto from = magneticCardOf(r.info.content);
            if (card->lastWritten && from) {
                out = *from;
                out.sides[static_cast<std::size_t>(*card->lastWritten)] = card->sides[static_cast<std::size_t>(*card->lastWritten)];
            }
            if (const TICardLabel* l = from ? cardLabelOf(*from) : nullptr) {
                description = l->description;
                cue = l->cueCard;
            }
        }
        std::string name = w.name;
        if (name.empty()) name = w.fileStem;
        if (name.find('"') != std::string::npos || name.find('\n') != std::string::npos) {
            problem = "A card name has no \" and no line end.";
            return {};
        }
        return {calcu::media::FileOut{{}, writeMagneticCardFile(out, name, description, w.asTemplate, cue, w.savedAt)}};
    }
};

} // namespace

std::string writeMagneticCardFile(const TIMagneticCardData& card, const std::string& name, const std::string& description,
                                  bool isTemplate, const std::optional<CueCardContent>& cueCard, const std::string& savedAt) {
    std::string out = "format: ti59-magnetic-card\nformat-version: 3\ncard-name: " + quoted(name) + "\n";
    if (!description.empty()) out += "description: " + quoted(description) + "\n";
    if (isTemplate) out += "template: true\n";
    if (!savedAt.empty()) out += "saved: " + savedAt + "\n";
    CueCardContent cue;
    if (cueCard) cue = *cueCard;
    else cue.title = name;   // media.md §1.4: a card saved without a cue card
    out += "cue-card:\n" + writeStandAloneCueCard(cue, 2);
    std::string sides;
    for (std::size_t i = 0; i < 2; ++i) {
        std::vector<std::uint8_t> d;
        if (const auto* b = std::get_if<std::vector<std::uint8_t>>(&card.sides[i])) d = *b;
        else if (const auto* def = std::get_if<TICardSideDef>(&card.sides[i])) d = buildCardBank(*def, kPowerOnProgramRegs);
        if (d.size() != kCardBankBytes) continue;
        const int bank = bankOfPageByte(d[2]);
        sides += "  " + std::to_string(i) + ":\n";
        sides += "    partition: " + hex2(d[0]) + "\n";
        sides += std::string("    data-type: ") + (d[1] == 0x11 ? "program" : "data") + "\n";
        sides += "    bank: " + std::to_string(bank) + "\n";
        sides += std::string("    protection: ") + (d[3] == 0x10 ? "true" : "false") + "\n";
        sides += "    checksum: " + hex2(d[244]) + "\n";
        sides += "    encoding: register-hex\n    bytes: |\n";
        for (int r = 0; r < 30; ++r) {
            char row[64];
            const std::uint8_t* b = d.data() + 4 + 8 * r;
            std::snprintf(row, sizeof row, "      R%03d: %02X %02X %02X %02X %02X %02X %02X %02X\n", (bank - 1) * 30 + r, b[0], b[1],
                          b[2], b[3], b[4], b[5], b[6], b[7]);
            sides += row;
        }
    }
    out += sides.empty() ? "sides: {}\n" : "sides:\n" + sides;
    return out;
}

std::shared_ptr<const TIMagneticCardData> magneticCardOf(const calcu::media::Content& c) {
    return std::dynamic_pointer_cast<const TIMagneticCardData>(c);
}

const TICardLabel* cardLabelOf(const TIMagneticCardData& c) { return dynamic_cast<const TICardLabel*>(c.label.get()); }

CueCardContent fittedCardCue(const TIMagneticCardData* card, const std::string& name) {
    CueCardContent cue;
    if (const TICardLabel* l = card ? cardLabelOf(*card) : nullptr; l && l->cueCard) cue = *l->cueCard;
    else cue.title = name;
    cue.template_ = CueCardTemplate::MagnetCard;
    if (card) {
        cue.bankLeft = sideBank(card->sides[0]);
        cue.bankRight = sideBank(card->sides[1]);
    }
    return cue;
}

const calcu::media::KindDecl& magneticCardKind() {
    static const MagneticCardCodec codec;
    static const calcu::media::KindDecl kind = [] {
        calcu::media::KindDecl k;
        k.id = kMagneticCardKind;
        k.label = "magnetic card";
        k.suffix = ".magnetcard.yaml";
        k.folder = calcu::settings::Folder::Save;
        k.codec = &codec;
        k.formatName = "Magnetic card format";
        k.describedAt = "docs/formats/media.md §1";
        k.currentVersion = kMagneticCardFormatVersion;
        k.implicitTemplates = {calcu::media::ImplicitTemplate{kFreshCardTemplate, "A fresh magnetic card",
                                                              [] { return std::make_shared<TIMagneticCardData>(); }}};
        k.words.insertStep = "card";
        k.words.armStep = "arm";
        k.words.inFileKey = "card";
        k.words.insertNeedsTime = true;
        k.rereadBeforeWrite = true;   // D-3
        k.offersNameAndSave = false;  // D-5
        k.replaceQuestion = "Replace card “{name}”? Its first write replaces both sides and the cue card.";
        return k;
    }();
    return kind;
}

} // namespace cu59
