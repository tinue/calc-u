#include "TestHarness.hpp"
#include "qt6/FixedFont.hpp"
#include <QFontDatabase>
#include <QGuiApplication>
#include <QLabel>

// The fixed-width font is a family the platform has, never a name Qt must
// stand in for (that search costs tens of milliseconds and a warning), and
// a stylesheet that names it gets that family.

using calcu::qt6::fixedFontCss;
using calcu::qt6::fixedFont;

TEST(fixed_font_is_a_family_of_the_platform) {
    CHECK(!fixedFont().family().isEmpty());
    CHECK(fixedFont().fixedPitch());
    CHECK(fixedFont().styleHint() == QFont::TypeWriter);
    // The offscreen platform of the tests names a generic "monospace" and has
    // no such family; a real platform (macOS: Menlo) has its own.
    SKIP_UNLESS(QGuiApplication::platformName() != QLatin1String("offscreen"), "the offscreen platform has no fixed font");
    CHECK(QFontDatabase::hasFamily(fixedFont().family()));
}

TEST(fixed_font_in_a_stylesheet) {
    CHECK(fixedFontCss(11) == QStringLiteral("font: 11px \"") + fixedFont().family() + QStringLiteral("\";"));
    CHECK(fixedFontCss(9, true) == QStringLiteral("font: bold 9px \"") + fixedFont().family() + QStringLiteral("\";"));
    QLabel label(QStringLiteral("SCOM"));
    label.setStyleSheet(QStringLiteral("QLabel { ") + fixedFontCss(11) + QStringLiteral(" }"));
    label.ensurePolished();
    CHECK(label.font().family() == fixedFont().family());
    CHECK(label.font().pixelSize() == 11 && !label.font().bold());
    QLabel head(QStringLiteral("SCOM"));
    head.setStyleSheet(QStringLiteral("QLabel { ") + fixedFontCss(9, true) + QStringLiteral(" }"));
    head.ensurePolished();
    CHECK(head.font().family() == fixedFont().family());
    CHECK(head.font().pixelSize() == 9 && head.font().bold());
}
