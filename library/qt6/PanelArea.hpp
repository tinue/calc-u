#pragma once
#include "sessions/Messages.hpp"
#include "qt6/ShellControlBar.hpp"
#include "shell/BarModel.hpp"
#include "shell/Chrome.hpp"
#include <QWidget>
#include <functional>
#include <map>
#include <string>
#include <vector>

class QBoxLayout;

// The area for panels of the Qt6 shell: the panels the control state lists,
// each made once by its factory (the library's paper panel by default) and
// kept, so that a change of owner keeps their content; a product puts its
// own panels into the same area (addPanel). A QWidget with a box layout the
// product may replace (Calc-U-59's splitter). Object name `panel-area`.

namespace calcu::qt6 {

class SessionHost;

class PanelArea : public QWidget {
    Q_OBJECT
public:
    using Factory = std::function<QWidget*(QWidget* parent)>;
    PanelArea(SessionHost&, QWidget* parent = nullptr);

    void set(const std::vector<shell::Panel>&);
    void setChoose(Choose c) { choose_ = std::move(c); }   // for the place controls on the paper
    void registerPanel(std::string kind, Factory);
    void addPanel(QWidget*);   // a product's panel, from its own product entry
    QWidget* panel(const std::string& kind) const;
    void deliver(const sessions::Message&);   // to the panels that read topics (the paper panel)
    // The product's chrome (alive as long as the area): the paper panel
    // in it, light or dark with the window.
    void setChrome(const shell::Chrome*);

protected:
    void changeEvent(QEvent*) override;

private:
    void styleChrome();

    SessionHost& host_;
    QBoxLayout* layout_;
    std::map<std::string, Factory> factories_;
    std::map<std::string, QWidget*> panels_;
    Choose choose_;
    const shell::Chrome* chrome_ = nullptr;
    std::vector<shell::BarControl> placesShown_;   // the paper's place controls as last built
};

} // namespace calcu::qt6
