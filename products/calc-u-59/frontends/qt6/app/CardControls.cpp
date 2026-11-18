#include "CardControls.hpp"
#include "ChromeStyle.hpp"
#include "EmulatorController.hpp"
#include <QHBoxLayout>
#include <QToolButton>

CardControls::CardControls(EmulatorController& controller, QWidget* parent) : QWidget(parent), m_controller(controller) {
    setStyleSheet(ChromeStyle::barButton());
    auto* row = new QHBoxLayout(this);
    row->setContentsMargins(0, 0, 0, 0);
    row->setSpacing(12);
    auto button = [this, row](const QString& text, const QString& name) {
        auto* b = new QToolButton(this);
        b->setText(text);
        b->setToolTip(name);
        b->setAccessibleName(name);
        row->addWidget(b);
        return b;
    };
    // Both read "Crd"; the icon and the name tell them apart.
    m_arm = button(QStringLiteral("⤓ Crd"), QStringLiteral("Arm Card"));
    m_fresh = button(QStringLiteral("＋ Crd"), QStringLiteral("Arm Fresh Card"));
    m_eject = button(QStringLiteral("⏏ Eject Card"), QStringLiteral("Eject Card"));
    connect(m_arm, &QToolButton::clicked, this, &CardControls::armRequested);
    connect(m_fresh, &QToolButton::clicked, this, &CardControls::armFreshRequested);
    connect(m_eject, &QToolButton::clicked, this, [this] { m_controller.ejectCard(); });
    connect(&m_controller, &EmulatorController::cardStateChanged, this, &CardControls::refresh);
    refresh();
}

void CardControls::refresh() {
    const bool reader = m_controller.cardReader() != nullptr;
    const bool passing = m_controller.cardPassing();
    m_arm->setVisible(reader && !passing);
    m_fresh->setVisible(reader && !passing);
    m_eject->setVisible(reader && passing);
}
