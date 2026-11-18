#pragma once
#include "opening/OpenRequest.hpp"
#include "sessions/Value.hpp"
#include <QEvent>
#include <QObject>
#include <QString>
#include <QStringList>
#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <vector>

class QDragEnterEvent;
class QDragMoveEvent;
class QDropEvent;
class QMimeData;
class QWidget;

// Opening files on a Qt6 host: what both products' Qt6 frontends use to hand
// a file to the session's `open`. The host decides nothing about the file;
// the session tells what it is. The window's part is one rule for both
// products: a drop and Open With bring the window to the front, a drop's
// proposed action is accepted and the file is opened after the drop handler
// returned.

namespace calcu::qt6 {

class SessionHost;

// The one local file a drag or a drop carries, or "": a drag of more than
// one file is not taken.
QString draggedFile(const QMimeData*);

// `open-would-take {name, size}` of a drag: the file's name and size, so
// that a file past the drop's limit is not taken.
sessions::Value wouldTakeArgs(const QString& path);
bool wouldTake(SessionHost&, const QString& path);

// The file at `path` as the request of `way`: its name, its absolute path,
// its bytes (none when it is no file or cannot be read). Past the
// way's limit in `limits` (the open part's `OpenSetup::maxBytes`) only one
// byte more is read, so that the session refuses it by size; a way
// without a limit reads the whole file.
opening::OpenRequest openRequest(const QString& path, opening::Way,
                                 const std::map<opening::Way, std::uint64_t>& limits);
// Sends it as `open`; a refusal is the session's notice.
void openFile(SessionHost&, const QString& path, opening::Way, const std::map<opening::Way, std::uint64_t>& limits);
// The same from a window: for every way but the start (the file
// named when the program starts) the window is raised and activated first.
void openFile(QWidget& window, SessionHost&, const QString& path, opening::Way,
              const std::map<opening::Way, std::uint64_t>& limits);
void bringToFront(QWidget& window, opening::Way);

// A window's drag and drop. dragEnter: one file the session
// would take, the proposed action accepted (the pointer shows the rest is
// not taken); dragMove: every move accepted once the enter was; drop: the
// one file's proposed action accepted and `open` called from the event loop
// after the drop (a flow and its dialogs would keep the drag source --
// Finder, Explorer -- waiting), unless `context` is gone by then.
void dragEnter(QDragEnterEvent*, SessionHost&);
void dragMove(QDragMoveEvent*);
void drop(QDropEvent*, QObject* context, std::function<void(const QString&)> open);

// The file named at the start: `opening::startFile` over the
// arguments, with Qt's test for an existing file.
std::optional<QString> startFile(const std::vector<std::string>& arguments);
std::optional<QString> startFile(const QStringList& arguments);

// macOS hands a file dropped onto the Dock icon, or opened from Finder
// (Open With), to the application object as a QFileOpenEvent, at a cold
// launch before there is a window. An event filter on the application
// keeps them until the window takes them; the handler opens each with the
// way Open With. A filter, not a QApplication subclass, so
// that a test sends it the same event.
class FileOpenFilter : public QObject {
public:
    using QObject::QObject;
    // Set: the files kept are handed over in their order, later ones at once.
    void setHandler(std::function<void(const QString&)> handler);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    std::function<void(const QString&)> onFileOpen_;
    QStringList pending_;
};

} // namespace calcu::qt6
