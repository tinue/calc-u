// The shell's data in the Value forms the Web page reads (shell/ValueForms).
#include "TestHarness.hpp"
#include "sessions/Value.hpp"
#include "shell/ValueForms.hpp"

using namespace calcu;
using sessions::Value;

// What the page needs of the product: its texts, the uploads' accept list,
// Name & Save, the items by id; the About links only when there are.
TEST(shell_product_value_form) {
    shell::ShellProduct p;
    p.name = "Test-Product";
    p.version = "1.0";   // not in the form: the page has its own
    p.helpUrl = "https://example.org/";
    p.aboutLabel = "About Test…";
    p.aboutLines = {"one", "two"};
    p.presetUploads = ".tp,.yaml";
    p.items.push_back(shell::ProductItem{shell::MenuGroup::FileMedia, "mount", "Mount…", "t.mount", {}});
    const Value want = Value::object({{"name", "Test-Product"},
                                      {"helpUrl", "https://example.org/"},
                                      {"aboutLabel", "About Test…"},
                                      {"aboutLines", Value(Value::Array{"one", "two"})},
                                      {"presetUploads", ".tp,.yaml"},
                                      {"nameAndSave", Value::object({{"slotButtons", false},
                                                                     {"prompt", "Name:"},
                                                                     {"prompts", Value(Value::Object{})}})},
                                      {"items", Value(Value::Array{Value::object({{"id", "mount"}, {"label", "Mount…"}})})}});
    CHECK_EQ(sessions::toText(shell::toValue(p)), sessions::toText(want));
    p.aboutLinks = {{"Source", "https://example.org/src"}};
    CHECK_EQ(sessions::toText(*shell::toValue(p).find("aboutLinks")),
             sessions::toText(Value(Value::Array{Value(Value::Array{"Source", "https://example.org/src"})})));
    p.nameAndSave.slotButtons = true;
    p.nameAndSave.prompts = {{"disk", "Disk name:"}};
    CHECK_EQ(sessions::toText(*shell::toValue(p).find("nameAndSave")),
             sessions::toText(Value::object({{"slotButtons", true},
                                             {"prompt", "Name:"},
                                             {"prompts", Value::object({{"disk", "Disk name:"}})}})));
}

// A menu item with every member, its kind, role and shortcut by name.
TEST(menu_item_value_form) {
    shell::MenuItem i;
    i.kind = shell::MenuItem::Kind::Submenu;
    i.id = "x";
    i.role = shell::MenuItem::Role::Preferences;
    i.shortcut = shell::MenuItem::Shortcut::ColdBoot;
    i.children.push_back(shell::MenuItem{});
    const Value v = shell::toValue(i);
    std::vector<std::string> keys;
    for (const auto& m : *v.members()) keys.push_back(m.key);
    CHECK(keys == (std::vector<std::string>{"kind", "id", "label", "checkable", "checked", "enabled", "reason", "role",
                                            "shortcut", "command", "args", "children", "confirm", "destructive"}));
    CHECK_EQ(*v.find("kind")->string(), std::string("submenu"));
    CHECK_EQ(*v.find("role")->string(), std::string("preferences"));
    CHECK_EQ(*v.find("shortcut")->string(), std::string("cold-boot"));
    CHECK_EQ(*(*v.find("children")->elements())[0].find("kind")->string(), std::string("action"));
}

// The bar's groups and controls by name.
TEST(bar_value_form) {
    shell::BarGroupModel g{shell::BarGroup::Media, {}, true};
    shell::BarControl c;
    c.kind = shell::BarControl::Kind::PlacePicker;
    c.id = "printer";
    g.controls.push_back(c);
    const Value v = shell::toValue(std::vector<shell::BarGroupModel>{g});
    const Value& group = (*v.elements())[0];
    CHECK_EQ(*group.find("group")->string(), std::string("Media"));
    CHECK(*group.find("pushed")->boolean());
    const Value& control = (*group.find("controls")->elements())[0];
    CHECK_EQ(*control.find("kind")->string(), std::string("picker"));
    CHECK_EQ(*control.find("id")->string(), std::string("printer"));
}

// A shown dialog row: its row, value, choices as pairs, min and max null when unset.
TEST(dialog_value_form) {
    shell::ShownRow r;
    r.row = {shell::DialogRow::Widget::Integer, "trace", "max", "Max"};
    r.value = "5";
    r.choices = {{"a", "A"}};
    r.min = 1;
    const Value v = shell::toValue(std::vector<shell::ShownSection>{{"Tracing", {r}}});
    const Value& row = (*(*v.elements())[0].find("rows")->elements())[0];
    CHECK_EQ(*row.find("row")->find("widget")->string(), std::string("integer"));
    CHECK_EQ(sessions::toText(*row.find("choices")), sessions::toText(Value(Value::Array{Value(Value::Array{"a", "A"})})));
    CHECK(*row.find("min")->integer() == 1);
    CHECK(row.find("max")->isNull());
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
