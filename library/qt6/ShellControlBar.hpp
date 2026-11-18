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
// is hidden: no gap.

namespace calcu::qt6 {

class Shell;

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
    QComboBox* picker(const std::vector<shell::MenuItem>& choices, QWidget* parent);
    void layoutGroup(shell::BarGroup);

    Shell& shell_;
    std::array<QWidget*, 6> groups_{};   // one per shell::BarGroup, Start first
    std::array<std::vector<std::pair<std::string, QWidget*>>, 6> shellWidgets_;
    std::array<std::vector<Product>, 6> productWidgets_;
    std::string shown_;
    Factory slotFactory_;
    std::map<std::string, Factory> mediaFactories_;
};

} // namespace calcu::qt6
