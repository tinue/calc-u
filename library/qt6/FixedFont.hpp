#pragma once
#include <QFont>
#include <QString>

// The platform's own fixed-width font (the system's on macOS, Consolas on
// Windows, the desktop's choice on Linux), for every fixed-width text of the
// Qt6 frontends. A family name such as "Menlo" exists on one platform only,
// and a generic word such as "monospace" is no family: Qt then searches every
// family for a stand-in at the first use (tens of milliseconds, and a warning
// in the app log). Resolved once; size and weight are the caller's.

namespace calcu::qt6 {

const QFont& fixedFont();
// It as a stylesheet declaration at `px` pixels: `font: [bold] <px>px "<family>";`.
QString fixedFontCss(int px, bool bold = false);

} // namespace calcu::qt6
