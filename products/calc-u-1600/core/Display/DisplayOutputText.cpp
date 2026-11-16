#include "Display/DisplayOutputText.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
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
    LcdText text = parseLcdText(lcdBitmapOf(output), sharpLcdFont(m), lcdCursorOf(m));
    text.status = statusWords(sharp::statusLineOf(output));
    return text;
}

DotMatrixFrame uncoverCursor(DotMatrixFrame f, const LcdFont& font, const LcdCursor& cursor) {
    const int cw = font.cellWidth, ch = font.cellHeight;
    if (cw <= 0 || ch <= 0 || f.width <= 0 || static_cast<int>(f.dots.size()) < f.width * f.height) return f;
    const std::uint8_t mask = static_cast<std::uint8_t>((1u << ch) - 1u);
    auto cellAt = [&](int tr, int tc) {
        LcdCell cell{};
        for (int c = 0; c < cw; ++c)
            for (int r = 0; r < ch; ++r)
                if (f.dots[static_cast<std::size_t>(tr * ch + r) * static_cast<std::size_t>(f.width) +
                           static_cast<std::size_t>(tc * cw + c)])
                    cell[static_cast<std::size_t>(c)] |= static_cast<std::uint8_t>(1u << r);
        return cell;
    };
    auto isCursor = [&](const LcdCell& cell) {
        for (const LcdCell& s : font.cursorShapes) {
            bool same = true;
            for (int c = 0; c < 8 && same; ++c) {
                const std::uint8_t want = c < cw ? static_cast<std::uint8_t>(s[static_cast<std::size_t>(c)] & mask) : 0;
                same = cell[static_cast<std::size_t>(c)] == want;
            }
            if (same) return true;
        }
        return false;
    };
    const int rows = f.height / ch, cols = f.width / cw;
    for (int tr = 0; tr < rows; ++tr)
        for (int tc = 0; tc < cols; ++tc) {
            if (cursor.row >= 0 && (tr != cursor.row || tc != cursor.col)) continue;
            if (!isCursor(cellAt(tr, tc))) continue;
            for (int c = 0; c < cw; ++c)
                for (int r = 0; r < ch; ++r)
                    f.dots[static_cast<std::size_t>(tr * ch + r) * static_cast<std::size_t>(f.width) +
                           static_cast<std::size_t>(tc * cw + c)] = (cursor.under[static_cast<std::size_t>(c)] >> r) & 1u;
            return f;   // the first cursor-shaped cell when the position is unknown (LcdCursor)
        }
    return f;
}

std::optional<LcdCursor> lcdCursorOf(Machine& m) {
    // The cursor as each model's ROM keeps it in RAM (PC1500LcdText.hpp,
    // PC1600LcdText.hpp).
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
    return cursor;
}

LcdText displayText(Machine& m) { return displayText(m, m.readDisplay()); }
