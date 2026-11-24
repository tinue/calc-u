#include "qt6/PaperWindow.hpp"
#include "qt6/WindowPlacing.hpp"
#include "settings/WindowPlace.hpp"
#include <QCloseEvent>
#include <QVBoxLayout>

namespace calcu::qt6 {

PaperWindow::PaperWindow(settings::SettingsFile* file, const QString& title, const QWidget* mainWindow)
    : QWidget(nullptr, Qt::Window), file_(file) {
    setObjectName(QStringLiteral("paper-window"));
    setWindowTitle(title);
    setAttribute(Qt::WA_QuitOnClose, false);   // the main window's close ends the app
    layout_ = new QVBoxLayout(this);
    layout_->setContentsMargins(0, 0, 0, 0);
    settings::WindowPlace p{std::nullopt, std::nullopt, settings::kPaperWindowWidth, settings::kPaperWindowHeight};
    if (file_) p = settings::windowPlace(*file_, settings::kPaperWindowPrefix);
    applyWindowPlace(this, p, mainWindow);
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
    if (file_) settings::setWindowPlace(*file_, settings::kPaperWindowPrefix, windowPlaceOf(this));
}

void PaperWindow::hideKeepingOwn() {
    if (!isVisible()) return;
    savePlace();
    hide();
}

void PaperWindow::closeEvent(QCloseEvent* e) {
    savePlace();
    QWidget::closeEvent(e);
    emit closed();
}

} // namespace calcu::qt6
