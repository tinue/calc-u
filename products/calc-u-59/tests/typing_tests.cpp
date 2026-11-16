// LIB-5 Calc-U-59 (design §6, T59-0 to T59-5): the library's typing on the
// TI models: the profile (AC-20), held until sampled (TR, D-3, AC-25), the
// keyboard map (AC-21), two keys (AC-23), on-screen keys (AC-24), the names
// of `key` (AC-26) and today's key tests ported (AC-27).
#include "SessionRig.hpp"
#include "TestHarness.hpp"
#include "../app/KeyIds.hpp"
#include "../app/KeyNames.hpp"
#include "../app/TITyping.hpp"
#include "sessions/MachineSession.hpp"
#include "typing/TypingTopics.hpp"
#include <cstdio>
#include <set>
#include <string>
#include <vector>

using namespace cu59;
using namespace std::chrono_literals;
using calcu::machine::KeyId;
using calcu::machine::Ticks;
using calcu::typing::HostKeyEvent;
using calcu::typing::Mapped;
using calcu::typing::NamedKey;
using calcu::typing::Press;
using Value = calcu::sessions::Value;

namespace {

// The keys the engine puts down and lets up, with their ticks.
struct KeyLog final : calcu::typing::KeyObserver {
    struct E {
        std::string key;
        bool down;
        Ticks tick;
    };
    std::vector<E> events;
    void keyChanged(const KeyId& k, bool down, Ticks t) override { events.push_back({k.value, down, t}); }
    std::vector<std::string> keys() const {
        std::vector<std::string> out;
        for (const auto& e : events) out.push_back(e.key + (e.down ? "+" : "-"));
        return out;
    }
    Ticks held(std::size_t i) const {   // the hold of the i-th press
        std::size_t n = 0;
        for (std::size_t a = 0; a < events.size(); ++a)
            if (events[a].down && n++ == i)
                for (std::size_t b = a + 1; b < events.size(); ++b)
                    if (!events[b].down && events[b].key == events[a].key) return events[b].tick - events[a].tick;
        return 0;
    }
};

struct TIRig : testroms::PartRig {
    KeyLog log;
    // The PC-100C as 2.3.0's start had it (since M-54 the built-in default
    // has none, LIB-18 AC-18): the print loops and Adv need it.
    explicit TIRig(Model m = Model::TI59) : PartRig(m) {
        attach("printer", "PC-100C");
        engine().setObserver(&log);
    }
    ~TIRig() { engine().setObserver(nullptr); }
    calcu::typing::TypingEngine& engine() { return part().typing()->engine(); }
    void down(const char* key, int pointer = 1) {
        post(calcu::typing::kScreenKeyDown, Value::object({{"key", key}, {"pointer", pointer}}));
    }
    void up(const char* key, int pointer = 1) {
        post(calcu::typing::kScreenKeyUp, Value::object({{"key", key}, {"pointer", pointer}}));
    }
    void host(const HostKeyEvent& e) { post(calcu::typing::kHostKeyDown, calcu::typing::toValue(e)); }
    void hostUp(const std::string& physical) {
        post(calcu::typing::kHostKeyUp, Value::object({{"physical", physical}}));
    }
    calcu::sessions::CallId key(std::initializer_list<const char*> names) {
        Value::Array a;
        for (const char* n : names) a.push_back(Value(n));
        return post(calcu::typing::kKey, Value::object({{"names", Value(std::move(a))}}));
    }
    bool ready() { return machine().isKeyReady(); }
    bool idleWait(std::chrono::milliseconds limit = 10s) { return until([&] { return ready(); }, limit); }
    Ticks minHold() const { return typingProfile(Model::TI59).minHold; }
    std::vector<KeyId> held() const {
        const auto* h = state<calcu::typing::HeldKeys>(calcu::typing::kHeldKeysTopic);
        return h ? h->live : std::vector<KeyId>{};
    }
};

HostKeyEvent letter(char c, bool shift = false) {
    HostKeyEvent e;
    e.physical = std::string("Key") + static_cast<char>(c - 'a' + 'A');
    e.carried = static_cast<char32_t>(c);
    e.produced = static_cast<char32_t>(shift ? c - 'a' + 'A' : c);
    e.mods.shift = shift;
    return e;
}
HostKeyEvent produced(char32_t c, const std::string& physical = "Other") {
    HostKeyEvent e;
    e.physical = physical;
    e.produced = c;
    return e;
}
HostKeyEvent named(NamedKey k) {
    HostKeyEvent e;
    e.physical = std::string(calcu::typing::namedKeyName(k));
    e.named = k;
    return e;
}

std::vector<std::string> ids(const std::optional<Mapped>& m) {
    std::vector<std::string> out;
    if (m)
        for (const auto& p : m->presses) out.push_back(p.key.value);
    return out;
}
std::vector<std::string> ids(const std::optional<std::vector<Press>>& p) {
    std::vector<std::string> out;
    if (p)
        for (const auto& x : *p) out.push_back(x.key.value);
    return out;
}

const std::string kPrintLoop = "76 11 99 61 11";   // Lbl A Prt GTO A
const std::string kRunLoop = "76 11 61 11";        // Lbl A GTO A
const std::string kPauseLoop = "76 11 66 61 11";   // Lbl A Pause GTO A

// A preset of format 2 with `keycodes` for the rig's model, the Master
// Library module and the PC-100C.
std::string programPreset(TIRig& r, const std::string& keycodes) {
    return "format-version: 2\nmodel: " + modelId(r.part().model()).value +
           "\nslot-1: { name: ML }\nprinter: PC-100C\nprogram:\n  keycodes: |\n    " + keycodes + "\n";
}

// A program loaded and started with A; true once it runs.
bool startProgram(TIRig& r, const std::string& program) {
    r.load(programPreset(r, program));
    if (!r.idleWait()) return false;
    r.down("A");
    r.up("A");
    return r.until([&] { return !r.ready(); }, 2s);
}

// A tap of R/S `phase` after the first printed line of the print loop (a
// line about every 0.3 s); true
// when the program stopped (back in the scan loop, no line printed after).
struct Tap {
    bool stopped = false;
    double heldMs = 0;
};
Tap tapRsOnPrintLoop(Model model, bool full, std::uint32_t samples, std::chrono::milliseconds phase) {
    calcu::typing::TypingProfile p = typingProfile(model);   // outlives the rig that uses it
    if (samples == 0) p.holdUntilSampled.reset();
    else p.holdUntilSampled->samplesNeeded = samples;
    TIRig r(model);
    if (full) r.post(calcu::sessions::kSpeedCommand, Value::object({{"full", true}}));
    Tap out;
    r.load(programPreset(r, kPrintLoop));
    if (!r.idleWait()) return out;
    r.engine().setMachine(r.session().machineSession().machine(), &p, nullptr);   // this rule from now on
    r.down("A");
    r.up("A");
    if (!r.until([&] { return !r.printedText().empty(); }, 10s)) return out;
    r.runFor(phase);
    r.down("R/S");
    r.up("R/S");
    r.runFor(4s);
    const auto lines = r.printedText().size();
    r.runFor(3s);
    out.stopped = r.ready() && r.printedText().size() == lines;
    for (std::size_t i = 0; i < r.log.events.size(); ++i)
        if (r.log.events[i].key == "R/S" && !r.log.events[i].down && i > 0)
            out.heldMs = 1000.0 * double(r.log.events[i].tick - r.log.events[i - 1].tick) /
                         r.machine().descriptor().tickRate;
    return out;
}

} // namespace

