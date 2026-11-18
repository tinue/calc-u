#pragma once
#include "LcdScreenshot.hpp"
#include "LcdText.hpp"
#include "machine/Display.hpp"
#include "machine/Machine.hpp"
#include <optional>

// The display of a Sharp machine as text and as a bitmap, read through the
// machine interface: the dots and indicators of
// readDisplay(), parsed with the ROM's own font (core/Display/LcdText.hpp).
// The golden replay, Copy Screen and the harness's `display-text` read the display this way.

// The dot matrix of display output as an LcdBitmap (poweredOn: the output's on).
LcdBitmap lcdBitmapOf(const calcu::machine::DisplayOutput&);

// The model's ROM font (PC-1500: ROM FCA0H; PC-1600: bank 6, read without a
// bus cycle). Empty for a machine that is not a Sharp one.
LcdFont sharpLcdFont(calcu::machine::Machine&);

// The cursor the model's ROM reports through its RAM (PC1500LcdText.hpp,
// PC1600LcdText.hpp), or none while it shows none.
std::optional<LcdCursor> lcdCursorOf(calcu::machine::Machine&);

// The dot matrix with the cell the cursor covers drawn as the ROM saved it
// (LcdCursor::under), as parseLcdText reads it (`display-text`
// reads the cell under the cursor as text). Unchanged when no cell there has
// a cursor shape of `font`.
calcu::machine::DotMatrixFrame uncoverCursor(calcu::machine::DotMatrixFrame, const LcdFont& font, const LcdCursor&);

// `output` as text with the ROM font, the cursor the ROM reports through
// its RAM, and the indicators as status words.
LcdText displayText(calcu::machine::Machine&, const calcu::machine::DisplayOutput& output);
// The same for the display now.
LcdText displayText(calcu::machine::Machine&);
