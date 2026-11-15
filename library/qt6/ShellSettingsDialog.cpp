#include "qt6/ShellSettingsDialog.hpp"
#include "qt6/Shell.hpp"
#include "shell/Inputs.hpp"
#include "shell/ObjectNames.hpp"
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QVBoxLayout>
#include <limits>

namespace calcu::qt6 {

namespace {

using shell::DialogRow;
using Widget = DialogRow::Widget;

QString q(const std::string& s) { return QString::fromStdString(s); }

const QString kChoose = QStringLiteral("Choose…");   // S4: Qt6 only

std::string structureOf(const std::vector<shell::ShownSection>& ss) {
    std::string s;
    for (const auto& sec : ss) {
        s += sec.title + "{";
        for (const auto& r : sec.rows) {
            s += r.row.section + "." + r.row.key + std::to_string(static_cast<int>(r.row.widget)) + "(";
            for (const auto& [v, l] : r.choices) s += v + "=" + l + ",";
            s += ")";
        }
        s += "}";
    }
    return s;
}

} // namespace

ShellSettingsDialog::ShellSettingsDialog(Shell& shell, QWidget* parent) : QDialog(parent), shell_(shell) {
    setObjectName(shell::names::kSettingsDialog);
    setWindowTitle(QString::fromUtf8(shell::kSettingsTitle));
    setWindowModality(Qt::WindowModal);   // D-18
    auto* layout = new QVBoxLayout(this);
    content_ = new QWidget(this);
    contentLayout_ = new QVBoxLayout(content_);
    contentLayout_->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(content_);
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);   // S1: the only button
    buttons->setObjectName(shell::names::kSettingsClose);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttons);
}

void ShellSettingsDialog::refresh() {
    shown_ = shell::dialog(shell_.inputs().settings, shell_.inputs().presetList, shell_.dialogSpec());
    const std::string structure = structureOf(shown_);
    if (structure != structure_) {
        structure_ = structure;
        rebuild();
    }
    update();
}

void ShellSettingsDialog::send(const DialogRow& row, const QString& value, bool null) {
    shell_.sendChecked(shell::commands::kSetSetting,
                       sessions::Value::object({{"section", row.section},
                                                {"key", row.key},
                                                {"value", null ? sessions::Value() : sessions::Value(value.toStdString())}}));
}

