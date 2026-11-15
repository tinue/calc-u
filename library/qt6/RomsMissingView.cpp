#include "qt6/RomsMissingView.hpp"
#include "shell/ObjectNames.hpp"

namespace calcu::qt6 {

RomsMissingView::RomsMissingView(QWidget* parent) : QLabel(parent) {
    setObjectName(shell::names::kRomsMissing);
    setWordWrap(true);
    setTextInteractionFlags(Qt::TextSelectableByMouse);
    setAlignment(Qt::AlignCenter);
    setMargin(24);
    hide();
}

void RomsMissingView::setMessage(const QString& text) { setText(text); }

} // namespace calcu::qt6
