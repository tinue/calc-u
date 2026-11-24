#pragma once
#include "shell/ShellProduct.hpp"

// What the host is, for the About dialog's Support Information and the
// app log's first line: the operating system and the hardware.
//   OS        macOS "Darwin arm64 27.2.0" (the kernel, as uname tells it);
//             Linux "Ubuntu 24.04.3 LTS (Linux 6.8.0)"; Windows
//             "Windows 11 25H2 (build 26200.7171)"
//   Hardware  macOS the model ("Mac16,11"); Linux and Windows the CPU's
//             name and the architecture ("AMD Ryzen 7 7840U, x86_64")
// A fact the host does not give stays empty (the dialog leaves its row out).

class QWidget;

namespace calcu::qt6 {

shell::SupportRows hostSupportRows();

// About's Display section for the screen `window` is on: its name, size
// and scale as Qt tells them, the panel's native mode on macOS, the
// window's size, and on Linux the windowing ("Wayland", "X11", "X11 on
// Wayland" for XWayland, else Qt's platform name).
shell::DisplayFacts displayFacts(const QWidget& window);

} // namespace calcu::qt6
