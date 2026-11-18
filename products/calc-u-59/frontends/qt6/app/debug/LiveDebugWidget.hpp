#pragma once
#include <QWidget>

class EmulatorController;
namespace calcu::qt6 {
class RunControlButtons;
}
class QLabel;
class QListWidget;
class QToolButton;

// CALCULATOR tab: FA/FB/IO/KEY, partition, program source, the program steps
// (±5 while running, the full listing when frozen), FIX/angle/Eng/2nd/INV,
// return stack, user flags, HIR, T and the non-zero registers — all from
// app/DebugSnapshots. The counterpart of App/Views/LiveDebugView.swift.
class LiveDebugWidget : public QWidget {
    Q_OBJECT
public:
    explicit LiveDebugWidget(EmulatorController& controller, QWidget* parent = nullptr);

protected:
    void resizeEvent(QResizeEvent* e) override;

private:
    void refresh();
    void refreshHeader();
    int baseFontSize() const;

    EmulatorController& m_controller;
    QLabel* m_title;
    calcu::qt6::RunControlButtons* m_run;   ///< FREEZE, STEP, RESUME
    QToolButton* m_freezeOnStart;
    QToolButton* m_armed;
    QLabel* m_debug;
    QLabel* m_partition;
    QLabel* m_source;
    QListWidget* m_steps;
    QLabel* m_fields;
    QLabel* m_returns;
    QLabel* m_flags;
    QLabel* m_hir;
    QLabel* m_xt;
    QLabel* m_vars;
};
