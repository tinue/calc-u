#include <QApplication>
#include <QCommandLineParser>
#include <QDir>
#include <QFileInfo>
#include <QIcon>
#include <QTemporaryDir>
#include <cstdio>
#include <functional>
#include <optional>
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
#include "MachineController.hpp"
#include "opening/OpenRequest.hpp"
#include "qt6/FileOpening.hpp"
#ifdef CALCU_ATTACH
#include "app/HarnessProduct.hpp"
#include "harness/Product.hpp"
#include "screenshots/PacingProbe.hpp"
#include "screenshots/ShotRunner.hpp"
#include "screenshots/ShotScenario.hpp"
#include <memory>
#endif

int main(int argc, char** argv) {
    AppLogging::install();
#ifdef __APPLE__
    macDisableWindowRestoration(); // see MacAppSupport.h: the restore prompt deadlocks the startup preset
    macDisablePressAndHold();      // see MacAppSupport.h: held letters stay held keys
    macKeepTimersPrecise();        // see MacAppSupport.h: real-time pace in the background (row 1600-28)
#endif
    QApplication app(argc, argv);
    calcu::qt6::FileOpenFilter fileOpen;
    app.installEventFilter(&fileOpen);
    // Without this, the running window's title-bar/taskbar icon is
    // whatever the platform defaults to (e.g. a generic AppImage icon on
    // Linux) -- the .desktop file's Icon= only covers desktop-environment
    // integration (launcher/file-manager icon), not the live window.
    // Not on macOS: there it would replace the bundle's system-masked
    // Dock icon (AppIcon.icon) with this full-bleed PNG.
#ifndef __APPLE__
    app.setWindowIcon(QIcon(":/app/icon.png"));
#endif

    QCommandLineParser parser;
    const QCommandLineOption dapOption(QStringLiteral("dap"),
                                       QStringLiteral("Accept a debugger on 127.0.0.1:<port> for this run (Settings unchanged)."),
                                       QStringLiteral("port"));
    parser.addOption(dapOption);
#ifdef CALCU_ATTACH
    // LIB-14 AC-44 (D.6): development and test builds offer harness attach
    // (`--attach-port N`, else 59059; 0: the system picks); the screenshot
    // scenarios and the pacing probe are its clients in this process.
    // Scripted screenshots (docs/developer/screenshots/README.md): play a
    // scenario, write its images, quit with 0 (all written) or 1 (any failed).
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
    const QCommandLineOption pacingProbeOption(QStringLiteral("pacing-probe"),
                                               QStringLiteral("Test hook: check the pacing over <s> seconds, then quit."),
                                               QStringLiteral("s"));
    const QCommandLineOption attachPortOption(QStringLiteral("attach-port"),
                                              QStringLiteral("Harness attach on 127.0.0.1:<port> (0: any)."),
                                              QStringLiteral("port"));
    parser.addOptions({shotsOption, shotsOutOption, shotsOnlyOption, shotsFailFastOption, pacingProbeOption, attachPortOption});
#endif
    // parse(), not process(): a normal launch must survive whatever extra
    // arguments macOS or an IDE add (`-NSDocumentRevisionsDebugMode YES`).
    const bool parsed = parser.parse(QCoreApplication::arguments());

    // C1600-3 AC-2, EC-1: --dap <port> for this run; a port out of range is
    // said on standard error and in a notice, and the server is off.
    std::optional<long> dapPort;
    if (parser.isSet(dapOption)) {
        bool number = false;
        const long port = parser.value(dapOption).toLong(&number);
        dapPort = number ? port : -1;
    }
    const auto applyDapPort = [&dapPort](MachineController* controller) {
        if (!dapPort || !controller) return;
        if (const std::optional<std::string> refused = controller->setDapRunPort(*dapPort))
            std::fprintf(stderr, "--dap %ld: %s\n", *dapPort, refused->c_str());
    };

#ifdef CALCU_ATTACH
    const bool shots = parser.isSet(shotsOption);
    const bool probe = parser.isSet(pacingProbeOption);
#else
    const bool shots = false;
    const bool probe = false;
#endif
    if (!shots && !probe) {
        MainWindow window;
        applyDapPort(window.controller());
        window.show();
#ifdef CALCU_ATTACH
        std::unique_ptr<calcu::harness::HarnessProduct> attachProduct = cu1600::makeHarnessProduct();
        window.host().openAttach(*attachProduct, calcu::qt6::SessionHost::attachPortFrom(QCoreApplication::arguments()),
                                 window.controller()->app().libraryParts());
#endif
        window.start();
        fileOpen.setHandler([&window](const QString& path) { window.openFile(path, calcu::opening::Way::OpenWith); });
        // LIB-13 AC-21 (OQ-2, D-14): the last argument that names a file is
        // opened; the session keeps it until the start is over (AC-7).
        if (const auto start = calcu::qt6::startFile(QCoreApplication::arguments()))
            window.openFile(*start, calcu::opening::Way::Start);
        const int result = app.exec();
        fileOpen.setHandler({});
        return result;
    }

#ifdef CALCU_ATTACH
    if (!parsed) {
        std::fprintf(stderr, "[shots] %s\n", qPrintable(parser.errorText()));
        return 2;
    }
    // A throwaway home and settings folder: the user's own settings
    // (startup presets, last model, folders) and saved media must not show
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
    if (probe) AppSettings::setStartupModelPreference(QStringLiteral("PC-1600"));   // LIB-17 AC-22: a PC-1600

    ShotScenario scenario;
    QString error;
    if (shots && !parseShotScenario(parser.value(shotsOption), &scenario, &error)) {
        std::fprintf(stderr, "[shots] %s\n", qPrintable(error));
        return 2;
    }
    // Qt's own file/color dialogs are widgets the script can capture and
    // close; the native ones are not.
    QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);

    MainWindow window;
    applyDapPort(window.controller());   // C1600-3 AC-18's scenario: a server to attach a client to
    // Attach on a port the system picks, for the client in this process.
    std::unique_ptr<calcu::harness::HarnessProduct> attachProduct = cu1600::makeHarnessProduct();
    const std::string problem = window.host().openAttach(
        *attachProduct, 0,
        window.controller()->app().libraryParts());
    if (!problem.empty()) {
        std::fprintf(stderr, "[shots] attach: %s\n", problem.c_str());
        return 2;
    }
    window.show();
    window.start();
    if (probe) return runPacingProbe(app, window, window.host().attachPort(), parser.value(pacingProbeOption).toDouble());

    ShotRunner::Options options;
    options.outDirOverride = parser.value(shotsOutOption);
    if (parser.isSet(shotsOnlyOption))
        options.only = parser.value(shotsOnlyOption).split(QLatin1Char(','), Qt::SkipEmptyParts);
    for (QString& name : options.only) name = name.trimmed();
    options.failFast = parser.isSet(shotsFailFastOption);
    ShotRunner runner(&window, scenario, options, window.host().attachPort());
    QObject::connect(&runner, &ShotRunner::finished, &app, &QCoreApplication::exit);
    runner.start();
    return app.exec();
#else
    (void)parsed;
    return 0;
#endif
}
