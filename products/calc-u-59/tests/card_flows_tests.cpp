// C59-7 T5 to T9 (design §6): the card reader as a media place (AC-6 to
// AC-10, EC-1, EC-2, EC-7), fitting (AC-11, AC-12, AC-15, AC-16, AC-24,
// AC-25), the cue card shown (AC-21, AC-22), presets and the harness
// (AC-17 to AC-20, EC-6), and today's card tests without the earlier formats
// (AC-27; moved from part_flows_tests.cpp). On the app session with a save
// folder of the test's own (tests/CardRig.hpp); the ROM runs.

#include "TestHarness.hpp"
#include "CardRig.hpp"
#include "../app/HarnessProduct.hpp"
#include "../app/KeyboardLayout.hpp"
#include "harness/HarnessHost.hpp"
#include "harness/Run.hpp"

using namespace cu59;
using namespace std::chrono_literals;
using testroms::CardRig;
using testroms::kProgramA;
using testroms::kProgramB;

namespace {

bool roms() { return testroms::romSet() != nullptr; }
#define NEEDS_ROMS SKIP_UNLESS(roms(), "the ROMs are not fetched (roms/fetch_roms.sh)")

const double kTop = kCardRect.y + 0.1 * kCardRect.h;   // a point in the top corners' row

/// A card "First" holding Lbl A 1 2 3 R/S on side 0, written by the machine.
void writeFirst(CardRig& r, const std::string& name = "First") {
    CHECK(r.load(CardRig::preset(kProgramA)).done);
    CHECK(r.keyWrite(1));
    CHECK(r.armFresh(name).done);
    CHECK(r.feed(0).done);
    CHECK(r.untilPassed());
    CHECK(r.hasCardFile(name));
}

} // namespace

// ── T5 The place ─────────────────────────────────────────────────────────────

TEST(c59_7_ac6_reader_on_ti59_only) {
    NEEDS_ROMS;
    CardRig r;
    const auto* m = r.media();
    CHECK(m && m->place("card-reader"));
    if (const auto* p = m ? m->place("card-reader") : nullptr) {
        CHECK(p->passThrough && p->sides == (std::vector<int>{0, 1}) && p->kind == "magnetic-card");
    }
    for (const char* model : {"TI-58", "TI-58C"}) {
        CHECK(r.model(model).done);
        r.runFor(50ms);
        CHECK(r.media() && !r.media()->place("card-reader"));
        CHECK(!testroms::cardPlace(r.machine()));
    }
}

TEST(c59_7_ac7_pass_reads_or_writes_and_gives_back) {
    NEEDS_ROMS;
    CardRig r;
    writeFirst(r);
    const std::string written = r.cardFile("First");
    CHECK(CardRig::sideOf(written, 0).has_value());
    CHECK(!r.passing());
    CHECK(r.fitted() && r.fitted()->name == "First");   // it stays fitted
    // A read: the program comes back; nothing is written.
    CHECK(r.load(CardRig::preset("")).done);
    r.runFor(500ms);
    CHECK_EQ(r.progStep(1), 0);
    CHECK(r.armSaved("First.magnetcard.yaml").done);
    CHECK(r.feed(0).done);
    CHECK(r.untilPassed());
    r.runFor(1s);
    CHECK_EQ(r.progStep(1), 11);
    CHECK_EQ(r.cardFile("First"), written);
}

TEST(c59_7_ac7_card_not_taken_stays_until_the_rom_takes_it) {
    NEEDS_ROMS;
    CardRig r;
    // A program that runs and asks for no card: the card waits at the reader.
    CHECK(r.load(CardRig::preset("program:\n  keycodes: |\n    76 11 61 00 00\n")).done);   // Lbl A GTO 000: a loop
    CHECK(r.armFresh("Waiting").done);
    r.post(calcu::typing::kKey, calcu::sessions::Value::object({{"names", calcu::sessions::Value::Array{calcu::sessions::Value("A")}}}));
    r.runFor(300ms);
    CHECK(r.feed(0).done);
    r.runFor(3s);
    CHECK(r.passing());   // nothing gives it back by a time of its own (OQ-8)
    CHECK(r.eject().done);
    CHECK(!r.passing());
    CHECK(r.fitted());
}

TEST(c59_7_ac8_bank_read_at_idle_without_a_key) {
    NEEDS_ROMS;
    CardRig r;
    writeFirst(r);
    CHECK(r.load(CardRig::preset("")).done);
    CHECK(r.untilKeyReady());
    CHECK(!r.waits());   // waiting for a key only: not a card wait (AC-9)
    CHECK(r.armSaved("First.magnetcard.yaml").done);
    CHECK(r.feed(0).done);   // no key pressed
    CHECK(r.untilPassed());
    r.runFor(1s);
    CHECK_EQ(r.progStep(1), 11);
}

TEST(c59_7_ec1_other_partition_not_stored_display_blinks) {
    NEEDS_ROMS;
    CardRig r;
    writeFirst(r);   // partition 479.59
    CHECK(r.load(CardRig::preset("partition: 159\n")).done);
    CHECK(r.untilKeyReady());
    CHECK(r.armSaved("First.magnetcard.yaml").done);
    CHECK(r.feed(0).done);
    CHECK(r.untilPassed());
    r.runFor(1s);
    CHECK_EQ(r.progStep(1), 0);   // not stored
    const std::string a = testroms::displayText(r.machine());
    bool blinks = false;
    for (int i = 0; i < 20 && !blinks; ++i) {
        r.runFor(100ms);
        blinks = testroms::displayText(r.machine()) != a;
    }
    CHECK(blinks);
}

