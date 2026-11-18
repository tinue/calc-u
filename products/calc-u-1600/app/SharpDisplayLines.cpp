#include "app/SharpDisplayLines.hpp"

#include <cstdio>
#include <optional>
#include <variant>

#include "Display/DisplayOutputText.hpp"

namespace cu1600 {

namespace {

std::string codeText(std::uint8_t code) {
    if (code == '\\') return "\\\\";
    if (code >= 0x20 && code < 0x7F) return std::string(1, static_cast<char>(code));
    char hex[8];
    std::snprintf(hex, sizeof hex, "\\x%02X", code);
    return hex;
}

} // namespace

calcu::harness::GlyphFont glyphFontOf(const LcdFont& f) {
    calcu::harness::GlyphFont g;
    g.cellWidth = f.cellWidth;
    g.cellHeight = f.cellHeight;
    for (const LcdGlyph& l : f.glyphs) {
        calcu::harness::Glyph x;
        x.text = codeText(l.code);
        for (std::size_t i = 0; i < 8; ++i) x.columns[i] = l.cell[i];
        g.glyphs.push_back(std::move(x));
    }
    return g;
}

calcu::harness::DisplayText displayLines(const calcu::machine::DisplayOutput& o,
                                         const calcu::machine::MachineDescriptor& d,
                                         const calcu::harness::GlyphFont& font) {
    const auto* f = std::get_if<calcu::machine::DotMatrixFrame>(&o.frame);
    calcu::harness::DisplayText t;
    if (f) t = calcu::harness::readDotMatrix(*f, font, o.indicators, d.display.indicators);
    t.on = o.on;
    return t;
}

calcu::harness::DisplayText displayLines(calcu::machine::Machine& m, const calcu::machine::DisplayOutput& o) {
    return displayLines(o, m.descriptor(), glyphFontOf(sharpLcdFont(m)));
}

calcu::harness::DisplayText displayTextNow(calcu::machine::Machine& m, const calcu::machine::DisplayOutput& o) {
    const LcdFont font = sharpLcdFont(m);
    const auto* f = std::get_if<calcu::machine::DotMatrixFrame>(&o.frame);
    const std::optional<LcdCursor> cursor = lcdCursorOf(m);
    if (!f || !cursor || !o.on) return displayLines(o, m.descriptor(), glyphFontOf(font));
    calcu::machine::DisplayOutput uncovered = o;
    uncovered.frame = uncoverCursor(*f, font, *cursor);
    return displayLines(uncovered, m.descriptor(), glyphFontOf(font));
}

} // namespace cu1600
