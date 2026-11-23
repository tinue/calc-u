#pragma once
#include "control/ControlState.hpp"
#include "machine/Ids.hpp"
#include "machine/Machine.hpp"
#include "sessions/Messages.hpp"
#include <optional>
#include <string>
#include <variant>
#include <vector>

// What a product's app layer adds to the control state or refuses in it.
// The library asks these and never reads what they give back.

namespace calcu::control {

struct ModelKey {
    machine::ModelId id;
    bool operator==(const ModelKey&) const = default;
};
struct VariantKey {   // a variant of the running model
    machine::VariantId id;
    bool operator==(const VariantKey&) const = default;
};
struct PlaceKey {   // the place as a whole: its reason passes to all its choices
    machine::PlacePath path;
    bool operator==(const PlaceKey&) const = default;
};
struct ChoiceKey {
    machine::PlacePath path;
    std::optional<machine::PeripheralId> peripheral;   // nullopt: the empty place
    std::optional<machine::VariantId> variant;         // nullopt: the peripheral's entry itself
    bool operator==(const ChoiceKey&) const = default;
};
struct InsertKey {
    machine::MediaPlaceId id;
    machine::PlacePath ownerPlace;
    bool operator==(const InsertKey&) const = default;
};
struct ActionKey {
    sessions::CommandName command;
    bool operator==(const ActionKey&) const = default;
};
using EntryKey = std::variant<ModelKey, VariantKey, PlaceKey, ChoiceKey, InsertKey, ActionKey>;

// Implemented by a product's app layer.
class ControlRules {
public:
    // A reason of the product's own why the entry cannot be chosen now, or none.
    virtual std::optional<std::string> unavailable(const EntryKey&, const machine::Machine&) const {
        return std::nullopt;
    }
    // The product's own entries, appended in order; names carry the product's
    // prefix. The ControlState holds every other entry, as computed now.
    virtual void productEntries(const machine::Machine&, const ControlState&, std::vector<ProductEntry>&) const {}

protected:
    ~ControlRules() = default;
};

} // namespace calcu::control
