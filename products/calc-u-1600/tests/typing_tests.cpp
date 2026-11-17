// LIB-5 in Calc-U-1600 (design §6, T16-1 to T16-8; spec AC-14, AC-28 to
// AC-31, AC-33 to AC-36, AC-40, AC-41): the Sharp typing profiles and
// typing support through the library's typing engine, on the product's app
// session (app/tests/SessionFixture) and in the harness. ROM-gated cases
// SKIP when the images are absent.

#include <cstdio>
#include <string>
#include <vector>

#include "../app/HarnessProduct.hpp"
#include "../app/tests/SessionFixture.hpp"
#include "Machine/SharpCatalogue.hpp"
#include "Machine/SharpProductKinds.hpp"
#include "TestHarness.hpp"
#include "app/SharpTyping.hpp"
#include "harness/Run.hpp"
#include "typing/TypingEngine.hpp"
#include "typing/TypingSupport.hpp"

namespace {

using calcu::machine::KeyId;
using calcu::machine::ModelId;
using calcu::machine::Ticks;
using calcu::typing::HostKeyEvent;
using calcu::typing::NamedKey;
using calcu::typing::StartRule;
using sessionfixture::SessionFixture;
using sessionfixture::romsPresent;

const ModelId kPC1500{sharp::kPC1500};
const ModelId kPC1500A{sharp::kPC1500A};
const ModelId kPC1600{sharp::kPC1600};

Ticks frameOf(const ModelId& m) { return sharp::sharpCatalogue().model(m)->tickRate / 60; }

// The keys the engine puts down and lets up, with their ticks.
struct Log final : calcu::typing::KeyObserver {
    struct E { std::string key; bool down; Ticks at; };
    std::vector<E> events;
    void keyChanged(const KeyId& k, bool down, Ticks at) override { events.push_back({k.value, down, at}); }
    std::string keys() const {
        std::string out;
        for (const auto& e : events)
            if (e.down) out += e.key + " ";
        return out;
    }
};

HostKeyEvent named(NamedKey k, const char* physical) {
    HostKeyEvent e;
    e.physical = physical;
    e.named = k;
    return e;
}

HostKeyEvent letter(char c, const char* physical) {
    HostKeyEvent e;
    e.physical = physical;
    e.produced = static_cast<char32_t>(c);
    e.carried = static_cast<char32_t>(c);
    return e;
}

// ── T16-1: the profiles (AC-14, AC-28, AC-30) ───────────────────────────

TEST(lib5_ac28_cadence_4_4_and_6_after_shift_pc1600) {
    for (const ModelId& m : {kPC1500, kPC1500A, kPC1600}) {
        const auto* p = cu1600::typingProfile(m);
        CHECK(p != nullptr);
        if (!p) continue;
        const Ticks f = frameOf(m);
        CHECK_EQ(p->minHold, 4 * f);   // D-10: live input too
        CHECK_EQ(p->typed.hold, 4 * f);
        CHECK_EQ(p->typed.gap, 4 * f);
        const Ticks extra = m == kPC1600 ? 6 * f : 0;
        CHECK_EQ(p->gapAfter(KeyId{"shift"}), 4 * f + extra);
        CHECK_EQ(p->gapAfter(KeyId{"kbii"}), 4 * f + extra);
        CHECK_EQ(p->gapAfter(KeyId{"a"}), 4 * f);
        CHECK(p->enter && p->enter->value == "enter");
        CHECK(!p->waitsForReadinessAfterRelease);   // what follows ENTER is not the profile's (AC-28)
    }
    // On the machine: shift 4 down, 4 + 6 up, A 4 down (a typed `a`).
    SKIP_UNLESS(romsPresent(), "no ROMs in roms/");
    SessionFixture s;
    CHECK(s.start("PC-1600"));
    Log log;
    s.engine().setObserver(&log);
    s.paste("a");
    CHECK(s.runUntilTyped(5));
    s.engine().setObserver(nullptr);
    CHECK_EQ(log.keys(), std::string("shift a "));
    if (log.events.size() == 4) {
        const Ticks f = frameOf(kPC1600);
        auto near = [](Ticks got, Ticks want) { return got >= want && got < want + 60; };   // within an instruction
        CHECK(near(log.events[1].at - log.events[0].at, 4 * f));
        CHECK(near(log.events[2].at - log.events[1].at, 10 * f));
        CHECK(near(log.events[3].at - log.events[2].at, 4 * f));
    }
}

TEST(lib5_ac14_type_starts_at_readiness_pc1600_bound_15s) {
    const auto* p = cu1600::typingProfile(kPC1600);
    CHECK(p->firstPressOfStep == StartRule::KeyReadiness);
    CHECK(p->firstPressBound && *p->firstPressBound == calcu::typing::ticksOf(15.0, sharp::sharpCatalogue().model(kPC1600)->tickRate));
    CHECK(!p->queuedWaitsForReadiness);   // D-6: then the cadence only
    // At the prompt (key ready) the first press goes down within a poll.
    SKIP_UNLESS(romsPresent(), "no ROMs in roms/");
    SessionFixture s;
    CHECK(s.start("PC-1600"));
    Log log;
    s.engine().setObserver(&log);
    const Ticks t0 = s.part().machine()->tickCount();
    s.typeLine("1");
    s.runFor(0.5);
    s.engine().setObserver(nullptr);
    CHECK_EQ(log.keys(), std::string("1 enter "));
    CHECK(!log.events.empty() && log.events[0].at - t0 <= frameOf(kPC1600));
}

TEST(lib5_ac14_type_starts_at_once_pc1500) {
    for (const ModelId& m : {kPC1500, kPC1500A}) {
        const auto* p = cu1600::typingProfile(m);
        CHECK(p->firstPressOfStep == StartRule::AsQueued);   // by the flag: at readiness (D-6)
        CHECK(!p->firstPressBound);
        CHECK(p->queuedWaitsForReadiness);
    }
}

TEST(lib5_ac30_characters_per_model) {
    auto keys = [](const ModelId& m, char32_t c) {
        std::string out;
        const auto k = cu1600::charPresses(m, c);
        if (!k) return std::string("none");
        for (const auto& p : *k) out += p.key.value + " ";
        return out;
    };
    for (const ModelId& m : {kPC1500, kPC1600}) {
        CHECK_EQ(keys(m, 'A'), std::string("a "));
        CHECK_EQ(keys(m, 'a'), std::string("shift a "));   // the case is kept
        CHECK_EQ(keys(m, '7'), std::string("7 "));
        for (char32_t c : {U'+', U'-', U'=', U'*', U'/', U'(', U')', U'.'}) CHECK_EQ(keys(m, c), std::string(1, static_cast<char>(c)) + " ");
        CHECK_EQ(keys(m, ' '), std::string("space "));
        CHECK_EQ(keys(m, '!'), std::string("shift f1 "));   // one of the 13 second legends
        CHECK_EQ(keys(m, '?'), std::string("shift / "));
    }
    CHECK_EQ(keys(kPC1600, '['), std::string("shift 2 "));   // a digit-row legend: the PC-1600 only
    CHECK_EQ(keys(kPC1500, '['), std::string("none"));
    CHECK_EQ(keys(kPC1600, U'Ä'), std::string("kbii q kbii "));   // Ä
    CHECK_EQ(keys(kPC1500, U'Ä'), std::string("none"));
    CHECK_EQ(keys(kPC1600, '\t'), std::string("none"));
}

// ── T16-3: held keys, PC-1500 readiness (AC-31) ─────────────────────────

// The exit check of M-36: a host key held on a PC-1500 auto-repeats as the
// ROM does. Type 1234567890, hold the cursor key left for two seconds, then
// type X: the ROM has moved the cursor back over the whole line, so X lands
// at its start, not on the last digit.
TEST(lib5_ac31_cursor_key_held_rom_repeats_pc1500) {
    SKIP_UNLESS(romsPresent(), "no ROMs in roms/");
    for (const char* model : {"PC-1500A", "PC-1600"}) {
        SessionFixture s;
        CHECK(s.start(model));
        if (std::string(model) != "PC-1600") {
            s.tap("cl");
            s.runFor(0.5);
        }
        s.paste("1234567890");
        CHECK(s.runUntilTyped(10));
        s.runFor(0.3);
        s.hostDown(named(NamedKey::Left, "ArrowLeft"));
        s.runFor(2.0);
        CHECK(!s.engine().liveHeld().empty());   // down all the while
        s.hostUp("ArrowLeft");
        s.runFor(0.3);
        s.paste("X");
        CHECK(s.runUntilTyped(5));
        s.runFor(0.3);
        const std::string text = s.screen();
        CHECK(text.find("X234567890") != std::string::npos || text.find("X1234567890") != std::string::npos);
        if (text.find("X2345") == std::string::npos && text.find("X12345") == std::string::npos)
            std::fprintf(stderr, "  %s screen: %s\n", model, text.c_str());
    }
}

TEST(lib5_ac31_every_key_held_pc1500a) {
    SKIP_UNLESS(romsPresent(), "no ROMs in roms/");
    SessionFixture s;
    CHECK(s.start("PC-1500A"));
    s.tap("cl");
    s.runFor(0.5);
    Log log;
    s.engine().setObserver(&log);
    s.hostDown(letter('a', "KeyA"));   // not queued: held (0.8.1 queued all but the cursor keys)
    s.runFor(1.0);
    CHECK_EQ(log.events.size(), std::size_t(1));
    CHECK(s.engine().liveHeld() == std::vector<KeyId>{KeyId{"a"}});
    s.hostUp("KeyA");
    CHECK_EQ(log.events.size(), std::size_t(2));
    CHECK(log.events.size() == 2 && log.events[1].at - log.events[0].at >= sharp::sharpCatalogue().model(kPC1500A)->tickRate);
    s.engine().setObserver(nullptr);
}

TEST(lib5_ac31_waiting_press_at_key_readiness_pc1500) {
    SKIP_UNLESS(romsPresent(), "no ROMs in roms/");
    SessionFixture s;
    CHECK(s.start("PC-1500A"));
    s.tap("cl");
    s.runFor(0.5);
    s.typeLine("NEW0");
    CHECK(s.runUntilTyped(10));
    s.typeLine("10 GOTO 10");
    CHECK(s.runUntilTyped(10));
    s.tap("mode");   // PRO -> RUN
    s.runFor(0.5);
    s.typeLine("RUN");
    CHECK(s.runUntilTyped(10));
    s.runFor(0.5);
    // The program runs: the machine is not ready for keys, so a press that
    // waits waits (EC-3); a live key goes down at once.
    Log log;
    s.engine().setObserver(&log);
    s.paste("1");
    s.runFor(2.0);
    CHECK(log.events.empty());
    CHECK_EQ(s.engine().waiting(), std::size_t(1));
    s.engine().setObserver(nullptr);
}

// ── T16-4: the SHIFT tap (AC-33), as the frontends send it ───────────────

TEST(lib5_ac33_shift_tap_is_one_shift_press) {
    SKIP_UNLESS(romsPresent(), "no ROMs in roms/");
    SessionFixture s;
    CHECK(s.start("PC-1600"));
    Log log;
    s.engine().setObserver(&log);
    s.screenDown("shift", 1000);   // what Qt6 and the page send for a tap within 400 ms
    s.screenUp("shift", 1000);
    s.runFor(0.5);
    CHECK_EQ(log.keys(), std::string("shift "));
    CHECK(log.events.size() == 2 && log.events[1].at - log.events[0].at >= 4 * frameOf(kPC1600));
    s.engine().setObserver(nullptr);
    auto* support = calcu::machine::first<calcu::typing::TypingSupport>(*s.part().machine());
    CHECK(support && support->latched("shift"));   // the ROM took it: SHIFT is latched
}

// ── T16-5: paste and latches (AC-34, AC-35) ─────────────────────────────

TEST(lib5_ac34_paste_releases_latched_shift_skips_under_kbii) {
    SKIP_UNLESS(romsPresent(), "no ROMs in roms/");
    SessionFixture s;
    CHECK(s.start("PC-1600"));
    s.tap("shift");   // SHIFT latched by the user
    s.runFor(0.5);
    Log log;
    s.engine().setObserver(&log);
    s.paste("\xC3\x84");   // Ä
    CHECK(s.runUntilTyped(5));
    CHECK_EQ(log.keys(), std::string("shift kbii q kbii "));   // SHIFT un-latched first
    s.runFor(0.3);
    log.events.clear();
    s.tap("end");   // nothing: no such key; KBII by its key
    s.tap("kbii");  // KBII latched
    s.runFor(0.5);
    log.events.clear();
    s.paste("1\xC3\x84" "2");
    CHECK(s.runUntilTyped(5));
    CHECK_EQ(log.keys(), std::string("1 2 "));   // left out under KBII
    s.engine().setObserver(nullptr);
}

TEST(lib5_ac34_second_paste_appended) {
    SKIP_UNLESS(romsPresent(), "no ROMs in roms/");
    SessionFixture s;
    CHECK(s.start("PC-1600"));
    Log log;
    s.engine().setObserver(&log);
    s.paste("12");
    s.paste("3");
    CHECK(s.runUntilTyped(5));
    CHECK_EQ(log.keys(), std::string("1 2 3 "));
    s.engine().setObserver(nullptr);
}

TEST(lib5_ac35_prompt_readiness_and_latches) {
    SKIP_UNLESS(romsPresent(), "no ROMs in roms/");
    for (const char* model : {"PC-1500A", "PC-1600"}) {
        SessionFixture s;
        CHECK(s.start(model));
        const bool pc1600 = std::string(model) == "PC-1600";
        if (!pc1600) {
            s.tap("cl");
            s.runFor(0.5);
        }
        s.runFor(0.5);
        auto* support = calcu::machine::first<calcu::typing::TypingSupport>(*s.part().machine());
        CHECK(support != nullptr);
        if (!support) continue;
        CHECK(support->promptReady() == std::optional<bool>(true));
        CHECK(!support->samples(KeyId{"a"}));   // no sample count on a Sharp machine
        if (pc1600) {
            CHECK(support->latches().size() == 2 && !support->latched("shift") && !support->latched("kbii"));
            s.tap("kbii");
            s.runFor(0.5);
            CHECK(support->latched("kbii"));
        } else {
            CHECK(support->latches().empty());
        }
    }
}

// ── T16-6: busy (AC-36) ─────────────────────────────────────────────────

TEST(lib5_ac36_keys_dropped_during_preset_operation) {
    SKIP_UNLESS(romsPresent(), "no ROMs in roms/");
    SessionFixture s;
    CHECK(s.start("PC-1600"));
    s.readCost = std::chrono::milliseconds(1);
    s.send(calcu::presets::kLoadPresetCommand, calcu::sessions::Value::object({{"ref", "tests/fixtures/presets/maxed-out-mem.pc1600"}}));
    s.call();
    CHECK(s.busy());
    s.screenDown("a");
    s.hostDown(letter('b', "KeyB"));
    s.paste("123");
    s.send(calcu::typing::kHostText, calcu::sessions::Value::object({{"text", "4"}}));
    CHECK(s.engine().idle());
    for (int i = 0; i < 6000 && s.busy(); ++i) s.call();
    CHECK(!s.busy());
    // The preset types through the engine too (M-46): the keys are watched
    // from its end on.
    Log log;
    s.engine().setObserver(&log);
    s.runFor(0.5);   // a key still held when the operation ended is not pressed then
    CHECK(log.events.empty());
    s.screenUp("a");
    s.hostUp("KeyB");
    CHECK(log.events.empty());
    s.engine().setObserver(nullptr);
}

// ── T16-8: BREAK and names (AC-40, AC-41) ───────────────────────────────

TEST(lib5_ac40_on_breaks_running_program) {
    SKIP_UNLESS(romsPresent(), "no ROMs in roms/");
    for (const char* model : {"PC-1500A", "PC-1600"}) {
        SessionFixture s;
        CHECK(s.start(model));
        if (std::string(model) != "PC-1600") {
            s.tap("cl");
            s.runFor(0.5);
            s.typeLine("NEW0");
            CHECK(s.runUntilTyped(10));
        } else {
            s.tap("mode");   // RUN -> PRO
            s.runFor(0.5);
        }
        s.typeLine("10 GOTO 10");
        CHECK(s.runUntilTyped(10));
        s.tap("mode");   // PRO -> RUN (NEW0 leaves the PC-1500 in PRO)
        s.runFor(0.5);
        s.typeLine("RUN");
        CHECK(s.runUntilTyped(10));
        s.runFor(1.0);
        s.tap("on");   // a live key: down at once, the ROM takes it as BREAK
        s.runFor(1.0);
        const std::string text = s.screen();
        CHECK(text.find("BREAK") != std::string::npos);
        if (text.find("BREAK") == std::string::npos) std::fprintf(stderr, "  %s screen: %s\n", model, text.c_str());
    }
}

TEST(lib5_ac41_break_alias_ent_refused) {
    for (const ModelId& m : {kPC1500, kPC1500A, kPC1600}) {
        const auto* p = cu1600::typingProfile(m);
        auto one = [p](const char* n) {
            const auto r = p->name(n);
            return r && r->size() == 1 ? (*r)[0].key.value : std::string("refused");
        };
        CHECK_EQ(one("break"), std::string("on"));
        CHECK_EQ(one("BREAK"), std::string("on"));
        CHECK_EQ(one("Enter"), std::string("enter"));
        CHECK_EQ(one("SHIFT"), std::string("shift"));
        CHECK_EQ(one("ent"), std::string("refused"));   // OQ-6
        CHECK_EQ(one("x"), std::string("x"));
        CHECK_EQ(one("nokey"), std::string("refused"));
        // LIB-6 AC-44 (M-46): the PC-1600's ctrl, kbii and bs are no names
        // (preset.md §5.1); `key-down` still presses them.
        CHECK_EQ(one("kbii"), std::string("refused"));
        CHECK_EQ(one("ctrl"), std::string("refused"));
        CHECK_EQ(one("rcl"), std::string("rcl"));
    }
}

// The harness commands `key` and `type` on a Sharp machine (AC-15 to AC-17;
// the preset parts are LIB-6's, M-46).
TEST(lib5_ac17_harness_key_and_type_on_a_sharp_machine) {
    SKIP_UNLESS(romsPresent(), "no ROMs in roms/");
    auto product = cu1600::makeHarnessProduct();
    std::string error;
    auto server = calcu::harness::Server::start(*product, {}, &error);
    CHECK(server != nullptr);
    if (!server) return;
    auto reply = [&](const std::string& l) { return server->line(l).text; };
    CHECK_EQ(reply("key CL"), std::string("done\n"));
    reply("wait 500ms");
    CHECK_EQ(reply("type NEW0"), std::string("done\n"));
    reply("wait 1s");
    CHECK_EQ(reply("type PRINT 6*7"), std::string("done\n"));   // the reply when ENTER is up: not yet idle
    reply("wait 2s");
    CHECK(reply("display-text").find("42") != std::string::npos);
    CHECK_EQ(reply("key ent"), std::string("refused: Unknown key: ent.\n"));
    CHECK_EQ(reply("type 1\xC3\x84"), std::string("refused: No key types the character \"\xC3\x84\".\n"));   // the PC-1500A
    server->finish();
}

} // namespace
