#include "MainWindow.hpp"
#include "EmulatorController.hpp"
#include "TIFileKinds.hpp"
#include "qt6/FileOpening.hpp"
#include "qt6/SessionHost.hpp"
#include <QApplication>
#include <QIcon>
#include <QPalette>
#include <QStyleFactory>
#include <QTimer>
#include <cstdio>
#include <memory>
#ifdef CALCU_ATTACH
#include "HarnessProduct.hpp"
#include "harness/Product.hpp"
#include "qt6/PacingProbe.hpp"
#include <cmath>
#endif

namespace {

/// The Swift app is always dark (preferredColorScheme(.dark)); so is this.
void applyDarkPalette(QApplication& app) {
    app.setStyle(QStyleFactory::create(QStringLiteral("Fusion")));
    QPalette p;
    p.setColor(QPalette::Window, QColor(37, 37, 37));
    p.setColor(QPalette::WindowText, Qt::white);
    p.setColor(QPalette::Base, QColor(28, 28, 28));
    p.setColor(QPalette::AlternateBase, QColor(45, 45, 45));
    p.setColor(QPalette::Text, Qt::white);
    p.setColor(QPalette::Button, QColor(50, 50, 50));
    p.setColor(QPalette::ButtonText, Qt::white);
    p.setColor(QPalette::Highlight, QColor(10, 132, 255));
    p.setColor(QPalette::HighlightedText, Qt::white);
    p.setColor(QPalette::ToolTipBase, QColor(50, 50, 50));
    p.setColor(QPalette::ToolTipText, Qt::white);
    p.setColor(QPalette::Disabled, QPalette::Text, QColor(128, 128, 128));
    p.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(128, 128, 128));
    app.setPalette(p);
}

#ifdef CALCU_ATTACH
// --pacing-probe <s> (LIB-17 AC-19, design §5, P59-5; LIB-14 design §10.7):
// the library's measurement through attach (qt6::runPacingProbe); today's
// lines, and exit 0 when the emulated time grew by the wall time within
// AC-1's bound (the TI-59's slice, 20 ms, LIB-17 AC-15) plus one host call,
// else 1.
int runPacingProbe(QApplication& app, std::uint16_t port, double seconds) {
    return calcu::qt6::runPacingProbe(app, port, seconds, [](const calcu::qt6::PacingSample& s) {
        const double w = s.wall;
        const double emulated = s.emulated();
        const double bound = 0.020 + 1.0 / 60;
        const bool ok = s.ticksAfter > s.ticksBefore && std::fabs(emulated - w) <= bound;
        std::printf("ticks: %llu\nseconds: %.6f\nwall: %.6f\nemulated: %.6f\ndifference: %+.6f\nbound: %.6f\n%s\n",
                    s.ticksAfter - s.ticksBefore, s.secondsAfter, w, emulated, emulated - w, bound,
                    ok ? "ok" : "FAIL: emulated time did not keep wall time");
        return ok ? 0 : 1;
    });
}
#endif

} // namespace

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    calcu::qt6::FileOpenFilter fileOpen;   // macOS Open With, at a cold launch before the window (LIB-13 AC-14)
    app.installEventFilter(&fileOpen);
    QApplication::setApplicationName(QStringLiteral("Calc-U-59"));
    QApplication::setOrganizationName(QStringLiteral("Calc-U-59"));
    QApplication::setWindowIcon(QIcon(QStringLiteral(":/app/icon.png")));   // also About's (LIB-14 D-38)
    applyDarkPalette(app);

    // Developer option: --grab <file.png> [--after <ms>] saves a screenshot of the
    // window and quits (run with QT_QPA_PLATFORM=offscreen to check rendering
    // without showing a window). The file named at the start (LIB-13 AC-14,
    // D-18, ledger 59-47): the program's own options and their values taken
    // out, the last argument that does not start with `-` and names an
    // existing file, opened as what it is; the session keeps it until the
    // start has ended (AC-7).
    QStringList args = QApplication::arguments();
    QString grabPath;
    int grabAfter = 3000;
    double probeSeconds = 0;
    std::vector<std::string> all;
    for (int i = 0; i < args.size(); ++i) {
        all.push_back(args[i].toStdString());
        if (args[i] == QLatin1String("--grab") && i + 1 < args.size()) grabPath = args[i + 1];
#ifdef CALCU_ATTACH
        else if (args[i] == QLatin1String("--pacing-probe") && i + 1 < args.size()) probeSeconds = args[i + 1].toDouble();
#endif
        else if (args[i] == QLatin1String("--after") && i + 1 < args.size()) grabAfter = args[i + 1].toInt();
    }

    // A window even when ROM files are missing: the shell shows which in
    // place of the calculator, and Help and About work (LIB-14 AC-19; row 59-54).
#ifdef CALCU_ATTACH
    // LIB-14 design §10.7: harness attach in development and test builds
    // (`--attach-port N`, else 59059; 0: the system picks); the pacing
    // probe is its client in this process. The product outlives the window.
    std::unique_ptr<calcu::harness::HarnessProduct> attachProduct = cu59::makeHarnessProduct();
#endif
    MainWindow window;
#ifdef CALCU_ATTACH
    const std::uint16_t port = probeSeconds > 0 ? 0 : calcu::qt6::SessionHost::attachPortFrom(args);
    const std::string problem = window.host().openAttach(
        *attachProduct, port,
        window.controller().app().libraryParts());
    if (probeSeconds > 0 && !problem.empty()) {
        std::fprintf(stderr, "pacing-probe: attach: %s\n", problem.c_str());
        return 2;
    }
#endif
    window.show();
    window.start();
#ifdef CALCU_ATTACH
    if (probeSeconds > 0) return runPacingProbe(app, window.host().attachPort(), probeSeconds);
#endif

    if (const auto startFile = calcu::qt6::startFile(cu59::programArguments(all)))
        window.openFile(*startFile, calcu::opening::Way::Start);
    fileOpen.setHandler([&window](const QString& path) { window.openFile(path, calcu::opening::Way::OpenWith); });
    if (!grabPath.isEmpty()) {
        QTimer::singleShot(grabAfter, &window, [&window, grabPath] {
            window.grab().save(grabPath);
            QApplication::quit();
        });
    }
    const int result = QApplication::exec();
    fileOpen.setHandler({});
    return result;
}
