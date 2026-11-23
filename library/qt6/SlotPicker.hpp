#pragma once
#include "qt6/MediaPlaceControl.hpp"
#include "sessions/Value.hpp"

// The module picker of a slot on the Qt6 control bar, the shell's for every
// product: the media place control of the slot, disabled, with the reason
// as its tooltip, while the slot cannot be changed (the place entry's
// `empty`); as wide as the chrome's slotPickerEm whatever it shows (0: as
// its text), so no control after it moves when a module comes or goes.
// Where the product wants it (ShellProduct::nameAndSave.slotButtons), the
// Name & Save button after it.

namespace calcu::qt6 {

class SlotPicker : public MediaPlaceControl {
    Q_OBJECT
public:
    // `entry`: the slot's place entry of the control state; `widthEm`: the
    // chrome's slotPickerEm.
    SlotPicker(Shell&, const sessions::Value& entry, double widthEm, QWidget* parent = nullptr);
};

} // namespace calcu::qt6
