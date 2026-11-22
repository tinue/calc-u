#pragma once
#include "shell/Chrome.hpp"
#include <QColor>
#include <QString>

class QWidget;

// The Qt6 component set's stylesheets over a product's chrome
// (shell/Chrome): thin functions, so that every panel's header, button bar
// and pill is built from the same values.

namespace calcu::qt6::chrome {

QColor qcolour(const shell::Colour&);
QString qcss(const shell::Colour&);   // "rgba(r,g,b,a)" for a stylesheet

// The window's appearance: dark when its palette's window colour is.
bool darkAppearance(const QWidget*);

// A panel header's declarations: its colours and its height.
QString header(const shell::ChromeColours&, const shell::ChromeSizes&);
// A panel button bar's declarations: its colour and its height.
QString buttonBar(const shell::ChromeColours&, const shell::ChromeSizes&);
// The pill's core declarations (shape, font, colours) for a selector of
// the caller's.
QString pillCore(const shell::ChromeColours&, const shell::ChromeSizes&, const shell::Colour& background);
// Pills of `selector` ("QPushButton", "QToolButton") with hover, pressed,
// checked (pill-on where the product has it, else pressed) and disabled.
QString pills(const shell::ChromeColours&, const shell::ChromeSizes&, const QString& selector);

} // namespace calcu::qt6::chrome