// ── T59-0: the sample rule (§3.4, D-3) ──────────────────────────────────────

TEST(lib5_tr_sample_rule_measurement) {
    // The smallest samplesNeeded with which a tap of R/S stops `Lbl A Prt
    // GTO A` in each of four phases, on the three models, at regular and
    // full speed. 0: the 100 ms minimum alone (FS 5: not always enough).
    std::printf("T59-0  model    speed    samples  phase(ms)  stopped  held(ms)\n");
    const Model models[] = {Model::TI59, Model::TI58, Model::TI58C};
    bool chosenAlways = true;
    for (Model m : models)
        for (bool full : {false, true})
            for (std::uint32_t n : {0u, 1u, 2u})
                for (auto phase : {0ms, 75ms, 150ms, 225ms}) {
                    const Tap t = tapRsOnPrintLoop(m, full, n, phase);
                    std::printf("T59-0  %-7s  %-7s  %7u  %9lld  %-7s  %8.1f\n", info(m).displayName,
                                full ? "full" : "regular", n, static_cast<long long>(phase.count()),
                                t.stopped ? "yes" : "no", t.heldMs);
                    if (n == kTISamplesNeeded && !t.stopped) chosenAlways = false;
                }
    CHECK(chosenAlways);   // the chosen rule stops the loop in every phase (AC-25)
}

