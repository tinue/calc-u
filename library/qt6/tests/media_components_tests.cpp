// LIB-7 L11 (design §6, D-7): the media components of the Qt6 component set,
// without a window, on the offscreen platform: the picker drawn from a place
// of the topic `media` (AC-8, AC-9) and the Name & Save dialog (AC-16; D-6).
#include "TestHarness.hpp"
#include "media/MediaTopics.hpp"
#include "qt6/MediaPicker.hpp"
#include "qt6/NameAndSaveDialog.hpp"
#include <QCheckBox>
#include <QLineEdit>
#include <string>
#include <vector>

namespace {

using namespace calcu;
using calcu::media::PickerEntry;
using calcu::media::PlaceState;
using G = PickerEntry::Group;

PlaceState slot() {
    PlaceState p;
    p.place = "slot-1";
    p.kind = "test-module";
    p.slot = true;
    p.picker = {PickerEntry{G::Empty, "", "", std::nullopt, true, std::nullopt},
                PickerEntry{G::BundledTemplate, "bundled:Blank.testmodule.yaml", "Blank", std::string("An empty module"), true, std::nullopt},
                PickerEntry{G::UserTemplate, "saved:Mine.testmodule.yaml", "Mine", std::nullopt, true, std::nullopt},
                PickerEntry{G::Instance, "saved:Bank.testmodule.yaml", "Bank", std::string("My bank"), true, std::nullopt},
                PickerEntry{G::Instance, "saved:Old.testmodule.yaml", "Old", std::nullopt, false,
                            std::string("The test module format version 1 is outdated: the current version is 2, described in x.")},
                PickerEntry{G::Module, "module:test-module", "Test module", std::nullopt, true, std::nullopt}};
    p.holds = PlaceState::Held{"Bank", std::nullopt, false, false, false};
    return p;
}

} // namespace

TEST(ac8_picker_draws_entries) {
    qt6::MediaPicker picker;
    std::vector<std::pair<std::string, std::string>> sent;
    picker.setSender([&](const sessions::CommandName& n, sessions::Value v) {
        sent.emplace_back(n.value, *v.find("entry")->string());
    });
    picker.setPlace(slot());
    CHECK_EQ(picker.objectName().toStdString(), std::string("media-picker-slot-1"));
    CHECK_EQ(picker.count(), 6);
    const std::vector<std::string> labels = {"(empty)", "Blank", "Mine", "Bank", "Old (outdated)", "Test module"};
    for (int i = 0; i < picker.count(); ++i) CHECK_EQ(picker.entryLabel(i).toStdString(), labels[static_cast<std::size_t>(i)]);
    CHECK_EQ(picker.entryTooltip(1).toStdString(), std::string("An empty module"));   // the description as its tooltip
    CHECK(picker.entryTooltip(2).isEmpty());
    CHECK_EQ(picker.currentIndex(), 3);   // what the place holds
    CHECK(sent.empty());                   // drawing sends nothing
    emit picker.activated(1);
    CHECK((sent == std::vector<std::pair<std::string, std::string>>{{"media.choose", "bundled:Blank.testmodule.yaml"}}));
    emit picker.activated(0);
    CHECK_EQ(sent.back().second, std::string(""));   // the empty place
}

TEST(ac9_outdated_entry_disabled_with_hint_tooltip) {
    qt6::MediaPicker picker;
    int sent = 0;
    picker.setSender([&](const sessions::CommandName&, sessions::Value) { ++sent; });
    picker.setPlace(slot());
    CHECK(!picker.entryEnabled(4));
    CHECK(picker.entryEnabled(3));
    CHECK(picker.entryTooltip(4).toStdString().find("version 1 is outdated") != std::string::npos);
    emit picker.activated(4);
    CHECK_EQ(sent, 0);   // it cannot be chosen
}

TEST(ac16_name_and_save_dialog_asks_name_and_switch) {
    qt6::NameAndSaveDialog dialog;
    PlaceState p = slot();
    p.nameAndSave = true;
    p.askWriteProtect = true;
    dialog.setPlace(p);
    dialog.setPrompt(QStringLiteral("Instance name:"));
    CHECK(dialog.asksWriteProtect());
    CHECK_EQ(dialog.prompt().toStdString(), std::string("Instance name:"));
    p.askWriteProtect = false;   // a medium without a switch, or with it on
    dialog.setPlace(p);
    CHECK(!dialog.asksWriteProtect());

    // OK sends name-and-save; an `asks:` reply is a question (D-6), yes sends it again confirmed.
    std::vector<std::string> sent;
    dialog.setSender([&](const sessions::CommandName& n, sessions::Value v) -> std::string {
        sent.push_back(n.value + " " + sessions::toText(v));
        return v.find("confirmed") ? std::string() : std::string("asks: A test module named “Bank” exists. Replace it?");
    });
    QString asked;
    dialog.setAsker([&](const QString& q) {
        asked = q;
        return true;
    });
    dialog.findChild<QLineEdit*>(QStringLiteral("media-name"))->setText(QStringLiteral("Bank"));
    CHECK(dialog.submit().empty());
    CHECK_EQ(asked.toStdString(), std::string("A test module named “Bank” exists. Replace it?"));
    CHECK_EQ(sent.size(), std::size_t(2));
    CHECK(sent[0].find("\"name\":\"Bank\"") != std::string::npos && sent[0].find("confirmed") == std::string::npos);
    CHECK(sent[1].find("\"confirmed\":true") != std::string::npos);
    CHECK(dialog.result() == QDialog::Accepted);
}

// Calc-U-1600 AC-40 (LIB-7 design §10.6 P4, D-25): the product's two Name & Save
// dialogs are this one with today's prompts -- "Instance name:" for a module,
// with the switch "Write-protect (switch to read-only, can't be undone)" where
// the module has one still off, and "Disk name:" for a floppy disk.
TEST(lib7_ac40_qt6_prompts_instance_and_disk_name) {
    qt6::NameAndSaveDialog module;
    PlaceState p = slot();
    p.nameAndSave = true;
    p.askWriteProtect = true;
    module.setPlace(p);
    module.setPrompt(QStringLiteral("Instance name:"));
    CHECK_EQ(module.prompt().toStdString(), std::string("Instance name:"));
    CHECK(module.asksWriteProtect());
    const auto* box = module.findChild<QCheckBox*>(QStringLiteral("media-protect"));
    CHECK(box && box->text().toStdString() == "Write-protect (switch to read-only, can't be undone)");
    qt6::NameAndSaveDialog disk;
    PlaceState d;
    d.place = "floppy";
    d.kind = "floppy-disk";
    d.sides = {0, 1};
    d.nameAndSave = true;
    disk.setPlace(d);
    disk.setPrompt(QStringLiteral("Disk name:"));
    CHECK_EQ(disk.prompt().toStdString(), std::string("Disk name:"));
    CHECK(!disk.asksWriteProtect());
    disk.findChild<QLineEdit*>(QStringLiteral("media-name"))->setText(QStringLiteral("Work"));
    CHECK(sessions::toText(disk.args()).find("\"place\":\"floppy\"") != std::string::npos);
}
