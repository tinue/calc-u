#include "harness/DisplayText.hpp"
#include <map>
#include <variant>

namespace calcu::harness {

using machine::DisplayOutput;
using machine::IndicatorDef;
using machine::IndicatorOutput;

namespace {
using Cell = std::array<std::uint8_t, 8>;

std::uint8_t rowMask(int h) { return static_cast<std::uint8_t>((1u << h) - 1u); }

Cell masked(Cell c, const GlyphFont& f) {
    for (int i = 0; i < 8; ++i) c[i] = i < f.cellWidth ? static_cast<std::uint8_t>(c[i] & rowMask(f.cellHeight)) : 0;
    return c;
}

Cell inverted(const Cell& c, const GlyphFont& f) {
    Cell out{};
    for (int i = 0; i < f.cellWidth; ++i) out[i] = static_cast<std::uint8_t>(~c[i] & rowMask(f.cellHeight));
    return out;
}

// The indicators that are on, in descriptor order.
std::vector<std::pair<std::string, std::optional<Decimal>>> indicatorsOn(std::span<const IndicatorOutput> out,
                                                                         const std::vector<IndicatorDef>& defs) {
    std::vector<std::pair<std::string, std::optional<Decimal>>> on;
    for (const auto& d : defs)
        for (const auto& i : out)
            if (i.name == d.name && i.on) {
                std::optional<Decimal> b;
                if (d.hasBrightness && i.brightness) b = fraction3(*i.brightness);
                on.emplace_back(d.name, b);
            }
    return on;
}

} // namespace

DisplayText readDotMatrix(const machine::DotMatrixFrame& frame, const GlyphFont& font,
                          std::span<const IndicatorOutput> indicators, const std::vector<IndicatorDef>& defs) {
    DisplayText t;
    t.dotMatrix = true;
    t.indicators = indicatorsOn(indicators, defs);
    if (font.cellWidth <= 0 || font.cellHeight <= 0 || font.cellWidth > 8 || font.cellHeight > 8) return t;
    std::map<Cell, const Glyph*> byCell;
    for (const Glyph& g : font.glyphs) byCell.emplace(masked(g.columns, font), &g);
    const int textRows = frame.height / font.cellHeight, textCols = frame.width / font.cellWidth;
    const Cell empty{};
    for (int tr = 0; tr < textRows; ++tr) {
        std::string line;
        for (int tc = 0; tc < textCols; ++tc) {
            Cell cell{};
            for (int c = 0; c < font.cellWidth; ++c)
                for (int r = 0; r < font.cellHeight; ++r) {
                    const std::size_t at = static_cast<std::size_t>(tr * font.cellHeight + r) * static_cast<std::size_t>(frame.width) +
                                           static_cast<std::size_t>(tc * font.cellWidth + c);
                    if (at < frame.dots.size() && frame.dots[at]) cell[static_cast<std::size_t>(c)] |= static_cast<std::uint8_t>(1u << r);
                }
            if (cell == empty) {
                line += ' ';
            } else if (auto it = byCell.find(cell); it != byCell.end()) {
                line += it->second->text;
            } else if (auto inv = byCell.find(inverted(cell, font)); inv != byCell.end()) {
                line += inv->second->text;
            } else {
                line += kNotText;
                ++t.notText;
            }
        }
        line.erase(line.find_last_not_of(' ') + 1);   // trailing blanks carry nothing
        t.rows.push_back(line);
    }
    return t;
}

DisplayText readSegments(const machine::SegmentFrame& frame, const machine::SegmentLayout& layout,
                         const SegmentRule& rule, bool rightmostFirst, std::span<const IndicatorOutput> indicators,
                         const std::vector<IndicatorDef>& defs) {
    DisplayText t;
    t.indicators = indicatorsOn(indicators, defs);
    std::string line;
    const std::size_t n = frame.positions.size();
    for (std::size_t i = 0; i < n; ++i) {
        const auto& p = frame.positions[rightmostFirst ? n - 1 - i : i];
        std::string s = rule ? rule(p, layout) : std::string();
        line += s.empty() ? std::string(" ") : s;
    }
    t.rows.push_back(line);
    return t;
}

std::vector<std::string> displayTextLines(const DisplayText& t) {
    if (!t.on) return {"display off"};
    std::vector<std::string> out = t.rows;
    std::string ind = "indicators:";
    for (const auto& [name, b] : t.indicators) ind += " " + name;
    out.push_back(ind);
    if (t.dotMatrix) out.push_back("not text: " + std::to_string(t.notText));
    return out;
}

Value displayTextValue(const DisplayText& t) {
    Value v = Value::object();
    v.set("on", t.on);
    Value rows = Value::array();
    Value ind = Value::array();
    if (t.on) {
        for (const auto& r : t.rows) rows.push(r);
        for (const auto& [name, b] : t.indicators) {
            Value i = Value::object();
            i.set("name", name);
            i.set("brightness", b ? Value(*b) : Value());
            ind.push(std::move(i));
        }
    }
    v.set("rows", std::move(rows));
    v.set("indicators", std::move(ind));
    v.set("notText", t.on && t.dotMatrix ? Value(t.notText) : Value());
    return v;
}

// ── display ─────────────────────────────────────────────────────────────

namespace {
std::string bits(const std::vector<bool>& b) {
    std::string s;
    for (bool x : b) s += x ? '1' : '0';
    return s;
}

// The indicators as bits in descriptor order, and their names.
std::pair<std::string, std::vector<std::string>> indicatorBits(const DisplayOutput& o,
                                                               const machine::DisplayDescriptor& d) {
    std::string s;
    std::vector<std::string> names;
    for (const auto& def : d.indicators) {
        bool on = false;
        for (const auto& i : o.indicators)
            if (i.name == def.name) on = i.on;
        s += on ? '1' : '0';
        names.push_back(def.name);
    }
    return {s, names};
}
} // namespace

std::vector<std::string> displayLines(const DisplayOutput& o, const machine::DisplayDescriptor& d) {
    std::vector<std::string> out;
    if (const auto* f = std::get_if<machine::DotMatrixFrame>(&o.frame)) {
        for (int y = 0; y < f->height; ++y) {
            std::string row;
            for (int x = 0; x < f->width; ++x) {
                const std::size_t at = static_cast<std::size_t>(y) * static_cast<std::size_t>(f->width) + static_cast<std::size_t>(x);
                row += at < f->dots.size() && f->dots[at] ? '1' : '0';
            }
            out.push_back(row);
        }
    } else {
        const auto& s = std::get<machine::SegmentFrame>(o.frame);
        std::string names;
        if (const auto* g = std::get_if<machine::SegmentGeometry>(&d.geometry))
            for (const auto& seg : g->layout.segments) names += (names.empty() ? "" : " ") + seg.name;
        out.push_back("segments: " + names);
        for (std::size_t i = 0; i < s.positions.size(); ++i) out.push_back(std::to_string(i) + " " + bits(s.positions[i].lit));
    }
    auto [b, names] = indicatorBits(o, d);
    out.push_back("indicators: " + b);
    std::string n = "names:";
    for (const auto& x : names) n += " " + x;
    out.push_back(n);
    out.push_back(std::string("on: ") + (o.on ? "true" : "false"));
    if (!o.parts.empty()) {
        std::string p = "parts:";
        for (const auto& x : o.parts) p += " " + x.name + (x.on ? " on" : " off") + (&x == &o.parts.back() ? "" : ",");
        out.push_back(p);
    }
    return out;
}

Value displayValue(const DisplayOutput& o, const machine::DisplayDescriptor& d) {
    Value v = Value::object();
    if (const auto* f = std::get_if<machine::DotMatrixFrame>(&o.frame)) {
        v.set("kind", "dots");
        Value rows = Value::array();
        for (int y = 0; y < f->height; ++y) {
            Value row = Value::array();
            for (int x = 0; x < f->width; ++x) {
                const std::size_t at = static_cast<std::size_t>(y) * static_cast<std::size_t>(f->width) + static_cast<std::size_t>(x);
                row.push(at < f->dots.size() && f->dots[at] ? 1 : 0);
            }
            rows.push(std::move(row));
        }
        v.set("rows", std::move(rows));
    } else {
        const auto& s = std::get<machine::SegmentFrame>(o.frame);
        v.set("kind", "segments");
        Value names = Value::array();
        if (const auto* g = std::get_if<machine::SegmentGeometry>(&d.geometry))
            for (const auto& seg : g->layout.segments) names.push(seg.name);
        v.set("segments", std::move(names));
        Value positions = Value::array();
        for (const auto& p : s.positions) {
            Value lit = Value::array();
            for (bool x : p.lit) lit.push(x ? 1 : 0);
            positions.push(std::move(lit));
        }
        v.set("positions", std::move(positions));
    }
    auto [b, names] = indicatorBits(o, d);
    Value bitsV = Value::array(), namesV = Value::array();
    for (char c : b) bitsV.push(c == '1' ? 1 : 0);
    for (const auto& x : names) namesV.push(x);
    v.set("indicators", std::move(bitsV));
    v.set("indicatorNames", std::move(namesV));
    v.set("on", o.on);
    Value parts = Value::array();
    for (const auto& x : o.parts) {
        Value part = Value::object();
        part.set("name", x.name);
        part.set("on", x.on);
        parts.push(std::move(part));
    }
    v.set("parts", std::move(parts));
    return v;
}

} // namespace calcu::harness
