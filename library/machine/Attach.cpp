#include "machine/Attach.hpp"
#include "machine/Machine.hpp"
#include <algorithm>

// The generic rules of an attach (LIB-2 design §3.1). Everything a
// peripheral needs comes in with the request; nothing here reads a file
// (AC-11).

namespace calcu::machine {

std::string placeText(const PlacePath& path) {
    std::string out;
    for (const auto& s : path.steps) {
        if (!out.empty()) out += '/';
        out += s.value;
    }
    return out;
}

namespace {

PlacePath prefixOf(const PlacePath& p) {
    PlacePath out = p;
    if (!out.steps.empty()) out.steps.pop_back();
    return out;
}

const PlugEntry* entryAt(const Plugged& plugged, const PlacePath& place, const PeripheralId* id = nullptr) {
    for (const auto& e : plugged.byPlace)
        if (e.place == place && (!id || e.peripheral == *id)) return &e;
    return nullptr;
}

// The place a path names: on the machine for one step, on the peripheral
// plugged in at the path's prefix otherwise (AC-9).
std::optional<PlaceInfo> placeAt(const Machine& m, const Plugged& plugged, const PlacePath& path,
                                 const Catalogue& cat) {
    if (path.steps.empty()) return std::nullopt;
    const PlaceName& name = path.steps.back();
    if (path.steps.size() == 1) {
        const MachineDescriptor& d = m.descriptor();
        const PlaceDescriptor* p = d.place(name);
        const ConnectorDescriptor* c = p ? d.connector(p->connector) : nullptr;
        if (!c) return std::nullopt;
        for (const auto& o : c->pinOuts)
            if (o.id == p->pinOut) return PlaceInfo{p, c, &o, {}};
        return std::nullopt;
    }
    const PlugEntry* host = entryAt(plugged, prefixOf(path));
    return host ? cat.place(host->peripheral, name) : std::nullopt;
}

std::string nameOf(const Catalogue& cat, const PeripheralId& id) {
    const PeripheralDescriptor* d = cat.peripheral(id);
    return d && !d->name.empty() ? d->name : id.value;
}

// The variant to plug: the one named, or the default; false when it does
// not belong to the peripheral (the same rule as build()).
bool resolveVariant(const PeripheralDescriptor& d, std::optional<VariantId>& variant) {
    if (d.variants.empty()) return !variant;
    if (!variant) variant = d.defaultVariant;
    return variant && std::any_of(d.variants.begin(), d.variants.end(),
                                  [&](const VariantDescriptor& v) { return v.id == *variant; });
}

// The connector a plugged place is on, or nullptr.
const ConnectorDescriptor* connectorOf(const Machine& m, const Plugged& plugged, const PlacePath& place,
                                       const Catalogue& cat) {
    const auto info = placeAt(m, plugged, place, cat);
    return info ? info->connector : nullptr;
}

Refusal refusal(std::string reason) { return Refusal{std::move(reason), {}}; }

} // namespace

std::optional<Refusal> Machine::check(const PeripheralRequest& req, const Catalogue& cat,
                                      const std::optional<PeripheralId>& replacing) const {
    const Plugged now = plugged();
    const std::string where = placeText(req.place);
    // 1. The place exists.
    const auto place = placeAt(*this, now, req.place, cat);
    if (!place) return refusal("The " + model().value + " has no place " + where + ".");
    // 2. The peripheral is known, also when added after the catalogue was built.
    const PeripheralDescriptor* d = cat.peripheral(req.peripheral);
    if (!d) return refusal(req.peripheral.value + " is not a known peripheral.");
    const std::string name = d->name.empty() ? d->id.value : d->name;
    // 3. It fits the place's pin-out (EC-1).
    if (!d->fitsPinOut(place->place->pinOut)) return refusal("The " + name + " does not fit " + where + ".");
    // 4. The variant belongs to it.
    std::optional<VariantId> variant = req.variant;
    if (!resolveVariant(*d, variant))
        return refusal("The " + name + " has no variant " + (req.variant ? req.variant->value : std::string()) + ".");
    // 5. Every ROM entry of the variant has an image (EC-4).
    std::vector<std::string> missing;
    for (const auto& e : d->romsFor(variant))
        if (std::none_of(req.roms.begin(), req.roms.end(), [&](const RomImage& i) { return i.fileName == e.fileName; }))
            missing.push_back(e.fileName);
    if (!missing.empty()) {
        std::string files;
        for (const auto& f : missing) files += (files.empty() ? "" : ", ") + f;
        return Refusal{"The " + name + " could not be attached: " + files + (missing.size() == 1 ? " is" : " are") +
                           " missing.",
                       std::move(missing)};
    }
    // 6. Room in the place, and on a Takes::One connector in no other place
    // (AC-3, AC-7). The peripheral a replace takes out does not count.
    const PlugEntry* first = nullptr;
    int held = 0;
    for (const auto& e : now.byPlace) {
        if (e.place != req.place || (replacing && e.peripheral == *replacing)) continue;
        if (!first) first = &e;
        ++held;
    }
    if (held >= place->place->takesAtOnce && first)
        return refusal(where + " already holds the " + nameOf(cat, first->peripheral) + ".");
    if (held >= place->place->takesAtOnce) return refusal(where + " takes no peripheral.");
    if (place->pinOut->takes == Takes::One) {
        const PlacePath owner = prefixOf(req.place);
        for (const auto& e : now.byPlace) {
            if (e.place == req.place || prefixOf(e.place) != owner || e.place.steps.size() != req.place.steps.size())
                continue;
            if (const ConnectorDescriptor* c = connectorOf(*this, now, e.place, cat); c && c->id == place->connector->id)
                return refusal(placeText(e.place) + " already holds the " + nameOf(cat, e.peripheral) + ".");
        }
    }
    // 7. The product's rules (AC-29).
    if (auto why = productRefusal(req, replacing)) return refusal(std::move(*why));
    return std::nullopt;
}

std::optional<Refusal> Machine::canAttach(const PeripheralRequest& req, const Catalogue& cat) const {
    return check(req, cat, std::nullopt);
}

ChangeResult Machine::attach(PeripheralRequest req, const Catalogue& cat) {
    if (auto r = check(req, cat, std::nullopt)) return ChangeResult{std::move(r), AfterChange::Nothing};
    const auto place = placeAt(*this, plugged(), req.place, cat);
    const PeripheralDescriptor& d = *cat.peripheral(req.peripheral);
    resolveVariant(d, req.variant);
    const PinOutDescriptor pinOut = *place->pinOut;   // a copy: plug() may change what the pointers reach
    plug(std::move(req), d, descriptor().tickRate);
    return ChangeResult{std::nullopt, followsNow(pinOut)};
}

ChangeResult Machine::detach(const PlacePath& place, const PeripheralId& id, const Catalogue& cat) {
    const Plugged now = plugged();
    if (!entryAt(now, place, &id)) return ChangeResult{};   // EC-2
    const auto info = placeAt(*this, now, place, cat);
    unplug(place, id);
    return ChangeResult{std::nullopt, info ? followsNow(*info->pinOut) : AfterChange::Nothing};
}

std::optional<Refusal> Machine::canReplace(const PeripheralId& old, const PeripheralRequest& req,
                                           const Catalogue& cat) const {
    if (!entryAt(plugged(), req.place, &old))
        return refusal(placeText(req.place) + " does not hold the " + nameOf(cat, old) + ".");
    return check(req, cat, old);
}

ChangeResult Machine::replace(const PeripheralId& old, PeripheralRequest req, const Catalogue& cat) {
    if (auto r = canReplace(old, req, cat)) return ChangeResult{std::move(r), AfterChange::Nothing};
    const auto place = placeAt(*this, plugged(), req.place, cat);
    const PinOutDescriptor pinOut = *place->pinOut;
    const PeripheralDescriptor& d = *cat.peripheral(req.peripheral);
    resolveVariant(d, req.variant);
    unplug(req.place, old);
    plug(std::move(req), d, descriptor().tickRate);
    return ChangeResult{std::nullopt, followsNow(pinOut)};   // asked once (AC-35)
}

std::optional<std::string> Machine::productRefusal(const PeripheralRequest&, const std::optional<PeripheralId>&) const {
    return std::nullopt;
}

void Machine::plug(PeripheralRequest, const PeripheralDescriptor&, TickRate) {}

void Machine::unplug(const PlacePath&, const PeripheralId&) {}

AfterChange Machine::followsNow(const PinOutDescriptor& p) const { return p.after; }

} // namespace calcu::machine
