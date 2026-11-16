#include "app/LcdTextPart.hpp"

#include <variant>

#include "Display/DisplayOutputText.hpp"
#include "app/SharpDisplayLines.hpp"

namespace shared {

void LcdTextPart::machineChanged(calcu::sessions::SessionContext& c) {
    if (calcu::machine::Machine* m = c.machine()) {
        font_ = sharpLcdFont(*m);
        glyphs_ = glyphFontOf(font_);
        cursor_ = lcdCursorOf(*m);
    } else {
        font_ = LcdFont{};
        glyphs_ = calcu::harness::GlyphFont{};
        cursor_.reset();
    }
}

void LcdTextPart::beforeFlush(calcu::sessions::SessionContext& c) {
    calcu::machine::Machine* m = c.machine();
    cursor_ = m ? lcdCursorOf(*m) : std::nullopt;
}

calcu::harness::DisplayText LcdTextPart::read(const calcu::machine::DisplayOutput& o,
                                              const calcu::machine::MachineDescriptor& d) const {
    const auto* f = std::get_if<calcu::machine::DotMatrixFrame>(&o.frame);
    if (!f || !cursor_ || !o.on) return displayLines(o, d, glyphs_);
    calcu::machine::DisplayOutput uncovered = o;
    uncovered.frame = uncoverCursor(*f, font_, *cursor_);
    return displayLines(uncovered, d, glyphs_);
}

} // namespace shared
