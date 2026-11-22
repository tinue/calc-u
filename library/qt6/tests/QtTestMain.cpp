#include "TestHarness.hpp"
#include <QApplication>
#include <QtGlobal>

// The runner of calcu_qt6_tests: the repository's one runner
// (library/testing/TestMain.cpp) with a QApplication around it, on the
// platform the environment names (CTest: QT_QPA_PLATFORM=offscreen); its
// arguments are the runner's (TestHarness.hpp).

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
    int qtArgc = 1;
    QApplication app(qtArgc, argv);
    return testharness::runTests(argc, argv);
}
