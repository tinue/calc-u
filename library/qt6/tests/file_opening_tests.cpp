// Part E simplification E-4: LIB-13 on a Qt6 host (library/qt6/FileOpening)
// and LIB-7 D-6's round trip (library/qt6/AskedSend), without a window.
#include "TestHarness.hpp"
#include "qt6/AskedSend.hpp"
#include "qt6/FileOpening.hpp"
#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QFileOpenEvent>
#include <QMimeData>
#include <QTemporaryDir>
#include <QUrl>
#include <string>
#include <vector>

namespace {

using namespace calcu;
using opening::Way;

QString writeFile(const QTemporaryDir& dir, const QString& name, int bytes) {
    const QString path = dir.filePath(name);
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly)) return path;
    f.write(QByteArray(bytes, 'x'));
    return path;
}

} // namespace

TEST(lib13_ac6_qt_dragged_file_is_one_local_file) {
    QMimeData one;
    one.setUrls({QUrl::fromLocalFile(QStringLiteral("/tmp/a.ti59"))});
    CHECK_EQ(qt6::draggedFile(&one).toStdString(), std::string("/tmp/a.ti59"));
    QMimeData two;
    two.setUrls({QUrl::fromLocalFile(QStringLiteral("/tmp/a.ti59")), QUrl::fromLocalFile(QStringLiteral("/tmp/b.ti59"))});
    CHECK(qt6::draggedFile(&two).isEmpty());   // more than one file: not taken
    QMimeData remote;
    remote.setUrls({QUrl(QStringLiteral("https://example.org/a.ti59"))});
    CHECK(qt6::draggedFile(&remote).isEmpty());
    QMimeData text;
    text.setText(QStringLiteral("a.ti59"));
    CHECK(qt6::draggedFile(&text).isEmpty());
    CHECK(qt6::draggedFile(nullptr).isEmpty());
}

TEST(lib13_ac6_qt_would_take_asks_by_name_and_size) {
    QTemporaryDir dir;
    const QString path = writeFile(dir, QStringLiteral("game.tseg"), 7);
    const sessions::Value a = qt6::wouldTakeArgs(path);
    CHECK(a.find("name") && *a.find("name")->string() == "game.tseg");
    CHECK(a.find("size") && *a.find("size")->integer() == 7);
}

TEST(lib13_ac20_qt_read_up_to_the_ways_limit) {
    QTemporaryDir dir;
    const QString path = writeFile(dir, QStringLiteral("big.bin"), 20);
    const std::map<Way, std::uint64_t> limits{{Way::Drop, 10}, {Way::OpenWith, 10}, {Way::Start, 10}};
    const opening::OpenRequest drop = qt6::openRequest(path, Way::Drop, limits);
    CHECK(drop.way == Way::Drop);
    CHECK_EQ(drop.files.size(), std::size_t(1));
    CHECK_EQ(drop.files[0].name, std::string("big.bin"));
    CHECK(drop.files[0].path && *drop.files[0].path == QFileInfo(path).absoluteFilePath().toStdString());
    CHECK(drop.files[0].bytes && drop.files[0].bytes->size() == 11);   // one byte past the limit: refused by size
    const opening::OpenRequest dialog = qt6::openRequest(path, Way::Dialog, limits);
    CHECK(dialog.files[0].bytes && dialog.files[0].bytes->size() == 20);   // the dialogs have no limit
    const opening::OpenRequest none = qt6::openRequest(path, Way::Drop, {});
    CHECK(none.files[0].bytes && none.files[0].bytes->size() == 20);   // a product without a limit (AC-11)
}

TEST(lib13_ec5_qt_unreadable_file_has_no_bytes) {
    QTemporaryDir dir;
    CHECK(!qt6::openRequest(dir.filePath(QStringLiteral("missing.ti59")), Way::OpenWith, {}).files[0].bytes);
    CHECK(!qt6::openRequest(dir.path(), Way::Drop, {}).files[0].bytes);   // a folder is no file
}

TEST(lib13_ac21_qt_start_file_is_last_existing_file) {
    QTemporaryDir dir;
    const QString path = writeFile(dir, QStringLiteral("p.ti59"), 1);
    const auto start = qt6::startFile(QStringList{QStringLiteral("app"), path, QStringLiteral("-NSDocumentRevisionsDebugMode"),
                                                  QStringLiteral("YES")});
    CHECK(start && *start == path);
    CHECK(!qt6::startFile(QStringList{QStringLiteral("app"), dir.filePath(QStringLiteral("none.ti59"))}));
}

TEST(lib13_ac21_qt_file_open_events_kept_until_the_handler) {
    qt6::FileOpenFilter filter;
    QObject target;
    target.installEventFilter(&filter);
    QFileOpenEvent first(QStringLiteral("/tmp/one.ti59"));
    QFileOpenEvent second(QStringLiteral("/tmp/two.ti59"));
    CHECK(QCoreApplication::sendEvent(&target, &first));   // taken, kept
    CHECK(QCoreApplication::sendEvent(&target, &second));
    std::vector<std::string> opened;
    filter.setHandler([&opened](const QString& p) { opened.push_back(p.toStdString()); });
    CHECK((opened == std::vector<std::string>{"/tmp/one.ti59", "/tmp/two.ti59"}));   // in their order
    QFileOpenEvent third(QStringLiteral("/tmp/three.ti59"));
    CHECK(QCoreApplication::sendEvent(&target, &third));
    CHECK_EQ(opened.back(), std::string("/tmp/three.ti59"));   // at once
    QEvent other(QEvent::User);
    QCoreApplication::sendEvent(&target, &other);   // anything else passes
    CHECK_EQ(opened.size(), std::size_t(3));
}

TEST(lib7_d6_qt_asked_send_round_trip) {
    std::vector<bool> sent;
    std::vector<std::string> asked;
    // Done at once: no question.
    auto r = qt6::sendAsking([&](bool c) { sent.push_back(c); return std::string(); },
                             [&](const QString& q) { asked.push_back(q.toStdString()); return true; });
    CHECK(r.done && !r.declined && r.refusal.empty());
    CHECK((sent == std::vector<bool>{false}) && asked.empty());
    // A question, yes: the same command confirmed.
    sent.clear();
    r = qt6::sendAsking([&](bool c) { sent.push_back(c); return c ? std::string() : std::string("asks: Replace it?"); },
                        [&](const QString& q) { asked.push_back(q.toStdString()); return true; });
    CHECK(r.done && !r.declined);
    CHECK((sent == std::vector<bool>{false, true}));
    CHECK((asked == std::vector<std::string>{"Replace it?"}));
    // A question, no: nothing more is sent.
    sent.clear();
    r = qt6::sendAsking([&](bool c) { sent.push_back(c); return std::string("asks: Replace it?"); },
                        [](const QString&) { return false; });
    CHECK(!r.done && r.declined && r.refusal.empty());
    CHECK((sent == std::vector<bool>{false}));
    // A refusal that is no question: not asked.
    asked.clear();
    r = qt6::sendAsking([](bool) { return std::string("The place is busy."); },
                        [&](const QString& q) { asked.push_back(q.toStdString()); return true; });
    CHECK(!r.done && !r.declined && r.refusal == "The place is busy.");
    CHECK(asked.empty());
    CHECK(!qt6::askedQuestion("asks:"));   // without the space: no question of D-6
}
