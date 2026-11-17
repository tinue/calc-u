#pragma once
#include <QWidget>

class EmulatorController;
class KeyboardWidget;

// The calculator column: the faceplate above, the shell's control bar below
// (LIB-14 AC-8, AC-27; M-57), and the shell's "ROMs missing" text in place of
// the faceplate when the product could not start (AC-19), on the dark panel
// colour (CalculatorView.calculatorBody in the Swift app).
class CalculatorPanel : public QWidget {
    Q_OBJECT
public:
    explicit CalculatorPanel(EmulatorController& controller, QWidget* parent = nullptr);
    KeyboardWidget* keyboard() const { return m_keyboard; }
    /// The shell's widgets, placed below and in place of the faceplate.
    void setShellWidgets(QWidget* controlBar, QWidget* romsMissing);

private:
    KeyboardWidget* m_keyboard;
};
