#pragma once
#include <QWidget>

class EmulatorController;
class QPushButton;
namespace calcu::qt6 {
class PaperPanel;
}

// The splitter's second pane (LIB-8 design §11.3, M-53): the library's paper
// panel (calcu::qt6::PaperPanel) with the PC-100C's connect toggle above it
// (adapter P-E1, until the bar's printer control of LIB-14 59 in M-57). The
// panel's owner is the PC-100C while `c59.printer` says it is attached, none
// otherwise; its controls are the PC-100C's PRINT and ADV (keys) and TRACE
// (a switch, its position from `c59.printer`). Copy, Cut and the controls
// are the panel's commands (paper.copy, paper.cut, set-control). Object name
// `paper`; the toggle's `paper-connect`. Replaces PrinterWidget.
class PaperPane : public QWidget {
    Q_OBJECT
public:
    explicit PaperPane(EmulatorController& controller, QWidget* parent = nullptr);
    calcu::qt6::PaperPanel* panel() const { return m_panel; }

private:
    void refresh();

    EmulatorController& m_controller;
    QPushButton* m_connect;
    calcu::qt6::PaperPanel* m_panel;
};
