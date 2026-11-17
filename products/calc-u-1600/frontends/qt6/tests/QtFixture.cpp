#include "QtFixture.hpp"

#include "AppPaths.hpp"
#include "AppSettings.hpp"
#include "control/ControlState.hpp"
#include "qt6/Shell.hpp"
#include "sessions/LibraryTopics.hpp"
#include "shell/Inputs.hpp"
#include "shell/ObjectNames.hpp"

#include <QAction>
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QMenuBar>
#include <QTemporaryDir>
#include <QThread>
#include <cstdlib>
#include <filesystem>

namespace qtfixture {

namespace fs = std::filesystem;

namespace {

const fs::path kProduct = CALCU1600_DIR;

QTemporaryDir& tempDir() {
    static QTemporaryDir dir;
    return dir;
}

void setupProcess() {
    static bool done = false;
    if (done) return;
    done = true;
    const fs::path root = tempDir().path().toStdString();
    const fs::path bundle = root / "bundle";
    fs::create_directories(bundle);
    for (const fs::path dir : {kProduct / "roms", kProduct / "frontends/qt6/resources/cards"})
        for (const auto& f : fs::directory_iterator(dir))
            if (f.is_regular_file()) fs::copy_file(f.path(), bundle / f.path().filename());
    const fs::path hostDrive = kProduct / "firmware/pc1600-hostdrive/PC1600-P1-B7-HOSTDRIVE.bin";
    if (fs::exists(hostDrive)) fs::copy_file(hostDrive, bundle / hostDrive.filename());
    if (romsMissingRun()) fs::remove(bundle / "PC1600-P2-B6-new.bin");
    calcu::settings::HostDirs dirs;
    dirs.frontend = calcu::settings::FrontendKind::Qt6;
    dirs.home = (root / "home").generic_string();
    dirs.settingsBase = (root / "settings").generic_string();
    fs::create_directories(dirs.home);
    AppPaths::useHostDirs(dirs);
    AppPaths::useBundleDirs(cu1600::BundleDirs{bundle.generic_string(), (kProduct / "examples").generic_string()});
}

} // namespace

bool romsPresent() { return fs::exists(kProduct / "roms/PC1600-P0-B0-new.bin"); }
bool romsMissingRun() { return std::getenv("CALCU1600_QT_ROMS_MISSING") != nullptr; }

Fixture::Fixture(const char* startupModel, bool started) {
    setupProcess();
    AppSettings::setStartupModelPreference(QString::fromLatin1(startupModel));
    window = std::make_unique<MainWindow>();
    calcu::qt6::ShellChoosers c;
    c.file = [this](QWidget*, const QString& title, const QString& start, const QString& filter) {
        fileAsks.push_back(title);
        fileStarts.push_back(start);
        fileFilters.push_back(filter);
        return fileAnswer;
    };
    c.folder = [this](QWidget*, const QString& title, const QString&) {
        folderAsks.push_back(title);
        return folderAnswer;
    };
    c.openUrl = [this](const QUrl& u) {
        opened.push_back(u);
        return true;
    };
    c.clipboardText = [] { return QStringLiteral("10 PRINT 1"); };
    window->shell().setChoosers(std::move(c));
    window->setClipboardSink([this](const QImage& image, const QString& text) {
        clipboardImage = image;
        clipboardText = text;
    });
    window->resize(1430, 1000);
    window->show();
    window->start();
    if (started) waitFor([this] { return !model().empty() && !busy(); }, 30000);
}

Fixture::~Fixture() {
    window.reset();
    run(10);
}

void Fixture::run(int ms) {
    QElapsedTimer t;
    t.start();
    while (t.elapsed() < ms) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 5);
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        QThread::msleep(1);
    }
}

bool Fixture::waitFor(const std::function<bool()>& done, int ms) {
    QElapsedTimer t;
    t.start();
    while (!done()) {
        if (t.elapsed() > ms) return false;
        QCoreApplication::processEvents(QEventLoop::AllEvents, 5);
        QThread::msleep(1);
    }
    QCoreApplication::processEvents();
    return true;
}

bool Fixture::busy() {
    return calcu::shell::busyOf(window->host().value(calcu::sessions::kBusyTopic)).operationRuns;
}

calcu::sessions::ReplyMessage Fixture::command(const std::string& name, calcu::sessions::Value args) {
    std::optional<calcu::sessions::ReplyMessage> got;
    window->host().send(calcu::sessions::CommandName{name}, std::move(args),
                        [&got](const calcu::sessions::ReplyMessage& r) { got = r; });
    waitFor([&got] { return got.has_value(); }, 60000);
    run(50);
    return got ? *got : calcu::sessions::ReplyMessage{};
}

QAction* Fixture::action(const std::string& itemId) const {
    return window->menuBar()->findChild<QAction*>(QString::fromStdString(calcu::shell::names::item(itemId)));
}

std::string Fixture::model() {
    return calcu::shell::read::text(window->host().value(calcu::control::kControlStateTopic), "model");
}

} // namespace qtfixture
