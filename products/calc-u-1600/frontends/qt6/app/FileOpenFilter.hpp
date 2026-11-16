#pragma once
#include <QEvent>
#include <QFileOpenEvent>
#include <QObject>
#include <QStringList>
#include <functional>
#include <utility>

// macOS hands a file dropped onto the Dock icon, or opened from Finder
// (Open With), to the application object as a QFileOpenEvent -- at a cold
// launch before there is a window. An event filter on the application keeps
// them until the window takes them; each is the session's `open` with the
// way Open With (LIB-13 AC-21; MainWindow::openFile). A filter, not a
// QApplication subclass, so that CalcU1600QtTests sends it the same event.
class FileOpenFilter : public QObject {
public:
    using QObject::QObject;

    void setHandler(std::function<void(const QString&)> handler) {
        m_onFileOpen = std::move(handler);
        if (m_onFileOpen)
            for (const QString& path : std::exchange(m_pending, {})) m_onFileOpen(path);
    }

protected:
    bool eventFilter(QObject* watched, QEvent* event) override {
        if (event->type() != QEvent::FileOpen) return QObject::eventFilter(watched, event);
        const QString path = static_cast<QFileOpenEvent*>(event)->file();
        if (m_onFileOpen)
            m_onFileOpen(path);
        else
            m_pending.append(path);
        return true;
    }

private:
    std::function<void(const QString&)> m_onFileOpen;
    QStringList m_pending;
};
