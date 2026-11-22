#pragma once
#include "shell/BarModel.hpp"
#include <QWidget>
#include <array>
#include <functional>
#include <map>
#include <string>
#include <vector>

class QComboBox;
class QHBoxLayout;

// The control bar of the Qt6 shell: one container per group, left to right;
// QComboBox pickers whose disabled items carry the reason as their tooltip;
// checkable QToolButtons for the one-peripheral places; the library's module
// picker and the media owners' controls from registered factories; the
// product's own controls in the group it names. A group without a control
// is hidden: no gap. A group the model pushes takes up the room before it
// (a spread bar); a picker the model shows short shows its short text
// (a slot's media picker: the entries' `short`).

namespace calcu::qt6 {

class Shell;

using Choose = std::function<void(const shell::MenuItem&)>;
// A picker of a control's choices; a disabled choice carries its reason as
// its tooltip; `choose` carries out the one picked.
QComboBox* shellPicker(const std::vector<shell::MenuItem>& choices, Choose choose, QWidget* parent);
// A place's control, on the bar or on the paper panel: a checkable button
// where one peripheral fits (checked while plugged in), a picker where
// several do. Object name bar-place-<place>.
QWidget* placeControl(const shell::BarControl&, Choose choose, QWidget* parent);

class ShellControlBar : public QWidget {
    Q_OBJECT
public:
    using Factory = std::function<QWidget*(const sessions::Value& entry, QWidget* parent)>;

    ShellControlBar(Shell&, QWidget* parent = nullptr);
    void set(const std::vector<shell::BarGroupModel>&);
    void addProductControl(shell::BarGroup, std::string after, QWidget*);
    void setSlotFactory(Factory f) {   // the next set() builds anew
        slotFactory_ = std::move(f);
        shown_.clear();
    }
    void setMediaFactory(std::string kind, Factory f) {
        mediaFactories_[std::move(kind)] = std::move(f);
        shown_.clear();
    }
    QWidget* group(shell::BarGroup g) const { return groups_[static_cast<std::size_t>(g)]; }


private:
    struct Product {
        std::string after;
        QWidget* widget;
    };
    QWidget* build(const shell::BarControl&, QWidget* parent);
    Choose choose() { return [this](const shell::MenuItem& i) { chooseItem(i); }; }
    void chooseItem(const shell::MenuItem&);
    void layoutGroup(shell::BarGroup);

    Shell& shell_;
    QHBoxLayout* row_ = nullptr;
    static constexpr std::size_t kGroups = 5;   // one per shell::BarGroup
    std::array<QWidget*, kGroups> groups_{};
    std::array<std::vector<std::pair<std::string, QWidget*>>, kGroups> shellWidgets_;
    std::array<std::vector<Product>, kGroups> productWidgets_;
    std::string shown_;
    Factory slotFactory_;
    std::map<std::string, Factory> mediaFactories_;
};

} // namespace calcu::qt6
