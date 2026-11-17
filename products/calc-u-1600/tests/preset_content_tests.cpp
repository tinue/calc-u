// C1600-4 (design §6, T1 to T7, T9): Calc-U-1600's keys and sections of the
// preset format on the library's reader and engine -- `program:`,
// `host-drive:`, `slot-N-rom:`, `bus-rom:`, `debug:` -- their errors, the
// cases of today's tests (AC-19), the upgraded presets (AC-15) and the
// bundle file (AC-17, AC-18). Read checks run the product's rules on
// presets::readPreset; loads run the product's harness entry in emulated
// time. ROM-gated cases SKIP without roms/.
#include "../app/SharpDebugBlock.hpp"
#include "../core/Connector/ExpansionCard.hpp"
#include "SharpTestSupport.hpp"
#include "../core/Machine/SharpCatalogue.hpp"
#include "../core/Machine/SharpProductKinds.hpp"
#include "HarnessPreset.hpp"
#include "PresetReadRig.hpp"
#include "TestHarness.hpp"
#include "../app/tests/TestSettings.hpp"
#include "presets/BundleFile.hpp"
#include "presets/Engine.hpp"
#include "formats/Flow.hpp"
#include "TestRoms.hpp"

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;
using presettest::HarnessPreset;
using presettest::PresetReadRig;

namespace {

bool roms() { return fullRomSetPresent(); }

bool has(const std::string& text, const std::string& part) {
    const bool found = text.find(part) != std::string::npos;
    if (!found) std::fprintf(stderr, "  \"%s\" not in \"%s\"\n", part.c_str(), text.c_str());
    return found;
}

// A PC-1600 machine-code file with its header (C1600-2 AC-2): MACHINE,
// the declared length, the load and auto-run addresses.
std::string pc1600Code(const std::vector<std::uint8_t>& payload, std::uint32_t load, std::uint32_t autorun) {
    std::string f(16, '\0');
    f[0] = char(0xFF);
    f[1] = char(0x10);
    f[4] = char(0x10);
    const std::uint32_t n = static_cast<std::uint32_t>(payload.size());
    for (int i = 0; i < 3; ++i) {
        f[5 + i] = char((n >> (8 * i)) & 0xFF);
        f[8 + i] = char((load >> (8 * i)) & 0xFF);
        f[11 + i] = char((autorun >> (8 * i)) & 0xFF);
    }
    f[0x0F] = char(0x0F);
    for (std::uint8_t b : payload) f.push_back(char(b));
    return f;
}

const std::string kPc1600 = "format-version: 2\nmodel: PC-1600\n";
const std::string kPc1500a = "format-version: 2\nmodel: PC-1500A\n";

std::uint8_t peekZ80(HarnessPreset& h, std::uint32_t address) {
    char a[16];
    std::snprintf(a, sizeof a, "&%04X", address);
    const std::string r = h.line(std::string("peek SC7852 memory ") + a + " 1");
    const std::size_t at = r.find(": ");
    return at == std::string::npos ? 0 : static_cast<std::uint8_t>(std::stoul(r.substr(at + 2), nullptr, 16));
}

} // namespace

// ── T1: `program:` read (AC-1, AC-2, AC-5, EC-1, EC-2, EC-5) ────────────

TEST(c1600_4_ac1_fields_once_one_of_file_text) {
    PresetReadRig r;
    r.write("p.bas", "10 PRINT 1\n");
    CHECK(r.ok(kPc1600 + "program:\n  file: p.bas\n"));
    CHECK(r.ok(kPc1600 + "program:\n  text: |\n    10 PRINT 1\n"));
    CHECK(has(r.error(kPc1600 + "program:\n  file: p.bas\n  text: |\n    10 A=1\n"), "exactly one of file: and text:"));
    CHECK(has(r.error(kPc1600 + "program:\n  typed: true\n"), "exactly one of file: and text:"));
    CHECK(has(r.error(kPc1600 + "program:\n  file: p.bas\n  file: p.bas\n"), "Duplicate key 'file'"));
}

TEST(c1600_4_ac1_text_not_with_typed_address_length) {
    PresetReadRig r;
    for (const char* f : {"typed: true", "address: &C000", "length: 4"})
        CHECK(has(r.error(kPc1600 + "program:\n  " + f + "\n  text: |\n    10 A=1\n"), "go with file:"));
}

TEST(c1600_4_ac1_sections_in_file_order_any_number) {
    PresetReadRig r;
    r.write("a.bas", "10 PRINT 1\n");
    const auto plan = r.read(kPc1600 + "keys:\n  - key: mode\nprogram:\n  file: a.bas\nkeys:\n  - key: mode\n"
                                       "program:\n  text: |\n    20 B=2\n");
    CHECK(std::holds_alternative<calcu::presets::LoadPlan>(plan));
    if (const auto* p = std::get_if<calcu::presets::LoadPlan>(&plan)) {
        CHECK_EQ(p->items.size(), std::size_t{4});
        CHECK(p->items.size() == 4 && p->items[1].item->logText().find("program: a.bas") == 0 &&
              p->items[3].item->logText() == "program: text");
    }
}

