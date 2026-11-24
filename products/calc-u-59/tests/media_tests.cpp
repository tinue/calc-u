// The media of Calc-U-59: the one kind and the one place, slot-1's picker of modules, the
// names of cards in the save folder, the write when a card is given back,
// and `card:` / `arm:` in presets and the harness. On the app session with a
// save folder of the test's own (tests/CardRig.hpp).

#include "TestHarness.hpp"
#include "CardRig.hpp"
#include "../app/HarnessProduct.hpp"
#include "../app/ModuleCatalog.hpp"
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

std::string cardText(const std::string& name, bool isTemplate = false) {
    TIMagneticCardData c;
    c.sides[0] = std::vector<std::uint8_t>(kCardBankBytes, 0x10);
    return writeMagneticCardFile(c, name, "", isTemplate, std::nullopt, "");
}
} // namespace

TEST(one_kind_one_place_pass_through_sides_0_1) {
    NEEDS_ROMS;
    CardRig r;
    const auto* m = r.media();
    CHECK(m && m->kinds.size() == 1);
    if (!m) return;
    CHECK(m->kinds[0].kind == "magnetic-card" && m->kinds[0].suffix == ".magnetcard.yaml");
    int places = 0;
    for (const auto& p : m->places)
        if (!p.slot) {
            ++places;
            CHECK(p.place == "card-reader" && p.passThrough && p.sides == (std::vector<int>{0, 1}));
        }
    CHECK_EQ(places, 1);
}

TEST(no_bundled_cards_fresh_card_template_without_file) {
    NEEDS_ROMS;
    CardRig r;
    const auto* k = r.media()->kindState("magnetic-card");
    CHECK(k && k->templates.size() == 1 && k->templates[0].name == "Fresh card" && k->templates[0].fileName.empty());
    CHECK(k && k->instances.empty());
    CHECK(magneticCardKind().bundledFolder.empty());
}

TEST(slot_picker_empty_and_14_modules_by_menu_sort_description_label) {
    NEEDS_ROMS;
    CardRig r;
    for (const char* model : {"TI-59", "TI-58", "TI-58C"}) {
        CHECK(r.model(model).done);
        r.runFor(50ms);
        const auto* slot = r.media()->place("slot-1");
        CHECK(slot && slot->slot);
        if (!slot) continue;
        CHECK_EQ(slot->picker.size(), std::size_t{15});
        CHECK(slot->picker[0].group == calcu::media::PickerEntry::Group::Empty);
        std::vector<std::string> labels;
        for (std::size_t i = 1; i < slot->picker.size(); ++i) {
            CHECK(slot->picker[i].group == calcu::media::PickerEntry::Group::Module && !slot->picker[i].tooltip);
            labels.push_back(slot->picker[i].label);
        }
        std::vector<std::string> want;
        for (const auto& md : allModuleMetadata(*testroms::romSet())) want.push_back(md.menuTitle);   // menu-sort order
        CHECK(labels == want);
        CHECK(!labels.empty() && labels[0] == "Master Library");
        CHECK(slot->holds && slot->holds->name == "Master Library");   // the module plugged
    }
}

TEST(choose_replaces_and_power_cycles) {
    NEEDS_ROMS;
    CardRig r;
    CHECK(r.step("key", "5").done);
    const auto chosen = r.operation(calcu::media::kChooseCommand,
                                    calcu::sessions::Value::object({{"place", "slot-1"}, {"entry", "module:LE"}}));
    CHECK(chosen.done);
    CHECK(r.untilKeyReady());
    CHECK_EQ(r.part().moduleID(), std::string("LE"));
    CHECK_EQ(testroms::displayText(r.machine()), std::string("0"));   // the power cycle
    CHECK(r.operation(calcu::media::kChooseCommand, calcu::sessions::Value::object({{"place", "slot-1"}, {"entry", ""}})).done);
    CHECK(r.untilKeyReady());
    CHECK(r.part().moduleID().empty());   // the empty entry: a detach
}

