#pragma once
#include <compare>
#include <string>
#include <vector>

// The strong ID types of the machine interface (LIB-1 design §3.1). An ID is
// a string with a type: presets, the harness and the control bar name them
// (docs/formats/preset.md); the advance loop never touches them.

namespace calcu::machine {

template <class Tag> struct Id {
    std::string value;
    auto operator<=>(const Id&) const = default;
};

using ModelId       = Id<struct ModelTag>;        // "TI-59", "PC-1500A"
using ModelFamilyId = Id<struct FamilyTag>;
using VariantId     = Id<struct VariantTag>;      // "A04", "New"
using KeyId         = Id<struct KeyTag>;          // "2nd", "enter"
using PlaceName     = Id<struct PlaceTag>;        // "slot-1", "printer"
using ConnectorId   = Id<struct ConnectorTag>;
using PinOutId      = Id<struct PinOutTag>;
using ComponentId   = Id<struct ComponentTag>;
using PeripheralId  = Id<struct PeripheralTag>;   // "PC-100C", "CE-161"
using MediaPlaceId  = Id<struct MediaPlaceTag>;
using ControlId     = Id<struct ControlTag>;
using CapabilityId  = Id<struct CapabilityTag>;
using DebugTargetId = Id<struct DebugTargetTag>;  // "TMC0501", "LH5801"

// {"printer"}; deeper for a place on a peripheral (LIB-2 AC-9). Empty: the
// machine itself.
struct PlacePath {
    std::vector<PlaceName> steps;
    auto operator<=>(const PlacePath&) const = default;
};

} // namespace calcu::machine
