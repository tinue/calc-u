#include "qt6/SlotPicker.hpp"
#include "qt6/MediaPicker.hpp"
#include "qt6/Shell.hpp"
#include "shell/Inputs.hpp"
#include <QFontInfo>

namespace calcu::qt6 {

SlotPicker::SlotPicker(Shell& owner, const sessions::Value& entry, double widthEm, QWidget* parent)
    : MediaPlaceControl(owner, shell::read::text(entry, "place"), parent) {
    if (widthEm > 0) picker()->setFixedWidth(qRound(widthEm * QFontInfo(picker()->font()).pixelSize()));
    if (owner.product().nameAndSave.slotButtons) addSaveButton();
    const shell::Availability a = shell::availabilityOf(entry, "empty");
    setEnabled(a.now);
    if (!a.now) setToolTip(QString::fromStdString(a.reason));
    refresh();
}

} // namespace calcu::qt6
