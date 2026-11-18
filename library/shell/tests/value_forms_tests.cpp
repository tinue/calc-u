// The shell's data in the Value forms the Web page reads
// (shell/ValueForms; library/web/components/shellModel.js).
#include "TestHarness.hpp"
#include "sessions/Value.hpp"
#include "shell/ValueForms.hpp"

using namespace calcu;
using sessions::Value;

TEST(shell_product_value_form) {
    shell::ShellProduct p;
    p.name = "Test-Product";
    p.version = "1.0";   // not in the form: the page has its own
    p.helpUrl = "https://example.org/";
    p.aboutLabel = "About Test…";
    p.aboutLines = {"one", "two"};
    p.variantLabels = {{"B", "Rev B"}, {"A", "Rev A"}};
    p.presetFilter = "Presets (*.tp)";
    p.copyScreen = [] {};   // the form says whether there is the item
    p.pasteText = true;
    p.moduleSlotPinOuts = {"module"};
    p.placesWithoutControl = {"host-drive"};
    shell::ProductItem i;
    i.group = shell::MenuGroup::FileMedia;
    i.id = "mount";
    i.label = "Mount…";
    i.entry = "t.mount";
    p.items.push_back(i);
    const Value want = Value::object(
        {{"name", "Test-Product"},
         {"helpUrl", "https://example.org/"},
         {"aboutLabel", "About Test…"},
         {"aboutLines", Value(Value::Array{"one", "two"})},
         {"variantLabels", Value(Value::Object{{"A", Value("Rev A")}, {"B", Value("Rev B")}})},
         {"presetFilter", "Presets (*.tp)"},
         {"copyScreen", true},
         {"pasteText", true},
         {"moduleSlotPinOuts", Value(Value::Array{"module"})},
         {"placesWithoutControl", Value(Value::Array{"host-drive"})},
         {"items", Value(Value::Array{Value::object(
                       {{"group", "FileMedia"}, {"id", "mount"}, {"label", "Mount…"}, {"entry", "t.mount"}})})}});
    CHECK_EQ(sessions::toText(shell::toValue(p)), sessions::toText(want));
}

// The About links when there are, Copy Screen as
// the product has it, an item's onBar and mediaPlace when set; a dialog
// row's labelled choices.
TEST(shell_product_form_has_links_and_copy_screen_flag) {
    shell::ShellProduct p;
    p.name = "Calc-U-Test";
    p.aboutLinks = {{"Source", "https://example.org/src"}, {"Privacy", "https://example.org/p"}};
    shell::ProductItem card;
    card.group = shell::MenuGroup::FileMedia;
    card.id = "arm-card";
    card.label = "Arm Card…";
    card.onBar = true;
    card.mediaPlace = "card-reader";
    p.items.push_back(card);
    const Value v = shell::toValue(p);
    CHECK_EQ(sessions::toText(*v.find("aboutLinks")),
             sessions::toText(Value(Value::Array{Value(Value::Array{"Source", "https://example.org/src"}),
                                                 Value(Value::Array{"Privacy", "https://example.org/p"})})));
    CHECK(*v.find("copyScreen")->boolean() == false);   // no Copy Screen: no Edit item
    const Value& item = (*v.find("items")->elements())[0];
    CHECK(*item.find("onBar")->boolean());
    CHECK_EQ(*item.find("mediaPlace")->string(), std::string("card-reader"));
    // Members in the documented order: aboutLinks after aboutLines.
    std::vector<std::string> keys;
    for (const auto& m : *v.members()) keys.push_back(m.key);
    CHECK(keys == (std::vector<std::string>{"name", "helpUrl", "aboutLabel", "aboutLines", "aboutLinks", "variantLabels",
                                            "presetFilter", "copyScreen", "pasteText", "moduleSlotPinOuts",
                                            "placesWithoutControl", "items"}));
    // A labelled choice row carries its choices; one without does not.
    shell::DialogSpec spec;
    shell::DialogRow style{shell::DialogRow::Widget::Choice, "display", "led-style", "Segment Style"};
    style.choices = {{"modernized", "Modernized"}, {"classic", "Classic"}};
    spec.sections.push_back({"Display", {style, {shell::DialogRow::Widget::Integer, "trace", "max", "Max"}}});
    const Value specValue = shell::toValue(spec);
    const auto& rows = *(*specValue.elements())[0].find("rows")->elements();
    CHECK_EQ(sessions::toText(*rows[0].find("choices")),
             sessions::toText(Value(Value::Array{Value(Value::Array{"modernized", "Modernized"}),
                                                 Value(Value::Array{"classic", "Classic"})})));
    CHECK(rows[1].find("choices") == nullptr);
}

