#pragma once
#include "settings/WindowPlace.hpp"

class QWidget;

// A desktop window's place ([window] as plain numbers, settings::WindowPlace)
// put on a Qt window and read back, the same for every window of every
// product (the main windows, the paper's own window).

namespace calcu::qt6 {

// The window at its place: its size, and its top-left corner where that
// keeps its title bar on a screen that is there. Without such a corner:
// beside `beside` (right of it, else just inside it) where one is given,
// else where the window system puts it.
void applyWindowPlace(QWidget* window, const settings::WindowPlace&, const QWidget* beside = nullptr);

// The window's place to keep: its normal size, and its normal top-left
// corner when it is maximised or full screen (it comes back at its last
// normal place), else where it is.
settings::WindowPlace windowPlaceOf(const QWidget* window);

} // namespace calcu::qt6
