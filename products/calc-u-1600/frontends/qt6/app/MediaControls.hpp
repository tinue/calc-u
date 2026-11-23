#pragma once
#include "qt6/MediaPlaceControl.hpp"

class MachineController;
class QLabel;
class QPushButton;

// Calc-U-1600's media controls on the shell's bar, each the library's media
// place control (the picker, Name & Save) with what the place has besides:
//
// - DiskControl: "Disk:", the CE-1600F's picker, the side, Name & Save and
//   the drive's lamp; shown while a PC-1600 has the CE-1600P (the product
//   entry `c1600.disk-controls`), a product control at the end of the
//   group Media, after the cassette.
// - TapeControl: "Cassette:", the cassette's picker, Name & Save (the blank
//   tape kept under a name) and the counter (`c1600.lamps`); the shell's
//   media factory of the kind cassette, shown while a peripheral with a
//   cassette is plugged in.
//
// Object names: media-picker-<place>, media-save-<place> (the library's),
// media-side-floppy, media-lamp-floppy, media-counter-tape, bar-disk.

class DiskControl : public calcu::qt6::MediaPlaceControl {
    Q_OBJECT
public:
    DiskControl(MachineController&, calcu::qt6::Shell&, QWidget* parent);
    void refresh() override;

private:
    void refreshLamp();
    MachineController& m_controller;
    QPushButton* m_side = nullptr;
    QLabel* m_lamp = nullptr;
};

class TapeControl : public calcu::qt6::MediaPlaceControl {
    Q_OBJECT
public:
    TapeControl(MachineController&, calcu::qt6::Shell&, QWidget* parent);
    void refresh() override;

private:
    void refreshCounter();
    MachineController& m_controller;
    QLabel* m_counter = nullptr;
};
