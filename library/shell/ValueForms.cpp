#include "shell/ValueForms.hpp"

#include <utility>

namespace calcu::shell {

using sessions::Value;

namespace {

template <class Strings>
Value strings(const Strings& v) {
    Value::Array out;
    for (const auto& s : v) out.emplace_back(s);
    return Value(std::move(out));
}

const char* widgetName(DialogRow::Widget w) {
    using W = DialogRow::Widget;
    switch (w) {
        case W::StartupModel: return "startup-model";
        case W::StartupPreset: return "startup-preset";
        case W::Folder: return "folder";
        case W::Integer: return "integer";
        case W::Choice: return "choice";
        case W::Note: return "note";
        case W::Text: break;
    }
    return "text";
}

const char* barGroupName(BarGroup g) {
    switch (g) {
        case BarGroup::AfterModel: return "AfterModel";
        case BarGroup::Slots: return "Slots";
        case BarGroup::Places: return "Places";
        case BarGroup::Media: return "Media";
        case BarGroup::End: return "End";
    }
    return "End";
}

const char* kindName(MenuItem::Kind k) {
    switch (k) {
        case MenuItem::Kind::Submenu: return "submenu";
        case MenuItem::Kind::Separator: return "separator";
        case MenuItem::Kind::Heading: return "heading";
        case MenuItem::Kind::Action: break;
    }
    return "action";
}

const char* roleName(MenuItem::Role r) {
    switch (r) {
        case MenuItem::Role::Preferences: return "preferences";
        case MenuItem::Role::About: return "about";
        case MenuItem::Role::Quit: return "quit";
        case MenuItem::Role::None: break;
    }
    return "none";
}

const char* shortcutName(MenuItem::Shortcut s) {
    using S = MenuItem::Shortcut;
    switch (s) {
        case S::LoadPreset: return "load-preset";
        case S::Reset: return "reset";
        case S::ColdBoot: return "cold-boot";
        case S::Preferences: return "preferences";
        case S::Quit: return "quit";
        case S::Copy: return "copy";
        case S::Paste: return "paste";
        case S::None: break;
    }
    return "none";
}

const char* controlName(BarControl::Kind k) {
    using K = BarControl::Kind;
    switch (k) {
        case K::SlotPicker: return "slot";
        case K::PlaceButton: return "button";
        case K::PlacePicker: return "picker";
        case K::Media: return "media";
        case K::More: return "more";
        case K::ModelPicker: break;
    }
    return "model";
}

Value items(const std::vector<MenuItem>& list) {
    Value::Array out;
    for (const auto& i : list) out.push_back(toValue(i));
    return Value(std::move(out));
}

Value pairs(const std::vector<std::pair<std::string, std::string>>& list) {
    Value::Array out;
    for (const auto& [a, b] : list) out.push_back(Value(Value::Array{Value(a), Value(b)}));
    return Value(std::move(out));
}

Value control(const BarControl& c) {
    return Value::object({{"kind", controlName(c.kind)}, {"id", c.id}, {"label", c.label}, {"icon", c.icon}, {"shown", c.shown},
                          {"shortShown", c.shortShown}, {"plugged", c.plugged}, {"choices", items(c.choices)},
                          {"entry", c.entry}, {"mediumKind", c.mediumKind}});
}

Value dialogRow(const DialogRow& r) {
    return Value::object({{"widget", widgetName(r.widget)}, {"section", r.section}, {"key", r.key},
                          {"label", r.label}, {"choices", pairs(r.choices)}});
}

} // namespace

Value toValue(const MenuItem& i) {
    return Value::object({{"kind", kindName(i.kind)}, {"id", i.id}, {"label", i.label}, {"checkable", i.checkable},
                          {"checked", i.checked}, {"enabled", i.enabled}, {"reason", i.reason},
                          {"role", roleName(i.role)}, {"shortcut", shortcutName(i.shortcut)},
                          {"command", i.command.value}, {"args", i.args}, {"children", items(i.children)},
                          {"confirm", i.confirm}, {"destructive", i.destructive}});
}

Value toValue(const std::vector<Menu>& menus) {
    Value::Array out;
    for (const auto& m : menus) out.push_back(Value::object({{"id", m.id}, {"label", m.label}, {"items", items(m.items)}}));
    return Value(std::move(out));
}

Value toValue(const std::vector<BarGroupModel>& groups) {
    Value::Array out;
    for (const auto& g : groups) {
        Value::Array controls;
        for (const auto& c : g.controls) controls.push_back(control(c));
        out.push_back(Value::object({{"group", barGroupName(g.group)}, {"controls", Value(std::move(controls))},
                                     {"pushed", g.pushed}}));
    }
    return Value(std::move(out));
}

Value toValue(const std::vector<Panel>& panels) {
    Value::Array out;
    for (const auto& p : panels) {
        Value::Array places;
        for (const auto& c : p.places) places.push_back(control(c));
        out.push_back(Value::object({{"kind", p.kind}, {"owner", p.owner}, {"ownerPlace", p.ownerPlace},
                                     {"hint", p.hint}, {"places", Value(std::move(places))}}));
    }
    return Value(std::move(out));
}

Value toValue(const std::vector<ShownSection>& sections) {
    Value::Array out;
    for (const auto& s : sections) {
        Value::Array rows;
        for (const auto& r : s.rows)
            rows.push_back(Value::object({{"row", dialogRow(r.row)}, {"value", r.value}, {"shown", r.shown},
                                          {"choices", pairs(r.choices)}, {"resettable", r.resettable},
                                          {"min", r.min ? Value(*r.min) : Value()},
                                          {"max", r.max ? Value(*r.max) : Value()}}));
        out.push_back(Value::object({{"title", s.title}, {"rows", Value(std::move(rows))}}));
    }
    return Value(std::move(out));
}

Value toValue(const Rect& r) { return Value::object({{"x", r.x}, {"y", r.y}, {"w", r.w}, {"h", r.h}}); }

Value toValue(const ShellProduct& p) {
    Value::Array items;
    for (const auto& i : p.items) items.push_back(Value::object({{"id", i.id}, {"label", i.label}}));
    Value::Object out{{"name", Value(p.name)},
                      {"helpUrl", Value(p.helpUrl)},
                      {"aboutLabel", Value(p.aboutLabel)},
                      {"aboutLines", strings(p.aboutLines)}};
    if (!p.aboutLinks.empty()) {   // only when there is one
        Value::Array links;
        for (const auto& [label, url] : p.aboutLinks) links.push_back(Value(Value::Array{Value(label), Value(url)}));
        out.push_back({"aboutLinks", Value(std::move(links))});
    }
    out.push_back({"presetUploads", Value(p.presetUploads)});
    Value::Object prompts;
    for (const auto& [place, prompt] : p.nameAndSave.prompts) prompts.push_back({place, Value(prompt)});
    out.push_back({"nameAndSave", Value::object({{"slotButtons", p.nameAndSave.slotButtons},
                                                 {"prompt", p.nameAndSave.prompt},
                                                 {"prompts", Value(std::move(prompts))}})});
    out.push_back({"items", Value(std::move(items))});
    return Value(std::move(out));
}

Value toValue(const HitMap& m) {
    Value::Array keys;
    for (const auto& [id, r] : m.keys)
        keys.push_back(Value::object({{"name", id.value}, {"x", r.x}, {"y", r.y}, {"w", r.w}, {"h", r.h}}));
    Value::Object out{{"distance", m.distance}};
    if (m.display) out.push_back({"display", toValue(*m.display)});
    out.push_back({"keys", Value(std::move(keys))});
    return Value(std::move(out));
}

} // namespace calcu::shell
