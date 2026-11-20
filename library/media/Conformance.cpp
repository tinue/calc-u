#include "media/Conformance.hpp"
#include "media/MediaPlace.hpp"
#include <optional>
#include <variant>

namespace calcu::media {

namespace {

// The pin-out of a place: the model's own, or one of the peripheral plugged
// at the path before it.
std::optional<machine::PinOutId> pinOutAt(const machine::ModelId& model, const machine::PlacePath& path,
                                          const machine::Plugged& plugged, const machine::Catalogue& cat) {
    if (path.steps.empty()) return std::nullopt;
    if (path.steps.size() == 1) {
        const auto info = cat.place(model, path.steps[0]);
        return info && info->place ? std::optional<machine::PinOutId>(info->place->pinOut) : std::nullopt;
    }
    machine::PlacePath host = path;
    host.steps.pop_back();
    for (const auto& e : plugged.byPlace)
        if (e.place == host) {
            const auto info = cat.place(e.peripheral, path.steps.back());
            return info && info->place ? std::optional<machine::PinOutId>(info->place->pinOut) : std::nullopt;
        }
    return std::nullopt;
}

} // namespace

std::vector<std::string> checkMediaPlaces(machine::Machine& m, const machine::Catalogue& cat) {
    std::vector<std::string> out;
    struct Want {
        machine::MediaPlaceId id;
        machine::PlacePath owner;   // empty: the machine
        std::string ownerText;
        int found = 0;
    };
    std::vector<Want> want;
    for (const auto& p : m.descriptor().mediaPlaces) want.push_back(Want{p.id, {}, "the machine", 0});
    const machine::Plugged plugged = m.plugged();
    for (const auto& e : plugged.byPlace)
        if (const machine::PeripheralDescriptor* d = cat.peripheral(e.peripheral)) {
            const std::optional<machine::PinOutId> at = pinOutAt(m.model(), e.place, plugged, cat);
            for (const auto& p : d->mediaPlaces)
                if (at && p.existsAt(*at))
                    want.push_back(Want{p.id, e.place, e.peripheral.value + " in " + machine::placeText(e.place), 0});
        }
    for (const auto& e : m.capabilities(MediaPlace::kKind)) {
        auto* cap = machine::capabilityAs<MediaPlace>(m, e.id);
        if (!cap) {
            out.push_back("media place: " + e.id.value + " is not of the kind media-place");
            continue;
        }
        const auto* owner = std::get_if<machine::PeripheralOwner>(&e.owner);
        const machine::PlacePath at = owner ? owner->place : machine::PlacePath{};
        bool matched = false;
        for (Want& w : want)
            if (w.owner == at && w.id == cap->id()) {
                ++w.found;
                matched = true;
            }
        if (!matched) out.push_back("media place: " + e.id.value + " (" + cap->id().value + ") is in no descriptor");
    }
    for (const Want& w : want) {
        if (w.found == 0) out.push_back("media place: " + w.id.value + " of " + w.ownerText + " has no capability");
        if (w.found > 1) out.push_back("media place: " + w.id.value + " of " + w.ownerText + " has more than one capability");
    }
    return out;
}

} // namespace calcu::media
