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
        for (const auto& r : s.rows) {
            Value::Object row{{"widget", Value(widgetName(r.widget))}, {"section", Value(r.section)},
                              {"key", Value(r.key)}, {"label", Value(r.label)}};
            if (!r.choices.empty()) {   // only when set: Calc-U-1600's form stays as it was
                Value::Array choices;
                for (const auto& [value, label] : r.choices) choices.push_back(Value(Value::Array{Value(value), Value(label)}));
                row.push_back({"choices", Value(std::move(choices))});
            }
            rows.push_back(Value(std::move(row)));
        }
        sections.push_back(Value::object({{"title", s.title}, {"rows", Value(std::move(rows))}}));
    }
    return Value(std::move(sections));
}

Value toValue(const ShellProduct& p) {
    Value::Object variantLabels;
    for (const auto& [id, label] : p.variantLabels) variantLabels.push_back({id, Value(label)});
    Value::Array items;
    for (const auto& i : p.items) {
        Value::Object item{{"group", Value(menuGroupName(i.group))}, {"id", Value(i.id)}, {"label", Value(i.label)},
                           {"entry", Value(i.entry)}};
        if (i.onBar) item.push_back({"onBar", Value(true)});                // only when set
        if (!i.mediaPlace.empty()) item.push_back({"mediaPlace", Value(i.mediaPlace)});
        items.push_back(Value(std::move(item)));
    }
    Value::Object out{{"name", Value(p.name)},
                      {"helpUrl", Value(p.helpUrl)},
                      {"aboutLabel", Value(p.aboutLabel)},
                      {"aboutLines", strings(p.aboutLines)}};
    if (!p.aboutLinks.empty()) {   // only when there is one
        Value::Array links;
        for (const auto& [label, url] : p.aboutLinks) links.push_back(Value(Value::Array{Value(label), Value(url)}));
        out.push_back({"aboutLinks", Value(std::move(links))});
    }
    out.push_back({"variantLabels", Value(std::move(variantLabels))});
    out.push_back({"presetFilter", Value(p.presetFilter)});
    out.push_back({"copyScreen", Value(static_cast<bool>(p.copyScreen))});   // whether the product has the item
    out.push_back({"pasteText", Value(p.pasteText)});
    out.push_back({"moduleSlotPinOuts", strings(p.moduleSlotPinOuts)});
    out.push_back({"placesWithoutControl", strings(p.placesWithoutControl)});
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
