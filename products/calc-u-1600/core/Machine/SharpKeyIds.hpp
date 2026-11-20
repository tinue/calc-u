#pragma once
#include <string>
#include <vector>

// The key IDs of the Sharp machines. The machine takes exactly these: no
// other spelling (`ent`, upper case, `break`), which presets and the
// harness resolve. `on` is a key ID like the others, though the core wires it to the ON/BREAK line and not to the key matrix.

namespace sharp {

// PC-1500 and PC-1500A: 65 keys, in the order of the descriptor.
const std::vector<std::string>& pc1500KeyIds();
// PC-1600: the 65 above and `ctrl`, `kbii`, `bs` (68).
const std::vector<std::string>& pc1600KeyIds();

inline constexpr const char* kOnKeyId = "on";
// The other spelling of `on`, for presets and the harness (every key step:
// `key`, `key-down`, `key-up`).
inline constexpr const char* kBreakSpelling = "break";

} // namespace sharp
