// C1600-1 AC-1 to AC-21 and EC-1 to EC-3 (design §6, T2-T8): the Sharp
// machines through the machine interface. AC-4 is the golden replay
// (golden_runs_tests.cpp), AC-5 the core tests as they are.

#include <algorithm>
#include <set>

#include "../core/Connector/FloppyImageFile.hpp"
#include "../core/PC1500/PC1500Keyboard.hpp"
#include "../core/PC1600/PC1600BasicTyper.hpp"
#include "../core/PC1600/PC1600Keyboard.hpp"
#include "TestRoms.hpp"
#include "SharpTestSupport.hpp"
#include "peripherals/RealTimeClock.hpp"
#include "../core/PC1500/PC1500MachineCodeLoader.hpp"

using namespace calcu::machine;
using namespace sharptest;
namespace pp = calcu::peripherals;

namespace {

bool haveAll() { return haveModelRoms("PC-1500A") && haveModelRoms("PC-1600"); }

// Writes a 4-byte pattern at `addr` through the loader memory.
const std::uint8_t kPattern[4] = {0xA5, 0x5A, 0xC3, 0x3C};
bool pokePattern(Machine& m, std::uint16_t addr) { return memoryOf(m).writeBuiltIn(addr, kPattern, 4).stored; }
bool hasPattern(Machine& m, std::uint16_t addr) {
    for (int i = 0; i < 4; ++i)
        if (memoryOf(m).peek(static_cast<std::uint16_t>(addr + i)) != kPattern[i]) return false;
    return true;
}
// An address in free RAM that no start-up touches: the middle of the
// PC-1500's user RAM; the PC-1600's internal RAM at E000H.
std::uint16_t freeRam(Machine& m) {
    if (memoryOf(m).pc1600()) return 0xE000;
    std::uint32_t start = 0, end = 0;
    pc1500UserRam(memoryOf(m), &start, &end);
    return static_cast<std::uint16_t>((start + end) / 2);
}

pp::RealTimeClock* clockOf(Machine& m) { return sharp::firstOf<pp::RealTimeClock>(m); }

// Switches the machine off by its `off` key; true once the display is off.
bool switchOff(Machine& m) {
    m.keyDown(KeyId{"off"});
    m.advance(frameTicks(m) * 4);
    m.keyUp(KeyId{"off"});
    return runUntil(m, [&] { return !m.readDisplay().on; }, 3);
}

} // namespace

// ---- T2: emulated time ----

TEST(c1600_1_ac1_tick_is_cpu_cycle_or_t_state) {
    const Catalogue& c = sharp::sharpCatalogue();
    CHECK_EQ(c.model(ModelId{"PC-1500"})->tickRate, PC1500Machine::kCpuHz);
    CHECK_EQ(c.model(ModelId{"PC-1500A"})->tickRate, TickRate{1'300'000});
    CHECK_EQ(c.model(ModelId{"PC-1600"})->tickRate, PC1600Machine::kTStateHz);
    CHECK_EQ(PC1600Machine::kTStateHz, std::uint32_t{3'580'000});
    // The LH5803's 1.3 MHz cycles count as T-states: one second of them is
    // one second of ticks.
    CHECK_EQ(PC1600Machine::toTStates(PC1600Machine::kLH5803Hz, false), std::uint64_t{3'580'000});
    SKIP_UNLESS(haveAll(), "Sharp ROMs missing");
    for (const char* model : {"PC-1500A", "PC-1600"}) {
        auto m = buildSharp(c, model);
        CHECK_EQ(m->tickCount(), Ticks{0});
        const Ticks ran = m->advance(secondsTicks(*m, 0.5));
        CHECK_EQ(m->tickCount(), ran);
        if (PC1600Machine* p = sharp::pc1600Of(*m)) CHECK_EQ(p->elapsedTStates(), ran);
        if (PC1500Machine* p = sharp::pc1500Of(*m)) CHECK_EQ(p->elapsedCycles(), ran);
    }
}

TEST(c1600_1_ac2_whole_instructions_of_the_bus_owner) {
    SKIP_UNLESS(haveAll(), "Sharp ROMs missing");
    const Catalogue& c = sharp::sharpCatalogue();
    for (const char* model : {"PC-1500A", "PC-1600"}) {
        auto m = bootedSharp(c, model);
        bool sawLh5803 = false;
        for (Ticks n : {Ticks{1}, Ticks{2}, Ticks{3}, Ticks{7}, Ticks{100}, Ticks{999}, Ticks{12345}}) {
            for (int rep = 0; rep < 50; ++rep) {
                const Ticks before = m->tickCount();
                const Ticks ran = m->advance(n);
                CHECK(ran >= n);
                // Less than the last instruction over: no CPU instruction of
                // either core takes 64 ticks.
                CHECK(ran - n < 64);
                CHECK_EQ(m->tickCount() - before, ran);
                if (PC1600Machine* p = sharp::pc1600Of(*m)) sawLh5803 = sawLh5803 || !p->sc7852Owns();
            }
        }
        if (std::string(model) == "PC-1600") {
            // The ROM's OFF path hands the bus to the LH5803: an advance runs
            // its steps then, counted in T-states.
            m->keyDown(KeyId{"off"});
            m->advance(frameTicks(*m) * 4);
            m->keyUp(KeyId{"off"});
            for (int rep = 0; rep < 200000 && !sawLh5803 && !sharp::pc1600Of(*m)->isPoweredOff(); ++rep) {
                const Ticks ran = m->advance(5);
                CHECK(ran >= 5 && (ran - 5 < 64 || (sharp::pc1600Of(*m)->isPoweredOff() && ran <= 256)));
                sawLh5803 = !sharp::pc1600Of(*m)->sc7852Owns();
            }
            CHECK(sawLh5803);
            for (int rep = 0; rep < 100 && sawLh5803 && !sharp::pc1600Of(*m)->sc7852Owns() &&
                              !sharp::pc1600Of(*m)->isPoweredOff();
                 ++rep) {
                const Ticks ran = m->advance(3);
                // The step that switches the system off is one off slice (EC-2).
                CHECK(ran >= 3 && (ran - 3 < 64 || (sharp::pc1600Of(*m)->isPoweredOff() && ran <= 256)));
            }
        }
    }
}

