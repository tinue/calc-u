// The debugger's machine layer -- CPU breakpoints
// with skip-once resume, memory watches (data accesses only, never
// fetches) and the Sharp debug targets (core/Debug/SharpDebugging):
// the cases on machine::Debugging (DebugTargetTestSupport), the debug
// targets and the SC7852's I/O space. The expressions, conditions and
// log messages are the DAP's (dap_layer_tests.cpp).
//
// Build & run: see tools/run_tests.sh

#include <cstdint>
#include <cstdio>
#include <map>
#include <string>
#include <vector>

#include "../core/CPU/LH5801/LH5801.hpp"
#include "../core/CPU/SC7852/SC7852.hpp"
#include "../core/Debug/CpuRegisters.hpp"
#include "../core/Debug/DebugExpression.hpp"
#include "../core/Debug/SharpDebugging.hpp"
#include "../core/PC1600/PC1600Machine.hpp"
#include "../core/PC1500/PC1500Machine.hpp"
#include "DebugTargetTestSupport.hpp"
#include "SharpTestSupport.hpp"
#include "TestRoms.hpp"
#include "machine/Conformance.hpp"
#include "TestHarness.hpp"

namespace {

class LhBus : public LH5801Bus {
public:
    uint8_t mem[65536]{};
    uint8_t me1[65536]{};
    uint8_t readME0(uint16_t a) override { return mem[a]; }
    void    writeME0(uint16_t a, uint8_t v) override { mem[a] = v; }
    uint8_t readME1(uint16_t a) override { return me1[a]; }
    void    writeME1(uint16_t a, uint8_t v) override { me1[a] = v; }
};

class ZBus : public SC7852Bus {
public:
    uint8_t mem[65536]{};
    uint8_t readMem(uint16_t a) override { return mem[a]; }
    void    writeMem(uint16_t a, uint8_t v) override { mem[a] = v; }
    bool intLine = false;
    bool interruptLevel() const override { return intLine; }
};

// ── CPU hooks ─────────────────────────────────────────────────────────────

TEST(z80_breakpoint_and_skip_once) {
    ZBus bus;
    const uint8_t code[] = {0x00, 0x00, 0x18, 0xFC}; // nop; nop; jr 0x0000
    for (size_t i = 0; i < sizeof code; i++) bus.mem[i] = code[i];
    SC7852 cpu(bus);
    cpu.reset();
    cpu.addBreakpoint(0x0001);
    cpu.step();                 // no check until enabled
    cpu.step();
    CHECK(cpu.pc() == 0x0002);
    cpu.setBreakpointsEnabled(true);
    cpu.step();                 // jr 0x0000
    cpu.step();                 // nop at 0
    CHECK(cpu.step() == 0 && cpu.pc() == 0x0001);
    CHECK(cpu.consumeBreakpointHit());
    CHECK(cpu.step() == 0);     // still parked
    CHECK(cpu.consumeBreakpointHit());
    cpu.resumePastBreakpoint();
    CHECK(cpu.step() > 0 && cpu.pc() == 0x0002);
    cpu.step(); cpu.step();     // around the loop again
    CHECK(cpu.step() == 0 && cpu.consumeBreakpointHit()); // the skip is once only
}

TEST(z80_breakpoint_not_between_prefix_and_opcode) {
    ZBus bus;
    const uint8_t code[] = {0xDD, 0xFD, 0x21, 0x00, 0x00}; // DD carried, then FD 21
    for (size_t i = 0; i < sizeof code; i++) bus.mem[i] = code[i];
    SC7852 cpu(bus);
    cpu.reset();
    cpu.setBreakpointsEnabled(true);
    cpu.addBreakpoint(0x0001); // the carried FD's address
    cpu.step();                // DD: carries FD into the next step
    CHECK(cpu.step() > 0);     // mid-prefix: no breakpoint check
    CHECK(cpu.iy() == 0x0000 && cpu.pc() == 0x0005);
}

TEST(z80_skip_is_tied_to_its_address) {
    // Parked on 0001 with an interrupt pending: the continue takes the
    // interrupt first. The handler's breakpoint still stops, and the skip
    // is still there for 0001 when the handler returns.
    ZBus bus;
    bus.mem[0x0000] = 0x00; bus.mem[0x0001] = 0x00; bus.mem[0x0002] = 0x00; // nop; nop; nop
    bus.mem[0x0038] = 0xC9;                                                  // ret
    SC7852 cpu(bus);
    cpu.reset();                 // IM 0, run as IM 1 (RST 38H)
    cpu.setBreakpointsEnabled(true);
    cpu.addBreakpoint(0x0001);
    cpu.addBreakpoint(0x0038);
    cpu.step();
    CHECK(cpu.step() == 0 && cpu.consumeBreakpointHit() && cpu.pc() == 0x0001);
    cpu.setIFF1(true);
    bus.intLine = true;
    cpu.resumePastBreakpoint();
    CHECK(cpu.step() > 0 && cpu.pc() == 0x0038); // interrupt entry
    bus.intLine = false;
    CHECK(cpu.step() == 0 && cpu.consumeBreakpointHit()); // not eaten by the skip
    cpu.removeBreakpoint(0x0038);
    CHECK(cpu.step() > 0 && cpu.pc() == 0x0001); // ret
    CHECK(cpu.step() > 0 && cpu.pc() == 0x0002); // the skip, spent where it was meant

    cpu.clearBreakpointSkip();   // and a cleared one is gone
    cpu.addBreakpoint(0x0002);
    CHECK(cpu.step() == 0 && cpu.consumeBreakpointHit());
}

TEST(z80_breakpoint_keeps_ei_shadow) {
    // ei; nop (breakpoint) with INT pending: parking on the nop must not
    // drop the EI delay, so the nop still runs before the interrupt.
    ZBus bus;
    bus.mem[0x0000] = 0xFB; bus.mem[0x0001] = 0x00; bus.mem[0x0002] = 0x00; // ei; nop; nop
    SC7852 cpu(bus);
    cpu.reset();
    bus.intLine = true;
    cpu.setBreakpointsEnabled(true);
    cpu.addBreakpoint(0x0001);
    CHECK(cpu.step() > 0 && cpu.iff1());          // ei
    CHECK(cpu.step() == 0 && cpu.consumeBreakpointHit() && cpu.pc() == 0x0001);
    cpu.resumePastBreakpoint();
    CHECK(cpu.step() > 0 && cpu.pc() == 0x0002);  // the nop, not the interrupt
    CHECK(cpu.step() > 0 && cpu.pc() == 0x0038);  // now the interrupt
}

TEST(lh5801_skip_once) {
    LhBus bus;
    bus.mem[0xFFFE] = 0x40; bus.mem[0xFFFF] = 0x00;
    bus.mem[0x4000] = 0x38; bus.mem[0x4001] = 0x9E; bus.mem[0x4002] = 0x03; // nop; bch 0x4000
    LH5801 cpu(bus);
    cpu.reset();
    cpu.setBreakpointsEnabled(true);
    cpu.addBreakpoint(0x4000);
    CHECK(cpu.step() == 0 && cpu.consumeBreakpointHit());
    cpu.resumePastBreakpoint();
    CHECK(cpu.step() > 0 && cpu.pc() == 0x4001);
    cpu.step();
    CHECK(cpu.pc() == 0x4000 && cpu.step() == 0);
}

TEST(breakpoint_set_add_remove_match) {
    BreakpointSet set;
    set.add(0x0000);
    set.add(0xFFFF);
    set.add(0x0040);
    CHECK(set.contains(0x0000) && set.contains(0xFFFF) && set.contains(0x0040));
    CHECK(!set.contains(0x0001) && !set.contains(0xFFFE) && !set.contains(0x003F) && !set.contains(0x0041));
    CHECK(!set.consumeHit() && !set.check(0x1234) && !set.consumeHit());
    CHECK(set.check(0xFFFF) && set.consumeHit() && !set.consumeHit());
    set.remove(0xFFFF);
    CHECK(!set.contains(0xFFFF) && set.contains(0x0000));
    set.clear();
    CHECK(!set.contains(0x0000) && !set.contains(0x0040));
}

TEST(trace_flags_leave_breakpoints_alone) {
    // Trace flags alone never check breakpoints.
    LhBus bus;
    bus.mem[0xFFFE] = 0x40; bus.mem[0xFFFF] = 0x00;
    bus.mem[0x4000] = 0x38; bus.mem[0x4001] = 0x9E; bus.mem[0x4002] = 0x03; // nop; bch 0x4000
    LH5801 cpu(bus);
    cpu.reset();
    cpu.addBreakpoint(0x4000);
    cpu.setTraceFlags(TRACE_FULL);
    CHECK(cpu.step() > 0);
    // A trace capture starting and ending leaves the debugger's enable as it was.
    PC1500Machine machine;
    machine.cpu().setBreakpointsEnabled(true);
    CHECK(machine.beginTraceRecords(TRACE_FULL));
    CHECK(machine.cpu().breakpointsEnabled());
    machine.endTraceRecord();
    machine.endTraceRecords();
    CHECK(machine.cpu().breakpointsEnabled() && machine.cpu().traceFlags() == TRACE_NONE);
}

TEST(reset_clears_breakpoint_state) {
    LhBus bus;
    bus.mem[0xFFFE] = 0x40; bus.mem[0xFFFF] = 0x00;
    bus.mem[0x4000] = 0x38;
    LH5801 cpu(bus);
    cpu.reset();
    cpu.setBreakpointsEnabled(true);
    cpu.addBreakpoint(0x4000);
    CHECK(cpu.step() == 0);            // hit latched, not consumed
    cpu.resumePastBreakpoint();        // and a pending skip
    cpu.reset();
    CHECK(!cpu.consumeBreakpointHit());
    CHECK(cpu.step() == 0 && cpu.consumeBreakpointHit()); // the skip didn't survive

    ZBus zbus;
    SC7852 z(zbus);
    z.reset();
    z.setBreakpointsEnabled(true);
    z.addBreakpoint(0x0000);
    CHECK(z.step() == 0);
    z.resumePastBreakpoint();
    z.reset();
    CHECK(!z.consumeBreakpointHit());
    CHECK(z.step() == 0 && z.consumeBreakpointHit());
}

TEST(lh5801_watches_skip_fetches) {
    LhBus bus;
    bus.mem[0xFFFE] = 0x40; bus.mem[0xFFFF] = 0x00;
    const uint8_t code[] = {
        0xB5, 0x5A,             // 4000 ldi a,0x5A
        0xA5, 0x40, 0x00,       // 4002 lda (0x4000)       -- a data read of code
        0xAE, 0x78, 0x10,       // 4005 sta (0x7810)
        0xFD, 0xAE, 0x78, 0x10, // 4008 sta #(0x7810)
    };
    for (size_t i = 0; i < sizeof code; i++) bus.mem[0x4000 + i] = code[i];
    LH5801 cpu(bus);
    cpu.reset();
    WatchSet w;
    w.add({0x4000, 0x4001, 0, true, false}); // read watch on the first instruction's bytes
    w.add({0x7810, 0x7810, 1, false, true}); // ME1 write watch
    cpu.setWatches(&w);
    cpu.step();                       // fetching 4000/4001 is not a data read
    CHECK(!w.hitPending());
    cpu.step();                       // lda (0x4000) is
    CHECK(w.hitPending());
    WatchHit h = w.consumeHit();
    CHECK(h.addr == 0x4000 && !h.write && h.value == 0xB5 && h.space == 0);
    cpu.step();                       // ME0 store: the watch is on ME1
    CHECK(!w.hitPending());
    cpu.step();
    CHECK(w.hitPending());
    h = w.consumeHit();
    CHECK(h.addr == 0x7810 && h.write && h.value == 0xB5 && h.space == 1);
    CHECK(bus.me1[0x7810] == 0xB5); // the instruction completed
    cpu.setWatches(nullptr);
}

TEST(z80_watches) {
    ZBus bus;
    const uint8_t code[] = {0x3E, 0x77, 0x32, 0x00, 0x80, 0xC5}; // ld a,0x77; ld (0x8000),a; push bc
    for (size_t i = 0; i < sizeof code; i++) bus.mem[i] = code[i];
    SC7852 cpu(bus);
    cpu.reset();
    cpu.setSP(0x9000);
    WatchSet w;
    w.add({0x8000, 0x8000, 0, false, true});
    w.add({0x8FFE, 0x8FFF, 0, false, true}); // the stack push
    cpu.setWatches(&w);
    cpu.step();
    CHECK(!w.hitPending());
    cpu.step();
    CHECK(w.hitPending() && w.consumeHit().value == 0x77);
    cpu.step();
    CHECK(w.hitPending() && w.consumeHit().addr == 0x8FFF);
    cpu.setWatches(nullptr);
}

// ── Machine targets ───────────────────────────────────────────────────────

TEST(pc1500_target) {
    PC1500Machine machine;
    if (!machine.loadROMFile("roms/PC-1500_A04.ROM")) {
        std::printf("  SKIP test_pc1500_target: roms/PC-1500_A04.ROM missing\n");
        return;
    }
    machine.reset();
    debugtest::Target target(machine);
    CHECK(target.threadCount() == 1 && target.busOwner() == 1);
    CHECK(target.pc(1) == 0xE000);
    CHECK(target.t(1).decodeAt(0xE000).text == "rie");

    // A breakpoint on the boot's input-buffer clear loop (D0B0).
    target.setBreakpoints(1, {0xD0B0});
    debugtest::Stop s = target.runFrom(2000000);
    CHECK(s.kind == debugtest::Stop::Breakpoint && s.thread == 1);
    CHECK(target.pc(1) == 0xD0B0);
    CHECK(target.t(1).historySize() >= 20);
    CHECK(target.t(1).historyEntry(0).len > 0);
    // Resuming leaves the breakpoint; it's a loop, so it comes back.
    s = target.runFrom(2000000);
    CHECK(s.kind == debugtest::Stop::Breakpoint && target.pc(1) == 0xD0B0);
    target.setBreakpoints(1, {});
    CHECK(!target.breakpointsActive());
    CHECK(!machine.cpu().breakpointsEnabled());

    // Step one instruction.
    const uint32_t before = target.retired(1);
    s = target.stepInstruction(1, 100);
    CHECK(s.kind == debugtest::Stop::None && target.retired(1) == before + 1);

    // Registers and expressions.
    CHECK(target.writeRegister(1, "a", 0x42));
    uint32_t v = 0;
    CHECK(target.readRegister(1, "a", &v) && v == 0x42);
    CHECK(target.writeRegister(1, "xh", 0x12) && target.readRegister(1, "x", &v) && (v >> 8) == 0x12);
    CHECK(target.writeRegister(1, "cf", 1) && target.readRegister(1, "t", &v) && (v & 1));
    debug::ExpressionResult r = debug::evaluate("a == 0x42 && cf", target.t(1).expressionContext());
    CHECK(r.ok && r.value == 1);

    // Memory: RAM round trip; ME1's I/O chip reads as its latches; the ROM refuses.
    CHECK(target.poke(1, debug::kSpaceMain, 0x7A00, 0x99));
    uint8_t b = 0;
    CHECK(target.peek(1, debug::kSpaceMain, 0x7A00, &b) && b == 0x99);
    CHECK(!target.poke(1, debug::kSpaceMain, 0xC000, 0x00));
    CHECK(target.poke(1, debug::kSpaceME1, 0x7A00, 0x00));   // ME1: the CPU's own store

    // A write watch on the RAM the ROM keeps updating while idle stops
    // right after the writing instruction.
    target.setWatches(1, {{0x7600, 0x7BFF, 0, false, true}});
    s = target.runFrom(20000000);
    CHECK(s.kind == debugtest::Stop::Watch && s.thread == 1 && s.hit.write);
    CHECK(s.hit.addr >= 0x7600 && s.hit.addr <= 0x7BFF);
    target.setWatches(1, {});
}

TEST(pc1600_target) {
    PC1600Machine machine;
    if (!bootPC1600(machine)) {
        std::printf("  SKIP test_pc1600_target: PC-1600 ROMs missing\n");
        return;
    }
    debugtest::Target target(machine);
    CHECK(target.threadCount() == 2);
    CHECK(target.busOwner() == 1);
    CHECK(target.t(1).historySize() > 0);
    CHECK(target.t(1).bankAt(target.pc(1)) >= 0);

    // The BASIC command loop spins; a breakpoint on the current PC stops
    // the next pass, and a resume comes back to it.
    const uint16_t pc = target.pc(1);
    target.setBreakpoints(1, {pc});
    debugtest::Stop s = target.runFrom(2000000);
    CHECK(s.kind == debugtest::Stop::Breakpoint && s.thread == 1 && target.pc(1) == pc);
    s = target.runFrom(2000000);
    CHECK(s.kind == debugtest::Stop::Breakpoint && target.pc(1) == pc);
    target.setBreakpoints(1, {});
    CHECK(!machine.sc7852().breakpointsEnabled());

    const uint32_t before = target.retired(1);
    s = target.stepInstruction(1, 100);
    CHECK(s.kind == debugtest::Stop::None && target.retired(1) == before + 1);

    uint32_t v = 0;
    CHECK(target.writeRegister(1, "bc'", 0xBEEF) && target.readRegister(1, "bc2", &v) && v == 0xBEEF);
    CHECK(target.writeRegister(1, "h", 0x12) && target.readRegister(1, "hl", &v) && (v >> 8) == 0x12);
    CHECK(target.readRegister(1, "zf", &v));

    // Z-80 RAM (page D, internal) round trip through the host path.
    uint8_t b = 0;
    CHECK(target.poke(1, debug::kSpaceMain, 0xC200, 0x5A) && target.peek(1, debug::kSpaceMain, 0xC200, &b) && b == 0x5A);
    // The LH5803 sees Z-80 8000-FFFF at its 0000-7FFF.
    CHECK(target.peek(2, debug::kSpaceMain, 0x4200, &b) && b == 0x5A);
    // Its UART block on ME1 can't be peeked without side effects.
    CHECK(!target.peek(2, debug::kSpaceME1, 0xA020, &b));
    CHECK(target.t(1).decodeAt(0x0000).len >= 1);
}

TEST(pc1600_lh5803_thread) {
    // Z-80: OUT (38H),A ; HALT hands the bus to the LH5803, whose ROM loops
    // at C000: sta (0x1000) (Z-80 0x9000) ; nop ; bch C000.
    PC1600Machine m;
    std::vector<uint8_t> lower(PC1600Memory::kBankSize, 0x00), upper(PC1600Memory::kBankSize, 0x00);
    lower[0] = 0xD3; lower[1] = 0x38; lower[2] = 0x76;
    CHECK(m.loadBank0(lower.data(), lower.size(), upper.data(), upper.size()));
    std::vector<uint8_t> rom(16384, 0x00);
    const uint8_t code[] = {0xAE, 0x10, 0x00, 0x38, 0x9E, 0x06};
    for (size_t i = 0; i < sizeof code; i++) rom[i] = code[i];
    rom[16384 - 2] = 0xC0; rom[16384 - 1] = 0x00;
    CHECK(m.loadLH5803Rom(rom.data(), rom.size()));
    m.reset();

    debugtest::Target target(m);
    target.setBreakpoints(2, {0xC003});
    debugtest::Stop s = target.runFrom(100000);
    CHECK(s.kind == debugtest::Stop::Breakpoint && s.thread == 2);
    CHECK(target.busOwner() == 2 && target.pc(2) == 0xC003);
    CHECK(target.t(2).decodeAt(0xC000).text == "sta (0x1000)");
    // Resumes past it and comes round again.
    s = target.runFrom(100000);
    CHECK(s.kind == debugtest::Stop::Breakpoint && target.pc(2) == 0xC003);
    target.setBreakpoints(2, {});

    // A write watch on the LH5803's 0x1000 fires; the same address on the
    // Z-80 side is a different place and doesn't.
    target.setWatches(1, {{0x1000, 0x1000, 0, false, true}});
    s = target.runFrom(1000);
    CHECK(s.kind == debugtest::Stop::None);
    target.setWatches(1, {});
    target.setWatches(2, {{0x1000, 0x1000, 0, false, true}});
    s = target.runFrom(100000);
    CHECK(s.kind == debugtest::Stop::Watch && s.thread == 2 && s.hit.addr == 0x1000);
    CHECK(target.pc(2) == 0xC003); // stopped after the store
    // Stepping reports the watch too.
    s = target.stepInstruction(2, 10); // nop
    CHECK(s.kind == debugtest::Stop::None);
    s = target.stepInstruction(2, 10); // bch
    s = target.stepInstruction(2, 10); // sta
    CHECK(s.kind == debugtest::Stop::Watch && s.thread == 2);
}

TEST(pc1600_skip_only_for_the_running_cpu) {
    // Same machine as test_pc1600_lh5803_thread(). The LH5803 sits at its
    // reset address C000 off the bus while the Z-80 runs; a continue must
    // not hand it the skip, so it stops at C000 when it gets the bus.
    PC1600Machine m;
    std::vector<uint8_t> lower(PC1600Memory::kBankSize, 0x00), upper(PC1600Memory::kBankSize, 0x00);
    lower[0] = 0xD3; lower[1] = 0x38; lower[2] = 0x76;
    CHECK(m.loadBank0(lower.data(), lower.size(), upper.data(), upper.size()));
    std::vector<uint8_t> rom(16384, 0x00);
    const uint8_t code[] = {0xAE, 0x10, 0x00, 0x38, 0x9E, 0x06};
    for (size_t i = 0; i < sizeof code; i++) rom[i] = code[i];
    rom[16384 - 2] = 0xC0; rom[16384 - 1] = 0x00;
    CHECK(m.loadLH5803Rom(rom.data(), rom.size()));
    m.reset();

    debugtest::Target target(m);
    CHECK(target.busOwner() == 1 && target.pc(2) == 0xC000);
    target.setBreakpoints(2, {0xC000});
    const uint32_t before = target.retired(2);
    debugtest::Stop s = target.runFrom(100000);
    CHECK(s.kind == debugtest::Stop::Breakpoint && s.thread == 2);
    CHECK(target.pc(2) == 0xC000 && target.retired(2) == before); // its first instruction, not a later pass
}

TEST(machine_step_latches_stops) {
    // PC-1500: a stepped store into a watched byte latches a watch stop on
    // CPU 1; a reset drops a stop nobody consumed.
    {
        PC1500Machine m;
        WatchSet w;
        w.add({0x4100, 0x4100, 0, false, true});
        m.setWatches(1, &w);
        const uint8_t code[] = {0xAE, 0x41, 0x00}; // sta (0x4100)
        for (size_t i = 0; i < sizeof code; i++) m.memory().poke(uint16_t(0x4000 + i), code[i]);
        m.cpu().setPC(0x4000);
        m.step();
        const DebugStop s = m.consumeDebugStop();
        CHECK(s.kind == DebugStop::Watch && s.cpu == 1);
        CHECK(m.consumeDebugStop().kind == DebugStop::None);
        m.cpu().setPC(0x4000);
        w.consumeHit();
        m.step();
        m.reset();
        CHECK(m.consumeDebugStop().kind == DebugStop::None);
        m.setWatches(1, nullptr);
    }
    // PC-1600: the Z-80's store latches CPU 1; after the handoff the
    // LH5803's store latches CPU 2 -- from step(), not only runCycles().
    {
        PC1600Machine m;
        std::vector<uint8_t> lower(PC1600Memory::kBankSize, 0x00), upper(PC1600Memory::kBankSize, 0x00);
        const uint8_t z80[] = {0x32, 0x00, 0x90, 0xD3, 0x38, 0x76}; // ld (0x9000),a ; out (38h),a ; halt
        for (size_t i = 0; i < sizeof z80; i++) lower[i] = z80[i];
        CHECK(m.loadBank0(lower.data(), lower.size(), upper.data(), upper.size()));
        std::vector<uint8_t> rom(16384, 0x00);
        const uint8_t lh[] = {0xAE, 0x10, 0x00, 0x9E, 0x05}; // sta (0x1000) ; bch C000
        for (size_t i = 0; i < sizeof lh; i++) rom[i] = lh[i];
        rom[16384 - 2] = 0xC0; rom[16384 - 1] = 0x00;
        CHECK(m.loadLH5803Rom(rom.data(), rom.size()));
        m.reset();
        WatchSet z, l;
        z.add({0x9000, 0x9000, 0, false, true});
        l.add({0x1000, 0x1000, 0, false, true});
        m.setWatches(1, &z);
        m.setWatches(2, &l);
        m.step();
        DebugStop s = m.consumeDebugStop();
        CHECK(s.kind == DebugStop::Watch && s.cpu == 1);
        z.consumeHit();
        s = {};
        for (int i = 0; i < 50 && s.kind == DebugStop::None; i++) {
            m.step();
            s = m.consumeDebugStop();
        }
        CHECK(s.kind == DebugStop::Watch && s.cpu == 2);
        m.setWatches(1, nullptr);
        m.setWatches(2, nullptr);
    }
}

TEST(pc1600_cpu_views) {
    PC1600Machine m;
    std::vector<uint8_t> lower(PC1600Memory::kBankSize, 0x76), upper(PC1600Memory::kBankSize, 0x00); // halt
    CHECK(m.loadBank0(lower.data(), lower.size(), upper.data(), upper.size()));
    std::vector<uint8_t> rom(16384, 0x38);
    rom[16384 - 2] = 0xC0; rom[16384 - 1] = 0x00;
    CHECK(m.loadLH5803Rom(rom.data(), rom.size()));
    m.reset();
    {
        debugtest::Target target(m);
        CHECK(target.threadCount() == 2 && target.t(1).kind() == debug::CpuKind::Z80 &&
              target.t(2).info().name.value == "LH5803");
        // A thread id beyond the CPUs names the last one.
        CHECK(target.t(7).kind() == debug::CpuKind::LH5803 && target.pc(7) == target.pc(2));
        // An edited PV reaches the LH5803's bus at once.
        CHECK(target.writeRegister(2, "pv", 1) && m.lh5803Memory().pv());
        CHECK(target.writeRegister(2, "pv", 0) && !m.lh5803Memory().pv());
        // The flags byte comes from the register marked as status, live
        // and in history; the bank state is the machine's own.
        uint8_t flags = 0;
        CHECK(debug::statusFlags(target.t(1).registers(), &flags) && flags == uint8_t(m.sc7852().af()));
        CHECK(debug::statusFlags(target.t(2).registers(), &flags) && flags == m.lh5803().statusReg());
        const auto z80Banks = target.t(1).bankState();
        CHECK(z80Banks.size() == 4 && z80Banks[1].name == "Page B (4000-7FFF)" && z80Banks[1].value.rfind("bank ", 0) == 0);
        const auto lhBanks = target.t(2).bankState();
        CHECK(lhBanks.size() == 2 && lhBanks[0].name == "PU" && lhBanks[1].name == "PV");
        // Halted: the Z-80 after HALT; the LH580x also when powered off.
        m.step();
        CHECK(target.t(1).historySize() > 0 && debug::statusFlags(target.t(1).historyEntry(0).registers, &flags));
        CHECK(target.t(1).halted());
        CHECK(!target.t(2).halted());
        target.setBreakpoints(1, {0x0000});
        target.setBreakpoints(2, {0xC000});
        target.setWatches(2, {{0x1000, 0x1000, 0, false, true}});
        CHECK(m.sc7852().breakpointsEnabled() && m.lh5803().breakpointsEnabled());
    }
    // The target took everything off the CPUs.
    CHECK(!m.sc7852().breakpointsEnabled() && !m.lh5803().breakpointsEnabled());
    m.lh5803().setBreakpointsEnabled(true);
    m.lh5803().setPC(0xC000);
    CHECK(m.lh5803().step() > 0); // no breakpoint left at C000
    m.lh5803().setBreakpointsEnabled(false);
}

TEST(register_reads_with_halves_and_aliases) {
    // Every listed register reads back its listed value, plus the halves
    // and aliases the list doesn't show.
    LhBus bus;
    LH5801 lh(bus);
    lh.reset();
    lh.setX(0x1234); lh.setY(0x5678); lh.setU(0x9ABC); lh.setA(0x42); lh.setStatusReg(0x15); lh.setPV(true);
    uint32_t v = 0;
    for (const debug::Register& r : debug::lhRegisters(lh))
        CHECK(debug::lhReadRegister(lh, r.name, &v) && v == r.value);
    CHECK(debug::lhReadRegister(lh, "xh", &v) && v == 0x12 && debug::lhReadRegister(lh, "ul", &v) && v == 0xBC);
    CHECK(debug::lhReadRegister(lh, "zf", &v) && !debug::lhReadRegister(lh, "q", &v));

    ZBus zbus;
    SC7852 z(zbus);
    z.reset();
    z.setAF(0x1234); z.setBC(0x5678); z.setIX(0xABCD); z.setAF2(0x4321); z.setI(0x3F);
    for (const debug::Register& r : debug::z80Registers(z))
        CHECK(debug::z80ReadRegister(z, r.name, &v) && v == r.value);
    CHECK(debug::z80ReadRegister(z, "af2", &v) && v == 0x4321);
    CHECK(debug::z80ReadRegister(z, "a", &v) && v == 0x12 && debug::z80ReadRegister(z, "f", &v) && v == 0x34);
    CHECK(debug::z80ReadRegister(z, "ixl", &v) && v == 0xCD && debug::z80ReadRegister(z, "c", &v) && v == 0x78);
    CHECK(debug::z80ReadRegister(z, "zf", &v) && !debug::z80ReadRegister(z, "q", &v));
}

TEST(watch_set_add_remove_match) {
    WatchSet w;
    w.add({0x0000, 0x0001, 0, true, false});   // read, ME0
    w.add({0xFFFF, 0xFFFF, 1, false, true});   // write, ME1
    w.add({0xFFF0, 0x000F, 0, true, true});    // wrapped: never matches
    w.check(0x0002, 0, false, 0);
    CHECK(!w.hitPending());
    w.check(0x0001, 0, true, 0);               // a write where only reads are watched
    CHECK(!w.hitPending());
    w.check(0xFFFF, 0, true, 0);               // the right address in the wrong space
    w.check(0xFFF5, 0, false, 0);              // inside the wrapped range
    CHECK(!w.hitPending());
    w.check(0x0001, 0x5A, false, 0);
    w.check(0xFFFF, 0x77, true, 1);            // later hits don't replace the first
    CHECK(w.hitPending());
    const WatchHit hit = w.consumeHit();
    CHECK(hit.addr == 0x0001 && hit.value == 0x5A && !hit.write && hit.space == 0 && !w.hitPending());
    w.check(0xFFFF, 0x77, true, 1);
    CHECK(w.hitPending() && w.consumeHit().space == 1);
    w.clear();
    w.check(0x0001, 0, false, 0);
    CHECK(!w.hitPending() && w.empty());
}


// ── The debug targets of the machines ────────────────────────────────────

using namespace calcu::machine;
using calcu::machine::BankSelect;
using calcu::machine::CellRead;

std::unique_ptr<Machine> built(const char* model, std::optional<VariantId> v = std::nullopt,
                               std::vector<PeripheralRequest> p = {}) {
    static const Catalogue c = sharptest::catalogueWithModules();
    return sharptest::buildSharp(c, model, std::move(v), std::move(p));
}

std::vector<std::string> names(const calcu::machine::Debugging& d) {
    std::vector<std::string> out;
    for (std::size_t i = 0; i < d.targetCount(); ++i) out.push_back(d.target(i).info().name.value);
    return out;
}

std::size_t spaceOf(const calcu::machine::DebugTarget& t, const std::string& name) {
    for (std::size_t i = 0; i < t.info().spaces.size(); ++i)
        if (t.info().spaces[i].name == name) return i;
    return 99;
}

TEST(pc1500_and_pc1500a_have_the_lh5801) {
    for (const char* model : {"PC-1500", "PC-1500A"}) {
        auto m = built(model);
        SKIP_UNLESS(m != nullptr, "ROMs missing");
        CHECK(m->debugging() != nullptr);
        CHECK(names(*m->debugging()) == std::vector<std::string>{"LH5801"});
        CHECK(m->debugTargets().size() == 1 && m->debugTargets()[0].value == "LH5801");   // as the machine names it
    }
}

TEST(pc1600_has_the_sc7852_and_the_lh5803) {
    auto m = built("PC-1600");
    SKIP_UNLESS(m != nullptr, "ROMs missing");
    CHECK((names(*m->debugging()) == std::vector<std::string>{"SC7852", "LH5803"}));
    CHECK(m->debugTargets().size() == 2 && m->debugTargets()[0].value == "SC7852" && m->debugTargets()[1].value == "LH5803");
}

TEST(registers_spaces_offers_history_32) {
    auto m = built("PC-1600");
    SKIP_UNLESS(m != nullptr, "ROMs missing");
    m->coldBoot();
    for (int i = 0; i < 30; ++i) m->advance(m->descriptor().tickRate / 60);
    calcu::machine::Debugging& d = *m->debugging();
    auto regNames = [](const calcu::machine::DebugTarget& t) {
        std::string s;
        for (const auto& r : t.info().registers) s += r.name + "/" + std::to_string(r.bits) + " ";
        return s;
    };
    CHECK(regNames(d.target(0)).find("af/16 bc/16 de/16 hl/16 ix/16 iy/16 sp/16 pc/16") == 0);
    CHECK(regNames(d.target(1)).find("a/8 x/16 y/16 u/16 s/16 p/16 t/8 pu/1 pv/1") == 0);
    std::vector<std::string> spaces;
    for (const auto& sp : d.target(0).info().spaces) spaces.push_back(sp.name);
    CHECK((spaces == std::vector<std::string>{"memory", "io", "io-written"}));
    spaces.clear();
    for (const auto& sp : d.target(1).info().spaces) spaces.push_back(sp.name);
    CHECK((spaces == std::vector<std::string>{"me0", "me1", "me1-written"}));
    for (std::size_t i = 0; i < 2; ++i) {
        const auto& o = d.target(i).info().offers;
        CHECK(o.watches && o.stepOverOut && o.decode && o.history == 32);
        CHECK_EQ(d.target(i).info().codeSpace, std::size_t(0));
    }
    const auto h = d.target(0).history(40);
    CHECK_EQ(h.size(), std::size_t(32));
    CHECK(!h.empty() && h[0].text && !h[0].text->empty());
    CHECK(!h.empty() && h[0].registers.size() == d.target(0).info().registers.size());
    CHECK(d.target(0).decode(0x0000).has_value());
}

TEST(qualifier_fields_bank_me_pu_pv) {
    auto m = built("PC-1600");
    SKIP_UNLESS(m != nullptr, "ROMs missing");
    m->coldBoot();
    for (int i = 0; i < 30; ++i) m->advance(m->descriptor().tickRate / 60);
    calcu::machine::Debugging& d = *m->debugging();
    const auto& zq = d.target(0).info().offers.qualifier;
    CHECK(zq.size() == 1 && zq[0].name == "bank" && zq[0].values.size() == 8);
    const auto& lq = d.target(1).info().offers.qualifier;
    CHECK(lq.size() == 3 && lq[0].name == "me" && lq[1].name == "pu" && lq[2].name == "pv");
    // The bank of the page the PC is in; a bank that is not switched in does not match.
    const std::uint64_t pc = d.target(0).nextAddress();
    const std::string now = d.target(0).qualifierNow().at(0);
    CHECK(d.target(0).qualifierMatches({now}, pc));
    CHECK(!d.target(0).qualifierMatches({now == "7" ? "6" : "7"}, pc));
    CHECK(d.target(0).qualifierMatches({""}, pc));   // a field left free
    // ME1: code never runs from there.
    CHECK(!d.target(1).qualifierMatches({"1", "", ""}, 0xC000));
}

TEST(conformance_on_every_model) {
    struct Case { const char* model; std::optional<VariantId> v; };
    const Case cases[] = {{"PC-1500", VariantId{"A01"}}, {"PC-1500", VariantId{"A03"}}, {"PC-1500", VariantId{"A04"}},
                          {"PC-1500A", std::nullopt}, {"PC-1600", VariantId{"New"}}, {"PC-1600", VariantId{"Old"}}};
    for (const Case& c : cases) {
        auto m = built(c.model, c.v);
        SKIP_UNLESS(m != nullptr, "ROMs missing");
        m->coldBoot();
        m->advance(m->descriptor().tickRate / 10);
        const auto problems = calcu::machine::checkDebugging(*m);
        for (const auto& p : problems) std::fprintf(stderr, "  %s: %s\n", c.model, p.c_str());
        CHECK(problems.empty());
    }
}

TEST(rom_and_empty_cells_refuse_a_write) {
    auto m = built("PC-1500A");
    SKIP_UNLESS(m != nullptr, "ROMs missing");
    calcu::machine::DebugTarget& t = m->debugging()->target(0);
    const std::size_t me0 = spaceOf(t, "me0");
    CHECK(!t.poke(me0, 0xC000, 0x00, {}));    // ROM
    CHECK(!t.poke(me0, 0x0000, 0x00, {}));    // the module area, no module: open bus
    CHECK(t.peek(me0, 0x0000, {}).value == 0xFF);
    CHECK(t.poke(me0, 0x7A00, 0x99, {}));     // RAM
    CHECK(t.peek(me0, 0x7A00, {}).value == 0x99);
    auto p = built("PC-1600");
    SKIP_UNLESS(p != nullptr, "ROMs missing");
    calcu::machine::DebugTarget& z = p->debugging()->target(0);
    CHECK(!z.poke(spaceOf(z, "memory"), 0x0000, 0x00, {}));   // the system ROM
}

TEST(me1_device_cells_peek_without_side_effect) {
    auto m = built("PC-1500A");
    SKIP_UNLESS(m != nullptr, "ROMs missing");
    calcu::machine::DebugTarget& t = m->debugging()->target(0);
    const std::size_t me1 = spaceOf(t, "me1");
    // The LH5811's interrupt flags read as a value, and a peek leaves them as they are.
    const CellRead flags = t.peek(me1, 0xF00B, {});
    CHECK(flags.kind == CellRead::Value);
    CHECK(t.peek(me1, 0xF00B, {}).value == flags.value);
    auto p = built("PC-1600");
    SKIP_UNLESS(p != nullptr, "ROMs missing");
    calcu::machine::DebugTarget& lh = p->debugging()->target(1);
    CHECK(lh.peek(spaceOf(lh, "me1"), 0xA020, {}).kind == CellRead::Unreadable);   // its UART block
}

TEST(lh5803_written_only_in_ram) {
    auto p = built("PC-1600");
    SKIP_UNLESS(p != nullptr, "ROMs missing");
    p->coldBoot();
    for (int i = 0; i < 30; ++i) p->advance(p->descriptor().tickRate / 60);
    calcu::machine::DebugTarget& z = p->debugging()->target(0);
    calcu::machine::DebugTarget& lh = p->debugging()->target(1);
    // The LH5803's 0000-7FFF is the Z-80's 8000-FFFF: internal RAM at C000 up is its 4000 up.
    CHECK(lh.poke(spaceOf(lh, "me0"), 0x4200, 0x5A, {}));
    CHECK(z.peek(spaceOf(z, "memory"), 0xC200, {}).value == 0x5A);
    CHECK(!lh.poke(spaceOf(lh, "me0"), 0xC000, 0x00, {}));   // its ROM
    CHECK(!lh.poke(spaceOf(lh, "me0"), 0x9000, 0x00, {}));   // neither RAM nor ROM of its own
}

TEST(refused_poke_changes_nothing) {
    auto m = built("PC-1500A");
    SKIP_UNLESS(m != nullptr, "ROMs missing");
    calcu::machine::DebugTarget& t = m->debugging()->target(0);
    const std::size_t me0 = spaceOf(t, "me0");
    const std::uint64_t before = t.peek(me0, 0xC000, {}).value;
    const calcu::machine::Ticks ticks = m->tickCount();
    CHECK(!t.poke(me0, 0xC000, before ^ 0xFF, {}));
    CHECK_EQ(t.peek(me0, 0xC000, {}).value, before);
    CHECK_EQ(m->tickCount(), ticks);
}

TEST(sc7852_io_space) {
    auto p = built("PC-1600");
    SKIP_UNLESS(p != nullptr, "ROMs missing");
    p->coldBoot();
    for (int i = 0; i < 30; ++i) p->advance(p->descriptor().tickRate / 60);
    calcu::machine::DebugTarget& z = p->debugging()->target(0);
    const std::size_t io = spaceOf(z, "io");
    CHECK(io < 99 && z.info().spaces[io].size == 256);
    // The bank register reads where reading has no effect; a device port does not.
    const CellRead bank = z.peek(io, 0x31, {});
    CHECK(bank.kind == CellRead::Value);
    CHECK(z.peek(io, 0x10, {}).kind == CellRead::Unreadable);
}

TEST(written_views_of_me1_and_io) {
    auto a = built("PC-1500A");
    auto p = built("PC-1600");
    SKIP_UNLESS(a != nullptr && p != nullptr, "ROMs missing");
    for (auto* m : {a.get(), p.get()}) {
        m->coldBoot();
        for (int i = 0; i < 60; ++i) m->advance(m->descriptor().tickRate / 60);
    }
    // The boot wrote the LH5811's port registers (ME1 F00x) and the bank port 31H.
    calcu::machine::DebugTarget& lh = a->debugging()->target(0);
    const std::size_t w = spaceOf(lh, "me1-written");
    CHECK(lh.info().spaces[w].writtenView);
    bool any = false;
    for (std::uint64_t addr = 0xF000; addr <= 0xF00F; ++addr) any = any || lh.peek(w, addr, {}).kind == CellRead::Value;
    CHECK(any);
    CHECK(lh.peek(w, 0x0001, {}).kind == CellRead::NoneWritten);
    CHECK(!lh.poke(w, 0xF000, 0, {}));   // read-only
    calcu::machine::DebugTarget& z = p->debugging()->target(0);
    const std::size_t iow = spaceOf(z, "io-written");
    const CellRead port31 = z.peek(iow, 0x31, {});
    CHECK(port31.kind == CellRead::Value);
    CHECK(port31.kind != CellRead::Value || port31.value == z.peek(spaceOf(z, "io"), 0x31, {}).value);
    // Reading them has no effect.
    const calcu::machine::Ticks ticks = p->tickCount();
    for (std::uint64_t port = 0; port < 256; ++port) (void)z.peek(iow, port, {});
    CHECK_EQ(p->tickCount(), ticks);
}

TEST(every_bank_of_the_pc1600_and_its_modules) {
    auto p = built("PC-1600", std::nullopt, {sharptest::moduleRequest("CE-1600M", "slot-1")});
    SKIP_UNLESS(p != nullptr, "ROMs missing");
    p->coldBoot();
    for (int i = 0; i < 60; ++i) p->advance(p->descriptor().tickRate / 60);
    calcu::machine::DebugTarget& z = p->debugging()->target(0);
    const std::size_t mem = spaceOf(z, "memory");
    const auto& banked = z.info().spaces[mem].banked;
    CHECK(banked.size() == 4);
    const std::uint8_t port31 = static_cast<std::uint8_t>(z.peek(spaceOf(z, "io"), 0x31, {}).value);
    // Page C bank 6 is the display/timer ROM; bank 0 is Slot 1's module.
    const CellRead rom6 = z.peek(mem, 0x8000, BankSelect{6, std::nullopt});
    CHECK(rom6.kind == CellRead::Value);
    CHECK(z.poke(mem, 0x8000, 0xA5, BankSelect{0, std::nullopt}));   // the module's RAM, through bank 0
    CHECK_EQ(z.peek(mem, 0x8000, BankSelect{0, std::nullopt}).value, std::uint64_t(0xA5));
    // Page B: bank 0 (system ROM upper half) and bank 3 differ.
    const CellRead b0 = z.peek(mem, 0x4000, BankSelect{0, std::nullopt});
    const CellRead b3 = z.peek(mem, 0x4000, BankSelect{3, std::nullopt});
    CHECK(b0.kind == CellRead::Value && b3.kind == CellRead::Value);
    // No bank was switched for the machine.
    CHECK_EQ(static_cast<std::uint8_t>(z.peek(spaceOf(z, "io"), 0x31, {}).value), port31);
}

TEST(vertical_banks_of_a_slot_2_module) {
    auto p = built("PC-1600", std::nullopt, {sharptest::moduleRequest("CE-1601M", "slot-2")});
    SKIP_UNLESS(p != nullptr, "ROMs missing");
    p->coldBoot();
    for (int i = 0; i < 60; ++i) p->advance(p->descriptor().tickRate / 60);
    calcu::machine::DebugTarget& z = p->debugging()->target(0);
    const std::size_t mem = spaceOf(z, "memory");
    const auto& c = z.info().spaces[mem].banked.at(2);
    CHECK(c.first == 0x8000 && c.vbanks > 0);
    // Two vertical banks of the module hold their own bytes; the machine's
    // vertical bank is left as it was.
    const CellRead vb = z.peek(spaceOf(z, "io"), 0x28, {});
    const bool w0 = z.poke(mem, 0x8100, 0x11, BankSelect{2, 0});
    const bool w1 = z.poke(mem, 0x8100, 0x22, BankSelect{2, 1});
    CHECK(w0 && w1);
    CHECK_EQ(z.peek(mem, 0x8100, BankSelect{2, 0}).value, std::uint64_t(0x11));
    CHECK_EQ(z.peek(mem, 0x8100, BankSelect{2, 1}).value, std::uint64_t(0x22));
    CHECK_EQ(z.peek(spaceOf(z, "io"), 0x28, {}).value, vb.value);
}

} // namespace

