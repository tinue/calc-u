#include "qt6/ShellLoading.hpp"
#include "shell/ObjectNames.hpp"
#include <QEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <algorithm>

namespace calcu::qt6 {

ShellLoading::ShellLoading(QWidget* over) : QWidget(over) {
    setObjectName(shell::names::kLoading);
    setAttribute(Qt::WA_TransparentForMouseEvents);   // takes no input (AC-16)
    setFocusPolicy(Qt::NoFocus);
    setAutoFillBackground(true);
    auto* l = new QHBoxLayout(this);
    l->setContentsMargins(10, 6, 10, 6);
    text_ = new QLabel(QStringLiteral("Loading…"), this);
    busy_ = new QProgressBar(this);   // a sign that work is going on: no end, no button
    busy_->setRange(0, 0);
    busy_->setTextVisible(false);
    busy_->setMaximumWidth(80);
    l->addWidget(text_);
    l->addWidget(busy_);
    hide();
    if (over) over->installEventFilter(this);
}

void ShellLoading::place(std::optional<QRect> display) {
    display_ = display;
    adjustSize();
    QWidget* p = parentWidget();
    if (!p) return;
    const QSize s = sizeHint();
    const int x = display ? display->center().x() - s.width() / 2 : (p->width() - s.width()) / 2;
    const int y = display ? display->bottom() + 4 : (p->height() - s.height()) / 2;
    setGeometry(std::max(0, x), std::max(0, y), s.width(), s.height());
    raise();
}

bool ShellLoading::eventFilter(QObject* o, QEvent* e) {
    if (o == parentWidget() && e->type() == QEvent::Resize) place(display_);
    return false;
}

} // namespace calcu::qt6
