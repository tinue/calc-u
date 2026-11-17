#include "qt6/NameAndSaveDialog.hpp"
#include "qt6/AskedSend.hpp"
#include "media/SaveRules.hpp"
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

namespace calcu::qt6 {

NameAndSaveDialog::NameAndSaveDialog(QWidget* parent)
    : QDialog(parent), prompt_(new QLabel(QStringLiteral("Name:"), this)), name_(new QLineEdit(this)),
      protect_(new QCheckBox(QString::fromUtf8(kProtectText), this)),
      buttons_(new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this)) {
    setObjectName(QStringLiteral("media-name-and-save"));
    setWindowTitle(QStringLiteral("Name & Save"));
    prompt_->setObjectName(QStringLiteral("media-name-prompt"));
    name_->setObjectName(QStringLiteral("media-name"));
    protect_->setObjectName(QStringLiteral("media-protect"));
    auto* layout = new QVBoxLayout(this);
    layout->addWidget(prompt_);
    layout->addWidget(name_);
    layout->addWidget(protect_);
    layout->addWidget(buttons_);
    protect_->setVisible(false);
    connect(buttons_, &QDialogButtonBox::accepted, this, [this] {
        const std::string why = submit();
        if (!why.empty()) QMessageBox::warning(this, windowTitle(), QString::fromStdString(why));
    });
    connect(buttons_, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(name_, &QLineEdit::textChanged, this,
            [this](const QString& t) { buttons_->button(QDialogButtonBox::Ok)->setEnabled(!t.trimmed().isEmpty()); });
    buttons_->button(QDialogButtonBox::Ok)->setEnabled(false);
}

void NameAndSaveDialog::setPlace(const media::PlaceState& p) {
    place_ = p.place;
    protect_->setChecked(false);
    protect_->setVisible(p.askWriteProtect);   // AC-16: only for a switch still off
    name_->clear();
}

void NameAndSaveDialog::setPrompt(const QString& p) { prompt_->setText(p); }

bool NameAndSaveDialog::asksWriteProtect() const { return protect_->isVisibleTo(const_cast<NameAndSaveDialog*>(this)); }

QString NameAndSaveDialog::prompt() const { return prompt_->text(); }

sessions::Value NameAndSaveDialog::args(bool confirmed) const {
    sessions::Value::Object o{{"place", place_}, {"name", name_->text().toStdString()}};
    if (asksWriteProtect() && protect_->isChecked()) o.push_back({"protect", true});
    if (confirmed) o.push_back({"confirmed", true});
    return sessions::Value(std::move(o));
}

std::string NameAndSaveDialog::submit() {
    if (!sender_) return "Nothing receives the save.";
    // D-6: the question, then the same command confirmed (AskedSend).
    const AskedSend r = sendAsking(
        [this](bool confirmed) { return sender_(media::kNameAndSaveCommand, args(confirmed)); },
        [this](const QString& question) {
            return asker_ ? asker_(question) : QMessageBox::question(this, windowTitle(), question) == QMessageBox::Yes;
        });
    if (r.done) accept();
    return r.refusal;
}

} // namespace calcu::qt6