TEST(c1600_4_ac2_kind_told_at_read_field_it_does_not_take_refused) {
    PresetReadRig r;
    r.write("p.bas", "10 PRINT 1\n");
    r.write("c.bin", pc1600Code({0xC9}, 0xD000, 0));
    CHECK(r.ok(kPc1600 + "program:\n  file: p.bas\n  typed: true\n"));
    CHECK(has(r.error(kPc1600 + "program:\n  file: p.bas\n  address: &C000\n"), "go with machine code; p.bas is BASIC"));
    CHECK(has(r.error(kPc1600 + "program:\n  file: c.bin\n  typed: true\n"), "c.bin is machine code"));
    CHECK(r.ok(kPc1600 + "program:\n  file: c.bin\n  length: 1\n"));
}

TEST(c1600_4_ac2_unreadable_empty_not_a_program_refused) {
    PresetReadRig r;
    r.write("empty.bas", "");
    r.write("note.txt", "Just some words here, nothing to run.\n");
    CHECK(has(r.error(kPc1600 + "program:\n  file: none.bas\n"), "cannot be read"));
    CHECK(has(r.error(kPc1600 + "program:\n  file: empty.bas\n"), "empty.bas is empty"));
    CHECK(has(r.error(kPc1600 + "program:\n  file: note.txt\n"), "not a program"));
}

TEST(c1600_4_ac2_tape_definition_is_not_a_program_file) {
    PresetReadRig r;
    r.write("t.cassette.yaml", "format: tape\nformat-version: 1\ncassette-name: T\n");
    CHECK(has(r.error(kPc1600 + "program:\n  file: t.cassette.yaml\n"), "not a program"));
}

TEST(c1600_4_ac5_blank_and_hash_lines_left_out_space_hash_typed) {
    SKIP_UNLESS(roms(), "no ROMs in roms/");
    HarnessPreset h;
    CHECK(h.ok());
    // PC-1500A: in program mode after its cold start (C-7, AC-4).
    CHECK(h.loadIdle(kPc1500a + "keys:\n  - key: cl\n  - type: NEW0\n"
                                "program:\n  text: |\n    # a comment line\n\n    10 A$=\"X # Y\"\n\n    # another\n"
                                "    20 PRINT A$\n",
                     "t.pc1500a"));
    CHECK(h.line("type LIST").rfind("done", 0) == 0);
    h.line("wait");
    CHECK(has(h.line("lcd"), "10:A$=\"X # Y\""));
}

TEST(c1600_4_ec1_text_without_lines_refused) {
    PresetReadRig r;
    CHECK(has(r.error(kPc1600 + "program:\n  text: |\n"), "text: needs the lines"));
    CHECK(has(r.error(kPc1600 + "program:\n  text: |\n    # only a comment\n"), "text: needs the lines"));
}

TEST(c1600_4_ec2_no_block_or_unknown_field_refused) {
    PresetReadRig r;
    CHECK(has(r.error(kPc1600 + "program:\n"), "program: is a block"));
    CHECK(has(r.error(kPc1600 + "program:\n  slot: 1\n  text: |\n    10 A=1\n"), "program: has no field slot"));
}

TEST(c1600_4_ec5_relative_path_without_folder_refused) {
    PresetReadRig r;
    const auto plan = r.readContent(kPc1600 + "program:\n  file: hanoi.bas\n");
    const auto* e = std::get_if<calcu::presets::ReadError>(&plan);
    CHECK(e && has(e->message, "handed over without its folder"));
    const auto hd = r.readContent(kPc1600 + "host-drive: { folder: shared }\n");
    const auto* e2 = std::get_if<calcu::presets::ReadError>(&hd);
    CHECK(e2 && has(e2->message, "handed over without its folder"));
}

// ── T2: `program:` carried out (AC-3, AC-4, EC-3) ───────────────────────

TEST(c1600_4_ac3_basic_listing_tokenized_loaded) {
    SKIP_UNLESS(roms(), "no ROMs in roms/");
    HarnessPreset h;
    std::ifstream in("tests/fixtures/basic/lissajou-1600.bbin", std::ios::binary);
    h.write("l.bbin", std::string(std::istreambuf_iterator<char>(in), {}));
    h.write("p.bas", "10 PRINT 4711\n");
    CHECK(h.loadIdle(kPc1600 + "program:\n  file: p.bas\nkeys:\n  - type: RUN\n"));
    CHECK(has(h.line("lcd"), "4711"));
    CHECK(h.loadIdle(kPc1600 + "program:\n  file: l.bbin\nkeys:\n  - key: mode\n  - type: LIST\n"));
    CHECK(has(h.line("lcd"), "10"));
}

