// C1600-2 machine code (AC-8 to AC-12, AC-23; design T5 to T7): the one
// plan (core/MachineCodeFile's planLoad through sharp::loaders::planCode),
// the writers on the loader memory access (PC1500MachineCodeLoader,
// PC1600MachineCodeLoader), the reply (advice, the work-area warning) and
// hex-address parsing. Ported from 0.8.1's machine_code_file_tests.cpp.

#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>
#include <variant>
#include <vector>

#include "../core/Loaders.hpp"
#include "../core/Machine/SharpLoaderMemory.hpp"
#include "../core/MachineCodeFile.hpp"
#include "support/PC1500BasicTyper.hpp"
#include "../core/PC1500/PC1500Machine.hpp"
#include "../core/PC1500/PC1500MachineCodeLoader.hpp"
#include "support/PC1600BasicTyper.hpp"
#include "../core/PC1600/PC1600Machine.hpp"
#include "../core/PC1600/PC1600MachineCodeLoader.hpp"
#include "LoaderTestSupport.hpp"
#include "TestCards.hpp"
#include "TestHarness.hpp"
#include "TestRoms.hpp"

namespace {

using machinecode::BasicArea;
using machinecode::Cpu;
using machinecode::File;
using machinecode::LoadError;
using machinecode::LoadOptions;
using machinecode::PC1600State;
using machinecode::Slot;
using machinecode::Target;

const std::vector<uint8_t> kCode = {0x3E, 0x41, 0xC9, 0x00, 0x11};  // 5 bytes

std::vector<uint8_t> pc1600File(const std::vector<uint8_t>& payload, uint32_t load, uint32_t autorun,
                                uint8_t type = 0x10) {
    std::vector<uint8_t> f = {0xFF, 0x10, 0x00, 0x00, type};
    const uint32_t n = static_cast<uint32_t>(payload.size());
    for (uint32_t v : {n, load, autorun}) {
        f.push_back(static_cast<uint8_t>(v & 0xFF));
        f.push_back(static_cast<uint8_t>((v >> 8) & 0xFF));
        f.push_back(static_cast<uint8_t>((v >> 16) & 0xFF));
    }
    f.push_back(0x00);
    f.push_back(0x0F);
    f.insert(f.end(), payload.begin(), payload.end());
    return f;
}

std::vector<uint8_t> ce158File(const std::vector<uint8_t>& payload, uint16_t load, uint16_t autorun,
                               uint8_t type = 0x42) {
    std::vector<uint8_t> f = {0x01, type, 'C', 'O', 'M'};
    const char name[16] = "TEST";
    f.insert(f.end(), name, name + 16);
    const uint16_t lenField = static_cast<uint16_t>(payload.size() - 1);  // stored as length - 1
    for (uint16_t v : {load, lenField, autorun}) {
        f.push_back(static_cast<uint8_t>(v >> 8));
        f.push_back(static_cast<uint8_t>(v & 0xFF));
    }
    f.insert(f.end(), payload.begin(), payload.end());
    return f;
}

const std::vector<BasicArea> kStockAreas = {{0, 0xC000, 0xEFFF, 0}};
const std::vector<BasicArea> kSlot1FirstAreas = {{1, 0x8000, 0xBFFF, 0}, {0, 0xC000, 0xEFFF, 0}};
const std::vector<BasicArea> kSlot2FirstAreas = {{2, 0x8000, 0xBFFF, 2}, {0, 0xC000, 0xEFFF, 0}};

PC1600State state(const std::vector<BasicArea>& areas, bool mode1 = false, int title = 0) {
    PC1600State st;
    st.mode1 = mode1;
    st.title = title;
    st.basicAreas = areas;
    st.ramEnd = 0xEB00;
    if (title != 0) {
        st.titleBase = 0x8000;
        st.titleStart = 0x80C5;
    }
    return st;
}
const PC1600State kStock = state(kStockAreas);
const PC1600State kSlot1First = state(kSlot1FirstAreas);
const PC1600State kSlot2First = state(kSlot2FirstAreas);

// The plan as every caller sees it: planLoad and its one sentence.
struct Plan {
    LoadError e = LoadError::None;
    std::string error;   // the sentence; empty when it holds or asks for an address
    bool needsAddress = false;
    uint32_t defaultAddr = 0;
    Cpu cpu = Cpu::Z80;
    Slot slot = Slot::S0;
    int bank = 0;
    uint32_t addr = 0, busAddr = 0;
    size_t len = 0;
};
Plan plan(Target t, const File& f, const PC1600State& st, LoadOptions o = {}) {
    o.target = t;
    const machinecode::LoadPlan lp = machinecode::planLoad(f, o, st);
    Plan p;
    p.e = lp.error;
    p.needsAddress = lp.error == LoadError::NeedsAddress;
    if (!p.needsAddress) p.error = machinecode::refusal(lp, f, o);
    p.defaultAddr = lp.defaultAddr;
    p.cpu = lp.cpu;
    p.slot = lp.slot;
    p.bank = lp.bank;
    p.addr = lp.addr;
    p.busAddr = lp.busAddr;
    p.len = lp.len;
    return p;
}
LoadOptions at(uint32_t address) {
    LoadOptions o;
    o.hasAddress = true;
    o.address = address;
    return o;
}
LoadOptions length(size_t n) {
    LoadOptions o;
    o.hasLength = true;
    o.length = n;
    return o;
}

bool bootPC1500(PC1500Machine& m) {
    if (!m.loadROMFile("roms/PC-1500_A04.ROM")) return false;
    m.reset();
    m.runCycles(static_cast<uint64_t>(1300000.0 * 2));
    waitIdle(m, static_cast<uint64_t>(1300000.0 * 5));
    return true;
}

// ── readFile ─────────────────────────────────────────────────────────────

TEST(test_read_ce158) {
    File f = machinecode::readFile(ce158File(kCode, 0x40C5, 0x40C7));
    CHECK(f.ok);
    CHECK(f.header == File::Header::CE158);
    CHECK(f.loadAddr == 0x40C5);
    CHECK(f.autorunAddr == 0x40C7);
    CHECK(f.payload == kCode);
}

TEST(test_read_ffff_autorun_is_none) {
    CHECK(machinecode::readFile(ce158File(kCode, 0x40C5, 0xFFFF)).autorunAddr == 0);
    CHECK(machinecode::readFile(pc1600File(kCode, 0xC0C5, 0xFFFF)).autorunAddr == 0);
}

TEST(test_read_pc1600) {
    File f = machinecode::readFile(pc1600File(kCode, 0xC0C5, 0));
    CHECK(f.ok);
    CHECK(f.header == File::Header::PC1600);
    CHECK(f.loadAddr == 0xC0C5);
    CHECK(f.autorunAddr == 0);
    CHECK(f.payload == kCode);
}

// `00` bytes before the header (e.g. from a serial capture) are skipped.
TEST(test_read_leading_noise) {
    auto bytes = ce158File(kCode, 0x40C5, 0x40C7);
    bytes.insert(bytes.begin(), {0x00, 0x00});
    File f = machinecode::readFile(bytes);
    CHECK(f.ok);
    CHECK(f.header == File::Header::CE158);
    CHECK(f.payload == kCode);
}

TEST(test_read_headerless) {
    File f = machinecode::readFile(kCode);
    CHECK(f.ok);
    CHECK(f.header == File::Header::None);
    CHECK(f.payload == kCode);
}

// ── AC-8: the refusals, row by row, in the table's order ────────────────

// Row 1: not machine code (a BASIC file with the hint), or a damaged header.
TEST(c1600_2_ac8_row1_not_code_basic_hint) {
    CHECK(plan(Target::PC1500, machinecode::readFile(ce158File(kCode, 0, 0, 0x40)), {}).error.find("Load BASIC Program") != std::string::npos);
    CHECK(plan(Target::PC1600, machinecode::readFile(pc1600File(kCode, 0, 0, 0x21)), kStock).error.find("Load BASIC Program") != std::string::npos);
    const std::string bas = "10 PRINT \"HI\"\n20 END\n";
    const Plan listing = plan(Target::PC1500, machinecode::readFile(std::vector<uint8_t>(bas.begin(), bas.end())), {});
    CHECK(listing.e == LoadError::BadFile && listing.error.find("BASIC listing") != std::string::npos &&
          listing.error.find("Load BASIC Program") != std::string::npos);
    const std::string notes = "Just some notes,\nnot a program.\n";
    CHECK(plan(Target::PC1500, machinecode::readFile(std::vector<uint8_t>(notes.begin(), notes.end())), {}).error.find("plain text") != std::string::npos);
    // A CE-158 header of an unknown type ('Z'): magic without a known header.
    CHECK(plan(Target::PC1500, machinecode::readFile(ce158File(kCode, 0x40C5, 0, 'Z')), {}).error.find("no complete header") != std::string::npos);
}

// Row 2: the header's length disagrees; both numbers; a length by hand takes it.
TEST(c1600_2_ac8_row2_length_mismatch_both_numbers) {
    auto longer = ce158File(kCode, 0x40C5, 0);
    longer.push_back(0xAA);
    const File f = machinecode::readFile(longer);
    CHECK(!f.ok && f.lengthMismatch && f.header == File::Header::CE158 && f.loadAddr == 0x40C5);
    CHECK(f.payload.size() == kCode.size() + 1);
    const Plan p = plan(Target::PC1500, f, {});
    CHECK(p.e == LoadError::BadFile);
    CHECK(p.error.find(std::to_string(kCode.size())) != std::string::npos && p.error.find(std::to_string(kCode.size() + 1)) != std::string::npos);
    CHECK(plan(Target::PC1500, f, {}, length(3)).e == LoadError::None);
    // Fewer bytes than the header says.
    auto shorter = pc1600File(kCode, 0xC0C5, 0);
    shorter.pop_back();
    const File t = machinecode::readFile(shorter);
    CHECK(!t.ok && t.lengthMismatch && t.payload.size() == kCode.size() - 1);
    CHECK(t.error.find(std::to_string(kCode.size() - 1)) != std::string::npos);
}

// Row 3: which machine or mode takes it.
TEST(c1600_2_ac8_row3_machine_or_mode) {
    const File ce158 = machinecode::readFile(ce158File(kCode, 0x40C5, 0));
    CHECK(plan(Target::PC1600, ce158, kStock).error.find("MODE 1") != std::string::npos);
    const Plan m1 = plan(Target::PC1600, ce158, state(kStockAreas, true));
    CHECK(m1.error.empty() && m1.cpu == Cpu::LH5803 && m1.busAddr == 0xC0C5 && m1.slot == Slot::S0);
    CHECK(plan(Target::PC1500, ce158, {}).error.empty());
    const File pc1600 = machinecode::readFile(pc1600File(kCode, 0xC0C5, 0));
    CHECK(plan(Target::PC1500, pc1600, {}).error.find("PC-1500") != std::string::npos);
    CHECK(plan(Target::PC1600, pc1600, kStock).error.empty());
    const Plan z = plan(Target::PC1600, pc1600, state(kStockAreas, true));
    CHECK(z.error.empty() && z.cpu == Cpu::Z80 && z.busAddr == 0xC0C5);
}

// Row 4: no code after the header, or a length of 0 by hand.
TEST(c1600_2_ac8_row4_no_code) {
    const File headerOnly = machinecode::readFile(ce158File({0x00}, 0x40C5, 0));
    CHECK(plan(Target::PC1500, headerOnly, {}, length(0)).error == "The file contains no code.");
    CHECK(plan(Target::PC1500, machinecode::readFile(kCode), {}, length(0)).e == LoadError::Empty);
}

// Row 5: headerless without an address; the PC-1600 proposes the start of
// the selected program area, in the MODE's address space.
TEST(c1600_2_ac8_row5_headerless_needs_address) {
    const File f = machinecode::readFile(kCode);
    const Plan p1500 = plan(Target::PC1500, f, {});
    CHECK(p1500.needsAddress && p1500.defaultAddr == 0);
    const Plan p1600 = plan(Target::PC1600, f, kStock);
    CHECK(p1600.needsAddress && p1600.defaultAddr == 0xC0C5 && p1600.cpu == Cpu::Z80);
    CHECK(plan(Target::PC1600, f, kSlot2First).defaultAddr == 0x80C5);
    CHECK(plan(Target::PC1600, f, PC1600State{}).defaultAddr == 0xC0C5);
    const Plan m1 = plan(Target::PC1600, f, state(kStockAreas, true));
    CHECK(m1.cpu == Cpu::LH5803 && m1.defaultAddr == 0x40C5);
    CHECK(plan(Target::PC1600, f, state(kStockAreas, false, 1)).defaultAddr == 0x80C5);   // TITLE "S1:"
    // The sentence names the proposal.
    LoadOptions o;
    o.target = Target::PC1600;
    const machinecode::LoadPlan lp = machinecode::planLoad(f, o, kStock);
    const std::string s = machinecode::refusal(lp, f, o);
    CHECK(s.find("address") != std::string::npos && s.find("&C0C5") != std::string::npos);
}

// Row 6: a length by hand that the file does not hold; both numbers.
TEST(c1600_2_ac8_row6_length_exceeds) {
    const Plan p = plan(Target::PC1500, machinecode::readFile(ce158File(kCode, 0x40C5, 0)), {}, length(99));
    CHECK(p.e == LoadError::LengthExceeds && p.error.find("99") != std::string::npos && p.error.find("5") != std::string::npos);
}

// Row 7: a PC-1600 header's bank: 0-3 only, &8000-&BFFF there, RAM in it.
TEST(c1600_2_ac8_row7_header_bank_and_range) {
    PC1600State ram = kSlot1First;
    ram.bankRamPages = {~uint64_t{0}, ~uint64_t{0}, ~uint64_t{0}, ~uint64_t{0}};
    CHECK(plan(Target::PC1600, machinecode::readFile(pc1600File(kCode, 0x0580C5, 0)), ram).error.find("banks 0-3") != std::string::npos);
    CHECK(plan(Target::PC1600, machinecode::readFile(pc1600File(kCode, 0x02C0C5, 0)), ram).error.find("&8000-&BFFF") != std::string::npos);
    CHECK(plan(Target::PC1600, machinecode::readFile(pc1600File(kCode, 0x01A000, 0)), kStock).error.find("no RAM") != std::string::npos);
    PC1600State partial = kSlot1First;
    partial.bankRamPages[3] = 1;  // page $80 only
    const Plan p = plan(Target::PC1600, machinecode::readFile(pc1600File(std::vector<uint8_t>(0x100, 0), 0x0380C5, 0)), partial);
    CHECK(p.error.find("bank 3") != std::string::npos && p.error.find("no RAM") != std::string::npos);
}

// Row 8: past &FFFF.
TEST(c1600_2_ac8_row8_past_ffff) {
    const Plan p = plan(Target::PC1500, machinecode::readFile(ce158File(kCode, 0xFFFE, 0)), {});
    CHECK(p.e == LoadError::PastEnd && p.error.find("runs past &FFFF") != std::string::npos && p.error.find("&FFFE") != std::string::npos);
    CHECK(plan(Target::PC1600, machinecode::readFile(kCode), kStock, at(0xFFFE)).e == LoadError::PastEnd);
}

// Row 9: LH5803 code outside &0000-&7FFF.
TEST(c1600_2_ac8_row9_lh5803_range) {
    LoadOptions o = at(0x7FFE);
    o.hasCpu = true;
    o.cpu = Cpu::LH5803;
    const Plan p = plan(Target::PC1600, machinecode::readFile(kCode), kSlot1First, o);
    CHECK(p.e == LoadError::LhRange && p.error.find("&0000-&7FFF") != std::string::npos);
}

// Row 10: ROM, or a module not part of the selected program area: where code can go.
TEST(c1600_2_ac8_row10_rom_or_module_where_instead) {
    const Plan rom = plan(Target::PC1600, machinecode::readFile(pc1600File(kCode, 0x7000, 0)), kSlot1First);
    CHECK(rom.e == LoadError::NoSlot && rom.error.find("ROM") != std::string::npos && rom.error.find("from &") != std::string::npos);
    const Plan module = plan(Target::PC1600, machinecode::readFile(pc1600File(kCode, 0x80C5, 0)), kStock);
    CHECK(module.e == LoadError::NoSlot && module.error.find("TITLE") != std::string::npos && module.error.find("&C0C5") != std::string::npos);
    // An 8 KB module's window starts at $A000.
    CHECK(!plan(Target::PC1600, machinecode::readFile(pc1600File(kCode, 0x80C5, 0)),
                state({{1, 0xA000, 0xBFFF, 0}, {0, 0xC000, 0xEFFF, 0}})).error.empty());
}

// A file that meets two rows is refused by the earlier one.
TEST(c1600_2_ac8_rows_checked_in_order) {
    // Row 2 before row 3: a PC-1600 file with a wrong length on a PC-1500.
    auto longer = pc1600File(kCode, 0xC0C5, 0);
    longer.push_back(0);
    CHECK(plan(Target::PC1500, machinecode::readFile(longer), {}).e == LoadError::BadFile);
    // Row 3 before row 4: a CE-158 file of no code on a PC-1600 in MODE 0.
    CHECK(plan(Target::PC1600, machinecode::readFile(ce158File(kCode, 0x40C5, 0)), kStock, length(0)).e == LoadError::HeaderMismatch);
    // Row 4 before row 5: headerless, no address, length 0.
    CHECK(plan(Target::PC1500, machinecode::readFile(kCode), {}, length(0)).e == LoadError::Empty);
    // Row 5 before row 6: headerless, no address, a length too long.
    CHECK(plan(Target::PC1500, machinecode::readFile(kCode), {}, length(99)).e == LoadError::NeedsAddress);
    // Row 8 before row 9: LH5803 code that also runs past &FFFF.
    LoadOptions lh = at(0xFFFE);
    lh.hasCpu = true;
    lh.cpu = Cpu::LH5803;
    CHECK(plan(Target::PC1600, machinecode::readFile(kCode), kStock, lh).e == LoadError::PastEnd);
    // Row 8 before row 10: in a module that is not selected, and past &FFFF.
    CHECK(plan(Target::PC1600, machinecode::readFile(std::vector<uint8_t>(0x8000, 0)), kStock, at(0x9000)).e == LoadError::PastEnd);
}

// The menu, a preset, the debugger and the harness plan with planCode: the
// same plan for the same file, request and machine.
TEST(c1600_2_ac8_same_plan_menu_preset_debugger_harness) {
    PC1600Machine m;
    SKIP_UNLESS(bootPC1600(m), "PC-1600 ROMs not found in roms/");
    sharp::PC1600LoaderMemory mem(m);
    sharp::loaders::CodeRequest r;
    r.address = 0xC100;
    const auto a = sharp::loaders::planCode(mem, kCode, r);
    LoadOptions o = at(0xC100);
    o.target = Target::PC1600;
    const machinecode::LoadPlan b = machinecode::planLoad(machinecode::readFile(kCode), o, pc1600LoadState(mem));
    CHECK(a.holds() && b.error == LoadError::None);
    CHECK(a.plan.busAddr == b.busAddr && a.plan.slot == b.slot && a.plan.bank == b.bank && a.plan.len == b.len);
}

// ── AC-23: where it goes ─────────────────────────────────────────────────

TEST(c1600_2_ac23_address_by_hand_else_header) {
    const File f = machinecode::readFile(pc1600File(kCode, 0xC0C5, 0));
    CHECK(plan(Target::PC1600, f, kStock).addr == 0xC0C5);
    CHECK(plan(Target::PC1600, f, kStock, at(0xC200)).addr == 0xC200);
    CHECK(plan(Target::PC1500, machinecode::readFile(kCode), {}, at(0x7C01)).addr == 0x7C01);
}

TEST(c1600_2_ac23_length_by_hand_else_payload) {
    const File f = machinecode::readFile(ce158File(kCode, 0x40C5, 0));
    CHECK(plan(Target::PC1500, f, {}).len == kCode.size());
    CHECK(plan(Target::PC1500, f, {}, length(3)).len == 3);
}

TEST(c1600_2_ac23_bank_from_header_or_area) {
    // Bank 0 is "no bank given": S0's first module run decides.
    const File noBank = machinecode::readFile(pc1600File(kCode, 0x0080C5, 0));
    Plan p = plan(Target::PC1600, noBank, kSlot2First);
    CHECK(p.error.empty() && p.slot == Slot::S2 && p.bank == 2);
    p = plan(Target::PC1600, noBank, kSlot1First);
    CHECK(p.error.empty() && p.slot == Slot::S1 && p.bank == 0);
    // A header bank 1-3 is honoured, whatever the program area is.
    PC1600State ram = kSlot1First;
    ram.bankRamPages = {~uint64_t{0}, ~uint64_t{0}, ~uint64_t{0}, ~uint64_t{0}};
    const File bank3 = machinecode::readFile(pc1600File(kCode, 0x0380C5, 0));
    p = plan(Target::PC1600, bank3, ram);
    CHECK(p.error.empty() && p.slot == Slot::S2 && p.bank == 3 && p.busAddr == 0x80C5);
    PC1600State stockRam = state(kStockAreas);
    stockRam.bankRamPages[1] = ~uint64_t{0};
    p = plan(Target::PC1600, machinecode::readFile(pc1600File(kCode, 0x01A000, 0)), stockRam);
    CHECK(p.error.empty() && p.slot == Slot::S1 && p.bank == 1 && p.busAddr == 0xA000);
    // An address by hand drops the header's bank: the area decides.
    p = plan(Target::PC1600, bank3, ram, at(0xC0C5));
    CHECK(p.e == LoadError::None && p.slot == Slot::S0 && p.bank == 0);
    // TITLE "S2:": the slot 2 program module, whatever S0 holds.
    p = plan(Target::PC1600, noBank, state(kSlot1FirstAreas, false, 2));
    CHECK(p.error.empty() && p.slot == Slot::S2 && p.bank == 2);
    // $BFFE + 5 bytes runs on into internal RAM, as BLOAD writes it.
    p = plan(Target::PC1600, machinecode::readFile(pc1600File(kCode, 0xBFFE, 0)), kSlot1First);
    CHECK(p.error.empty() && p.slot == Slot::S1);
}

TEST(c1600_2_ac23_headerless_no_bank) {
    const Plan p = plan(Target::PC1600, machinecode::readFile(kCode), kSlot2First, at(0x80C5));
    CHECK(p.error.empty() && p.slot == Slot::S2 && p.bank == 2);   // the area's bank, not one of the file
}

// ── AC-9: the processor ──────────────────────────────────────────────────

TEST(c1600_2_ac9_cpu_by_header_caller_mode) {
    CHECK(plan(Target::PC1600, machinecode::readFile(pc1600File(kCode, 0xC0C5, 0)), state(kStockAreas, true)).cpu == Cpu::Z80);
    CHECK(plan(Target::PC1600, machinecode::readFile(ce158File(kCode, 0x40C5, 0)), state(kStockAreas, true)).cpu == Cpu::LH5803);
    LoadOptions o = at(0x1000);
    o.hasCpu = true;
    o.cpu = Cpu::LH5803;
    const Plan p = plan(Target::PC1600, machinecode::readFile(kCode), kSlot1First, o);
    CHECK(p.e == LoadError::None && p.cpu == Cpu::LH5803 && p.addr == 0x1000 && p.busAddr == 0x9000 && p.slot == Slot::S1);
    CHECK(plan(Target::PC1600, machinecode::readFile(kCode), kStock, at(0xC0C5)).cpu == Cpu::Z80);
    const Plan m1 = plan(Target::PC1600, machinecode::readFile(kCode), state(kStockAreas, true), at(0x40C5));
    CHECK(m1.e == LoadError::None && m1.cpu == Cpu::LH5803 && m1.busAddr == 0xC0C5);
    CHECK(plan(Target::PC1600, machinecode::readFile(kCode), kStock, at(0x40C5)).e == LoadError::NoSlot);   // Z-80 &40C5 is ROM
}

TEST(c1600_2_ac9_addresses_in_cpu_space) {
    // In MODE 1 a refusal names the address as typed, in the LH5803 view.
    const Plan p = plan(Target::PC1600, machinecode::readFile(ce158File(kCode, 0x00C5, 0)), state(kStockAreas, true));
    CHECK(p.error.find("LH5803 &C5") != std::string::npos && p.error.find("&40C5") != std::string::npos);
    CHECK(p.error.find("&80C5") == std::string::npos);
    const std::string w = machinecode::pc1600WorkAreaWarning(0xF800, kCode.size(), Cpu::LH5803);
    CHECK(w.find("LH5803 &7000") != std::string::npos && w.find("7C01") != std::string::npos);
}

// ── AC-10: the write ─────────────────────────────────────────────────────

TEST(c1600_2_ac10_written_any_bank_switched_in) {
    PC1600Machine m;
    sharp::PC1600LoaderMemory mem(m);
    std::string err;
    CHECK(loadPC1600MachineCode(mem, 0, 0xC0C5, kCode.data(), kCode.size(), &err, 0));
    std::vector<uint8_t> ram(PC1600Machine::kInternalRamSize);
    m.debugCopyInternalRam(ram.data());
    CHECK(std::vector<uint8_t>(ram.begin() + 0xC5, ram.begin() + 0xC5 + 5) == kCode);
    // Slot 1: nothing attached -> refused; with 32 KB RAM -> image offset $00C5.
    CHECK(!loadPC1600MachineCode(mem, 1, 0x80C5, kCode.data(), kCode.size(), &err, 0));
    m.memory().attachSlot1Card(plainRamCard(0x8000));
    CHECK(loadPC1600MachineCode(mem, 1, 0x80C5, kCode.data(), kCode.size(), &err, 0));
    std::vector<uint8_t> image = m.debugSlotImage(1);
    if (image.size() >= 0xC5 + 5) CHECK(std::vector<uint8_t>(image.begin() + 0xC5, image.begin() + 0xC5 + 5) == kCode);
    // Bank 1 (not the one switched in): the module's upper 16 KB, image offset $40C5.
    CHECK(loadPC1600MachineCode(mem, 1, 0x80C5, kCode.data(), kCode.size(), &err, 1));
    image = m.debugSlotImage(1);
    if (image.size() >= 0x40C5 + 5) CHECK(std::vector<uint8_t>(image.begin() + 0x40C5, image.begin() + 0x40C5 + 5) == kCode);
    CHECK(!loadPC1600MachineCode(mem, 1, 0x80C5, kCode.data(), kCode.size(), &err, 2));   // bank 2 is slot 2's
    const PC1600State st = pc1600LoadState(mem);
    CHECK(st.bankRamPages[0] == ~uint64_t{0} && st.bankRamPages[1] == ~uint64_t{0});
    CHECK(st.bankRamPages[2] == 0 && st.bankRamPages[3] == 0);
    CHECK(!loadPC1600MachineCode(mem, 0, 0xBFFF, kCode.data(), kCode.size(), &err, 0));
}

TEST(c1600_2_ac10_not_ram_nothing_written_pc1500) {
    PC1500Machine m;
    SKIP_UNLESS(m.loadROMFile("roms/PC-1500_A04.ROM"), "roms/PC-1500_A04.ROM not found");
    sharp::PC1500LoaderMemory mem(m);
    std::string err;
    CHECK(loadPC1500MachineCode(mem, 0x40C5, kCode.data(), kCode.size(), &err));
    for (size_t i = 0; i < kCode.size(); i++) CHECK(m.memory().peek(static_cast<uint16_t>(0x40C5 + i)) == kCode[i]);
    // A range that starts in RAM and runs past its end: the first page from
    // $4000 that is not RAM is the edge.
    uint16_t edge = 0;
    for (uint32_t a = 0x4000; a < 0xC000; a += 0x100) {
        const uint8_t old = m.memory().peek(static_cast<uint16_t>(a));
        if (!m.memory().poke(static_cast<uint16_t>(a), old)) {
            edge = static_cast<uint16_t>(a);
            break;
        }
    }
    SKIP_UNLESS(edge != 0, "no RAM edge found");
    const uint16_t start = static_cast<uint16_t>(edge - 2);
    const uint8_t before0 = m.memory().peek(start), before1 = m.memory().peek(static_cast<uint16_t>(start + 1));
    CHECK(!loadPC1500MachineCode(mem, start, kCode.data(), kCode.size(), &err));
    CHECK(err.find("not RAM") != std::string::npos);
    CHECK(m.memory().peek(start) == before0 && m.memory().peek(static_cast<uint16_t>(start + 1)) == before1);   // OQ-2
    CHECK(!loadPC1500MachineCode(mem, 0xFFFE, kCode.data(), kCode.size(), &err));
}

TEST(c1600_2_ac10_not_ram_nothing_written_pc1600) {
    PC1600Machine m;
    m.memory().attachSlot1Card(plainRamCard(0x2000));   // 8 KB: RAM at $A000-$BFFF of bank 0 only
    sharp::PC1600LoaderMemory mem(m);
    std::string err;
    const std::vector<uint8_t> before = m.debugSlotImage(1);
    CHECK(!loadPC1600MachineCode(mem, 1, 0x9FFE, kCode.data(), kCode.size(), &err, 0));
    CHECK(m.debugSlotImage(1) == before);
}

TEST(c1600_2_ac10_crossing_bfff_both_checked) {
    PC1600Machine m;
    m.memory().attachSlot1Card(plainRamCard(0x8000));
    sharp::PC1600LoaderMemory mem(m);
    std::string err;
    CHECK(loadPC1600MachineCode(mem, 1, 0xBFFE, kCode.data(), kCode.size(), &err, 0));
    std::vector<uint8_t> image = m.debugSlotImage(1);
    if (image.size() >= 0x4000) CHECK(image[0x3FFE] == kCode[0] && image[0x3FFF] == kCode[1]);
    CHECK(m.debugPeek(0xC000) == kCode[2] && m.debugPeek(0xC002) == kCode[4]);
    // Without RAM in the window, the internal RAM part is not written either.
    PC1600Machine bare;
    sharp::PC1600LoaderMemory bareMem(bare);
    const uint8_t c000 = bare.debugPeek(0xC000);
    CHECK(!loadPC1600MachineCode(bareMem, 1, 0xBFFE, kCode.data(), kCode.size(), &err, 0));
    CHECK(bare.debugPeek(0xC000) == c000);
}

// ── AC-11, AC-12: the reply ──────────────────────────────────────────────

// The parts: the range, (PC-1600) the area and bank, NEW with its note, the
// starting command with its note, warnings.
TEST(c1600_2_ac11_parts_of_reply) {
    PC1600Machine m;
    SKIP_UNLESS(bootPC1600(m), "PC-1600 ROMs not found in roms/");
    auto r = loadertest::loadCode(m, pc1600File(kCode, 0xC0C5, 0));
    CHECK(std::holds_alternative<sharp::loaders::CodeLoaded>(r));
    if (!std::holds_alternative<sharp::loaders::CodeLoaded>(r)) return;
    const auto& c = std::get<sharp::loaders::CodeLoaded>(r);
    CHECK(c.range == "&C0C5-&C0C9" && c.area == "S0" && c.bank == 0);
    CHECK(!c.advice.newCommand.empty() && !c.advice.newNote.empty());
    CHECK(c.advice.callCommand == "CALL &C0C5" && !c.advice.callNote.empty());
    CHECK(c.warnings.empty());
    CHECK(m.debugPeek(0xC0C5) == kCode[0]);

    PC1500Machine p;
    SKIP_UNLESS(bootPC1500(p), "roms/PC-1500_A04.ROM not found");
    sharp::loaders::CodeRequest req;
    req.address = 0x7C01;
    auto q = loadertest::loadCode(p, kCode, req);
    CHECK(std::holds_alternative<sharp::loaders::CodeLoaded>(q));
    if (std::holds_alternative<sharp::loaders::CodeLoaded>(q)) {
        const auto& d = std::get<sharp::loaders::CodeLoaded>(q);
        CHECK(d.area.empty() && d.range == "&7C01-&7C05" && d.advice.callCommand == "CALL &7C01");
        CHECK(d.summary == "Loaded 5 bytes at &7C01–&7C05");
    }
}

// Row 1: PC-1500 inside BASIC's memory, NEW &e; (MODE 1 with S0 below).
TEST(c1600_2_ac11_new_pc1500_inside_basic) {
    auto a = machinecode::advice(Target::PC1500, Slot::S0, 0, 0x40C5, 0x20, 0, 0x4000, 0x5800, {}, Cpu::Z80);
    CHECK(a.newCommand == "NEW &40E5");
    // Inside the reserve: no NEW helps.
    a = machinecode::advice(Target::PC1500, Slot::S0, 0, 0x4010, 0x20, 0x4012, 0x4000, 0x5800, {}, Cpu::Z80);
    CHECK(a.newCommand.empty() && a.newNote.find("reserve") != std::string::npos);
    // MODE 1, S0: the PC-1500 way.
    const PC1600State mode1 = state(kStockAreas, true);
    a = machinecode::advice(Target::PC1600, Slot::S0, 0, 0x40C5, 0x40, 0, 0, 0, mode1, Cpu::LH5803);
    CHECK(a.newCommand == "NEW &4105");
    a = machinecode::advice(Target::PC1600, Slot::S0, 0, 0xC0C5, 0x40, 0, 0, 0, mode1, Cpu::Z80);
    CHECK(a.newCommand == "NEW &4105");
}

// Row 2: PC-1500 outside BASIC's memory: none needed.
TEST(c1600_2_ac11_new_pc1500_outside_none_needed) {
    const auto a = machinecode::advice(Target::PC1500, Slot::S0, 0, 0x7C01, 0x10, 0, 0x4000, 0x5800, {}, Cpu::Z80);
    CHECK(a.newCommand.empty() && a.newNote.find("no NEW is needed") != std::string::npos);
}

// Row 3: the selected S1 / S2: NEW "Sn:",&size.
TEST(c1600_2_ac11_new_selected_s1_s2) {
    const auto a = machinecode::advice(Target::PC1600, Slot::S1, 0, 0x80C5, 0x40, 0, 0, 0, state(kStockAreas, false, 1), Cpu::Z80);
    CHECK(a.newCommand == "NEW \"S1:\",&105");
}

// Row 4: MODE 0, the first part of S0: NEW "S0:",&size, a note on the bytes below.
TEST(c1600_2_ac11_new_s0_first_part_with_note) {
    auto a = machinecode::advice(Target::PC1600, Slot::S0, 0, 0xC0C5, 0x100, 0, 0, 0, kStock, Cpu::Z80);
    CHECK(a.newCommand == "NEW \"S0:\",&1C5" && a.newNote.find("Warning") == std::string::npos);
    a = machinecode::advice(Target::PC1600, Slot::S0, 0, 0xC200, 0x10, 0, 0, 0, kStock, Cpu::Z80);
    CHECK(a.newCommand == "NEW \"S0:\",&210" && a.newNote.find("lost to BASIC") != std::string::npos);
    a = machinecode::advice(Target::PC1600, Slot::S1, 0, 0x80C5, 0x40, 0x80D0, 0, 0, kSlot1First, Cpu::Z80);
    CHECK(a.newCommand == "NEW \"S0:\",&105");
    a = machinecode::advice(Target::PC1600, Slot::S2, 2, 0x80C5, 0x40, 0, 0, 0, kSlot2First, Cpu::Z80);
    CHECK(a.newCommand == "NEW \"S0:\",&105");
}

// Row 5: a later part of S0, or above the area in internal RAM: a warning.
TEST(c1600_2_ac11_new_later_part_or_above_warning) {
    auto a = machinecode::advice(Target::PC1600, Slot::S0, 0, 0xC0C5, 0x40, 0, 0, 0, kSlot1First, Cpu::Z80);
    CHECK(a.newCommand.empty() && a.newNote.find("overwrite") != std::string::npos);
    PC1600State low = kStock;
    low.basicAreas = {{0, 0xC000, 0xD000, 0}};
    a = machinecode::advice(Target::PC1600, Slot::S0, 0, 0xE000, 0x40, 0, 0, 0, low, Cpu::Z80);
    CHECK(a.newCommand.empty() && a.newNote.find("overwrite") != std::string::npos);
}

// Row 6: elsewhere: BASIC does not use this memory.
TEST(c1600_2_ac11_new_elsewhere_not_used) {
    const auto a = machinecode::advice(Target::PC1600, Slot::S2, 2, 0x80C5, 0x40, 0, 0, 0, kSlot1First, Cpu::Z80);
    CHECK(a.newCommand.empty() && a.newNote.find("doesn't use") != std::string::npos);
}

TEST(c1600_2_ac11_call_forms_call_callbank_xcall) {
    PC1600State ram = kSlot1First;
    CHECK(machinecode::advice(Target::PC1500, Slot::S0, 0, 0x40C5, 0x20, 0, 0x4000, 0x5800, {}, Cpu::Z80).callCommand == "CALL &40C5");
    CHECK(machinecode::advice(Target::PC1600, Slot::S0, 0, 0xC0C5, 0x40, 0, 0, 0, kStock, Cpu::Z80).callCommand == "CALL &C0C5");
    CHECK(machinecode::advice(Target::PC1600, Slot::S2, 3, 0x80C5, 0x40, 0, 0, 0, ram, Cpu::Z80).callCommand == "CALL #3,&80C5");
    CHECK(machinecode::advice(Target::PC1600, Slot::S1, 1, 0x80C5, 0x40, 0x0180D0, 0, 0, ram, Cpu::Z80).callCommand == "CALL #1,&80D0");
    CHECK(machinecode::advice(Target::PC1600, Slot::S0, 0, 0x40C5, 0x40, 0, 0, 0, state(kStockAreas, true), Cpu::LH5803).callCommand == "XCALL &40C5");
}

TEST(c1600_2_ac11_start_address_header_else_first_byte) {
    auto a = machinecode::advice(Target::PC1500, Slot::S0, 0, 0x4010, 0x20, 0x4012, 0x4000, 0x5800, {}, Cpu::Z80);
    CHECK(a.callCommand == "CALL &4012" && a.callNote.find("auto-run") != std::string::npos);
    a = machinecode::advice(Target::PC1500, Slot::S0, 0, 0x40C5, 0x20, 0, 0x4000, 0x5800, {}, Cpu::Z80);
    CHECK(a.callCommand == "CALL &40C5" && a.callNote.find("first byte") != std::string::npos);
}

TEST(c1600_2_ac12_work_area_warning_loaded) {
    PC1600Machine m;
    SKIP_UNLESS(bootPC1600(m), "PC-1600 ROMs not found in roms/");
    auto r = loadertest::loadCode(m, pc1600File(kCode, 0xFF3A, 0xFF3A));
    CHECK(std::holds_alternative<sharp::loaders::CodeLoaded>(r));
    if (!std::holds_alternative<sharp::loaders::CodeLoaded>(r)) return;
    const auto& c = std::get<sharp::loaders::CodeLoaded>(r);
    CHECK(c.warnings.size() == 1 && c.warnings[0].find("WAKE$") != std::string::npos);
    CHECK(m.debugPeek(0xFF3A) == kCode[0]);
    CHECK(machinecode::pc1600WorkAreaWarning(0xF800, kCode.size(), Cpu::Z80).find("work area") != std::string::npos);
    CHECK(machinecode::pc1600WorkAreaWarning(0xFF40, 0xC0, Cpu::Z80).find("CE-1F01A") != std::string::npos);
    CHECK(machinecode::pc1600WorkAreaWarning(0xEF00, 0x10, Cpu::Z80).empty());
}

// ── parseHexAddress ──────────────────────────────────────────────────────

TEST(test_parse_hex_address) {
    uint32_t v = 0;
    CHECK(machinecode::parseHexAddress("C0C5", &v) && v == 0xC0C5);
    CHECK(machinecode::parseHexAddress(" &c0c5 ", &v) && v == 0xC0C5);
    CHECK(machinecode::parseHexAddress("$40C5", &v) && v == 0x40C5);
    CHECK(machinecode::parseHexAddress("0x7C01", &v) && v == 0x7C01);
    CHECK(!machinecode::parseHexAddress("", &v));
    CHECK(!machinecode::parseHexAddress("&", &v));
    CHECK(!machinecode::parseHexAddress("12G4", &v));
    CHECK(!machinecode::parseHexAddress("10000", &v));
}

TEST(test_pc1600_basic_areas) {
    {
        PC1600Machine m;
        SKIP_UNLESS(bootPC1600(m), "PC-1600 ROMs not found in roms/");
        auto areas = pc1600BasicAreas(sharp::PC1600LoaderMemory(m));
        CHECK(areas.size() == 1);
        if (!areas.empty()) CHECK(areas[0].slot == 0 && areas[0].windowBase == 0xC000);
    }
    {
        PC1600Machine m;
        m.memory().attachSlot1Card(plainRamCard(0x8000));  // 32 KB RAM module in slot 1, folded into S0 at boot
        CHECK(bootPC1600(m));
        auto areas = pc1600BasicAreas(sharp::PC1600LoaderMemory(m));
        CHECK(areas.size() >= 2);
        if (areas.size() >= 2) {
            CHECK(areas.front().slot == 1 && areas.front().windowBase == 0x8000);
            CHECK(areas.front().bank == 0);
            CHECK(areas.back().slot == 0);
        }
    }
}

}  // namespace