TEST(lib5_tr_idle_tap_stays_100ms_adv_as_without_the_rule) {
    // With the rule a key at the idle loop is held the 100 ms (it is read at
    // once) and gives one keycode; Adv (a key that repeats while held) feeds
    // the lines of a 100 ms press: two, as without the rule (measured; the
    // design's "one line" guessed low, 2.3.0's KeySequencer says "a couple").
    TIRig r;
    CHECK(r.idleWait());
    r.down("5");
    r.up("5");
    r.runFor(300ms);
    CHECK(r.log.held(0) >= r.minHold() && r.log.held(0) < r.minHold() + r.minHold() / 20);
    CHECK_EQ(testroms::displayText(r.machine()), std::string("5"));
    const auto before = r.printedText().size();
    const auto c = r.key({"Adv"});
    CHECK(r.until([&] { return r.reply(c) != nullptr; }, 3s));
    r.runFor(2s);
    CHECK_EQ(r.printedText().size(), before + 2);
    CHECK(r.log.held(2) < r.minHold() + r.minHold() / 20);   // the . of 2nd . held 100 ms
}

// ── T59-1: the TI profile, sampled, pause (AC-20, AC-25) ────────────────────

TEST(lib5_ac20_ti_profile) {
    for (Model m : {Model::TI59, Model::TI58, Model::TI58C}) {
        const auto& p = typingProfile(m);
        const auto rate = tiCatalogue().model(modelId(m))->tickRate;
        CHECK_EQ(p.minHold, calcu::typing::ticksOf(0.100, rate));   // 100 ms of emulated time
        CHECK_EQ(p.typed.hold, p.minHold);
        CHECK_EQ(p.typed.gap, Ticks{0});
        CHECK(p.queuedWaitsForReadiness && p.waitsForReadinessAfterRelease);
        CHECK(p.firstPressOfStep == calcu::typing::StartRule::AsQueued);
        CHECK(!p.firstPressBound);   // EC-3: waits without limit
        CHECK(p.holdUntilSampled && p.holdUntilSampled->samplesNeeded == kTISamplesNeeded);
        CHECK(!p.charKeys && !p.enter);   // no characters, no `type` (AC-16, D-8)
        CHECK(typingProduct().profile(modelId(m)) == &p);
        CHECK(!typingProduct().typesText());
    }
}

TEST(lib5_ac20_freeze_inside_hold_key_up_on_frozen_machine) {
    TIRig r;
    CHECK(r.idleWait());
    r.down("5");
    r.runFor(30ms);
    // A pause where the machine stands (as a pause-on-start firing): the run control's (LIB-10).
    r.part().levelRules()->landNextPauseAtOnce();
    r.part().debuggerPart()->runControl().pause(calcu::debugger::kCpu);
    r.frame();
    const Ticks frozenAt = r.ticks();
    r.up("5");
    r.runFor(40ms);   // less than the rest of the 100 ms
    CHECK_EQ(r.log.events.size(), std::size_t(1));
    r.runFor(100ms);   // the rest passes in host time
    CHECK_EQ(r.log.events.size(), std::size_t(2));
    CHECK(r.log.events.size() == 2 && r.log.events[1].tick == frozenAt);   // up on the paused machine
    CHECK_EQ(r.ticks(), frozenAt);
    // A press that has not gone down yet waits for the machine to run.
    r.host(letter('a', true));   // 2nd, A: a sequence (presses that wait)
    r.runFor(300ms);
    CHECK_EQ(r.log.events.size(), std::size_t(2));
    r.part().debuggerPart()->runControl().resume();
    r.runFor(1s);
    CHECK(r.log.keys().size() == 6 && r.log.keys()[2] == "2nd+" && r.log.keys()[4] == "A+");
}