TEST(c59_7_ec2_empty_side_zeroes_rom_errors_nothing_written) {
    NEEDS_ROMS;
    CardRig r;
    CHECK(r.load(CardRig::preset("")).done);
    CHECK(r.untilKeyReady());
    CHECK(r.armFresh("Blank").done);
    CHECK(r.feed(0).done);   // a fresh side at idle: zeroes for the ROM
    r.runFor(5s);
    if (r.passing()) CHECK(r.eject().done);   // FS 10: the ROM ends the read with an error
    r.runFor(500ms);
    CHECK(!r.hasCardFile("Blank"));   // nothing written
    CHECK(r.fitted());
}

TEST(c59_7_ac9_waits_for_medium_in_write_not_at_idle) {
    NEEDS_ROMS;
    CardRig r;
    CHECK(r.load(CardRig::preset(kProgramA)).done);
    CHECK(r.untilKeyReady());
    CHECK(!r.waits());
    CHECK(!r.media()->place("card-reader")->waits);
    CHECK(r.keyWrite(1));   // n Write: the machine waits for a medium
    r.runFor(100ms);
    CHECK(r.media()->place("card-reader")->waits);
    // A side inserted ends the wait (the ROM writes it).
    CHECK(r.armFresh("Ends").done);
    CHECK(r.feed(0).done);
    CHECK(r.untilPassed());
    CHECK(!r.waits() && !r.media()->place("card-reader")->waits);
}

TEST(c59_7_ac10_eject_keeps_whole_bank_only) {
    NEEDS_ROMS;
    CardRig r;
    CHECK(r.load(CardRig::preset(kProgramA)).done);
    CHECK(r.keyWrite(1));
    CHECK(r.armFresh("Half").done);
    CHECK(r.feed(0).done);
    r.runFor(10ms);   // a part of the bank only
    CHECK(r.eject().done);
    r.runFor(300ms);
    CHECK(!r.hasCardFile("Half"));   // a part of a bank is dropped (D-8)
    CHECK(r.fitted());
    // A whole bank written, then Eject Card: kept.
    CardRig w;
    writeFirst(w, "Whole");
    CHECK(CardRig::sideOf(w.cardFile("Whole"), 0).has_value());
}

TEST(c59_7_ec7_protected_bank_as_the_rom_does) {
    NEEDS_ROMS;
    CardRig r;
    // A protected bank (−1 Write) built from an in-file card, read back: the ROM's matter.
    CHECK(r.load(CardRig::preset("keys:\n  - card: { mem: P }\n",
                                 "magnetic-cards:\n  - card-name: P\n    sides:\n      0:\n        bank: 1\n        protected: true\n"
                                 "        program: |\n          76 11 01 91\n")).done);
    r.runFor(1s);
    CHECK_EQ(r.progStep(1), 11);   // read in
}

// ── T6 Fitting ───────────────────────────────────────────────────────────────

TEST(c59_7_ac11_fitted_cue_card_with_bank_numbers_machine_sees_nothing) {
    NEEDS_ROMS;
    CardRig r;
    writeFirst(r);
    CHECK(r.load(CardRig::preset("")).done);
    CHECK(r.untilKeyReady());
    const auto ticks = r.ticks();
    CHECK(r.armSaved("First.magnetcard.yaml").done);
    r.runFor(300ms);
    CHECK(!r.passing() && !r.waits() && r.ticks() > ticks);
    const auto& cue = r.part().cueCard();
    CHECK(cue && cue->template_ == CueCardTemplate::MagnetCard && cue->title == "First");
    CHECK(cue && cue->bankLeft == 1 && !cue->bankRight);
    CHECK_EQ(r.progStep(1), 0);   // the machine saw nothing
    const auto* s = r.state<TICueCardState>(topics::kCueCard);
    CHECK(s && s->onShow && !s->litCorner);
}

TEST(c59_7_ac12_corner_feeds_side_lit_while_in_reader_stays_fitted) {
    NEEDS_ROMS;
    CardRig r;
    writeFirst(r);
    CHECK(r.load(CardRig::preset("")).done);
    CHECK(r.untilKeyReady());
    CHECK(r.armSaved("First.magnetcard.yaml").done);
    CHECK(!r.tap(0.5, kTop));   // not a corner
    CHECK(r.tap(0.05, kTop));   // top left: side 0
    r.runFor(16ms);
    CHECK(r.part().litCardCorner() == 0);
    CHECK(r.untilPassed());
    CHECK(!r.part().litCardCorner());
    CHECK(r.fitted());
    r.runFor(1s);
    CHECK_EQ(r.progStep(1), 11);
}

TEST(c59_7_ac12_corner_size_fifth_width_27_7_height) {
    CHECK(cardCornerAt(0.01, kTop) == 0);
    CHECK(cardCornerAt(0.99, kTop) == 1);
    CHECK(!cardCornerAt(0.5, kTop));
    CHECK(!cardCornerAt(0.21, kTop));
    CHECK(cardCornerAt(0.19, kTop) == 0);
    CHECK(!cardCornerAt(0.1, kCardRect.y + 0.3 * kCardRect.h));
    CHECK(cardCornerAt(0.1, kCardRect.y + 0.27 * kCardRect.h) == 0);
    CHECK(!cardCornerAt(0.1, kCardRect.y - 0.001));
}

