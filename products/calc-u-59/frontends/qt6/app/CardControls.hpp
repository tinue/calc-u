#pragma once
#include <QWidget>

class EmulatorController;
class QToolButton;

// C59-7's card controls on the shell's bar (AC-13; LIB-14 AC-8, AC-27, M-57):
// made by the shell's media factory for the media place `card-reader`
// (registerMediaControls("magnetic-card", …)), so they exist only while the
// control state lists the card reader (the TI-59; LIB-4 AC-12). Two "Crd"
// buttons, Arm Card and Arm Fresh Card, while no card passes; Eject Card
// while one does (the media topic's card reader). The two Crd buttons stand
// for the File menu's card items (`onBar`, LIB-14 M14).
class CardControls : public QWidget {
    Q_OBJECT
public:
    explicit CardControls(EmulatorController& controller, QWidget* parent = nullptr);

signals:
    void armRequested();
    void armFreshRequested();

private:
    void refresh();
    EmulatorController& m_controller;
    QToolButton* m_arm;
    QToolButton* m_fresh;
    QToolButton* m_eject;
};
