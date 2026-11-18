#pragma once
#include "machine/Descriptor.hpp"
#include "machine/Ids.hpp"
#include "machine/ProductData.hpp"
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

// Attach and detach: what a request to plug a peripheral
// in carries, and what the machine answers. The members are on
// calcu::machine::Machine (Machine.hpp).
//
// The pin-out data of Descriptor.hpp means:
// - PinOutDescriptor::level: contact level or functional; descriptive, read
//   by nobody in the library.
// - PinOutDescriptor::takes: One, the connector holds one peripheral over
//   all its places; Chain, one per place, all at once.
// - PinOutDescriptor::after: what follows an attach or a detach on it while
//   the machine is on.
// - PlaceDescriptor::takesAtOnce: how many the place holds.
// - PlaceDescriptor::pinOut: the one pin-out the place presents.
// - PeripheralDescriptor::fits: it fits a place whose pin-out is listed.

namespace calcu::machine {

struct RomImage {
    std::string fileName;
    std::vector<std::uint8_t> bytes;   // the file as bundled; the core decodes
};

// A switch position handed in with a peripheral: the position saved
// with it, or kept through a detach and attach (the PC-100C's TRACE switch).
struct SwitchPosition {
    ControlId id;
    bool on = false;
};

// Everything a peripheral needs comes in with the request; it reads no
// file.
struct PeripheralRequest {
    PlacePath place;
    PeripheralId peripheral;
    std::optional<VariantId> variant;   // std::nullopt: the default (or none)
    std::vector<RomImage> roms;
    std::shared_ptr<const ProductData> content;
    std::vector<SwitchPosition> switches;
};

// Why an attach is refused: an English sentence, from the
// library for the general cases and from the core for product rules.
struct Refusal {
    std::string reason;
    std::vector<std::string> missingRoms;   // the missing ROM files named
};

struct ChangeResult {
    std::optional<Refusal> refused;           // set: nothing changed
    AfterChange follows = AfterChange::Nothing;
};

// "slot-1", "a/jack": a place path as the sentences name it.
std::string placeText(const PlacePath&);

} // namespace calcu::machine
