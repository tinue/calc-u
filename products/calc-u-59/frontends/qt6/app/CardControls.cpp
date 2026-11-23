#include "CardControls.hpp"
#include "EmulatorController.hpp"
#include "TIShellProduct.hpp"
#include "qt6/ShellControlBar.hpp"
#include <QHBoxLayout>
#include <QToolButton>

CardControls::CardControls(EmulatorController& controller, QWidget* parent) : QWidget(parent), m_controller(controller) {
    // The bar's style sheet (CalculatorPanel's, at the faceplate's scale) applies.
    m_row = new QHBoxLayout(this);
    m_row->setContentsMargins(0, 0, 0, 0);
    m_row->setSpacing(12);
    // The labels are the bar's: shown where the bar fits the faceplate with
    // them (the shell's fitting, shell::barFit).
    new calcu::qt6::BarLabels(this, [this] { return labelsWidth(); }, [this](bool on) { setShowLabels(on); });
    connect(&m_controller, &EmulatorController::entriesChanged, this, &CardControls::refresh);
    refresh();
}

QToolButton* CardControls::button(const std::string& id) {
    if (auto it = m_buttons.find(id); it != m_buttons.end()) return it->second;
    auto* b = new QToolButton(this);
    m_row->addWidget(b);
    connect(b, &QToolButton::clicked, this, [this, id] {
        if (id == cu59::kItemArmCard) emit armRequested();
        else if (id == cu59::kItemArmFreshCard) emit armFreshRequested();
        else if (id == cu59::kCardControlEject) m_controller.ejectCard();
    });
    m_buttons.emplace(id, b);
    return b;
}

void CardControls::draw(QToolButton* b, const cu59::CardControl& c) const {
    b->setText(QString::fromStdString(c.icon + (m_labels ? " " + c.label : std::string())));
    b->setAccessibleName(QString::fromStdString(c.name));
    b->setToolTip(QString::fromStdString(c.available ? c.name : c.reason));
    b->setEnabled(c.available);
}

void CardControls::refresh() {
    auto declared = m_controller.cardControls();
    if (declared == m_declared) return;
    m_declared = std::move(declared);
    for (auto& [id, b] : m_buttons) b->setVisible(false);
    for (const auto& c : m_declared) {
        QToolButton* b = button(c.id);
        draw(b, c);
        b->setVisible(true);
    }
}

void CardControls::setShowLabels(bool on) {
    m_labels = on;
    for (const auto& c : m_declared) draw(m_buttons.at(c.id), c);
}

int CardControls::labelsWidth() const {
    int w = 0;
    for (const auto& c : m_declared)
        w += m_buttons.at(c.id)->fontMetrics().horizontalAdvance(QString::fromStdString(" " + c.label));
    return w;
}