TEST(lib5_ac25_rs_stops_running_program) {
    for (const std::string* prog : {&kRunLoop, &kPauseLoop}) {
        TIRig r;
        CHECK(startProgram(r, *prog));
        r.runFor(500ms);
        CHECK(!r.ready());   // runs
        const Ticks t = r.ticks();
        r.down("R/S");   // live: down at once, whatever readiness says (AC-2)
        CHECK(!r.log.events.empty() && r.log.events.back().key == "R/S" && r.log.events.back().tick == t);
        r.up("R/S");
        CHECK(r.until([&] { return r.ready(); }, 3s));   // the program stopped
    }
}

TEST(lib5_ac25_rs_stops_printing_loop_every_phase) {
    for (Model m : {Model::TI59, Model::TI58, Model::TI58C})
        for (bool full : {false, true})
            for (auto phase : {0ms, 75ms, 150ms, 225ms}) CHECK(tapRsOnPrintLoop(m, full, kTISamplesNeeded, phase).stopped);
}

TEST(lib5_ac25_gto_acted_on) {
    // At the prompt GTO A R/S runs the program from A (keyed live).
    TIRig r;
    r.load(programPreset(r, kRunLoop));
    CHECK(r.idleWait());
    for (const char* k : {"GTO", "A", "R/S"}) {
        r.down(k);
        r.up(k);
        r.runFor(300ms);
    }
    CHECK(!r.ready());   // runs
    // While it runs, GTO goes down at once and the ROM leaves the program
    // running, as a raw press of GTO does (2.3.0's tap).
    TIRig raw;
    CHECK(startProgram(raw, kRunLoop));
    raw.post(calcu::sessions::kKeyDownCommand, Value::object({{"key", "GTO"}}));
    raw.runFor(100ms);
    raw.post(calcu::sessions::kKeyUpCommand, Value::object({{"key", "GTO"}}));
    raw.runFor(1s);
    const Ticks t = r.ticks();
    r.down("GTO");
    CHECK(!r.log.events.empty() && r.log.events.back().key == "GTO" && r.log.events.back().tick == t);
    r.up("GTO");
    r.runFor(1s);
    CHECK_EQ(r.ready(), raw.ready());
}

// ── T59-2: the keyboard map (AC-21) ─────────────────────────────────────────

TEST(lib5_ac21_the_26_bindings) {
    TIKeyboardMap map;
    using V = std::vector<std::string>;
    std::vector<std::string> bound;
    for (char c : std::string("abcdex")) {
        const auto m = map.map(letter(c));
        CHECK(m && m->held && m->presses.size() == 1);
        bound.push_back(ids(m).empty() ? "" : ids(m)[0]);
    }
    CHECK(bound == (V{"A", "B", "C", "D", "E", "\xC3\x97"}));
    for (char c = '0'; c <= '9'; ++c) CHECK(ids(map.map(produced(static_cast<char32_t>(c)))) == V{std::string(1, c)});
    CHECK(ids(map.map(produced('.'))) == V{"."});
    CHECK(ids(map.map(produced(','))) == V{"."});
    CHECK(ids(map.map(produced('+'))) == V{"+"});
    CHECK(ids(map.map(produced('-'))) == V{"-"});
    CHECK(ids(map.map(produced('*'))) == V{"\xC3\x97"});
    CHECK(ids(map.map(produced('/'))) == V{"\xC3\xB7"});
    CHECK(ids(map.map(produced('='))) == V{"="});
    CHECK(ids(map.map(named(NamedKey::Return))) == V{"="});
    CHECK(ids(map.map(named(NamedKey::Enter))) == V{"="});
    CHECK(ids(map.map(produced('('))) == V{"("});
    CHECK(ids(map.map(produced(')'))) == V{")"});
    CHECK(ids(map.map(produced('<'))) == V{"+/-"});
    CHECK(ids(map.map(produced('>'))) == V{"EE"});
    CHECK(ids(map.map(named(NamedKey::Escape))) == V{"CLR"});
    // The 26 keys and no others: 5 letters, x, 10 digits, ., + - × ÷, =, ( ), +/-, EE, CLR.
    std::set<std::string> keys;
    for (char32_t c = 1; c < 0x80; ++c) {
        for (const auto& k : ids(map.map(produced(c)))) keys.insert(k);
        if ((c >= 'a' && c <= 'z'))
            for (const auto& k : ids(map.map(letter(static_cast<char>(c))))) keys.insert(k);
    }
    for (NamedKey k : {NamedKey::Escape, NamedKey::Return, NamedKey::Enter, NamedKey::Tab, NamedKey::Backspace,
                       NamedKey::Up, NamedKey::F1})
        for (const auto& x : ids(map.map(named(k)))) keys.insert(x);
    CHECK_EQ(keys.size(), std::size_t(26));
    CHECK(ids(map.map(letter('f'))).empty());   // an unbound letter
    CHECK(ids(map.map(named(NamedKey::Tab))).empty());
}

