#include <QApplication>
#include <QCommandLineParser>
#include <QDir>
#include <QFileInfo>
#include <QFileOpenEvent>
#include <QIcon>
#include <QTemporaryDir>
#include <cstdio>
#include <functional>
#include <string>
#include <utility>
#include <vector>
#include "AppLogging.hpp"
#include "AppPaths.hpp"
#include "AppSettings.hpp"
#ifdef __APPLE__
#include "MacAppSupport.h"
#endif
#include "MainWindow.hpp"
#include "debug/DebugController.hpp"
#include "screenshots/ShotRunner.hpp"
#include "screenshots/ShotScenario.hpp"
#include "MachineController.hpp"
#include "app/StartFile.hpp"
#include "opening/OpenRequest.hpp"
#include "sessions/LibraryTopics.hpp"
#include <QElapsedTimer>
#include <QTimer>
#include <cmath>

namespace {

// macOS hands a file dropped onto the Dock icon, or opened from Finder, to
// the application object as a QFileOpenEvent -- at a cold launch before
// there is a window. Kept until the window takes them; each is the session's
// `open` with the way Open With (LIB-13 AC-21; MainWindow::openFile).
class Application : public QApplication {
public:
    using QApplication::QApplication;

    void setFileOpenHandler(std::function<void(const QString&)> handler) {
        m_onFileOpen = std::move(handler);
        for (const QString& path : std::exchange(m_pending, {})) m_onFileOpen(path);
    }

protected:
    bool event(QEvent* event) override {
        if (event->type() != QEvent::FileOpen) return QApplication::event(event);
        const QString path = static_cast<QFileOpenEvent*>(event)->file();
        if (m_onFileOpen)
            m_onFileOpen(path);
        else
            m_pending.append(path);
        return true;
    }

private:
    std::function<void(const QString&)> m_onFileOpen;
    QStringList m_pending;
};

#ifdef CALCU1600_TEST_HOOKS
// --pacing-probe <s> (LIB-17 AC-22; design §5, adapter until the Qt6
// frontend gets attach): once the start is over, `s` seconds of wall time at
// regular speed; then the session's request for emulated time (the one the
// harness's `ticks` uses), printed with the wall time. Exit 0 when the two
// grew alike within the bound of AC-1 (one host call of 16.7 ms plus the
// last instruction; 25 ms here), else 1.
int runPacingProbe(QApplication& app, double seconds) {
    MainWindow window;
    window.show();
    MachineController* c = window.controller();
    QElapsedTimer wall;
    double t0 = -1;
    auto emulatedSeconds = [c] {
        const auto r = c->listener().ask(calcu::sessions::kTicksRequest);
        const auto* t = r.done ? calcu::sessions::payloadAs<calcu::sessions::EmulatedTimeReply>(r.result) : nullptr;
        return t ? t->seconds : -1.0;
    };
    QTimer poll;
    int result = 1;
    QObject::connect(&poll, &QTimer::timeout, &app, [&] {
        if (t0 < 0) {
            if (c->busy() || !c->hasLiveMachine()) return;
            t0 = emulatedSeconds();
            wall.start();
            return;
        }
        if (wall.elapsed() < static_cast<qint64>(seconds * 1000)) return;
        const double w = static_cast<double>(wall.nsecsElapsed()) * 1e-9;
        const double e = emulatedSeconds() - t0;
        result = std::fabs(e - w) <= 0.025 ? 0 : 1;
        std::printf("pacing-probe: wall %.3f s, emulated %.3f s, difference %+.4f s, %llu host calls, longest gap %.1f ms, %.1f ms in calls: %s\n",
                    w, e, e - w, static_cast<unsigned long long>(c->calls()), c->longestGapMs(), c->callMs(),
                    result == 0 ? "ok" : "beyond the bound");
        poll.stop();
        app.quit();
    });
    poll.start(5);
    app.exec();
    return result;
}
#endif

}  // namespace

