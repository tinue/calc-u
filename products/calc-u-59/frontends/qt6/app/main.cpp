#include "MainWindow.hpp"
#include "EmulatorController.hpp"
#include "TIFileKinds.hpp"
#include "opening/StartFile.hpp"
#include "qt6/SessionHost.hpp"
#include <QApplication>
#include <QFileInfo>
#include <QFileOpenEvent>
#include <QIcon>
#include <QPalette>
#include <QStyleFactory>
#include <QTimer>
#include <cstdio>
#include <memory>
#ifdef CALCU_ATTACH
#include "HarnessProduct.hpp"
#include "harness/Product.hpp"
#include "qt6/AttachLineClient.hpp"
#include <QElapsedTimer>
#include <cmath>
#endif

namespace {

/// Catches files opened from the Dock or Finder (macOS QFileOpenEvent), which
/// can arrive before the window exists.
class App : public QApplication {
public:
    using QApplication::QApplication;
    MainWindow* window = nullptr;
    QString pendingFile;

protected:
    bool event(QEvent* e) override {
        if (e->type() == QEvent::FileOpen) {
            QString path = static_cast<QFileOpenEvent*>(e)->file();
            if (window) window->openFile(path, false);   // LIB-13 AC-14: Open With
            else pendingFile = path;
            return true;
        }
        return QApplication::event(e);
    }
};

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
double secondsOf(const calcu::qt6::AttachLineClient::Reply& r) {
    for (const QString& l : r.lines)
        if (l.startsWith(QLatin1String("seconds: "))) return l.mid(9).toDouble();
    return -1;
}
unsigned long long ticksOf(const calcu::qt6::AttachLineClient::Reply& r) {
    for (const QString& l : r.lines)
        if (l.startsWith(QLatin1String("ticks: "))) return l.mid(7).toULongLong();
    return 0;
}

// --pacing-probe <s> (LIB-17 AC-19, design §5, P59-5; LIB-14 design §10.7):
// an attach client in the app's own process. Once the start is over (the
// bare `wait`), `ticks` through attach, `s` seconds of wall time, `ticks`
// again; today's lines, and exit 0 when the emulated time grew by the wall
// time within AC-1's bound (the TI-59's slice, 20 ms, LIB-17 AC-15) plus one
// host call, else 1. It steps the machine only through attach.
int runPacingProbe(QApplication& app, std::uint16_t port, double seconds) {
    calcu::qt6::AttachLineClient client;
    if (!client.connectTo(port)) {
        std::printf("pacing-probe: attach is not open on port %u\n", port);
        return 1;
    }
    int result = 1;
    calcu::qt6::AttachLineClient::Reply first;
    QElapsedTimer wall;
    client.send(QStringLiteral("wait"), [&](const calcu::qt6::AttachLineClient::Reply&) {
        client.send(QStringLiteral("ticks"), [&](const calcu::qt6::AttachLineClient::Reply& r) {
            first = r;
            wall.start();
            QTimer::singleShot(static_cast<int>(seconds * 1000), &app, [&] {
                client.send(QStringLiteral("ticks"), [&](const calcu::qt6::AttachLineClient::Reply& r2) {
                    const double w = static_cast<double>(wall.nsecsElapsed()) * 1e-9;
                    const double emulated = secondsOf(r2) - secondsOf(first);
                    const double bound = 0.020 + 1.0 / 60;
                    const bool ok = ticksOf(r2) > ticksOf(first) && std::fabs(emulated - w) <= bound;
                    std::printf("ticks: %llu\nseconds: %.6f\nwall: %.6f\nemulated: %.6f\ndifference: %+.6f\nbound: %.6f\n%s\n",
                                ticksOf(r2) - ticksOf(first), secondsOf(r2), w, emulated, emulated - w, bound,
                                ok ? "ok" : "FAIL: emulated time did not keep wall time");
                    std::fflush(stdout);
                    result = ok ? 0 : 1;
                    app.quit();
                });
            });
        });
    });
    app.exec();
    return result;
}
#endif

} // namespace

int main(int argc, char* argv[]) {
    App app(argc, argv);
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
    app.window = &window;
#ifdef CALCU_ATTACH
    const std::uint16_t port = probeSeconds > 0 ? 0 : calcu::qt6::SessionHost::attachPortFrom(args);
    const std::string problem = window.host().openAttach(
        *attachProduct, port,
        calcu::harness::SessionParts{&window.controller().app().presetsPart()->vocabulary(), window.controller().app().mediaPart()});
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

    const auto startFile = calcu::opening::startFile(cu59::programArguments(all), [](const std::string& f) {
        return QFileInfo(QString::fromStdString(f)).isFile();
    });
    if (startFile) window.openFile(QString::fromStdString(*startFile), true);
    else if (!app.pendingFile.isEmpty()) window.openFile(app.pendingFile, false);
    if (!grabPath.isEmpty()) {
        QTimer::singleShot(grabAfter, &window, [&window, grabPath] {
            window.grab().save(grabPath);
            QApplication::quit();
        });
    }
    return QApplication::exec();
}
