// The PC-100C and the module slot as functional connectors, as the library's
// peripherals. Machine-level tests through the places and the capabilities;
// the golden runs (golden_runs_tests.cpp) guard the printed lines, the
// module runs and the empty slot, the app session's part is in
// machine_control_tests.cpp.

#include "TestHarness.hpp"
#include "TestRoms.hpp"
#include "paper/PaperOutput.hpp"
#include "../app/ModuleCatalog.hpp"
#include "../core/ModuleFile.hpp"
#include "../core/PC100C.hpp"
#include "../core/SolidStateRomModule.hpp"
#include "peripherals/RealTimeClock.hpp"
#include "peripherals/SerialPort.hpp"
#include <algorithm>
#include <array>

using cu59::ModelId;
using cu59::kTI59;
using cu59::kTI58;
using cu59::kTI58C;
using namespace testroms;
using namespace calcu::machine;

namespace {

const ModelId kModels[] = {ModelId{kTI59}, ModelId{kTI58}, ModelId{kTI58C}};
constexpr int k2nd = 21, kLrn = 31, kSbr = 71, kEquals = 95, kPlus = 85, kPrtKey = 94, kAdvKey = 93, kRecip = 35;
constexpr int kDigit[10] = {92, 82, 83, 84, 72, 73, 74, 62, 63, 64};

void keys(Machine& m, ModelId model, std::initializer_list<int> codes) {
    for (int k : codes) tap(m, model, k);
}

std::unique_ptr<Machine> build(ModelId model, std::optional<std::string> module, bool printer, bool trace = false) {
    auto m = cu59::buildMachine(model, *romSet(), cu59::MachinePeripherals{std::move(module), printer, trace});
    testroms::freshMachine(*m);
    return m;
}

bool hasCapability(const Machine& m, const KindId& kind) { return !m.capabilities(kind).empty(); }

// The display lit or dark at each of `samples` 30 ms windows; the number of
// changes between them (an error blink changes about every 0.2 to 0.5 s).
int litChanges(Machine& m, ModelId model, int samples) {
    int changes = 0;
    bool last = false;
    m.readDisplay();
    for (int i = 0; i < samples; ++i) {
        run(m, static_cast<uint32_t>(instructionHz(model) * 0.03));
        bool lit = false;
        const auto out = m.readDisplay();
        for (const auto& p : positions(out)) lit = lit || p.brightness.value_or(0) > 0;
        if (i > 0 && lit != last) ++changes;
        last = lit;
    }
    return changes;
}

// The error blink period of 0 1/x on the TI-59 (see core_tests.cpp).
double ti59BlinkPeriod(Machine& m) {
    runToScanLoop(m, ModelId{kTI59});
    keys(m, ModelId{kTI59}, {kDigit[0], kRecip});
    std::vector<uint64_t> litStarts;
    uint64_t lastSetIdle = 0;
    for (int i = 0; i < 400'000 && litStarts.size() < 4; ++i) {
        m.advance(1);
        if (dbg(m).pc() != 0x0655) continue;
        uint64_t t = m.tickCount() / 32;
        if (lastSetIdle == 0 || t - lastSetIdle > 2000) litStarts.push_back(t);
        lastSetIdle = t;
    }
    if (litStarts.size() < 4) return 0;
    return double(litStarts[3] - litStarts[1]) / 2 / instructionHz(ModelId{kTI59});
}

} // namespace

// ── Places, pin-outs, the 14 modules ───────────────────────────────────────

