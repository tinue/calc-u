#include "typing/KeyNames.hpp"
#include "machine/Names.hpp"

namespace calcu::typing {

const machine::KeyDescriptor* keyNamed(const machine::MachineDescriptor& d, std::string_view name) {
    for (const auto& k : d.keys)
        if (k.id.value == name) return &k;
    for (const auto& k : d.keys)
        if (machine::sameIgnoringCase(k.id.value, name)) return &k;
    for (const auto& k : d.keys)
        for (const auto& o : k.otherSpellings)
            if (machine::sameIgnoringCase(o, name)) return &k;
    return nullptr;
}

namespace {

std::optional<std::vector<Press>> pressesOfName(const TypingProfile* p, const machine::MachineDescriptor& d,
                                                std::string_view name) {
    if (p && p->name) return p->name(name);
    if (const machine::KeyDescriptor* k = keyNamed(d, name)) return std::vector<Press>{Press{k->id}};
    return std::nullopt;
}

} // namespace

std::variant<std::vector<Press>, KeyNamesRefused> pressesOfNames(const TypingProfile* p,
                                                                 const machine::MachineDescriptor& d,
                                                                 const std::vector<std::string>& names) {
    std::vector<Press> presses;
    for (const std::string& n : names) {
        const auto one = pressesOfName(p, d, n);
        if (!one) return KeyNamesRefused{n, false};
        presses.insert(presses.end(), one->begin(), one->end());
    }
    if (p && p->names) {
        auto all = p->names(names);
        if (!all) return KeyNamesRefused{names.empty() ? std::string() : names.front(), true};
        presses = std::move(*all);
    }
    return presses;
}

} // namespace calcu::typing