TEST(c59_7_ac15_fresh_card_title_is_name_replace_asked_written_at_first_write) {
    NEEDS_ROMS;
    CardRig r;
    writeFirst(r, "Taken");
    const std::string before = r.cardFile("Taken");
    const auto asked = r.armFresh("taken");
    CHECK(!asked.done);
    CHECK_EQ(asked.reason, std::string("asks: Replace card “Taken”? Its first write replaces both sides and the cue card."));
    CHECK(r.armFresh("Taken", true).done);
    CHECK(r.part().cueCard() && r.part().cueCard()->title == "Taken" && !r.part().cueCard()->bankLeft);
    CHECK_EQ(r.cardFile("Taken"), before);   // nothing written until the first write
    CHECK(r.load(CardRig::preset(kProgramB)).done);
    // A preset load makes a new machine: nothing fitted; fit again and write.
    CHECK(r.armFresh("Taken", true).done);
    CHECK(r.keyWrite(2));
    CHECK(r.feed(1).done);
    CHECK(r.untilPassed());
    const std::string after = r.cardFile("Taken");
    CHECK(after != before);
    CHECK(!CardRig::sideOf(after, 0).has_value());   // both sides replaced: side 0 is fresh now
    CHECK(CardRig::sideOf(after, 1).has_value());
    CHECK(after.find("title: Taken") != std::string::npos);
}

TEST(c59_7_ac16_fit_replaces_nothing_while_a_card_passes) {
    NEEDS_ROMS;
    CardRig r;
    writeFirst(r);
    CHECK(r.load(CardRig::preset(kProgramB)).done);
    CHECK(r.armFresh("One").done);
    CHECK(r.armSaved("First.magnetcard.yaml").done);   // takes the place of the one before
    CHECK(r.fitted() && r.fitted()->name == "First");
    CHECK(r.keyWrite(1));
    CHECK(r.feed(0).done);
    r.runFor(5ms);
    CHECK(r.passing());
    r.armFresh("Two");
    CHECK(r.fitted() && r.fitted()->name == "First");   // nothing fitted while it passes
    CHECK(r.noticed("passing") || !r.armFresh("Three").done);
}

TEST(c59_7_ac24_module_card_covers_corners_do_nothing) {
    NEEDS_ROMS;
    CardRig r;
    CHECK(r.armFresh("Mine").done);
    auto mine = [&] { return r.part().cueCard() && r.part().cueCard()->title == "Mine"; };
    r.runFor(50ms);
    CHECK(mine());
    CHECK(r.step("key", "Pgm 0 1").done);
    CHECK(r.until([&] { return !mine(); }));
    CHECK(r.part().cueCard() && r.part().cueCard()->id == "ML-01");
    CHECK(!r.tap(0.05, kTop));   // not on show
    CHECK(!r.passing());
    CHECK(r.fitted());
    CHECK(r.step("key", "Pgm 0 0").done);
    CHECK(r.until(mine));
}

TEST(c59_7_ac25_reset_power_cycle_cold_boot_keep_fitted_passing_card_taken_out) {
    NEEDS_ROMS;
    CardRig r;
    writeFirst(r);
    CHECK(r.load(CardRig::preset(kProgramB)).done);
    CHECK(r.armSaved("First.magnetcard.yaml").done);
    for (const auto& cmd : {calcu::startup::kResetCommand, calcu::startup::kPowerCycleCommand, calcu::startup::kColdBootCommand}) {
        CHECK(r.keyWrite(1));
        CHECK(r.feed(0).done);
        r.runFor(5ms);
        CHECK(r.passing());
        CHECK(r.operation(cmd).done);
        r.runFor(300ms);
        CHECK(!r.passing());   // taken out as by Eject Card
        CHECK(r.fitted() && r.fitted()->name == "First");
        CHECK(r.part().cueCard() && r.part().cueCard()->title == "First");
        CHECK(r.state<TICueCardState>(topics::kCueCard)->onShow);
    }
}

TEST(c59_7_ac25_new_machine_nothing_fitted) {
    NEEDS_ROMS;
    CardRig r;
    CHECK(r.armFresh("Mine").done);
    CHECK(r.model("TI-58").done);
    CHECK(r.model("TI-59").done);
    r.runFor(50ms);
    CHECK(!r.fitted() && !r.part().cueCard());
    CHECK(r.armFresh("Mine").done);
    CHECK(r.load(CardRig::preset("")).done);
    r.runFor(50ms);
    CHECK(!r.fitted() && !r.part().cueCard());
}

// ── T7 The cue card shown ────────────────────────────────────────────────────

TEST(c59_7_ac21_product_kind_gives_selected_program) {
    NEEDS_ROMS;
    CardRig r;
    auto selected = [&] {
        for (const auto& e : r.machine().capabilities(LibraryProgram::kKind))
            if (auto* k = calcu::machine::capabilityAs<LibraryProgram>(r.machine(), e.id)) return k->selected();
        return std::optional<int>(-1);
    };
    CHECK(!selected());
    CHECK(r.step("key", "Pgm 1 9").done);
    r.runFor(100ms);
    CHECK(selected() == 19);
    CHECK_EQ(r.part().activeProgram(), 19);
    CHECK(r.step("key", "Pgm 0 0").done);
    r.runFor(100ms);
    CHECK(!selected());
}