// The names of cards in the save folder.
TEST(names_and_collisions_for_cards) {
    NEEDS_ROMS;
    CardRig r;
    r.writeCardFile("Prog", cardText("My Program"));        // the name inside, not the file's
    r.writeCardFile("Tpl", cardText("My Template", true));   // a template by its file
    r.send(calcu::media::kRefreshCommand);
    const auto* k = r.media()->kindState("magnetic-card");
    bool prog = false, tpl = false;
    for (const auto& i : k->instances) prog = prog || (i.name == "My Program" && i.fileName == "Prog.magnetcard.yaml");
    for (const auto& i : k->templates) tpl = tpl || (i.name == "My Template" && i.fileName == "Tpl.magnetcard.yaml");
    CHECK(prog && tpl);
    // A new card's file is named after its name; a taken file name gets -1.
    r.writeCardFile("Mine", "not a card");
    r.send(calcu::media::kRefreshCommand);
    CHECK(r.load(CardRig::preset(kProgramA + "keys:\n  - key: 1 Write\n  - card: { mem: M }\n"
                                 "  - saveas: { card: M, as: live, name: Mine }\n")).done);
    CHECK(testroms::fs::exists(r.folder.path() / "Mine-1.magnetcard.yaml"));
    CHECK_EQ(r.cardFile("Mine"), std::string("not a card"));
    // One list of names, in any case.
    const auto again = r.load(CardRig::preset(kProgramA + "keys:\n  - key: 1 Write\n  - card: { mem: M }\n"
                                              "  - saveas: { card: M, as: template, name: MY PROGRAM }\n"));
    CHECK(!again.done && again.reason.find("name of an instance") != std::string::npos);
}

TEST(instance_written_when_given_back) {
    NEEDS_ROMS;
    CardRig r;
    r.writeCardFile("Prog", cardText("Prog"));
    r.send(calcu::media::kRefreshCommand);
    const std::string before = r.cardFile("Prog");
    CHECK(r.load(CardRig::preset(kProgramA)).done);
    CHECK(r.armSaved("Prog.magnetcard.yaml").done);
    CHECK(r.keyWrite(2));
    CHECK(r.feed(1).done);
    r.runFor(5ms);
    CHECK_EQ(r.cardFile("Prog"), before);   // not while it passes
    CHECK(r.untilPassed());
    const std::string after = r.cardFile("Prog");
    CHECK(after != before && after.find("saved: ") != std::string::npos);
    CHECK(CardRig::sideOf(after, 0) == CardRig::sideOf(before, 0));   // the other side kept
    // "Saved …" says how the app ran: the app log has it, the calculator's log not.
    CHECK(r.appLog.has(calcu::applog::AppLogLevel::Info, "Saved magnetic card"));
    CHECK(!r.logged("Saved magnetic card"));
}

TEST(read_pass_writes_nothing) {
    NEEDS_ROMS;
    CardRig r;
    r.writeCardFile("Prog", cardText("Prog"));
    r.send(calcu::media::kRefreshCommand);
    const std::string before = r.cardFile("Prog");
    CHECK(r.load(CardRig::preset("")).done);
    CHECK(r.untilKeyReady());
    CHECK(r.armSaved("Prog.magnetcard.yaml").done);
    CHECK(r.feed(0).done);
    CHECK(r.untilPassed());
    r.runFor(1s);
    CHECK_EQ(r.cardFile("Prog"), before);
    CHECK(!r.logged("Saved magnetic card"));
}

