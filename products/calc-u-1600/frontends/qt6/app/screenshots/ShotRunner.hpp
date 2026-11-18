#pragma once
#include <QElapsedTimer>
#include <QList>
#include <QObject>
#include <QPointer>
#include <QTcpSocket>
#include <QPoint>
#include <QSize>
#include <QString>
#include <QStringList>
#include <cstdint>
#include <functional>

#include "ShotScenario.hpp"

namespace calcu::qt6 {
class AttachLineClient;
}
using calcu::qt6::AttachLineClient;
class MainWindow;
class QAction;
class QProcess;
class QWidget;

// Plays a ShotScenario against the live MainWindow and writes its captures.
//
// Timer-driven, one step per QTimer::singleShot, never blocking the GUI
// thread: a step that opens a modal dialog or a menu (both run their own
// nested event loop) is performed from a zero-delay timer *after* the next
// step is already scheduled, so the script carries on inside that nested
// loop -- captures the dialog, then closes it.
//
// The machine is a client's of harness attach: the
// steps that load a preset, let time pass, press keys and type go to the
// app's own attach endpoint as the harness's `preset`, `wait`, `key`,
// `type`, `reset`, `cold-boot` (AttachLineClient); the runner never steps
// the machine itself, so the app runs on its own clock and the captures are
// taken after `wait-idle` (the bare `wait`) where a shot asks for it. UI
// steps (the shell's widgets by their object names; a
// pointer on the faceplate for `hold-key:`) stay widget steps.
class ShotRunner : public QObject {
    Q_OBJECT
public:
    struct Options {
        QString outDirOverride;  // replaces the scenario's `out:`
        QStringList only;        // shot names to run; empty = all
        bool failFast = false;
    };

    // `attachPort`: the app's attach endpoint (SessionHost::openAttach on port 0).
    ShotRunner(MainWindow* window, ShotScenario scenario, Options options, std::uint16_t attachPort,
               QObject* parent = nullptr);

    // Validates `only` against the scenario, applies window size and
    // appearance, then starts stepping. finished() always follows.
    void start();

signals:
    void finished(int exitCode);

private:
    MainWindow* m_window;
    QPointer<QTcpSocket> m_dapClient;   // `dap-client: <port>`
    std::uint16_t m_attachPort = 0;
    AttachLineClient* m_client = nullptr;
    // A machine step's command went to attach: the next step waits for its reply.
    bool m_awaiting = false;
    void machineStep(const QString& line, int line_, std::function<void(const QStringList&)> after = {});
    bool m_pointerDown = false;   // `hold-key:` holds the faceplate's pointer
    QPoint m_pointerAt;
    void pointer(bool down, const QPoint& at = QPoint());
    ShotScenario m_scenario;
    Options m_options;
    QString m_outDir;

    QList<int> m_shotOrder; // indices into m_scenario.shots
    int m_shotPos = 0;
    int m_stepIdx = 0;
    int m_failures = 0;
    int m_written = 0;
    bool m_done = false;
    // Native (macOS menu-bar) menus opened by `menu:` -- how many levels
    // deep, so `close` knows how many Escapes to send.
    int m_nativeMenuDepth = 0;
    // The osascript that opened them. It only exits once the menu closes,
    // so an early non-zero exit means it failed (usually a missing
    // Automation/Accessibility permission) -- see menuScriptError().
    QProcess* m_menuScript = nullptr;
    bool menuScriptError(QString* error);
    // osascript gives no word on when the menu is up, and a fixed pause
    // lost the race whenever System Events was slow: the capture missed
    // the menu, the Escape went out before it opened, and the menu then
    // stayed open, blocking the run. So the runner watches the menus
    // themselves (QMenu::aboutToShow/-Hide, forwarded from the NSMenu
    // delegate) and holds the next step until `menu:` has really opened
    // the deepest level, or `close` really closed the top one.
    enum class NativeMenuWait { None, Open, Close };
    NativeMenuWait m_nativeMenuWait = NativeMenuWait::None;
    QElapsedTimer m_nativeMenuWaitClock;
    int m_nativeMenuWaitLine = 0;  // the `menu:` step, for its error
    int m_nativeMenuCloseDepth = 0; // Escapes per (re)try
    int m_nativeMenuCloseTries = 0;
    bool m_nativeMenuShown = false; // deepest level has opened
    bool m_nativeMenuOpen = false;  // top level is open
    QList<QMetaObject::Connection> m_nativeMenuConnections;
    // Called before each step while a wait is pending: true = go ahead,
    // false = it has scheduled the next try itself.
    bool nativeMenuSettled();
    void sendNativeMenuEscapes(int count);

    const Shot& currentShot() const { return m_scenario.shots[m_shotOrder[m_shotPos]]; }
    void scheduleNext(int delayMs);
    void runNextStep();
    // Performs `step`. `delayMs` is how long to let the UI settle before
    // the next one; `deferred` (UI verbs) is run from a zero-delay timer
    // after the next step has been scheduled -- see the class comment.
    // Lets go of the held faceplate key and gives the ROM time to react.
    void releaseKey();
    // `host-key-down` / `-up`: the key event a host keyboard gives the
    // window (`shift` or one character), Shift held as a modifier.
    void sendHostKey(const QString& name, bool down);
    bool m_hostShiftDown = false;
    bool runStep(const ShotStep& step, int* delayMs, std::function<void()>* deferred, QString* error);
    void failStep(const QString& message, int line);
    // The current shot's `window:`, else the scenario's (invalid = none).
    QSize wantedWindowSize() const;
    void applyWindowSize();
    void finishAll();

    QWidget* findWidget(const QString& objectName, QString* error) const;
    QAction* findMenuAction(const QString& path, QString* error) const;
    bool openMenu(const QString& path, std::function<void()>* deferred, QString* error);
    bool closeTopmost(QString* error);
    void closeAllTransient();
    bool capture(const ShotCaptureSpec& spec, QString* error);
    void log(const QString& message) const;
};