TEST(c1600_4_ac3_typed_true_typed_in) {
    SKIP_UNLESS(roms(), "no ROMs in roms/");
    HarnessPreset h;
    h.write("p.bas", "10 PRINT 4712\n");
    CHECK(h.loadIdle(kPc1600 + "keys:\n  - key: mode\nprogram:\n  file: p.bas\n  typed: true\nkeys:\n  - key: mode\n  - type: RUN\n"));
    CHECK(has(h.line("lcd"), "4712"));
}

TEST(c1600_4_ac3_machine_code_at_address_and_started) {
    SKIP_UNLESS(roms(), "no ROMs in roms/");
    HarnessPreset h;
    // LD A,&5A; LD (&D3F0),A; RET -- started by its header's auto-run address.
    h.write("c.bin", pc1600Code({0x3E, 0x5A, 0x32, 0xF0, 0xD3, 0xC9}, 0xD300, 0xD300));
    CHECK(h.loadIdle(kPc1600 + "program:\n  file: c.bin\nkeys:\n  - key: cl\n"));
    CHECK_EQ(int(peekZ80(h, 0xD300)), 0x3E);
    CHECK_EQ(int(peekZ80(h, 0xD3F0)), 0x5A);   // it ran
    // Headerless at `address:` (not started).
    h.write("raw.bin", std::string("\x11\x22\x33", 3));
    CHECK(h.loadIdle(kPc1600 + "program:\n  file: raw.bin\n  address: &D100\n"));
    CHECK_EQ(int(peekZ80(h, 0xD101)), 0x22);
}

TEST(c1600_4_ac3_loader_failure_fails_preset_with_reason) {
    SKIP_UNLESS(roms(), "no ROMs in roms/");
    HarnessPreset h;
    h.write("raw.bin", std::string("\x11\x22\x33", 3));   // headerless, no address
    const std::string r = h.load(kPc1600 + "program:\n  file: raw.bin\n");
    CHECK(has(r, "refused: "));
    CHECK(has(r, "line 3: "));
    CHECK(has(r, "raw.bin: "));
}

TEST(c1600_4_ac4_text_typed_in_program_mode) {
    SKIP_UNLESS(roms(), "no ROMs in roms/");
    HarnessPreset h;
    CHECK(h.loadIdle(kPc1600 + "keys:\n  - key: mode\nprogram:\n  text: |\n    10 PRINT 4713\nkeys:\n  - key: mode\n  - type: RUN\n"));
    CHECK(has(h.line("lcd"), "4713"));
}

TEST(c1600_4_ac4_not_in_program_mode_fails_naming_lines) {
    SKIP_UNLESS(roms(), "no ROMs in roms/");
    HarnessPreset h;
    // The PC-1600 starts in RUN mode (C-7): the lines are not stored.
    const std::string r = h.load(kPc1600 + "program:\n  text: |\n    10 PRINT 1\n");
    CHECK(has(r, "refused: "));
    CHECK(has(r, "program line was stored"));
}

TEST(c1600_4_ec3_second_section_after_first_finished) {
    SKIP_UNLESS(roms(), "no ROMs in roms/");
    HarnessPreset h;
    // The first section's code runs a while (a loop of 65536 rounds) and
    // stores a mark; the second section reads it.
    h.write("a.bin", pc1600Code({0x01, 0x00, 0x00, 0x0B, 0x78, 0xB1, 0x20, 0xFB, 0x3E, 0x77, 0x32, 0xF0, 0xD3, 0xC9},
                                0xD300, 0xD300));
    h.write("b.bas", "10 PRINT PEEK &D3F0\n");
    CHECK(h.loadIdle(kPc1600 + "program:\n  file: a.bin\nprogram:\n  file: b.bas\nkeys:\n  - type: RUN\n"));
    CHECK(has(h.line("lcd"), "119"));
}

TEST(c1600_4_ec3_code_not_returned_fails_at_timeout) {
    SKIP_UNLESS(roms(), "no ROMs in roms/");
    HarnessPreset h;
    // DI; JR $ -- with the interrupts off: the timer interrupt's key scan
    // would count as key readiness (C1600-1 AC-21).
    h.write("loop.bin", pc1600Code({0xF3, 0x18, 0xFE}, 0xD300, 0xD300));
    const std::string r = h.load(kPc1600 + "timeout: 1\nprogram:\n  file: loop.bin\nkeys:\n  - key: cl\n");
    CHECK(has(r, "refused: "));
    CHECK(has(r, "timeout:"));
}

// ── T3: `host-drive:` (AC-6, AC-7) ──────────────────────────────────────

