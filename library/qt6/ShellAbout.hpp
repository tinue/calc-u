#pragma once
#include "shell/ShellProduct.hpp"
#include <QDialog>

// The About dialog of the Qt6 shell: the product's name, its version and
// build, the texts and links the product gives, one button to close it.
// Window-modal through open(). Object name `about-dialog`.

namespace calcu::qt6 {

class ShellAbout : public QDialog {
    Q_OBJECT
public:
    ShellAbout(const shell::ShellProduct&, QWidget* parent);
};

} // namespace calcu::qt6
