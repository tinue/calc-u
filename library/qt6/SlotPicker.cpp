#include "qt6/SlotPicker.hpp"
#include "media/MediaTopics.hpp"
#include "qt6/MediaPicker.hpp"
#include "qt6/NameAndSaveDialog.hpp"
#include "qt6/SessionHost.hpp"
#include "shell/Inputs.hpp"
#include <QEventLoop>
#include <QFontInfo>
#include <QHBoxLayout>
#include <QPushButton>
#include <QStyle>
#include <optional>

namespace calcu::qt6 {

namespace {

const media::PlaceState* placeOf(const SessionHost& host, const std::string& place) {
    const auto* s = host.topics().stateOf<media::MediaState>(media::kMediaTopic);
    return s ? s->place(place) : nullptr;
}

} // namespace

void nameAndSave(SessionHost& host, QWidget* parent, const std::string& place, const QString& prompt) {
    const media::PlaceState* p = placeOf(host, place);
    if (!p) return;
    NameAndSaveDialog dialog(parent);
    dialog.setPlace(*p);
    dialog.setPrompt(prompt);
    dialog.setSender([&host](const sessions::CommandName& name, sessions::Value args) -> std::string {
        QEventLoop loop;
        std::optional<sessions::ReplyMessage> got;
        host.send(name, std::move(args), [&](const sessions::ReplyMessage& r) {
            got = r;
            loop.quit();
        });
        if (!got) loop.exec();
        return got->done ? std::string() : got->reason;
    });
    dialog.exec();
}

SlotPicker::SlotPicker(SessionHost& host, const sessions::Value& entry, const SlotPickerOptions& options, double widthEm,
                       Send send, QWidget* parent)
    : QWidget(parent), host_(host), place_(shell::read::text(entry, "place")), options_(options),
      picker_(new MediaPicker(this)) {
    auto* l = new QHBoxLayout(this);
    l->setContentsMargins(0, 0, 0, 0);
    l->setSpacing(2);
    l->addWidget(picker_);
    picker_->setSender(std::move(send));
    if (widthEm > 0) picker_->setFixedWidth(qRound(widthEm * QFontInfo(picker_->font()).pixelSize()));
    if (options_.nameAndSave) {
        save_ = new QPushButton(this);
        save_->setIcon(style()->standardIcon(QStyle::SP_DialogSaveButton));
        save_->setToolTip(QStringLiteral("Name & Save"));
        save_->setFocusPolicy(Qt::NoFocus);
        save_->setFixedWidth(save_->iconSize().width() + 12);
        save_->setObjectName(QString::fromStdString("media-save-" + place_));
        l->addWidget(save_);
        connect(save_, &QPushButton::clicked, this, [this] { nameAndSave(host_, window(), place_, options_.prompt); });
    }
    const shell::Availability a = shell::availabilityOf(entry, "empty");
    setEnabled(a.now);
    if (!a.now) setToolTip(QString::fromStdString(a.reason));
    connect(&host_, &SessionHost::topicChanged, this, [this](const QString& topic) {
        if (topic.toStdString() == media::kMediaTopic.value) refresh();
    });
    refresh();
}

void SlotPicker::refresh() {
    const media::PlaceState* p = placeOf(host_, place_);
    media::PlaceState none;
    none.place = place_;
    picker_->setPlace(p ? *p : none);
    if (save_) save_->setEnabled(p && p->nameAndSave);
}

} // namespace calcu::qt6
