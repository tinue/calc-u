#include "qt6/ShellNotices.hpp"
#include "shell/ObjectNames.hpp"
#include <QAbstractButton>
#include <QMessageBox>
#include <QPushButton>

namespace calcu::qt6 {

ShellNotices::ShellNotices(QWidget* window) : QObject(window), window_(window) {}

void ShellNotices::show(const shell::NoticeQueue::Shown& shown, std::function<void(bool)> done) {
    auto* box = new QMessageBox(window_);
    box->setObjectName(shell::names::kNotice);
    box->setWindowTitle(QString());   // a notice is a whole sentence: no title
    box->setText(QString::fromStdString(shown.text));
    box->setIcon(shown.question ? QMessageBox::Question : QMessageBox::NoIcon);
    box->setWindowModality(Qt::WindowModal);
    box->setAttribute(Qt::WA_DeleteOnClose);
    if (shown.question) {
        box->addButton(QMessageBox::Yes)->setObjectName(shell::names::kNoticeYes);
        box->addButton(QMessageBox::No)->setObjectName(shell::names::kNoticeNo);
        box->setDefaultButton(QMessageBox::No);
    } else {
        box->addButton(QMessageBox::Ok)->setObjectName(shell::names::kNoticeOk);
    }
    connect(box, &QMessageBox::finished, this, [box, question = shown.question, done = std::move(done)](int) {
        QAbstractButton* clicked = box->clickedButton();   // none: closed otherwise, which is No for a question
        const bool yes = clicked ? clicked->objectName() != QLatin1String(shell::names::kNoticeNo) : !question;
        if (done) done(yes);
    });
    box_ = box;
    box->open();   // no nested loop
}

} // namespace calcu::qt6
