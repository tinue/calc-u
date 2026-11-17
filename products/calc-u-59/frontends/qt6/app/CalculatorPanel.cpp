#include "CalculatorPanel.hpp"
#include "ChromeStyle.hpp"
#include "KeyboardWidget.hpp"
#include <QVBoxLayout>

CalculatorPanel::CalculatorPanel(EmulatorController& controller, QWidget* parent) : QWidget(parent) {
    setObjectName(QStringLiteral("calculator"));
    setAutoFillBackground(true);
    QPalette pal = palette();
    pal.setColor(QPalette::Window, ChromeStyle::kPanel);
    setPalette(pal);
    auto* col = new QVBoxLayout(this);
    col->setContentsMargins(8, 8, 8, 8);
    col->setSpacing(0);
    m_keyboard = new KeyboardWidget(controller, this);
    col->addWidget(m_keyboard, 1);
}

void CalculatorPanel::setShellWidgets(QWidget* controlBar, QWidget* romsMissing) {
    auto* col = static_cast<QVBoxLayout*>(layout());
    romsMissing->setParent(this);
    col->insertWidget(1, romsMissing, 1);
    controlBar->setParent(this);
    controlBar->setAutoFillBackground(true);
    QPalette pal = controlBar->palette();
    pal.setColor(QPalette::Window, ChromeStyle::kBar);
    controlBar->setPalette(pal);
    controlBar->setStyleSheet(ChromeStyle::barButton());
    col->addWidget(controlBar);
}
