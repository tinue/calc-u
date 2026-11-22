#pragma once
#include "TIChrome.hpp"
#include "qt6/ChromeStyle.hpp"
#include <QColor>
#include <QString>
#include <cstdint>

// The Qt6 frontend's stylesheets over app/'s chrome (TIChrome): thin
// functions, the values are app/'s.
namespace ChromeStyle {

inline const calcu::shell::Chrome& chrome() { return cu59::tiChrome(); }
inline const calcu::shell::ChromeColours& colours() { return cu59::tiChrome().light; }
/// A colour of the product's own (cu59::chrome::k…).
inline QColor named(std::string_view name) { return calcu::qt6::chrome::qcolour(colours().named(name)); }
inline QColor qcolour(const calcu::shell::Colour& c) { return calcu::qt6::chrome::qcolour(c); }
/// The chrome's text colour at this opacity.
inline QColor text(double opacity) { return qcolour(colours().text.withAlpha(opacity)); }
inline QString rgb(const QColor& c) { return c.alpha() == 255 ? c.name(QColor::HexRgb) : QStringLiteral("rgba(%1,%2,%3,%4)").arg(c.red()).arg(c.green()).arg(c.blue()).arg(c.alpha()); }
inline QString rgb(std::string_view name) { return rgb(named(name)); }

/// A packed 0xRRGGBB colour from app/.
inline QColor fromRgb(uint32_t c, double alpha = 1.0) {
    QColor q(static_cast<int>(c >> 16 & 0xFF), static_cast<int>(c >> 8 & 0xFF), static_cast<int>(c & 0xFF));
    q.setAlphaF(alpha);
    return q;
}

/// Pill buttons (QPushButton) of the chrome's shape.
inline QString pillButton() { return calcu::qt6::chrome::pills(colours(), chrome().sizes, QStringLiteral("QPushButton")); }

/// A panel button bar of the chrome's height with pill buttons.
inline QString buttonBar() {
    return calcu::qt6::chrome::panelButtons(colours(), chrome().sizes, QStringLiteral("QPushButton"));
}

/// Plain text button on a bar (Reset, Preset, …): no frame, dims when
/// pressed; the bar's pickers as plain text without a box or an arrow,
/// opening their list (the iOS bar's menus). `scale`: the text's scale with
/// the faceplate (fontScale).
inline QString barButton(double scale = 1.0) {
    const auto& c = colours();
    return QStringLiteral(
        "QToolButton { color: %3; background: transparent; border: none; padding: 4px 6px; font-size: %1px; }"
        "QToolButton:pressed { color: %4; }"
        "QToolButton:disabled { color: %5; }"
        "QComboBox { color: %3; background: transparent; border: none; padding: 4px 6px;"
        " font-weight: 600; font-size: %2px; }"
        "QComboBox:disabled { color: %5; }"
        "QComboBox::drop-down { width: 0px; border: none; }"
        "QComboBox::down-arrow { image: none; width: 0px; }")
        .arg(qRound(13 * scale))
        .arg(qRound(14 * scale))
        .arg(rgb(qcolour(c.text)), rgb(text(0.5)), rgb(qcolour(c.textDim)));
}

/// The bar's and the paper panel's text scale for a faceplate this wide
/// (app/'s rule, TIChrome).
inline double fontScale(double faceplateWidth) { return chrome().textScale(faceplateWidth); }

/// Monospaced header action (RESUME / FREEZE / … in the debug headers).
inline QString headerButton(const QColor& color, int pixelSize = 13) {
    return QStringLiteral(
        "QToolButton { color: %1; background: transparent; border: none; font-weight: bold;"
        " font-family: monospace; font-size: %5px; }"
        "QToolButton:disabled { color: rgba(%2,%3,%4,0.4); }")
        .arg(rgb(color)).arg(color.red()).arg(color.green()).arg(color.blue()).arg(pixelSize);
}

} // namespace ChromeStyle