TEST(c1600_4_ac6_folder_shown_as_s3) {
    SKIP_UNLESS(roms(), "no ROMs in roms/");
    HarnessPreset h;
    fs::create_directories(h.folder.path() / "shared");
    h.write("shared/HELLO.BAS", "10 PRINT 1\n");
    CHECK(h.loadIdle(kPc1600 + "host-drive: { folder: shared }\nkeys:\n  - type: FILES \"S3:\"\n"));
    CHECK(has(h.line("lcd"), "HELLO"));
    CHECK(has(h.line("capabilities"), "place host-drive host-drive"));
}

TEST(c1600_4_ac6_missing_folder_or_not_pc1600_refused_at_read) {
    PresetReadRig r;
    CHECK(has(r.error(kPc1600 + "host-drive: { folder: nowhere }\n"), "does not exist"));
    fs::create_directories(r.folder.path() / "shared");
    CHECK(r.ok(kPc1600 + "host-drive: { folder: shared }\n"));
    CHECK(has(r.error(kPc1500a + "host-drive: { folder: shared }\n", "t.pc1500a"), "not available on the PC-1500A"));
    CHECK(has(r.error(kPc1600 + "host-drive: shared\n"), "host-drive: is { folder: <path> }"));
}

TEST(c1600_4_ac7_refused_where_frontend_mounts_no_folder) {
    PresetReadRig r(cu1600::SharpKeywordSetup{[] { return false; }});
    fs::create_directories(r.folder.path() / "shared");
    CHECK(has(r.error(kPc1600 + "host-drive: { folder: shared }\n"), "The host drive is not available in this frontend."));
}

// ── T4: ROM images, bus ROMs (AC-8 to AC-10, EC-6) ──────────────────────

TEST(c1600_4_ac8_slot_rom_bytes_read_at_every_load) {
    SKIP_UNLESS(roms(), "no ROMs in roms/");
    HarnessPreset h;
    const std::string preset = "format-version: 2\nmodel: PC-1500\nslot-1: { name: CE-502B }\nslot-1-rom: { file: build/rom.bin }\n";
    fs::create_directories(h.folder.path() / "build");
    for (const int first : {0x55, 0x66}) {   // rebuilt between the loads
        std::string bytes(0x4000, '\0');
        bytes[0] = char(first);
        h.write("build/rom.bin", bytes);
        CHECK(h.loadIdle(preset, "t.pc1500"));
        calcu::machine::Machine* m = h.machine();
        const ExpansionCard* card = m ? sharptest::moduleIn(*m, 1) : nullptr;
        CHECK(card && !card->debugImage().empty() && card->debugImage()[0] == first);
    }
}

TEST(c1600_4_ac8_no_module_twice_unreadable_wrong_size_refused) {
    SKIP_UNLESS(roms(), "no ROMs in roms/");
    PresetReadRig r;
    if (auto set = testRomSet()) sharp::addModuleDefinitions(r.catalogue, *set);
    r.write("rom.bin", std::string(0x4000, '\0'));
    r.write("small.bin", std::string(0x800, '\0'));
    const std::string head = "format-version: 2\nmodel: PC-1500\n";
    CHECK(r.ok(head + "slot-1: { name: CE-502B }\nslot-1-rom: { file: rom.bin }\n", "t.pc1500"));
    CHECK(has(r.error(head + "slot-1-rom: { file: rom.bin }\n", "t.pc1500"), "slot-1-rom: needs a module in slot-1"));
    CHECK(has(r.error(head + "slot-1: { name: CE-502B }\nslot-1-rom: { file: rom.bin }\nslot-1-rom: { file: rom.bin }\n", "t.pc1500"),
              "given twice"));
    CHECK(has(r.error(head + "slot-1: { name: CE-502B }\nslot-1-rom: { file: none.bin }\n", "t.pc1500"), "cannot be read"));
    CHECK(has(r.error(head + "slot-1: { name: CE-502B }\nslot-1-rom: { file: small.bin }\n", "t.pc1500"),
              "2048 bytes, the module's ROM is 16384 bytes"));
    CHECK(has(r.error(head + "slot-1: { name: CE-155 }\nslot-1-rom: { file: rom.bin }\n", "t.pc1500"), "not a ROM module"));
    CHECK(has(r.error(head + "slot-2-rom: { file: rom.bin }\n", "t.pc1500"), "not available on the PC-1500"));
}

