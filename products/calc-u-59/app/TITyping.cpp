#include "TITyping.hpp"
#include "../core/TICatalogue.hpp"
#include <algorithm>
#include <map>

namespace cu59 {

using calcu::machine::KeyId;
using calcu::typing::Mapped;
using calcu::typing::Press;

namespace {

std::string lower(std::string_view s) {
    std::string out(s);
    for (char& c : out)
        if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
    return out;
}

const KeyId k2nd{"2nd"};

// docs/formats/preset.md §8.1, "Second functions": the key, then its names.
struct Second {
    const char* key;
    std::vector<const char*> names;
};
const std::vector<Second>& secondFunctions() {
    static const std::vector<Second> s = {
        {"A", {"A'", "A\xE2\x80\xB2"}}, {"B", {"B'", "B\xE2\x80\xB2"}}, {"C", {"C'", "C\xE2\x80\xB2"}},
        {"D", {"D'", "D\xE2\x80\xB2"}}, {"E", {"E'", "E\xE2\x80\xB2"}},
        {"lnx", {"log"}}, {"CE", {"CP"}}, {"LRN", {"Pgm"}}, {"x\xE2\x87\x84t", {"P/R", "P\xE2\x86\x92R"}},
        {"x\xC2\xB2", {"sin"}}, {"\xE2\x88\x9Ax", {"cos"}}, {"1/x", {"tan"}},
        {"SST", {"Ins"}}, {"STO", {"CMs"}}, {"RCL", {"Exc"}}, {"SUM", {"Prd"}}, {"y\xCB\xA3", {"Ind"}},
        {"BST", {"Del"}}, {"EE", {"Eng"}}, {"(", {"Fix"}}, {")", {"Int"}}, {"\xC3\xB7", {"|x|", "IxI"}},
        {"GTO", {"Pause", "PAU"}}, {"7", {"x=t", "EQ"}}, {"8", {"Nop"}}, {"9", {"Op"}},
        {"\xC3\x97", {"Deg"}}, {"SBR", {"Lbl"}}, {"4", {"x\xE2\x89\xA5t", "GE", "x>=t"}},
        {"5", {"\xCE\xA3+", "\xE2\x88\x91+", "sum+"}}, {"6", {"x\xCC\x84", "xbar"}}, {"-", {"Rad"}},
        {"RST", {"STF"}}, {"1", {"IFF"}}, {"2", {"D.MS", "DMS"}}, {"3", {"\xCF\x80", "pi"}},
        {"+", {"Grad", "GRD"}}, {"R/S", {"Write", "WRT"}}, {"0", {"Dsz"}}, {".", {"Adv"}},
        {"+/-", {"Prt"}}, {"=", {"List", "LST"}},
    };
    return s;
}

// The names `key` takes on a TI model, lower case → presses (§8.1, §8.1.1).
const std::map<std::string, std::vector<Press>>& nameTable() {
    static const std::map<std::string, std::vector<Press>> table = [] {
        std::map<std::string, std::vector<Press>> t;
        const auto& keys = tiCatalogue().models().front().keys;
        for (const auto& k : keys) {   // the key IDs and their other spellings
            t[lower(k.id.value)] = {Press{k.id}};
            for (const auto& o : k.otherSpellings) t[lower(o)] = {Press{k.id}};
        }
        for (const auto& s : secondFunctions())
            for (const char* n : s.names) t[lower(n)] = {Press{k2nd}, Press{KeyId{s.key}}};
        auto seq = [&t](const char* name, std::initializer_list<const char*> of) {
            std::vector<Press> p;
            for (const char* n : of)
                for (const Press& x : t.at(lower(n))) p.push_back(x);
            t[lower(name)] = p;
        };
        // §8.1.1: the printer names that are key sequences (merged codes).
        seq("PG*", {"Pgm", "Ind"});
        seq("EX*", {"Exc", "Ind"});
        seq("PD*", {"Prd", "Ind"});
        seq("ST*", {"STO", "Ind"});
        seq("RC*", {"RCL", "Ind"});
        seq("SM*", {"SUM", "Ind"});
        seq("GO*", {"GTO", "Ind"});
        seq("OP*", {"Op", "Ind"});
        seq("RTN", {"INV", "SBR"});
        return t;   // HIR: no key sequence enters it, refused
    }();
    return table;
}

calcu::typing::TypingProfile makeProfile(Model m) {
    const calcu::machine::TickRate rate = tiCatalogue().model(modelId(m))->tickRate;
    calcu::typing::TypingProfile p;
    p.minHold = calcu::typing::ticksOf(kTIMinHoldSeconds, rate);
    p.typed = calcu::typing::Cadence{p.minHold, 0};
    p.queuedWaitsForReadiness = true;
    p.waitsForReadinessAfterRelease = true;
    p.readinessPoll = std::max<calcu::machine::Ticks>(1, calcu::typing::ticksOf(kTIReadinessPoll, rate));
    p.holdUntilSampled = calcu::typing::SampleRule{kTISamplesNeeded, calcu::typing::ticksOf(kTIMaxHoldSeconds, rate)};
    p.name = tiKeyName;
    p.names = tiKeyNames;
    return p;   // no characters, no ENTER: no `type` (AC-16, D-8)
}

// AC-21: the character → key ID of 2.3.0's 26 bindings (KeyboardMap.cpp).
std::optional<KeyId> binding(char32_t c) {
    switch (c) {
    case 'a': return KeyId{"A"};
    case 'b': return KeyId{"B"};
    case 'c': return KeyId{"C"};
    case 'd': return KeyId{"D"};
    case 'e': return KeyId{"E"};
    case '/': return KeyId{"\xC3\xB7"};
    case '*': case 'x': return KeyId{"\xC3\x97"};
    case '-': return KeyId{"-"};
    case '+': return KeyId{"+"};
    case '=': return KeyId{"="};
    case '.': case ',': return KeyId{"."};
    case '<': return KeyId{"+/-"};
    case '>': return KeyId{"EE"};
    case '(': return KeyId{"("};
    case ')': return KeyId{")"};
    default: break;
    }
    if (c >= '0' && c <= '9') return KeyId{std::string(1, static_cast<char>(c))};
    return std::nullopt;
}

class TIProduct final : public calcu::typing::TypingProduct {
public:
    const calcu::typing::TypingProfile* profile(const calcu::machine::ModelId& id) const override {
        const auto m = modelFrom(id);
        return m ? &typingProfile(*m) : nullptr;
    }
    const calcu::typing::KeyboardMap* keyboardMap(const calcu::machine::ModelId&) const override { return &map_; }
    bool typesText() const override { return false; }

private:
    TIKeyboardMap map_;
};

} // namespace

const calcu::typing::TypingProfile& typingProfile(Model m) {
    static const calcu::typing::TypingProfile profiles[3] = {makeProfile(Model::TI59), makeProfile(Model::TI58),
                                                             makeProfile(Model::TI58C)};
    return profiles[static_cast<int>(m)];
}

std::optional<std::vector<Press>> tiKeyName(std::string_view name) {
    const auto& t = nameTable();
    const auto it = t.find(lower(name));
    if (it == t.end()) return std::nullopt;
    return it->second;
}

std::optional<std::vector<Press>> tiKeyNames(const std::vector<std::string>& names) {
    std::vector<Press> out;
    for (std::size_t i = 0; i < names.size(); ++i) {
        auto p = tiKeyName(names[i]);
        if (!p) return std::nullopt;
        // `2nd Pgm` is `Pgm`: a written 2nd before a second function once (AC-26).
        if (p->size() == 1 && p->front().key == k2nd && i + 1 < names.size()) {
            const auto next = tiKeyName(names[i + 1]);
            if (next && next->size() > 1 && next->front().key == k2nd) continue;
        }
        out.insert(out.end(), p->begin(), p->end());
    }
    return out;
}

std::optional<Mapped> TIKeyboardMap::map(const calcu::typing::HostKeyEvent& e) const {
    using calcu::typing::NamedKey;
    // Command, Control, Option (and the Windows key, as 2.3.0's Qt app): never the calculator's.
    if (e.mods.command || e.mods.control || e.mods.alt || e.mods.meta) return std::nullopt;
    if (e.named == NamedKey::Escape) return Mapped{{Press{KeyId{"CLR"}}}, true};
    if (e.named == NamedKey::Return || e.named == NamedKey::Enter) return Mapped{{Press{KeyId{"="}}}, true};
    if (e.carried) {   // a letter key: the letter it carries (OQ-2); Shift is 2nd
        char32_t c = e.carried;
        if (c >= 'A' && c <= 'Z') c = c - 'A' + 'a';
        const auto k = (c >= 'a' && c <= 'z') ? binding(c) : std::nullopt;
        if (!k) return std::nullopt;
        if (e.mods.shift) return Mapped{{Press{k2nd}, Press{*k}}, false};
        return Mapped{{Press{*k}}, true};
    }
    if (const auto k = binding(e.produced); k && !(e.produced >= 'a' && e.produced <= 'z'))
        return Mapped{{Press{*k}}, true};   // the character the layout produced
    return std::nullopt;
}

const calcu::typing::TypingProduct& typingProduct() {
    static const TIProduct p;
    return p;
}

} // namespace cu59
