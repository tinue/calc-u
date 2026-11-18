#pragma once
#include "debugger/ButtonRule.hpp"
#include "debugger/RunState.hpp"
#include "sessions/Messages.hpp"
#include <QString>
#include <QWidget>
#include <functional>
#include <memory>
#include <optional>

class QTimer;
class QToolButton;

// The run-control buttons of the Qt6 component set: pause, step and resume
// for one level, fed from the session's `run-state` topic (deliver() or
// setRunState()) and sending `pause {level}`, `step {level, count: 1}` and
// `resume` through the sender. Which button can be used is
// debugger::buttonUse(); step fires on press and every 250 ms while held,
// until release or until step cannot be used. The labels and the place are
// the product's.
//
// Object names: run-control-pause, run-control-step, run-control-resume.

namespace calcu::qt6 {

struct RunControlLabels {
    QString pause = QStringLiteral("Pause");
    QString step = QStringLiteral("Step");
    QString resume = QStringLiteral("Resume");
};

class RunControlButtons : public QWidget {
    Q_OBJECT
public:
    using Sender = std::function<void(const sessions::CommandName&, sessions::Value)>;
    using Labels = RunControlLabels;

    explicit RunControlButtons(debugger::LevelName level, Labels labels = {}, QWidget* parent = nullptr);
    ~RunControlButtons() override;

    void deliver(const sessions::Message&);       // the `run-state` topic's state messages
    void setRunState(const debugger::RunState*);  // nullptr: none yet
    void setSender(Sender s) { sender_ = std::move(s); }

    debugger::ButtonUse use() const { return use_; }
    QToolButton* pauseButton() const { return pause_; }
    QToolButton* stepButton() const { return step_; }
    QToolButton* resumeButton() const { return resume_; }

private:
    void refresh();
    void send(const sessions::CommandName&, sessions::Value);
    void stepOnce();

    debugger::LevelName level_;
    std::optional<debugger::RunState> state_;
    debugger::ButtonUse use_;
    Sender sender_;
    QToolButton* pause_ = nullptr;
    QToolButton* step_ = nullptr;
    QToolButton* resume_ = nullptr;
    QTimer* repeat_ = nullptr;
};

} // namespace calcu::qt6
