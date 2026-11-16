#pragma once
#include <QString>
#include <QWidget>
#include <string>

class MachineController;
class QLabel;
class QPushButton;
namespace calcu::qt6 {
class MediaPicker;
}

// Calc-U-1600's media controls on the shell's bar (LIB-14 Part D, D.3; LIB-7
// AC-38 to AC-42): each draws one place of LIB-7's topic `media` with the
// library's MediaPicker and refreshes itself when the topic changes.
//
// - SlotControl: a module slot's picker and Name & Save (the shell's slot
//   factory, Shell::registerSlotPicker).
// - DiskControl: the CE-1600F's picker, the side and Name & Save, with the
//   drive's lamp; on every PC-1600's bar, usable while the CE-1600P is
//   plugged in (the product entry `c1600.disk-controls`, D-29), so a
//   product control of the group Media, not the shell's media control.
// - TapeControl: the cassette's picker, Save (a new tape to record onto)
//   and the counter (`c1600.lamps`); the shell's media factory of the kind
//   cassette, shown while a peripheral with a cassette is plugged in.
//
// Object names: media-picker-<place> (MediaPicker), media-save-<place>,
// media-side-floppy, media-lamp-floppy, media-counter-tape, bar-disk.

class MediaPlaceControl : public QWidget {
    Q_OBJECT
public:
    MediaPlaceControl(MachineController&, std::string place, QWidget* parent);

public:   // refresh(): also after the shell put it on the bar
    virtual void refresh();
    virtual void refreshLamps() {}   // c1600.lamps: every host call, only the lamp and the counter
    void nameAndSave(const QString& prompt);
    MachineController& m_controller;
    std::string m_place;
    calcu::qt6::MediaPicker* m_picker = nullptr;
    QPushButton* m_save = nullptr;
};

class SlotControl : public MediaPlaceControl {
    Q_OBJECT
public:
    SlotControl(MachineController&, std::string place, QWidget* parent);
};

class DiskControl : public MediaPlaceControl {
    Q_OBJECT
public:
    DiskControl(MachineController&, QWidget* parent);

public:
    void refresh() override;
    void refreshLamps() override;

private:
    QLabel* m_label = nullptr;
    QPushButton* m_side = nullptr;
    QLabel* m_lamp = nullptr;
};

class TapeControl : public MediaPlaceControl {
    Q_OBJECT
public:
    TapeControl(MachineController&, QWidget* parent);

public:
    void refresh() override;
    void refreshLamps() override;

private:
    void saveTape();
    QLabel* m_counter = nullptr;
};
