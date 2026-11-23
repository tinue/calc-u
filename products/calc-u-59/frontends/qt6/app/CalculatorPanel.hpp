#pragma once
#include <QWidget>

class EmulatorController;
class KeyboardWidget;
namespace calcu::qt6 {
class Shell;
}

// The calculator column: the faceplate above, the shell's control bar below,
// and the shell's "ROMs missing" text in place of
// the faceplate when the product could not start, on the dark panel
// colour (CalculatorView.calculatorBody in the Swift app).
class CalculatorPanel : public QWidget {
    Q_OBJECT
public:
    explicit CalculatorPanel(EmulatorController& controller, QWidget* parent = nullptr);
    KeyboardWidget* keyboard() const { return m_keyboard; }

signals:
    /// The bar's and the paper's text scale changed with the faceplate
    /// (app/'s rule, TIChrome::textScale).
    void fontScaleChanged(double scale);

public:
    /// The shell's bar (fitted below the faceplate) and "ROMs missing" (in place of it).
    void setShellWidgets(calcu::qt6::Shell&);

protected:
    bool eventFilter(QObject*, QEvent*) override;
    // The faceplate no taller than the column's width gives it, the bar
    // right below it (a narrow page leaves its room at the bottom).
    void resizeEvent(QResizeEvent*) override;

private:
    // The bar as wide as the faceplate, wider only where its controls
    // need more; centred under the faceplate.
    void fitBar();

    KeyboardWidget* m_keyboard;
    QWidget* m_bar = nullptr;
    calcu::qt6::Shell* m_shell = nullptr;
    double m_fontScale = 0;
};
