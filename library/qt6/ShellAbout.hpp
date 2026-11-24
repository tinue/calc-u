#pragma once
#include "shell/ShellProduct.hpp"
#include <QDialog>

// The About dialog of the Qt6 shell: the product's name, its version and
// build, the texts and links the product gives, its sections (`about-section`:
// Support Information with build time, OS, hardware; Display of the screen
// the parent window is on), Copy (`about-copy`: shell::aboutText) and OK.
// Window-modal through open(). Object name `about-dialog`.

namespace calcu::qt6 {

class ShellAbout : public QDialog {
    Q_OBJECT
public:
    ShellAbout(const shell::ShellProduct&, QWidget* parent);
};

} // namespace calcu::qt6
