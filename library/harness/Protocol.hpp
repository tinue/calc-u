#pragma once
#include <span>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

// Lines and the one-shot form.
// - The word between the commands of a one-shot run is `then`: `calcu
//   --product c1600 wait 2s then display-text then ticks`. The words of each command
//   are joined with one space into a line, so the one-shot form and `serve`
//   share one parser. A literal argument `then` cannot be written in the
//   one-shot form; `serve` can.
// - A product command is reached by its product word first (`test count`,
//   later `ti59 …`, `pc1600 …`); a common command at the top level.
// - The value of a step: the rest of the line after the command's
//   name, as written, trimmed; a preset step `- floppy: { name: Disk2 }`
//   becomes the line `floppy { name: Disk2 }`.
// - A `\r` before the end of a line is dropped (Windows); an empty line is
//   no command.

namespace calcu::harness {

class CommandRegistry;

struct CommandLine {
    std::vector<std::string> path;   // {"display-text"} or {"test", "count"}
    std::string argText;             // the rest, as written, trimmed
};

inline constexpr const char* kThen = "then";

// The line split into its command and argument text, or the refusal
// (`unknown command <word>`). An empty line gives an empty path.
std::variant<CommandLine, std::string> parseLine(std::string_view line, const CommandRegistry&);

// The commands of a one-shot run, one line each.
std::vector<std::string> splitOneShot(std::span<const std::string> words);

// The text without leading and trailing spaces, tabs and a final \r.
std::string trimmed(std::string_view);

} // namespace calcu::harness
