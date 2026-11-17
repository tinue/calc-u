// Tests for the debug panel's inspector (Core/Debug/Inspect): the table and
// dump formatting, the bank-state and LH5803 reads it builds on, and the
// views on machines set up by preset (modules, INIT, NEW, TITLE).

#include <cctype>
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

#include "../core/Debug/Inspect/HexDump.hpp"
#include "../core/Debug/Inspect/Inspector.hpp"
#include "../core/Debug/Inspect/PC1500Inspector.hpp"
#include "../core/Debug/Inspect/PC1600Inspector.hpp"
#include "../core/Debug/Inspect/TextTable.hpp"
#include "../core/PC1500/PC1500Machine.hpp"
#include "../core/PC1600/PC1600Machine.hpp"
#include "ScriptedSetup.hpp"
#include "../core/Connector/MemoryCardCatalog.hpp"  // kCardFileSuffix
#include "../core/Connector/SoftwareDefinedCard.hpp"
#include <sstream>
#include "TestRoms.hpp"
#include "TestHarness.hpp"

namespace {

std::string joined(const std::vector<std::string>& lines) {
    std::string s;
    for (const std::string& l : lines) s += l + "\n";
    return s;
}

bool has(const std::vector<std::string>& lines, const std::string& text) {
    return joined(lines).find(text) != std::string::npos;
}

/// A line holding every one of `parts`.
bool row(const std::vector<std::string>& lines, std::initializer_list<const char*> parts) {
    for (const std::string& l : lines) {
        bool all = true;
        for (const char* p : parts) all = all && l.find(p) != std::string::npos;
        if (all) return true;
    }
    return false;
}

std::vector<std::string> view1600(const PC1600Machine& m, inspect::View v) {
    return m.debugInspect([&](const PC1600Machine& x) { return inspect::pc1600View(x, v); });
}

bool enabled1600(const PC1600Machine& m, inspect::View v) {
    const auto entries = m.debugInspect([&](const PC1600Machine& x) {
        auto a = inspect::pc1600Menu(x, false), b = inspect::pc1600Menu(x, true);
        a.insert(a.end(), b.begin(), b.end());
        return a;
    });
    for (const inspect::MenuEntry& e : entries)
        if (e.view == v) return e.enabled;
    return false;
}

/// Sets a PC-1600 up as the version 1 preset `yaml` did -- its `slot-N:`
/// modules, then its `- key:` and `- type:` steps -- on the old class with
/// the typer (M-46: the version 1 loader is gone; these tests are about the
/// views). False (test skipped) without the ROM images.
bool preset1600(PC1600Machine& m, const std::string& yaml) {
    if (!loadPC1600Roms(m)) {
        testharness::skip("PC-1600 ROM images not found");
        return false;
    }
    std::istringstream in(yaml);
    std::vector<std::string> steps;
    for (std::string line; std::getline(in, line);) {
        for (int slot : {1, 2}) {
            const std::string key = "slot-" + std::to_string(slot) + ": ";
            if (line.rfind(key, 0) != 0) continue;
            // The bundled file of a module: CE-1600M is ce1600m.module.yaml.
            std::string path = "frontends/qt6/resources/cards/", err;
            for (char c : line.substr(key.size()))
                if (c != '-') path += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            path += kCardFileSuffix;
            auto card = makeSoftwareDefinedCard(path, slot == 1 ? CardHost::PC1600Slot1 : CardHost::PC1600Slot2, &err);
            if (!card) {
                std::fprintf(stderr, "FAIL card: %s\n", err.c_str());
                testharness::failCount()++;
                return false;
            }
            if (slot == 1) m.attachSlot1Card(std::move(card));
            else m.attachSlot2Card(std::move(card));
        }
        if (line.rfind("  - ", 0) == 0) steps.push_back(line.substr(4));
    }
    scripted::bootPC1600(m);
    for (const std::string& step : steps) {
        if (step.rfind("key: ", 0) == 0) tapKey(m, step.substr(5));
        else if (step.rfind("type: ", 0) == 0 && !scripted::type(m, step.substr(6))) {
            testharness::failCount()++;
            return false;
        }
    }
    return true;
}

// ── Formatting ───────────────────────────────────────────────────────────

TEST(test_text_table_draws_boxes_and_spans) {
    inspect::TextTable t({"Name", "Size"}, {inspect::TextTable::Align::Left, inspect::TextTable::Align::Right});
    t.addSpan("Built in");
    t.addRow({"RAM", "16K"});
    t.addRow({"Σ", "1"});
    const auto out = t.render();
    CHECK(out.size() == 8);
    CHECK(out[0] == "┌──────┬──────┐");
    CHECK(out[1] == "│ Name │ Size │");
    CHECK(out[2] == "├──────┴──────┤");  // the columns end above a span
    CHECK(out[3] == "│ Built in    │");
    CHECK(out[4] == "├──────┬──────┤");  // and start again below it
    CHECK(out[5] == "│ RAM  │  16K │");
    CHECK(out[6] == "│ Σ    │    1 │");  // a multi-byte character counts once
    CHECK(out[7] == "└──────┴──────┘");
    for (const std::string& l : out) CHECK(inspect::displayWidth(l) == 15);
}

TEST(test_text_table_widens_for_a_long_span) {
    inspect::TextTable t({"A", "B"});
    t.addSpan("a span longer than the columns");
    t.addRow({"x", "y"});
    const auto out = t.render();
    for (const std::string& l : out) CHECK(inspect::displayWidth(l) == inspect::displayWidth(out[0]));
}

TEST(test_hex_dump_folds_filler_rows) {
    std::vector<uint8_t> bytes(0x60, 0x00);
    bytes[0x00] = 'H';
    bytes[0x01] = 'I';
    bytes[0x3F] = 0xFF;
    bytes[0x50] = 0x41;
    const auto out = inspect::hexDump(bytes, 0xC000);
    CHECK(out.size() == 3);
    CHECK(out[0].rfind("C000: 48 49 00", 0) == 0);
    CHECK(out[0].find("HI..") != std::string::npos);
    CHECK(out[1] == "C010–C04F: (00/FF only)");
    CHECK(out[2].rfind("C050: 41", 0) == 0);
    CHECK(inspect::sizeLabel(0x4000) == "16K");
    CHECK(inspect::sizeLabel(197) == "197 B");
}

TEST(test_view_names_round_trip) {
    for (inspect::View v : {inspect::View::Pointers, inspect::View::Z80View, inspect::View::DumpDiskS2,
                            inspect::View::DumpMlArea}) {
        inspect::View back;
        CHECK(inspect::viewFromName(inspect::viewName(v), &back) && back == v);
    }
    inspect::View v;
    CHECK(!inspect::viewFromName("nonsense", &v));
    CHECK(std::string(inspect::viewTitle(inspect::View::LhView)) == "LH5803 View");
    CHECK(std::string(inspect::viewTitle(inspect::View::LhView, true)) == "LH5801 View");
}

// ── What the views read ──────────────────────────────────────────────────

// Page B banks 4-7 go out on the 60-pin bus; the bank state names the card
// that answers instead of reporting open bus.
TEST(test_bank_state_names_the_bus_card_in_page_b) {
    PC1600Machine m;
    const std::vector<uint8_t> rom(PC1600HostDriveCard::kRomSize, 0xC9);
    CHECK(m.attachHostDrive(rom.data(), rom.size(), "/tmp"));
    m.bank().writePort31(0x0E);  // page B bank 7
    PC1600Machine::DebugBankState s = m.debugBankState();
    CHECK(s.target[1] == PC1600Machine::PageTarget::BusCard);
    CHECK(s.busCard == "Host drive");
    m.bank().writePort31(0x0C);  // page B bank 6: nothing there
    s = m.debugBankState();
    CHECK(s.target[1] == PC1600Machine::PageTarget::OpenBus);
}

TEST(test_bank_state_empty_slot_is_open_bus) {
    PC1600Machine m;
    m.bank().writePort31(0x00);  // page C bank 0 = Slot 1, empty
    CHECK(m.debugBankState().target[2] == PC1600Machine::PageTarget::OpenBus);
}

// LHA90: the LH5803's 7400H-744FH reads 7600H-764FH, in the debugger's peek
// as on the bus.
TEST(test_lh5803_debug_peek_applies_lha90) {
    PC1600Machine m;
    m.memory().write(0xF610, 0x5A);  // LH5803 7610H
    bool readable = false;
    CHECK(m.lh5803Memory().debugPeek(0x7410, false, &readable) == 0x5A && readable);
    CHECK(m.lh5803Memory().debugPeek(0x7410, false, &readable) == m.lh5803Memory().readME0(0x7410));
}

// A slot read for the inspector leaves the card as it was (no probe write).
TEST(test_slot_bus_read_has_no_side_effects) {
    PC1600Machine m;
    if (!preset1600(m, "format-version: 1\nmodel: PC-1600\nslot-1: CE-1600M\n")) return;
    const uint64_t before = m.memory().slot1CardRevision();
    uint8_t v = 0;
    CHECK(m.memory().slotBusRead(1, 0x8000, v));
    CHECK(!m.memory().slotBusRead(2, 0x8000, v));  // Slot 2 is empty
    CHECK(m.memory().slot1CardRevision() == before);
}

// ── Views ────────────────────────────────────────────────────────────────

// CE-1601M in Slot 2, INIT "F" then "M": vertical bank 0 back to BASIC,
// vertical bank 1 the RAM disk; the CE-1600M in Slot 1 is expansion memory.
TEST(test_pc1600_views_split_ce1601m) {
    PC1600Machine m;
    if (!preset1600(m, "format-version: 1\nmodel: PC-1600\nslot-1: CE-1600M\nslot-2: CE-1601M\n"
                       "keys:\n  - key: mode\n  - type: NEW0\n  - type: INIT\"S2:\",\"F\"\n"
                       "  - type: INIT\"S2:\",\"M\"\n"))
        return;
    const auto disks = view1600(m, inspect::View::DiskAreas);
    CHECK(row(disks, {"S2:", "CE-1601M", "32K", "vertical bank 1", "media F1H"}));
    CHECK(row(disks, {"S1:", "no RAM disk"}));
    CHECK(enabled1600(m, inspect::View::DumpDiskS2));
    CHECK(!enabled1600(m, inspect::View::DumpDiskS1));
    CHECK(!enabled1600(m, inspect::View::DumpProgramS1));

    const auto phys = view1600(m, inspect::View::PhysicalRam);
    CHECK(row(phys, {"8000–FFFF", "32K", "RAM disk S2:"}));
    CHECK(row(phys, {"S0: header + system reserve"}));

    const auto basic = view1600(m, inspect::View::BasicArea);
    CHECK(has(basic, "Slot 2 — CE-1601M, bank 2"));
    CHECK(has(basic, "Slot 1 — CE-1600M, bank 0"));
    CHECK(row(basic, {"F000–FFFF", "work area"}));

    const auto inv = view1600(m, inspect::View::Inventory);
    CHECK(row(inv, {"Slot 2 — CE-1601M", "RAM", "64K", "2 × 32K (port 28H)"}));
    CHECK(row(inv, {"(nothing attached)"}));

    const auto dump = view1600(m, inspect::View::DumpDiskS2);
    CHECK(row(dump, {"08000: 55 80"}));  // the boot sector at vertical bank 1
}

// CE-1600M as a program module with an ML reserve, selected by TITLE.
TEST(test_pc1600_views_program_module) {
    PC1600Machine m;
    if (!preset1600(m, "format-version: 1\nmodel: PC-1600\nslot-1: CE-1600M\n"
                       "keys:\n  - key: mode\n  - type: NEW0\n  - type: INIT\"S1:\",\"P\"\n"
                       "  - type: NEW\"S0:\",&1000\n  - type: NEW\"S1:\",&800\n  - key: mode\n"
                       "  - type: TITLE\"S1:\"\n"))
        return;
    const auto areas = view1600(m, inspect::View::ProgramAreas);
    CHECK(row(areas, {"S1", "CE-1600M", "8800", "◀ TITLE"}));
    CHECK(row(areas, {"S0", "D000"}));
    CHECK(enabled1600(m, inspect::View::DumpProgramS1));

    const auto phys = view1600(m, inspect::View::PhysicalRam);
    CHECK(row(phys, {"00C5–07FF", "S1: machine-language reserve"}));
    CHECK(row(phys, {"00C5–0FFF", "S0: machine-language reserve"}));

    const auto ptrs = view1600(m, inspect::View::Pointers);
    CHECK(row(ptrs, {"TITLE", "F1D5", "S1"}));
    CHECK(row(ptrs, {"ML reserve", "3899"}));
    CHECK(row(ptrs, {"S1 free", "STATUS 259"}));
}

TEST(test_pc1600_z80_and_lh_views_on_a_booted_machine) {
    PC1600Machine m;
    if (!bootPC1600(m)) return;
    const auto z80 = view1600(m, inspect::View::Z80View);
    CHECK(row(z80, {"A", "0000–3FFF", "System ROM, lower 16K", "ROM"}));
    CHECK(row(z80, {"D", "C000–FFFF", "Internal RAM", "RAM"}));
    const auto lh = view1600(m, inspect::View::LhView);
    CHECK(row(lh, {"ME0", "C000–FFFF", "LH5803 ROM"}));
    CHECK(row(lh, {"ME0", "8000–BFFF", "open bus"}));
    CHECK(view1600(m, inspect::View::DumpMlArea).size() == 1);  // a PC-1500 view: one line saying so
}

// The LH5803's 0000H-3FFFH during a PC-1500 statement, and whether MODE 1
// is possible: P_MAPPRG maps the program bank in MODE 1 only; PC15MAP
// refuses MODE 1 when S0 spans more than one module bank.
TEST(test_pc1600_lh_view_mode1_rows) {
    {
        PC1600Machine m;
        if (!preset1600(m, "format-version: 1\nmodel: PC-1600\nslot-1: CE-161\nkeys:\n  - type: MODE 1\n")) return;
        const auto lh = view1600(m, inspect::View::LhView);
        CHECK(row(lh, {"statements, XPEEK", "Slot 1 — CE-161, bank 0", "P_MAPPRG"}));
        CHECK(has(lh, "MODE 1 is on."));
        CHECK(row(lh, {"functions, arithmetic", "ROM IV (bank 6)", "always page C as it is"}));
    }
    {
        PC1600Machine m;
        if (!preset1600(m, "format-version: 1\nmodel: PC-1600\nslot-1: CE-1600M\nslot-2: CE-1600M\n")) return;
        const auto lh = view1600(m, inspect::View::LhView);
        CHECK(row(lh, {"statements, XPEEK", "ROM IV (bank 6)", "MODE 0"}));
        CHECK(has(lh, "MODE 1 would be refused with ERROR 110"));
    }
    {
        PC1600Machine m;
        if (!preset1600(m, "format-version: 1\nmodel: PC-1600\nslot-1: CE-1600M\n"
                           "keys:\n  - type: POKE &F1BC,PEEK(&F1BC) OR 64\n"))
            return;
        const auto lh = view1600(m, inspect::View::LhView);
        CHECK(row(lh, {"statements, XPEEK", "CE-1600M, bank 0"}));
        CHECK(has(lh, "MODE 1 is forced"));
    }
}

TEST(test_pc1500_views) {
    SKIP_UNLESS(testRomSet() != nullptr, "the ROMs are not fetched (roms/fetch_roms.sh)");
    PC1500Machine m(PC1500Variant::PC1500A);
    std::string err;
    CHECK(presetroms::loadPC1500Rom(m, "A04", testPresetRoms(), &err));
    scripted::bootPC1500(m);
    CHECK(scripted::type(m, "NEW0"));
    const auto view = [&](inspect::View v) {
        return m.debugInspect([&](const PC1500Machine& x) { return inspect::pc1500View(x, v); });
    };
    const auto ptrs = view(inspect::View::Pointers);
    CHECK(row(ptrs, {"BREAKPARAM", "788A"}));
    CHECK(!has(ptrs, "BREAK_STAT"));
    CHECK(!has(ptrs, "WARM_START"));
    CHECK(!has(ptrs, "CE-150"));  // its group only with a CE-150 attached
    CHECK(row(ptrs, {"MEM"}));
    const auto inv = view(inspect::View::Inventory);
    CHECK(row(inv, {"User RAM", "6K", "4000–57FF"}));
    CHECK(row(inv, {"System RAM", "2K", "7800–7FFF"}));
    const auto lh = view(inspect::View::LhView);
    CHECK(has(lh, "── LH5801 View ──"));
    CHECK(row(lh, {"ME0", "C000–FFFF", "System ROM"}));
    const auto basic = view(inspect::View::BasicArea);
    CHECK(row(basic, {"40C5", "program"}));
    CHECK(view(inspect::View::DumpMlArea).front() == "── Machine-language area 7C00–7FFF ──");
}

}  // namespace