TEST(lib5_ac21_letter_by_carried_letter) {
    TIKeyboardMap map;
    using V = std::vector<std::string>;
    // OQ-2: the letter the key carries counts, not its position: the key at
    // QWERTY's Q carries A on AZERTY.
    HostKeyEvent azertyA;
    azertyA.physical = "KeyQ";
    azertyA.carried = 'a';
    azertyA.produced = 'a';
    CHECK(ids(map.map(azertyA)) == V{"A"});
    // Shift means 2nd: a sequence of two presses (AC-8).
    const auto shifted = map.map(letter('e', true));
    CHECK(shifted && !shifted->held && ids(shifted) == (V{"2nd", "E"}));
    CHECK(ids(map.map(letter('x', true))) == (V{"2nd", "\xC3\x97"}));
    // For anything else the character the layout produced: Shift+. gives > (EE).
    HostKeyEvent shiftDot = produced('>', "Period");
    shiftDot.mods.shift = true;
    CHECK(ids(map.map(shiftDot)) == V{"EE"});
    CHECK(ids(map.map(produced(U'ä'))).empty());   // a non-ASCII character
}

TEST(lib5_ac21_command_modifiers_map_nothing) {
    TIKeyboardMap map;
    for (int which = 0; which < 3; ++which) {
        HostKeyEvent a = letter('a');
        HostKeyEvent five = produced('5');
        for (HostKeyEvent* e : {&a, &five}) {
            if (which == 0) e->mods.command = true;
            if (which == 1) e->mods.control = true;
            if (which == 2) e->mods.alt = true;
            CHECK(!map.map(*e));
        }
    }
}

TEST(lib5_ac22_host_key_reaches_the_machine_through_the_session) {
    TIRig r;
    CHECK(r.idleWait());
    r.host(produced('7', "Digit7"));
    CHECK(r.log.keys() == std::vector<std::string>{"7+"});
    CHECK(r.held() == std::vector<KeyId>{KeyId{"7"}});
    r.runFor(200ms);
    r.hostUp("Digit7");
    r.runFor(300ms);
    CHECK(r.log.keys() == (std::vector<std::string>{"7+", "7-"}));
    CHECK(r.held().empty());
    CHECK_EQ(testroms::displayText(r.machine()), std::string("7"));
}

// ── T59-4: two keys, names (AC-23, AC-26) ───────────────────────────────────

TEST(lib5_ac23_second_key_does_not_release_first) {
    TIRig r;
    CHECK(r.idleWait());
    r.host(produced('7', "Digit7"));
    r.runFor(300ms);
    r.host(produced('1', "Digit1"));   // held together: both down (row 59-13)
    CHECK(r.log.keys() == (std::vector<std::string>{"7+", "1+"}));
    CHECK(r.held() == (std::vector<KeyId>{KeyId{"7"}, KeyId{"1"}}));
    r.hostUp("Digit1");
    r.runFor(200ms);
    r.hostUp("Digit7");
    r.runFor(200ms);
    CHECK(r.log.keys() == (std::vector<std::string>{"7+", "1+", "1-", "7-"}));
    // Two keys of one matrix row: the ROM rejects the pair and reads neither
    // (TMC0501's multi-key rule), so the later one is held until maxHold.
    TIRig s;
    CHECK(s.idleWait());
    s.host(produced('7', "Digit7"));
    s.runFor(300ms);
    s.host(produced('8', "Digit8"));
    s.hostUp("Digit8");
    s.runFor(300ms);
    CHECK(s.log.keys() == (std::vector<std::string>{"7+", "8+"}));
    s.hostUp("Digit7");
    s.runFor(1500ms);
    CHECK(s.log.keys() == (std::vector<std::string>{"7+", "8+", "7-", "8-"}));
    CHECK(s.log.held(1) <= typingProfile(Model::TI59).holdUntilSampled->maxHold + 512);
}