TEST(c1600_4_ac9_forms_and_ranges) {
    PresetReadRig r;
    r.write("b.bin", std::string(0x100, '\x5A'));
    r.write("big.bin", std::string(0x4001, '\0'));
    CHECK(r.ok(kPc1600 + "bus-rom:\n  - file: b.bin\n    bank: 7\n"));
    CHECK(r.ok(kPc1600 + "bus-rom:\n  - file: b.bin\n    address: &8000\n    me: 1\n    pv: 0\n    pu: 1\n"));
    CHECK(r.ok(kPc1600 + "bus-rom:\n  - { file: b.bin, bank: 4 }\n  - { file: b.bin, address: 0xFF00 }\n"));
    CHECK(has(r.error(kPc1600 + "bus-rom:\n  - file: b.bin\n    bank: 3\n"), "bus-rom: bank: is a bank from 4 to 7"));
    CHECK(has(r.error(kPc1600 + "bus-rom:\n  - file: b.bin\n    address: &10000\n"), "bus-rom: address:"));
    CHECK(has(r.error(kPc1600 + "bus-rom:\n  - file: b.bin\n    bank: 7\n    me: 1\n"), "go with address:"));
    CHECK(has(r.error(kPc1600 + "bus-rom:\n  - file: b.bin\n"), "give either bank:"));
    CHECK(has(r.error(kPc1600 + "bus-rom:\n  - file: b.bin\n    address: &8000\n    pv: 2\n"), "pv: is 0 or 1"));
    CHECK(has(r.error(kPc1600 + "bus-rom:\n  - file: big.bin\n    bank: 5\n"), "a bank holds 1 to 16384"));
    CHECK(has(r.error(kPc1600 + "bus-rom:\n  - file: b.bin\n    address: &FFF0\n"), "the room from its address"));
    CHECK(has(r.error(kPc1600 + "bus-rom:\n  - file: b.bin\n    bank: 7\n    colour: red\n"), "has no key colour"));
    CHECK(has(r.error(kPc1600 + "bus-rom: b.bin\n"), "bus-rom: is a list"));
}

TEST(c1600_4_ac9_twice_refused) {
    PresetReadRig r;
    r.write("b.bin", std::string(0x10, '\0'));
    CHECK(has(r.error(kPc1600 + "bus-rom:\n  - { file: b.bin, bank: 7 }\nbus-rom:\n  - { file: b.bin, bank: 6 }\n"),
              "bus-rom: is given twice"));
}

// LIB-6 D-33 (Part D simplification P-6): the header keys' peripherals come
// after the places in the machine plan, the host drive before the bus ROMs
// whatever their order in the file, the bus ROMs in list order; a slot ROM
// is the content of its module's request.
TEST(c1600_4_ac10_header_peripherals_in_plan_order) {
    PresetReadRig r;
    r.write("shared/x.txt", "");
    r.write("a.bin", std::string(0x10, '\x01'));
    r.write("b.bin", std::string(0x10, '\x02'));
    const auto plan = r.read(kPc1600 + "bus-rom:\n  - { file: a.bin, bank: 7 }\n  - { file: b.bin, bank: 6 }\n"
                                       "host-drive: { folder: shared }\nprinter: CE-1600P\n");
    const auto* p = std::get_if<calcu::presets::LoadPlan>(&plan);
    CHECK(p != nullptr);
    if (!p) return;
    const calcu::startup::MachinePlan m = calcu::presets::machinePlan(*p);
    std::vector<std::string> order;
    for (const auto& q : m.peripherals) order.push_back(calcu::machine::placeText(q.place) + "=" + q.peripheral.value);
    CHECK(order == (std::vector<std::string>{"printer=CE-1600P", "host-drive=" + std::string(sharp::kHostDrive),
                                             "bus-rom=" + std::string(sharp::kBusRom), "bus-rom=" + std::string(sharp::kBusRom)}));
    if (m.peripherals.size() == 4) {
        const auto* first = dynamic_cast<const sharp::BusRomRequest*>(m.peripherals[2].content.get());
        CHECK(first && first->bank == 7);
    }
}

TEST(c1600_4_ac10_later_rom_found_first_and_before_peripheral) {
    SKIP_UNLESS(roms(), "no ROMs in roms/");
    HarnessPreset h;
    // PC-1600 with the host drive, whose ROM is the system bus's bank 7,
    // and two bus ROMs in bank 7: the later one is found first, and before
    // the host drive's (C1600-4 AC-10).
    fs::create_directories(h.folder.path() / "shared");
    h.write("first.bin", std::string(0x10, '\x11'));
    h.write("second.bin", std::string(0x10, '\x22'));
    CHECK(h.loadIdle(kPc1600 + "host-drive: { folder: shared }\nbus-rom:\n  - { file: first.bin, bank: 7 }\n"
                               "  - { file: second.bin, bank: 7 }\n"));
    CHECK(has(h.line("peek SC7852 memory &4000 1 bank=7"), "0x22"));   // page B
}

