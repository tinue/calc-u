#include "qt6/MediaPlaceControl.hpp"
#include "qt6/MediaPicker.hpp"
#include "qt6/SessionHost.hpp"
#include "qt6/Shell.hpp"
#include <QHBoxLayout>
#include <QPushButton>
#include <QStyle>

namespace calcu::qt6 {

MediaPlaceControl::MediaPlaceControl(Shell& shell, std::string place, QWidget* parent)
    : QWidget(parent), shell_(shell), place_(std::move(place)), row_(new QHBoxLayout(this)),
      picker_(new MediaPicker(this)) {
    row_->setContentsMargins(0, 0, 0, 0);
    row_->setSpacing(2);
    row_->addWidget(picker_);
    picker_->setSender([this](const sessions::CommandName& name, sessions::Value args) {
        shell_.sendChecked(name, std::move(args));
    });
    connect(&shell_.host(), &SessionHost::topicChanged, this, [this](const QString& topic) {
        if (topic.toStdString() == media::kMediaTopic.value) refresh();
    });
}

const media::PlaceState* MediaPlaceControl::placeState() const {
    const auto* s = shell_.host().topics().stateOf<media::MediaState>(media::kMediaTopic);
    return s ? s->place(place_) : nullptr;
}

void MediaPlaceControl::addSaveButton() {
    save_ = new QPushButton(this);
    save_->setIcon(style()->standardIcon(QStyle::SP_DialogSaveButton));
    save_->setToolTip(QStringLiteral("Name & Save"));
    save_->setFocusPolicy(Qt::NoFocus);
    save_->setFixedWidth(save_->iconSize().width() + 12);
    save_->setObjectName(QString::fromStdString("media-save-" + place_));
    row_->addWidget(save_);
    connect(save_, &QPushButton::clicked, this, [this] { shell_.nameAndSave(place_); });
}

void MediaPlaceControl::refresh() {
    const media::PlaceState* p = placeState();
    media::PlaceState none;
    none.place = place_;
    picker_->setPlace(p ? *p : none);
    if (save_) save_->setEnabled(p && p->nameAndSave);
}

} // namespace calcu::qt6
