#include "qt6/WindowPlacing.hpp"
#include <QGuiApplication>
#include <QScreen>
#include <QWidget>
#include <algorithm>

namespace calcu::qt6 {

namespace {
// Smaller, a window is hard to find and to grab.
constexpr long kLeastSide = 160;

// The point on its title bar that must be on a screen.
bool onAScreen(QPoint topLeft) { return QGuiApplication::screenAt(topLeft + QPoint(40, 10)) != nullptr; }
} // namespace

void applyWindowPlace(QWidget* window, const settings::WindowPlace& p, const QWidget* beside) {
    window->resize(static_cast<int>(std::max(kLeastSide, p.width)), static_cast<int>(std::max(kLeastSide, p.height)));
    if (p.x && p.y) {
        const QPoint at(static_cast<int>(*p.x), static_cast<int>(*p.y));
        if (onAScreen(at)) {
            window->move(at);
            return;
        }
    }
    if (!beside) return;
    const QRect m = beside->frameGeometry();
    const QPoint right(m.right() + 8, m.top());
    window->move(onAScreen(right) ? right : m.topLeft() + QPoint(40, 40));
}

settings::WindowPlace windowPlaceOf(const QWidget* window) {
    const QRect g = window->normalGeometry().isValid() ? window->normalGeometry() : window->geometry();
    const QPoint at = window->isMaximized() || window->isFullScreen() ? g.topLeft() : window->pos();
    return {at.x(), at.y(), g.width(), g.height()};
}

} // namespace calcu::qt6
