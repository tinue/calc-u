#include "MediaControls.hpp"
#include "app/SharpChrome.hpp"
#include "qt6/ChromeStyle.hpp"

#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QStyle>

#include "MachineController.hpp"
#include "app/SharpControlRules.hpp"
#include "control/ControlState.hpp"
#include "media/MediaTopics.hpp"
#include "qt6/MediaPicker.hpp"
#include "qt6/Shell.hpp"
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
    namespace ch = calcu::qt6::chrome;
    const calcu::shell::ChromeColours& c = cu1600::sharpChrome().colours(ch::darkAppearance(lamp));
    lamp->setStyleSheet(QStringLiteral("color: %1;").arg(ch::qcss(c.named(on ? cu1600::chrome::kLampOn : cu1600::chrome::kLampOff))));
}

} // namespace

MediaPlaceControl::MediaPlaceControl(MachineController& c, calcu::qt6::Shell& shell, std::string place, QWidget* parent)
    : QWidget(parent), m_controller(c), m_shell(shell), m_place(std::move(place)) {
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

void MediaPlaceControl::nameAndSave() { m_shell.nameAndSave(m_place); }

DiskControl::DiskControl(MachineController& c, calcu::qt6::Shell& shell, QWidget* parent)
    : MediaPlaceControl(c, shell, "floppy", parent) {
    setObjectName(QStringLiteral("bar-disk"));
    auto* l = new QHBoxLayout(this);
    l->setContentsMargins(0, 0, 0, 0);
    l->setSpacing(2);
    l->addWidget(new QLabel(tr("Disk:"), this));
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
    connect(m_save, &QPushButton::clicked, this, [this] { nameAndSave(); });
    refresh();
}

void DiskControl::refresh() {
    MediaPlaceControl::refresh();
    // The product entry.
    const auto* cs = m_controller.state<calcu::control::ControlState>(calcu::control::kControlStateTopic);
    const calcu::control::ProductEntry* e = cs ? cs->productEntry(cu1600::kDiskControlsEntry) : nullptr;
    const sessions::Value v = e && e->value ? e->value->toValue() : sessions::Value();
    setVisible(calcu::shell::read::flag(v, "shown"));
    const calcu::media::MediaState* s = m_controller.media();
    const calcu::media::PlaceState* p = s ? s->place("floppy") : nullptr;
    m_side->setText(QString::number(p && p->holds && p->holds->side ? *p->holds->side : 0));
    refreshLamps();
}

void DiskControl::refreshLamps() { lampStyle(m_lamp, m_controller.lamps().floppyMotor); }

TapeControl::TapeControl(MachineController& c, calcu::qt6::Shell& shell, QWidget* parent)
    : MediaPlaceControl(c, shell, "cassette", parent) {
    auto* l = new QHBoxLayout(this);
    l->setContentsMargins(0, 0, 0, 0);
    l->setSpacing(2);
    l->addWidget(new QLabel(tr("Cassette:"), this));
    m_picker->setToolTip(tr("A cassette to play (CLOAD)"));
    l->addWidget(m_picker);
    // The blank tape records into memory; Name & Save keeps it under a name.
    m_save = compactButton(this, tr("Name & Save"), QStringLiteral("media-save-cassette"));
    l->addWidget(m_save);
    m_counter = new QLabel(this);
    m_counter->setObjectName(QStringLiteral("media-counter-tape"));
    // As wide as its widest text, whatever it shows: no control moves.
    m_counter->setFixedWidth(m_counter->fontMetrics().horizontalAdvance(QStringLiteral("\u25CF 00:00 ")));
    m_counter->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    l->addWidget(m_counter);
    connect(m_save, &QPushButton::clicked, this, [this] { nameAndSave(); });
    refresh();
}

void TapeControl::refresh() {
    MediaPlaceControl::refresh();
    refreshLamps();
}

void TapeControl::refreshLamps() {
    const cu1600::LampsPayload lamps = m_controller.lamps();
    m_counter->setText(QString::fromStdString(lamps.tapeCounter()));
    m_counter->setToolTip(QString::fromStdString(lamps.tapeTip()));
    m_counter->setEnabled(!lamps.tapeEmpty());   // dimmed without a cassette
}
