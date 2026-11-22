#pragma once
#include <QWidget>

class EmulatorController;
class KeyboardWidget;

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
    /// (ChromeStyle::fontScale).
    void fontScaleChanged(double scale);

public:
    /// The shell's widgets, placed below and in place of the faceplate.
    void setShellWidgets(QWidget* controlBar, QWidget* romsMissing);

protected:
    bool eventFilter(QObject*, QEvent*) override;

private:
    // The bar as wide as the faceplate, wider only where its controls
    // need more; centred under the faceplate.
    void fitBar();

    KeyboardWidget* m_keyboard;
    QWidget* m_bar = nullptr;
    double m_fontScale = 0;
};
