#include "app/SharpTyping.hpp"
#include "machine/Names.hpp"

#include <cctype>

#include "KeyPaste.hpp"
#include "Machine/SharpCatalogue.hpp"
#include "Machine/SharpKeyIds.hpp"
#include "PC1500/PC1500TypedInput.hpp"
#include "PC1600/PC1600TypedInput.hpp"
#include "SharpShiftedSymbols.hpp"
#include "typing/TypingSupport.hpp"

namespace cu1600 {

using calcu::machine::KeyId;
using calcu::machine::ModelId;
using calcu::machine::Ticks;
using calcu::typing::HostKeyEvent;
using calcu::typing::Mapped;
using calcu::typing::NamedKey;
using calcu::typing::Press;
using calcu::typing::TypingProfile;
using calcu::typing::TypingSupport;

namespace {

// A key name of the core's typing helpers: a letter as its capital
// (PC1500TypedInput.hpp, PC1600TypedInput.hpp); the machine takes its key IDs.
KeyId keyIdOf(const std::string& name) {
    if (name.size() == 1 && name[0] >= 'A' && name[0] <= 'Z') return KeyId{std::string(1, static_cast<char>(name[0] - 'A' + 'a'))};
    return KeyId{name};
}

std::vector<Press> pressesOf(const std::string& base, bool shift) {
    std::vector<Press> out;
    if (shift) out.push_back(Press{KeyId{"shift"}});
    out.push_back(Press{keyIdOf(base)});
    return out;
}

// An accented character on the PC-1600 is KBII, [SHIFT,] its
// key, KBII, by the latches of the moment (kbiiSequence); the closing KBII
// must finish, even when live input drops the rest. With KBII latched it is left out (an empty list).
std::vector<Press> kbiiPresses(const std::string& base, bool shift, const TypingSupport* s) {
    KeyLatches l;
    if (s) {
        l.shift = s->latched("shift");
        l.kbii = s->latched("kbii");
    }
    std::vector<Press> out;
    const std::vector<KbiiTap> taps = kbiiSequence(base, shift, l);
    for (std::size_t i = 0; i < taps.size(); ++i) out.push_back(Press{keyIdOf(taps[i].key), i + 1 == taps.size()});
    return out;
}

std::optional<std::vector<Press>> pc1500Char(char32_t c, const TypingSupport*) {
    if (c >= 0x80 || c < 0x20 || c == 0x7F) return std::nullopt;
    std::string base;
    bool shift = false;
    if (!pc1500ResolveTypedChar(static_cast<char>(c), &base, &shift)) return std::nullopt;
    return pressesOf(base, shift);
}

std::optional<std::vector<Press>> pc1600Char(char32_t c, const TypingSupport* s) {
    if (c < 0x20 || c == 0x7F) return std::nullopt;
    std::string base;
    bool shift = false;
    if (c >= 0x80) {
        if (!pc1600ResolveKbiiChar(c, &base, &shift)) return std::nullopt;
        return kbiiPresses(base, shift, s);
    }
    if (!pc1600ResolveTypedChar(static_cast<char>(c), &base, &shift)) return std::nullopt;
    return pressesOf(base, shift);
}

// The model's key IDs in any case, `break` for `on`; nothing else
// (`ent` is no longer a name). Every key ID is a name, the PC-1600's
// `ctrl`, `kbii` and `bs` too.
std::function<std::optional<std::vector<Press>>(std::string_view)> namesOf(const std::vector<std::string>& ids) {
    return [&ids](std::string_view n) -> std::optional<std::vector<Press>> {
        const std::string w = calcu::machine::lowerAscii(n);
        if (w == "break") return std::vector<Press>{Press{KeyId{sharp::kOnKeyId}}};
        for (const std::string& k : ids)
            if (k == w) return std::vector<Press>{Press{KeyId{k}}};
        return std::nullopt;
    };
}

TypingProfile sharpProfile(const char* model) {
    const bool pc1600 = std::string(model) == sharp::kPC1600;
    const auto rate = sharp::sharpCatalogue().model(ModelId{model})->tickRate;
    // A frame as the cores and the preset typers count it: rate / 60 whole
    // ticks (kPC1500CyclesPerFrame), so a press is 4 + 4 frames.
    const auto frame = [rate](int n) -> Ticks { return static_cast<Ticks>(n) * (rate / 60); };
    TypingProfile p;
    p.minHold = frame(4);   // every press, live input too
    p.typed = calcu::typing::Cadence{frame(4), frame(4)};
    p.readinessPoll = frame(1);   // a waiting press looks again each frame
    p.enter = KeyId{"enter"};
    if (pc1600) {
        const Ticks six = frame(6);
        p.extraGapAfter = [six](const KeyId& k) -> Ticks { return k.value == "shift" || k.value == "kbii" ? six : 0; };
        p.queuedWaitsForReadiness = false;   // queued presses follow the cadence only, as the PC-1600's paste feeder did
        p.firstPressOfStep = calcu::typing::StartRule::KeyReadiness;
        p.firstPressBound = calcu::typing::ticksOf(15.0, rate);
        p.charKeys = pc1600Char;
        p.name = namesOf(sharp::pc1600KeyIds());
    } else {
        p.queuedWaitsForReadiness = true;   // the ROM's WAIT_4_KB: key readiness is &7B0E bit 0 clear
        p.charKeys = pc1500Char;
        p.name = namesOf(sharp::pc1500KeyIds());
    }
    return p;
}

std::optional<Mapped> held(const std::string& name) { return Mapped{std::vector<Press>{Press{KeyId{name}}}, true}; }
std::optional<Mapped> shifted(const std::string& name) {
    return Mapped{std::vector<Press>{Press{KeyId{"shift"}}, Press{KeyId{name}}}, false};
}

std::optional<std::string> characterKey(char c) {
    if (c == ' ') return std::string("space");
    if (c >= 'a' && c <= 'z') return std::string(1, c);
    if (c >= 'A' && c <= 'Z') return std::string(1, static_cast<char>(c - 'A' + 'a'));
    if (c >= '0' && c <= '9') return std::string(1, c);
    switch (c) {
    case '+': case '-': case '=': case '*': case '/': case '(': case ')': case '.': return std::string(1, c);
    default: return std::nullopt;
    }
}

} // namespace

const TypingProfile* typingProfile(const ModelId& m) {
    static const TypingProfile p1500 = sharpProfile(sharp::kPC1500);
    static const TypingProfile p1500a = sharpProfile(sharp::kPC1500A);
    static const TypingProfile p1600 = sharpProfile(sharp::kPC1600);
    if (m.value == sharp::kPC1500) return &p1500;
    if (m.value == sharp::kPC1500A) return &p1500a;
    if (m.value == sharp::kPC1600) return &p1600;
    return nullptr;
}

std::optional<Mapped> SharpKeyboardMap::map(const HostKeyEvent& e) const {
    // Command modifiers never type: Command or Control on a Mac;
    // Control, Alt or the Windows key elsewhere, but Control+Alt (AltGr).
    const auto& mods = e.mods;
    if (mods.macHost) {
        if (mods.command || mods.control) return std::nullopt;
    } else {
        if (mods.meta) return std::nullopt;
        if (mods.control != mods.alt) return std::nullopt;
    }
    switch (e.named) {
    case NamedKey::Backspace: return held(m_pc1600 ? "bs" : "left");
    case NamedKey::F1: return held("f1");
    case NamedKey::F2: return held("f2");
    case NamedKey::F3: return held("f3");
    case NamedKey::F4: return held("f4");
    case NamedKey::F5: return held("f5");
    case NamedKey::F6: return held("f6");
    case NamedKey::ScrollLock:
    case NamedKey::Insert: return held("rsv");
    case NamedKey::Home: return m_pc1600 && mods.shift ? held("ctrl") : held("rcl");
    case NamedKey::End: return m_pc1600 && mods.shift ? held("kbii") : held("sml");
    case NamedKey::PageUp: return mods.shift ? held("def") : shifted("left");
    case NamedKey::PageDown: return shifted("right");
    case NamedKey::Return:
    case NamedKey::Enter: return held("enter");
    case NamedKey::Up: return held("up");
    case NamedKey::Down: return held("down");
    case NamedKey::Left: return held("left");
    case NamedKey::Right: return held("right");
    case NamedKey::Delete: return mods.shift ? shifted("cl") : held("cl");
    case NamedKey::Tab: return held("mode");
    case NamedKey::Escape:
    case NamedKey::None: break;
    }
    const char32_t typed = e.produced;
    if (typed == 0) return std::nullopt;
    if (m_pc1600 && typed > 0x7f) {   // an accented character, typed as typed
        std::string base;
        bool shift = false;
        if (pc1600ResolveKbiiChar(typed, &base, &shift)) return Mapped{kbiiPresses(base, shift, nullptr), false};
    }
    if (typed > 0x7f) return std::nullopt;
    const char c = static_cast<char>(typed);
    if (auto k = characterKey(c)) return held(*k);
    std::string base;
    if (sharpShiftedSymbolBaseKey(c, &base)) return shifted(base);
    if (m_pc1600 && pc1600DigitRowShiftedBaseKey(c, &base)) return shifted(base);
    return std::nullopt;
}

const SharpKeyboardMap* keyboardMapOf(const ModelId& m) {
    static const SharpKeyboardMap pc1500(false), pc1600(true);
    if (m.value == sharp::kPC1600) return &pc1600;
    if (m.value == sharp::kPC1500 || m.value == sharp::kPC1500A) return &pc1500;
    return nullptr;
}

const calcu::typing::TypingProduct& typingProduct() {
    struct Product final : calcu::typing::TypingProduct {
        const TypingProfile* profile(const ModelId& m) const override { return typingProfile(m); }
        const calcu::typing::KeyboardMap* keyboardMap(const ModelId& m) const override { return keyboardMapOf(m); }
        bool typesText() const override { return true; }
    };
    static const Product p;
    return p;
}

std::optional<std::vector<Press>> charPresses(const ModelId& m, char32_t c) {
    const TypingProfile* p = typingProfile(m);
    return p && p->charKeys ? p->charKeys(c, nullptr) : std::nullopt;
}

} // namespace cu1600