TEST(c59_7_ac22_one_fitted_at_a_time_card_section_constant_memory) {
    NEEDS_ROMS;
    CardRig r;
    CHECK(r.load(CardRig::preset("cue-card:\n  title: Section\n")).done);
    r.runFor(50ms);
    CHECK(r.part().cueCard() && r.part().cueCard()->title == "Section");
    CHECK(r.armFresh("Card").done);
    r.runFor(50ms);
    CHECK(r.part().cueCard() && r.part().cueCard()->title == "Card");   // the card takes the section's place
    CHECK(r.load(CardRig::preset("keys:\n  - arm: { mem: X }\ncue-card:\n  title: Later\n",
                                 "magnetic-cards:\n  - card-name: X\n    sides:\n      0:\n        bank: 1\n")).done);
    r.runFor(50ms);
    CHECK(r.part().cueCard() && r.part().cueCard()->title == "Later");   // the section after it takes its place
    CHECK(!r.state<TICueCardState>(topics::kCueCard)->onShow);
    // The constant memory file's card: tests/constant_memory_tests.cpp (C59-3 AC-7), on a TI-58C without a reader.
}

TEST(c59_7_ac22_module_card_of_selected_program_or_fitted) {
    NEEDS_ROMS;
    CardRig r;
    CHECK(r.armFresh("Mine").done);
    CHECK(r.step("key", "Pgm 0 2").done);
    r.runFor(100ms);
    CHECK(r.part().cueCard() && r.part().cueCard()->id == "ML-02");
    // A module whose program has no card in the file: the fitted card shows.
    // (Every bundled program has one, T4; the module file decides.)
    CHECK(r.step("key", "Pgm 0 0").done);
    r.runFor(100ms);
    CHECK(r.part().cueCard() && r.part().cueCard()->title == "Mine");
}

TEST(c59_7_ac22_follows_within_half_a_second) {
    NEEDS_ROMS;
    CardRig r;
    CHECK(r.step("key", "Pgm 0 3").done);
    const auto start = r.now;
    CHECK(r.until([&] { return r.part().cueCard() && r.part().cueCard()->id == "ML-03"; }, 500ms));
    CHECK(r.now - start <= 500ms);
}

// ── T8 Presets and the harness ───────────────────────────────────────────────

namespace {
const std::string kCardX = "magnetic-cards:\n  - card-name: X\n    cue-card:\n      title: In file\n    sides:\n      0:\n"
                           "        bank: 1\n        program: |\n          76 11 01 02 03 91\n";
const std::string kLoop = "program:\n  keycodes: |\n    76 11 61 00 00\n";   // Lbl A GTO 000
}

TEST(c59_7_ac17_card_finishes_when_passed_and_fitted) {
    NEEDS_ROMS;
    CardRig r;
    CHECK(r.load(CardRig::preset("keys:\n  - card: { mem: X }\n  - wait: 1s\n", kCardX)).done);
    CHECK_EQ(r.progStep(1), 11);
    CHECK(!r.passing());
    CHECK(r.fitted() && r.fitted()->name == "X");   // every card is fitted after it has passed (AC-30)
    CHECK(r.part().cueCard() && r.part().cueCard()->title == "In file" && r.part().cueCard()->bankLeft == 1);
}

TEST(c59_7_ac17_arm_finishes_when_fitted) {
    NEEDS_ROMS;
    CardRig r;
    CHECK(r.load(CardRig::preset("keys:\n  - arm: { mem: X }\n  - wait: 500ms\n", kCardX)).done);
    CHECK(r.fitted() && r.fitted()->name == "X" && !r.passing());
    CHECK_EQ(r.progStep(1), 0);   // fitted, not read
    CHECK(r.tap(0.05, kTop));     // the user feeds it
    CHECK(r.untilPassed());
    r.runFor(1s);
    CHECK_EQ(r.progStep(1), 11);
}

TEST(c59_7_ac17_card_not_taken_fails_after_timeout_last_item_not_waited) {
    NEEDS_ROMS;
    CardRig r;
    const auto failed = r.load(CardRig::preset(kLoop + "keys:\n  - key: A\n  - skip-wait:\n  - card: { mem: X }\n  - wait: 1s\n",
                                               "timeout: 2\n" + kCardX));
    CHECK(!failed.done && failed.reason.find("has not passed after the timeout") != std::string::npos);
    CardRig last;
    CHECK(last.load(CardRig::preset(kLoop + "keys:\n  - key: A\n  - skip-wait:\n  - card: { mem: X }\n", "timeout: 2\n" + kCardX)).done);
    CHECK(last.passing());   // as the last item it is not waited for
}

TEST(c59_7_ac17_ti58_refuses_card_arm_and_magnetic_cards) {
    NEEDS_ROMS;
    CardRig r;
    for (const char* model : {"TI-58", "TI-58C"}) {
        const std::string head = std::string("format-version: 2\nmodel: ") + model + "\n";
        CHECK(!r.load(head + "keys:\n  - card: { mem: X }\n").done);
        CHECK(!r.load(head + "keys:\n  - arm: { mem: X }\n").done);
        CHECK(!r.load(head + kCardX).done);
    }
}

TEST(c59_7_ac17_harness_commands_card_and_arm) {
    NEEDS_ROMS;
    auto product = makeHarnessProduct();
    calcu::testing::TempFolder save("calcu59-harness-cards");
    std::string error;
    calcu::harness::RunOptions o;
    o.saveFolder = save.str();
    auto server = calcu::harness::Server::start(*product, o, &error);
    CHECK(server != nullptr);
    if (!server) return;
    CHECK_EQ(server->line("c59 program 76 11 01 02 03 91").text, std::string("done\n"));
    CHECK_EQ(server->line("key 1 Write").text, std::string("done\n"));
    CHECK_EQ(server->line("card { mem: X, side: 1 }").text, std::string("done\n"));   // LIB-11 AC-22
    CHECK_EQ(server->line("saveas { card: X, as: live, name: Saved }").text, std::string("done\n"));
    CHECK(testroms::fs::exists(save.path() / "Saved.magnetcard.yaml"));
    CHECK_EQ(server->line("arm { name: Saved }").text, std::string("done\n"));
    const std::string media = server->line("state media").text;
    CHECK(media.find("Saved") != std::string::npos);
    server->finish();
}

