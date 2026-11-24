#include "qt6/FileOpening.hpp"
#include "qt6/AppLogSink.hpp"
#include "opening/StartFile.hpp"
#include "qt6/SessionHost.hpp"
#include "sessions/Payload.hpp"
#include <QFile>
#include <QFileInfo>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QFileOpenEvent>
#include <QList>
#include <QMimeData>
#include <QTimer>
#include <QUrl>
#include <QWidget>
#include <utility>

namespace calcu::qt6 {

QString draggedFile(const QMimeData* mime) {
    const QList<QUrl> urls = mime && mime->hasUrls() ? mime->urls() : QList<QUrl>();
    return urls.size() == 1 && urls.first().isLocalFile() ? urls.first().toLocalFile() : QString();
}

sessions::Value wouldTakeArgs(const QString& path) {
    const QFileInfo info(path);
    return sessions::Value::object(
        {{"name", info.fileName().toStdString()}, {"size", static_cast<std::int64_t>(info.size())}});
}

bool wouldTake(SessionHost& host, const QString& path) {
    bool takes = false;   // requests are answered at once
    host.ask(opening::kWouldTakeRequest, wouldTakeArgs(path), [&takes](const sessions::ReplyMessage& r) {
        const auto* w = sessions::payloadAs<opening::WouldTake>(r.result);
        takes = w && w->takes;
    });
    return takes;
}

opening::OpenRequest openRequest(const QString& path, opening::Way way,
                                 const std::map<opening::Way, std::uint64_t>& limits) {
    const QFileInfo info(path);
    opening::OpenedFile file;
    file.name = info.fileName().toStdString();
    file.path = info.absoluteFilePath().toStdString();   // UTF-8: a preset is loaded from its folder
    QFile f(path);
    if (info.isFile() && f.open(QIODevice::ReadOnly)) {
        const auto limit = limits.find(way);
        const QByteArray raw =
            limit != limits.end() ? f.read(static_cast<qint64>(limit->second) + 1) : f.readAll();
        file.bytes = std::vector<std::uint8_t>(raw.begin(), raw.end());
    } else if (info.isFile()) {   // the session says it cannot be opened; the cause is Qt's
        appLog().warn("The file " + info.absoluteFilePath().toStdString() + " cannot be read: " +
                      f.errorString().toStdString());
    }
    opening::OpenRequest request;
    request.files.push_back(std::move(file));
    request.way = way;
    return request;
}

void openFile(SessionHost& host, const QString& path, opening::Way way,
              const std::map<opening::Way, std::uint64_t>& limits) {
    host.send(opening::kOpenCommand, openRequest(path, way, limits).toValue());
}

void bringToFront(QWidget& window, opening::Way way) {
    if (way == opening::Way::Start) return;
    window.raise();
    window.activateWindow();
}

void openFile(QWidget& window, SessionHost& host, const QString& path, opening::Way way,
              const std::map<opening::Way, std::uint64_t>& limits) {
    bringToFront(window, way);
    openFile(host, path, way, limits);
}

void dragEnter(QDragEnterEvent* e, SessionHost& host) {
    const QString path = draggedFile(e->mimeData());
    if (!path.isEmpty() && wouldTake(host, path)) e->acceptProposedAction();
}

void dragMove(QDragMoveEvent* e) { e->acceptProposedAction(); }

void drop(QDropEvent* e, QObject* context, std::function<void(const QString&)> open) {
    const QString path = draggedFile(e->mimeData());
    if (path.isEmpty()) return;
    e->acceptProposedAction();
    QTimer::singleShot(0, context, [open = std::move(open), path] { open(path); });
}

std::optional<QString> startFile(const std::vector<std::string>& arguments) {
    const auto start = opening::startFile(
        arguments, [](const std::string& p) { return QFileInfo(QString::fromStdString(p)).isFile(); });
    if (!start) return std::nullopt;
    return QString::fromStdString(*start);
}

std::optional<QString> startFile(const QStringList& arguments) {
    std::vector<std::string> all;
    for (const QString& a : arguments) all.push_back(a.toStdString());
    return startFile(all);
}

void FileOpenFilter::setHandler(std::function<void(const QString&)> handler) {
    onFileOpen_ = std::move(handler);
    if (onFileOpen_)
        for (const QString& path : std::exchange(pending_, {})) onFileOpen_(path);
}

bool FileOpenFilter::eventFilter(QObject* watched, QEvent* event) {
    if (event->type() != QEvent::FileOpen) return QObject::eventFilter(watched, event);
    const QString path = static_cast<QFileOpenEvent*>(event)->file();
    if (onFileOpen_)
        onFileOpen_(path);
    else
        pending_.append(path);
    return true;
}

} // namespace calcu::qt6