TEST(card_and_arm_by_name_file_mem) {
    NEEDS_ROMS;
    CardRig r;
    r.writeCardFile("Named", cardText("By Name"));
    r.send(calcu::media::kRefreshCommand);
    CHECK(r.load(CardRig::preset("keys:\n  - arm: { name: by name }\n")).done);
    CHECK(r.fitted() && r.fitted()->name == "By Name");
    const std::string file = (r.folder.path() / "Named.magnetcard.yaml").string();
    CHECK(r.load(CardRig::preset("keys:\n  - card: { file: " + file + ", side: 0 }\n  - wait: 1s\n")).done);
    CHECK(r.fitted() && r.fitted()->name == "By Name" && !r.passing());
    CHECK(r.load(CardRig::preset("keys:\n  - arm: { mem: In }\n")).done);
    CHECK(r.fitted() && r.fitted()->name == "In");
    CHECK(!r.load(CardRig::preset("keys:\n  - arm: { mem: In, side: 1 }\n")).done);   // arm: has no side
}

TEST(missing_name_or_file_refuses) {
    NEEDS_ROMS;
    CardRig r;
    const auto byName = r.load(CardRig::preset("keys:\n  - card: { name: Ghost }\n"));
    CHECK(!byName.done && byName.reason.find("there is no magnetic card “Ghost”") != std::string::npos);
    const auto byFile = r.load(CardRig::preset("keys:\n  - card: { file: ghost.magnetcard.yaml }\n"));
    CHECK(!byFile.done);
    CHECK(!r.hasCardFile("Ghost") && !r.hasCardFile("ghost"));   // no blank card saved under it
}

TEST(undefined_mem_fresh) {
    NEEDS_ROMS;
    CardRig r;
    r.send(calcu::sessions::kLogLevelCommand, calcu::sessions::Value::object({{"level", "info"}}));   // the progress lines
    CHECK(r.load(CardRig::preset("keys:\n  - arm: { mem: Nobody }\n")).done);
    const auto c = r.fitted() ? magneticCardOf(r.fitted()->content) : nullptr;
    CHECK(c && std::holds_alternative<std::monostate>(c->sides[0]) && std::holds_alternative<std::monostate>(c->sides[1]));
    CHECK(r.logged("Nobody"));
}

TEST(card_file_written_in_place_unless_template) {
    NEEDS_ROMS;
    CardRig r;
    r.writeCardFile("Live", cardText("Live"));
    r.writeCardFile("Tpl", cardText("Tpl", true));
    r.send(calcu::media::kRefreshCommand);
    const std::string tpl = r.cardFile("Tpl");
    CHECK(r.load(CardRig::preset(kProgramB + "keys:\n  - key: 1 Write\n  - card: { name: Live }\n  - key: 1 Write\n"
                                 "  - card: { name: Tpl }\n  - wait: 1s\n")).done);
    CHECK(CardRig::sideOf(r.cardFile("Live"), 0) != CardRig::sideOf(cardText("Live"), 0));   // written in place
    CHECK_EQ(r.cardFile("Tpl"), tpl);   // a template is never written
}

TEST(harness_card_mem_side_and_saveas_card) {
    NEEDS_ROMS;
    auto product = makeHarnessProduct();
    calcu::testing::TempFolder save("calcu59-harness-media");
    calcu::harness::RunOptions o;
    o.saveFolder = save.str();
    std::string error;
    auto server = calcu::harness::Server::start(*product, o, &error);
    CHECK(server != nullptr);
    if (!server) return;
    CHECK_EQ(server->line("c59 program 76 11 01 02 03 91").text, std::string("done\n"));
    CHECK_EQ(server->line("key 2 Write").text, std::string("done\n"));
    CHECK_EQ(server->line("card { mem: X, side: 0 }").text, std::string("done\n"));
    CHECK_EQ(server->line("saveas { card: X, as: live, name: Kept }").text, std::string("done\n"));
    const auto text = testroms::readFile((save.path() / "Kept.magnetcard.yaml").string()).value_or("");
    const auto side = CardRig::sideOf(text, 0);
    CHECK(side && bankOfPageByte((*side)[2]) == 2);
    CHECK(server->line("media").text.find("Kept") != std::string::npos);
    server->finish();
}
