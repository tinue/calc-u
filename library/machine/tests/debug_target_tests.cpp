// The debug-target interface on the test
// processor (TestCpu.hpp) behind test-seg's `test-cpu` and test-dot's
// `main` and `inner`; the conformance check.
#include "TestHarness.hpp"
#include "TestMachines.hpp"
#include "machine/Conformance.hpp"

using namespace calcu::machine;
using namespace calcu::machine::test;
using namespace calcu::machine::test::cpu;

namespace {

std::unique_ptr<TestMachine> withCpu(const ModelId& model, TestProgram p = {}) {
    TestBuilder builder;
    const Catalogue& cat = testCatalogue();
    BuildRequest req = requestFor(cat, model);
    req.content = std::make_shared<TestProgram>(std::move(p));
    BuildResult r = build(cat, builder, req);
    return std::unique_ptr<TestMachine>(static_cast<TestMachine*>(r.machine.release()));
}

// ld a, #0x42; st a, [0x30]; ld a, [0x31]; out 0x00, a; nop; jmp 0
TestProgram simple() {
    return TestProgram({kLdA, 0x42, kSt, 0x30, kLdMem, 0x31, kOut, 0x00, kNop, kJmp, 0x00}, {0, 9});
}

std::size_t regIndex(const DebugTarget& t, const std::string& name) {
    const auto& r = t.info().registers;
    for (std::size_t i = 0; i < r.size(); ++i)
        if (r[i].name == name) return i;
    return r.size();
}

} // namespace

TEST(one_target_per_processor_named_as_descriptor) {
    auto seg = withCpu(kSeg);
    auto dot = withCpu(kDot);
    CHECK(seg && dot);
    if (!seg || !dot) return;
    CHECK(buildTestMachine(kSeg)->debugging() == nullptr);   // without the option: none
    Debugging* d = seg->debugging();
    CHECK(d && d->targetCount() == 1);
    CHECK(d->target(0).info().name == DebugTargetId{"test-cpu"});
    CHECK(seg->debugTarget(DebugTargetId{"test-cpu"}) == &d->target(0));
    CHECK(seg->debugTarget(DebugTargetId{"nothing"}) == nullptr);
    Debugging* e = dot->debugging();
    CHECK(e && e->targetCount() == 2);
    CHECK(e->target(0).info().name == DebugTargetId{"main"});
    CHECK(e->target(1).info().name == DebugTargetId{"inner"});
    CHECK(dot->debugTargets() == (std::vector<DebugTargetId>{DebugTargetId{"main"}, DebugTargetId{"inner"}}));
}

TEST(target_states_registers_spaces_code_space_offers_qualifier_now) {
    auto m = withCpu(kSeg, simple());
    const TargetInfo& info = m->debugging()->target(0).info();
    CHECK_EQ(info.registers.size(), std::size_t(5));
    CHECK(info.registers[0].name == "a" && info.registers[0].bits == 8);
    CHECK(info.registers[2].name == "pc" && info.registers[2].bits == 16 && !info.registers[2].writable);
    CHECK(info.registers[4].name == "z" && info.registers[4].bits == 1);
    CHECK_EQ(info.spaces.size(), std::size_t(3));
    CHECK(info.spaces[0].name == "memory" && info.spaces[0].cellBits == 8 && info.spaces[0].size == 256);
    CHECK(info.spaces[1].name == "io" && info.spaces[1].size == 16);
    CHECK(info.spaces[2].name == "io-written" && info.spaces[2].writtenView);
    CHECK_EQ(info.codeSpace, std::size_t(0));
    CHECK(info.offers.watches && info.offers.stepOverOut && info.offers.decode);
    CHECK_EQ(info.offers.history, std::size_t(8));
    CHECK(info.offers.qualifier.size() == 1 && info.offers.qualifier[0].name == "bank" &&
          info.offers.qualifier[0].values.size() == 4);
    DebugTarget& t = m->debugging()->target(0);
    CHECK(t.qualifierNow() == std::vector<std::string>{"0"});   // the value it has now
    m->advance(30);   // out 0x00, a with a = 9: bank 9 & 3 = 1
    CHECK(t.qualifierNow() == std::vector<std::string>{"1"});
}

