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

class QMimeData;

// LIB-13 on a Qt6 host (Part E simplification E-4; LIB-13 design, section
// "Part E simplification E-4"): what both products' Qt6 frontends wrote to
// hand a file to the session's `open`. The host decides nothing about the
// file; the session tells what it is (AC-1). What each product shows around
// it (raising the window, when the drop is sent) stays the product's.

namespace calcu::qt6 {

class SessionHost;

// The one local file a drag or a drop carries, or "": a drag of more than
// one file is not taken (AC-6).
QString draggedFile(const QMimeData*);

// `open-would-take {name, size}` of a drag (AC-6): the file's name and size,
// so that a file past the drop's limit is not taken (AC-20).
sessions::Value wouldTakeArgs(const QString& path);
bool wouldTake(SessionHost&, const QString& path);

// The file at `path` as the request of `way`: its name, its absolute path,
// its bytes (none when it is no file or cannot be read: EC-5). Past the
// way's limit in `limits` (the open part's `OpenSetup::maxBytes`) only one
// byte more is read, so that the session refuses it by size (AC-20); a way
// without a limit reads the whole file.
opening::OpenRequest openRequest(const QString& path, opening::Way,
                                 const std::map<opening::Way, std::uint64_t>& limits);
// Sends it as `open`; a refusal is the session's notice (AC-1, AC-15).
void openFile(SessionHost&, const QString& path, opening::Way, const std::map<opening::Way, std::uint64_t>& limits);

// The file named at the start (AC-14, AC-21): `opening::startFile` over the
// arguments, with Qt's test for an existing file.
std::optional<QString> startFile(const std::vector<std::string>& arguments);
std::optional<QString> startFile(const QStringList& arguments);

// macOS hands a file dropped onto the Dock icon, or opened from Finder
// (Open With), to the application object as a QFileOpenEvent, at a cold
// launch before there is a window. An event filter on the application
// keeps them until the window takes them; the handler opens each with the
// way Open With (AC-14, AC-21). A filter, not a QApplication subclass, so
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