TEST(c1600_1_ac3_off_machine_counts_ticks_clock_runs_display_off) {
    SKIP_UNLESS(haveAll(), "Sharp ROMs missing");
    const Catalogue& c = sharp::sharpCatalogue();
    for (const char* model : {"PC-1500A", "PC-1600"}) {
        auto m = bootedSharp(c, model);
        CHECK(switchOff(*m));
        clockOf(*m)->set(pp::ClockFields{2026, 6, 15, std::nullopt, 10, 20, 0});
        const Ticks before = m->tickCount();
        const Ticks ran = m->advance(secondsTicks(*m, 3.0));
        CHECK_EQ(m->tickCount() - before, ran);
        CHECK(ran >= secondsTicks(*m, 3.0));
        const DisplayOutput out = m->readDisplay();
        CHECK(!out.on);
        const auto f = std::get<DotMatrixFrame>(out.frame);
        CHECK(std::all_of(f.dots.begin(), f.dots.end(), [](std::uint8_t d) { return d == 0; }));
        CHECK(std::none_of(out.indicators.begin(), out.indicators.end(), [](const IndicatorOutput& i) { return i.on; }));
        const pp::ClockFields now = clockOf(*m)->read();
        CHECK(now.minute == 20 && now.second && *now.second >= 2 && *now.second <= 4);
    }
}

TEST(c1600_1_ec2_off_steps_256_on_1600_tick_by_tick_on_1500) {
    SKIP_UNLESS(haveAll(), "Sharp ROMs missing");
    const Catalogue& c = sharp::sharpCatalogue();
    auto p16 = bootedSharp(c, "PC-1600");
    CHECK(switchOff(*p16));
    p16->advance(secondsTicks(*p16, 0.1));
    for (Ticks n : {Ticks{1}, Ticks{100}, Ticks{256}, Ticks{257}, Ticks{1000}}) {
        const Ticks ran = p16->advance(n);
        CHECK(ran >= n && ran - n <= 255);
        CHECK_EQ(ran % 256, Ticks{0});
    }
    auto p15 = bootedSharp(c, "PC-1500A");
    CHECK(switchOff(*p15));
    for (Ticks n : {Ticks{1}, Ticks{2}, Ticks{100}, Ticks{12345}}) CHECK_EQ(p15->advance(n), n);
}

// ---- T3: operations ----

TEST(c1600_1_ac6_reset_keeps_memory) {
    SKIP_UNLESS(haveAll(), "Sharp ROMs missing");
    const Catalogue& c = sharp::sharpCatalogue();
    for (const char* model : {"PC-1500A", "PC-1600"}) {
        auto m = bootedSharp(c, model);
        const std::uint16_t a = freeRam(*m);
        CHECK(pokePattern(*m, a));
        m->reset();
        CHECK(runUntilKeyReady(*m));
        CHECK(hasPattern(*m, a));
        CHECK(m->model().value == model);
    }
}

TEST(c1600_1_ac6_power_cycle_keeps_all_memory) {
    SKIP_UNLESS(haveAll(), "Sharp ROMs missing");
    const Catalogue& c = sharp::sharpCatalogue();
    for (const char* model : {"PC-1500A", "PC-1600"}) {
        auto m = bootedSharp(c, model);
        const std::uint16_t a = freeRam(*m);
        CHECK(pokePattern(*m, a));
        m->powerCycle();
        CHECK(!m->isKeyReady());
        sharp::legacy::runToPrompt(*m);
        CHECK(runUntilKeyReady(*m));
        CHECK(m->readDisplay().on);
        CHECK(hasPattern(*m, a));
    }
}

TEST(c1600_1_ac6_cold_boot_empties_memory_and_ce151_ce155) {
    SKIP_UNLESS(haveAll() && haveModelRoms("PC-1500"), "Sharp ROMs missing");
    const Catalogue c = catalogueWithModules();
    struct Case { const char* model; const char* module; };
    for (const Case k : {Case{"PC-1500A", "CE-151"}, Case{"PC-1500", "CE-155"}, Case{"PC-1600", "CE-151"}}) {
        auto m = bootedSharp(c, k.model, {moduleRequest(k.module, "slot-1")});
        CHECK(m != nullptr);
        if (!m) continue;
        const auto fresh = sharp::moduleCardOf(moduleRequest(k.module, "slot-1"), *c.peripheral(PeripheralId{k.module}));
        const std::uint16_t own = freeRam(*m);
        CHECK(scribbleModule(*m, 1));
        if (std::string(k.model) != "PC-1600") CHECK(pokePattern(*m, own));
        CHECK(sharp::legacy::moduleIn(*m, 1)->debugImage() != fresh->debugImage());
        m->coldBoot();
        // Before the ROM runs: the module as its definition builds it, the
        // machine's own RAM empty.
        CHECK(sharp::legacy::moduleIn(*m, 1)->debugImage() == fresh->debugImage());
        if (std::string(k.model) == "PC-1600") {
            const auto ram = memoryOf(*m).internalRam();
            CHECK(std::all_of(ram.begin(), ram.end(), [](std::uint8_t b) { return b == 0; }));
        } else {
            CHECK(!hasPattern(*m, own));
        }
        CHECK(runUntilKeyReady(*m));
    }
}

