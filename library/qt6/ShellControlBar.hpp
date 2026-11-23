#pragma once
#include "shell/Chrome.hpp"
#include "shell/BarModel.hpp"
#include "qt6/SlotPicker.hpp"
#include <QWidget>
#include <array>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <vector>

class QComboBox;
class QHBoxLayout;
class QMenu;
class QToolButton;

// The control bar of the Qt6 shell: one container per group, left to right;
// QComboBox pickers whose disabled items carry the reason as their tooltip;
// checkable QToolButtons for the one-peripheral places; a module slot's
// picker (SlotPicker); the media owners' controls from registered factories;
// the product's own controls in the group it names; at the end the More button,
// a menu of the bar model's More items. A group without a control
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
    void addProductControl(shell::BarGroup, QWidget*);   // at the end of the group
    void setMediaFactory(std::string kind, Factory f) {
        mediaFactories_[std::move(kind)] = std::move(f);
        shown_.reset();
    }
    QWidget* group(shell::BarGroup g) const { return groups_[static_cast<std::size_t>(g)]; }
    // The product's chrome: More is a pill of it, light or dark with the window.
    void setChrome(const shell::Chrome*);

protected:
    void changeEvent(QEvent*) override;


private:
    QWidget* build(const shell::BarControl&, QWidget* parent);
    Choose choose() { return [this](const shell::MenuItem& i) { chooseItem(i); }; }
    void chooseItem(const shell::MenuItem&);
    void chooseMore(const shell::MenuItem&);   // an item with `confirm` asks first
    QWidget* moreButton(const shell::BarControl&, QWidget* parent);
    void fillMoreMenu();
    void styleMore();
    void layoutGroup(shell::BarGroup);

    Shell& shell_;
    QHBoxLayout* row_ = nullptr;
    static constexpr std::size_t kGroups = 5;   // one per shell::BarGroup
    std::array<QWidget*, kGroups> groups_{};
    std::array<std::vector<std::pair<std::string, QWidget*>>, kGroups> shellWidgets_;
    std::array<std::vector<QWidget*>, kGroups> productWidgets_;
    std::optional<std::vector<shell::BarGroupModel>> shown_;   // the model the widgets were built of
    std::map<std::string, Factory> mediaFactories_;
    const shell::Chrome* chrome_ = nullptr;
    QToolButton* more_ = nullptr;
    QMenu* moreMenu_ = nullptr;
    std::vector<shell::MenuItem> moreItems_;
    std::optional<std::vector<shell::MenuItem>> moreShown_;   // the items the menu was filled with
};

} // namespace calcu::qt6
