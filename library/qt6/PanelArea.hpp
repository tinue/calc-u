#pragma once
#include "sessions/Messages.hpp"
#include "qt6/ShellControlBar.hpp"
#include "shell/BarModel.hpp"
#include "settings/SettingsFile.hpp"
#include "shell/Chrome.hpp"
#include <QString>
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
// On a desktop window the paper panel may live in a window of its own
// (setPaperOwnWindow, its button on the panel): the same panel, moved, so
// it keeps its paper; closing that window puts it back. The product's
// layout follows paperOwnWindowChanged.

namespace calcu::qt6 {

class PaperWindow;
class SessionHost;

class PanelArea : public QWidget {
    Q_OBJECT
public:
    using Factory = std::function<QWidget*(QWidget* parent)>;
    PanelArea(SessionHost&, QWidget* parent = nullptr);
    ~PanelArea() override;

    void set(const std::vector<shell::Panel>&);
    void setChoose(Choose c) { choose_ = std::move(c); }   // for the place controls on the paper
    void registerPanel(std::string kind, Factory);
    void addPanel(QWidget*);   // a product's panel, from its own product entry
    QWidget* panel(const std::string& kind) const;
    // A listed panel the product's view leaves out for now (Calc-U-1600's
    // paper without a plotter in a narrow window); shown again with true.
    void setShown(const std::string& kind, bool shown);
    void deliver(const sessions::Message&);   // to the panels that read topics (the paper panel)
    // The product's chrome (alive as long as the area): the paper panel
    // in it, light or dark with the window.
    void setChrome(const shell::Chrome*);

    // The paper's own window (the shell offers it on a desktop window):
    // its place in `file`'s [window] keys (none: not remembered), `title`
    // the window's title. Shows the paper panel's button.
    void offerPaperOwnWindow(settings::SettingsFile* file, QString title);
    void setPaperOwnWindow(bool own);
    bool paperOwnWindow() const { return paperWindow_ != nullptr; }
    PaperWindow* paperWindow() const { return paperWindow_; }
    // The main window closes: the paper's window goes too, still its own
    // (the setting is kept for the next start).
    void closePaperWindow();

signals:
    void paperOwnWindowChanged(bool own);

protected:
    void changeEvent(QEvent*) override;

private:
    void styleChrome();

    SessionHost& host_;
    QBoxLayout* layout_;
    std::map<std::string, Factory> factories_;
    std::map<std::string, QWidget*> panels_;
    std::map<std::string, bool> left_;   // setShown(kind, false)
    Choose choose_;
    const shell::Chrome* chrome_ = nullptr;
    std::vector<shell::BarControl> placesShown_;   // the paper's place controls as last built
    bool ownWindowOffered_ = false;
    settings::SettingsFile* windowFile_ = nullptr;
    QString windowTitle_;
    PaperWindow* paperWindow_ = nullptr;   // while the paper is in its own window
    int paperIndex_ = -1;                  // its place in the layout before it went
};

} // namespace calcu::qt6