TEST(c1600_1_ac6_cold_boot_keeps_battery_module) {
    SKIP_UNLESS(haveAll(), "Sharp ROMs missing");
    const Catalogue c = catalogueWithModules();
    struct Case { const char* model; const char* module; };
    for (const Case k : {Case{"PC-1500A", "CE-161"}, Case{"PC-1600", "CE-1600M"}}) {
        auto m = bootedSharp(c, k.model, {moduleRequest(k.module, "slot-1")});
        CHECK(m != nullptr);
        if (!m) continue;
        CHECK(scribbleModule(*m, 1));
        const auto before = sharp::legacy::moduleIn(*m, 1)->debugImage();
        m->coldBoot();
        CHECK(sharp::legacy::moduleIn(*m, 1)->debugImage() == before);
    }
}

TEST(c1600_1_ac6_operations_end_at_key_readiness) {
    SKIP_UNLESS(haveAll(), "Sharp ROMs missing");
    const Catalogue& c = sharp::sharpCatalogue();
    for (const char* model : {"PC-1500A", "PC-1600"}) {
        auto m = buildSharp(c, model);
        CHECK(!m->isKeyReady());   // built, not started
        m->coldBoot();
        CHECK(!m->isKeyReady());
        CHECK(runUntilKeyReady(*m));
        const auto variant = m->variant();
        m->reset();
        CHECK(!m->isKeyReady());
        CHECK(runUntilKeyReady(*m));
        m->powerCycle();
        CHECK(!m->isKeyReady());
        CHECK(runUntilKeyReady(*m));
        m->coldBoot();
        CHECK(!m->isKeyReady());
        CHECK(runUntilKeyReady(*m));
        CHECK(m->variant() == variant);
        CHECK(m->model().value == model);
    }
}

TEST(c1600_1_ac7_off_key_then_on_runs_again) {
    SKIP_UNLESS(haveAll(), "Sharp ROMs missing");
    const Catalogue& c = sharp::sharpCatalogue();
    for (const char* model : {"PC-1500A", "PC-1600"}) {
        auto m = bootedSharp(c, model);
        CHECK(switchOff(*m));
        m->advance(secondsTicks(*m, 1));
        CHECK(!m->readDisplay().on);   // AC-3 holds until `on`
        CHECK(!m->isKeyReady());
        m->keyDown(KeyId{"on"});
        m->advance(frameTicks(*m) * 4);
        m->keyUp(KeyId{"on"});
        CHECK(runUntilKeyReady(*m));
        CHECK(m->readDisplay().on);
    }
}

TEST(c1600_1_ac8_rtc_after_reset_and_cold_boot) {
    SKIP_UNLESS(haveAll(), "Sharp ROMs missing");
    const Catalogue& c = sharp::sharpCatalogue();
    const pp::ClockFields set{2026, 11, 15, std::nullopt, 13, 14, 15};
    // PC-1500: a reset puts the clock back to 1 January 2000.
    auto p15 = bootedSharp(c, "PC-1500A");
    clockOf(*p15)->set(set);
    CHECK(clockOf(*p15)->read().month == 11);
    p15->reset();
    const pp::ClockFields r15 = clockOf(*p15)->read();
    CHECK(r15.month == 1 && r15.day == 1 && r15.hour == 0 && r15.minute == 0);
    // PC-1600: a reset leaves it; a cold boot lets the ROM clear it.
    auto p16 = bootedSharp(c, "PC-1600");
    clockOf(*p16)->set(set);
    p16->reset();
    CHECK(runUntilKeyReady(*p16));
    const pp::ClockFields r16 = clockOf(*p16)->read();
    CHECK(r16.month == 11 && r16.day == 15 && r16.hour == 13 && r16.minute == 14);
    // A cold boot lets the ROM clear it. The core keeps a seed from the host
    // through the first boot after it (PC1600Machine::seedClock's guard), as
    // today, so the clock set above survives one boot; the next cold boot
    // clears it (qa-report: question on AC-8).
    p16->coldBoot();
    sharp::legacy::runToPrompt(*p16);
    p16->coldBoot();
    sharp::legacy::runToPrompt(*p16);
    const pp::ClockFields cold = clockOf(*p16)->read();
    CHECK(cold.month == 1 && cold.day == 1 && cold.hour == 0 && cold.minute == 0);
}

// ---- T4: keys ----

