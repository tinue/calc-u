#include "PaperPane.hpp"
#include "ChromeStyle.hpp"
#include "EmulatorController.hpp"
#include "qt6/PaperPanel.hpp"
#include "PC100C.hpp"
#include "TICatalogue.hpp"
#include <QHBoxLayout>
#include <QPushButton>
#include <QVBoxLayout>

PaperPane::PaperPane(EmulatorController& controller, QWidget* parent) : QWidget(parent), m_controller(controller) {
    setObjectName(QStringLiteral("paper"));
    setAutoFillBackground(true);
    QPalette pal = palette();
    pal.setColor(QPalette::Window, ChromeStyle::kPanel);
    setPalette(pal);
    setMaximumWidth(320);

    auto* col = new QVBoxLayout(this);
    col->setContentsMargins(8, 8, 8, 8);
    col->setSpacing(6);

    // P-E1: the PC-100C attached or detached (a power cycle follows, row 59-11).
    auto* header = new QWidget(this);
    auto* hrow = new QHBoxLayout(header);
    hrow->setContentsMargins(0, 0, 0, 0);
    m_connect = new QPushButton(header);
    m_connect->setObjectName(QStringLiteral("paper-connect"));
    m_connect->setStyleSheet(ChromeStyle::pillButton());
    m_connect->setCursor(Qt::PointingHandCursor);
    m_connect->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    connect(m_connect, &QPushButton::clicked, this, [this] { m_controller.togglePrinterConnection(); });
    hrow->addStretch(1);
    hrow->addWidget(m_connect);
    hrow->addStretch(1);
    col->addWidget(header);

    m_panel = new calcu::qt6::PaperPanel(this);
    m_panel->setSender([this](const calcu::sessions::CommandName& n, calcu::sessions::Value v) {
        m_controller.sendCommand(n, std::move(v));
    });
    col->addWidget(m_panel, 1);

    m_controller.addMessageHandler([this](const calcu::sessions::Message& m) { m_panel->deliver(m); });
    connect(&m_controller, &EmulatorController::printerChanged, this, &PaperPane::refresh);
    refresh();
}

void PaperPane::refresh() {
    const bool on = m_controller.printerConnected();
    m_connect->setText(QStringLiteral("PC-100C ") + (on ? QStringLiteral("●") : QStringLiteral("○")));
    if (!on) {   // LIB-8 AC-29: an empty paper without a name
        m_panel->setOwner(std::nullopt);
        m_panel->setControls({});
        return;
    }
    using calcu::machine::ControlForm;
    m_panel->setOwner(kPC100C);
    m_panel->setControls({{PC100C::kPrint, ControlForm::Key, false},
                          {PC100C::kAdv, ControlForm::Key, false},
                          {PC100C::kTrace, ControlForm::Switch, m_controller.printerTrace()}});
}