TEST(registers_read_written_where_writable) {
    auto m = withCpu(kSeg, simple());
    DebugTarget& t = m->debugging()->target(0);
    CHECK(t.writeRegister(regIndex(t, "a"), 0x7f));
    CHECK_EQ(t.readRegister(regIndex(t, "a")), std::uint64_t(0x7f));
    CHECK(t.writeRegister(regIndex(t, "sp"), 0x90));
    CHECK_EQ(t.readRegister(regIndex(t, "sp")), std::uint64_t(0x90));
    CHECK(!t.writeRegister(regIndex(t, "pc"), 4));   // read-only: refused
    CHECK_EQ(t.readRegister(regIndex(t, "pc")), std::uint64_t(0));
    CHECK(!t.writeRegister(regIndex(t, "a"), 0x100));   // does not fit: refused, nothing changed
    CHECK_EQ(t.readRegister(regIndex(t, "a")), std::uint64_t(0x7f));
}

TEST(peek_no_effect_unreadable_device_cell) {
    auto m = withCpu(kSeg, simple());
    DebugTarget& t = m->debugging()->target(0);
    const Ticks ticks = m->tickCount();
    const CellRead c = t.peek(kMemory, 0x31, {});
    CHECK(c.kind == CellRead::Value && c.value == 9);
    for (std::uint64_t p = 0; p < 16; ++p) CHECK(t.peek(kIo, p, {}).kind == CellRead::Unreadable);
    CHECK_EQ(m->cpu()->deviceReads(0), std::uint8_t(0));   // the device was not read
    CHECK_EQ(m->tickCount(), ticks);
}

TEST(refused_poke_changes_nothing) {
    auto m = withCpu(kSeg, simple());
    DebugTarget& t = m->debugging()->target(0);
    CHECK(t.poke(kMemory, 0x40, 0x11, {}));
    CHECK_EQ(t.peek(kMemory, 0x40, {}).value, std::uint64_t(0x11));
    const std::uint64_t rom = t.peek(kMemory, 0xf3, {}).value;
    CHECK(!t.poke(kMemory, 0xf3, 0x00, {}));   // ROM
    CHECK_EQ(t.peek(kMemory, 0xf3, {}).value, rom);
    CHECK(!t.poke(kMemory, 0x40, 0x100, {}));   // too wide
    CHECK_EQ(t.peek(kMemory, 0x40, {}).value, std::uint64_t(0x11));
    CHECK(!t.poke(kIoWritten, 0x03, 1, {}));   // the written view is read-only
    CHECK(t.peek(kIoWritten, 0x03, {}).kind == CellRead::NoneWritten);
}

TEST(next_address_and_pre_execution_state) {
    auto m = withCpu(kSeg, simple());
    DebugTarget& t = m->debugging()->target(0);
    CHECK_EQ(t.nextAddress(), std::uint64_t(0));
    m->advance(1);   // one instruction: ld a, #0x42
    CHECK_EQ(t.nextAddress(), std::uint64_t(2));
    CHECK_EQ(t.readRegister(regIndex(t, "a")), std::uint64_t(0x42));
    // Between two instructions the cell st a, [0x30] writes still holds its old value.
    CHECK_EQ(t.peek(kMemory, 0x30, {}).value, std::uint64_t(0));
    CHECK_EQ(t.retired(), std::uint64_t(1));
}

TEST(history_newest_first_with_registers_before) {
    auto m = withCpu(kSeg, simple());
    DebugTarget& t = m->debugging()->target(0);
    for (int i = 0; i < 3; ++i) m->advance(1);
    const auto h = t.history(10);
    CHECK_EQ(h.size(), std::size_t(3));
    CHECK_EQ(h[0].address, std::uint64_t(4));   // ld a, [0x31], the newest
    CHECK_EQ(h[2].address, std::uint64_t(0));
    CHECK_EQ(h[2].registers[regIndex(t, "a")], std::uint64_t(0));       // before ld a, #0x42
    CHECK_EQ(h[1].registers[regIndex(t, "a")], std::uint64_t(0x42));    // before st
    CHECK(h[0].text && *h[0].text == "ld a, [0x31]");
    for (int i = 0; i < 20; ++i) m->advance(1);
    CHECK_EQ(t.history(100).size(), std::size_t(8));   // how many it keeps is the target's
}