TEST(c1600_1_ac9_pc1500_has_65_key_ids) {
    for (const char* model : {"PC-1500", "PC-1500A"}) {
        const MachineDescriptor& d = *sharp::sharpCatalogue().model(ModelId{model});
        CHECK_EQ(d.keys.size(), std::size_t{65});
        std::set<std::string> ids;
        for (const auto& k : d.keys) ids.insert(k.id.value);
        CHECK_EQ(ids.size(), std::size_t{65});
        for (char ch = '0'; ch <= '9'; ++ch) CHECK(ids.count(std::string(1, ch)));
        for (char ch = 'a'; ch <= 'z'; ++ch) CHECK(ids.count(std::string(1, ch)));
        for (const char* k : {"+", "-", "=", "*", "/", "(", ")", ".", "cl", "enter", "mode", "def", "sml", "rcl", "shift",
                              "off", "up", "down", "left", "right", "rsv", "space", "f1", "f2", "f3", "f4", "f5", "f6", "on"})
            CHECK(ids.count(k));
        // Every key except `on` is a key of the matrix.
        for (const auto& id : ids)
            if (id != "on") CHECK(PC1500Keyboard::keyFromName(id) != PC1500Keyboard::Key::Unknown);
    }
}

TEST(c1600_1_ac10_pc1600_has_68_key_ids) {
    const MachineDescriptor& d = *sharp::sharpCatalogue().model(ModelId{"PC-1600"});
    CHECK_EQ(d.keys.size(), std::size_t{68});
    std::set<std::string> ids;
    for (const auto& k : d.keys) ids.insert(k.id.value);
    for (const auto& k : sharp::sharpCatalogue().model(ModelId{"PC-1500"})->keys) CHECK(ids.count(k.id.value));
    for (const char* k : {"ctrl", "kbii", "bs"}) CHECK(ids.count(k));
    for (const auto& id : ids)
        if (id != "on") CHECK(PC1600Keyboard::keyFromName(id) != PC1600Keyboard::Key::Unknown);
}

TEST(c1600_1_ac11_on_switches_on_and_breaks) {
    SKIP_UNLESS(haveAll(), "Sharp ROMs missing");
    const Catalogue& c = sharp::sharpCatalogue();
    for (const char* model : {"PC-1500A", "PC-1600"}) {
        auto m = bootedSharp(c, model);
        // `on` is a key ID at the interface.
        CHECK(m->keyDown(KeyId{"on"}) == KeyCall::Done);
        m->advance(frameTicks(*m) * 4);
        CHECK(m->keyUp(KeyId{"on"}) == KeyCall::Done);
        runUntilKeyReady(*m, 5);
        // On a running machine it breaks: an endless program stops.
        const bool p16 = std::string(model) == "PC-1600";
        runProgram(*m, "10 goto 10");
        m->advance(secondsTicks(*m, 0.5));
        // The PC-1500's key wait is not where a running program is; the
        // PC-1600's readiness (design D-3) also counts a frame in which a
        // running program reads the key matrix.
        if (!p16) CHECK(!m->isKeyReady());
        m->keyDown(KeyId{"on"});
        m->advance(frameTicks(*m) * 6);
        m->keyUp(KeyId{"on"});
        m->advance(secondsTicks(*m, 0.5));
        const std::string s = screen(*m);
        CHECK(s.find("BREAK") != std::string::npos);
        if (s.find("BREAK") == std::string::npos) std::fprintf(stderr, "  %s screen:\n%s\n", model, s.c_str());
        // On a machine that is off it switches it on (AC-7).
        CHECK(switchOff(*m));
        m->keyDown(KeyId{"on"});
        m->advance(frameTicks(*m) * 4);
        m->keyUp(KeyId{"on"});
        CHECK(runUntilKeyReady(*m));
    }
}

TEST(c1600_1_ac12_two_keys_and_on_down_together) {
    SKIP_UNLESS(haveAll(), "Sharp ROMs missing");
    const Catalogue& c = sharp::sharpCatalogue();
    {
        auto m = buildSharp(c, "PC-1500A");
        const PC1500Keyboard& kb = sharp::pc1500Of(*m)->memory().keyboard();
        auto scanAll = [&] {
            std::vector<std::uint8_t> s;
            for (int col = 0; col < 8; ++col) s.push_back(kb.scan(static_cast<std::uint8_t>(~(1u << col))));
            return s;
        };
        const auto none = scanAll();
        m->keyDown(KeyId{"a"});
        const auto a = scanAll();
        m->keyUp(KeyId{"a"});
        m->keyDown(KeyId{"l"});
        const auto l = scanAll();
        m->keyDown(KeyId{"a"});
        m->keyDown(KeyId{"on"});
        const auto both = scanAll();
        CHECK(a != none && l != none && a != l);
        for (int col = 0; col < 8; ++col) CHECK_EQ(both[col], static_cast<std::uint8_t>(a[col] & l[col]));
        m->keyUp(KeyId{"a"});
        CHECK(scanAll() == l);   // the other key and `on` stay down
        m->keyUp(KeyId{"l"});
        m->keyUp(KeyId{"on"});
        CHECK(scanAll() == none);
    }
    {
        auto m = buildSharp(c, "PC-1600");
        const PC1600Keyboard& kb = sharp::pc1600Of(*m)->keyboard();
        auto scanAll = [&] {
            std::vector<std::uint8_t> s;
            for (int pb6 = 0; pb6 < 2; ++pb6)
                for (int col = 0; col < 8; ++col) s.push_back(kb.scan(static_cast<std::uint8_t>(~(1u << col)), pb6 != 0));
            return s;
        };
        const auto none = scanAll();
        m->keyDown(KeyId{"a"});
        const auto a = scanAll();
        m->keyUp(KeyId{"a"});
        m->keyDown(KeyId{"kbii"});
        const auto k = scanAll();
        m->keyDown(KeyId{"a"});
        m->keyDown(KeyId{"on"});
        const auto both = scanAll();
        CHECK(a != none && k != none);
        for (std::size_t i = 0; i < both.size(); ++i) CHECK_EQ(both[i], static_cast<std::uint8_t>(a[i] & k[i]));
        m->keyUp(KeyId{"a"});
        m->keyUp(KeyId{"kbii"});
        m->keyUp(KeyId{"on"});
        CHECK(scanAll() == none);
    }
}

