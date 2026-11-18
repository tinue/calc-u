#pragma once
#include "shell/Notices.hpp"
#include <QMessageBox>
#include <QObject>
#include <QPointer>
#include <functional>

class QWidget;

// The notice dialog of the Qt6 shell: a notice as it is, without a title, with OK; a question
// (`asks:`) with Yes and No. Window-modal through QMessageBox::open(), with
// no nested event loop, so the session goes on being called behind it.
// Object names `notice`, `notice-ok`, `notice-yes`, `notice-no`.

namespace calcu::qt6 {

class ShellNotices : public QObject {
    Q_OBJECT
public:
    explicit ShellNotices(QWidget* window);
    bool showing() const { return box_ != nullptr; }
    // Shows `shown`; `done(yes)` when it is closed (OK counts as yes).
    void show(const shell::NoticeQueue::Shown& shown, std::function<void(bool yes)> done);
    QMessageBox* box() const { return box_; }

private:
    QWidget* window_;
    QPointer<QMessageBox> box_;
};

} // namespace calcu::qt6
