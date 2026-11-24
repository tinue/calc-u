#pragma once
#include <QtGlobal>

// The native mode of a Mac's screen, for About's Display section: macOS's
// "looks like" modes draw more pixels than the panel has and scale them
// down, which Qt does not tell.
#ifdef Q_OS_MACOS
class QScreen;
namespace calcu::qt6 {
// The panel's own pixels; false when the system names no native mode.
bool macPanelSize(const QScreen&, int* width, int* height);
} // namespace calcu::qt6
#endif
