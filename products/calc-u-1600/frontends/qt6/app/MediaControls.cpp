#include "MediaControls.hpp"

#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QStyle>

#include "MachineController.hpp"
#include "app/SharpControlRules.hpp"
#include "control/ControlState.hpp"
#include "media/MediaTopics.hpp"
#include "media/SaveRules.hpp"
#include "qt6/MediaPicker.hpp"
#include "qt6/NameAndSaveDialog.hpp"
#include "shell/Inputs.hpp"

namespace sessions = calcu::sessions;

namespace {

QPushButton* compactButton(QWidget* parent, const QString& tip, const QString& name) {
    auto* b = new QPushButton(parent);
    b->setIcon(parent->style()->standardIcon(QStyle::SP_DialogSaveButton));
    b->setToolTip(tip);
    b->setFocusPolicy(Qt::NoFocus);
    b->setFixedWidth(b->iconSize().width() + 12);
    b->setObjectName(name);
    return b;
}

void lampStyle(QLabel* lamp, bool on) {
    lamp->setStyleSheet(on ? QStringLiteral("color: #2ecc40;") : QStringLiteral("color: palette(mid);"));
}

} // namespace

MediaPlaceControl::MediaPlaceControl(MachineController& c, std::string place, QWidget* parent)
    : QWidget(parent), m_controller(c), m_place(std::move(place)) {
    m_picker = new calcu::qt6::MediaPicker(this);
    m_picker->setFocusPolicy(Qt::NoFocus);
    m_picker->setMinimumContentsLength(9);
    m_picker->setSender([this](const sessions::CommandName& name, sessions::Value args) {
        m_controller.send(name, std::move(args));   // a refusal is a notice (the shell shows it)
    });
    connect(&c.host(), &calcu::qt6::SessionHost::topicChanged, this, [this](const QString& topic) {
        if (topic == QLatin1String("media") || topic == QLatin1String("control-state")) refresh();
        else if (topic == QLatin1String("c1600.lamps")) refreshLamps();
    });
}

void MediaPlaceControl::refresh() {
    const calcu::media::MediaState* s = m_controller.media();
    const calcu::media::PlaceState* p = s ? s->place(m_place) : nullptr;
    calcu::media::PlaceState shown;
    shown.place = m_place;
    m_picker->setPlace(p ? *p : shown);
    if (m_save) m_save->setEnabled(p && p->nameAndSave);
}

void MediaPlaceControl::nameAndSave(const QString& prompt) {
    const calcu::media::MediaState* s = m_controller.media();
    const calcu::media::PlaceState* p = s ? s->place(m_place) : nullptr;
    if (!p) return;
    calcu::qt6::NameAndSaveDialog dialog(window());
    dialog.setPlace(*p);
    dialog.setPrompt(prompt);   // LIB-7 AC-40: "Instance name:" or "Disk name:"
    dialog.setSender([this](const sessions::CommandName& name, sessions::Value args) -> std::string {
        // The save with the switch on is an operation (its power cycle, AC-19).
        const sessions::ReplyMessage r = m_controller.sendAndWait(name, std::move(args));
        return r.done ? std::string() : r.reason;
    });
    dialog.exec();
}

SlotControl::SlotControl(MachineController& c, std::string place, QWidget* parent) : MediaPlaceControl(c, place, parent) {
    auto* l = new QHBoxLayout(this);
    l->setContentsMargins(0, 0, 0, 0);
    l->setSpacing(2);
    l->addWidget(m_picker);
    m_save = compactButton(this, tr("Name & Save"), QString::fromStdString("media-save-" + m_place));
    l->addWidget(m_save);
    connect(m_save, &QPushButton::clicked, this, [this] { nameAndSave(tr("Instance name:")); });
    refresh();
}

DiskControl::DiskControl(MachineController& c, QWidget* parent) : MediaPlaceControl(c, "floppy", parent) {
    setObjectName(QStringLiteral("bar-disk"));
    auto* l = new QHBoxLayout(this);
    l->setContentsMargins(0, 0, 0, 0);
    l->setSpacing(2);
    m_label = new QLabel(tr("Disk:"), this);
    l->addWidget(m_label);
    l->addWidget(m_picker);
    m_side = new QPushButton(QStringLiteral("0"), this);
    m_side->setFocusPolicy(Qt::NoFocus);
    m_side->setToolTip(tr("Eject and turn the disk over"));
    m_side->setObjectName(QStringLiteral("media-side-floppy"));
    m_side->setFixedWidth(m_side->fontMetrics().horizontalAdvance(QStringLiteral("00")) + 16);
    l->addWidget(m_side);
    m_save = compactButton(this, tr("Name & Save"), QStringLiteral("media-save-floppy"));
    l->addWidget(m_save);
    m_lamp = new QLabel(QStringLiteral("●"), this);
    m_lamp->setObjectName(QStringLiteral("media-lamp-floppy"));
    m_lamp->setFixedWidth(14);
    m_lamp->setAlignment(Qt::AlignCenter);
    m_lamp->setToolTip(tr("Drive active -- wait for this to go dark before turning the disk over"));
    l->addWidget(m_lamp);
    connect(m_side, &QPushButton::clicked, this,
            [this] { m_controller.send(calcu::media::kTurnCommand, sessions::Value::object({{"place", "floppy"}})); });
    connect(m_save, &QPushButton::clicked, this, [this] { nameAndSave(tr("Disk name:")); });
    refresh();
}