TEST(places_functional_power_cycle_follows) {
    for (const auto& d : tiCatalogue().models()) {
        for (const char* place : {"slot-1", "printer"}) {
            const auto info = tiCatalogue().place(d.model, PlaceName{place});
            CHECK(info.has_value());
            if (!info) continue;
            CHECK(info->pinOut->level == PinOutLevel::Functional);
            CHECK(info->pinOut->takes == Takes::One);
            CHECK(info->pinOut->after == AfterChange::PowerCycle);
        }
        const auto slot = tiCatalogue().place(d.model, kTIModulePlace);
        const auto printer = tiCatalogue().place(d.model, kTIPrinterPlace);
        if (!slot || !printer) continue;
        CHECK_EQ(slot->fitting.size(), size_t{14});
        CHECK(printer->fitting == (std::vector<PeripheralId>{kPC100C}));
    }
    // On a running machine: an attach and a detach say a power cycle follows.
    auto m = build(ModelId{kTI59}, std::nullopt, false);
    CHECK(m->attach(cu59::moduleRequest("ML", *romSet()), tiCatalogue()).follows == AfterChange::PowerCycle);
    CHECK(m->attach(cu59::printerRequest(false), tiCatalogue()).follows == AfterChange::PowerCycle);
    CHECK(m->detach(kPrinterPlace, kPC100C, tiCatalogue()).follows == AfterChange::PowerCycle);
    CHECK(m->detach(kSlotPlace, PeripheralId{"ML"}, tiCatalogue()).follows == AfterChange::PowerCycle);
}

TEST(fourteen_modules_by_id_description_menu_sort) {
    // The module table: ID, description, ROM chip.
    const std::vector<std::array<const char*, 3>> table = {
        {"ML", "Master Library", "TMC0541"}, {"ST", "Applied Statistics", "TMC0542"},
        {"RE", "Investment", "TMC0543"}, {"SY", "Surveying", "TMC0544"},
        {"NG", "Marine Navigation", "TMC0545"}, {"AV", "Aviation", "TMC0546"},
        {"LE", "Leisure Library", "TMC0547"}, {"SA", "Securities Analysis", "TMC0548"},
        {"BD", "Business Decisions", "TMC0549"}, {"MU", "Math/Utilities", "TMC0550"},
        {"EE", "EE Library", "TMC0551"}, {"SE", "Structural Engineering", "TMC0553"},
        {"AG", "Agriculture", "TMC0554"}, {"RP", "RPN Simulator", "TMC0555"},
    };
    CHECK_EQ(tiModules().size(), table.size());
    for (size_t i = 0; i < table.size() && i < tiModules().size(); ++i) {
        CHECK_EQ(std::string(tiModules()[i].id), std::string(table[i][0]));
        CHECK_EQ(std::string(tiModules()[i].name), std::string(table[i][1]));
        CHECK_EQ(std::string(tiModules()[i].chip), std::string(table[i][2]));
        // The ID is the module file's `module-name`, the name its `description`.
        const auto* bytes = romSet()->bytes(std::string(table[i][0]) + ".rom.yaml");
        CHECK(bytes != nullptr);
        if (!bytes) continue;
        std::string error;
        auto f = readModuleFile({reinterpret_cast<const char*>(bytes->data()), bytes->size()}, error);
        CHECK(error.empty());
        CHECK_EQ(f.moduleName, std::string(table[i][0]));
        CHECK_EQ(f.description, std::string(table[i][1]));
        CHECK(isTIModule(table[i][0]));
    }
    CHECK(!isTIModule("ZZ") && !isTIModule("PC-100C"));
    // The list the picker shows: from the module files, by `menu-sort`, ties
    // in the catalogue's order (ST before RE).
    const auto all = cu59::allModuleMetadata(*romSet());
    std::vector<std::string> ids;
    for (const auto& m : all) ids.push_back(m.id);
    CHECK(ids == (std::vector<std::string>{"ML", "LE", "MU", "EE", "RP", "ST", "RE", "SY", "NG", "AV", "SA", "BD",
                                           "SE", "AG"}));
    if (!all.empty()) {
        CHECK_EQ(all[0].menuTitle, std::string("Master Library"));
        CHECK_EQ(all[0].title, std::string("MASTER LIBRARY MODULE -1-"));
    }
    for (size_t i = 1; i < all.size(); ++i) CHECK(all[i - 1].menuSort <= all[i].menuSort);
    // Each can be plugged into slot-1 by its ID, on every model.
    for (ModelId model : kModels)
        for (const auto& m : tiModules()) {
            auto machine = build(model, std::string(m.id), false);
            CHECK_EQ(moduleIn(*machine), std::string(m.id));
        }
}