TEST(menu_group_value_names) {
    CHECK_EQ(std::string(shell::menuGroupName(shell::MenuGroup::FileOpen)), std::string("FileOpen"));
    CHECK_EQ(std::string(shell::menuGroupName(shell::MenuGroup::FileKinds)), std::string("FileKinds"));
    CHECK_EQ(std::string(shell::menuGroupName(shell::MenuGroup::FileEnd)), std::string("FileEnd"));
    CHECK_EQ(std::string(shell::menuGroupName(shell::MenuGroup::Edit)), std::string("Edit"));
    CHECK_EQ(std::string(shell::menuGroupName(shell::MenuGroup::MachineEnd)), std::string("MachineEnd"));
    CHECK_EQ(std::string(shell::menuGroupName(shell::MenuGroup::Help)), std::string("Help"));
}

TEST(dialog_spec_value_form) {
    using W = shell::DialogRow::Widget;
    shell::DialogSpec spec;
    spec.sections.push_back({"General", {{W::StartupModel, "startup", "model", "Model"}, {W::Text, "s", "k", "Text"}}});
    spec.sections.push_back({"Other",
                             {{W::StartupPreset, "preset", "A", "A"},
                              {W::Folder, "folders", "f", "F"},
                              {W::Integer, "debug", "port", "Port"},
                              {W::Choice, "debug", "on", "On"},
                              {W::Note, "storage", "n", "Note"}}});
    const Value v = shell::toValue(spec);
    const auto* sections = v.elements();
    CHECK(sections && sections->size() == 2);
    const Value& first = (*sections)[0];
    CHECK_EQ(*first.find("title")->string(), std::string("General"));
    const auto& rows = *first.find("rows")->elements();
    CHECK_EQ(sessions::toText(rows[0]),
             sessions::toText(Value::object({{"widget", "startup-model"}, {"section", "startup"}, {"key", "model"}, {"label", "Model"}})));
    CHECK_EQ(*rows[1].find("widget")->string(), std::string("text"));
    std::vector<std::string> widgets;
    for (const Value& r : *(*sections)[1].find("rows")->elements()) widgets.push_back(*r.find("widget")->string());
    CHECK(widgets == (std::vector<std::string>{"startup-preset", "folder", "integer", "choice", "note"}));
}

TEST(hit_map_value_form) {
    shell::HitMap m;
    m.keys.push_back({machine::KeyId{"a"}, shell::Rect{1, 2, 3, 4}});
    m.keys.push_back({machine::KeyId{"b"}, shell::Rect{5, 6, 7, 8}});
    m.display = shell::Rect{10, 20, 30, 40};
    m.distance = 24;
    const Value want = Value::object(
        {{"distance", 24.0},
         {"display", Value::object({{"x", 10.0}, {"y", 20.0}, {"w", 30.0}, {"h", 40.0}})},
         {"keys", Value(Value::Array{Value::object({{"name", "a"}, {"x", 1.0}, {"y", 2.0}, {"w", 3.0}, {"h", 4.0}}),
                                     Value::object({{"name", "b"}, {"x", 5.0}, {"y", 6.0}, {"w", 7.0}, {"h", 8.0}})})}});
    CHECK_EQ(sessions::toText(shell::toValue(m)), sessions::toText(want));
    // No display area: no member `display`.
    m.display.reset();
    CHECK(shell::toValue(m).find("display") == nullptr);
}