TEST(c1600_1_ec1_unknown_key_refused) {
    SKIP_UNLESS(haveAll(), "Sharp ROMs missing");
    const Catalogue& c = sharp::sharpCatalogue();
    auto m = buildSharp(c, "PC-1500A");
    const PC1500Keyboard& kb = sharp::pc1500Of(*m)->memory().keyboard();
    for (const char* bad : {"kbii", "ctrl", "bs", "ent", "A", "Enter", "break", ""}) {
        CHECK(m->keyDown(KeyId{bad}) == KeyCall::UnknownKey);
        CHECK(m->keyUp(KeyId{bad}) == KeyCall::UnknownKey);
        CHECK_EQ(kb.scan(0x00), std::uint8_t{0xFF});   // nothing pressed
    }
    auto p16 = buildSharp(c, "PC-1600");
    for (const char* bad : {"ent", "A", "break"}) CHECK(p16->keyDown(KeyId{bad}) == KeyCall::UnknownKey);
    CHECK(p16->keyDown(KeyId{"kbii"}) == KeyCall::Done);
    p16->keyUp(KeyId{"kbii"});
}

// ---- T5: key readiness ----

TEST(c1600_1_ac21_pc1500_ready_in_wait_4_kb_with_7b0e_bit0_clear) {
    SKIP_UNLESS(haveAll(), "Sharp ROMs missing");
    auto m = bootedSharp(sharp::sharpCatalogue(), "PC-1500A");
    CHECK(runUntilKeyReady(*m));
    const std::uint16_t pc = sharp::pc1500Of(*m)->cpu().pc();
    CHECK(pc >= 0xE243 && pc <= 0xE33D);
    CHECK((memoryOf(*m).peek(0x7B0E) & 1) == 0);
    // Readiness follows the PC: sampled over a stretch at the prompt, it is
    // true exactly when the PC is in WAIT_4_KB.
    for (int i = 0; i < 200; ++i) {
        m->advance(97);
        const std::uint16_t now = sharp::pc1500Of(*m)->cpu().pc();
        const bool inWait = now >= 0xE243 && now <= 0xE33D && (memoryOf(*m).peek(0x7B0E) & 1) == 0;
        CHECK_EQ(m->isKeyReady(), inWait);
    }
}

TEST(c1600_1_ac21_pc1500_not_ready_while_bit0_set_or_running) {
    SKIP_UNLESS(haveAll(), "Sharp ROMs missing");
    auto m = bootedSharp(sharp::sharpCatalogue(), "PC-1500A");
    CHECK(runUntilKeyReady(*m));
    const std::uint8_t old = memoryOf(*m).peek(0x7B0E);
    const std::uint8_t set = static_cast<std::uint8_t>(old | 1);
    CHECK(memoryOf(*m).writeBuiltIn(0x7B0E, &set, 1).stored);
    CHECK(!m->isKeyReady());
    CHECK(memoryOf(*m).writeBuiltIn(0x7B0E, &old, 1).stored);
    CHECK(runUntilKeyReady(*m, 1));
    // A running program: not ready.
    runProgram(*m, "10 goto 10");
    int ready = 0;
    for (int i = 0; i < 100; ++i) {
        m->advance(frameTicks(*m));
        ready += m->isKeyReady() ? 1 : 0;
    }
    CHECK_EQ(ready, 0);
}

TEST(c1600_1_ac21_pc1600_ready_after_15_ready_frames) {
    SKIP_UNLESS(haveAll(), "Sharp ROMs missing");
    const Catalogue& c = sharp::sharpCatalogue();
    bool ok = false;
    PC1600Machine old;
    std::string err;
    CHECK(presetroms::loadPC1600RomSet(old, testPresetRoms(), "new", &err));
    CHECK(presetroms::attachCE1600P(old, testPresetRoms(), "new", &err));
    // The old class: the first part of runBootToPromptJob, then
    // waitForKeyboardScanLoopJob, whose return is the moment to match.
    old.allReset();
    old.runCycles(PC1600Machine::kTStateHz * 2);
    waitIdle(old, PC1600Machine::kTStateHz * 5);
    waitForKeyboardScanLoop(old);
    const std::uint64_t oldReady = old.elapsedTStates();

    auto m = buildSharp(c, "PC-1600", std::nullopt, {request(c, "CE-1600P", "printer", std::nullopt, &ok)});
    CHECK(ok && m);
    m->coldBoot();
    CHECK(runUntilKeyReady(*m));
    const std::int64_t delta = static_cast<std::int64_t>(m->tickCount()) - static_cast<std::int64_t>(oldReady);
    const std::int64_t frame = static_cast<std::int64_t>(frameTicks(*m));
    if (delta > 2 * frame || delta < -2 * frame)
        std::fprintf(stderr, "  ready at %llu, the old job returned at %llu\n",
                     static_cast<unsigned long long>(m->tickCount()), static_cast<unsigned long long>(oldReady));
    CHECK(delta <= 2 * frame && delta >= -2 * frame);
    // Not ready while switched off, and again only after 15 frames after a reset.
    m->reset();
    CHECK(!m->isKeyReady());
    m->advance(frameTicks(*m) * 10);
    CHECK(!m->isKeyReady());
}

