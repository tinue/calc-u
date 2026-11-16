#include "TIMagneticCardsKey.hpp"
#include "Bcd.hpp"
#include "CueCardYaml.hpp"
#include "Keycodes.hpp"
#include "TIMagneticCardKind.hpp"
#include "TIPresetKeys.hpp"
#include "formats/Flow.hpp"
#include "media/PresetMedia.hpp"
#include "presets/Reader.hpp"
#include <set>

namespace cu59 {

using calcu::formats::YamlNode;
using calcu::presets::ReadContext;

namespace {

const calcu::machine::ModelId kTI59{"TI-59"};

std::string text(const YamlNode& n) { return calcu::formats::scalarText(n); }
int lineOf(const YamlNode& n, int fallback) { return n.line ? n.line : fallback; }

// A side of an item (AC-18): the definition, or nullopt with the error given.
std::optional<TICardSideDef> readSide(const YamlNode& n, const std::string& name, const std::string& side, ReadContext& ctx) {
    const std::string where = "magnetic-cards: " + name + " side " + side + ": ";
    const int at = n.line;
    if (!n.isMap()) {
        ctx.error(at, where + "is a block with bank:.");
        return std::nullopt;
    }
    for (const auto& [k, v] : n.map)
        if (k != "bank" && k != "protected" && k != "partition" && k != "program" && k != "data") {
            ctx.error(lineOf(v, at), where + "has no key " + k + ".");
            return std::nullopt;
        }
    TICardSideDef def;
    const YamlNode* bank = n.find("bank");
    const std::string b = bank ? text(*bank) : std::string();
    if (b.size() != 1 || b[0] < '1' || b[0] > '4') {
        ctx.error(bank ? lineOf(*bank, at) : at, where + "bank: is required, 1 to 4.");
        return std::nullopt;
    }
    def.bank = b[0] - '0';
    if (const YamlNode* p = n.find("protected")) {
        const std::string t = calcu::presets::lowerAscii(text(*p));
        if (t != "true" && t != "false") {
            ctx.error(lineOf(*p, at), where + "protected: is true or false.");
            return std::nullopt;
        }
        def.isProtected = t == "true";
    }
    if (const YamlNode* p = n.find("partition")) {
        auto r = partitionSteps(text(*p), lastStepOf(kTI59));
        if (auto* why = std::get_if<std::string>(&r)) {
            ctx.error(lineOf(*p, at), where + *why);
            return std::nullopt;
        }
        def.partitionSteps = std::get<int>(r);
    }
    if (const YamlNode* p = n.find("program")) {
        if (!p->isScalar()) {
            ctx.error(lineOf(*p, at), where + "program: is a block of keycodes (program: |).");
            return std::nullopt;
        }
        auto r = readKeycodes(p->scalar, lastStepOf(kTI59));
        if (auto* e = std::get_if<KeycodeError>(&r)) {
            ctx.error(lineOf(*p, at), where + "program: " + e->message);
            return std::nullopt;
        }
        // Bare keycodes start at the bank's first step; a listing names its steps.
        bool listing = false;
        for (std::size_t i = 0; i < p->scalar.size();) {
            const std::size_t end = p->scalar.find('\n', i);
            const std::string line = p->scalar.substr(i, end == std::string::npos ? std::string::npos : end - i);
            i = end == std::string::npos ? p->scalar.size() : end + 1;
            const std::size_t first = line.find_first_not_of(' ');
            if (first == std::string::npos || line[first] == '#') continue;
            std::size_t digits = 0;
            while (first + digits < line.size() && std::isdigit(static_cast<unsigned char>(line[first + digits]))) ++digits;
            listing = digits >= 3;
            break;
        }
        const int offset = listing ? 0 : firstStepOfBank(def.bank);
        for (const ProgramStep& s : std::get<std::vector<ProgramStep>>(r)) {
            const int step = s.step + offset;
            if (!stepInBank(step, def.bank)) {
                ctx.error(lineOf(*p, at), where + "program: step " + std::to_string(step) + " is not in bank " +
                                              std::to_string(def.bank) + ".");
                return std::nullopt;
            }
            def.steps[step] = s.keycode;
        }
    }
    if (const YamlNode* d = n.find("data")) {
        if (!d->isMap()) {
            ctx.error(lineOf(*d, at), where + "data: is a mapping of registers, NN: value.");
            return std::nullopt;
        }
        for (const auto& [k, v] : d->map) {
            const int line = lineOf(v, at);
            auto key = dataKey(k, kTI59);
            if (auto* why = std::get_if<std::string>(&key)) {
                ctx.error(line, where + *why);
                return std::nullopt;
            }
            const int reg = std::get<DataKey>(key).index;
            if (!registerInBank(reg, def.bank)) {
                ctx.error(line, where + "data: register " + k + " is not in bank " + std::to_string(def.bank) + ".");
                return std::nullopt;
            }
            const auto value = v.isScalar() ? parseDecimal(text(v)) : std::nullopt;
            if (!value) {
                ctx.error(line, where + "data: " + k + ": the value is no number.");
                return std::nullopt;
            }
            if (!def.registers.emplace(reg, *value).second) {
                ctx.error(line, where + "data: " + k + " is given twice.");
                return std::nullopt;
            }
        }
    }
    return def;
}

void readMagneticCards(const YamlNode& v, int line, ReadContext& ctx) {
    if (!v.isSeq()) {
        ctx.error(line, "magnetic-cards: is a list of cards (- card-name: …).");
        return;
    }
    std::set<std::string> names;
    for (const YamlNode& item : v.seq) {
        const int at = lineOf(item, line);
        if (!item.isMap()) {
            ctx.error(at, "magnetic-cards: an item is a card with card-name: and sides:.");
            continue;
        }
        bool ok = true;
        for (const auto& [k, n] : item.map)
            if (k != "card-name" && k != "description" && k != "cue-card" && k != "sides") {
                ctx.error(lineOf(n, at), "magnetic-cards: a card has no key " + k + ".");
                ok = false;
            }
        const YamlNode* nameNode = item.find("card-name");
        const std::string name = nameNode ? text(*nameNode) : std::string();
        if (name.empty()) {
            ctx.error(at, "magnetic-cards: a card needs card-name:.");
            continue;
        }
        if (!names.insert(calcu::presets::lowerAscii(name)).second) {
            ctx.error(lineOf(*nameNode, at), "magnetic-cards: the card-name " + name + " is given twice.");
            continue;
        }
        auto card = std::make_shared<TIMagneticCardData>();
        auto label = std::make_shared<TICardLabel>();
        label->name = name;
        if (const YamlNode* d = item.find("description")) label->description = text(*d);
        if (const YamlNode* c = item.find("cue-card")) {
            auto cue = readStandAloneCueCard(*c, [&ctx](int l, const std::string& m) { ctx.error(l, "magnetic-cards: " + m); });
            if (!cue) continue;
            label->cueCard = *cue;
        }
        const YamlNode* sides = item.find("sides");
        if (!sides || !sides->isMap() || sides->map.empty()) {
            ctx.error(sides ? lineOf(*sides, at) : at, "magnetic-cards: " + name + ": sides: has 0, 1 or both.");
            continue;
        }
        for (const auto& [k, n] : sides->map) {
            if (k != "0" && k != "1") {
                ctx.error(lineOf(n, at), "magnetic-cards: " + name + ": sides: has 0 and 1 only.");
                ok = false;
                continue;
            }
            auto def = readSide(n, name, k, ctx);
            if (!def) {
                ok = false;
                continue;
            }
            card->sides[k == "1" ? 1 : 0] = std::move(*def);
        }
        if (!ok) continue;
        card->label = label;
        calcu::media::defineInFile(ctx, magneticCardKind(), name, card);
    }
}

} // namespace

void addMagneticCardsKey(calcu::presets::Vocabulary& v) {
    v.add(calcu::presets::HeaderKeyDef{"magnetic-cards", readMagneticCards});
}

} // namespace cu59
