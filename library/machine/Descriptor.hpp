#pragma once
#include "machine/EmulatedTime.hpp"
#include "machine/Ids.hpp"
#include "machine/ProductData.hpp"
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

// Machine and peripheral descriptors (LIB-1 design §3.3; machine.md M-4,
// M-8): plain values a product builds without a machine and without ROM
// files (AC-1). What is plugged in and present now is asked from the
// running machine, never written into a descriptor.

namespace calcu::machine {

// A ROM entry (roms.md R-1, R-4). The form is LIB-12's; defined here because
// descriptors carry it from M-18.
enum class RomEncoding { Binary, Text };
enum class ContentUnit { Bytes, Words, Rows };
struct RomEntry {
    std::string fileName;
    RomEncoding encoding;
    std::uint64_t decodedAmount;   // size of the decoded content, in `unit`
    ContentUnit unit;
    std::string md5;               // of the known dump
};
struct VariantDescriptor { VariantId id; std::vector<RomEntry> roms; };

// Display kinds (machine.md M-2).
struct SegmentDef    { std::string name; bool ownBrightness; };   // "A".."G", "DP"
struct SegmentLayout { std::vector<SegmentDef> segments; };        // any number; names and order (D-3)
struct IndicatorDef  { std::string name; bool hasBrightness; };
struct SegmentGeometry   { int positions; SegmentLayout layout; bool positionBrightness; };
struct CellGrouping      { int cellWidth, cellHeight, columns, rows; };
struct DotMatrixGeometry { int width, height; std::optional<CellGrouping> cells; };
struct DisplayDescriptor {
    std::variant<SegmentGeometry, DotMatrixGeometry> geometry;     // the display kind
    std::vector<IndicatorDef> indicators;
};

// Other spellings are resolved by presets and the harness, not by the
// machine (D-2).
struct KeyDescriptor { KeyId id; std::vector<std::string> otherSpellings; };

// Connectors, pin-outs and places (peripherals.md P-3, P-6).
enum class PinOutLevel { ContactLevel, Functional };
enum class Takes { One, Chain };
enum class AfterChange { PowerCycle, Nothing };   // LIB-2 AC-1; the semantics are LIB-2's
struct PinOutDescriptor    { PinOutId id; PinOutLevel level; Takes takes; AfterChange after; };
struct ConnectorDescriptor { ConnectorId id; std::string form; std::vector<PinOutDescriptor> pinOuts; };
struct PlaceDescriptor     { PlaceName name; ConnectorId connector; PinOutId pinOut; int takesAtOnce; };

// Components and their modelling depth (machine.md I-4).
enum class Depth { Separate, Folded, Functional };
struct ComponentDescriptor {
    ComponentId id;
    std::string standsFor;                  // the chip or block
    Depth depth;
    std::optional<ComponentId> foldedInto;  // set for Depth::Folded
};

// Media places; the medium kinds are LIB-7's ("magnetic-card", ...).
enum class MediaPassage { PassThrough, Holding };
struct MediaPlaceDescriptor { MediaPlaceId id; std::string mediumKind; MediaPassage passage; std::vector<int> sides; };

// A peripheral's keys and switches (LIB-2 AC-14); never machine keys (AC-14).
enum class ControlForm { Key, Switch };
struct ControlDescriptor { ControlId id; ControlForm form; };

struct MachineDescriptor {
    ModelId model;
    ModelFamilyId family;
    std::vector<VariantDescriptor> variants;    // empty: no variant (EC-6)
    std::optional<VariantId> defaultVariant;    // the only one when there is one (EC-6)
    std::vector<RomEntry> roms;                 // used when `variants` is empty
    TickRate tickRate = 0;
    DisplayDescriptor display;
    bool hasSound = false;
    std::vector<KeyDescriptor> keys;
    std::vector<ConnectorDescriptor> connectors;
    std::vector<PlaceDescriptor> places;
    std::vector<ComponentDescriptor> components;
    std::vector<MediaPlaceDescriptor> mediaPlaces;
    std::vector<DebugTargetId> debugTargets;

    // The ROM entries of a variant; std::nullopt is the default variant (or
    // no variant). An unknown variant gives an empty list.
    const std::vector<RomEntry>& romsFor(const std::optional<VariantId>&) const;
    const PlaceDescriptor* place(const PlaceName&) const;
    const ConnectorDescriptor* connector(const ConnectorId&) const;
    bool hasKey(const KeyId&) const;
};

struct PeripheralDescriptor {
    PeripheralId id;
    std::string name;
    std::vector<VariantDescriptor> variants;
    std::optional<VariantId> defaultVariant;
    std::vector<RomEntry> roms;                 // used when `variants` is empty
    std::vector<PinOutId> fits;                 // the pin-outs it fits (LIB-2 AC-2)
    std::vector<ConnectorDescriptor> connectors;   // its own (P-3)
    std::vector<PlaceDescriptor> places;
    std::vector<MediaPlaceDescriptor> mediaPlaces;
    std::vector<ControlDescriptor> controls;    // its keys and switches; never machine keys (AC-14)
    std::vector<ComponentDescriptor> components;
    std::shared_ptr<const ProductData> productData;   // e.g. the parsed module definition (§3.9)

    const std::vector<RomEntry>& romsFor(const std::optional<VariantId>&) const;
    bool fitsPinOut(const PinOutId&) const;
};

} // namespace calcu::machine