TEST(lib5_ac24_screen_key_through_the_engine_minimum_hold) {
    TIRig r;
    CHECK(r.idleWait());
    r.down("5");
    r.up("5");   // a click shorter than the debounce
    r.runFor(300ms);
    CHECK(r.log.keys() == (std::vector<std::string>{"5+", "5-"}));
    CHECK(r.log.held(0) >= r.minHold());
    CHECK_EQ(testroms::displayText(r.machine()), std::string("5"));
    const auto bad = r.post(calcu::typing::kScreenKeyDown, Value::object({{"key", "Q"}, {"pointer", 1}}));
    CHECK(r.reply(bad) && !r.reply(bad)->done);   // no such key (LIB-1 EC-2)
}

TEST(lib5_ac26_names_of_preset_md_8_1) {
    using V = std::vector<std::string>;
    // The 45 key IDs and their other spellings, any case.
    for (const auto& k : tiCatalogue().models().front().keys) {
        CHECK(ids(tiKeyName(k.id.value)) == V{k.id.value});
        for (const auto& o : k.otherSpellings) CHECK(ids(tiKeyName(o)) == V{k.id.value});
    }
    CHECK(ids(tiKeyName("lnx")) == V{"lnx"});
    CHECK(ids(tiKeyName("LNX")) == V{"lnx"});
    CHECK(ids(tiKeyName("r/s")) == V{"R/S"});
    CHECK(ids(tiKeyName("x")) == V{"\xC3\x97"});
    // Second functions: 2nd, then the key.
    CHECK(ids(tiKeyName("Pgm")) == (V{"2nd", "LRN"}));
    CHECK(ids(tiKeyName("pgm")) == (V{"2nd", "LRN"}));
    CHECK(ids(tiKeyName("A'")) == (V{"2nd", "A"}));
    CHECK(ids(tiKeyName("E\xE2\x80\xB2")) == (V{"2nd", "E"}));
    CHECK(ids(tiKeyName("P/R")) == (V{"2nd", "x\xE2\x87\x84t"}));
    CHECK(ids(tiKeyName("sum+")) == (V{"2nd", "5"}));
    CHECK(ids(tiKeyName("xbar")) == (V{"2nd", "6"}));
    CHECK(ids(tiKeyName("STF")) == (V{"2nd", "RST"}));
    CHECK(ids(tiKeyName("pi")) == (V{"2nd", "3"}));
    CHECK(ids(tiKeyName("IxI")) == (V{"2nd", "\xC3\xB7"}));
    CHECK(ids(tiKeyName("x>=t")) == (V{"2nd", "4"}));
    CHECK(ids(tiKeyName("adv")) == (V{"2nd", "."}));
    // §8.1.1: every printer name, the merged codes as key sequences; HIR none.
    CHECK(ids(tiKeyName("PG*")) == (V{"2nd", "LRN", "2nd", "y\xCB\xA3"}));
    CHECK(ids(tiKeyName("RTN")) == (V{"INV", "SBR"}));
    CHECK(ids(tiKeyName("go*")) == (V{"GTO", "2nd", "y\xCB\xA3"}));
    CHECK(!tiKeyName("HIR"));
    for (int kc = 0; kc < 100; ++kc) {
        const std::string n = mnemonic(static_cast<uint8_t>(kc));
        if (n == "HIR" || n.rfind('?', 0) == 0) continue;
        CHECK(tiKeyName(n).has_value());
    }
    // In the session: `key` with names.
    TIRig r;
    CHECK(r.idleWait());
    const auto c = r.key({"2", "x^2"});   // 2 x²: 4
    CHECK(r.until([&] { return r.reply(c) != nullptr; }, 3s));
    CHECK(r.reply(c) && r.reply(c)->done);
    r.runFor(300ms);
    CHECK_EQ(testroms::displayText(r.machine()), std::string("4"));
}

