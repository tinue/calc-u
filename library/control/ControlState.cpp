#include "control/ControlState.hpp"
#include "machine/Attach.hpp"
#include <cstddef>
#include <utility>

namespace calcu::control {

using sessions::Value;

namespace {

void add(Value::Object& o, const char* key, Value v) { o.push_back(Value::Member{key, std::move(v)}); }

// {available: bool, reason?}, into an object being built.
void addAvailability(Value::Object& o, const Availability& a, const char* key = "available") {
    add(o, key, a.now);
    if (!a.now) add(o, "reason", a.reason);
}
Value availabilityValue(const Availability& a) {
    Value::Object o;
    addAvailability(o, a);
    return Value(std::move(o));
}

Value owner(const machine::PlacePath& place, const machine::PeripheralId& id) {
    return Value::object({{"place", machine::placeText(place)}, {"peripheral", id.value}});
}

Value variantList(const std::vector<VariantChoice>& vs) {
    Value::Array a;
    for (const auto& v : vs) {
        Value::Object o;
        add(o, "id", v.id.value);
        addAvailability(o, v.available);
        a.push_back(Value(std::move(o)));
    }
    return Value(std::move(a));
}

Value choiceValue(const PeripheralChoice& c) {
    Value::Object o;
    add(o, "id", c.id.value);
    add(o, "name", c.name);
    add(o, "plugged", c.plugged);
    if (c.variant) add(o, "variant", c.variant->value);
    add(o, "variants", variantList(c.variants));
    addAvailability(o, c.available);
    return Value(std::move(o));
}

Value placeValue(const PlaceEntry& p) {
    Value::Object o;
    add(o, "place", machine::placeText(p.path));
    add(o, "connector", p.connector.value);
    add(o, "pinOut", p.pinOut.value);
    addAvailability(o, p.available);
    add(o, "empty", availabilityValue(p.empty));
    Value::Array fitting;
    for (const auto& c : p.fitting) fitting.push_back(choiceValue(c));
    add(o, "fitting", Value(std::move(fitting)));
    return Value(std::move(o));
}

Value mediaPlaceValue(const MediaPlaceEntry& m) {
    Value::Object o;
    add(o, "id", m.id.value);
    if (m.owner) add(o, "owner", owner(m.ownerPlace, *m.owner));
    add(o, "kind", m.mediumKind);
    add(o, "passage", m.passage == machine::MediaPassage::Holding ? "holding" : "pass-through");
    Value::Array sides;
    for (int s : m.sides) sides.push_back(Value(s));
    add(o, "sides", Value(std::move(sides)));
    add(o, "insert", availabilityValue(m.insert));
    return Value(std::move(o));
}

} // namespace

bool ControlState::same(const ControlState& o) const {
    if (!(models == o.models && model == o.model && variants == o.variants && variant == o.variant &&
          places == o.places && mediaPlaces == o.mediaPlaces && panels == o.panels && controls == o.controls &&
          actions == o.actions))
        return false;
    if (productEntries.size() != o.productEntries.size()) return false;
    for (std::size_t i = 0; i < productEntries.size(); ++i) {
        const ProductEntry& a = productEntries[i];
        const ProductEntry& b = o.productEntries[i];
        if (a.name != b.name || !a.value != !b.value) return false;
        if (a.value && !a.value->equals(*b.value)) return false;
    }
    return true;
}

Value ControlState::toValue() const {
    Value::Object o;
    Value::Array ms;
    for (const auto& m : models) {
        Value::Object e;
        add(e, "id", m.id.value);
        addAvailability(e, m.available);
        ms.push_back(Value(std::move(e)));
    }
    add(o, "models", Value(std::move(ms)));
    add(o, "model", model.value);
    add(o, "variants", variantList(variants));
    add(o, "variant", variant ? Value(variant->value) : Value());
    Value::Array ps;
    for (const auto& p : places) ps.push_back(placeValue(p));
    add(o, "places", Value(std::move(ps)));
    Value::Array mps;
    for (const auto& m : mediaPlaces) mps.push_back(mediaPlaceValue(m));
    add(o, "mediaPlaces", Value(std::move(mps)));
    Value::Array panelValues;
    for (const auto& p : panels) {
        Value::Object e;
        add(e, "kind", p.kind);
        if (p.owner && p.ownerPlace) add(e, "owner", owner(*p.ownerPlace, *p.owner));
        panelValues.push_back(Value(std::move(e)));
    }
    add(o, "panels", Value(std::move(panelValues)));
    Value::Array cs;
    for (const auto& c : controls) {
        Value::Object e;
        add(e, "place", machine::placeText(c.place));
        add(e, "peripheral", c.peripheral.value);
        add(e, "id", c.control.id.value);
        add(e, "form", c.control.form == machine::ControlForm::Key ? "key" : "switch");
        add(e, "on", c.control.on);
        add(e, "label", c.control.label);
        add(e, "shortLabel", c.control.shortLabel);
        cs.push_back(Value(std::move(e)));
    }
    add(o, "controls", Value(std::move(cs)));
    Value::Array as;
    for (const auto& a : actions) {
        Value::Object e;
        add(e, "command", a.command.value);
        addAvailability(e, a.available);
        as.push_back(Value(std::move(e)));
    }
    add(o, "actions", Value(std::move(as)));
    Value::Object product;
    for (const auto& p : productEntries) add(product, p.name.c_str(), p.value ? p.value->toValue() : Value());
    add(o, "product", Value(std::move(product)));
    return Value(std::move(o));
}

const PlaceEntry* ControlState::place(const machine::PlacePath& path) const {
    for (const auto& p : places)
        if (p.path == path) return &p;
    return nullptr;
}

std::vector<peripherals::ControlInfo> ControlState::controlsAt(const machine::PlacePath& path) const {
    std::vector<peripherals::ControlInfo> out;
    for (const auto& c : controls)
        if (c.place == path) out.push_back(c.control);
    return out;
}

const ActionEntry* ControlState::action(const sessions::CommandName& c) const {
    for (const auto& a : actions)
        if (a.command == c) return &a;
    return nullptr;
}

const MediaPlaceEntry* ControlState::mediaPlace(const machine::MediaPlaceId& id,
                                                const machine::PlacePath& ownerPlace) const {
    for (const auto& m : mediaPlaces)
        if (m.id == id && m.ownerPlace == ownerPlace) return &m;
    return nullptr;
}

const ProductEntry* ControlState::productEntry(const std::string& name) const {
    for (const auto& p : productEntries)
        if (p.name == name) return &p;
    return nullptr;
}

const PeripheralChoice* choice(const PlaceEntry& place, const machine::PeripheralId& id) {
    for (const auto& c : place.fitting)
        if (c.id == id) return &c;
    return nullptr;
}

} // namespace calcu::control
