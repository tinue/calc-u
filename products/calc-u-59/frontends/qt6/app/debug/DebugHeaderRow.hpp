#pragma once
#include "debugger/Levels.hpp"
#include <QWidget>
#include <map>
#include <string>

class EmulatorController;
class QLabel;
class QToolButton;
namespace calcu::qt6 { class RunControlButtons; }

// The header of a debug tab (CALCULATOR or CPU) as app/'s debugHeader
// describes it for every GUI: the title, then RESUME, FREEZE, FREEZE ON
// START, ARMED and STEP with their labels and colours, at the panel's base
// font size; short forms below debugHeaderNarrow. FREEZE, STEP and RESUME
// are the library's run-control buttons of the tab's level; FREEZE ON START
// and ARMED arm and disarm its pause-on-start.
class DebugHeaderRow : public QWidget {
    Q_OBJECT
public:
    DebugHeaderRow(EmulatorController& controller, bool cpu, QWidget* parent = nullptr);
    /// The title and the labels at this base font size, the buttons' states now.
    void refresh(int baseFontSize);

private:
    EmulatorController& m_controller;
    bool m_cpu;
    calcu::debugger::LevelName m_level;
    QLabel* m_title;
    calcu::qt6::RunControlButtons* m_run;
    std::map<std::string, QToolButton*> m_buttons;   ///< by app/'s button ID
    int m_fontSize = 0;
};
