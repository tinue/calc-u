#include "qt6/PaperWindow.hpp"
#include "settings/WindowPlace.hpp"
#include <QCloseEvent>
#include <QGuiApplication>
#include <QScreen>
#include <QVBoxLayout>
#include <algorithm>

namespace calcu::qt6 {

PaperWindow::PaperWindow(settings::SettingsFile* file, const QString& title, const QWidget* mainWindow)
    : QWidget(nullptr, Qt::Window), file_(file) {
    setObjectName(QStringLiteral("paper-window"));
    setWindowTitle(title);
    setAttribute(Qt::WA_QuitOnClose, false);   // the main window's close ends the app
    layout_ = new QVBoxLayout(this);
    layout_->setContentsMargins(0, 0, 0, 0);
    restorePlace(mainWindow);
}

void PaperWindow::restorePlace(const QWidget* mainWindow) {
    settings::WindowPlace p{std::nullopt, std::nullopt, settings::kPaperWindowWidth, settings::kPaperWindowHeight};
    if (file_) p = settings::windowPlace(*file_, settings::kPaperWindowPrefix);
    resize(static_cast<int>(std::max(160L, p.width)), static_cast<int>(std::max(160L, p.height)));
    QPoint at;
    bool placed = false;
    if (p.x && p.y) {
        at = QPoint(static_cast<int>(*p.x), static_cast<int>(*p.y));
        // Its title bar on a screen that is there.
        placed = QGuiApplication::screenAt(at + QPoint(40, 10)) != nullptr;
    }
    if (!placed && mainWindow) {
        const QRect m = mainWindow->frameGeometry();
        at = QPoint(m.right() + 8, m.top());
        if (!QGuiApplication::screenAt(at + QPoint(40, 10))) at = m.topLeft() + QPoint(40, 40);
        placed = true;
    }
    if (placed) move(at);
}

void PaperWindow::take(QWidget* panel) {
    panel->setParent(this);
    layout_->addWidget(panel);
    panel->show();
}

void PaperWindow::release(QWidget* panel) { layout_->removeWidget(panel); }

void PaperWindow::setBackground(const QColor& c) {
    setAutoFillBackground(true);
    QPalette pal = palette();
    pal.setColor(QPalette::Window, c);
    setPalette(pal);
}

void PaperWindow::savePlace() {
    if (!file_) return;
    const QRect g = normalGeometry().isValid() ? normalGeometry() : geometry();
    settings::setWindowPlace(*file_, settings::kPaperWindowPrefix,
                             {pos().x(), pos().y(), g.width(), g.height()});
}

void PaperWindow::hideKeepingOwn() {
    if (!isVisible()) return;
    keepingOwn_ = true;
    close();
    keepingOwn_ = false;
}

void PaperWindow::closeEvent(QCloseEvent* e) {
    savePlace();
    QWidget::closeEvent(e);
    if (!keepingOwn_) emit closed();
}

} // namespace calcu::qt6