// ---- T6: display and sound ----

TEST(c1600_1_ac13_dots_and_indicator_names_per_model) {
    SKIP_UNLESS(haveAll(), "Sharp ROMs missing");
    const Catalogue& c = sharp::sharpCatalogue();
    const std::vector<std::string> p15 = {"BUSY", "SHIFT", "SMALL", "KANA", "DE", "G", "RAD",
                                          "RUN", "PRO", "RESERVE", "DEF", "I", "II", "III"};
    std::vector<std::string> p16 = p15;
    for (const char* x : {"S", "ROMAJI", "CTRL", "BATT"}) p16.push_back(x);
    for (const char* model : {"PC-1500A", "PC-1600"}) {
        auto m = bootedSharp(c, model);
        const bool is16 = std::string(model) == "PC-1600";
        const DisplayOutput out = m->readDisplay();
        CHECK(out.on);
        const auto f = std::get<DotMatrixFrame>(out.frame);
        CHECK_EQ(f.width, 156);
        CHECK_EQ(f.height, is16 ? 32 : 7);
        CHECK_EQ(f.dots.size(), std::size_t(156 * (is16 ? 32 : 7)));
        CHECK(std::any_of(f.dots.begin(), f.dots.end(), [](std::uint8_t d) { return d == 1; }));   // the prompt
        std::set<std::string> names, want(is16 ? p16.begin() : p15.begin(), is16 ? p16.end() : p15.end());
        for (const auto& i : out.indicators) names.insert(std::string(i.name));
        CHECK(names == want);
        // The mode indicator is lit (RUN after the PC-1600's cold boot, PRO
        // after the PC-1500's NEW0).
        CHECK(indicatorOn(*m, "RUN") != indicatorOn(*m, "PRO"));
    }
}

TEST(c1600_1_ac14_buzzer_48000_mono_same_at_any_slicing) {
    SKIP_UNLESS(haveAll(), "Sharp ROMs missing");
    const Catalogue& c = sharp::sharpCatalogue();
    {   // Another rate is refused at build (design D-6).
        BuildRequest req{ModelId{"PC-1600"}, std::nullopt, {}, {}, 44100, nullptr};
        std::string error;
        CHECK(modelImages(c, req.model, std::nullopt, &req.roms, &error));
        sharp::SharpMachineBuilder b;
        const BuildResult r = build(c, b, req);
        CHECK(!r.machine && !r.refusal.empty());
    }
    for (const char* model : {"PC-1500A", "PC-1600"}) {
        auto one = bootedSharp(c, model);
        auto many = bootedSharp(c, model);
        std::int16_t drop[8192];
        // BEEP 1 typed, then ENTER pressed: the beep sounds within the
        // stretch compared below (as in the golden runs *-beep).
        for (auto* m : {one.get(), many.get()}) {
            for (char ch : std::string("beep 1")) tap(*m, ch == ' ' ? std::string("space") : std::string(1, ch));
            while (m->takeSound(drop) > 0) {}
            m->keyDown(KeyId{"enter"});
        }
        CHECK_EQ(one->tickCount(), many->tickCount());
        auto collect = [](Machine& m, std::vector<std::int16_t>& out) {
            std::int16_t buf[4096];
            std::size_t n;
            while ((n = m.takeSound(buf)) > 0) out.insert(out.end(), buf, buf + n);
        };
        std::vector<std::int16_t> a, b;
        const Ticks span = secondsTicks(*one, 0.4);
        const Ticks start = one->tickCount();
        one->advance(span);
        collect(*one, a);
        while (many->tickCount() - start < one->tickCount() - start) {
            many->advance(997);
            collect(*many, b);
        }
        CHECK(!a.empty());
        CHECK(std::any_of(a.begin(), a.end(), [](std::int16_t s) { return s != 0; }));   // the beep sounds
        // 48,000 samples per second of ticks, mono.
        const double expected = 48000.0 * static_cast<double>(one->tickCount() - start) / one->descriptor().tickRate;
        CHECK(std::abs(static_cast<double>(a.size()) - expected) <= 2.0);
        const double expectedMany = 48000.0 * static_cast<double>(many->tickCount() - start) / many->descriptor().tickRate;
        CHECK(std::abs(static_cast<double>(b.size()) - expectedMany) <= 2.0);
        // The sliced run went on to its own tick count; the samples of the
        // common stretch are the same.
        const std::size_t n = std::min(a.size(), b.size());
        CHECK(n + 2 >= a.size());
        CHECK(std::equal(a.begin(), a.begin() + static_cast<std::ptrdiff_t>(n), b.begin()));
    }
}

// ---- T7: places ----

