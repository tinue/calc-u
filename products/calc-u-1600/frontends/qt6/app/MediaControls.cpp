#include "MediaControls.hpp"
#include "MachineController.hpp"
#include "app/SharpChrome.hpp"
#include "app/SharpControlRules.hpp"
#include "control/ControlState.hpp"
#include "media/MediaTopics.hpp"
#include "qt6/ChromeStyle.hpp"
#include "qt6/MediaPicker.hpp"
#include "qt6/SessionHost.hpp"
#include "qt6/Shell.hpp"
#include "qt6/ShellControlBar.hpp"
#include "shell/Inputs.hpp"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>

namespace sessions = calcu::sessions;

namespace {

// "Disk:", "Cassette:" before the picker, left out where the bar would not
// fit the faceplate.
void addLabel(QHBoxLayout* row, const QString& text, QWidget* parent) {
    auto* label = new QLabel(text, parent);
    row->insertWidget(0, label);
    calcu::qt6::barLabel(label);
}

void lampStyle(QLabel* lamp, bool on) {
    namespace ch = calcu::qt6::chrome;
    const calcu::shell::ChromeColours& c = cu1600::sharpChrome().colours(ch::darkAppearance(lamp));
    lamp->setStyleSheet(QStringLiteral("color: %1;").arg(ch::qcss(c.named(on ? cu1600::chrome::kLampOn : cu1600::chrome::kLampOff))));
}

} // namespace

DiskControl::DiskControl(MachineController& c, calcu::qt6::Shell& shell, QWidget* parent)
    : MediaPlaceControl(shell, "floppy", parent), m_controller(c) {
    setObjectName(QStringLiteral("bar-disk"));
    addLabel(row(), tr("Disk:"), this);
    picker()->setMinimumContentsLength(9);
    m_side = new QPushButton(QStringLiteral("0"), this);
    m_side->setFocusPolicy(Qt::NoFocus);
    m_side->setToolTip(tr("Eject and turn the disk over"));
    m_side->setObjectName(QStringLiteral("media-side-floppy"));
    m_side->setFixedWidth(m_side->fontMetrics().horizontalAdvance(QStringLiteral("00")) + 16);
    row()->addWidget(m_side);
    addSaveButton();
    m_lamp = new QLabel(QStringLiteral("●"), this);
    m_lamp->setObjectName(QStringLiteral("media-lamp-floppy"));
    m_lamp->setFixedWidth(14);
    m_lamp->setAlignment(Qt::AlignCenter);
    m_lamp->setToolTip(tr("Drive active -- wait for this to go dark before turning the disk over"));
    row()->addWidget(m_lamp);
    connect(m_side, &QPushButton::clicked, this,
            [this] { shell_.sendChecked(calcu::media::kTurnCommand, sessions::Value::object({{"place", place()}})); });
    connect(&c.host(), &calcu::qt6::SessionHost::topicChanged, this, [this](const QString& topic) {
        if (topic == QLatin1String("control-state")) refresh();   // the product entry
        else if (topic == QLatin1String("c1600.lamps")) refreshLamp();
    });
    refresh();
}

void DiskControl::refresh() {
    MediaPlaceControl::refresh();
    const auto* cs = m_controller.state<calcu::control::ControlState>(calcu::control::kControlStateTopic);
    const calcu::control::ProductEntry* e = cs ? cs->productEntry(cu1600::kDiskControlsEntry) : nullptr;
    const sessions::Value v = e && e->value ? e->value->toValue() : sessions::Value();
    setVisible(calcu::shell::read::flag(v, "shown"));
    const calcu::media::PlaceState* p = placeState();
    m_side->setText(QString::number(p && p->holds && p->holds->side ? *p->holds->side : 0));
    refreshLamp();
}

void DiskControl::refreshLamp() { lampStyle(m_lamp, m_controller.lamps().floppyMotor); }

TapeControl::TapeControl(MachineController& c, calcu::qt6::Shell& shell, QWidget* parent)
    : MediaPlaceControl(shell, "cassette", parent), m_controller(c) {
    addLabel(row(), tr("Cassette:"), this);
    picker()->setMinimumContentsLength(9);
    picker()->setToolTip(tr("A cassette to play (CLOAD)"));
    // The blank tape records into memory; Name & Save keeps it under a name.
    addSaveButton();
    m_counter = new QLabel(this);
    m_counter->setObjectName(QStringLiteral("media-counter-tape"));
    // As wide as its widest text, whatever it shows: no control moves.
    m_counter->setFixedWidth(m_counter->fontMetrics().horizontalAdvance(QStringLiteral("● 00:00 ")));
    m_counter->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    row()->addWidget(m_counter);
    connect(&c.host(), &calcu::qt6::SessionHost::topicChanged, this, [this](const QString& topic) {
        if (topic == QLatin1String("c1600.lamps")) refreshCounter();
    });
    refresh();
}

void TapeControl::refresh() {
    MediaPlaceControl::refresh();
    refreshCounter();
}

void TapeControl::refreshCounter() {
    const cu1600::LampsPayload lamps = m_controller.lamps();
    m_counter->setText(QString::fromStdString(lamps.tapeCounter()));
    m_counter->setToolTip(QString::fromStdString(lamps.tapeTip()));
    m_counter->setEnabled(!lamps.tapeEmpty());   // dimmed without a cassette
}
