#include "CardPickerDialog.hpp"
#include "ChromeStyle.hpp"
#include <QDir>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

namespace {
constexpr int kEntryRole = Qt::UserRole + 1;
}

CardPickerDialog::CardPickerDialog(Mode mode, const std::vector<calcu::media::PickerEntry>& cards, const QString& folder,
                                   QWidget* parent)
    : QDialog(parent), m_mode(mode) {
    setWindowTitle(mode == Mode::Arm ? QStringLiteral("Arm Card") : QStringLiteral("Arm Fresh Card"));
    setMinimumSize(320, 300);
    auto* col = new QVBoxLayout(this);

    if (mode == Mode::ArmFresh) {
        m_name = new QLineEdit(this);
        m_name->setPlaceholderText(QStringLiteral("Card name"));
        col->addWidget(m_name);
    }

    m_list = new QListWidget(this);
    col->addWidget(m_list, 1);
    if (cards.empty()) {
        auto* item = new QListWidgetItem(mode == Mode::Arm ? QStringLiteral("No cards found") : QStringLiteral("No saved cards"));
        item->setFlags(Qt::NoItemFlags);
        m_list->addItem(item);
    }
    for (const auto& e : cards) {
        auto* item = new QListWidgetItem(QString::fromStdString(e.label), m_list);
        item->setData(kEntryRole, QString::fromStdString(e.id));
        if (!e.canChoose) {   // not now, with its hint
            item->setFlags(Qt::NoItemFlags);
            if (e.hint) item->setToolTip(QString::fromStdString(*e.hint));
        } else if (e.tooltip) {
            item->setToolTip(QString::fromStdString(*e.tooltip));
        }
    }
    connect(m_list, &QListWidget::itemClicked, this, [this](QListWidgetItem* item) {
        if (m_mode == Mode::ArmFresh) m_name->setText(item->text());
    });
    connect(m_list, &QListWidget::itemActivated, this, [this](QListWidgetItem* item) {
        if (m_mode == Mode::ArmFresh) m_name->setText(item->text());
        accept();
    });

    auto* footer = new QHBoxLayout;
    auto* path = new QLabel(QDir::toNativeSeparators(folder), this);
    path->setStyleSheet(QStringLiteral("color: %1; font-size: 10px;").arg(calcu::qt6::chrome::qcss(ChromeStyle::colours().textDim)));
    path->setTextInteractionFlags(Qt::TextSelectableByMouse);
    footer->addWidget(path, 1);
    auto* cancel = new QPushButton(QStringLiteral("Cancel"), this);
    connect(cancel, &QPushButton::clicked, this, &QDialog::reject);
    footer->addWidget(cancel);
    m_ok = new QPushButton(QStringLiteral("Arm"), this);
    m_ok->setDefault(true);
    connect(m_ok, &QPushButton::clicked, this, &CardPickerDialog::accept);
    if (m_name) {
        m_ok->setEnabled(false);
        connect(m_name, &QLineEdit::textChanged, this, [this](const QString& t) { m_ok->setEnabled(!t.trimmed().isEmpty()); });
    }
    footer->addWidget(m_ok);
    col->addLayout(footer);
}

QString CardPickerDialog::freshName() const { return m_name ? m_name->text().trimmed() : QString(); }

bool CardPickerDialog::confirmReplace(const QString& question, QWidget* parent) {
    QMessageBox ask(QMessageBox::Question, QStringLiteral("Replace card"), question, QMessageBox::Cancel, parent);
    QPushButton* replace = ask.addButton(QStringLiteral("Replace"), QMessageBox::DestructiveRole);
    ask.exec();
    return ask.clickedButton() == replace;
}

void CardPickerDialog::accept() {
    if (m_mode == Mode::ArmFresh) {
        if (freshName().isEmpty()) return;
    } else {
        QListWidgetItem* item = m_list->currentItem();
        if (!item || !(item->flags() & Qt::ItemIsEnabled)) return;
        m_selected = item->data(kEntryRole).toString();
        if (m_selected.isEmpty()) return;
    }
    QDialog::accept();
}