TEST(conformance_check_debugging) {
    for (const ModelId& model : {kSeg, kDot}) {
        auto m = withCpu(model, simple());
        const auto problems = checkDebugging(*m);
        for (const auto& p : problems) std::fprintf(stderr, "  %s\n", p.c_str());
        CHECK(problems.empty());
    }
    auto bare = buildTestMachine(kSeg);   // listed targets, none offered
    CHECK(!checkDebugging(*bare).empty());
}

TEST(written_view_last_value_or_none_no_effect) {
    auto m = withCpu(kSeg, simple());
    DebugTarget& t = m->debugging()->target(0);
    CHECK(t.peek(kIoWritten, 0, {}).kind == CellRead::NoneWritten);   // nothing written since the build
    m->advance(30);   // out 0x00, a with a = 9
    const CellRead c = t.peek(kIoWritten, 0, {});
    CHECK(c.kind == CellRead::Value && c.value == 9);
    CHECK(t.peek(kIo, 0, {}).kind == CellRead::Unreadable);   // the port itself cannot be read back
    CHECK(t.poke(kIo, 5, 0x33, {}));
    CHECK_EQ(t.peek(kIoWritten, 5, {}).value, std::uint64_t(0x33));
    CHECK_EQ(t.peek(kIoWritten, 5, {}).value, std::uint64_t(0x33));   // reading the view has no effect
}

TEST(every_bank_and_vbank_by_name_no_switch) {
    auto m = withCpu(kSeg, simple());
    DebugTarget& t = m->debugging()->target(0);
    const SpaceInfo& memory = t.info().spaces[kMemory];
    CHECK(memory.banked.size() == 1 && memory.banked[0].banks == 4 && memory.banked[0].vbanks == 2);
    CHECK_EQ(t.peek(kMemory, 0x80, {}).value, std::uint64_t(0x10));   // bank 0, switched in
    CHECK_EQ(t.peek(kMemory, 0x80, BankSelect{2, std::nullopt}).value, std::uint64_t(0x30));
    CHECK_EQ(t.peek(kMemory, 0x80, BankSelect{3, 0}).value, std::uint64_t(0x40));
    CHECK_EQ(t.peek(kMemory, 0x80, BankSelect{3, 1}).value, std::uint64_t(0x50));
    CHECK(t.poke(kMemory, 0x81, 0x77, BankSelect{3, 1}));
    CHECK_EQ(t.peek(kMemory, 0x81, BankSelect{3, 1}).value, std::uint64_t(0x77));
    CHECK_EQ(t.peek(kMemory, 0x81, BankSelect{3, 0}).value, std::uint64_t(0x40));
    CHECK_EQ(m->cpu()->bank(), std::uint32_t(0));   // no bank switched
    CHECK_EQ(t.peek(kMemory, 0x81, {}).value, std::uint64_t(0x10));
}

TEST(range_across_banks_refused) {
    // The library refuses a range that leaves its window before the target is
    // asked (the session's peek, DebuggerPart); the target itself refuses a
    // bank outside a window.
    auto m = withCpu(kSeg, simple());
    DebugTarget& t = m->debugging()->target(0);
    CHECK(t.peek(kMemory, 0x7f, BankSelect{1, std::nullopt}).kind == CellRead::Unreadable);
    CHECK(!t.poke(kMemory, 0xc0, 1, BankSelect{1, std::nullopt}));
    CHECK(t.peek(kMemory, 0x80, BankSelect{1, 1}).kind == CellRead::Unreadable);   // bank 1 has no vertical bank 1
}

TEST(peek_poke_outside_space_refused) {
    auto m = withCpu(kSeg, simple());
    DebugTarget& t = m->debugging()->target(0);
    CHECK(t.peek(kMemory, 256, {}).kind == CellRead::Unreadable);
    CHECK(!t.poke(kMemory, 256, 1, {}));
    CHECK(!t.poke(kIo, 16, 1, {}));
}
