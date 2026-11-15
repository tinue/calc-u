#pragma once
#include "media/MediaTopics.hpp"
#include "sessions/Messages.hpp"
#include "sessions/Value.hpp"
#include <QDialog>
#include <functional>
#include <string>

class QCheckBox;
class QDialogButtonBox;
class QLabel;
class QLineEdit;

// The Name & Save dialog of the Qt6 component set (LIB-7 design §2, §3.6,
// D-6, D-7; spec AC-16, AC-17, AC-19): it asks for the name only, and for the
// write-protect switch where the place's medium has one that is still off
// (PlaceState::askWriteProtect). The product sets its prompt ("Instance
// name:", "Disk name:"; Calc-U-1600 AC-40). On OK it sends
// `media.name-and-save {place, name, protect}`; a refusal that starts with
// "asks: " is a question (D-6): the dialog shows it and, on yes, sends the
// same command again with `confirmed: true`.
//
// Object names: media-name-and-save, media-name, media-protect,
// media-name-prompt.

namespace calcu::qt6 {

class NameAndSaveDialog : public QDialog {
    Q_OBJECT
public:
    // The reply's reason: empty when it was done.
    using Sender = std::function<std::string(const sessions::CommandName&, sessions::Value)>;
    // Asks a yes/no question; true: yes. Default: a QMessageBox.
    using Asker = std::function<bool(const QString&)>;
    static constexpr const char* kProtectText = "Write-protect (switch to read-only, can't be undone)";

    explicit NameAndSaveDialog(QWidget* parent = nullptr);

    void setPlace(const media::PlaceState&);
    void setPrompt(const QString&);
    void setSender(Sender s) { sender_ = std::move(s); }
    void setAsker(Asker a) { asker_ = std::move(a); }

    // What the dialog shows and would send (the tests).
    bool asksWriteProtect() const;
    QString prompt() const;
    sessions::Value args(bool confirmed = false) const;
    // OK: sends, asks when the reply asks, and closes when done; the reason of a refusal otherwise.
    std::string submit();

private:
    std::string place_;
    QLabel* prompt_;
    QLineEdit* name_;
    QCheckBox* protect_;
    QDialogButtonBox* buttons_;
    Sender sender_;
    Asker asker_;
};

} // namespace calcu::qt6
