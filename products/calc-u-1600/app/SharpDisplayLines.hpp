#pragma once
#include "Display/LcdText.hpp"
#include "harness/DisplayText.hpp"
#include "machine/Display.hpp"
#include "machine/Machine.hpp"

// The display of a Sharp machine as text (LIB-6 design §10.2; LIB-11 AC-10,
// AC-19): one function for the preset steps `expect:`/`not-expect:` and the
// harness's `display-text`. Each cell is read with the ROM's own font; a cell that is
// no glyph is U+FFFD. Printable ASCII is itself, `\` is `\\`, any other code
// `\xHH` (the encoding of --lcd-text's rows).

namespace cu1600 {

// The ROM font in the harness's form.
calcu::harness::GlyphFont glyphFontOf(const LcdFont&);

calcu::harness::DisplayText displayLines(const calcu::machine::DisplayOutput&, const calcu::machine::MachineDescriptor&,
                                         const calcu::harness::GlyphFont&);
// With the font of the machine's ROM.
calcu::harness::DisplayText displayLines(calcu::machine::Machine&, const calcu::machine::DisplayOutput&);
// As the harness's `display-text` reads it (app/LcdTextPart): with the font
// of the machine's ROM and the cell under the cursor the ROM reports read as
// the character it saved there. The DAP's `c1600/display-text` (C1600-3 AC-17).
calcu::harness::DisplayText displayTextNow(calcu::machine::Machine&, const calcu::machine::DisplayOutput&);

} // namespace cu1600
