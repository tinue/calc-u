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

// Calc-U-1600's media controls on the shell's bar: each draws one place of
// the library's topic `media` with the
// library's MediaPicker and refreshes itself when the topic changes.
//
// - DiskControl: the CE-1600F's picker, the side and Name & Save, with the
//   drive's lamp; shown while a PC-1600 has the CE-1600P (the product
//   entry `c1600.disk-controls`), a product control at the end of the
//   group Media, after the cassette.
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

class DiskControl : public MediaPlaceControl {
    Q_OBJECT
public:
    DiskControl(MachineController&, QWidget* parent);

public:
    void refresh() override;
    void refreshLamps() override;

private:
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
    QLabel* m_counter = nullptr;
};
