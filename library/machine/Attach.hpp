#pragma once
#include "machine/Descriptor.hpp"
#include "machine/Ids.hpp"
#include "machine/ProductData.hpp"
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

// Attach and detach (LIB-2 design §3.1; spec AC-4 to AC-11, AC-35, EC-1 to
// EC-4; peripherals.md P-1, P-5, P-6): what a request to plug a peripheral
// in carries, and what the machine answers. The members are on
// calcu::machine::Machine (Machine.hpp).
//
// The pin-out data of Descriptor.hpp means (AC-1):
// - PinOutDescriptor::level: contact level or functional; descriptive, read
//   by nobody in the library (P-4, I-4).
// - PinOutDescriptor::takes: One, the connector holds one peripheral over
//   all its places; Chain, one per place, all at once (AC-8, P-6).
// - PinOutDescriptor::after: what follows an attach or a detach on it while
//   the machine is on (P-5).
// - PlaceDescriptor::takesAtOnce: how many the place holds (AC-3, M-8).
// - PlaceDescriptor::pinOut: the one pin-out the place presents (AC-2).
// - PeripheralDescriptor::fits: it fits a place whose pin-out is listed.

namespace calcu::machine {

struct RomImage {
    std::string fileName;
    std::vector<std::uint8_t> bytes;   // the file as bundled; the core decodes (D-4)
};

// A switch position handed in with a peripheral (D-3): the position saved
// with it, or kept through a detach and attach (C59-6 AC-6).
struct SwitchPosition {
    ControlId id;
    bool on = false;
};

// Everything a peripheral needs comes in with the request; it reads no
// file (AC-11).
struct PeripheralRequest {
    PlacePath place;
    PeripheralId peripheral;
    std::optional<VariantId> variant;   // std::nullopt: the default (or none)
    std::vector<RomImage> roms;
    std::shared_ptr<const ProductData> content;
    std::vector<SwitchPosition> switches;
};

// Why an attach is refused: an English sentence (spec OQ-1; D-7), from the
// library for the general cases and from the core for product rules.
struct Refusal {
    std::string reason;
    std::vector<std::string> missingRoms;   // EC-4: the files named
};

struct ChangeResult {
    std::optional<Refusal> refused;           // set: nothing changed (AC-6)
    AfterChange follows = AfterChange::Nothing;   // AC-4, AC-5, AC-35
};

// "slot-1", "a/jack": a place path as the sentences name it.
std::string placeText(const PlacePath&);

} // namespace calcu::machine
