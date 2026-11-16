#pragma once
#include <QDialog>

#include "app/SharpTopics.hpp"
#include "sessions/Messages.hpp"
#include <functional>

class QDialogButtonBox;
class QLabel;
class QLineEdit;

// "Load Machine Code…" question dialog for a file without a header (C1600-2
// AC-18): asks the start address. What it says and whether an address holds
// come from the session's `c1600.plan-code` (app/SharpLoaderPart; MODE /
// TITLE decide the address space and the target on a PC-1600), asked again
// with each change of the text; Load is enabled only while the plan holds.
// `args` are the file's ({bytes, file?}); `plan` is the first plan, without
// an address. MainWindow skips the dialog when the file has a header.
class MachineCodeLoadDialog : public QDialog {
    Q_OBJECT
public:
    using Ask = std::function<calcu::sessions::ReplyMessage(const calcu::sessions::RequestName&, calcu::sessions::Value)>;
    MachineCodeLoadDialog(QWidget* parent, Ask ask, calcu::sessions::Value args,
                          const shared::CodePlanReply& plan);

    // The address as the user typed it.
    QString address() const;

private:
    void refresh();

    Ask m_ask;
    calcu::sessions::Value m_args;
    QLineEdit* m_addressField = nullptr;
    QLabel* m_hint = nullptr;
    QDialogButtonBox* m_buttons = nullptr;
};