// ── The module out of TMC0501; the empty slot ──────────────────────────────

TEST(second_module_refused_until_detached) {
    auto m = build(ModelId{kTI59}, std::string("ML"), false);
    auto r = m->attach(cu59::moduleRequest("LE", *romSet()), tiCatalogue());
    CHECK(r.refused.has_value());
    if (r.refused) CHECK_EQ(r.refused->reason, std::string("slot-1 already holds the Master Library."));
    CHECK_EQ(moduleIn(*m), std::string("ML"));
    CHECK(!m->detach(kSlotPlace, PeripheralId{"ML"}, tiCatalogue()).refused);
    CHECK(!m->attach(cu59::moduleRequest("LE", *romSet()), tiCatalogue()).refused);
    CHECK_EQ(moduleIn(*m), std::string("LE"));
}

TEST(empty_slot_machine_runs) {
    for (ModelId model : kModels) {
        auto m = build(model, std::nullopt, false);
        CHECK(m->plugged().byPlace.empty());
        CHECK(runToScanLoop(*m, model));
        keys(*m, model, {kDigit[1], kPlus, kDigit[2], kEquals});
        CHECK_EQ(displayText(*m), std::string("3"));
    }
}

TEST(empty_slot_runs_and_blinks_on_pgm_01) {
    // The ROM finds no module: 2nd Pgm 01 SBR = blinks as for an error, as
    // 2nd Pgm 30 does with the Master Library (user, from the real machine).
    for (ModelId model : kModels) {
        auto m = build(model, std::nullopt, false);
        CHECK(runToScanLoop(*m, model));
        keys(*m, model, {k2nd, kLrn, kDigit[0], kDigit[1], kSbr, kEquals});
        CHECK(litChanges(*m, model, 100) >= 4);
    }
    auto ml = build(ModelId{kTI59}, std::string("ML"), false);
    CHECK(runToScanLoop(*ml, ModelId{kTI59}));
    keys(*ml, ModelId{kTI59}, {k2nd, kLrn, kDigit[3], kDigit[0], kSbr, kEquals});
    CHECK(litChanges(*ml, ModelId{kTI59}, 100) >= 4);
    // A plain calculation does not blink.
    auto ok = build(ModelId{kTI59}, std::nullopt, false);
    CHECK(runToScanLoop(*ok, ModelId{kTI59}));
    keys(*ok, ModelId{kTI59}, {kDigit[1], kPlus, kDigit[1], kEquals});
    CHECK_EQ(litChanges(*ok, ModelId{kTI59}, 100), 0);
}

TEST(module_program_runs_from_the_slot) {
    // ML-02 is not needed: the Leisure Library's and the Master Library's
    // golden runs (ti59-module-le, ti59-module-ml) check display, memory and
    // printed lines; here the module bytes reach the ROM through the slot:
    // the interpreter's fetch latch moves inside the module.
    auto m = build(ModelId{kTI59}, std::string("ML"), false);
    CHECK(runToScanLoop(*m, ModelId{kTI59}));
    CHECK_EQ(card(*m).libExecPC(), uint16_t{0xFFFF});
    keys(*m, ModelId{kTI59}, {k2nd, kLrn, kDigit[0], kDigit[1], kSbr, kEquals});
    CHECK(card(*m).libExecPC() != 0xFFFF && card(*m).libExecPC() < SolidStateRomModule::kBytes);
}

// ── A module without its ROM ──────────────────────────────────────────────

TEST(module_without_rom_refused_file_named) {
    auto m = build(ModelId{kTI59}, std::nullopt, false);
    PeripheralRequest req = cu59::moduleRequest("SY", *romSet());
    req.roms.clear();   // the file not handed in
    CHECK(m->canAttach(req, tiCatalogue()).has_value());
    auto r = m->attach(req, tiCatalogue());
    CHECK(r.refused.has_value());
    if (r.refused) {
        CHECK_EQ(r.refused->reason, std::string("The Surveying could not be attached: SY.rom.yaml is missing."));
        CHECK(r.refused->missingRoms == (std::vector<std::string>{"SY.rom.yaml"}));
    }
    CHECK(m->plugged().byPlace.empty());
    CHECK(r.follows == AfterChange::Nothing);
}

