// The keyboard map of Calc-U-1600 (app/SharpTyping, LIB-5 AC-29, AC-37):
// 0.8.1's HostKeyMap rules, now shared::SharpKeyboardMap per model.

#include <cstdio>

#include "Machine/SharpCatalogue.hpp"
#include "TestHarness.hpp"
#include "app/SharpTyping.hpp"

namespace {

using calcu::machine::ModelId;
using calcu::typing::HostKeyEvent;
using calcu::typing::Mapped;
using calcu::typing::Modifiers;
using calcu::typing::NamedKey;

Modifiers shift() { Modifiers m; m.shift = true; return m; }

std::optional<Mapped> resolve(NamedKey key, Modifiers mods, char32_t typed, bool pc1600, bool mac) {
    HostKeyEvent e;
    e.physical = "k";
    e.named = key;
    e.produced = typed;
    e.mods = mods;
    e.mods.macHost = mac;
    return shared::keyboardMapOf(ModelId{pc1600 ? sharp::kPC1600 : sharp::kPC1500A})->map(e);
}

// One key held (AC-31), or shift then the key (AC-8).
bool is(const std::optional<Mapped>& r, const char* key, bool shifted = false) {
    if (!r) return false;
    if (shifted) return !r->held && r->presses.size() == 2 && r->presses[0].key.value == "shift" && r->presses[1].key.value == key;
    return r->held && r->presses.size() == 1 && r->presses[0].key.value == key;
}

TEST(lib5_ac29_named_keys_and_shift_forms_per_model) {
    for (bool mac : {false, true}) {
        CHECK(is(resolve(NamedKey::Backspace, {}, 0, false, mac), "left"));
        CHECK(is(resolve(NamedKey::Backspace, {}, 0, true, mac), "bs"));
        CHECK(is(resolve(NamedKey::F3, {}, 0, false, mac), "f3"));
        CHECK(is(resolve(NamedKey::Return, {}, '\r', false, mac), "enter"));
        CHECK(is(resolve(NamedKey::Enter, {}, '\r', true, mac), "enter"));
        CHECK(is(resolve(NamedKey::Tab, {}, '\t', false, mac), "mode"));
        CHECK(is(resolve(NamedKey::Delete, shift(), 0, false, mac), "cl", true));
        CHECK(is(resolve(NamedKey::Delete, {}, 0, false, mac), "cl"));
        CHECK(is(resolve(NamedKey::Home, shift(), 0, true, mac), "ctrl"));
        CHECK(is(resolve(NamedKey::Home, shift(), 0, false, mac), "rcl"));
        CHECK(is(resolve(NamedKey::End, shift(), 0, true, mac), "kbii"));
        CHECK(is(resolve(NamedKey::End, {}, 0, true, mac), "sml"));
        CHECK(is(resolve(NamedKey::PageUp, {}, 0, false, mac), "left", true));
        CHECK(is(resolve(NamedKey::PageUp, shift(), 0, false, mac), "def"));
        CHECK(is(resolve(NamedKey::PageDown, {}, 0, false, mac), "right", true));
        CHECK(is(resolve(NamedKey::ScrollLock, {}, 0, false, mac), "rsv"));
        CHECK(is(resolve(NamedKey::Insert, {}, 0, false, mac), "rsv"));
        for (auto [k, n] : {std::pair{NamedKey::Up, "up"}, std::pair{NamedKey::Down, "down"},
                            std::pair{NamedKey::Left, "left"}, std::pair{NamedKey::Right, "right"}})
            CHECK(is(resolve(k, {}, 0, false, mac), n));
    }
}

TEST(test_characters) {
    CHECK(is(resolve(NamedKey::None, {}, 'A', false, true), "a"));
    CHECK(is(resolve(NamedKey::None, {}, 'z', false, true), "z"));
    CHECK(is(resolve(NamedKey::None, {}, '7', false, true), "7"));
    CHECK(is(resolve(NamedKey::None, {}, ' ', false, true), "space"));
    CHECK(is(resolve(NamedKey::None, {}, '+', false, true), "+"));
    CHECK(resolve(NamedKey::None, {}, 0, false, true) == std::nullopt);
    // A second-legend punctuation character: shift and its base key.
    CHECK(is(resolve(NamedKey::None, {}, '"', false, true), "f2", true));
    // The PC-1600's digit-row legends; the PC-1500 has none.
    CHECK(is(resolve(NamedKey::None, {}, '\'', true, true), "1", true));
    CHECK(resolve(NamedKey::None, {}, '\'', false, true) == std::nullopt);
    // PC-1600 accents go through KBII; the PC-1500 has none.
    const auto e = resolve(NamedKey::None, {}, U'é', true, true);
    CHECK(e && !e->held && e->presses.size() == 4 && e->presses[0].key.value == "kbii" && e->presses[1].key.value == "shift" &&
          e->presses[2].key.value == "s" && e->presses[3].key.value == "kbii" && e->presses[3].mustFinish);
    CHECK(resolve(NamedKey::None, {}, U'é', false, true) == std::nullopt);
}

TEST(lib5_ac29_command_modifiers_nothing_altgr_types) {
    Modifiers cmd; cmd.command = true;
    Modifiers ctrl; ctrl.control = true;
    Modifiers alt; alt.alt = true;
    Modifiers altGr; altGr.control = true; altGr.alt = true;
    Modifiers meta; meta.meta = true;
    // macOS: Cmd and Control never type; Option does.
    CHECK(resolve(NamedKey::None, cmd, 'a', false, true) == std::nullopt);
    CHECK(resolve(NamedKey::None, ctrl, 'a', false, true) == std::nullopt);
    CHECK(is(resolve(NamedKey::None, alt, 'a', false, true), "a"));
    // Elsewhere: Control or Alt alone and the Windows key never type; AltGr does.
    CHECK(resolve(NamedKey::None, ctrl, 'a', false, false) == std::nullopt);
    CHECK(resolve(NamedKey::None, alt, 'a', false, false) == std::nullopt);
    CHECK(resolve(NamedKey::None, meta, 'a', false, false) == std::nullopt);
    CHECK(is(resolve(NamedKey::None, altGr, '+', false, false), "+"));
    // The same map for the PC-1500 and the PC-1500A.
    CHECK(shared::keyboardMapOf(ModelId{sharp::kPC1500}) == shared::keyboardMapOf(ModelId{sharp::kPC1500A}));
}

} // namespace
