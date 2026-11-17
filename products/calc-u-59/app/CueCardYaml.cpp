#include "CueCardYaml.hpp"
#include "TextUtil.hpp"
#include "formats/Flow.hpp"

namespace cu59 {

namespace {

using calcu::formats::YamlNode;

// A′–E′ → 0–4, A–E → 5–9 (any case; ′ or ').
int labelOf(const std::string& key) {
    std::string k = calcu::machine::lowerAscii(key);
    const std::string prime = "′";
    bool primed = false;
    if (k.size() == 2 && k[1] == '\'') primed = true, k.resize(1);
    else if (k.size() == 1 + prime.size() && k.compare(1, std::string::npos, prime) == 0) primed = true, k.resize(1);
    if (k.size() != 1 || k[0] < 'a' || k[0] > 'e') return -1;
    return (primed ? 0 : 5) + (k[0] - 'a');
}

std::optional<TextAlign> alignOf(const std::string& v) {
    const std::string l = calcu::machine::lowerAscii(v);
    if (l == "left") return TextAlign::Left;
    if (l == "center") return TextAlign::Center;
    if (l == "right") return TextAlign::Right;
    return std::nullopt;
}

bool needsQuotes(const std::string& v) {
    if (v.empty()) return true;
    if (v.front() == ' ' || v.back() == ' ') return true;
    if (v.find(": ") != std::string::npos || v.find(" #") != std::string::npos || v.back() == ':') return true;
    const char c = v.front();
    return c == '[' || c == '{' || c == '"' || c == '\'' || c == '#' || c == '-' || c == '|' || c == '>' || c == '&' ||
           c == '*' || c == '!' || c == '%' || c == '@' || c == '`';
}

std::string quoted(const std::string& v) {
    if (!needsQuotes(v)) return v;
    if (v.find('"') == std::string::npos) return "\"" + v + "\"";
    return "'" + v + "'";
}

} // namespace

std::optional<CueCardContent> readStandAloneCueCard(const YamlNode& block, const CueCardError& error) {
    if (!block.isMap()) {
        error(block.line, "cue-card: is a block of keys (title, A, B, …).");
        return std::nullopt;
    }
    CueCardContent c;
    c.template_ = CueCardTemplate::CueCard;
    bool ok = true;
    std::vector<std::string> seen;
    for (const auto& [key, v] : block.map) {
        const int line = v.line ? v.line : block.line;
        auto fail = [&](const std::string& m) {
            error(line, "cue-card: " + m);
            ok = false;
        };
        for (const std::string& s : seen)
            if (s == key) fail(key + ": is given twice.");
        seen.push_back(key);
        if (!v.isScalar() && !v.isNull()) {
            fail(key + ": takes one text.");
            continue;
        }
        const std::string text = v.isScalar() ? calcu::formats::scalarText(v) : std::string();
        if (key == "title") c.title = expandMathTokens(text);
        else if (key == "row-1") c.row1 = expandMathTokens(text);
        else if (key == "row-2") c.row2 = expandMathTokens(text);
        else if (key == "row-2-r") c.row2R = expandMathTokens(text);
        else if (key == "row-1-align" || key == "row-2-align" || key == "row-2-r-align") {
            const auto a = alignOf(text);
            if (!a) {
                fail(key + ": is left, center or right.");
                continue;
            }
            (key == "row-1-align" ? c.row1Align : key == "row-2-align" ? c.row2Align : c.row2RAlign) = *a;
        } else if (key == "style") {
            const std::string l = calcu::machine::lowerAscii(text);
            if (l == "none") c.style = CardButtonStyle::None;
            else if (l == "button") c.style = CardButtonStyle::Button;
            else fail("style: is none or button.");
        } else if (key == "pencil-color") {
            const std::string l = calcu::machine::lowerAscii(text);
            if (l == "black") c.color = CueCardInkColor::Black;
            else if (l == "pencil") c.color = CueCardInkColor::Pencil;
            else if (l == "sharpie") c.color = CueCardInkColor::Sharpie;
            else fail("pencil-color: is black, pencil or sharpie.");
        } else if (const int i = labelOf(key); i >= 0) {
            c.labels[static_cast<std::size_t>(i)] = expandMathTokens(text);
        } else {
            fail("a stand-alone cue card has no key " + key +
                 "; its keys are title, row-1, row-2, row-2-r, row-1-align, row-2-align, row-2-r-align, style, "
                 "pencil-color, A to E and A' to E'.");
        }
    }
    if (!ok) return std::nullopt;
    return c;
}

std::string writeStandAloneCueCard(const CueCardContent& c, int indent) {
    const std::string pad(static_cast<std::size_t>(indent), ' ');
    std::string out;
    auto add = [&](const std::string& key, const std::string& v) { out += pad + key + ": " + quoted(v) + "\n"; };
    auto label = [](const std::string& l) { return l == kCueCardInvisibleMarker ? std::string("\\blank") : l; };
    if (!c.title.empty()) add("title", c.title);
    static const char* const kPrime[] = {"\"A'\"", "\"B'\"", "\"C'\"", "\"D'\"", "\"E'\""};
    static const char* const kPlain[] = {"A", "B", "C", "D", "E"};
    for (std::size_t i = 0; i < 5; ++i)
        if (!c.labels[i].empty()) add(kPrime[i], label(c.labels[i]));
    for (std::size_t i = 0; i < 5; ++i)
        if (!c.labels[i + 5].empty()) add(kPlain[i], label(c.labels[i + 5]));
    if (!c.row1.empty()) add("row-1", c.row1);
    if (c.row1Align != TextAlign::Center) add("row-1-align", toString(c.row1Align));
    if (!c.row2.empty()) add("row-2", c.row2);
    if (c.row2Align != TextAlign::Left) add("row-2-align", toString(c.row2Align));
    if (!c.row2R.empty()) add("row-2-r", c.row2R);
    if (c.row2RAlign != TextAlign::Left) add("row-2-r-align", toString(c.row2RAlign));
    if (c.style != CardButtonStyle::None) add("style", toString(c.style));
    if (c.color != CueCardInkColor::Black) add("pencil-color", toString(c.color));
    return out;
}

} // namespace cu59
