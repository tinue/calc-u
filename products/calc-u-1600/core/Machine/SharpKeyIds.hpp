#pragma once
#include <string>
#include <vector>

// The key IDs of the Sharp machines (C1600-1 AC-9, AC-10, AC-11; LIB-1
// design §3.5 "keys"). The machine takes exactly these: no other spelling
// (`ent`, upper case, `break`), which presets and the harness resolve
// (LIB-5 AC-41, OQ-5). `on` is a key ID like the others, though the core
// wires it to the ON/BREAK line and not to the key matrix (AC-11).

namespace sharp {

// PC-1500 and PC-1500A: 65 keys, in the order of the descriptor.
const std::vector<std::string>& pc1500KeyIds();
// PC-1600: the 65 above and `ctrl`, `kbii`, `bs` (68).
const std::vector<std::string>& pc1600KeyIds();

inline constexpr const char* kOnKeyId = "on";

} // namespace sharp
