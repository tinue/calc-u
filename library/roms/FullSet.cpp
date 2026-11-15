#include "roms/FullSet.hpp"
#include <algorithm>

namespace calcu::roms {

namespace {

bool sameForm(const machine::RomEntry& a, const machine::RomEntry& b) {
    return a.encoding == b.encoding && a.decodedAmount == b.decodedAmount && a.unit == b.unit && a.md5 == b.md5;
}

void add(FullSet& set, const machine::RomEntry& entry, const NeededBy& who) {
    auto it = std::find_if(set.files.begin(), set.files.end(),
                           [&](const FullSetEntry& f) { return f.entry.fileName == entry.fileName; });
    if (it == set.files.end()) {
        set.files.push_back({entry, {who}});
        return;
    }
    if (!sameForm(it->entry, entry) &&
        std::find(set.catalogueErrors.begin(), set.catalogueErrors.end(), entry.fileName) == set.catalogueErrors.end())
        set.catalogueErrors.push_back(entry.fileName);
    if (std::find(it->neededBy.begin(), it->neededBy.end(), who) == it->neededBy.end()) it->neededBy.push_back(who);
}

template <class Descriptor, class Id>
void addAll(FullSet& set, const Descriptor& d, const Id& id) {
    if (d.variants.empty()) {
        for (const auto& e : d.roms) add(set, e, NeededBy{id, std::nullopt});
        return;
    }
    for (const auto& v : d.variants)
        for (const auto& e : v.roms) add(set, e, NeededBy{id, v.id});
}

} // namespace

FullSet fullSet(const machine::Catalogue& catalogue, std::span<const FullSetEntry> extra) {
    FullSet set;
    for (const auto& m : catalogue.models()) addAll(set, m, m.model);
    for (const auto& p : catalogue.peripherals()) addAll(set, p, p.id);
    for (const auto& x : extra)
        for (const auto& who : x.neededBy) add(set, x.entry, who);
    return set;
}

std::string describe(const NeededBy& n) {
    std::string s = std::visit([](const auto& id) { return id.value; }, n.who);
    if (n.variant) s += " (variant " + n.variant->value + ")";
    return s;
}

} // namespace calcu::roms
