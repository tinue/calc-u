#include "AsmOverlayBox.hpp"
#include "qt6/FixedFont.hpp"
#include "ChromeStyle.hpp"
#include "EmulatorController.hpp"
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

namespace {
QLabel* sectionTitle(const QString& text, QWidget* parent) {
    auto* l = new QLabel(text, parent);
    l->setAlignment(Qt::AlignCenter);
    l->setStyleSheet(QStringLiteral("color: %2; font-weight: bold; font-size: 11px;"
                                    " background: %1; padding: 6px;").arg(ChromeStyle::rgb(cu59::chrome::kDebugHeader), ChromeStyle::rgb(ChromeStyle::text(0.6))));
    return l;
}
} // namespace

AsmOverlayBox::AsmOverlayBox(EmulatorController& controller, QWidget* parent) : QWidget(parent), m_controller(controller) {
    auto* col = new QVBoxLayout(this);
    col->setContentsMargins(0, 0, 0, 0);
    col->setSpacing(0);
    col->addWidget(sectionTitle(QStringLiteral("ASM OVERLAY"), this));

    auto* box = new QWidget(this);
    box->setStyleSheet((QStringLiteral("QWidget { background: %1; } QLabel { ") + calcu::qt6::fixedFontCss(11) +
                        QStringLiteral(" }"))
                           .arg(ChromeStyle::rgb(cu59::chrome::kSection)));
    auto* boxCol = new QVBoxLayout(box);
    boxCol->setContentsMargins(10, 10, 10, 10);
    m_file = new QLabel(box);
    m_file->setStyleSheet(QStringLiteral("color: %1;").arg(ChromeStyle::rgb(ChromeStyle::text(0.85))));
    m_words = new QLabel(box);
    m_words->setStyleSheet(QStringLiteral("color: %1;").arg(ChromeStyle::rgb(ChromeStyle::text(0.75))));
    m_status = new QLabel(box);
    m_status->setStyleSheet(QStringLiteral("color: %1;").arg(ChromeStyle::rgb(ChromeStyle::text(0.7))));
    m_status->setWordWrap(true);
    boxCol->addWidget(m_file);
    boxCol->addWidget(m_words);
    boxCol->addWidget(m_status);
    auto* boxWrap = new QVBoxLayout;
    boxWrap->setContentsMargins(8, 8, 8, 8);
    boxWrap->addWidget(box);
    col->addLayout(boxWrap);

    auto* bar = new QWidget(this);
    bar->setObjectName(QStringLiteral("panel-buttons"));
    bar->setAttribute(Qt::WA_StyledBackground, true);
    bar->setStyleSheet(ChromeStyle::buttonBar());
    auto* row = new QHBoxLayout(bar);
    row->setContentsMargins(10, 0, 10, 0);
    auto* select = new QPushButton(QStringLiteral("Select File"), bar);
    connect(select, &QPushButton::clicked, this, &AsmOverlayBox::selectFileRequested);
    m_run = new QPushButton(QStringLiteral("Run"), bar);
    connect(m_run, &QPushButton::clicked, this, [this] { m_controller.runAsmOverlay(); });
    auto* clear = new QPushButton(QStringLiteral("Clear"), bar);
    connect(clear, &QPushButton::clicked, this, [this] { m_controller.clearAsmOverlay(); });
    row->addWidget(select);
    row->addWidget(m_run);
    row->addWidget(clear);
    row->addStretch(1);
    col->addWidget(bar);

    connect(&m_controller, &EmulatorController::asmChanged, this, &AsmOverlayBox::refresh);
    connect(&m_controller, &EmulatorController::entriesChanged, this, &AsmOverlayBox::refresh);
    refresh();
}

void AsmOverlayBox::refresh() {
    m_file->setText(QStringLiteral("File: ") + m_controller.asmFileName());
    m_words->setText(QStringLiteral("Words: %1").arg(m_controller.asmWordCount()));
    m_status->setText(m_controller.asmStatus());
    // The product entry c59.asm-run (instructions loaded, no operation running).
    m_run->setEnabled(m_controller.entryAvailable(cu59::entries::kAsmRun));
    m_run->setToolTip(m_controller.entryReason(cu59::entries::kAsmRun));
}