TEST(c59_7_ac18_items_read_bank_program_data_protected_partition) {
    NEEDS_ROMS;
    CardRig r;
    const std::string cards = "magnetic-cards:\n  - card-name: D\n    description: data\n    sides:\n      1:\n        bank: 4\n"
                              "        protected: false\n        partition: 159\n        data:\n          00: 1.5\n          29: 2.25e-7\n";
    CHECK(r.load(CardRig::preset("keys:\n  - card: { mem: D, side: 1 }\n  - wait: 1s\n", cards)).done);
    CHECK(r.fitted() && r.fitted()->name == "D");
    const auto c = magneticCardOf(r.fitted()->content);
    CHECK(c && sideBank(c->sides[1]) == 4 && std::holds_alternative<std::monostate>(c->sides[0]));
    // Read by the ROM only with the partition of the item = the calculator's (EC-1): here 159 ≠ 479.
    CHECK(r.reg(0) == 0.0);
    CHECK(r.load(CardRig::preset("partition: 159\nkeys:\n  - card: { mem: D, side: 1 }\n  - wait: 1s\n", cards)).done);
    CHECK_EQ(r.reg(0), 1.5);
    CHECK_EQ(r.reg(29), 2.25e-7);
}

TEST(c59_7_ac18_outside_the_bank_or_two_names_refused) {
    NEEDS_ROMS;
    CardRig r;
    const std::string side = "    sides:\n      0:\n        bank: 3\n";
    const auto outside = r.load(CardRig::preset("", "magnetic-cards:\n  - card-name: A\n" + side + "        data:\n          29: 1\n"));
    CHECK(!outside.done && outside.reason.find("not in bank 3") != std::string::npos);
    const auto step = r.load(CardRig::preset("", "magnetic-cards:\n  - card-name: A\n" + side + "        program: |\n          100 01 1\n"));
    CHECK(!step.done && step.reason.find("not in bank 3") != std::string::npos);
    const auto twice = r.load(CardRig::preset("", "magnetic-cards:\n  - card-name: A\n" + side + "  - card-name: a\n" + side));
    CHECK(!twice.done && twice.reason.find("given twice") != std::string::npos);
    const auto unknown = r.load(CardRig::preset("", "magnetic-cards:\n  - card-name: A\n" + side + "        colour: red\n"));
    CHECK(!unknown.done && unknown.reason.find("colour") != std::string::npos);
    const auto noBank = r.load(CardRig::preset("", "magnetic-cards:\n  - card-name: A\n    sides:\n      0:\n        protected: true\n"));
    CHECK(!noBank.done && noBank.reason.find("bank:") != std::string::npos);
}

TEST(c59_7_ac19_side_last_written_kept_through_reset_until_load) {
    NEEDS_ROMS;
    CardRig r;
    // X side 0 defined; a program writes bank 1 to it; Reset, power cycle, Cold boot keep it.
    CHECK(r.load(CardRig::preset(kProgramB + "keys:\n  - key: 1 Write\n  - card: { mem: X }\n  - wait: 1s\n", kCardX)).done);
    for (const auto& cmd : {calcu::startup::kResetCommand, calcu::startup::kPowerCycleCommand, calcu::startup::kColdBootCommand}) {
        CHECK(r.operation(cmd).done);
        CHECK(r.untilKeyReady());
        CHECK(r.feed(0).done);   // the fitted in-file card gives back what was written
        CHECK(r.untilPassed());
        r.runFor(1s);
        CHECK_EQ(r.progStep(1), 12);   // Lbl B, not the definition's Lbl A
    }
    // The next load: as the item defines it.
    CHECK(r.load(CardRig::preset("keys:\n  - card: { mem: X }\n  - wait: 1s\n", kCardX)).done);
    CHECK_EQ(r.progStep(1), 11);
}

TEST(c59_7_ac20_undefined_mem_is_fresh_logged_saved_only_by_saveas) {
    NEEDS_ROMS;
    CardRig r;
    CHECK(r.load(CardRig::preset(kProgramA + "keys:\n  - key: 1 Write\n  - card: { mem: New }\n  - wait: 1s\n")).done);
    CHECK(r.fitted() && r.fitted()->name == "New");
    CHECK(!r.hasCardFile("New"));   // not saved without saveas:
    CHECK(r.load(CardRig::preset(kProgramA + "keys:\n  - key: 1 Write\n  - card: { mem: New }\n  - wait: 1s\n"
                                 "  - saveas: { card: New, as: live, name: New }\n")).done);
    CHECK(r.hasCardFile("New"));
    CHECK(CardRig::sideOf(r.cardFile("New"), 0).has_value());
    CHECK(r.cardFile("New").find("title: New") != std::string::npos);   // a card saved without a cue card
}

