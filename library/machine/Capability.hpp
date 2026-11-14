#pragma once
#include "machine/Ids.hpp"
#include <compare>
#include <string_view>
#include <variant>
#include <vector>

// Capabilities (LIB-1 design §3.7; machine.md M-5, M-7; capabilities.md C-1).
// A kind is a class that derives from Capability and has
// `static constexpr KindId kKind`. Generic kinds live in their feature's
// library area, product kinds in the product's core with the product's
// prefix. The library compares KindId values only and names no kind
// (capabilities.md I-1).

namespace calcu::machine {

struct KindId {
    std::string_view value;
    auto operator<=>(const KindId&) const = default;
};

class Capability {
public:
    virtual ~Capability() = default;
    virtual KindId kind() const = 0;
};

struct ComponentOwner  { ComponentId component; bool operator==(const ComponentOwner&) const = default; };
struct PeripheralOwner { PeripheralId peripheral; PlacePath place; bool operator==(const PeripheralOwner&) const = default; };
using Owner = std::variant<ComponentOwner, PeripheralOwner>;

// The ID is unique within a machine and the same for machines built alike
// (AC-21): "<place-path>/<kind>" for a peripheral, "<component>/<kind>" for
// a component, with an index only where an owner has two of one kind.
struct CapabilityEntry { KindId kind; CapabilityId id; Owner owner; };

// The typed interface of kind K under `id`, or nullptr when it is absent or
// of another kind. M is the machine (calcu::machine::Machine); a template
// parameter so that this header needs no Machine.
template <class K, class M> K* capabilityAs(M& machine, const CapabilityId& id) {
    Capability* c = machine.capability(id);
    return c && c->kind() == K::kKind ? static_cast<K*>(c) : nullptr;
}

// The capabilities of kind K present now; empty if none (EC-5).
template <class K, class M> std::vector<CapabilityEntry> capabilitiesOf(const M& machine) {
    return machine.capabilities(K::kKind);
}

} // namespace calcu::machine