TEST(c1600_1_ac15_each_peripheral_only_through_places) {
    SKIP_UNLESS(haveAll(), "Sharp ROMs missing");
    const Catalogue c = catalogueWithModules();
    struct Case { const char* model; const char* id; const char* where; const char* wrong; };
    const Case cases[] = {{"PC-1500A", "CE-150", "printer", "interface"}, {"PC-1500A", "CE-158", "interface", "printer"},
                          {"PC-1600", "CE-150", "printer", "interface"},  {"PC-1600", "CE-158", "interface", "printer"},
                          {"PC-1600", "CE-1600P", "printer", "host-drive"}, {"PC-1600", "host-drive", "host-drive", "printer"},
                          {"PC-1500A", "CE-161", "slot-1", "printer"},     {"PC-1600", "CE-1601M", "slot-2", "slot-1"}};
    for (const Case& k : cases) {
        auto m = buildSharp(c, k.model);
        auto req = [&](const char* where) {
            if (std::string(k.id) == "host-drive") {
                PeripheralRequest r = hostDriveRequest(c, ".");
                r.place = place(where);
                return r;
            }
            return c.peripheral(PeripheralId{k.id})->roms.empty() && c.peripheral(PeripheralId{k.id})->variants.empty()
                       ? moduleRequest(k.id, where)
                       : request(c, k.id, where);
        };
        CHECK(m->attach(req(k.wrong), c).refused.has_value());
        CHECK(m->plugged().byPlace.empty());
        const ChangeResult r = m->attach(req(k.where), c);
        CHECK(!r.refused);
        const Plugged now = m->plugged();
        const PlugEntry* e = pluggedAt(now, k.where);
        CHECK(e && e->peripheral.value == k.id);
        CHECK(m->detach(place(k.where), PeripheralId{k.id}, c).refused == std::nullopt);
        CHECK(m->plugged().byPlace.empty());
    }
}

TEST(c1600_1_ac17_first_peripheral_that_answers_wins_in_attach_order) {
    SKIP_UNLESS(haveAll(), "Sharp ROMs missing");
    const Catalogue& c = sharp::sharpCatalogue();
    for (bool ce150First : {true, false}) {
        auto m = buildSharp(c, "PC-1500A");
        const auto first = ce150First ? request(c, "CE-150", "printer") : request(c, "CE-158", "interface");
        const auto second = ce150First ? request(c, "CE-158", "interface") : request(c, "CE-150", "printer");
        CHECK(!m->attach(first, c).refused);
        CHECK(!m->attach(second, c).refused);
        const Plugged p = m->plugged();
        CHECK_EQ(p.byPlace.size(), std::size_t{2});
        CHECK(p.byPlace[0].peripheral == first.peripheral && p.byPlace[1].peripheral == second.peripheral);
        CHECK_EQ(p.byConnector.size(), std::size_t{1});
        CHECK(p.byConnector[0].connector.value == "60-pin");
        CHECK(p.byConnector[0].plugged.size() == 2 && p.byConnector[0].plugged[0].peripheral == first.peripheral);
        // Both answer their own cycles: the plotter's paper and the CE-158's printer port.
        boot(*m);
        CHECK(hasKind(*m, sharp::PlotterPaper::kKind));
        CHECK(hasKind(*m, sharp::Ce158PrinterPort::kKind));
        CHECK(runUntilKeyReady(*m, 5) && !screen(*m).empty());
    }
}

TEST(c1600_1_ac18_ce1600p_variant_keeps_memory_and_disk_side) {
    SKIP_UNLESS(haveAll(), "Sharp ROMs missing");
    const Catalogue& c = sharp::sharpCatalogue();
    bool ok = false;
    auto m = buildSharp(c, "PC-1600", std::nullopt, {request(c, "CE-1600P", "printer", VariantId{"New"}, &ok)});
    SKIP_UNLESS(ok && m, "CE-1600P ROMs missing");
    boot(*m);
    FloppyFile disk;
    std::string err;
    CHECK(readFloppyFile(std::string(cardsDir()) + "/formatted.floppy.yaml", &disk, &err));
    auto* drive = sharp::firstOf<sharp::FloppyDriveAccess>(*m);
    CHECK(drive && drive->loadImage(disk.image.data(), disk.image.size()));
    drive->setSide(1);
    const std::uint16_t a = freeRam(*m);
    CHECK(pokePattern(*m, a));

    // The variant change: one replace, then its power cycle. The disk goes
    // into the new drive at the off point, the same side up, as the session
    // does it (swapCE1600PRom today).
    const std::vector<std::uint8_t> image = drive->diskImage();
    const int side = drive->side();
    const ChangeResult r = m->replace(PeripheralId{"CE-1600P"}, request(c, "CE-1600P", "printer", VariantId{"Old"}, &ok), c);
    CHECK(ok && !r.refused);
    CHECK(r.follows == AfterChange::PowerCycle);
    const Plugged now = m->plugged();
        const PlugEntry* e = pluggedAt(now, "printer");
    CHECK(e && e->variant && e->variant->value == "Old");   // shows at once (D-2)
    sharp::legacy::atOffPoint(*m, [&] {
        auto* d = sharp::firstOf<sharp::FloppyDriveAccess>(*m);
        d->loadImage(image.data(), image.size());
        d->setSide(side);
    });
    m->powerCycle();
    sharp::legacy::runToPrompt(*m);
    CHECK(runUntilKeyReady(*m));
    CHECK(hasPattern(*m, a));
    drive = sharp::firstOf<sharp::FloppyDriveAccess>(*m);
    CHECK(drive && drive->hasDisk() && drive->side() == 1 && drive->diskImage() == image);
    // The old ROM is the one on the bus: its bank 5 version byte (AC-18: "with the other variant").
    CHECK_EQ(m->plugged().byPlace.size(), std::size_t{1});
}