TEST(c59_7_ac20_one_name_one_card_two_steps_fill_two_sides) {
    NEEDS_ROMS;
    CardRig r;
    CHECK(r.load(CardRig::preset(kProgramA + "keys:\n  - key: 1 Write\n  - card: { mem: Two }\n  - key: 2 Write\n"
                                 "  - card: { mem: Two, side: 1 }\n  - saveas: { card: Two, as: live, name: Two }\n")).done);
    const std::string text = r.cardFile("Two");
    CHECK(CardRig::sideOf(text, 0).has_value() && CardRig::sideOf(text, 1).has_value());
    const auto s0 = CardRig::sideOf(text, 0), s1 = CardRig::sideOf(text, 1);
    CHECK(s0 && s1 && bankOfPageByte((*s0)[2]) == 1 && bankOfPageByte((*s1)[2]) == 2);
}

TEST(c59_7_ec6_missing_outdated_unreadable_card_refuses_preset) {
    NEEDS_ROMS;
    CardRig r;
    const auto missing = r.load(CardRig::preset("keys:\n  - card: { name: Nowhere }\n"));
    CHECK(!missing.done && missing.reason.find("no magnetic card") != std::string::npos);
    const auto noFile = r.load(CardRig::preset("keys:\n  - arm: { file: nowhere.magnetcard.yaml }\n"));
    CHECK(!noFile.done);
    r.writeCardFile("Old", "format: ti59-magnetic-card\nformat-version: 2\ncard-name: Old\nsides: {}\n");
    r.writeCardFile("Bad", "format: ti59-magnetic-card\nformat-version: 3\ncard-name: Bad\nsides:\n  0:\n    bank: 1\n");
    r.send(calcu::media::kRefreshCommand);
    const auto old = r.load(CardRig::preset("keys:\n  - card: { name: Old }\n"));
    CHECK(!old.done && old.reason.find("outdated") != std::string::npos);
    const auto bad = r.load(CardRig::preset("keys:\n  - card: { file: " + (r.folder.path() / "Bad.magnetcard.yaml").string() + " }\n"));
    CHECK(!bad.done && bad.reason.find("cannot be read") != std::string::npos);
}

// ── T9 Today's tests (AC-27), without the earlier formats ────────────────────

TEST(c59_7_ac27_part_writes_and_reads_a_card) {
    NEEDS_ROMS;
    CardRig r;
    writeFirst(r, "card");
    CHECK(r.load(CardRig::preset("")).done);
    r.runFor(1s);
    CHECK_EQ(r.progStep(1), 0);
    CHECK(r.armSaved("card.magnetcard.yaml").done);
    CHECK(r.feed(0).done);
    CHECK(r.untilPassed());
    r.runFor(1s);
    CHECK_EQ(r.progStep(1), 11);
    CHECK(r.notices().empty());
}

// LIB-13 AC-12 (M-56's cases on `.magnetcard.yaml` since M-55): a card file
// opened (dropped) is fitted where it lies and read at idle; read, not written.
TEST(lib13_ac12_card_fitted_where_it_lies_read_at_idle) {
    NEEDS_ROMS;
    CardRig r;
    writeFirst(r);
    const std::string card = r.cardFile("First");
    calcu::testing::TempFolder elsewhere("calcu59-dropped");
    elsewhere.write("dropped.magnetcard.yaml", card);
    CHECK(r.load(CardRig::preset("")).done);
    r.runFor(1s);
    CHECK(r.drop("dropped.magnetcard.yaml", elsewhere / "dropped.magnetcard.yaml"));
    CHECK(r.fitted() && !r.passing());   // not fed until a corner is tapped
    r.runFor(500ms);
    CHECK_EQ(r.progStep(1), 0);
    CHECK(r.feed(0).done);
    r.runFor(16ms);
    CHECK(r.part().litCardCorner() == 0);
    CHECK(r.untilPassed());
    r.runFor(1s);
    CHECK_EQ(r.progStep(1), 11);
    CHECK_EQ(testroms::readFile(elsewhere / "dropped.magnetcard.yaml").value_or(""), card);
    CHECK(!r.hasCardFile("dropped"));   // nothing imported
}

// LIB-13 AC-12, C59-7 AC-5: written back where it lies.
TEST(lib13_ac12_card_fitted_where_it_lies_written_back) {
    NEEDS_ROMS;
    CardRig r;
    writeFirst(r);
    calcu::testing::TempFolder elsewhere("calcu59-dropped");
    elsewhere.write("dropped.magnetcard.yaml", r.cardFile("First"));
    CHECK(r.load(CardRig::preset(kProgramB)).done);
    CHECK(r.keyWrite(1));
    CHECK(r.drop("dropped.magnetcard.yaml", elsewhere / "dropped.magnetcard.yaml"));
    CHECK(r.feed(0).done);
    CHECK(r.untilPassed());
    const std::string after = testroms::readFile(elsewhere / "dropped.magnetcard.yaml").value_or("");
    CHECK(after != r.cardFile("First"));
    CHECK(!r.hasCardFile("dropped"));
    CHECK(r.load(CardRig::preset("")).done);
    r.runFor(1s);
    CHECK(r.drop("dropped.magnetcard.yaml", elsewhere / "dropped.magnetcard.yaml"));
    CHECK(r.feed(0).done);
    CHECK(r.untilPassed());
    r.runFor(1s);
    CHECK_EQ(r.progStep(1), 12);
    // In a Web frontend (no path) it is imported and fitted (LIB-13 AC-4).
    CHECK(r.drop("Uploaded.magnetcard.yaml", "", writeMagneticCardFile(TIMagneticCardData{}, "Uploaded", "", false, std::nullopt, "")));
    CHECK(r.hasCardFile("Uploaded") && r.fitted() && r.fitted()->name == "Uploaded");
}

