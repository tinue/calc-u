#pragma once
#include <QObject>
#include <functional>

class MachineController;
class QTimer;

// The 60 Hz frame timer of the Qt6 frontend (LIB-17 design §2): each tick is
// one host call of the app session (MachineController::call(); the
// library's pacer turns it into emulated time -- exactly the wall time that
// passed, or, while the LCD is held, full speed for the call's allowance),
// then the window refreshes its views. The --shots runner freezes it; then
// emulated time moves only through runFor() / runUntilPasteDone(), on the
// controller's frozen clock.
class EmulationPacer : public QObject {
    Q_OBJECT
public:
    EmulationPacer(MachineController* controller, std::function<void()> refreshViews, QObject* parent = nullptr);

    /// Stop / start the frame timer (unless frozen).
    void suspend();
    void resume();

    /// The display held: full speed (LIB-3 AC-21, the speed command).
    void setTurbo(bool active);

    /// Frozen: no frame timer at all (see MachineController::setFrozen()).
    void setFrozen(bool frozen);
    /// Runs `seconds` of host calls at 60 Hz, then refreshes the views.
    void runFor(double seconds);
    /// Runs until a MachineController::pasteText() has been fully typed;
    /// false if it is still typing after `capSeconds`.
    bool runUntilPasteDone(double capSeconds);

private:
    void onTick();

    MachineController* m_controller; // not owned
    std::function<void()> m_refreshViews;
    QTimer* m_timer = nullptr;
    bool m_frozen = false;
};
