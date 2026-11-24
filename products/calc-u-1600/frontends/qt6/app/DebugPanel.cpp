#include "DebugPanel.hpp"
#include "qt6/FixedFont.hpp"

#include <QEvent>
#include <QFont>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QPalette>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollBar>
#include <QStyle>
#include <QTextCursor>
#include <QVBoxLayout>

#include <algorithm>
#include <cstdio>

#include "AppPaths.hpp"
#include "AppSettings.hpp"
#include "app/SharpChrome.hpp"
#include "qt6/ChromeStyle.hpp"
#include "Debug/Inspect/HexDump.hpp"
#include "MachineController.hpp"
#include "app/ModuleYamlDump.hpp"
#include "app/Text.hpp"

namespace {

using inspect::fmt;

}  // namespace

DebugPanel::DebugPanel(MachineController* controller, QWidget* parent)
    : QWidget(parent), m_controller(controller) {
    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->setSpacing(0);

    // ── Header ───────────────────────────────────────────────────────
    m_header = new QLabel(tr("INSPECTOR"), this);
    m_header->setAlignment(Qt::AlignCenter);
    QFont headerFont = m_header->font();
    headerFont.setBold(true);
    headerFont.setPointSize(headerFont.pointSize() - 1);
    m_header->setFont(headerFont);
    outer->addWidget(m_header);

    // ── Output area ──────────────────────────────────────────────────
    m_output = new QPlainTextEdit(this);
    m_output->setReadOnly(true);
    m_output->setLineWrapMode(QPlainTextEdit::NoWrap);
    // Tables don't wrap; a table wider than the panel needs a scroll bar the
    // user can see (macOS hides overlay scroll bars until one scrolls).
    m_output->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    // Never take keyboard focus from MainWindow (which owns physical-
    // keyboard typing for the calculator) -- same convention as
    // ControlBar's widgets.
    m_output->setFocusPolicy(Qt::NoFocus);
    connect(m_output, &QPlainTextEdit::copyAvailable, this, &DebugPanel::outputSelectionChanged);
    QFont monoFont = calcu::qt6::fixedFont();
    monoFont.setPointSize(11);
    m_output->setFont(monoFont);
    m_output->setMinimumHeight(120);
    outer->addWidget(m_output, /*stretch=*/1);

    // ── Button bar ───────────────────────────────────────────────────
    m_buttonBar = new QWidget(this);
    m_buttonBar->setObjectName(QStringLiteral("panel-buttons"));
    m_buttonBar->setAttribute(Qt::WA_StyledBackground, true);
    auto* row = new QHBoxLayout(m_buttonBar);
    row->setContentsMargins(10, 0, 10, 0);
    row->setSpacing(8);
    m_pointersButton = new QPushButton(tr("Pointers"), m_buttonBar);
    m_memoryButton = new QPushButton(tr("Memory ▾"), m_buttonBar);
    m_dumpButton = new QPushButton(tr("Dump ▾"), m_buttonBar);
    m_moduleYamlButton = new QPushButton(tr("Module YAML"), m_buttonBar);
    m_moduleYamlButton->setToolTip(tr("The module in each slot, as initial-content: YAML for a module definition"));
    m_clearButton = new QPushButton(m_buttonBar);
    m_clearButton->setIcon(style()->standardIcon(QStyle::SP_TrashIcon));
    m_clearButton->setToolTip(tr("Clear"));
    m_clearButton->setEnabled(false);
    m_pointersButton->setObjectName(QStringLiteral("debugpanel.pointers"));
    m_memoryButton->setObjectName(QStringLiteral("debugpanel.memory"));
    m_dumpButton->setObjectName(QStringLiteral("debugpanel.dump"));
    m_moduleYamlButton->setObjectName(QStringLiteral("debugpanel.module-yaml"));
    m_clearButton->setObjectName(QStringLiteral("debugpanel.clear"));
    for (auto* b : {m_pointersButton, m_memoryButton, m_dumpButton, m_moduleYamlButton, m_clearButton}) {
        b->setFocusPolicy(Qt::NoFocus);
    }
    // The menus are filled each time they open: what they offer depends on
    // the model and on what is plugged in (inspect::MenuEntry).
    m_memoryButton->setMenu(new QMenu(m_memoryButton));
    m_dumpButton->setMenu(new QMenu(m_dumpButton));
    connect(m_memoryButton->menu(), &QMenu::aboutToShow, this, [this] { fillMenu(m_memoryButton->menu(), false); });
    connect(m_dumpButton->menu(), &QMenu::aboutToShow, this, [this] { fillMenu(m_dumpButton->menu(), true); });
    row->addWidget(m_pointersButton);
    row->addWidget(m_memoryButton);
    row->addWidget(m_dumpButton);
    row->addStretch(1);
    row->addWidget(m_moduleYamlButton);
    row->addWidget(m_clearButton);
    outer->addWidget(m_buttonBar);

    connect(m_pointersButton, &QPushButton::clicked, this, [this] { showView(inspect::View::Pointers); });
    connect(m_moduleYamlButton, &QPushButton::clicked, this, &DebugPanel::debugDumpModuleAsYaml);
    connect(m_clearButton, &QPushButton::clicked, this, &DebugPanel::clearDebug);

    applyChrome();
}