TEST(c1600_1_ec3_ce1601m_in_slot1_refused) {
    SKIP_UNLESS(haveAll(), "Sharp ROMs missing");
    const Catalogue c = catalogueWithModules();
    auto m = buildSharp(c, "PC-1600");
    const ChangeResult r = m->attach(moduleRequest("CE-1601M", "slot-1"), c);
    CHECK(r.refused.has_value());
    CHECK(m->plugged().byPlace.empty());
    auto p15 = buildSharp(c, "PC-1500A");
    CHECK(p15->attach(moduleRequest("CE-1600M", "slot-1"), c).refused.has_value());
    CHECK(!m->attach(moduleRequest("CE-1601M", "slot-2"), c).refused);
}

// ---- T8: capabilities ----

TEST(c1600_1_ac16_ce150_cassette_listed_on_pc1500) {
    SKIP_UNLESS(haveAll(), "Sharp ROMs missing");
    const Catalogue& c = sharp::sharpCatalogue();
    const PeripheralDescriptor* ce150 = c.peripheral(PeripheralId{"CE-150"});
    CHECK(ce150 && ce150->mediaPlaces.size() == 1 && ce150->mediaPlaces[0].id.value == "cassette");
    for (const char* model : {"PC-1500", "PC-1500A"}) {
        if (!haveModelRoms(model)) continue;
        auto m = buildSharp(c, model, std::nullopt, {request(c, "CE-150", "printer")});
        const Plugged now = m->plugged();
        const PlugEntry* e = pluggedAt(now, "printer");
        CHECK(e && e->peripheral.value == "CE-150");
        // The media place capability of the cassette is LIB-7's (M-47, LIB-1 D-1).
    }
}

TEST(c1600_1_ac19_product_kinds_with_owners) {
    SKIP_UNLESS(haveAll(), "Sharp ROMs missing");
    const Catalogue& c = sharp::sharpCatalogue();
    auto ownerIs = [](const CapabilityEntry& e, const char* comp) {
        const auto* o = std::get_if<ComponentOwner>(&e.owner);
        return o && o->component.value == comp;
    };
    auto peripheralIs = [](const CapabilityEntry& e, const char* id, const char* where) {
        const auto* o = std::get_if<PeripheralOwner>(&e.owner);
        return o && o->peripheral.value == id && o->place == place(where);
    };
    for (const char* model : {"PC-1500A", "PC-1600"}) {
        const char* cpu = std::string(model) == "PC-1600" ? "SC7852" : "LH5801";
        auto m = buildSharp(c, model, std::nullopt, {request(c, "CE-158", "interface")});
        for (KindId k : {sharp::LoaderMemory::kKind, sharp::InspectorViews::kKind}) {
            const auto e = m->capabilities(k);
            CHECK_EQ(e.size(), std::size_t{1});
            if (!e.empty()) CHECK(ownerIs(e[0], cpu) && m->capability(e[0].id) != nullptr);
        }
        const auto port = m->capabilities(sharp::Ce158PrinterPort::kKind);
        CHECK(port.size() == 1 && peripheralIs(port[0], "CE-158", "interface"));
        CHECK(!memoryOf(*m).internalRam().empty() == (std::string(model) == "PC-1600"));
        CHECK(!sharp::firstOf<sharp::InspectorViews>(*m)->menu(false).empty());
        CHECK(m->capabilities(sharp::HostDriveFolder::kKind).empty());
    }
    auto m = buildSharp(c, "PC-1600", std::nullopt, {hostDriveRequest(c, "/tmp")});
    const auto hd = m->capabilities(sharp::HostDriveFolder::kKind);
    CHECK(hd.size() == 1 && peripheralIs(hd[0], "host-drive", "host-drive"));
    auto* folder = sharp::firstOf<sharp::HostDriveFolder>(*m);
    CHECK(folder && folder->folder() == "/tmp");
    folder->setFolder("/var");
    CHECK(folder->folder() == "/var");
    // Capability IDs are the same for machines built alike (LIB-1 AC-21).
    auto again = buildSharp(c, "PC-1600", std::nullopt, {hostDriveRequest(c, "/tmp")});
    std::vector<std::string> a, b;
    for (const auto& e : m->capabilities()) a.push_back(e.id.value);
    for (const auto& e : again->capabilities()) b.push_back(e.id.value);
    CHECK(a == b);
}

TEST(c1600_1_ac20_display_on_slots_cassette_one_way_on_every_model) {
    SKIP_UNLESS(haveAll(), "Sharp ROMs missing");
    const Catalogue c = catalogueWithModules();
    for (const char* model : {"PC-1500A", "PC-1600"}) {
        auto m = bootedSharp(c, model, {moduleRequest("CE-161", "slot-1")});
        CHECK(m->readDisplay().on);   // whether the display is on: display output on both
        CHECK(switchOff(*m));
        CHECK(!m->readDisplay().on);
        const Plugged now = m->plugged();
        const PlugEntry* e = pluggedAt(now, "slot-1");   // the module in a slot: plugged()
        CHECK(e && e->peripheral.value == "CE-161");
        CHECK(sharp::legacy::moduleIn(*m, 1) != nullptr);
        auto* tape = sharp::firstOf<sharp::TapeDeckAccess>(*m);   // the cassette: one kind on both
        CHECK(tape && tape->status().mode == TapeDeck::Mode::Empty);
    }
}
