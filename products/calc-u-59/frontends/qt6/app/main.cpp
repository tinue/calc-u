#include "AppSettings.hpp"
#include "EmulatorController.hpp"
#include "MainWindow.hpp"
#include <QApplication>
#include <QFileOpenEvent>
#include <QIcon>
#include <QPalette>
#include <QStyleFactory>
#include <QTimer>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <memory>

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
            if (window) window->openStateFile(path);
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

} // namespace

int main(int argc, char* argv[]) {
    App app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("Calc-U-59"));
    QApplication::setOrganizationName(QStringLiteral("Calc-U-59"));
    QApplication::setWindowIcon(QIcon(QStringLiteral(":/app/icon.png")));
    applyDarkPalette(app);

    EmulatorController controller;
    if (!controller.started()) return 1;   // the ROM files are missing (LIB-12 AC-10)
    MainWindow window(controller);
    app.window = &window;
    window.show();

    // Developer option: --grab <file.png> [--after <ms>] saves a screenshot of the
    // window and quits (run with QT_QPA_PLATFORM=offscreen to check rendering
    // without showing a window). Any other argument is a preset to open.
    QStringList args = QApplication::arguments();
    QString grabPath;
    int grabAfter = 3000;
    QString stateFile;
    double probeSeconds = 0;
    for (int i = 1; i < args.size(); ++i) {
        if (args[i] == QLatin1String("--grab") && i + 1 < args.size()) grabPath = args[++i];
#ifdef CALCU59_PACING_PROBE
        else if (args[i] == QLatin1String("--pacing-probe") && i + 1 < args.size()) probeSeconds = args[++i].toDouble();
#endif
        else if (args[i] == QLatin1String("--after") && i + 1 < args.size()) grabAfter = args[++i].toInt();
        else stateFile = args[i];
    }
    if (!stateFile.isEmpty()) window.openStateFile(stateFile);
    else if (!app.pendingFile.isEmpty()) window.openStateFile(app.pendingFile);
    if (!grabPath.isEmpty()) {
        QTimer::singleShot(grabAfter, &window, [&window, grabPath] {
            window.grab().save(grabPath);
            QApplication::quit();
        });
    }
#ifdef CALCU59_PACING_PROBE
    // Test hook of builds with the tests (LIB-17 design §5, P59-5; AC-19):
    // --pacing-probe <s> reads the session's emulated time (the request
    // LIB-11's `ticks` uses) a second after the start and again <s> seconds of
    // wall time later, prints both and the wall time, and exits 0 when the
    // emulated time grew by the wall time within AC-1's bound (the TI-59's
    // slice, 20 ms, LIB-17 AC-15) plus one host call (each reading is made
    // between two calls), else 1. Until the Qt6 frontend has attach (LIB-14).
    if (probeSeconds > 0) {
        using Clock = std::chrono::steady_clock;
        struct Probe { Clock::time_point wall; std::pair<uint64_t, double> time; };
        auto first = std::make_shared<Probe>();
        QTimer::singleShot(1000, &window, [&controller, first, probeSeconds, &window] {
            first->wall = Clock::now();
            first->time = controller.emulatedTime().value_or(std::make_pair(uint64_t{0}, 0.0));
            QTimer::singleShot(static_cast<int>(probeSeconds * 1000), &window, [&controller, first] {
                const auto wallNow = Clock::now();
                const auto now = controller.emulatedTime().value_or(std::make_pair(uint64_t{0}, 0.0));
                const double wall = std::chrono::duration<double>(wallNow - first->wall).count();
                const double emulated = now.second - first->time.second;
                const double bound = 0.020 + 1.0 / 60;
                const bool ok = now.first > first->time.first && std::fabs(emulated - wall) <= bound;
                std::printf("ticks: %llu\nseconds: %.6f\nwall: %.6f\nemulated: %.6f\ndifference: %+.6f\nbound: %.6f\n%s\n",
                            static_cast<unsigned long long>(now.first - first->time.first), now.second, wall, emulated,
                            emulated - wall, bound, ok ? "ok" : "FAIL: emulated time did not keep wall time");
                std::fflush(stdout);
                QApplication::exit(ok ? 0 : 1);
            });
        });
    }
#endif
    return QApplication::exec();
}
