#include "paper/Replies.hpp"
#include <cstdio>
#include <sstream>

namespace calcu::paper {

namespace {

std::string numberText(Coord c) {
    const bool negative = c < 0;
    const std::uint64_t a = negative ? std::uint64_t(0) - static_cast<std::uint64_t>(c) : static_cast<std::uint64_t>(c);
    std::string out = (negative ? "-" : "") + std::to_string(a / std::uint64_t(kPerUnit));
    std::uint64_t frac = a % std::uint64_t(kPerUnit);
    if (frac == 0) return out;
    std::string digits = std::to_string(frac);
    digits.insert(0, 4 - digits.size(), '0');
    while (!digits.empty() && digits.back() == '0') digits.pop_back();
    return out + "." + digits;
}

std::optional<Coord> parseNumberText(std::string_view t) {
    if (t.empty()) return std::nullopt;
    bool negative = false;
    std::size_t i = 0;
    if (t[0] == '-') {
        negative = true;
        i = 1;
    }
    std::int64_t whole = 0, frac = 0;
    int fracDigits = 0, wholeDigits = 0;
    for (; i < t.size() && t[i] != '.'; ++i) {
        if (t[i] < '0' || t[i] > '9' || wholeDigits > 14) return std::nullopt;
        whole = whole * 10 + (t[i] - '0');
        ++wholeDigits;
    }
    if (wholeDigits == 0) return std::nullopt;
    if (i < t.size()) {
        for (++i; i < t.size(); ++i) {
            if (t[i] < '0' || t[i] > '9' || fracDigits == 4) return std::nullopt;
            frac = frac * 10 + (t[i] - '0');
            ++fracDigits;
        }
        if (fracDigits == 0) return std::nullopt;
    }
    for (int k = fracDigits; k < 4; ++k) frac *= 10;
    const Coord v = whole * kPerUnit + frac;
    return negative ? -v : v;
}

} // namespace

std::string colourText(Rgb c) {
    char buf[8];
    std::snprintf(buf, sizeof buf, "#%02x%02x%02x", c.r, c.g, c.b);
    return buf;
}

std::vector<std::string> textReply(const std::vector<TextLine>& lines) {
    std::vector<std::string> out;
    out.reserve(lines.size());
    for (const auto& l : lines) out.push_back(l.text);
    return out;
}

std::vector<std::string> marksReply(Reach reach, const std::vector<Mark>& marks) {
    std::vector<std::string> out;
    out.reserve(marks.size() + 1);
    out.push_back("reach " + numberText(reach.from) + " " + numberText(reach.to));
    for (const Mark& m : marks) {
        if (const auto* d = std::get_if<Dot>(&m)) {
            out.push_back("dot " + numberText(d->at.x) + " " + numberText(d->at.y) + " " + std::to_string(int(d->darkness)));
            continue;
        }
        const Stroke& s = std::get<Stroke>(m);
        std::string line = "stroke " + colourText(s.colour);
        for (const Position& p : s.path) line += " " + numberText(p.x) + "," + numberText(p.y);
        out.push_back(std::move(line));
    }
    return out;
}

std::vector<std::string> textReply(const Paper& p) { return textReply(p.text()); }
std::vector<std::string> marksReply(const Paper& p) { return marksReply(p.reach(), p.marks()); }

namespace {
std::vector<std::string> words(const std::string& line) {
    std::vector<std::string> out;
    std::istringstream in(line);
    for (std::string w; in >> w;) out.push_back(w);
    return out;
}
std::optional<Rgb> parseColour(const std::string& w) {
    if (w.size() != 7 || w[0] != '#') return std::nullopt;
    unsigned v[3];
    for (int k = 0; k < 3; ++k) {
        v[k] = 0;
        for (int j = 0; j < 2; ++j) {
            const char ch = w[std::size_t(1 + 2 * k + j)];
            int n;
            if (ch >= '0' && ch <= '9') n = ch - '0';
            else if (ch >= 'a' && ch <= 'f') n = ch - 'a' + 10;
            else return std::nullopt;
            v[k] = v[k] * 16 + unsigned(n);
        }
    }
    return Rgb{std::uint8_t(v[0]), std::uint8_t(v[1]), std::uint8_t(v[2])};
}
} // namespace

std::optional<MarksRead> readMarksReply(const std::vector<std::string>& lines, std::string* error) {
    auto fail = [&](std::size_t i, const char* why) -> std::optional<MarksRead> {
        if (error) *error = "line " + std::to_string(i + 1) + ": " + why;
        return std::nullopt;
    };
    MarksRead r;
    if (lines.empty()) return fail(0, "no reach line");
    for (std::size_t i = 0; i < lines.size(); ++i) {
        const auto w = words(lines[i]);
        if (i == 0) {
            if (w.size() != 3 || w[0] != "reach") return fail(i, "not `reach <from> <to>`");
            const auto f = parseNumberText(w[1]), t = parseNumberText(w[2]);
            if (!f || !t) return fail(i, "not a number");
            r.reach = Reach{*f, *t};
            continue;
        }
        if (w.size() == 4 && w[0] == "dot") {
            const auto x = parseNumberText(w[1]), y = parseNumberText(w[2]);
            int d = -1;
            if (w[3].size() <= 3 && w[3].find_first_not_of("0123456789") == std::string::npos) d = std::stoi(w[3]);
            if (!x || !y || d < 0 || d > 255) return fail(i, "not `dot <x> <y> <darkness>`");
            r.marks.push_back(Dot{Position{*x, *y}, static_cast<std::uint8_t>(d)});
            continue;
        }
        if (w.size() >= 3 && w[0] == "stroke") {
            const auto c = parseColour(w[1]);
            if (!c) return fail(i, "not a colour #rrggbb");
            Stroke s{*c, {}};
            for (std::size_t k = 2; k < w.size(); ++k) {
                const auto comma = w[k].find(',');
                if (comma == std::string::npos) return fail(i, "not a position <x>,<y>");
                const auto x = parseNumberText(std::string_view(w[k]).substr(0, comma));
                const auto y = parseNumberText(std::string_view(w[k]).substr(comma + 1));
                if (!x || !y) return fail(i, "not a position <x>,<y>");
                s.path.push_back(Position{*x, *y});
            }
            r.marks.push_back(std::move(s));
            continue;
        }
        return fail(i, "not a dot or a stroke");
    }
    return r;
}

} // namespace calcu::paper