// A poke on an I/O space is the CPU's own write, with its effects: an OUT
// of the SC7852, a store to ME1 of the LH5801/LH5803; the written views keep it.
TEST(poke_on_io_and_me1_is_a_real_write) {
    auto a = built("PC-1500A");
    auto p = built("PC-1600");
    SKIP_UNLESS(a != nullptr && p != nullptr, "ROMs missing");
    for (auto* m : {a.get(), p.get()}) {
        m->coldBoot();
        for (int i = 0; i < 60; ++i) m->advance(m->descriptor().tickRate / 60);
    }
    // PC-1500A: the LH5811's DDA (ME1 F00C) takes the value and reads it back.
    calcu::machine::DebugTarget& lh = a->debugging()->target(0);
    CHECK(lh.poke(spaceOf(lh, "me1"), 0xF00C, 0x5A, {}));
    CHECK_EQ(lh.peek(spaceOf(lh, "me1"), 0xF00C, {}).value, std::uint64_t{0x5A});
    CHECK_EQ(lh.peek(spaceOf(lh, "me1-written"), 0xF00C, {}).value, std::uint64_t{0x5A});
    CHECK(!lh.poke(spaceOf(lh, "me1-written"), 0xF00C, 0x00, {}));   // a written view is not written
    // PC-1600: OUT (58H),3EH switches IC2's display off.
    calcu::machine::DebugTarget& z = p->debugging()->target(0);
    CHECK(z.poke(spaceOf(z, "io"), 0x58, 0x3E, {}));
    CHECK_EQ(z.peek(spaceOf(z, "io-written"), 0x58, {}).value, std::uint64_t{0x3E});
    const auto parts = p->readDisplay().parts;
    CHECK(parts.size() == 2 && parts[0].name == "IC2" && !parts[0].on && parts[1].on);
    CHECK(!z.poke(spaceOf(z, "io-written"), 0x58, 0x3F, {}));
    // The LH5803's ME1 store is kept in its written view.
    calcu::machine::DebugTarget& lh3 = p->debugging()->target(1);
    CHECK(lh3.poke(spaceOf(lh3, "me1"), 0xF00C, 0x21, {}));
    CHECK_EQ(lh3.peek(spaceOf(lh3, "me1-written"), 0xF00C, {}).value, std::uint64_t{0x21});
}