TEST(c1600_4_ac10_listed_in_plugged_no_control) {
    SKIP_UNLESS(roms(), "no ROMs in roms/");
    HarnessPreset h;
    h.write("b.bin", std::string(0x10, '\x11'));
    CHECK(h.loadIdle(kPc1600 + "bus-rom:\n  - { file: b.bin, bank: 7 }\n  - { file: b.bin, bank: 6 }\n"));
    const std::string caps = h.line("capabilities");
    CHECK(has(caps, "place bus-rom bus-rom"));
    CHECK(caps.find("peripheral:bus-rom") == std::string::npos);   // no capability, no control
    CHECK(caps.find("bus-rom/controls") == std::string::npos);
}

TEST(c1600_4_ac10_attach_command_refused) {
    SKIP_UNLESS(roms(), "no ROMs in roms/");
    HarnessPreset h;
    CHECK(has(h.line("attach bus-rom bus-rom"), "A bus ROM is plugged in by a preset only."));
}

TEST(c1600_4_ec6_bank_on_pc1500_refused) {
    PresetReadRig r;
    r.write("b.bin", std::string(0x10, '\0'));
    CHECK(has(r.error(kPc1500a + "bus-rom:\n  - { file: b.bin, bank: 7 }\n", "t.pc1500a"), "takes address:, not bank:"));
}

// ── T5: `debug:` (AC-11, AC-12, AC-20, EC-4) ────────────────────────────

TEST(c1600_4_ac11_keys_values_paths_from_preset_folder) {
    PresetReadRig r;
    const auto plan = r.read(kPc1600 +
                             "debug:\n  reset: allReset\n  stopOnEntry: true\n  command: CALL &C0C5\n"
                             "  program:\n    bin: hello.bin\n    listing: hello.lst\n    entry: START\n    after: call\n"
                             "    address: &C0C5\n    bank: 1\n"
                             "  listings:\n    - rom/b7.lst\n    - { path: rom/b6.lst, cpu: z80, bank: 6 }\n"
                             "  symbols:\n    - x.sym\n");
    const auto* p = std::get_if<calcu::presets::LoadPlan>(&plan);
    CHECK(p != nullptr);
    if (!p) return;
    const auto it = p->productHeader.find("debug");
    CHECK(it != p->productHeader.end());
    const auto* block = dynamic_cast<const cu1600::DebugBlock*>(it->second.get());
    CHECK(block && block->node.find("program") && block->node.find("program")->find("bin"));
    if (!block) return;
    const std::string bin = calcu::formats::scalarText(*block->node.find("program")->find("bin"));
    CHECK(bin == (r.folder.path() / "hello.bin").generic_string());
    const std::string first = calcu::formats::scalarText(block->node.find("listings")->seq[0]);
    CHECK(first == (r.folder.path() / "rom/b7.lst").generic_string());
}

TEST(c1600_4_ac11_unknown_key_wrong_value_refused) {
    PresetReadRig r;
    CHECK(has(r.error(kPc1600 + "debug:\n  colour: red\n"), "debug: unknown key colour"));
    CHECK(has(r.error(kPc1600 + "debug:\n  reset: sometimes\n"), "debug: reset is one of none, reset, allReset"));
    CHECK(has(r.error(kPc1600 + "debug:\n  stopOnEntry: \"true\"\n"), "true or false, without quotes"));
    CHECK(has(r.error(kPc1600 + "debug:\n  program:\n    listing: a.lst\n"), "debug: program: needs at least bin"));
    CHECK(has(r.error(kPc1600 + "debug:\n  program:\n    bin: a.bin\n    bank: 9\n"), "debug: bank is a number from 0 to 7"));
    CHECK(has(r.error(kPc1600 + "debug:\n  listings: a.lst\n"), "debug: listings: expects a list"));
    CHECK(has(r.error(kPc1600 + "debug: yes\n"), "debug: takes a block of settings"));
}

TEST(c1600_4_ac12_load_does_nothing_with_block) {
    SKIP_UNLESS(roms(), "no ROMs in roms/");
    HarnessPreset h;
    CHECK(h.loadIdle(kPc1600 + "debug:\n  command: CALL &C0C5\n  stopOnEntry: true\n"));
    CHECK(h.line("lcd").find("CALL") == std::string::npos);   // the command is not typed
    const std::uint64_t before = h.machine() ? h.machine()->tickCount() : 0;
    h.line("wait 100ms");
    CHECK(h.machine() && h.machine()->tickCount() > before);   // not stopped on entry
}

