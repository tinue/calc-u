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

// The menu bar of the Qt6 shell (LIB-14 design §3.8, "Menus"; spec AC-1 to
// AC-7, EC-4): QMenuBar from the menu model; a menu whose model changed is
// filled anew in place, the others are left as they are. Roles per M2, M8,
// M12 (AC-6), shortcuts per AC-4 and D-14 as window shortcuts, the reason of
// a disabled item as its tooltip (AC-5). Object names of §3.10.

namespace calcu::qt6 {

class Shell;

class ShellMenus : public QObject {
    Q_OBJECT
public:
    ShellMenus(QMenuBar&, Shell&);
    void set(const std::vector<shell::Menu>&);
    QMenu* menu(const std::string& id) const;     // "file", …
    QAction* action(const std::string& itemId) const;
    int rebuilds() const { return rebuilds_; }    // menus filled anew (EC-4: in place, the window stays)

private:
    void fill(QMenu&, const std::vector<shell::MenuItem>&);
    QMenuBar& bar_;
    Shell& shell_;
    std::vector<std::string> order_;
    std::map<std::string, QMenu*> menus_;
    std::map<std::string, std::string> shown_;   // a menu's model as text
    int rebuilds_ = 0;
};

// The key sequence of a shortcut of the model (AC-4, D-14).
QKeySequence keySequence(shell::MenuItem::Shortcut);

} // namespace calcu::qt6
