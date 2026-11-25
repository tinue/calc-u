// Headless C++ tests for the CE-150 printer/plotter/cassette interface:
// Ce150Card's LH5810 register + ROM-window decode, its use of the shared
// AlpsPlotterMechanism, and its integration into PC1500Machine over the
// 60-pin SystemBus (including the ME1 0xB00x shadow of the internal
// LH5811). Same no-framework, assert-and-tally style as lh5801_tests.cpp.
//
// Build & run: see tools/run_tests.sh

#include <cstdio>
#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

#include "../core/Connector/Ce150Card.hpp"
#include "../core/Connector/PC1500SignalDecode.hpp"
#include "../core/Connector/PlotterPaperOutput.hpp"
#include "../core/PC1500/PC1500Machine.hpp"
#include "support/Typing.hpp"
#include "TestHarness.hpp"
#include "TestRoms.hpp"
#include "paper/Paper.hpp"

namespace {

// cmotor.cpp's 8-position drive ring (see AlpsPlotterMechanism / ce1600p_tests):
// adjacent entries are a half-step (+/-1) in the X motor's own convention.
constexpr uint8_t kRing[8] = {0x9, 0x8, 0xc, 0x4, 0x6, 0x2, 0x3, 0x1};

// Cycles as the PC-1500 drives the 60-pin contacts (PU low).
SystemBusPins me1Read(uint16_t addr) {
    return PC1500SignalDecode::systemBusPins(addr, /*forWrite=*/false, /*me1=*/true, false, false);
}
SystemBusPins me1Write(uint16_t addr) {
    return PC1500SignalDecode::systemBusPins(addr, /*forWrite=*/true, /*me1=*/true, false, false);
}
SystemBusPins me0Read(uint16_t addr, bool pv = false) {
    return PC1500SignalDecode::systemBusPins(addr, /*forWrite=*/false, /*me1=*/false, false, pv);
}

std::vector<uint8_t> fakeRom() {
    std::vector<uint8_t> rom(Ce150Card::kRomSize);
    for (size_t i = 0; i < rom.size(); ++i) rom[i] = uint8_t(i * 7 + 3);
    return rom;
}

// ── ROM window ────────────────────────────────────────────────────────

TEST(test_rom_window_decode_and_negatives) {
    Ce150Card card;
    auto rom = fakeRom();
    CHECK(card.loadRom(rom.data(), rom.size()));
    CHECK(!card.loadRom(rom.data(), rom.size() - 1)); // wrong size rejected

    uint8_t v = 0;
    SystemBusPins p = me0Read(0xA000);
    CHECK(card.respondsToRead(p, v) && v == rom[0]);
    p = me0Read(0xBFFF);
    CHECK(card.respondsToRead(p, v) && v == rom[0x1FFF]);

    // Not the ROM: PV high, a write, an ME1 access, or an address outside
    // 0xA000-0xBFFF all decline the ROM window.
    p = me0Read(0xA000, /*pv=*/true);
    CHECK(!card.respondsToRead(p, v));
    p = me0Read(0xA000); p.forWrite = true;
    CHECK(!card.respondsToWrite(p, 0x55)); // ROM is read-only
    p = me0Read(0x9FFF);
    CHECK(!card.respondsToRead(p, v));
    p = me0Read(0xC000);
    CHECK(!card.respondsToRead(p, v));
}

// ── LH5810 register block ─────────────────────────────────────────────

TEST(test_lh5810_latches_and_direction_masking) {
    Ce150Card card;
    uint8_t v = 0;

    // G / MSK / IF / DDA / DDB are plain read/write latches.
    for (uint16_t addr : {uint16_t(0xB009), uint16_t(0xB00A), uint16_t(0xB00B),
                          uint16_t(0xB00C), uint16_t(0xB00D)}) {
        SystemBusPins w = me1Write(addr);
        CHECK(card.respondsToWrite(w, 0x5A));
        SystemBusPins r = me1Read(addr);
        CHECK(card.respondsToRead(r, v) && v == 0x5A);
    }

    // OPA read returns input bits only (GetReg(OPA) = r_opa & ~r_dda): the
    // output bits it wrote (incl. the latched-but-ignored RMT bits) read
    // back as 0, and the CE-150 injects no OPA input level.
    { SystemBusPins w = me1Write(0xB00C); card.respondsToWrite(w, 0x06); }     // DDA: bits 1,2 (RMT) = output
    { SystemBusPins w = me1Write(0xB00E); card.respondsToWrite(w, 0xFF); }     // OPA := 0xFF (only bits 1,2 stick)
    { SystemBusPins r = me1Read(0xB00E); card.respondsToRead(r, v); CHECK(v == 0x00); }

    // Unclaimed neighbours: 0xB007 (below the block) is not ours.
    SystemBusPins r = me1Read(0xB007);
    CHECK(!card.respondsToRead(r, v));
}

// ── steppers via OPC ──────────────────────────────────────────────────

void writeOpc(Ce150Card& card, uint8_t value) {
    SystemBusPins w = me1Write(0xB008);
    card.respondsToWrite(w, value);
}

// A paper connected to the card's paper output, as the
// session connects one (the strokes are the paper's, not the mechanism's).
struct PaperOf {
    Ce150Card& card;
    calcu::paper::Paper paper{card.paperOutput().description()};
    explicit PaperOf(Ce150Card& c) : card(c) { paper.bind(&card.paperOutput()); }
    ~PaperOf() { paper.bind(nullptr); }
    std::size_t strokes() const { return paper.marks().size(); }
    const calcu::paper::Stroke& last() const { return std::get<calcu::paper::Stroke>(paper.marks().back()); }
};

TEST(ce150_stepper_steps_become_places) {
    Ce150Card card;
    // Anchor both motors at ring position 0 (phase 0x9 in both nibbles).
    writeOpc(card, 0x99);
    CHECK(card.mechanism().penX() == 0);
    CHECK(card.mechanism().penY() == 0);

    // Walk X forward 4 ring positions (low nibble), Y still parked (high
    // nibble held at 0x9). 4 half-steps forward = +4 in X's convention.
    int pos = 0;
    for (int i = 0; i < 4; ++i) {
        pos = (pos + 1) % 8;
        writeOpc(card, uint8_t(0x90 | kRing[pos]));
    }
    CHECK(card.mechanism().penX() == 4);
    CHECK(card.mechanism().penY() == 0);

    // Now walk Y forward 3 (high nibble), X parked. Y applies the opposite
    // sign (see AlpsPlotterMechanism::stepXY) -> penY goes negative.
    int ypos = 0;
    for (int i = 0; i < 3; ++i) {
        ypos = (ypos + 1) % 8;
        writeOpc(card, uint8_t((kRing[ypos] << 4) | kRing[pos]));
    }
    CHECK(card.mechanism().penY() == -3);
    CHECK(card.mechanism().penX() == 4); // unchanged: X nibble held constant
}

// ── pen up/down via OPB ───────────────────────────────────────────────

TEST(ce150_a_stroke_per_pen_down) {
    Ce150Card card;
    PaperOf p(card);
    writeOpc(card, 0x99); // anchor

    { SystemBusPins w = me1Write(0xB00D); card.respondsToWrite(w, 0x03); } // DDB: PB0/PB1 output
    CHECK(!card.mechanism().penDown());

    { SystemBusPins w = me1Write(0xB00F); card.respondsToWrite(w, 0x02); } // PB1 descending -> pen down
    CHECK(card.mechanism().penDown());
    CHECK(p.strokes() == 1);

    // move X while down -> stroke extends
    int pos = 0;
    for (int i = 0; i < 3; ++i) { pos = (pos + 1) % 8; writeOpc(card, uint8_t(0x90 | kRing[pos])); }
    CHECK(p.last().path.size() == 4); // pen-down point + 3 moves

    { SystemBusPins w = me1Write(0xB00F); card.respondsToWrite(w, 0x01); } // PB0 ascending -> pen up
    CHECK(!card.mechanism().penDown());
}

// ── colour turret at the carriage left stop ───────────────────────────

TEST(ce150_colour_every_third_arrival) {
    Ce150Card card;
    writeOpc(card, 0x99); // anchor X at ring pos 0

    using Color = AlpsPlotterMechanism::PenColor;
    CHECK(card.mechanism().penColor() == Color::Black);

    int pos = 0;
    auto driveLeft = [&](int steps) {
        for (int i = 0; i < steps; ++i) { pos = (pos - 1 + 8) % 8; writeOpc(card, uint8_t(0x90 | kRing[pos])); }
    };
    auto driveRight = [&](int steps) {
        for (int i = 0; i < steps; ++i) { pos = (pos + 1) % 8; writeOpc(card, uint8_t(0x90 | kRing[pos])); }
    };

    // First run to the stop: 1 detent (Rot 0 -> 1), colour unchanged (needs
    // 3 detents). The colour magnet reads true only at Rot == 0 in COLOR0
    // -- i.e. before the first detent or after a full turret revolution --
    // so it is already false once the carriage has clicked once. The stop
    // clamp is at -90 (kCE150LeftStopX), so the leftward runs below have to
    // clear that; each ring-walk step here is one raw penX unit.
    driveLeft(110);
    CHECK(card.mechanism().penX() == -90);      // clamped at the physical stop
    CHECK(card.mechanism().penColor() == Color::Black);
    CHECK(!card.mechanism().colorMagnet());

    // Two more arm/arrive cycles -> the 3rd detent advances the colour.
    // driveRight must clear the arm threshold (-32) so the next leftward
    // pass re-arms; driveLeft must reach the -90 stop.
    driveRight(70); driveLeft(90);
    driveRight(70); driveLeft(90);
    CHECK(card.mechanism().penColor() == Color::Blue);
    CHECK(!card.mechanism().colorMagnet());

    // Holding against the stop must not ratchet further detents.
    Color held = card.mechanism().penColor();
    driveLeft(20);
    CHECK(card.mechanism().penColor() == held);
}

TEST(reset_keeps_the_paper) {
    Ce150Card card;
    PaperOf p(card);
    writeOpc(card, 0x99);
    { SystemBusPins w = me1Write(0xB00D); card.respondsToWrite(w, 0x03); }
    { SystemBusPins w = me1Write(0xB00F); card.respondsToWrite(w, 0x02); } // pen down -> opens a stroke
    int pos = 0;
    for (int i = 0; i < 5; ++i) { pos = (pos + 1) % 8; writeOpc(card, uint8_t(0x90 | kRing[pos])); }
    CHECK(card.mechanism().penX() == 5);
    CHECK(p.strokes() == 1);
    // Feed 4 half steps with the pen down: the stroke goes down to y = 4.
    int ypos = 0;
    for (int i = 0; i < 4; ++i) { ypos = (ypos - 1 + 8) % 8; writeOpc(card, uint8_t((kRing[ypos] << 4) | kRing[pos])); }
    CHECK(card.mechanism().penY() == 4);

    card.reset();
    CHECK(card.mechanism().penX() == 0);
    CHECK(card.mechanism().penY() == 0);
    CHECK(!card.mechanism().penDown());
    CHECK(p.strokes() == 1); // ink stays on the paper
    CHECK(card.paperOutput().standing() == 4 * calcu::paper::kPerUnit);   // the paper stands where it stood

    // The next plot goes on below what was drawn, not over it.
    writeOpc(card, 0x99);
    { SystemBusPins w = me1Write(0xB00D); card.respondsToWrite(w, 0x03); }
    { SystemBusPins w = me1Write(0xB00F); card.respondsToWrite(w, 0x02); }
    CHECK(p.strokes() == 2);
    CHECK(p.last().path.front().y == 4 * calcu::paper::kPerUnit);
    card.reset();

    uint8_t v = 0;
    SystemBusPins r = me1Read(0xB00D);
    CHECK(card.respondsToRead(r, v) && v == 0x00); // DDB latch cleared
}

// ── integration on a real PC1500Machine over the 60-pin SystemBus ─────

TEST(test_machine_rom_visible_and_me1_block_routed) {
    PC1500Machine machine(PC1500Variant::PC1500A);
    auto rom = fakeRom();
    CHECK(machine.attachCE150(rom.data(), rom.size()));
    CHECK(machine.ce150Attached());

    // ROM byte reachable through resolve()->readOpenBus()->SystemBus.
    CHECK(machine.memory().peek(0xA000) == rom[0]);
    CHECK(machine.memory().peek(0xB123) == rom[0xB123 - 0xA000]);

    // ME1 0xB00x round-trips to the card (DDA latch).
    machine.memory().writeME1(0xB00C, 0x77);
    CHECK(machine.memory().readME1(0xB00C) == 0x77);

    machine.detachCE150();
    CHECK(!machine.ce150Attached());
    CHECK(machine.memory().peek(0xA000) == 0xFF); // open bus again
}

TEST(test_ce150_and_internal_lh5811_coexist) {
    PC1500Machine machine(PC1500Variant::PC1500A);
    auto rom = fakeRom();
    machine.attachCE150(rom.data(), rom.size());

    // The internal LH5811 still answers at its canonical 0xF00x address...
    machine.memory().writeME1(0xF00C, 0xAB); // internal DDA
    CHECK(machine.memory().readME1(0xF00C) == 0xAB);

    // ...independently of the CE-150's own DDA at 0xB00C.
    machine.memory().writeME1(0xB00C, 0x14);
    CHECK(machine.memory().readME1(0xB00C) == 0x14);
    CHECK(machine.memory().readME1(0xF00C) == 0xAB); // internal DDA untouched

    // With no card attached, 0xB00B still reads the internal IF register
    // (0xB00x aliases the internal chip on real hardware).
    PC1500Machine bare(PC1500Variant::PC1500A);
    bare.memory().writeME1(0xF00B, 0x02);
    CHECK((bare.memory().readME1(0xB00B) & 0x02) == 0x02);
}

// ── preset loader: plotter: CE-150 ────────────────────────────────────

const char* kSysRom = "roms/PC-1500_A04.ROM";
const char* kCe150Rom = "roms/CE-150.ROM";

bool fileExists(const char* p) { std::ifstream f(p); return f.good(); }

bool writeScratchPreset(const std::string& path, const std::string& body) {
    std::ofstream f(path);
    f << body;
    return f.good();
}

// The CE-150 on the bus before the cold boot (a preset's `printer: CE-150`,
// built by the library's builder; here on the class built directly with its ROMs).
TEST(test_preset_plotter_ce150_attaches_before_reset) {
    SKIP_UNLESS(testRomSet() != nullptr, "the ROMs are not fetched (roms/fetch_roms.sh)");
    PC1500Machine machine(PC1500Variant::PC1500A);
    std::string err;
    CHECK(presetroms::loadPC1500Rom(machine, "A04", testPresetRoms(), &err));
    CHECK(presetroms::attachCE150(machine, testPresetRoms(), &err));
    sharptest::bootToPrompt(machine);
    CHECK(machine.ce150Attached());

    // The CE-150 ROM in 0x8000-0xBFFF is only visible with PV = 0: the
    // system ROM rests at PV = 1 (its own extension tables) and issues RPV
    // just before jumping into the peripheral ROM (Ref/CPU/LH5801-PU-PV-Signals.md). Boot
    // settles at PV = 1, so at rest the window is open bus; drop PV and the
    // CE-150 ROM appears.
    machine.memory().updatePUPV(machine.cpu().pu(), /*pv=*/true);
    CHECK(machine.memory().peek(0xA000) == 0xFF);
    machine.memory().updatePUPV(machine.cpu().pu(), /*pv=*/false);
    CHECK(machine.memory().peek(0xA000) != 0xFF);
}

} // namespace
