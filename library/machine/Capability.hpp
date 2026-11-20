#pragma once
#include "machine/Ids.hpp"
#include <compare>
#include <string_view>
#include <variant>
#include <vector>

// Capabilities.
// A kind is a class that derives from Capability and has
// `static constexpr KindId kKind`. Generic kinds live in their feature's
// library area, product kinds in the product's core with the product's
// prefix. The library compares KindId values only and names no kind.

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

// The ID is unique within a machine and the same for machines built alike:
// "<place-path>/<kind>" for a peripheral, "<component>/<kind>" for
// a component, with an index only where an owner has two of one kind.
struct CapabilityEntry { KindId kind; CapabilityId id; Owner owner; };

// The typed interface of kind K under `id`, or nullptr when it is absent or
// of another kind. M is the machine (calcu::machine::Machine); a template
// parameter so that this header needs no Machine.
template <class K, class M> K* capabilityAs(M& machine, const CapabilityId& id) {
    Capability* c = machine.capability(id);
    return c && c->kind() == K::kKind ? static_cast<K*>(c) : nullptr;
}
template <class K, class M> const K* capabilityAs(const M& machine, const CapabilityId& id) {
    const Capability* c = machine.capability(id);
    return c && c->kind() == K::kKind ? static_cast<const K*>(c) : nullptr;
}

// The first capability of kind K the machine has now, or nullptr: the one
// form of "the machine's K" for library and products.
template <class K, class M> K* first(M& machine) {
    for (const auto& e : machine.capabilities(K::kKind))
        if (K* k = capabilityAs<K>(machine, e.id)) return k;
    return nullptr;
}
template <class K, class M> const K* first(const M& machine) {
    for (const auto& e : machine.capabilities(K::kKind))
        if (const K* k = capabilityAs<K>(machine, e.id)) return k;
    return nullptr;
}

} // namespace calcu::machine
