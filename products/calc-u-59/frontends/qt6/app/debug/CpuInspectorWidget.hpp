#pragma once
#include <QWidget>

class DebugHeaderRow;
class EmulatorController;
namespace calcu::qt6 {
class RunControlButtons;
}
class QLabel;
class QListWidget;
class QToolButton;
class RomHeatmapWidget;

// CPU tab: instruction history (live: the last 32; frozen: the whole trace
// ring plus the next instruction), the selected instruction's registers,
// control registers and display state, and the ROM heatmap. Up/Down move
// the selection. The counterpart of App/Views/CPUInspectorView.swift.
class CpuInspectorWidget : public QWidget {
    Q_OBJECT
public:
    explicit CpuInspectorWidget(EmulatorController& controller, QWidget* parent = nullptr);

protected:
    void resizeEvent(QResizeEvent* e) override;

private:
    void refreshHeader();
    void refreshList();
    void refreshDetails();
    int baseFontSize() const;

    EmulatorController& m_controller;
    DebugHeaderRow* m_header;   ///< title, RESUME, FREEZE, FREEZE ON START, ARMED, STEP
    QListWidget* m_list;
    QLabel* m_registers;
    QLabel* m_control;
    QLabel* m_displayState;
    QLabel* m_hover;
    RomHeatmapWidget* m_heatmap;
    int m_selected = -1;
};
