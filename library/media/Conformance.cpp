#include "media/Conformance.hpp"
#include "media/MediaPlace.hpp"
#include <variant>

namespace calcu::media {

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
    for (const auto& e : m.plugged().byPlace)
        if (const machine::PeripheralDescriptor* d = cat.peripheral(e.peripheral))
            for (const auto& p : d->mediaPlaces)
                want.push_back(Want{p.id, e.place, e.peripheral.value + " in " + machine::placeText(e.place), 0});
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
