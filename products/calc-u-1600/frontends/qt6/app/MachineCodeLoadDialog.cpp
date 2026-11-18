#include "MachineCodeLoadDialog.hpp"

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

#include <algorithm>

MachineCodeLoadDialog::MachineCodeLoadDialog(QWidget* parent, Ask ask,
                                             calcu::sessions::Value args, const cu1600::CodePlanReply& plan)
    : QDialog(parent), m_ask(std::move(ask)), m_args(std::move(args)) {
    setWindowTitle(tr("Load Machine Code"));
    setObjectName(QStringLiteral("dialog.machinecode"));
    // Wide enough that the wrapped notes read as sentences; the height
    // follows the text (refresh() re-fits it whenever the hint changes).
    setMinimumWidth(460);

    auto* layout = new QVBoxLayout(this);
    auto* form = new QFormLayout();
    layout->addLayout(form);

    m_addressField = new QLineEdit(this);
    m_addressField->setPlaceholderText(QString::fromStdString(plan.placeholder));
    m_addressField->setText(QString::fromStdString(plan.proposed));
    form->addRow(tr("Start address:"), m_addressField);
    connect(m_addressField, &QLineEdit::textChanged, this, &MachineCodeLoadDialog::refresh);
    form->addRow(tr("Length:"), new QLabel(tr("%1 bytes").arg(static_cast<qulonglong>(plan.length)), this));

    if (!plan.cpuNote.empty()) {
        auto* note = new QLabel(QString::fromStdString(plan.cpuNote), this);
        note->setWordWrap(true);
        layout->addWidget(note);
    }

    m_hint = new QLabel(this);
    m_hint->setWordWrap(true);
    layout->addWidget(m_hint);

    m_buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    m_buttons->button(QDialogButtonBox::Ok)->setText(tr("Load"));
    layout->addWidget(m_buttons);
    connect(m_buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(m_buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    resize(minimumWidth(), height());
    refresh();
}

QString MachineCodeLoadDialog::address() const { return m_addressField->text(); }

void MachineCodeLoadDialog::refresh() {
    // The plan with the typed address (C1600-2 AC-18): its refusal, or where
    // the code would go; an empty field asks again, saying nothing.
    const std::string text = m_addressField->text().toStdString();
    calcu::sessions::Value::Object o = *m_args.members();
    o.push_back({"address", calcu::sessions::Value(text)});
    const calcu::sessions::ReplyMessage r =
        m_ask(cu1600::requests::kCodePlan, calcu::sessions::Value(std::move(o)));
    const auto* plan = calcu::sessions::payloadAs<cu1600::CodePlanReply>(r.result);
    bool holds = false;
    QString hint;
    if (!r.done) {
        hint = QString::fromStdString(r.reason);
    } else if (plan && !plan->needsAddress) {
        holds = plan->refusal.empty();
        hint = QString::fromStdString(holds ? plan->where : plan->refusal);
    }
    m_hint->setText(hint);
    m_hint->setVisible(!hint.isEmpty());
    m_buttons->button(QDialogButtonBox::Ok)->setEnabled(holds);
    // Re-fit the height to the wrapped text at the current width, or a long
    // hint gets clipped.
    layout()->activate();
    const int h = layout()->hasHeightForWidth() ? layout()->totalHeightForWidth(width()) : sizeHint().height();
    resize(width(), std::max(h, layout()->totalMinimumSize().height()));
}