TEST(lib5_ac26_2nd_before_second_function_once) {
    using V = std::vector<std::string>;
    CHECK(ids(tiKeyNames({"2nd", "Pgm"})) == (V{"2nd", "LRN"}));
    CHECK(ids(tiKeyNames({"Pgm"})) == (V{"2nd", "LRN"}));
    CHECK(ids(tiKeyNames({"2nd", "LRN"})) == (V{"2nd", "LRN"}));
    CHECK(ids(tiKeyNames({"2nd", "CLR"})) == (V{"2nd", "CLR"}));
    CHECK(ids(tiKeyNames({"2ND", "pi"})) == (V{"2nd", "3"}));
    CHECK(ids(tiKeyNames({"2nd", "2nd"})) == (V{"2nd", "2nd"}));
    CHECK(!tiKeyNames({"2nd", "nothing"}));
    // In the session the command presses 2nd once.
    TIRig r;
    CHECK(r.idleWait());
    const auto c = r.key({"2nd", "pi"});
    CHECK(r.until([&] { return r.reply(c) != nullptr; }, 3s));
    CHECK(r.log.keys() == (std::vector<std::string>{"2nd+", "2nd-", "3+", "3-"}));
}

TEST(lib5_ac26_old_names_refused) {
    for (const char* n : {"2ndCLR", "P>R", "S+", "mean", "StFlg", "IfFlg", "ent", "nothing"}) CHECK(!tiKeyName(n));
    TIRig r;
    CHECK(r.idleWait());
    const auto c = r.key({"1", "S+"});
    CHECK(r.reply(c) && !r.reply(c)->done && r.reply(c)->reason == "Unknown key: S+.");
    CHECK(r.log.events.empty());   // nothing typed
    const auto t = r.post(calcu::typing::kType, Value::object({{"text", "1"}}));
    CHECK(r.reply(t) && !r.reply(t)->done && r.reply(t)->reason == "Unknown command: type.");   // D-8
}

// ── T59-5: today's key tests, with emulated time (AC-27) ────────────────────

TEST(lib5_ac27_sequence_key_waits_readiness_held_min) {
    // shared_cooperative_tests.cpp:111-186 at the pin: a key of a sequence
    // waits for key readiness and is held exactly the minimum.
    TIRig r;
    CHECK(r.idleWait());
    r.host(letter('a', true));   // 2nd, A
    r.runFor(1s);
    CHECK(r.log.keys() == (std::vector<std::string>{"2nd+", "2nd-", "A+", "A-"}));
    const Ticks inst = 128;   // one instruction at most (C59-1: 32 ticks, 128 in IDLE)
    for (std::size_t i = 0; i < 2; ++i) CHECK(r.log.held(i) >= r.minHold() && r.log.held(i) < r.minHold() + inst * 8);
    // The second waited for the machine to be ready after the first.
    CHECK(r.log.events.size() == 4 && r.log.events[2].tick > r.log.events[1].tick);
}

TEST(lib5_ac27_key_up_deferred) {
    TIRig r;
    CHECK(r.idleWait());
    r.host(produced('5', "Digit5"));
    r.hostUp("Digit5");   // right after: deferred to the minimum
    CHECK(r.log.keys() == std::vector<std::string>{"5+"});
    r.runFor(50ms);
    CHECK(r.log.keys() == std::vector<std::string>{"5+"});
    r.runFor(200ms);
    CHECK(r.log.keys() == (std::vector<std::string>{"5+", "5-"}));
    CHECK(r.log.held(0) >= r.minHold());
}

TEST(lib5_ac27_shift_binding_two_presses_new_key_drops_second) {
    TIRig r;
    CHECK(r.idleWait());
    r.host(letter('a', true));
    r.runFor(20ms);
    r.host(produced('5', "Digit5"));   // drops the A still waiting
    r.hostUp("Digit5");
    r.runFor(1s);
    CHECK(r.log.keys() == (std::vector<std::string>{"2nd+", "2nd-", "5+", "5-"}));
}

