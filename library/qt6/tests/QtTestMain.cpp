#include "TestHarness.hpp"
#include <QApplication>
#include <cstring>
#include <QtGlobal>

// The runner of calcu_qt6_tests: the repository's one runner (ADR 0038,
// library/testing/TestMain.cpp) with a QApplication around it, on the
// platform the environment names (CTest: QT_QPA_PLATFORM=offscreen). An
// optional argument filters by substring of the test name.

namespace {
// The offscreen platform's notes on what it does not do (size hints, font
// aliases) are not findings of the tests.
void quietOffscreen(QtMsgType type, const QMessageLogContext&, const QString& msg) {
    if (type == QtDebugMsg || type == QtInfoMsg) return;
    if (msg.contains(QStringLiteral("propagateSizeHints")) || msg.contains(QStringLiteral("font family"))) return;
    std::fprintf(stderr, "%s\n", qPrintable(msg));
}
} // namespace

int main(int argc, char** argv) {
    qInstallMessageHandler(quietOffscreen);
    const char* filter = argc > 1 ? argv[1] : nullptr;
    int qtArgc = 1;
    QApplication app(qtArgc, argv);
    int ran = 0;
    for (auto& c : testharness::registry()) {
        if (filter && !std::strstr(c.name, filter)) continue;
        int before = testharness::failCount();
        testharness::currentName() = c.name;
        c.fn();
        ++ran;
        if (testharness::failCount() != before) std::fprintf(stderr, "  in %s\n", c.name);
    }
    std::printf("%d tests (%d skipped), %d checks passed, %d failed\n", ran, testharness::skipCount(),
                testharness::passCount(), testharness::failCount());
    return testharness::failCount() == 0 ? 0 : 1;
}
