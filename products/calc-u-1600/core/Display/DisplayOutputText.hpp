#pragma once
#include "LcdScreenshot.hpp"
#include "LcdText.hpp"
#include "machine/Display.hpp"
#include "machine/Machine.hpp"

// The display of a Sharp machine as text and as a bitmap, read through the
// machine interface (C1600-1 design §3.5): the dots and indicators of
// readDisplay(), parsed with the ROM's own font (core/Display/LcdText.hpp).
// The golden replay, Copy Screen and the CLIs read the display this way.

// The dot matrix of display output as an LcdBitmap (poweredOn: the output's on).
LcdBitmap lcdBitmapOf(const calcu::machine::DisplayOutput&);

// The model's ROM font (PC-1500: ROM FCA0H; PC-1600: bank 6, read without a
// bus cycle). Empty for a machine that is not a Sharp one.
LcdFont sharpLcdFont(calcu::machine::Machine&);

// `output` as text with the ROM font, the cursor the ROM reports through
// its RAM, and the indicators as status words.
LcdText displayText(calcu::machine::Machine&, const calcu::machine::DisplayOutput& output);
// The same for the display now.
LcdText displayText(calcu::machine::Machine&);
