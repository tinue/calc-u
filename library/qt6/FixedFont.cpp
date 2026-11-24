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

QString fixedFamilyCss() { return QStringLiteral("\"%1\"").arg(fixedFont().family()); }

} // namespace calcu::qt6