TEST(c1600_4_ac12_debug_block_of_for_the_debugger) {
    PresetReadRig r;
    const fs::path p = r.write("proj.pc1600", kPc1600 + "debug:\n  program:\n    bin: a.bin\n");
    const auto b = cu1600::debugBlockOf(p.generic_string(), r.setup(), nullptr);
    const auto* block = std::get_if<std::shared_ptr<const cu1600::DebugBlock>>(&b);
    CHECK(block && *block && (*block)->node.find("program"));
    const fs::path none = r.write("plain.pc1600", kPc1600);
    const auto n = cu1600::debugBlockOf(none.generic_string(), r.setup(), nullptr);
    const auto* empty = std::get_if<std::shared_ptr<const cu1600::DebugBlock>>(&n);
    CHECK(empty && !*empty);
    const fs::path bad = r.write("bad.pc1600", kPc1600 + "debug:\n  colour: red\n");
    CHECK(std::holds_alternative<std::string>(cu1600::debugBlockOf(bad.generic_string(), r.setup(), nullptr)));
}

TEST(c1600_4_ac20_blocks_by_the_file_syntax) {
    PresetReadRig r;
    r.write("b.bin", std::string(0x10, '\0'));
    // Every block form of today's tests: block maps, flow maps, flow lists,
    // comments, quoted text, hex numbers in each notation.
    CHECK(r.ok(kPc1600 + "debug:   # the project\n  listings: [a.lst, \"b c.lst\"]\n  symbols:\n    - { path: s.sym, cpu: lh5803, me: 1 }\n"
                         "  program: { bin: x.bin, address: 0xC0C5, entry: $C0C5 }\n"
                         "bus-rom:\n  - file: b.bin   # comment\n    address: $8000\n"));
}

TEST(c1600_4_ec4_mistake_refused_without_debugger) {
    SKIP_UNLESS(roms(), "no ROMs in roms/");
    HarnessPreset h;
    CHECK(has(h.load(kPc1600 + "debug:\n  after: later\n"), "refused: "));
}

// ── T6: errors (AC-14) and today's cases (AC-19) ────────────────────────

TEST(c1600_4_ac14_line_key_and_expected_debug_and_bus_rom_prefixed) {
    PresetReadRig r;
    r.write("b.bin", std::string(0x10, '\0'));
    const std::string a = r.error(kPc1600 + "\n\ndebug:\n  reset: sometimes\n");
    CHECK(has(a, ", line 6: debug: reset is one of"));
    const std::string b = r.error(kPc1600 + "bus-rom:\n  - file: b.bin\n    bank: 9\n");
    CHECK(has(b, ", line 5: bus-rom: bank: is a bank from 4 to 7."));
    const std::string c = r.error(kPc1600 + "program:\n  address: x\n  file: b.bin\n");
    CHECK(has(c, ", line 4: program: address: is a number"));
}

TEST(c1600_4_ac19_test_slot_rom_keys) {
    PresetReadRig r;
    CHECK(has(r.error(kPc1600 + "slot-1-rom: build/x.bin\n"), "slot-1-rom: is { file: <path> }"));
    CHECK(has(r.error(kPc1600 + "slot-1-rom: { folder: x }\n"), "slot-1-rom: is { file: <path> }"));
}

TEST(c1600_4_ac19_test_preset_parser_program_address_forms) {
    PresetReadRig r;
    r.write("c.bin", std::string(4, '\x11'));
    for (const char* a : {"&C000", "0xC000", "$C000", "49152"}) CHECK(r.ok(kPc1600 + "program:\n  file: c.bin\n  address: " + a + "\n"));
    CHECK(has(r.error(kPc1600 + "program:\n  file: c.bin\n  address: 65536\n"), "address: is a number from 0 to &FFFF"));
    CHECK(has(r.error(kPc1600 + "program:\n  file: c.bin\n  address: C000\n"), "address: is a number"));
}

TEST(c1600_4_ac19_test_program_kind_follows_the_file) {
    PresetReadRig r;
    std::ifstream in("tests/fixtures/basic/lissajou-1600.bbin", std::ios::binary);
    r.write("l.bbin", std::string(std::istreambuf_iterator<char>(in), {}));
    CHECK(r.ok(kPc1600 + "program:\n  file: l.bbin\n"));
    CHECK(has(r.error(kPc1600 + "program:\n  file: l.bbin\n  typed: true\n"), "is tokenized BASIC"));
}

TEST(c1600_4_ac19_test_preset_parser_sequential_keys_and_program_blocks) {
    PresetReadRig r;
    const auto plan = r.read(kPc1600 + "keys:\n  - key: mode\nprogram:\n  text: |\n    10 A=1\nkeys:\n  - type: RUN\n");
    const auto* p = std::get_if<calcu::presets::LoadPlan>(&plan);
    CHECK(p && p->items.size() == 3);
}

TEST(c1600_4_ac19_test_parser_bus_rom_forms) {
    PresetReadRig r;
    r.write("x.bin", std::string(8, '\0'));
    CHECK(r.ok(kPc1500a + "bus-rom:\n  - file: x.bin\n    address: &8800\n    pv: 0\n", "t.pc1500a"));
    CHECK(has(r.error(kPc1500a + "bus-rom:\n  - file: x.bin\n    bank: 7\n", "t.pc1500a"), "takes address:"));
}

