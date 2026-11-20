// A TI machine has one debug target,
// TMC0501, with the registers of the processor frame, the spaces rom, ram,
// scom and program, a history and decoding, and no watches or stepping over
// and out (core/TIDebugging.hpp).

#include "TestHarness.hpp"
#include "TestRoms.hpp"
#include "machine/Conformance.hpp"

using namespace cu59;
using namespace calcu::machine;

namespace {
const ModelId kModels[] = {ModelId{kTI59}, ModelId{kTI58}, ModelId{kTI58C}};

std::vector<std::string> names(const TargetInfo& t) {
    std::vector<std::string> out;
    for (const auto& r : t.registers) out.push_back(r.name);
    return out;
}
} // namespace

TEST(one_target_tmc0501_on_every_model) {
    for (ModelId model : kModels) {
        auto m = testroms::makeMachine(model);
        Debugging* d = m->debugging();
        CHECK(d != nullptr);
        if (!d) continue;
        CHECK_EQ(d->targetCount(), std::size_t{1});
        CHECK_EQ(d->target(0).info().name.value, std::string("TMC0501"));
        CHECK_EQ(d->current(), std::size_t{0});
    }
}

TEST(registers_of_the_processor_frame) {
    auto m = testroms::makeMachine(ModelId{kTI59});
    testroms::runToScanLoop(*m, ModelId{kTI59});
    const DebugTarget& t = m->debugging()->target(0);
    CHECK(names(t.info()) == (std::vector<std::string>{"A", "B", "C", "D", "E", "KR", "SR", "fA", "fB", "flags", "R5",
                                                       "digit", "EXT", "PREG", "pc"}));
    for (const auto& r : t.info().registers) CHECK(!r.writable);
    CHECK_EQ(t.info().registers[0].bits, 64);
    const CpuFrame f = testroms::dbg(*m).snapshotCPU();
    std::uint64_t a = 0;
    for (int i = 15; i >= 0; --i) a = (a << 4) | f.A[i];
    CHECK_EQ(t.readRegister(0), a);
    CHECK_EQ(t.readRegister(14), std::uint64_t{f.pc});
    CHECK_EQ(t.nextAddress(), std::uint64_t{testroms::dbg(*m).pc()});
}

TEST(spaces_rom_ram_scom_program) {
    for (ModelId model : kModels) {
        auto m = testroms::makeMachine(model);
        const TargetInfo& i = m->debugging()->target(0).info();
        CHECK_EQ(i.spaces.size(), std::size_t{4});
        if (i.spaces.size() != 4) continue;
        CHECK(i.spaces[0].name == "rom" && i.spaces[0].cellBits == 13 && i.spaces[0].size == 0x2000);
        CHECK(i.spaces[1].name == "ram" && i.spaces[1].cellBits == 64);
        CHECK_EQ(i.spaces[1].size, std::uint64_t(testroms::mem(*m).ramRegCount()));
        CHECK(i.spaces[2].name == "scom" && i.spaces[2].size == 16);
        CHECK(i.spaces[3].name == "program" && i.spaces[3].cellBits == 8);
        CHECK_EQ(i.spaces[3].size, i.spaces[1].size * 8);
        CHECK_EQ(i.codeSpace, std::size_t{0});
    }
}

TEST(ram_and_program_written_rom_scom_refused) {
    auto m = testroms::makeMachine(ModelId{kTI59});
    testroms::runToScanLoop(*m, ModelId{kTI59});
    DebugTarget& t = m->debugging()->target(0);
    CHECK(t.poke(1, 7, 0x0123456789ABCDEFull, {}));
    CHECK_EQ(t.peek(1, 7, {}).value, 0x0123456789ABCDEFull);
    CHECK(t.poke(3, 5, 42, {}));
    CHECK_EQ(t.peek(3, 5, {}).value, std::uint64_t{42});
    CHECK_EQ(int(testroms::mem(*m).readProgramStep(5)), 42);
    CHECK(!t.poke(3, 5, 100, {}));   // a program step holds 00 to 99
    const auto rom = t.peek(0, 0x100, {}).value;
    CHECK(!t.poke(0, 0x100, rom ^ 1, {}));
    CHECK_EQ(t.peek(0, 0x100, {}).value, rom);
    CHECK(!t.poke(2, 3, 1, {}));
    CHECK(!t.writeRegister(0, 1));
    CHECK(t.peek(1, 1000, {}).kind == CellRead::Unreadable);
}

TEST(history_and_decoding_no_watches_no_over_out) {
    auto m = testroms::makeMachine(ModelId{kTI59});
    testroms::runToScanLoop(*m, ModelId{kTI59});
    DebugTarget& t = m->debugging()->target(0);
    const TargetOffers& o = t.info().offers;
    CHECK(!o.watches && !o.stepOverOut && o.decode && o.history > 0 && o.qualifier.empty());
    CHECK(!t.setWatches({WatchRange{1, 0, 1}}));
    const auto dec = t.decode(t.nextAddress());
    CHECK(dec.has_value() && !dec->text.empty());
    // A history while the run control's checks are set (the frame ring).
    struct Count final : InstructionObserver {
        int n = 0;
        bool stopAfter(const InstructionEvent&) override { return ++n == 100; }
    } count;
    m->debugging()->setChecks(Checks{false, false, &count});
    m->advance(1'000'000);
    m->debugging()->setChecks({});
    CHECK_EQ(count.n, 100);
    const auto h = t.history(10);
    CHECK_EQ(h.size(), std::size_t{10});
    if (!h.empty()) {
        CHECK_EQ(h[0].registers.size(), t.info().registers.size());
        CHECK(h[0].text.has_value());
    }
    CHECK(t.retired() > 0);
}

TEST(conformance_on_every_model) {
    for (ModelId model : kModels) {
        auto m = testroms::makeMachine(model);
        testroms::runToScanLoop(*m, model);
        const auto problems = checkDebugging(*m);
        for (const auto& p : problems) std::printf("  %s\n", p.c_str());
        CHECK(problems.empty());
    }
}
