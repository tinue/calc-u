#pragma once
#include "machine/Descriptor.hpp"
#include "machine/Ids.hpp"
#include "peripherals/Controls.hpp"
#include "sessions/Messages.hpp"
#include "sessions/Payload.hpp"
#include "sessions/Value.hpp"
#include <optional>
#include <string>
#include <vector>

// The control state: what the controls around the machine offer now, computed from the descriptors and
// the running machine (Compute.hpp) and sent as the state topic
// `control-state` (ControlPart.hpp). Every entry is named as commands and
// presets name it: model and variant IDs, a place path as placeText writes
// it, peripheral IDs, media place IDs, command names. Nothing here names a
// model, a peripheral type or a product.

namespace calcu::control {

inline const sessions::TopicName kControlStateTopic{"control-state"};

// Available now, or not, with one reason (the first that applies, in the
// order of Compute.hpp).
struct Availability {
    bool now = true;
    std::string reason;   // one English sentence when !now
    bool operator==(const Availability&) const = default;
};

struct VariantChoice {
    machine::VariantId id;
    Availability available;
    bool operator==(const VariantChoice&) const = default;
};

struct ModelEntry {
    machine::ModelId id;
    Availability available;
    bool operator==(const ModelEntry&) const = default;
};

// One peripheral that fits a place.
struct PeripheralChoice {
    machine::PeripheralId id;
    std::string name;                            // the descriptor's name
    bool plugged = false;
    std::optional<machine::VariantId> variant;   // plugged: the one that runs; else the one an attach uses
    std::vector<VariantChoice> variants;         // empty: the peripheral has none
    Availability available;                      // not plugged: its attach (a replace when the place is full); plugged: its detach
    bool operator==(const PeripheralChoice&) const = default;
};

struct PlaceEntry {
    machine::PlacePath path;   // {"printer"}; {"a", "jack"} on a peripheral
    machine::ConnectorId connector;
    machine::PinOutId pinOut;
    Availability available;                  // the place as a whole: only a product rule refuses it
    Availability empty;                      // the empty place as a choice; every place has one
    std::vector<PeripheralChoice> fitting;   // catalogue order; modules read later at the end
    bool operator==(const PlaceEntry&) const = default;
};

struct MediaPlaceEntry {
    machine::MediaPlaceId id;
    machine::PlacePath ownerPlace;                // empty: the machine
    std::optional<machine::PeripheralId> owner;   // the peripheral plugged in at ownerPlace
    std::string mediumKind;
    machine::MediaPassage passage = machine::MediaPassage::PassThrough;
    std::vector<int> sides;
    Availability insert;   // the action "insert a medium" here
    bool operator==(const MediaPlaceEntry&) const = default;
};

// "paper" is the only panel the library knows.
inline constexpr const char* kPaperPanel = "paper";
struct PanelEntry {
    std::string kind;
    std::optional<machine::PlacePath> ownerPlace;
    std::optional<machine::PeripheralId> owner;   // the plugged-in printer, or none
    bool operator==(const PanelEntry&) const = default;
};

// One control of a plugged-in peripheral, named by its
// owner, with its form, its position (a key down, a switch on) and the
// labels every GUI writes on it.
struct ControlEntry {
    machine::PlacePath place;
    machine::PeripheralId peripheral;
    peripherals::ControlInfo control;
    bool operator==(const ControlEntry&) const = default;
};

// The actions that start an operation and are not a choice among entries,
// by their command names (reset, cold-boot, load-preset, open), in this
// order.
inline const sessions::CommandName kResetAction{"reset"};
inline const sessions::CommandName kColdBootAction{"cold-boot"};
inline const sessions::CommandName kLoadPresetAction{"load-preset"};
inline const sessions::CommandName kOpenAction{"open"};

struct ActionEntry {
    sessions::CommandName command;
    Availability available;
    bool operator==(const ActionEntry&) const = default;
};

// A product's own entry: carried unread. The name
// carries the product's prefix ("c1600.disk-controls").
struct ProductEntry {
    std::string name;
    sessions::PayloadPtr value;
};

struct ControlState final : sessions::PayloadOf<ControlState> {
    static constexpr sessions::PayloadKind kKind{"control-state"};
    std::vector<ModelEntry> models;
    machine::ModelId model;
    std::vector<VariantChoice> variants;
    std::optional<machine::VariantId> variant;
    std::vector<PlaceEntry> places;
    std::vector<MediaPlaceEntry> mediaPlaces;
    std::vector<PanelEntry> panels;
    std::vector<ControlEntry> controls;   // in the order of the place walk, each owner's in its descriptor's
    std::vector<ActionEntry> actions;
    std::vector<ProductEntry> productEntries;

    // The fields by ==, the product entries by name and Payload::equals.
    bool same(const ControlState&) const;
    // One object, keys in the order of the fields; `reason` only where
    // `available` is false; `controls` as [{place, peripheral, id, form, on, label, shortLabel}];
    // `product` as {name: value}.
    sessions::Value toValue() const override;

    // Lookups for translators and tests; nullptr when not listed.
    const PlaceEntry* place(const machine::PlacePath&) const;
    // The controls of the peripheral plugged into a place, in the
    // order of `controls`; empty when none is plugged in or it has none.
    std::vector<peripherals::ControlInfo> controlsAt(const machine::PlacePath&) const;
    const ActionEntry* action(const sessions::CommandName&) const;
    const MediaPlaceEntry* mediaPlace(const machine::MediaPlaceId&, const machine::PlacePath& ownerPlace = {}) const;
    const ProductEntry* productEntry(const std::string& name) const;
};

// A choice of a place entry by peripheral, or nullptr.
const PeripheralChoice* choice(const PlaceEntry&, const machine::PeripheralId&);

} // namespace calcu::control
