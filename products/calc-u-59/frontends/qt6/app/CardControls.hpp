#pragma once
#include "TIControlRules.hpp"
#include <QWidget>
#include <map>
#include <string>
#include <vector>

class EmulatorController;
class QHBoxLayout;
class QToolButton;

// The magnetic card controls on the shell's bar:
// made by the shell's media factory for the media place `card-reader`
// (registerMediaControls("magnetic-card", …)), so they exist only while the
// control state lists the card reader (the TI-59). It draws the controls
// the control state declares (`c59.card-controls`, app/TIControlRules):
// one button each, disabled with the reason as its tooltip while it cannot
// be used. The two Arm buttons stand for the File menu's card items.
class CardControls : public QWidget {
    Q_OBJECT
public:
    explicit CardControls(EmulatorController& controller, QWidget* parent = nullptr);
    /// The icons alone, or with their labels ("Crd", "Eject Card") beside them.
    void setShowLabels(bool on);   // the bar decides (its BarLabels)
    /// How much wider the visible buttons are with their labels.
    int labelsWidth() const;

signals:
    void armRequested();
    void armFreshRequested();

private:
    void refresh();
    QToolButton* button(const std::string& id);   // the control's button, made the first time it is declared
    void draw(QToolButton*, const cu59::CardControl&) const;
    EmulatorController& m_controller;
    QHBoxLayout* m_row;
    std::vector<cu59::CardControl> m_declared;
    std::map<std::string, QToolButton*> m_buttons;
    bool m_labels = true;
};
