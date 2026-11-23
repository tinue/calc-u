#pragma once
#include "media/MediaTopics.hpp"
#include <QWidget>
#include <string>

class QHBoxLayout;
class QPushButton;

// The control of one media place on the Qt6 control bar, the shell's for
// every product: a row with the library's MediaPicker over the place's
// entries in the topic `media`, drawn anew when the topic changes, and
// where the control wants it a Name & Save button, usable while the
// place's medium can be named and saved, which opens the shell's dialog
// (Shell::nameAndSave). A choice goes through Shell::sendChecked: a
// refusal is shown as a notice or asked as a question. The module slots'
// picker (SlotPicker) and a product's own media controls build on it and
// add their controls to the row.
//
// Object names: media-picker-<place> (the MediaPicker), media-save-<place>.

namespace calcu::qt6 {

class MediaPicker;
class Shell;

class MediaPlaceControl : public QWidget {
    Q_OBJECT
public:
    MediaPlaceControl(Shell&, std::string place, QWidget* parent = nullptr);

    MediaPicker* picker() const { return picker_; }
    const std::string& place() const { return place_; }
    // The place as the topic `media` has it now; nullptr while it lists none.
    const media::PlaceState* placeState() const;
    // Draws the place anew; the control calls it once it is built.
    virtual void refresh();

protected:
    QHBoxLayout* row() const { return row_; }   // the picker, then what the control adds
    void addSaveButton();                       // Name & Save, at the row's end
    Shell& shell_;

private:
    std::string place_;
    QHBoxLayout* row_;
    MediaPicker* picker_;
    QPushButton* save_ = nullptr;
};

} // namespace calcu::qt6
