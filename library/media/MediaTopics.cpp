#include "media/MediaTopics.hpp"

namespace calcu::media {

using sessions::Value;

namespace {

const char* groupText(PickerEntry::Group g) {
    switch (g) {
    case PickerEntry::Group::Empty: return "empty";
    case PickerEntry::Group::BundledTemplate: return "bundled-template";
    case PickerEntry::Group::UserTemplate: return "user-template";
    case PickerEntry::Group::Instance: return "instance";
    case PickerEntry::Group::Module: return "module";
    }
    return "";
}

Value itemValue(const KindState::Item& i) {
    Value::Object o{{"name", i.name}, {"description", i.description}, {"fileName", i.fileName},
                    {"origin", originText(i.origin)}, {"template", i.isTemplate}};
    if (i.inPlace) o.push_back({"inPlace", *i.inPlace});
    return Value(std::move(o));
}

Value entryValue(const PickerEntry& e) {
    Value::Object o{{"group", groupText(e.group)}, {"id", e.id}, {"label", e.label}, {"canChoose", e.canChoose}};
    if (e.tooltip) o.push_back({"tooltip", *e.tooltip});
    if (e.hint) o.push_back({"hint", *e.hint});
    return Value(std::move(o));
}

} // namespace

Value MediaState::toValue() const {
    Value::Array ks;
    for (const KindState& k : kinds) {
        Value::Array t, i;
        for (const auto& x : k.templates) t.push_back(itemValue(x));
        for (const auto& x : k.instances) i.push_back(itemValue(x));
        ks.push_back(Value(Value::Object{{"kind", k.kind},
                                         {"label", k.label},
                                         {"suffix", k.suffix},
                                         {"templates", Value(std::move(t))},
                                         {"instances", Value(std::move(i))}}));
    }
    Value::Array ps;
    for (const PlaceState& p : places) {
        Value::Array sides, picker;
        for (int s : p.sides) sides.push_back(Value(s));
        for (const auto& e : p.picker) picker.push_back(entryValue(e));
        Value::Object o{{"place", p.place}, {"kind", p.kind}, {"slot", p.slot}, {"passThrough", p.passThrough},
                        {"sides", Value(std::move(sides))}};
        if (p.holds) {
            Value::Object h{{"name", p.holds->name}, {"template", p.holds->isTemplate}, {"unsaved", p.holds->unsaved},
                            {"writeProtected", p.holds->writeProtected}};
            if (p.holds->side) h.push_back({"side", *p.holds->side});
            o.push_back({"holds", Value(std::move(h))});
        }
        if (p.passing) o.push_back({"passing", *p.passing});
        o.push_back({"waits", p.waits});
        o.push_back({"nameAndSave", p.nameAndSave});
        o.push_back({"askWriteProtect", p.askWriteProtect});
        o.push_back({"picker", Value(std::move(picker))});
        ps.push_back(Value(std::move(o)));
    }
    return Value(Value::Object{{"kinds", Value(std::move(ks))}, {"places", Value(std::move(ps))}});
}

const PlaceState* MediaState::place(const std::string& word) const {
    for (const auto& p : places)
        if (p.place == word) return &p;
    return nullptr;
}

const KindState* MediaState::kindState(const std::string& id) const {
    for (const auto& k : kinds)
        if (k.kind == id) return &k;
    return nullptr;
}

} // namespace calcu::media