// ── The PC-100C out of TMC0501; its pads ──────────────────────────────────

TEST(rom_finds_the_printer_as_on_the_real_device) {
    // The ROM prints only when it finds the printer line (D0 on the TI-59
    // and TI-58, D10 on the TI-58C): 2nd Prt prints a line with the PC-100C
    // and nothing without it.
    for (ModelId model : kModels) {
        for (bool printer : {true, false}) {
            auto m = build(model, std::string("ML"), printer);
            CHECK(runToScanLoop(*m, model));
            keys(*m, model, {kDigit[7], k2nd, kPrtKey});
            run(*m, static_cast<uint32_t>(instructionHz(model)));
            const auto lines = printedLines(*m);
            CHECK_EQ(lines.size(), size_t{printer ? 1u : 0u});
            if (printer && !lines.empty()) CHECK_EQ(lines[0], std::string("          7.        "));
            CHECK_EQ(displayText(*m), std::string("7"));
        }
    }
    // The blink of the real TI-59: 0.899 s on the PC-100C, 0.449 s alone.
    auto on = build(ModelId{kTI59}, std::string("ML"), true), off = build(ModelId{kTI59}, std::string("ML"), false);
    const double pOn = ti59BlinkPeriod(*on), pOff = ti59BlinkPeriod(*off);
    CHECK(pOn > 0.899 * 0.98 && pOn < 0.899 * 1.02);
    CHECK(pOff > 0.449 * 0.98 && pOff < 0.449 * 1.02);
}

TEST(busy_counts_instructions_idle_once) {
    // The PC-100C alone: busy for exactly 4165 instructions after a line,
    // 1370 after a feed, one count per instruction cycle.
    PC100C p({});
    p.recordCodeLines(true);
    p.word(PrinterWord::Clear, 0);
    p.word(PrinterWord::Go, 0);
    uint32_t n = 0;
    while (p.busy()) { p.instructionCycle(); ++n; }
    CHECK_EQ(n, PC100C::kLineInstructions);
    CHECK_EQ(n, uint32_t{4165});
    p.word(PrinterWord::Feed, 0);
    n = 0;
    while (p.busy()) { p.instructionCycle(); ++n; }
    CHECK_EQ(n, uint32_t{1370});
    const auto codes = p.takeCodeLines();
    CHECK_EQ(codes.size(), size_t{2});
    if (!codes.empty()) CHECK_EQ(int(codes[0].rowCount), 7);
    // In a machine: Busy (FLG_BUSY in the CPU's flags) stays up for 4165
    // instructions after the line. The count is per instruction, not per
    // tick: TMC0501::step() gives the connector one Φ1 per instruction, an
    // idle one (four times the ticks) included; the busy window's ticks are
    // therefore at least 4165 × 32.
    auto m = build(ModelId{kTI59}, std::string("ML"), true);
    CHECK(runToScanLoop(*m, ModelId{kTI59}));
    keys(*m, ModelId{kTI59}, {kDigit[7], k2nd, kPrtKey});
    run(*m, static_cast<uint32_t>(instructionHz(ModelId{kTI59})));
    printedLines(*m);
    printedCodeLines(*m);
    tap(*m, ModelId{kTI59}, k2nd);
    press(*m, kPrtKey);
    uint32_t busy = 0;
    Ticks start = 0, end = 0;
    bool seen = false;
    for (int i = 0; i < 40'000; ++i) {
        m->advance(1);
        const bool b = (dbg(*m).snapshotCPU().cpuFlags & 0x8000) != 0;
        if (b) {
            if (!seen) start = m->tickCount();
            seen = true;
            ++busy;
            end = m->tickCount();
        } else if (seen) {
            break;
        }
    }
    release(*m, kPrtKey);
    CHECK_EQ(busy, uint32_t{4165});
    CHECK(end - start >= Ticks{(4165 - 1) * 32});
}