// LIB-13 AC-12, LIB-7 AC-25: an opened card whose file is a template is never written.
TEST(lib13_ac12_template_card_opened_never_written) {
    NEEDS_ROMS;
    CardRig r;
    calcu::testing::TempFolder elsewhere("calcu59-dropped");
    const std::string tpl = writeMagneticCardFile(TIMagneticCardData{}, "Tpl", "", true, std::nullopt, "");
    elsewhere.write("tpl.magnetcard.yaml", tpl);
    CHECK(r.load(CardRig::preset(kProgramA)).done);
    CHECK(r.keyWrite(1));
    CHECK(r.drop("tpl.magnetcard.yaml", elsewhere / "tpl.magnetcard.yaml"));
    CHECK(r.fitted() && r.fitted()->isTemplate);
    CHECK(r.feed(0).done);
    CHECK(r.untilPassed());
    CHECK_EQ(testroms::readFile(elsewhere / "tpl.magnetcard.yaml").value_or(""), tpl);
    CHECK(r.fitted() && r.fitted()->unsaved);   // what was written stays with the fitted card
}

// LIB-13 AC-12: a card on a model without a reader is refused with a notice.
TEST(lib13_ac12_refused_on_ti58_and_ti58c_with_notice) {
    NEEDS_ROMS;
    CardRig r;
    calcu::testing::TempFolder elsewhere("calcu59-dropped");
    elsewhere.write("c.magnetcard.yaml", writeMagneticCardFile(TIMagneticCardData{}, "c", "", false, std::nullopt, ""));
    for (const char* m : {"TI-58", "TI-58C"}) {
        CHECK(r.model(m).done);
        const auto before = r.notices().size();
        CHECK(!r.drop("c.magnetcard.yaml", elsewhere / "c.magnetcard.yaml") || r.notices().size() > before);
        r.runFor(100ms);
        CHECK(r.notices().size() > before);
        CHECK(!r.fitted());
    }
    // C59-7 AC-4: a .u59 or .bin of 2.3.0 is no card: refused as no file of the product.
    CHECK(r.model("TI-59").done);
    elsewhere.write("old.u59", "Calc-U-59 Card 2.0\nEND-CARD\n");
    const auto before = r.notices().size();
    r.drop("old.u59", elsewhere / "old.u59");
    r.runFor(100ms);
    CHECK(r.notices().size() > before && !r.fitted());
}

// LIB-13 EC-4, C59-7 AC-16: a card opened while one passes is refused, nothing imported.
TEST(lib13_ec4_card_while_one_passes_refused_nothing_imported) {
    NEEDS_ROMS;
    CardRig r;
    writeFirst(r);
    const std::string card = r.cardFile("First");
    calcu::testing::TempFolder elsewhere("calcu59-dropped");
    elsewhere.write("one.magnetcard.yaml", card);
    elsewhere.write("two.magnetcard.yaml", card);
    CHECK(r.load(CardRig::preset(kProgramB)).done);
    CHECK(r.drop("one.magnetcard.yaml", elsewhere / "one.magnetcard.yaml"));
    CHECK(r.drop("two.magnetcard.yaml", elsewhere / "two.magnetcard.yaml"));   // takes the place of the one before
    CHECK(r.fitted() && r.fitted()->path == elsewhere / "two.magnetcard.yaml");
    CHECK(r.keyWrite(1));
    CHECK(r.feed(0).done);
    r.runFor(5ms);
    CHECK(r.passing());
    const auto before = r.notices().size();
    r.drop("Three.magnetcard.yaml", "", card);   // an upload while one passes
    r.runFor(16ms);
    CHECK(r.notices().size() > before);
    CHECK(!r.hasCardFile("Three") && !r.hasCardFile("First-1"));
    CHECK(r.fitted() && r.fitted()->path == elsewhere / "two.magnetcard.yaml");
}

TEST(c59_7_ac27_fresh_card_writes_both_sides_into_one_file) {
    NEEDS_ROMS;
    CardRig r;
    CHECK(r.load(CardRig::preset(kProgramA)).done);
    CHECK(r.armFresh("Fresh").done);
    CHECK(r.part().cueCard() && r.part().cueCard()->title == "Fresh" && !r.part().cueCard()->bankLeft && !r.part().cueCard()->bankRight);
    CHECK(r.keyWrite(1));
    CHECK(r.feed(0).done);
    CHECK(r.untilPassed());
    CHECK(r.untilKeyReady());
    CHECK(r.keyWrite(2));
    CHECK(r.feed(1).done);
    CHECK(r.untilPassed());
    const std::string text = r.cardFile("Fresh");
    CHECK(CardRig::sideOf(text, 0) && CardRig::sideOf(text, 1));
    CHECK(r.part().cueCard() && r.part().cueCard()->bankLeft == 1 && r.part().cueCard()->bankRight == 2);   // the corners show what was written
    // Side 1 reads bank 2; the card stays fitted after the pass.
    CHECK(r.load(CardRig::preset("")).done);
    r.runFor(1s);
    CHECK(!r.fitted());
    CHECK(r.armSaved("Fresh.magnetcard.yaml").done);
    CHECK(r.tap(0.9, kTop));
    r.runFor(16ms);
    CHECK(r.part().litCardCorner() == 1);
    CHECK(r.untilPassed());
    CHECK(!r.part().litCardCorner());
    CHECK(r.fitted());
}

