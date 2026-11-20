#include "qt6/MediaPicker.hpp"
#include <QStandardItemModel>

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
    auto* model = qobject_cast<QStandardItemModel*>(this->model());
    int current = 0;
    bool listed = !p.holds;
    for (const media::PickerEntry& e : p.picker) {
        addItem(e.group == media::PickerEntry::Group::Empty ? emptyLabel() : QString::fromStdString(e.label));
        const int i = count() - 1;
        ids_.push_back(e.id);
        const std::optional<std::string>& tip = e.canChoose ? e.tooltip : e.hint;
        if (tip) setItemData(i, QString::fromStdString(*tip), Qt::ToolTipRole);
        if (model && !e.canChoose) model->item(i)->setEnabled(false);
        const bool held = p.holds ? (e.group != media::PickerEntry::Group::Empty && e.label == p.holds->name)
                                  : e.group == media::PickerEntry::Group::Empty;
        if (held && e.canChoose) {
            current = i;
            listed = true;
        }
    }
    // A medium the place holds that no entry lists (a file opened from
    // where it lies; a tape started under a name before its first write):
    // shown as what the place holds, not a choice.
    if (!listed) {
        addItem(QString::fromStdString(p.holds->name));
        ids_.push_back(std::string());
        current = count() - 1;
        if (model) model->item(current)->setEnabled(false);
    }
    setCurrentIndex(current);
    filling_ = false;
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
