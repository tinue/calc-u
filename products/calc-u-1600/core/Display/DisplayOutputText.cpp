#include "Display/DisplayOutputText.hpp"

#include <variant>

#include "../Machine/SharpCores.hpp"
#include "../Machine/SharpDisplayOutput.hpp"
#include "../PC1500/PC1500LcdText.hpp"
#include "../PC1600/PC1600LcdText.hpp"

using namespace calcu::machine;

LcdBitmap lcdBitmapOf(const DisplayOutput& o) {
    LcdBitmap b;
    b.poweredOn = o.on;
    if (const auto* f = std::get_if<DotMatrixFrame>(&o.frame)) {
        b.cols = f->width;
        b.rows = f->height;
        b.pixels.assign(f->dots.begin(), f->dots.end());
    }
    return b;
}

LcdFont sharpLcdFont(Machine& m) {
    if (PC1600Machine* p = sharp::pc1600Of(m)) return pc1600LcdFont(*p);
    if (PC1500Machine* p = sharp::pc1500Of(m)) return pc1500LcdFont(*p);
    return {};
}

LcdText displayText(Machine& m, const DisplayOutput& output) {
    // The cursor as each model's ROM keeps it in RAM (PC1500LcdText.hpp,
    // PC1600LcdText.hpp), read through the loader memory.
    std::optional<LcdCursor> cursor;
    if (PC1600Machine* p = sharp::pc1600Of(m)) {
        if (p->debugPeek(0xF05E) & 0x01) {
            cursor = LcdCursor{};
            cursor->row = p->debugPeek(0xF05F);
            cursor->col = p->debugPeek(0xF060);
            for (int i = 0; i < 6; ++i) cursor->under[i] = p->debugPeek(static_cast<std::uint16_t>(0xF069 + i));
        }
    } else if (PC1500Machine* p = sharp::pc1500Of(m)) {
        if ((p->debugPeek(0x787C) & 0x81) == 0x81) {
            const std::uint8_t code = p->debugPeek(0x787D);
            cursor = LcdCursor{};
            const LcdFont font = pc1500LcdFont(*p);
            if (code >= 0x20 && code < 0x80) cursor->under = font.glyphs[code - 0x20].cell;
        }
    }
    LcdText text = parseLcdText(lcdBitmapOf(output), sharpLcdFont(m), cursor);
    text.status = statusWords(sharp::statusLineOf(output));
    return text;
}

LcdText displayText(Machine& m) { return displayText(m, m.readDisplay()); }