TEST(ti58c_key_readiness_with_and_without_printer) {
    // The TI-58C ROM rescans the keys with a printer; key readiness is
    // reached and holds in the scan loop either way, and after a print.
    for (bool printer : {false, true}) {
        auto m = build(ModelId{kTI58C}, std::string("ML"), printer);
        CHECK(runToScanLoop(*m, ModelId{kTI58C}));
        keys(*m, ModelId{kTI58C}, {kDigit[4], k2nd, kPrtKey});
        CHECK(runToScanLoop(*m, ModelId{kTI58C}));
        bool steady = true;
        for (int i = 0; i < 20'000; ++i) {
            m->advance(1);
            steady = steady && m->isKeyReady();
        }
        CHECK(steady);
        keys(*m, ModelId{kTI58C}, {kDigit[1], kPlus, kDigit[2], kEquals});
        CHECK_EQ(displayText(*m), std::string("3"));
    }
}

// ── The PC-100C attached and detached, its capabilities ────────────────────

TEST(printer_plugged_and_removed) {
    for (ModelId model : kModels) {
        auto m = build(model, std::string("ML"), false);
        CHECK(!hasCapability(*m, calcu::paper::PaperOutput::kKind) && !hasCapability(*m, calcu::peripherals::Controls::kKind));
        CHECK(!setPrinter(*m, true).refused);
        CHECK(hasCapability(*m, calcu::paper::PaperOutput::kKind) && hasCapability(*m, calcu::peripherals::Controls::kKind));
        bool listed = false;
        for (const auto& e : m->plugged().byPlace) listed = listed || (e.place == kPrinterPlace && e.peripheral == kPC100C);
        CHECK(listed);
        for (const auto& c : m->plugged().byConnector)
            if (c.connector.value == "printer-connector") CHECK_EQ(c.plugged.size(), size_t{1});
        // A second PC-100C does not fit: the place is taken.
        CHECK(m->attach(cu59::printerRequest(false), tiCatalogue()).refused.has_value());
        // Nothing else fits the printer place, and the PC-100C fits no slot.
        PeripheralRequest wrong = cu59::moduleRequest("ML", *romSet());
        wrong.place = kPrinterPlace;
        CHECK(m->canAttach(wrong, tiCatalogue()).has_value());
        CHECK(!setPrinter(*m, false).refused);
        CHECK(!hasCapability(*m, calcu::paper::PaperOutput::kKind) && !hasCapability(*m, calcu::peripherals::Controls::kKind));
        CHECK_EQ(m->plugged().byPlace.size(), size_t{1});
        m->powerCycle();
        CHECK(runToScanLoop(*m, model));
    }
}

TEST(pc100c_print_adv_trace) {
    auto m = build(ModelId{kTI59}, std::string("ML"), true);
    const auto entries = m->capabilities(calcu::peripherals::Controls::kKind);
    CHECK_EQ(entries.size(), size_t{1});
    if (entries.empty()) return;
    CHECK(entries[0].id.value == "printer/controls");
    CHECK(std::get<PeripheralOwner>(entries[0].owner) == (PeripheralOwner{kPC100C, kPrinterPlace}));
    auto* c = printerControls(*m);
    CHECK(c != nullptr);
    if (!c) return;
    using calcu::peripherals::ControlInfo;
    CHECK(c->list() == (std::vector<ControlInfo>{{ControlId{"PRINT"}, ControlForm::Key, false},
                                                 {ControlId{"ADV"}, ControlForm::Key, false},
                                                 {ControlId{"TRACE"}, ControlForm::Switch, false}}));
    CHECK(c->set(ControlId{"PRINT"}, true) == calcu::peripherals::ControlCall::WrongForm);
    CHECK(c->press(ControlId{"TRACE"}) == calcu::peripherals::ControlCall::WrongForm);
    CHECK(c->press(ControlId{"PAPER"}) == calcu::peripherals::ControlCall::UnknownControl);
    // The descriptor's controls are these.
    const auto* d = tiCatalogue().peripheral(kPC100C);
    CHECK(d && d->controls.size() == 3);
}

