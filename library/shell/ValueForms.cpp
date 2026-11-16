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

} // namespace

const char* menuGroupName(MenuGroup g) {
    switch (g) {
        case MenuGroup::FileOpen: return "FileOpen";
        case MenuGroup::FileKinds: return "FileKinds";
        case MenuGroup::FileMedia: return "FileMedia";
        case MenuGroup::FileEnd: return "FileEnd";
        case MenuGroup::Edit: return "Edit";
        case MenuGroup::MachineEnd: return "MachineEnd";
        case MenuGroup::Help: return "Help";
    }
    return "FileKinds";
}

Value toValue(const Rect& r) { return Value::object({{"x", r.x}, {"y", r.y}, {"w", r.w}, {"h", r.h}}); }

Value toValue(const DialogSpec& spec) {
    Value::Array sections;
    for (const auto& s : spec.sections) {
        Value::Array rows;
        for (const auto& r : s.rows)
            rows.push_back(Value::object(
                {{"widget", widgetName(r.widget)}, {"section", r.section}, {"key", r.key}, {"label", r.label}}));
        sections.push_back(Value::object({{"title", s.title}, {"rows", Value(std::move(rows))}}));
    }
    return Value(std::move(sections));
}

Value toValue(const ShellProduct& p) {
    Value::Object variantLabels;
    for (const auto& [id, label] : p.variantLabels) variantLabels.push_back({id, Value(label)});
    Value::Array items;
    for (const auto& i : p.items)
        items.push_back(Value::object({{"group", menuGroupName(i.group)}, {"id", i.id}, {"label", i.label}, {"entry", i.entry}}));
    return Value::object({{"name", p.name},
                          {"helpUrl", p.helpUrl},
                          {"aboutLabel", p.aboutLabel},
                          {"aboutLines", strings(p.aboutLines)},
                          {"variantLabels", Value(std::move(variantLabels))},
                          {"presetFilter", p.presetFilter},
                          {"copyScreen", true},
                          {"pasteText", p.pasteText},
                          {"moduleSlotPinOuts", strings(p.moduleSlotPinOuts)},
                          {"placesWithoutControl", strings(p.placesWithoutControl)},
                          {"items", Value(std::move(items))}});
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
