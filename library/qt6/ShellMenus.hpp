#pragma once
#include "shell/MenuModel.hpp"
#include <QKeySequence>
#include <QObject>
#include <map>
#include <string>
#include <vector>

class QAction;
class QMenu;
class QMenuBar;

// The menu bar of the Qt6 shell: QMenuBar from the menu model; a menu whose
// model changed is filled anew in place, the others are left as they are.
// The model's roles (Preferences, About, Quit) as Qt's menu roles, every
// other item without one; the model's shortcuts as window shortcuts; the
// reason of a disabled item as its tooltip. Object names from
// shell/ObjectNames.hpp.

namespace calcu::qt6 {

class Shell;

class ShellMenus : public QObject {
    Q_OBJECT
public:
    ShellMenus(QMenuBar&, Shell&);
    void set(const std::vector<shell::Menu>&);
    QMenu* menu(const std::string& id) const;     // "file", …
    QAction* action(const std::string& itemId) const;
    int rebuilds() const { return rebuilds_; }    // menus filled anew (in place, the window stays)

private:
    void fill(QMenu&, const std::vector<shell::MenuItem>&);
    QMenuBar& bar_;
    Shell& shell_;
    std::vector<std::string> order_;
    std::map<std::string, QMenu*> menus_;
    std::map<std::string, std::string> shown_;   // a menu's model as text
    int rebuilds_ = 0;
};

} // namespace calcu::qt6