TEST(no_pc100c_no_controls) {
    for (ModelId model : kModels) {
        auto m = build(model, std::string("ML"), false);
        CHECK(m->capabilities(calcu::peripherals::Controls::kKind).empty());
        CHECK(printerControls(*m) == nullptr);
        CHECK(m->capability(CapabilityId{"printer/controls"}) == nullptr);
    }
}

TEST(no_serial_port_no_clock) {
    for (ModelId model : kModels)
        for (bool printer : {false, true}) {
            auto m = build(model, std::string("ML"), printer);
            CHECK(m->capabilities(calcu::peripherals::SerialPort::kKind).empty());
            CHECK(m->capabilities(calcu::peripherals::RealTimeClock::kKind).empty());
        }
    for (const auto& d : tiCatalogue().models())
        for (const auto& c : d.components) CHECK(c.id.value != "rtc" && c.id.value != "serial-port");
}

// ── The controls ──────────────────────────────────────────────────────────

TEST(adv_feeds_while_held) {
    auto feeds = [](double seconds) {
        auto m = build(ModelId{kTI59}, std::string("ML"), true);
        runToScanLoop(*m, ModelId{kTI59});
        printerControl(*m, PC100C::kAdv, true);
        run(*m, static_cast<uint32_t>(instructionHz(ModelId{kTI59}) * seconds));
        printerControl(*m, PC100C::kAdv, false);
        run(*m, static_cast<uint32_t>(instructionHz(ModelId{kTI59}) * 0.5));
        size_t n = 0;
        for (const auto& l : printedLines(*m)) n += l.empty();
        return n;
    };
    const size_t shortHold = feeds(0.3), longHold = feeds(2.0);
    CHECK(shortHold >= 2);
    CHECK(longHold > shortHold + 4);
}

TEST(trace_prints_while_on) {
    auto m = build(ModelId{kTI59}, std::string("ML"), true);
    runToScanLoop(*m, ModelId{kTI59});
    printerControl(*m, PC100C::kTrace, true);
    keys(*m, ModelId{kTI59}, {kDigit[4], kPlus, kDigit[5], kEquals});
    run(*m, static_cast<uint32_t>(instructionHz(ModelId{kTI59})));
    const auto on = printedLines(*m);
    CHECK(on == (std::vector<std::string>{"          4.      + ", "          5.      = ", "          9.        "}));
    printerControl(*m, PC100C::kTrace, false);
    keys(*m, ModelId{kTI59}, {kDigit[4], kPlus, kDigit[5], kEquals});
    run(*m, static_cast<uint32_t>(instructionHz(ModelId{kTI59})));
    CHECK(printedLines(*m).empty());
}

// ── TRACE belongs to the PC-100C ──────────────────────────────────────────

TEST(trace_survives_reset_power_cycle_cold_boot) {
    for (ModelId model : kModels) {
        auto m = build(model, std::string("ML"), true, true);   // TRACE from the request
        auto traceOn = [&] {
            auto* c = printerControls(*m);
            if (!c) return false;
            for (const auto& i : c->list())
                if (i.id == PC100C::kTrace) return i.on;
            return false;
        };
        CHECK(traceOn());
        printerControl(*m, PC100C::kPrint, true);
        m->reset();
        CHECK(traceOn());
        m->powerCycle();
        CHECK(traceOn());
        m->coldBoot();
        CHECK(traceOn());
        // PRINT is released by the operation.
        for (const auto& i : printerControls(*m)->list())
            if (i.id == PC100C::kPrint) CHECK(!i.on);
        CHECK(runToScanLoop(*m, model));
        keys(*m, model, {kDigit[2], kPlus, kDigit[3], kEquals});
        run(*m, static_cast<uint32_t>(instructionHz(model)));
        CHECK_EQ(printedLines(*m).size(), size_t{3});   // it prints as it calculates
    }
}

// ── A detach in the middle of a line ──────────────────────────────────────

