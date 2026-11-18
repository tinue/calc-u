#pragma once
#include "typing/KeyboardMap.hpp"
#include "typing/TypingPart.hpp"
#include "typing/TypingProfile.hpp"
#include "typing/TypingSupport.hpp"
#include <cctype>
#include <optional>
#include <string>
#include <vector>

// A typing profile and a keyboard map made for the tests:
// for `test-type` (TypingTestMachine.hpp) and test-seg at 100 kHz.
//
//   minimum hold 1000 ticks (10 ms); typed text 1000 down, 500 up; 300 more
//   after `shift` and `kbii`; presses that wait start at key readiness,
//   polled every 50 ticks; `enter` ends `type`.
//   Characters: 0-9 and a-c as their key; A-C as shift and the key; é as
//   kbii, a, kbii (the closing kbii must finish); with `kbii` latched é is
//   left out, with `shift` latched shift is pressed first.
//   Names: the key IDs in any case, and `sum` for shift then 1.
//   Host keys: a-c by the carried letter (held), with Shift the sequence
//   shift, letter; digits by the produced character (held); Return as enter;
//   é as its character's sequence; Command maps nothing.

namespace calcu::typing::test {

using machine::KeyId;
using machine::Ticks;

inline std::vector<Press> keys(std::initializer_list<const char*> ids) {
    std::vector<Press> out;
    for (const char* k : ids) out.push_back(Press{KeyId{k}});
    return out;
}

inline std::optional<std::vector<Press>> testCharKeys(char32_t c, const TypingSupport* s) {
    if (c >= '0' && c <= '9') return std::vector<Press>{Press{KeyId{std::string(1, static_cast<char>(c))}}};
    if (c >= 'a' && c <= 'c') return std::vector<Press>{Press{KeyId{std::string(1, static_cast<char>(c))}}};
    if (c >= 'A' && c <= 'C')
        return std::vector<Press>{Press{KeyId{"shift"}}, Press{KeyId{std::string(1, static_cast<char>(c - 'A' + 'a'))}}};
    if (c == U'é') {
        if (s && s->latched("kbii")) return std::vector<Press>{};   // left out
        std::vector<Press> out;
        if (s && s->latched("shift")) out.push_back(Press{KeyId{"shift"}});
        out.push_back(Press{KeyId{"kbii"}});
        out.push_back(Press{KeyId{"a"}});
        out.push_back(Press{KeyId{"kbii"}, true});
        return out;
    }
    return std::nullopt;
}

inline TypingProfile testProfile() {
    TypingProfile p;
    p.minHold = 1000;
    p.typed = Cadence{1000, 500};
    p.extraGapAfter = [](const KeyId& k) -> Ticks { return k.value == "shift" || k.value == "kbii" ? 300 : 0; };
    p.queuedWaitsForReadiness = true;
    p.readinessPoll = 50;
    p.charKeys = testCharKeys;
    p.enter = KeyId{"enter"};
    p.name = [](std::string_view n) -> std::optional<std::vector<Press>> {
        std::string w(n);
        for (char& c : w) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        if (w == "sum") return keys({"shift", "1"});
        for (const char* k : {"0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "a", "b", "c", "shift", "kbii", "enter",
                              "clr", "on"})
            if (w == k) return std::vector<Press>{Press{KeyId{k}}};
        return std::nullopt;
    };
    return p;
}

class TestKeyboardMap final : public KeyboardMap {
public:
    std::optional<Mapped> map(const HostKeyEvent& e) const override {
        if (e.mods.command) return std::nullopt;
        if (e.named == NamedKey::Return) return Mapped{keys({"enter"}), true};
        if (e.carried >= 'a' && e.carried <= 'c') {
            const std::string k(1, static_cast<char>(e.carried));
            if (e.mods.shift) return Mapped{std::vector<Press>{Press{KeyId{"shift"}}, Press{KeyId{k}}}, false};
            return Mapped{std::vector<Press>{Press{KeyId{k}}}, true};
        }
        if (e.produced >= '0' && e.produced <= '9')
            return Mapped{std::vector<Press>{Press{KeyId{std::string(1, static_cast<char>(e.produced))}}}, true};
        if (e.produced == U'é') return Mapped{*testCharKeys(U'é', nullptr), false};
        return std::nullopt;
    }
};

// The product of the tests: the profile for every model, the map for every
// model; `text` false gives a product without `type`.
class TestTypingProduct final : public TypingProduct {
public:
    explicit TestTypingProduct(TypingProfile p = testProfile(), bool text = true) : profile_(std::move(p)), text_(text) {}
    const TypingProfile* profile(const machine::ModelId&) const override { return &profile_; }
    const KeyboardMap* keyboardMap(const machine::ModelId&) const override { return &map_; }
    bool typesText() const override { return text_; }
    TypingProfile profile_;

private:
    TestKeyboardMap map_;
    bool text_;
};

inline HostKeyEvent letter(char c, const std::string& physical, bool shift = false) {
    HostKeyEvent e;
    e.physical = physical;
    e.carried = static_cast<char32_t>(c);
    e.produced = static_cast<char32_t>(shift ? c - 'a' + 'A' : c);
    e.mods.shift = shift;
    return e;
}

inline HostKeyEvent digit(char c, const std::string& physical) {
    HostKeyEvent e;
    e.physical = physical;
    e.produced = static_cast<char32_t>(c);
    return e;
}

} // namespace calcu::typing::test
