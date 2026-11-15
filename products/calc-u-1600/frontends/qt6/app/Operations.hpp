#pragma once

#include <QObject>
#include <QString>

#include <functional>
#include <memory>

#include "MachineController.hpp"

class QProgressDialog;
class QTimer;
class QWidget;

// ── Operations: the window's side of the session's operations ────────────
//
// A preset, a reset, a model switch, a plotter power cycle, a load: every
// one -- from a menu, the control bar or the debugger -- is a command of the
// app session (`c1600.*`, app/SharpTopics.hpp). An operation runs in parts,
// one per host call of the frame timer, so the window keeps painting and
// the display and paper keep updating (LIB-3 AC-35; 0.8.1 ran it to its end). While
// one runs, the window shows a wait cursor, and "Loading…" once it has run
// at full speed for half a second (the busy topic's speed, LIB-3 AC-14;
// 0.8.1's kPopupDelayMs). The reply comes at the operation's end; a failure
// arrived as a notice already (AC-18), other refusals are shown here.
class Operations : public QObject {
    Q_OBJECT
public:
    Operations(QWidget* window, MachineController* machines, QObject* parent = nullptr);
    ~Operations() override;

    using Done = std::function<void(bool ok, const QString& reason)>;
    /// Sends `name`; `done` gets the outcome when it is over. A refusal
    /// (not already a notice) goes to a warning box titled `title`.
    void run(const QString& title, const calcu::sessions::CommandName& name, calcu::sessions::Value args = {},
             Done done = {});
    /// The same, waiting for the end in a nested event loop (the frame timer
    /// goes on calling the session meanwhile; frozen, the controller drives
    /// it). For the debugger's requests, which answer when the load is done
    /// (adapter until M-45), and the --shots runner. A failure goes to
    /// `error`, or a warning box when it is null.
    bool runAndWait(const QString& title, const calcu::sessions::CommandName& name, calcu::sessions::Value args,
                    QString* error = nullptr);

    void loadPreset(const QString& path);
    /// An operation is running (the busy topic).
    bool busy() const;

private:
    void onBusy(bool busy);
    void showPopupIfLong();

    QWidget* m_window;
    MachineController* m_machines;
    QString m_title;
    QTimer* m_popupTimer = nullptr;
    std::unique_ptr<QProgressDialog> m_popup;
};