QWidget* ShellSettingsDialog::rowWidget(const shell::ShownRow& r, QWidget* parent) {
    const DialogRow row = r.row;
    const QString name = q(shell::names::setting(row.section, row.key));
    switch (row.widget) {
    case Widget::StartupModel:
    case Widget::Choice:
    case Widget::StartupPreset: {
        auto* box = new QComboBox(parent);
        box->setObjectName(name);
        for (const auto& [v, l] : r.choices) box->addItem(q(l), q(v));
        if (row.widget == Widget::StartupPreset) box->addItem(kChoose, QString());
        connect(box, &QComboBox::activated, this, [this, row, box](int index) {
            if (row.widget == Widget::StartupPreset && box->itemText(index) == kChoose && index == box->count() - 1) {
                const QString file = shell_.choosers().file(this, QStringLiteral("Startup preset for %1").arg(q(row.key)),
                                                            shell_.startFolder("presets"), q(shell_.product().presetFilter));
                if (!file.isEmpty()) send(row, file);
                else update();   // cancelled: the row shows what is set
                return;
            }
            send(row, box->itemData(index).toString());
        });
        return box;
    }
    case Widget::Folder: {   // S5: the folder in use, Change…, Reset
        auto* w = new QWidget(parent);
        w->setObjectName(name);
        auto* l = new QHBoxLayout(w);
        l->setContentsMargins(0, 0, 0, 0);
        auto* shown = new QLabel(w);
        shown->setObjectName(name + QStringLiteral("-shown"));
        shown->setTextInteractionFlags(Qt::TextSelectableByMouse);
        auto* change = new QPushButton(QStringLiteral("Change…"), w);
        change->setObjectName(name + QStringLiteral("-change"));
        auto* reset = new QPushButton(QStringLiteral("Reset"), w);
        reset->setObjectName(name + QStringLiteral("-reset"));
        l->addWidget(shown, 1);
        l->addWidget(change);
        l->addWidget(reset);
        connect(change, &QPushButton::clicked, this, [this, row] {
            QString start;
            for (const auto& s : shown_)
                for (const auto& x : s.rows)
                    if (x.row.section == row.section && x.row.key == row.key) start = q(x.shown);
            if (start.startsWith(QLatin1Char('~'))) start = q(shell::read::text(shell_.inputs().settings, "home")) + start.mid(1);
            const QString folder = shell_.choosers().folder(this, q(row.label), start);
            if (!folder.isEmpty()) send(row, folder);
        });
        connect(reset, &QPushButton::clicked, this, [this, row] { send(row, QString(), true); });
        return w;
    }
    case Widget::Integer: {
        auto* spin = new QSpinBox(parent);
        spin->setObjectName(name);
        spin->setRange(r.min ? static_cast<int>(*r.min) : 0,
                       r.max ? static_cast<int>(*r.max) : std::numeric_limits<int>::max());
        spin->setKeyboardTracking(false);
        connect(spin, &QSpinBox::valueChanged, this, [this, row](int v) { send(row, QString::number(v)); });
        return spin;
    }
    case Widget::Text: {
        auto* edit = new QLineEdit(parent);
        edit->setObjectName(name);
        connect(edit, &QLineEdit::editingFinished, this, [this, row, edit] { send(row, edit->text()); });
        return edit;
    }
    case Widget::Note: {
        auto* label = new QLabel(q(row.label), parent);
        label->setObjectName(name);
        label->setWordWrap(true);
        return label;
    }
    }
    return nullptr;
}

void ShellSettingsDialog::rebuild() {
    delete content_;
    content_ = new QWidget(this);
    contentLayout_ = new QVBoxLayout(content_);
    contentLayout_->setContentsMargins(0, 0, 0, 0);
    static_cast<QVBoxLayout*>(layout())->insertWidget(0, content_);
    for (const auto& sec : shown_) {   // S1: a section without a row is not in the model
        auto* box = new QGroupBox(q(sec.title), content_);
        auto* form = new QFormLayout(box);
        for (const auto& r : sec.rows) {
            QWidget* w = rowWidget(r, box);
            if (r.row.widget == Widget::Note) form->addRow(w);
            else form->addRow(q(r.row.label), w);
        }
        contentLayout_->addWidget(box);
    }
}

void ShellSettingsDialog::update() {
    for (const auto& sec : shown_)
        for (const auto& r : sec.rows) {
            const QString name = q(shell::names::setting(r.row.section, r.row.key));
            switch (r.row.widget) {
            case Widget::StartupModel:
            case Widget::Choice:
            case Widget::StartupPreset:
                if (auto* box = content_->findChild<QComboBox*>(name)) {
                    QSignalBlocker block(box);
                    box->setCurrentIndex(box->findData(q(r.value)));
                }
                break;
            case Widget::Folder:
                if (auto* shown = content_->findChild<QLabel*>(name + QStringLiteral("-shown"))) shown->setText(q(r.shown));
                if (auto* reset = content_->findChild<QPushButton*>(name + QStringLiteral("-reset")))
                    reset->setEnabled(r.resettable);   // only while it differs from the default
                break;
            case Widget::Integer:
                if (auto* spin = content_->findChild<QSpinBox*>(name)) {
                    QSignalBlocker block(spin);
                    spin->setValue(QString::fromStdString(r.value).toInt());
                }
                break;
            case Widget::Text:
                if (auto* edit = content_->findChild<QLineEdit*>(name)) {
                    QSignalBlocker block(edit);
                    if (!edit->hasFocus()) edit->setText(q(r.value));
                }
                break;
            case Widget::Note: break;
            }
        }
}

} // namespace calcu::qt6
