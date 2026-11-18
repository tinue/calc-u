#pragma once
#include "machine/Descriptor.hpp"
#include "typing/TypingProfile.hpp"
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

// Key names to presses, once for the `key` command, the `key:` step and the
// harness's key-down. Each caller words its own refusal.

namespace calcu::typing {

// The key a name stands for without a profile: the key ID, then an ID
// in any case, then another spelling of a key in any case; nullptr when none.
const machine::KeyDescriptor* keyNamed(const machine::MachineDescriptor&, std::string_view name);

// The presses of one name: the profile's `name` when it has one, else the
// key keyNamed finds; nullopt when the name is no key.
std::optional<std::vector<Press>> pressesOfName(const TypingProfile*, const machine::MachineDescriptor&,
                                                std::string_view name);

// Why a list of names gives no presses: `name` is no key, or (together) the
// profile's `names` refuses the names as a whole (`name` the first).
struct KeyNamesRefused {
    std::string name;
    bool together = false;
};
// The presses of the names, name by name, then by the profile's `names` when
// it has one (all or nothing).
std::variant<std::vector<Press>, KeyNamesRefused> pressesOfNames(const TypingProfile*, const machine::MachineDescriptor&,
                                                                 const std::vector<std::string>& names);

} // namespace calcu::typing
