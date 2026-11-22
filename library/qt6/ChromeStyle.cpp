#include "qt6/ChromeStyle.hpp"
#include <QPalette>
#include <QWidget>

namespace calcu::qt6::chrome {

QColor qcolour(const shell::Colour& c) { return QColor(c.r, c.g, c.b, c.a); }

QString qcss(const shell::Colour& c) { return QStringLiteral("rgba(%1,%2,%3,%4)").arg(c.r).arg(c.g).arg(c.b).arg(c.a); }

bool darkAppearance(const QWidget* w) { return w->palette().color(QPalette::Window).lightness() < 128; }

QString header(const shell::ChromeColours& c, const shell::ChromeSizes& s) {
    const int h = qRound(s.panelHeaderHeight);
    return QStringLiteral("background-color: %1; color: %2; min-height: %3px; max-height: %3px; padding: 0;")
        .arg(qcss(c.header), qcss(c.headerText))
        .arg(h);
}

QString buttonBar(const shell::ChromeColours& c, const shell::ChromeSizes& s) {
    const int h = qRound(s.panelButtonsHeight);
    return QStringLiteral("background-color: %1; min-height: %2px; max-height: %2px;").arg(qcss(c.buttonBar)).arg(h);
}

QString pillCore(const shell::ChromeColours& c, const shell::ChromeSizes& s, const shell::Colour& background) {
    QString font;
    if (s.pillFontSize > 0) font += QStringLiteral(" font-size: %1px;").arg(qRound(s.pillFontSize));
    if (s.pillBold) font += QStringLiteral(" font-weight: bold;");
    // The height less the padding: a stylesheet's height is the content's.
    const int content = qRound(s.pill.height - 2 * s.pill.paddingY);
    return QStringLiteral("border: none; border-radius: %1px; padding: %2px %3px; min-height: %4px; max-height: %4px;"
                          " background-color: %5; color: %6;%7")
        .arg(qRound(s.pill.radius))
        .arg(qRound(s.pill.paddingY))
        .arg(qRound(s.pill.paddingX))
        .arg(content)
        .arg(qcss(background), qcss(c.pillText), font);
}

QString pills(const shell::ChromeColours& c, const shell::ChromeSizes& s, const QString& sel) {
    const shell::Colour* on = c.find("pill-on");
    return QStringLiteral("%1 { %2 }"
                          "%1:hover:!disabled { background-color: %3; }"
                          "%1:pressed, %1:hover:pressed { background-color: %4; }"
                          "%1:checked { background-color: %5; }"
                          "%1:disabled { background-color: %6; color: %7; }"
                          "%1::menu-indicator { image: none; width: 0; }")
        .arg(sel, pillCore(c, s, c.pill), qcss(c.pillHover), qcss(c.pillPressed), qcss(on ? *on : c.pillPressed),
             qcss(c.pillOff), qcss(c.pillTextOff));
}

} // namespace calcu::qt6::chrome
