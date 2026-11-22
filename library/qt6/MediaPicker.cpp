#include "qt6/MediaPicker.hpp"
#include <QStandardItemModel>
#include <QStyleOptionComboBox>
#include <QStylePainter>

namespace calcu::qt6 {

MediaPicker::MediaPicker(QWidget* parent) : QComboBox(parent) {
    setFocusPolicy(Qt::NoFocus);
    connect(this, qOverload<int>(&QComboBox::activated), this, [this](int i) { chosen(i); });
}

void MediaPicker::setPlace(const media::PlaceState& p) {
    filling_ = true;
    place_ = p.place;
    setObjectName(QString::fromStdString("media-picker-" + p.place));
    clear();
    ids_.clear();
    shorts_.clear();
    auto* model = qobject_cast<QStandardItemModel*>(this->model());
    int current = 0;
    for (const media::PickerEntry& e : p.picker) {
        addItem(e.group == media::PickerEntry::Group::Empty ? emptyLabel() : QString::fromStdString(e.label));
        const int i = count() - 1;
        ids_.push_back(e.id);
        shorts_.push_back(QString::fromStdString(media::shortLabel(e)));
        const std::optional<std::string>& tip = e.canChoose ? e.tooltip : e.hint;
        if (tip) setItemData(i, QString::fromStdString(*tip), Qt::ToolTipRole);
        if (model && !e.canChoose) model->item(i)->setEnabled(false);
        if (e.chosen) current = i;   // the topic's (a held medium no entry lists: its own entry)
    }
    setCurrentIndex(current);
    filling_ = false;
    updateGeometry();   // the short label's width
}

void MediaPicker::setShowShort(bool on) {
    showShort_ = on;
    updateGeometry();
    update();
}

QString MediaPicker::closedText() const {
    const int i = currentIndex();
    return i >= 0 && static_cast<std::size_t>(i) < shorts_.size() ? shorts_[static_cast<std::size_t>(i)] : currentText();
}

QSize MediaPicker::sizeHint() const {
    if (!showShort_) return QComboBox::sizeHint();
    QStyleOptionComboBox opt;
    initStyleOption(&opt);
    const QSize text = fontMetrics().size(Qt::TextSingleLine, closedText());
    return style()->sizeFromContents(QStyle::CT_ComboBox, &opt, text, this);
}

QSize MediaPicker::minimumSizeHint() const { return showShort_ ? sizeHint() : QComboBox::minimumSizeHint(); }

void MediaPicker::paintEvent(QPaintEvent* e) {
    if (!showShort_) {
        QComboBox::paintEvent(e);
        return;
    }
    QStylePainter p(this);
    QStyleOptionComboBox opt;
    initStyleOption(&opt);
    opt.currentText = closedText();
    p.drawComplexControl(QStyle::CC_ComboBox, opt);
    p.drawControl(QStyle::CE_ComboBoxLabel, opt);
}

QString MediaPicker::entryLabel(int i) const { return itemText(i); }

QString MediaPicker::entryTooltip(int i) const { return itemData(i, Qt::ToolTipRole).toString(); }

bool MediaPicker::entryEnabled(int i) const {
    const auto* model = qobject_cast<const QStandardItemModel*>(this->model());
    return model && model->item(i) && model->item(i)->isEnabled();
}

std::string MediaPicker::entryId(int i) const {
    return i >= 0 && static_cast<std::size_t>(i) < ids_.size() ? ids_[static_cast<std::size_t>(i)] : std::string();
}

void MediaPicker::chosen(int index) {
    if (filling_ || !sender_ || !entryEnabled(index)) return;
    sender_(media::kChooseCommand, sessions::Value::object({{"place", place_}, {"entry", entryId(index)}}));
}

} // namespace calcu::qt6
