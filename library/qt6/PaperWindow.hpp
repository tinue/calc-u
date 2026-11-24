#pragma once
#include "settings/SettingsFile.hpp"
#include <QWidget>

class QVBoxLayout;

// The paper panel's own window: a top-level window of its own (no parent,
// so the window system treats it as any other window, not one kept above
// the main window), titled "<product> — Printer", without a menu bar.
// Its place is in [window] paper-x/y/width/height (settings::windowPlace),
// restored when it is made, onto a screen that is there (a place off every
// screen: beside the given main window); written when it closes or hides.
// Closing it asks for the paper back (closed()); the panel area owns it.
// Object name `paper-window`.

namespace calcu::qt6 {

class PaperWindow : public QWidget {
    Q_OBJECT
public:
    PaperWindow(settings::SettingsFile*, const QString& title, const QWidget* mainWindow);

    void take(QWidget* panel);       // the panel into the window
    void release(QWidget* panel);    // out of it again (the caller reparents it)
    void savePlace();
    void setBackground(const QColor&);
    // Hides the window without asking for the paper back: the main
    // window's close (the setting stays: the next start opens it again).
    void hideKeepingOwn();

signals:
    void closed();   // the user closed it: the paper goes back

protected:
    void closeEvent(QCloseEvent*) override;

private:
    void restorePlace(const QWidget* mainWindow);

    settings::SettingsFile* file_;
    QVBoxLayout* layout_;
    bool keepingOwn_ = false;
};

} // namespace calcu::qt6
