#pragma once
#include <QWidget>

class EmulatorController;
class QToolButton;

// The magnetic card controls on the shell's bar:
// made by the shell's media factory for the media place `card-reader`
// (registerMediaControls("magnetic-card", …)), so they exist only while the
// control state lists the card reader (the TI-59). Two "Crd"
// buttons, Arm Card and Arm Fresh Card, while no card passes; Eject Card
// while one does (the media topic's card reader). The two Crd buttons stand
// for the File menu's card items.
class CardControls : public QWidget {
    Q_OBJECT
public:
    explicit CardControls(EmulatorController& controller, QWidget* parent = nullptr);
    /// The icons alone, or with "Crd" / "Eject Card" beside them.
    void setShowLabels(bool on);   // the bar decides (its BarLabels)
    bool showsLabels() const { return m_labels; }
    /// How much wider the visible buttons are with their labels.
    int labelsWidth() const;

signals:
    void armRequested();
    void armFreshRequested();

private:
    void refresh();
    EmulatorController& m_controller;
    QToolButton* m_arm;
    QToolButton* m_fresh;
    QToolButton* m_eject;
    bool m_labels = true;
};
