#pragma once
#include <algorithm>
#include <QColor>
#include <QString>
#include <cstdint>

// Panel colours and control styles, matching the Swift app's dark chrome
// (CalculatorView, PrinterView, DebugView, ChromeControlStyles.swift).
namespace ChromeStyle {

inline QColor gray(double white) { return QColor::fromRgbF(white, white, white); }

inline const QColor kPanel = QColor(29, 29, 28);        ///< calculator / printer background
inline const QColor kBar = gray(0.15);                  ///< button bars
inline const QColor kHeader = gray(0.08);               ///< printer header
inline const QColor kDebugBackground = gray(0.10);
inline const QColor kDebugHeader = gray(0.07);
inline const QColor kSectionBody = gray(0.13);
inline const QColor kPill = gray(0.25);                 ///< pill buttons
inline const QColor kPaper = QColor::fromRgbF(0.96f, 0.94f, 0.88f);

inline QString rgb(const QColor& c) { return c.name(QColor::HexRgb); }

/// A packed 0xRRGGBB colour from app/.
inline QColor fromRgb(uint32_t c, double alpha = 1.0) {
    QColor q(static_cast<int>(c >> 16 & 0xFF), static_cast<int>(c >> 8 & 0xFF), static_cast<int>(c & 0xFF));
    q.setAlphaF(alpha);
    return q;
}

/// Flat pill button ("PC-100C", "T", "TRACE", …).
inline QString pillButton() {
    return QStringLiteral(
        "QPushButton { color: white; background: %1; border: none; border-radius: 6px;"
        " padding: 4px 10px; font-weight: bold; font-size: 11px; }"
        "QPushButton:pressed { background: %2; }"
        "QPushButton:disabled { color: rgba(255,255,255,0.4); background: %3; }")
        .arg(rgb(kPill), rgb(gray(0.18)), rgb(gray(0.15)));
}

/// Plain text button on a bar (Reset, Preset, …): no frame, dims when
/// pressed; the bar's pickers as plain text without a box or an arrow,
/// opening their list (the iOS bar's menus). `scale`: the text's size
/// relative to a faceplate 440 px wide (fontScale).
inline QString barButton(double scale = 1.0) {
    return QStringLiteral(
        "QToolButton { color: white; background: transparent; border: none; padding: 4px 6px; font-size: %1px; }"
        "QToolButton:pressed { color: rgba(255,255,255,0.5); }"
        "QToolButton:disabled { color: rgba(255,255,255,0.35); }"
        "QComboBox { color: white; background: transparent; border: none; padding: 4px 6px;"
        " font-weight: 600; font-size: %2px; }"
        "QComboBox:disabled { color: rgba(255,255,255,0.35); }"
        "QComboBox::drop-down { width: 0px; border: none; }"
        "QComboBox::down-arrow { image: none; width: 0px; }")
        .arg(qRound(13 * scale)).arg(qRound(14 * scale));
}

/// The bar's and the paper panel's text size for a faceplate this wide: as
/// designed at 440 px, growing with the faceplate up to twice that, so the
/// texts keep their size next to the calculator's.
inline double fontScale(double faceplateWidth) { return std::clamp(faceplateWidth / 440.0, 1.0, 2.0); }

/// Monospaced header action (RESUME / FREEZE / … in the debug headers).
inline QString headerButton(const QColor& color, int pixelSize = 13) {
    return QStringLiteral(
        "QToolButton { color: %1; background: transparent; border: none; font-weight: bold;"
        " font-family: monospace; font-size: %5px; }"
        "QToolButton:disabled { color: rgba(%2,%3,%4,0.4); }")
        .arg(rgb(color)).arg(color.red()).arg(color.green()).arg(color.blue()).arg(pixelSize);
}

} // namespace ChromeStyle
