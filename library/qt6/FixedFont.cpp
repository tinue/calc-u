#include "qt6/FixedFont.hpp"

#include <QFontDatabase>

namespace calcu::qt6 {

const QFont& fixedFont() {
    static const QFont font = [] {
        QFont f = QFontDatabase::systemFont(QFontDatabase::FixedFont);
        f.setStyleHint(QFont::TypeWriter);   // where the platform has no such font, a fixed-width stand-in
        f.setFixedPitch(true);
        return f;
    }();
    return font;
}

QString fixedFontCss(int px, bool bold) {
    return QStringLiteral("font: %1%2px \"%3\";").arg(bold ? QStringLiteral("bold ") : QString()).arg(px).arg(fixedFont().family());
}

} // namespace calcu::qt6