TEST(c59_7_ac27_a_side_can_hold_any_bank_and_the_corner_follows) {
    NEEDS_ROMS;
    CardRig r;
    CHECK(r.load(CardRig::preset(kProgramA)).done);
    CHECK(r.armFresh("c").done);
    CHECK(r.keyWrite(1));
    CHECK(r.feed(1).done);
    CHECK(r.untilPassed());
    CHECK(r.untilKeyReady());
    CHECK(r.keyWrite(1));
    CHECK(r.feed(0).done);
    CHECK(r.untilPassed());
    CHECK(r.part().cueCard() && r.part().cueCard()->bankLeft == 1 && r.part().cueCard()->bankRight == 1);
}

TEST(c59_7_ac27_write_keeps_the_side_changed_elsewhere_and_refuses_a_broken_file) {
    NEEDS_ROMS;
    CardRig r;
    writeFirst(r);
    const auto first = CardRig::sideOf(r.cardFile("First"), 0);
    // "Shared" holds only side 1 by now (written by another device after it was fitted).
    auto other = std::make_shared<TIMagneticCardData>();
    r.writeCardFile("Shared", writeMagneticCardFile(TIMagneticCardData{}, "Shared", "", false, std::nullopt, ""));
    r.send(calcu::media::kRefreshCommand);
    CHECK(r.load(CardRig::preset(kProgramB)).done);
    CHECK(r.armSaved("Shared.magnetcard.yaml").done);
    other->sides[1] = *first;
    r.writeCardFile("Shared", writeMagneticCardFile(*other, "Shared", "", false, std::nullopt, ""));
    CHECK(r.keyWrite(1));
    CHECK(r.feed(0).done);
    CHECK(r.untilPassed());
    const std::string merged = r.cardFile("Shared");
    CHECK(CardRig::sideOf(merged, 0) && CardRig::sideOf(merged, 1) == first);
    // A broken file at the next write: left alone, reported (EC-4).
    CHECK(r.untilKeyReady());
    r.writeCardFile("Shared", "format: ti59-magnetic-card\nformat-version: 3\nbroken");
    CHECK(r.keyWrite(1));
    CHECK(r.feed(0).done);
    CHECK(r.untilPassed());
    CHECK_EQ(r.cardFile("Shared"), std::string("format: ti59-magnetic-card\nformat-version: 3\nbroken"));
    CHECK(r.noticed("could not be saved"));
}

TEST(c59_7_ac27_fresh_card_replaces_the_file_of_that_name_at_its_first_write_only) {
    NEEDS_ROMS;
    CardRig r;
    auto held = std::make_shared<TIMagneticCardData>();
    held->sides[1] = std::vector<std::uint8_t>(kCardBankBytes, 0x10);
    const std::string before = writeMagneticCardFile(*held, "same", "", false, std::nullopt, "");
    r.writeCardFile("same", before);
    r.send(calcu::media::kRefreshCommand);
    CHECK(r.load(CardRig::preset(kProgramA)).done);
    CHECK(!r.armFresh("same").done);   // the replace question first (AC-15)
    CHECK(r.armFresh("same", true).done);
    CHECK_EQ(r.cardFile("same"), before);   // still there
    CHECK(r.keyWrite(1));
    CHECK(r.feed(0).done);
    CHECK(r.untilPassed());
    const std::string after = r.cardFile("same");
    CHECK(CardRig::sideOf(after, 0).has_value() && !CardRig::sideOf(after, 1).has_value());   // replaced whole at the first write
    // An unreadable file of that name is never replaced: the fresh card gets its own (LIB-7 AC-4).
    r.writeCardFile("other", "whatever was here");
    r.send(calcu::media::kRefreshCommand);
    CHECK(r.untilKeyReady());
    CHECK(r.armFresh("other").done);
    CHECK(r.keyWrite(1));
    CHECK(r.feed(0).done);
    CHECK(r.untilPassed());
    CHECK_EQ(r.cardFile("other"), std::string("whatever was here"));
    CHECK(CardRig::sideOf(testroms::readFile((r.folder.path() / "other-1.magnetcard.yaml").string()).value_or(""), 0).has_value());
}

TEST(c59_7_ac27_blank_side_fed_at_partition_0_writes_nothing) {
    NEEDS_ROMS;
    CardRig r;
    CHECK(r.step("key", "0 Op 1 7").done);
    r.runFor(1s);
    CHECK(r.armFresh("blank").done);
    CHECK(r.feed(0).done);
    r.runFor(5s);
    if (r.passing()) CHECK(r.eject().done);
    r.runFor(500ms);
    CHECK(!r.hasCardFile("blank"));
    CHECK(r.fitted());
}

TEST(c59_7_ac27_card_armed_and_never_used_logged) {
    NEEDS_ROMS;
    CardRig r;
    CHECK(r.load(CardRig::preset("keys:\n  - arm: { mem: X }\n", kCardX)).done);
    CHECK(r.load(CardRig::preset("")).done);   // the next build
    CHECK(r.logged("was armed and never fed"));
}

TEST(c59_7_ac27_card_written_then_inserted) {
    NEEDS_ROMS;
    CardRig r;
    CHECK(r.load(CardRig::preset(kProgramA + "keys:\n  - key: 1 Write\n  - card: { mem: W }\n  - wait: 1s\n"
                                 "  - key: CP\n  - card: { mem: W }\n  - wait: 1s\n")).done);
    CHECK_EQ(r.progStep(1), 11);   // read back what was written to the in-file card
}
