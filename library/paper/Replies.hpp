#pragma once
#include "paper/Paper.hpp"
#include <optional>
#include <string>
#include <string_view>
#include <vector>

// The text forms of `paper text` and `paper marks`. The reference runs of
// the products are compared over them, so the form is fixed: a change is a
// finding.
//
//   paper text    one line per text line, as printed
//   paper marks   `reach <from> <to>`, then one line per mark, in the order
//                 made: `dot <x> <y> <darkness>` or
//                 `stroke #rrggbb <x>,<y> <x>,<y> …`
//
// A number is Coord / 10000 with at most four decimals, no trailing zeros
// and no point for a whole number: 7, 6.53, -2, 66.9474.

namespace calcu::paper {

std::string numberText(Coord);
std::optional<Coord> parseNumberText(std::string_view);   // the inverse; nullopt for anything else
std::string colourText(Rgb);                               // #rrggbb

std::vector<std::string> textReply(const std::vector<TextLine>&);
std::vector<std::string> marksReply(Reach, const std::vector<Mark>&);
std::vector<std::string> textReply(const Paper&);
std::vector<std::string> marksReply(const Paper&);

// A `paper marks` reply read back: the reach and the marks (the tests and
// the products' converters). nullopt with `error` set for a line that is not
// of the form.
struct MarksRead {
    Reach reach;
    std::vector<Mark> marks;
};
std::optional<MarksRead> readMarksReply(const std::vector<std::string>&, std::string* error = nullptr);

} // namespace calcu::paper
