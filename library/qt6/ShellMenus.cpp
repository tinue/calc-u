#include "qt6/ShellMenus.hpp"
#include "qt6/Shell.hpp"
#include "shell/ObjectNames.hpp"
#include <QAction>
#include <QKeySequence>
#include <QMenu>
#include <QMenuBar>

namespace calcu::qt6 {

namespace {

using shell::MenuItem;

QAction::MenuRole role(MenuItem::Role r) {
    switch (r) {
    case MenuItem::Role::Preferences: return QAction::PreferencesRole;
    case MenuItem::Role::About: return QAction::AboutRole;
    case MenuItem::Role::Quit: return QAction::QuitRole;
    case MenuItem::Role::None: break;
    }
    return QAction::NoRole;   // Qt's heuristic moves nothing else
}

// The key sequence of a shortcut of the model.
QKeySequence keySequence(MenuItem::Shortcut s) {
    switch (s) {
    case MenuItem::Shortcut::LoadPreset: return QKeySequence(Qt::CTRL | Qt::Key_O);   // CTRL: Command on macOS
    case MenuItem::Shortcut::Reset: return QKeySequence(Qt::CTRL | Qt::Key_R);
    case MenuItem::Shortcut::ColdBoot: return QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_R);
    case MenuItem::Shortcut::Preferences: return QKeySequence(QKeySequence::Preferences);
    case MenuItem::Shortcut::Quit: return QKeySequence(QKeySequence::Quit);
    case MenuItem::Shortcut::Copy: return QKeySequence(QKeySequence::Copy);
    case MenuItem::Shortcut::Paste: return QKeySequence(QKeySequence::Paste);
    case MenuItem::Shortcut::None: break;
    }
    return QKeySequence();
}

} // namespace

ShellMenus::ShellMenus(QMenuBar& bar, Shell& shell) : QObject(&bar), bar_(bar), shell_(shell) {
    bar_.setObjectName(shell::names::kMenuBar);
}

void ShellMenus::set(const std::vector<shell::Menu>& model) {
    std::vector<std::string> order;
    for (const auto& m : model) order.push_back(m.id);
    if (order != order_) {   // a menu came or went: the bar anew
        bar_.clear();
        for (auto& [id, m] : menus_) delete m;
        menus_.clear();
        shown_.clear();
        for (const auto& m : model) {
            auto* menu = new QMenu(QString::fromStdString(m.label), &bar_);
            menu->setObjectName(QString::fromStdString(shell::names::menu(m.id)));
            bar_.addMenu(menu);
            menus_[m.id] = menu;
        }
        order_ = order;
    }
    for (const auto& m : model) {
        if (auto it = shown_.find(m.id); it != shown_.end() && it->second == m.items) continue;   // only what changed
        QMenu* menu = menus_[m.id];
        for (QMenu* sub : menu->findChildren<QMenu*>(QString(), Qt::FindDirectChildrenOnly)) delete sub;
        menu->clear();
        fill(*menu, m.items);
        shown_[m.id] = m.items;
        ++rebuilds_;
    }
}

void ShellMenus::fill(QMenu& menu, const std::vector<MenuItem>& items) {
    menu.setToolTipsVisible(true);   // a disabled item's reason
    for (const auto& item : items) {
        const QString name = QString::fromStdString(shell::names::item(item.id));
        if (item.kind == MenuItem::Kind::Separator) {
            menu.addSeparator();
            continue;
        }
        if (item.kind == MenuItem::Kind::Submenu) {
            auto* sub = new QMenu(QString::fromStdString(item.label), &menu);
            sub->setObjectName(name);
            sub->menuAction()->setObjectName(name + QStringLiteral("-menu"));
            sub->menuAction()->setMenuRole(QAction::NoRole);
            fill(*sub, item.children);
            menu.addMenu(sub);
            continue;
        }
        QAction* a = menu.addAction(QString::fromStdString(item.label));
        a->setObjectName(name);
        a->setMenuRole(role(item.role));
        a->setCheckable(item.checkable);
        a->setChecked(item.checked);
        a->setEnabled(item.enabled);
        if (!item.reason.empty()) a->setToolTip(QString::fromStdString(item.reason));
        if (item.shortcut != MenuItem::Shortcut::None) {
            a->setShortcut(keySequence(item.shortcut));
            a->setShortcutContext(Qt::WindowShortcut);   // before the focused widget's keyPressEvent
        }
        connect(a, &QAction::triggered, this, [this, item] { shell_.choose(item); });
    }
}

QMenu* ShellMenus::menu(const std::string& id) const {
    auto it = menus_.find(id);
    return it == menus_.end() ? nullptr : it->second;
}

QAction* ShellMenus::action(const std::string& itemId) const {
    return bar_.findChild<QAction*>(QString::fromStdString(shell::names::item(itemId)));
}

} // namespace calcu::qt6