TEST(lib5_ac27_map_rules) {
    // shared_tables_tests.cpp:42-61 at the pin (keyboard_map_rules), on TIKeyboardMap.
    TIKeyboardMap map;
    using V = std::vector<std::string>;
    CHECK(ids(map.map(letter('a'))) == V{"A"});
    CHECK(ids(map.map(letter('a', true))) == (V{"2nd", "A"}));   // Shift+A → 2nd A = A′
    CHECK(ids(map.map(letter('e', true))) == (V{"2nd", "E"}));
    CHECK(ids(map.map(letter('f'))).empty());                     // unbound letter
    CHECK(ids(map.map(letter('x'))) == V{"\xC3\x97"});            // × via x
    CHECK(ids(map.map(letter('x', true))) == (V{"2nd", "\xC3\x97"}));
    CHECK(ids(map.map(produced('>'))) == V{"EE"});                // Shift+. → '>' → EE
    CHECK(ids(map.map(produced('<'))) == V{"+/-"});               // Shift+, → '<' → +/−
    CHECK(ids(map.map(produced('*'))) == V{"\xC3\x97"});          // produced char wins
    CHECK(ids(map.map(named(NamedKey::Escape))) == V{"CLR"});
    CHECK(ids(map.map(named(NamedKey::Return))) == V{"="});
    CHECK(ids(map.map(named(NamedKey::Enter))) == V{"="});
    CHECK(ids(map.map(produced('5'))) == V{"5"});
    HostKeyEvent cmd = letter('a');
    cmd.mods.command = true;
    CHECK(ids(map.map(cmd)).empty());                              // Command chords never reach the calculator
    HostKeyEvent umlaut;
    umlaut.carried = U'ä';
    umlaut.produced = U'ä';
    CHECK(ids(map.map(umlaut)).empty());                           // non-ASCII letter
}

// ── EC-5 on the TI: reset, model switch ─────────────────────────────────────

TEST(lib5_ec5_reset_and_model_switch_release_held_keys) {
    TIRig r;
    CHECK(r.idleWait());
    r.host(produced('7', "Digit7"));
    CHECK_EQ(r.held().size(), std::size_t(1));
    r.post(calcu::startup::kResetCommand);
    CHECK(r.log.keys() == (std::vector<std::string>{"7+", "7-"}));   // up at once (D-11)
    CHECK(r.held().empty());
    r.host(produced('8', "Digit8"));
    r.post(calcu::startup::kModelCommand, Value::object({{"name", "TI-58"}}));
    CHECK(r.held().empty());
    CHECK(r.part().typing()->engine().idle());
}






// ── R-1: the exit check of M-36 (LIB-5 design §8; row 59-23) ────────────────

namespace {
std::string replaced(std::string text, const std::string& from, const std::string& to) {
    for (std::size_t at = text.find(from); at != std::string::npos; at = text.find(from, at + to.size()))
        text.replace(at, from.size(), to);
    return text;
}
std::vector<std::string> paperOf(const std::string& text) {
    testroms::SessionRig r;
    r.runFor(1s);
    const auto reply = r.operation(calcu::presets::kLoadPresetCommand,
                                   Value::object({{"content", text}, {"name", "printer-timing.ti59"}}), 120s);
    CHECK(reply.done);
    r.runFor(2s);
    return r.printedText();
}
} // namespace

TEST(lib5_exit_printer_timing_same_paper_at_regular_and_full_speed) {
    // examples/debug/printer-timing.ti59 (format 2 since M-54) with every
    // speed step at regular speed, and at full speed: a key of a preset (Prt,
    // Adv) is held 100 ms of emulated time and until read at every speed, so
    // both give the same paper, and the paper of the file as it is (2.3.0's
    // 104 lines).
    const auto text = testroms::readFile("examples/debug/printer-timing.ti59");
    CHECK(text.has_value());
    if (!text) return;
    const auto regular = paperOf(replaced(*text, "- full-speed:", "- regular-speed:"));
    const auto full = paperOf(replaced(*text, "- regular-speed:", "- full-speed:"));
    const auto asIs = paperOf(*text);
    std::printf("printer-timing: %zu lines at regular speed, %zu at full speed, %zu as it is\n", regular.size(),
                full.size(), asIs.size());
    CHECK(!regular.empty());
    CHECK(regular == full);
    CHECK(asIs == regular);
    CHECK_EQ(asIs.size(), std::size_t(104));
}
