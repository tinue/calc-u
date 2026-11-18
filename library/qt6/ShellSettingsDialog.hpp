#pragma once
#include "shell/SettingsModel.hpp"
#include <QDialog>
#include <map>
#include <string>
#include <vector>

class QVBoxLayout;
class QWidget;

// The settings dialog of the Qt6 shell: titled "Settings", a group per
// section of the dialog model, one widget per row named
// `setting-<section>-<key>`, Close as its only button. A change is sent as
// `set-setting` at once; the dialog follows the `settings` topic, in place
// while its rows stay the same. Window-modal through open(); the busy topic
// holds nothing back.

namespace calcu::qt6 {

class Shell;

class ShellSettingsDialog : public QDialog {
    Q_OBJECT
public:
    ShellSettingsDialog(Shell&, QWidget* parent);
    void refresh();   // from the shell's inputs (the `settings` topic, the preset list)
    const std::vector<shell::ShownSection>& shown() const { return shown_; }

private:
    void rebuild();
    void update();
    void send(const shell::DialogRow&, const QString& value, bool null = false);
    QWidget* rowWidget(const shell::ShownRow&, QWidget* parent);

    Shell& shell_;
    QWidget* content_ = nullptr;
    QVBoxLayout* contentLayout_ = nullptr;
    std::vector<shell::ShownSection> shown_;
    std::string structure_;
};

} // namespace calcu::qt6
