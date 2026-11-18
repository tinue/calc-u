#pragma once
#include "Keycodes.hpp"
#include <cstdint>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

// Assemblies of a keystroke program: one line per instruction, the step in three digits, two
// spaces, the mnemonic and its operands (`007  Dsz Ind 03 A`).
//
// The mnemonic of a keycode is the key ID of docs/formats/preset.md §8.1,
// the name the command `key` takes for that key: the one table is
// the typing's (app/TITyping, tiKeyNames); the assembler reads a mnemonic
// by the presses `key` makes for it. A merged code is its key and `Ind`
// (`STO Ind 05` for `72 05`), keycode 20 is `2nd CLR`, 92 is `RTN`.
// Three keycodes no key enters keep another form: 82 its printer name
// `HIR`, 26 and 27 (printed `2ND` and `INV` like 21 and 22) their two digits.
//
// A listing is the PC-100C's `List` form, `010 43 RCL`:
// app/KeyNames' printer names, an operand step as its two digits.

namespace cu59 {

/// The assembly mnemonic of a keycode 00–99 (above: its two digits).
std::string assemblyMnemonic(std::uint8_t keycode);

struct ListedLine {
    int step = 0;
    std::string text;
};

/// The program `steps` (step 0 first) from `first` to `last`, one line per
/// step as the PC-100C lists it (`010 43 RCL`). The grammar is read from
/// step 0, so an operand step is told from an opcode.
std::vector<ListedLine> listing(const std::vector<std::uint8_t>& steps, int first, int last);

/// The instructions that start from `first` to `last`, one line each
/// (`002  STO 05`); an operand past the end of `steps` is written `?`.
std::vector<ListedLine> disassemble(const std::vector<std::uint8_t>& steps, int first, int last);

/// The last step at or after `first` that is not `00`, or first - 1.
int lastUsedStep(const std::vector<std::uint8_t>& steps, int first);

/// An assembly read back: every keycode at its step, or the error of
/// the first line that refuses it (an unknown mnemonic, an operand missing or
/// out of range, past `lastStep`, two lines on one step).
std::variant<std::vector<ProgramStep>, KeycodeError> assemble(std::string_view text, int lastStep);

} // namespace cu59
