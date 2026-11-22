#include "DebugPanel.hpp"
#include "AsmOverlayBox.hpp"
#include "ChromeStyle.hpp"
#include "CpuInspectorWidget.hpp"
#include "EmulatorController.hpp"
#include "LiveDebugWidget.hpp"
#include "LogWidget.hpp"
#include <QHBoxLayout>
#include <QStackedWidget>
#include <QToolButton>
#include <QVBoxLayout>

DebugPanel::DebugPanel(EmulatorController& controller, QWidget* parent) : QWidget(parent), m_controller(controller) {
    setAutoFillBackground(true);
    QPalette pal = palette();
    pal.setColor(QPalette::Window, ChromeStyle::named(cu59::chrome::kDebugBackground));
    setPalette(pal);
    setMinimumWidth(220);

    // Inset as the printer beside it, the tab row as high as its header:
    // the panels' headers and button bars line up (TIChrome).
    const calcu::shell::ChromeSizes& sizes = ChromeStyle::chrome().sizes;
    const int inset = qRound(sizes.panelInset);
    auto* col = new QVBoxLayout(this);
    col->setContentsMargins(0, inset, 0, inset);
    col->setSpacing(0);

    auto* tabBar = new QWidget(this);
    tabBar->setFixedHeight(qRound(sizes.panelHeaderHeight));
    tabBar->setAutoFillBackground(true);
    QPalette tp = tabBar->palette();
    tp.setColor(QPalette::Window, ChromeStyle::named(cu59::chrome::kDebugHeader));
    tabBar->setPalette(tp);
    auto* trow = new QHBoxLayout(tabBar);
    trow->setContentsMargins(0, 0, 0, 0);
    trow->setSpacing(0);
    const char* names[3] = {"CALCULATOR", "CPU", "LOG"};
    for (int i = 0; i < 3; ++i) {
        m_tabs[i] = new QToolButton(tabBar);
        m_tabs[i]->setText(QLatin1String(names[i]));
        connect(m_tabs[i], &QToolButton::clicked, this, [this, i] { select(i); });
        trow->addWidget(m_tabs[i]);
    }
    trow->addStretch(1);
    col->addWidget(tabBar);

    m_stack = new QStackedWidget(this);
    m_stack->addWidget(new LiveDebugWidget(controller, m_stack));
    auto* cpuPage = new QWidget(m_stack);
    auto* cpuCol = new QVBoxLayout(cpuPage);
    cpuCol->setContentsMargins(0, 0, 0, 0);
    cpuCol->setSpacing(0);
    cpuCol->addWidget(new CpuInspectorWidget(controller, cpuPage), 1);
    auto* asmBox = new AsmOverlayBox(controller, cpuPage);
    connect(asmBox, &AsmOverlayBox::selectFileRequested, this, &DebugPanel::selectAsmFileRequested);
    cpuCol->addWidget(asmBox);
    m_stack->addWidget(cpuPage);
    m_stack->addWidget(new LogWidget(controller, m_stack));
    col->addWidget(m_stack, 1);

    // A running ASM overlay belongs to the CPU tab: the product
    // entry c59.calculator-tab says no meanwhile; Run asks for the CPU tab
    // through c59.debug-view.
    auto calculatorTab = [this] {
        const bool can = m_controller.entryAvailable(cu59::entries::kCalculatorTab);
        m_tabs[0]->setEnabled(can);
        m_tabs[0]->setToolTip(m_controller.entryReason(cu59::entries::kCalculatorTab));
        if (!can && m_tab == 0) select(1);
    };
    connect(&m_controller, &EmulatorController::entriesChanged, this, calculatorTab);
    connect(&m_controller, &EmulatorController::asmChanged, this, calculatorTab);
    connect(&m_controller, &EmulatorController::debugPanelRequested, this, [this](int tab) { select(tab); });
    select(0);
}

void DebugPanel::select(int tab) {
    m_tab = tab;
    m_stack->setCurrentIndex(tab);
    for (int i = 0; i < 3; ++i) {
        bool on = i == tab;
        m_tabs[i]->setStyleSheet(QStringLiteral("QToolButton { color: %1; background: %2; border: none;"
                                                " font-weight: bold; font-size: 10px; padding: 0 12px; }"
                                                "QToolButton:disabled { color: %3; }")
                                     .arg(ChromeStyle::rgb(ChromeStyle::text(on ? 1 : 0.4)),
                                          on ? ChromeStyle::rgb(cu59::chrome::kTabSelected) : QStringLiteral("transparent"),
                                          ChromeStyle::rgb(ChromeStyle::text(0.14))));
    }
    notifyVisibility();
}

void DebugPanel::notifyVisibility() {
    // Snapshots are built (and the core traces) only for a visible tab.
    bool visible = isVisible();
    m_controller.setLiveDebugEnabled(visible && m_tab == 0);
    m_controller.setCpuDebugEnabled(visible && m_tab == 1);
}

void DebugPanel::showEvent(QShowEvent* e) {
    QWidget::showEvent(e);
    notifyVisibility();
}

void DebugPanel::hideEvent(QHideEvent* e) {
    QWidget::hideEvent(e);
    notifyVisibility();
}
