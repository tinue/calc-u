#pragma once
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

// The keycode notation of docs/formats/preset.md §4.1 (C59-2 AC-3, OQ-5,
// design §3.3, D-5): one reader for a preset's `program: keycodes: |`, a
// keystroke program file (§4.1.1, `.keystrokes.yaml`) and `c59 program`.
//
// A line is cut at a `#` that starts it or follows a space (a comment).
// Listing: a first token of three or more digits is a step number (up to
// the model's last step), followed by exactly one two-digit keycode; what
// follows is the mnemonic and is not read (`473 00 0`). Compact: every token
// is a two-digit keycode, written to the steps after the last one written
// (step 000 at the start). Any other token refuses the text.

namespace cu59 {

struct ProgramStep {
    int step = 0;
    std::uint8_t keycode = 0;
    bool operator==(const ProgramStep&) const = default;
};

struct KeycodeError {
    int lineIndex = 0;   // 0-based, among the lines of the text
    std::string message;
};

std::variant<std::vector<ProgramStep>, KeycodeError> readKeycodes(std::string_view text, int lastStep);

/// A keystroke program file (format `ti-keystroke-program`, version 1).
struct KeystrokeProgram {
    std::string name, description;
    std::vector<ProgramStep> steps;
};
/// The file's text; the program, or the sentence why not (with its line).
std::variant<KeystrokeProgram, std::string> readKeystrokeProgramFile(std::string_view text, int lastStep);

/// The lines of `text` as the YAML reader keeps them in a block scalar
/// (blank and comment-only lines left out): their 1-based line numbers in
/// the file, from `firstLine` on (the block's first line).
std::vector<int> blockLineNumbers(std::string_view fileText, int firstLine, std::size_t count);

} // namespace cu59
