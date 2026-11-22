#include "CalculatorPanel.hpp"
#include "CardControls.hpp"
#include "ChromeStyle.hpp"
#include "KeyboardWidget.hpp"
#include "shell/BarModel.hpp"
#include <QEvent>
#include <QHBoxLayout>
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

void CalculatorPanel::setShellWidgets(QWidget* controlBar, QWidget* romsMissing) {
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
    // Expanding up to its maximum (fitBar() sets it), centred between two
    // stretches that take only what is left: a widget laid out with an
    // alignment would keep its size hint instead.
    controlBar->setSizePolicy(QSizePolicy::Expanding, controlBar->sizePolicy().verticalPolicy());
    auto* row = new QHBoxLayout;
    row->setContentsMargins(0, 0, 0, 0);
    row->setSpacing(0);
    row->addStretch(1);
    row->addWidget(controlBar, 1000);
    row->addStretch(1);
    col->addLayout(row);
    m_bar = controlBar;
    m_keyboard->installEventFilter(this);   // the faceplate's size
    controlBar->installEventFilter(this);   // its controls came or went
    fitBar();
}

bool CalculatorPanel::eventFilter(QObject* o, QEvent* e) {
    if ((o == m_keyboard && e->type() == QEvent::Resize) || (o == m_bar && e->type() == QEvent::LayoutRequest)) fitBar();
    return QWidget::eventFilter(o, e);
}

void CalculatorPanel::fitBar() {
    if (!m_bar) return;
    const int face = static_cast<int>(m_keyboard->canvasRect().width());
    if (const double scale = ChromeStyle::fontScale(face); std::abs(scale - m_fontScale) > 0.02) {
        m_fontScale = scale;
        m_bar->setStyleSheet(ChromeStyle::barButton(scale));
        emit fontScaleChanged(scale);
    }
    // The bar with and without the card buttons' labels, from the text
    // widths (switching the labels then asks for no second round); the
    // library's rule decides (calcu::shell::barFit).
    int without = m_bar->sizeHint().width(), extra = 0;
    auto* cards = m_bar->findChild<CardControls*>();
    if (cards) {
        extra = cards->labelsWidth();
        if (cards->showsLabels()) without -= extra;
    }
    const auto fit = calcu::shell::barFit(face, without, without + extra);
    if (cards && cards->showsLabels() != fit.labels) cards->setShowLabels(fit.labels);
    const int width = static_cast<int>(fit.width);
    if (m_bar->maximumWidth() != width) m_bar->setMaximumWidth(width);
}