void DiskControl::refresh() {
    MediaPlaceControl::refresh();
    // The product entry (LIB-4 AC-17, LIB-14 AC-42, D-29).
    const auto* cs = m_controller.state<calcu::control::ControlState>(calcu::control::kControlStateTopic);
    const calcu::control::ProductEntry* e = cs ? cs->productEntry(shared::kDiskControlsEntry) : nullptr;
    const sessions::Value v = e && e->value ? e->value->toValue() : sessions::Value();
    const bool shown = calcu::shell::read::flag(v, "shown");
    const calcu::shell::Availability a = calcu::shell::availabilityOf(v);
    setVisible(shown);
    for (QWidget* w : {static_cast<QWidget*>(m_label), static_cast<QWidget*>(m_picker), static_cast<QWidget*>(m_side)}) {
        w->setEnabled(a.now);
        w->setToolTip(a.now ? QString() : QString::fromStdString(a.reason));
    }
    if (!a.now) m_save->setEnabled(false);
    const calcu::media::MediaState* s = m_controller.media();
    const calcu::media::PlaceState* p = s ? s->place("floppy") : nullptr;
    m_side->setText(QString::number(p && p->holds && p->holds->side ? *p->holds->side : 0));
    refreshLamps();
}

void DiskControl::refreshLamps() { lampStyle(m_lamp, m_controller.lamps().floppyMotor); }

TapeControl::TapeControl(MachineController& c, QWidget* parent) : MediaPlaceControl(c, "tape", parent) {
    auto* l = new QHBoxLayout(this);
    l->setContentsMargins(0, 0, 0, 0);
    l->setSpacing(2);
    l->addWidget(new QLabel(tr("Tape:"), this));
    m_picker->setToolTip(tr("A tape to play (CLOAD)"));
    l->addWidget(m_picker);
    m_save = compactButton(this, tr("A new tape to record onto (CSAVE)"), QStringLiteral("media-save-tape"));
    l->addWidget(m_save);
    m_counter = new QLabel(QStringLiteral("–"), this);
    m_counter->setObjectName(QStringLiteral("media-counter-tape"));
    m_counter->setMinimumWidth(m_counter->fontMetrics().horizontalAdvance(QStringLiteral("● 00:00 ")));
    l->addWidget(m_counter);
    connect(m_save, &QPushButton::clicked, this, &TapeControl::saveTape);
    refresh();
}

void TapeControl::refresh() {
    MediaPlaceControl::refresh();
    m_save->setEnabled(true);   // Save starts a new tape; always there while the cassette is
    refreshLamps();
}

void TapeControl::refreshLamps() {
    const shared::LampsPayload lamps = m_controller.lamps();
    const auto& t = lamps.tape;
    const int seconds = static_cast<int>(t.mode == TapeDeck::Mode::Record ? t.length : t.position);
    const QString time = QStringLiteral("%1:%2").arg(seconds / 60, 2, 10, QLatin1Char('0')).arg(seconds % 60, 2, 10, QLatin1Char('0'));
    const QString mark = t.mode == TapeDeck::Mode::Record ? QStringLiteral("●") : t.mode == TapeDeck::Mode::Play ? QStringLiteral("▶") : QString();
    m_counter->setText(t.mode == TapeDeck::Mode::Empty ? QStringLiteral("–") : (mark + QLatin1Char(' ') + time));
    m_counter->setToolTip(QString::fromStdString(lamps.tapeName));
}

// Save: a new tape to record onto (CSAVE), named first (LIB-7 AC-20, D-24:
// `media.start-new` on the empty tape). A tape of that name: asked before it
// is replaced.
void TapeControl::saveTape() {
    bool ok = false;
    const QString name =
        QInputDialog::getText(window(), tr("Save Tape"), tr("Tape name:"), QLineEdit::Normal, QString(), &ok).trimmed();
    if (!ok) return;
    std::string empty;   // the bundled empty tape's entry
    if (const calcu::media::MediaState* s = m_controller.media())
        if (const calcu::media::PlaceState* p = s->place("tape"))
            for (const auto& e : p->picker)
                if (e.group == calcu::media::PickerEntry::Group::BundledTemplate && e.label == "Empty") empty = e.id;
    sessions::Value::Object args{{"place", sessions::Value("tape")},
                                 {"template", sessions::Value(empty)},
                                 {"name", sessions::Value(name.toStdString())}};
    sessions::ReplyMessage r = m_controller.sendAndWait(calcu::media::kStartNewCommand, sessions::Value(args));
    if (r.done) return;
    const QString error = QString::fromStdString(r.reason);
    const QString asks = QString::fromLatin1(calcu::media::kAsks);
    if (!error.startsWith(asks)) {
        QMessageBox::warning(window(), tr("Save Tape"), error);
        return;
    }
    if (QMessageBox::question(window(), tr("Save Tape"), error.mid(asks.size())) != QMessageBox::Yes) return;
    args.push_back({"confirmed", sessions::Value(true)});
    r = m_controller.sendAndWait(calcu::media::kStartNewCommand, sessions::Value(std::move(args)));
    if (!r.done) QMessageBox::warning(window(), tr("Save Tape"), QString::fromStdString(r.reason));
}
