#include "CalculatorPanel.hpp"
#include "ChromeStyle.hpp"
#include "KeyboardWidget.hpp"
#include "qt6/Shell.hpp"
#include <QEvent>
#include <QResizeEvent>
#include <QVBoxLayout>
#include <algorithm>
#include <cmath>

CalculatorPanel::CalculatorPanel(EmulatorController& controller, QWidget* parent) : QWidget(parent) {
    setObjectName(QStringLiteral("calculator"));
    setAutoFillBackground(true);
    QPalette pal = palette();
    pal.setColor(QPalette::Window, ChromeStyle::qcolour(ChromeStyle::colours().window));
    setPalette(pal);
    const int inset = qRound(ChromeStyle::chrome().sizes.panelInset);
    auto* col = new QVBoxLayout(this);
    col->setContentsMargins(inset, inset, inset, inset);
    col->setSpacing(0);
    m_keyboard = new KeyboardWidget(controller, this);
    col->addWidget(m_keyboard, 1);
}

void CalculatorPanel::setShellWidgets(calcu::qt6::Shell& shell) {
    m_shell = &shell;
    QWidget* controlBar = shell.controlBar();
    QWidget* romsMissing = shell.romsMissingView();
    auto* col = static_cast<QVBoxLayout*>(layout());
    romsMissing->setParent(this);
    col->insertWidget(1, romsMissing, 1);
    controlBar->setParent(this);
    controlBar->setAutoFillBackground(true);
    QPalette pal = controlBar->palette();
    pal.setColor(QPalette::Window, ChromeStyle::qcolour(ChromeStyle::colours().buttonBar));
    controlBar->setPalette(pal);
    controlBar->setStyleSheet(ChromeStyle::barButton());
    // As high as the panels' button bars beside it (TIChrome).
    controlBar->setFixedHeight(qRound(ChromeStyle::chrome().sizes.barHeight));
    col->addLayout(shell.centredBarRow());
    col->addStretch(0);
    m_bar = controlBar;
    m_keyboard->installEventFilter(this);   // the faceplate's size
    fitBar();
}

void CalculatorPanel::resizeEvent(QResizeEvent* e) {
    const double inset = ChromeStyle::chrome().sizes.panelInset;
    const int tallest = static_cast<int>(std::ceil((width() - 2 * inset) / cu59::kCanvasAspect));
    if (m_keyboard->maximumHeight() != tallest) m_keyboard->setMaximumHeight(std::max(1, tallest));
    QWidget::resizeEvent(e);
}

bool CalculatorPanel::eventFilter(QObject* o, QEvent* e) {
    if (o == m_keyboard && e->type() == QEvent::Resize) fitBar();
    return QWidget::eventFilter(o, e);
}

void CalculatorPanel::fitBar() {
    if (!m_bar || !m_shell) return;
    const int face = static_cast<int>(m_keyboard->canvasRect().width());
    if (const double scale = ChromeStyle::fontScale(face); std::abs(scale - m_fontScale) > 0.02) {
        m_fontScale = scale;
        m_bar->setStyleSheet(ChromeStyle::barButton(scale));
        emit fontScaleChanged(scale);
    }
    // Its labels ("Crd", "More") and its width by the library's rule.
    m_shell->fitBar(face);
}