TEST(c1600_4_ac19_test_debug_block_numbers_follow_the_preset_rule) {
    PresetReadRig r;
    for (const char* n : {"&C0C5", "0xC0C5", "$C0C5", "49349"})
        CHECK(r.ok(kPc1600 + "debug:\n  program:\n    bin: a.bin\n    address: " + n + "\n"));
    CHECK(has(r.error(kPc1600 + "debug:\n  program:\n    bin: a.bin\n    address: &10000\n"), "a number up to &FFFF"));
}

TEST(c1600_4_ac19_test_parser_rejects_cross_model_fields) {
    PresetReadRig r;
    CHECK(has(r.error(kPc1500a + "slot-2: { name: CE-1600M }\n", "t.pc1500a"), "slot-2 is not available on the PC-1500A"));
    CHECK(has(r.error(kPc1500a + "printer: CE-1600P\n", "t.pc1500a"), "does not fit"));
}

// ── T7: the upgrade (AC-15) ─────────────────────────────────────────────

TEST(c1600_4_ac15_no_version_1_left) {
    // Every preset of the source repository in format 2: since M-47 also the
    // six that waited for LIB-7's media (C1600-4 design §1), without exceptions.
    const std::set<std::string> waiting = {};
    int found = 0;
    for (const char* root : {"examples", "tests/fixtures", "dev", "docs", "vscode"})
        for (const auto& e : fs::recursive_directory_iterator(root)) {
            const std::string ext = e.path().extension().string();
            if (!e.is_regular_file() || (ext != ".pc1500" && ext != ".pc1500a" && ext != ".pc1600")) continue;
            if (e.path().string().find("node_modules") != std::string::npos) continue;
            ++found;
            std::ifstream in(e.path());
            const std::string text((std::istreambuf_iterator<char>(in)), {});
            const bool v1 = text.find("format-version: 1") != std::string::npos;
            const bool waits = waiting.count(e.path().filename().string()) > 0;
            CHECK(v1 == waits);
            if (v1 != waits) std::fprintf(stderr, "  %s\n", e.path().string().c_str());
        }
    CHECK(found > 50);
}

// ── T9: the bundle file, the names (AC-17, AC-18) ───────────────────────

TEST(c1600_4_ac17_bundle_lists_todays_entries) {
    calcu::presets::DirectoryPresetBundle bundle("examples");
    const auto read = calcu::presets::readBundleFile(bundle);
    const auto* entries = std::get_if<std::vector<calcu::presets::BundleEntry>>(&read);
    CHECK(entries != nullptr);
    if (!entries) return;
    std::vector<std::string> got;
    for (const auto& e : *entries) got.push_back((e.startup ? "startup " : e.group + " ") + e.preset);
    // examples/bundled.yaml:12-31 of 0.8.1, startup/default-pc1600 since M-47.
    const std::vector<std::string> want = {"startup startup/default-pc1500.pc1500",
                                           "startup startup/default-pc1500a.pc1500a",
                                           "startup startup/default-pc1600.pc1600",
                                           "BASIC basic/hanoi.pc1600",
                                           "Plotter plotter/lissajou-1500.pc1500a",
                                           "Plotter plotter/lissajou-1600.pc1600",
                                           "Plotter plotter/rectangles-1600.pc1600",
                                           "Plotter plotter/globus.pc1600",
                                           "Plotter plotter/biorhythmus_1600.pc1600",
                                           "Plotter plotter/ascii.pc1600",
                                           "Machine code machine-code/memtest_1500a.pc1500a"};
    CHECK(got == want);
    for (const auto& e : *entries)
        if (e.preset == "basic/hanoi.pc1600") CHECK(e.files == std::vector<std::string>{"basic/hanoi.bas"});
}

TEST(c1600_4_ac18_names_short_described_once_unique_per_model) {
    PresetReadRig r;
    if (auto set = testRomSet()) sharp::addModuleDefinitions(r.catalogue, *set);
    calcu::presets::DirectoryPresetBundle bundle("examples");
    const std::vector<std::string> problems = calcu::presets::checkBundle(bundle, r.setup());
    for (const auto& p : problems) std::fprintf(stderr, "  bundle.yaml: %s\n", p.c_str());
    CHECK(problems.empty());
    std::vector<std::string> left;
    const auto list = calcu::presets::presetList(bundle, r.rules, r.catalogue, &left);
    CHECK(left.empty());
    std::set<std::string> seen;
    for (const auto& e : list) {
        CHECK(!e.name.empty() && e.name.size() <= 18);
        CHECK(!e.description.empty());
        CHECK(seen.insert(e.model.value + "/" + e.name).second);
    }
}
