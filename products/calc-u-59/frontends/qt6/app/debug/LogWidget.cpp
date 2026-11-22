#include "LogWidget.hpp"
#include "ChromeStyle.hpp"
#include "EmulatorController.hpp"
#include <QHBoxLayout>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollBar>
#include <QVBoxLayout>

LogWidget::LogWidget(EmulatorController& controller, QWidget* parent) : QWidget(parent), m_controller(controller) {
    auto* col = new QVBoxLayout(this);
    col->setContentsMargins(0, 0, 0, 0);
    col->setSpacing(0);

    auto* title = new QLabel(QStringLiteral("DEBUG LOG"), this);
    title->setAlignment(Qt::AlignCenter);
    title->setStyleSheet(QStringLiteral("color: %2; font-weight: bold; font-size: 11px;"
                                        " background: %1; padding: 6px;").arg(ChromeStyle::rgb(cu59::chrome::kDebugHeader), ChromeStyle::rgb(ChromeStyle::text(0.6))));
    col->addWidget(title);

    m_text = new QPlainTextEdit(this);
    m_text->setReadOnly(true);
    m_text->setStyleSheet(QStringLiteral("QPlainTextEdit { background: %1; color: %2; border: 1px solid %3;"
                                         " font-family: Menlo, monospace; font-size: 11px; padding: 8px; }")
                              .arg(ChromeStyle::rgb(cu59::chrome::kSection), ChromeStyle::rgb(cu59::chrome::kLogText),
                                   ChromeStyle::rgb(ChromeStyle::qcolour(ChromeStyle::colours().pill))));
    col->addWidget(m_text, 1);

    // Two rows, each a panel button bar of the chrome's height: the lower
    // one lines up with the printer's.
    auto* bar = new QWidget(this);
    auto* rows = new QVBoxLayout(bar);
    rows->setContentsMargins(0, 0, 0, 0);
    rows->setSpacing(0);
    auto barRow = [bar, rows] {
        auto* w = new QWidget(bar);
        w->setObjectName(QStringLiteral("panel-buttons"));
        w->setAttribute(Qt::WA_StyledBackground, true);
        w->setStyleSheet(ChromeStyle::buttonBar());
        auto* row = new QHBoxLayout(w);
        row->setContentsMargins(10, 0, 10, 0);
        rows->addWidget(w);
        return row;
    };
    auto* row1 = barRow();
    struct Dump { const char* name; void (EmulatorController::*fn)(); };
    for (Dump d : {Dump{"SCOM", &EmulatorController::dumpScom}, Dump{"Memory", &EmulatorController::dumpMemory},
                   Dump{"Prog", &EmulatorController::dumpProg}, Dump{"Vars", &EmulatorController::dumpVars}}) {
        auto* b = new QPushButton(QLatin1String(d.name), bar);
        connect(b, &QPushButton::clicked, this, [this, fn = d.fn] { (m_controller.*fn)(); });
        row1->addWidget(b);
    }
    row1->addStretch(1);
    m_clear = new QPushButton(QStringLiteral("🗑"), bar);
    m_clear->setToolTip(QStringLiteral("Clear the log"));
    connect(m_clear, &QPushButton::clicked, this, [this] { m_controller.clearLog(); });
    row1->addWidget(m_clear);

    auto* row2 = barRow();
    m_trace = new QPushButton(bar);
    m_trace->setToolTip(QStringLiteral("Write every executed instruction to the binary trace file"));
    // The library's trace session, the button's file (trace {button: true}).
    connect(m_trace, &QPushButton::clicked, this, [this] { m_controller.setTrace(!m_controller.traceRunning()); });
    m_level = new QPushButton(bar);
    m_level->setToolTip(QStringLiteral("Log level: off / info (how presets run) / debug (every register write)"));
    connect(m_level, &QPushButton::clicked, this, [this] { m_controller.cycleDebugLevel(); });
    row2->addWidget(m_trace);
    row2->addWidget(m_level);
    row2->addStretch(1);
    col->addWidget(bar);

    connect(&m_controller, &EmulatorController::logChanged, this, &LogWidget::refresh);
    connect(&m_controller, &EmulatorController::traceChanged, this, &LogWidget::refresh);
    refresh();
}

void LogWidget::refresh() {
    if (m_text->toPlainText() != m_controller.logText()) {
        m_text->setPlainText(m_controller.logText());
        m_text->verticalScrollBar()->setValue(m_text->verticalScrollBar()->maximum());
    }
    m_clear->setEnabled(!m_controller.logText().isEmpty());
    m_trace->setText(m_controller.traceRunning() ? QStringLiteral("TRACE ●") : QStringLiteral("TRACE ○"));
    // TRACE can always be pressed; a trace folder that cannot be
    // written refuses the start.
    m_trace->setStyleSheet(m_controller.traceRunning() ? QStringLiteral("QPushButton { color: %1; }").arg(ChromeStyle::rgb(cu59::chrome::kOrange)) : QString());
    const char* levels[3] = {"LOG ○", "LOG ●", "LOG ●"};
    int level = static_cast<int>(m_controller.debugLevel());
    m_level->setText(QLatin1String(levels[level]));
    m_level->setStyleSheet(level == 0 ? QString()
                                      : QStringLiteral("QPushButton { color: %1; }").arg(ChromeStyle::rgb(level == 1 ? cu59::chrome::kOrange : cu59::chrome::kRed)));
}
