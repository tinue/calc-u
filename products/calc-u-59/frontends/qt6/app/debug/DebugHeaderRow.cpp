#include "DebugHeaderRow.hpp"
#include "../ChromeStyle.hpp"
#include "DebugText.hpp"
#include "../EmulatorController.hpp"
#include "qt6/RunControlButtons.hpp"
#include <QHBoxLayout>
#include <QLabel>
#include <QToolButton>

DebugHeaderRow::DebugHeaderRow(EmulatorController& controller, bool cpu, QWidget* parent)
    : QWidget(parent), m_controller(controller), m_cpu(cpu),
      m_level(cpu ? calcu::debugger::kCpu : calcu::debugger::kProgram) {
    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet(QStringLiteral("DebugHeaderRow { background: %1; }").arg(ChromeStyle::rgb(cu59::chrome::kDebugHeader)));
    auto* row = new QHBoxLayout(this);
    row->setContentsMargins(10, 6, 10, 6);
    row->setSpacing(8);
    m_title = new QLabel(this);
    m_title->setStyleSheet(QStringLiteral("color: %1; font-weight: bold; font-size: 11px;").arg(ChromeStyle::rgb(ChromeStyle::text(0.6))));
    m_title->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    m_title->setMinimumWidth(40);
    row->addWidget(m_title, 1);   // takes the spare width; shrinks first
    m_run = new calcu::qt6::RunControlButtons(m_level, {QStringLiteral("FREEZE"), QStringLiteral("STEP"), QStringLiteral("RESUME")}, this);
    m_run->setSender([this](const calcu::sessions::CommandName& n, calcu::sessions::Value v) {
        m_controller.sendCommand(n, std::move(v));
    });
    m_run->hide();   // its buttons go into this row
    for (const auto& b : cu59::debugHeader(cpu).buttons) {
        QToolButton* w = nullptr;
        if (b.id == "resume") w = m_run->resumeButton();
        else if (b.id == "freeze") w = m_run->pauseButton();
        else if (b.id == "step") w = m_run->stepButton();
        else {
            w = new QToolButton(this);
            if (b.id == "freeze-on-start") connect(w, &QToolButton::clicked, this, [this] { m_controller.armStart(m_level); });
            else connect(w, &QToolButton::clicked, this, [this] { m_controller.disarmStart(); });
        }
        w->setParent(this);
        w->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);   // the title gives way first
        m_buttons[b.id] = w;
        row->addWidget(w);
    }
}

void DebugHeaderRow::refresh(int baseFontSize) {
    const cu59::DebugHeader h = cu59::debugHeader(m_cpu);
    const bool narrow = cu59::debugHeaderNarrow(baseFontSize);
    m_title->setText(QString::fromStdString(narrow ? h.narrowTitle : h.title));
    for (const auto& b : h.buttons) {
        QToolButton* w = m_buttons[b.id];
        w->setText(QString::fromStdString(narrow ? b.narrowLabel : b.label));
        if (baseFontSize != m_fontSize) w->setStyleSheet(ChromeStyle::headerButton(ChromeStyle::fromRgb(b.colour), baseFontSize));
    }
    m_fontSize = baseFontSize;
    m_run->setRunState(m_controller.runState());
    const cu59::FreezeButtons f = m_controller.freezeButtons(m_level);
    m_buttons["freeze-on-start"]->setEnabled(f.freezeOnStart);
    m_buttons["armed"]->setEnabled(f.armed);
}