// The rows printed before a detach in the middle of
// a line stay on its paper; the rest of the line is not printed.
TEST(detach_mid_line_keeps_rows) {
    auto m = build(ModelId{kTI59}, std::string("ML"), true);
    runToScanLoop(*m, ModelId{kTI59});
    printedLines(*m);   // the tap bound from here on
    tap(*m, ModelId{kTI59}, kDigit[7]);
    tap(*m, ModelId{kTI59}, k2nd);
    press(*m, kPrtKey);
    // Up to PRT.GO: the text line is on the paper at once, then 1200
    // instructions of the 4165 the line takes (2 rows of 508, row 3 started).
    std::vector<std::string> text;
    for (int i = 0; i < 40'000 && text.empty(); ++i) {
        m->advance(1);
        text = printedLines(*m);
    }
    CHECK_EQ(text.size(), size_t{1});
    CHECK(testroms::paperTap(*m).paper.marks().empty());   // the dots come when the line ends
    for (int i = 0; i < 1200; ++i) m->advance(1);
    release(*m, kPrtKey);
    auto r = setPrinter(*m, false);   // TIMachine::unplug ends the line where the head is
    CHECK(!r.refused && r.follows == AfterChange::PowerCycle);
    const auto& sheet = testroms::paperTap(*m).paper;
    calcu::paper::Coord lowest = 0;
    bool thirdRow = false;
    for (const auto& mk : sheet.marks()) {
        const auto& d = std::get<calcu::paper::Dot>(mk);
        lowest = std::max(lowest, d.at.y);
        thirdRow = thirdRow || d.at.y / calcu::paper::kPerUnit == 2;
    }
    CHECK(!sheet.marks().empty());
    CHECK(lowest < 3 * calcu::paper::kPerUnit);   // 2 rows and the columns of the third that fired
    CHECK(thirdRow);
    CHECK_EQ(sheet.reach().to, 3 * calcu::paper::kPerUnit);   // the line takes only its rows
    m->powerCycle();
    CHECK(runToScanLoop(*m, ModelId{kTI59}));
    CHECK(paper(*m) == nullptr);
}

// ── A change and the power cycle ──────────────────────────────────────────

TEST(module_change_then_power_cycle_like_fresh_build) {
    // ML → LE in one replace, then the power cycle: the ROM finds LE, and
    // 2nd Pgm 01 SBR = shows and prints what a machine built with LE does.
    auto play = [](Machine& m) {
        runToScanLoop(m, ModelId{kTI59});
        keys(m, ModelId{kTI59}, {k2nd, kLrn, kDigit[0], kDigit[1], kSbr, kEquals});
        run(m, static_cast<uint32_t>(instructionHz(ModelId{kTI59}) * 3));
        std::string out = displayText(m);
        for (const auto& l : printedLines(m)) out += "|" + l;
        return out;
    };
    auto changed = build(ModelId{kTI59}, std::string("ML"), true);
    runToScanLoop(*changed, ModelId{kTI59});
    auto r = changed->replace(PeripheralId{"ML"}, cu59::moduleRequest("LE", *romSet()), tiCatalogue());
    CHECK(!r.refused && r.follows == AfterChange::PowerCycle);
    changed->powerCycle();
    auto fresh = build(ModelId{kTI59}, std::string("LE"), true);
    const std::string a = play(*changed), b = play(*fresh);
    CHECK_EQ(a, b);
    auto ml = build(ModelId{kTI59}, std::string("ML"), true);
    CHECK(play(*ml) != b);   // the two modules differ in program 01
}

TEST(printer_attach_detach_then_rom_finds_it) {
    auto m = build(ModelId{kTI59}, std::string("ML"), false);
    double p = ti59BlinkPeriod(*m);
    CHECK(p > 0.449 * 0.98 && p < 0.449 * 1.02);
    CHECK(setPrinter(*m, true).follows == AfterChange::PowerCycle);
    m->powerCycle();
    p = ti59BlinkPeriod(*m);
    CHECK(p > 0.899 * 0.98 && p < 0.899 * 1.02);
    CHECK(setPrinter(*m, false).follows == AfterChange::PowerCycle);
    m->powerCycle();
    p = ti59BlinkPeriod(*m);
    CHECK(p > 0.449 * 0.98 && p < 0.449 * 1.02);
}