int main(int argc, char** argv) {
    AppLogging::install();
#ifdef __APPLE__
    macDisableWindowRestoration(); // see MacAppSupport.h: the restore prompt deadlocks the startup preset
    macDisablePressAndHold();      // see MacAppSupport.h: held letters stay held keys
    macKeepTimersPrecise();        // see MacAppSupport.h: real-time pace in the background (row 1600-28)
#endif
    Application app(argc, argv);
    // Without this, the running window's title-bar/taskbar icon is
    // whatever the platform defaults to (e.g. a generic AppImage icon on
    // Linux) -- the .desktop file's Icon= only covers desktop-environment
    // integration (launcher/file-manager icon), not the live window.
    // Not on macOS: there it would replace the bundle's system-masked
    // Dock icon (AppIcon.icon) with this full-bleed PNG.
#ifndef __APPLE__
    app.setWindowIcon(QIcon(":/app/icon.png"));
#endif

    // Scripted screenshots (docs/developer/screenshots/README.md): play a
    // scenario, write its images, quit with 0 (all written) or 1 (any failed).
    QCommandLineParser parser;
    const QCommandLineOption shotsOption(QStringLiteral("shots"),
                                         QStringLiteral("Run a screenshot scenario (*.shots.yaml), then quit."),
                                         QStringLiteral("scenario"));
    const QCommandLineOption shotsOutOption(QStringLiteral("shots-out"),
                                            QStringLiteral("Write the images here instead of the scenario's 'out:'."),
                                            QStringLiteral("dir"));
    const QCommandLineOption shotsOnlyOption(QStringLiteral("shots-only"),
                                             QStringLiteral("Only run these shots (comma-separated names)."),
                                             QStringLiteral("names"));
    const QCommandLineOption shotsFailFastOption(QStringLiteral("shots-fail-fast"),
                                                 QStringLiteral("Stop at the first failing shot."));
    const QCommandLineOption dapOption(QStringLiteral("dap"),
                                       QStringLiteral("Accept a debugger on 127.0.0.1:<port> for this run (Settings unchanged)."),
                                       QStringLiteral("port"));
    parser.addOptions({shotsOption, shotsOutOption, shotsOnlyOption, shotsFailFastOption, dapOption});
#ifdef CALCU1600_TEST_HOOKS
    const QCommandLineOption pacingProbeOption(QStringLiteral("pacing-probe"),
                                               QStringLiteral("Test hook: check the pacing over <s> seconds, then quit."),
                                               QStringLiteral("s"));
    parser.addOption(pacingProbeOption);
#endif
    // parse(), not process(): a normal launch must survive whatever extra
    // arguments macOS or an IDE add (`-NSDocumentRevisionsDebugMode YES`).
    const bool parsed = parser.parse(QCoreApplication::arguments());

    if (parser.isSet(dapOption)) DebugController::setCommandLinePort(parser.value(dapOption).toInt());

#ifdef CALCU1600_TEST_HOOKS
    if (parser.isSet(pacingProbeOption)) {
        // Isolated as the --shots runner: the user's settings are not used.
        QTemporaryDir probeHome;
        calcu::settings::HostDirs dirs;
        dirs.frontend = calcu::settings::FrontendKind::Qt6;
        dirs.home = probeHome.filePath(QStringLiteral("home")).toStdString();
        dirs.settingsBase = probeHome.filePath(QStringLiteral("settings")).toStdString();
        AppPaths::useHostDirs(dirs);
        AppSettings::setStartupModelPreference(QStringLiteral("PC-1600"));   // AC-22: a PC-1600
        return runPacingProbe(app, parser.value(pacingProbeOption).toDouble());
    }
#endif
    if (!parser.isSet(shotsOption)) {
        MainWindow window;
        window.show();
        app.setFileOpenHandler([&window](const QString& path) { window.openFile(path, calcu::opening::Way::OpenWith); });
        // LIB-13 AC-21 (OQ-2, D-14): the last argument that names a file is
        // opened; the session keeps it until the start is over (AC-7).
        std::vector<std::string> arguments;
        for (const QString& a : QCoreApplication::arguments()) arguments.push_back(a.toStdString());
        if (const auto start = shared::startFile(arguments, [](const std::string& p) {
                return QFileInfo(QString::fromStdString(p)).isFile();
            }))
            window.openFile(QString::fromStdString(*start), calcu::opening::Way::Start);
        const int result = app.exec();
        app.setFileOpenHandler({});
        return result;
    }

    if (!parsed) {
        std::fprintf(stderr, "[shots] %s\n", qPrintable(parser.errorText()));
        return 2;
    }
    ShotScenario scenario;
    QString error;
    if (!parseShotScenario(parser.value(shotsOption), &scenario, &error)) {
        std::fprintf(stderr, "[shots] %s\n", qPrintable(error));
        return 2;
    }
    // A throwaway home and settings folder: the user's own settings
    // (default presets, last model, folders) and saved cards must not show
    // up in the images, and the run must not change them (LIB-9 AC-18).
    QTemporaryDir isolated;
    if (!isolated.isValid()) {
        std::fprintf(stderr, "[shots] could not create a temporary directory\n");
        return 2;
    }
    calcu::settings::HostDirs shotDirs;
    shotDirs.frontend = calcu::settings::FrontendKind::Qt6;
    shotDirs.home = isolated.filePath(QStringLiteral("home")).toStdString();
    shotDirs.settingsBase = isolated.filePath(QStringLiteral("settings")).toStdString();
    AppPaths::useHostDirs(shotDirs);
    // Qt's own file/color dialogs are widgets the script can capture and
    // close; the native ones are not.
    QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);

    ShotRunner::Options options;
    options.outDirOverride = parser.value(shotsOutOption);
    if (parser.isSet(shotsOnlyOption))
        options.only = parser.value(shotsOnlyOption).split(QLatin1Char(','), Qt::SkipEmptyParts);
    for (QString& name : options.only) name = name.trimmed();
    options.failFast = parser.isSet(shotsFailFastOption);

    MainWindow window;
    ShotRunner runner(&window, scenario, options);
    QObject::connect(&runner, &ShotRunner::finished, &app, &QCoreApplication::exit);
    window.show();
    runner.start();
    return app.exec();
}
