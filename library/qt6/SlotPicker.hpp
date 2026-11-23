#pragma once
#include "sessions/Messages.hpp"
#include "sessions/Value.hpp"
#include <QString>
#include <QWidget>
#include <functional>
#include <string>

class QPushButton;

// The module picker of a slot on the Qt6 control bar, the shell's for every
// product: the library's MediaPicker over the slot's entries in the topic
// `media`, drawn anew when the topic changes; disabled, with the reason as
// its tooltip, while the slot cannot be changed (the place entry's `empty`);
// as wide as the chrome's slotPickerEm whatever it shows (0: as its text),
// so no control after it moves when a module comes or goes. Where the
// product wants it, a Name & Save button after it (the library's dialog,
// usable while the slot's module can be named and saved).
//
// Object names: media-picker-<place> (the MediaPicker), media-save-<place>.

namespace calcu::qt6 {

class MediaPicker;
class SessionHost;

struct SlotPickerOptions {
    bool nameAndSave = false;
    QString prompt = QStringLiteral("Instance name:");   // the Name & Save dialog's
};

// The Name & Save dialog over a media place as the topic `media` has it now;
// it sends through the host and waits for the reply (a save with the power
// on is an operation). Nothing when the topic does not list the place.
void nameAndSave(SessionHost&, QWidget* parent, const std::string& place, const QString& prompt);

class SlotPicker : public QWidget {
    Q_OBJECT
public:
    using Send = std::function<void(const sessions::CommandName&, sessions::Value)>;
    // `entry`: the slot's place entry of the control state; `widthEm`: the
    // chrome's slotPickerEm.
    SlotPicker(SessionHost&, const sessions::Value& entry, const SlotPickerOptions&, double widthEm, Send,
               QWidget* parent = nullptr);

    MediaPicker* picker() const { return picker_; }
    void refresh();

private:
    SessionHost& host_;
    std::string place_;
    SlotPickerOptions options_;
    MediaPicker* picker_;
    QPushButton* save_ = nullptr;
};

} // namespace calcu::qt6
