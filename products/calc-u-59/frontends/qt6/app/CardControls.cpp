#include "CardControls.hpp"
#include "ChromeStyle.hpp"
#include "EmulatorController.hpp"
#include "qt6/ShellControlBar.hpp"
#include <QHBoxLayout>
#include <QToolButton>

CardControls::CardControls(EmulatorController& controller, QWidget* parent) : QWidget(parent), m_controller(controller) {
    // The bar's style sheet (CalculatorPanel's, at the faceplate's scale) applies.
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
    m_arm = button(QString(), QStringLiteral("Arm Card"));
    m_fresh = button(QString(), QStringLiteral("Arm Fresh Card"));
    m_eject = button(QString(), QStringLiteral("Eject Card"));
    setShowLabels(true);
    // "Crd" and "Eject Card" are the bar's labels: shown where the bar fits
    // the faceplate with them (the shell's fitting, shell::barFit).
    new calcu::qt6::BarLabels(this, [this] { return labelsWidth(); }, [this](bool on) { setShowLabels(on); });
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

namespace {
const QString kArmIcon = QStringLiteral("⤓"), kFreshIcon = QStringLiteral("＋"), kEjectIcon = QStringLiteral("⏏");
const QString kCrd = QStringLiteral(" Crd"), kEject = QStringLiteral(" Eject Card");
} // namespace

void CardControls::setShowLabels(bool on) {
    m_arm->setText(kArmIcon + (on ? kCrd : QString()));
    m_fresh->setText(kFreshIcon + (on ? kCrd : QString()));
    m_eject->setText(kEjectIcon + (on ? kEject : QString()));
}

int CardControls::labelsWidth() const {
    const QFontMetrics fm = m_arm->fontMetrics();
    int w = 0;
    if (m_arm->isVisibleTo(this)) w += fm.horizontalAdvance(kCrd);
    if (m_fresh->isVisibleTo(this)) w += fm.horizontalAdvance(kCrd);
    if (m_eject->isVisibleTo(this)) w += fm.horizontalAdvance(kEject);
    return w;
}