QString DebugPanel::selectedOutputText() const {
    // QTextCursor uses U+2029 as the paragraph separator.
    return m_output->textCursor().selectedText().replace(QChar::ParagraphSeparator, QLatin1Char('\n'));
}

DebugPanel::~DebugPanel() = default;

void DebugPanel::changeEvent(QEvent* event) {
    QWidget::changeEvent(event);
    if (event->type() == QEvent::PaletteChange || event->type() == QEvent::ApplicationPaletteChange) {
        applyChrome();
    }
}

void DebugPanel::applyChrome() {
    namespace ch = calcu::qt6::chrome;
    const calcu::shell::Chrome& chrome = cu1600::sharpChrome();
    const calcu::shell::ChromeColours& c = chrome.colours(ch::darkAppearance(this));
    m_header->setStyleSheet(ch::header(c, chrome.sizes));
    m_output->setStyleSheet(ch::output(c, QStringLiteral("QPlainTextEdit")));
    m_buttonBar->setStyleSheet(ch::panelButtons(c, chrome.sizes, QStringLiteral("QPushButton")));
}

// ── Ring buffer / paging ──────────────────────────────────────────────

void DebugPanel::ringWrite(const std::string& line) {
    m_ringBuf[m_ringHead % kRingBufCap] = line;
    ++m_ringHead;
}

void DebugPanel::ringWriteAll(const std::vector<std::string>& lines) {
    for (const auto& line : lines) ringWrite(line);
    refreshDebugDisplay();
}

void DebugPanel::refreshDebugDisplay() {
    const int head = m_ringHead;
    const int newSinceLast = head - m_debugLastDisplayHead;
    m_debugLastDisplayHead = head;

    const int totalAvailable = std::min(head, kRingBufCap);
    if (totalAvailable <= 0) {
        m_output->setPlainText(QString());
        m_clearButton->setEnabled(false);
        return;
    }

    const int linesToShow = newSinceLast > kDisplayLinesPerPage
        ? std::min(kDisplayLinesPerPage, totalAvailable)
        : std::min(kDisplayLinesPerPage * kDisplayPageCap, totalAvailable);
    const int startPos = head - linesToShow;

    QString text;
    for (int i = 0; i < linesToShow; ++i) {
        if (i > 0) text += '\n';
        text += QString::fromStdString(m_ringBuf[(startPos + i) % kRingBufCap]);
    }
    m_output->setPlainText(text);
    // To the last line's start, not its end: a wide table or dump line would
    // otherwise scroll the view sideways to its right edge.
    QTextCursor cursor = m_output->textCursor();
    cursor.movePosition(QTextCursor::End);
    cursor.movePosition(QTextCursor::StartOfBlock);
    m_output->setTextCursor(cursor);
    m_output->horizontalScrollBar()->setValue(0);
    m_clearButton->setEnabled(true);
}

void DebugPanel::clearDebug() {
    m_ringHead = 0;
    m_debugLastDisplayHead = 0;
    m_output->setPlainText(QString());
    m_clearButton->setEnabled(false);
}

// ── Inspector views ────────────────────────────────────────────────────

void DebugPanel::fillMenu(QMenu* menu, bool dumps) {
    menu->clear();
    const bool pc1500 = !m_controller->app().isPC1600();
    for (const inspect::MenuEntry& e : m_controller->inspectorMenu(dumps)) {
        QAction* a = menu->addAction(QString::fromUtf8(inspect::viewTitle(e.view, pc1500)));
        a->setEnabled(e.enabled);
        const inspect::View v = e.view;
        connect(a, &QAction::triggered, this, [this, v] { showView(v); });
    }
}

void DebugPanel::showView(inspect::View view) {
    const std::vector<std::string> lines = m_controller->inspectorView(view);
    // One ring entry, so a long dump isn't cut to the last display page.
    if (!lines.empty()) ringWriteAll({cu1600::joinLines(lines)});
}

// ── Module YAML ─────────────────────────────────────────────────────────

void DebugPanel::debugDumpModuleAsYaml() { ringWriteAll(cu1600::moduleYamlDump(m_controller->app())); }
