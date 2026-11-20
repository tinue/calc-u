#include "qt6/RunControlButtons.hpp"
#include <QHBoxLayout>
#include <QTimer>
#include <QToolButton>

namespace calcu::qt6 {

RunControlButtons::RunControlButtons(debugger::LevelName level, Labels labels, QWidget* parent)
    : QWidget(parent), level_(std::move(level)) {
    auto* row = new QHBoxLayout(this);
    row->setContentsMargins(0, 0, 0, 0);
    auto make = [&](const QString& text, const char* name) {
        auto* b = new QToolButton(this);
        b->setFocusPolicy(Qt::NoFocus);
        b->setText(text);
        b->setObjectName(QString::fromLatin1(name));
        row->addWidget(b);
        return b;
    };
    pause_ = make(labels.pause, "run-control-pause");
    step_ = make(labels.step, "run-control-step");
    resume_ = make(labels.resume, "run-control-resume");
    repeat_ = new QTimer(this);
    repeat_->setInterval(static_cast<int>(debugger::StepRepeat::kInterval.count()));

    connect(pause_, &QToolButton::clicked, this, [this] {
        if (use_.pause) send(debugger::kPauseCommand, sessions::Value::object({{"level", level_.value}}));
    });
    connect(resume_, &QToolButton::clicked, this, [this] {
        if (use_.resume) send(debugger::kResumeCommand, {});
    });
    // Step: on press, then every 250 ms while held.
    connect(step_, &QToolButton::pressed, this, [this] {
        repeat_->start();   // first: the button stays usable while the step runs
        stepOnce();
    });
    connect(step_, &QToolButton::released, this, [this] {
        repeat_->stop();
        refresh();
    });
    connect(repeat_, &QTimer::timeout, this, [this] {
        if (!use_.step || !step_->isDown()) {   // until step cannot be used, or released
            repeat_->stop();
            refresh();
            return;
        }
        stepOnce();
    });
    refresh();
}

RunControlButtons::~RunControlButtons() = default;

void RunControlButtons::stepOnce() {
    if (use_.step) send(debugger::kStepCommand, sessions::Value::object({{"level", level_.value}, {"count", 1}}));
}

void RunControlButtons::send(const sessions::CommandName& n, sessions::Value args) {
    if (sender_) sender_(n, std::move(args));
}

void RunControlButtons::deliver(const sessions::Message& m) {
    const auto* s = std::get_if<sessions::StateMessage>(&m);
    if (!s || s->topic != debugger::kRunStateTopic) return;
    setRunState(sessions::payloadAs<debugger::RunState>(s->state));
}

void RunControlButtons::setRunState(const debugger::RunState* s) {
    state_ = s ? std::optional<debugger::RunState>(*s) : std::nullopt;
    refresh();
}

void RunControlButtons::refresh() {
    use_ = debugger::buttonUse(level_, state_ ? &*state_ : nullptr);
    pause_->setEnabled(use_.pause);
    resume_->setEnabled(use_.resume);
    // A held step keeps its button enabled between two steps (while the
    // step runs the run state says running), so that the hold goes on.
    step_->setEnabled(use_.step || (step_->isDown() && repeat_->isActive()));
}

} // namespace calcu::qt6
