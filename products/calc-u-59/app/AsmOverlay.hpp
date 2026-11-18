#pragma once
#include <cstdint>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

// The ASM overlay format, version 2.0 (examples/assembly/README.md):
// hand-assembled TMC0501
// code put over the ROM at 1800 to 1FFF (the product kind c59.asm-overlay).
//
//   Calc-U-59 ASM 2.0          ← exactly, the first line
//   …                          ← for the reader, not read
//   HEX:                       ← any case, alone on its line
//   01D8 01DB 0x0D00 ; a comment
//   0A3718051007               ← a run of 4n digits is n instructions
//
// Lines end with LF or CRLF. After HEX: a `;` starts a comment to the end of
// its line; what remains is runs of hexadecimal digits (either case, each
// optionally after 0x) parted by spaces and line ends. Each instruction is
// 0000 to 1FFF; the first goes to 1800.

namespace cu59 {

inline constexpr std::string_view kAsmOverlayFirstLine = "Calc-U-59 ASM 2.0";

/// The instructions, or the sentence of the refusal,
/// which names the file by `name`.
std::variant<std::vector<std::uint16_t>, std::string> readAsmOverlay(std::string_view name, std::string_view text);

} // namespace cu59
