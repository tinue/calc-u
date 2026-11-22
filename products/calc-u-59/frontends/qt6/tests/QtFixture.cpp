#include "QtFixture.hpp"
#include "AppPaths.hpp"
#include "EmulatorController.hpp"
#include "control/ControlState.hpp"
#include "qt6/Shell.hpp"
#include "sessions/LibraryTopics.hpp"
#include "settings/SettingsFile.hpp"
#include "settings/Paths.hpp"
#include "shell/Inputs.hpp"
#include "shell/ObjectNames.hpp"
#include <QAction>
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QMenuBar>
#include <QTemporaryDir>
#include <QThread>
#include <filesystem>
#include <memory>
#include <optional>

namespace qtfixture {

bool romsPresent() { return std::filesystem::exists(CALCU59_DIR "/roms/TMC0571B.txt"); }

namespace {

// Folders of the test process's own: the user's settings and cards stay
// untouched (and their startup presets out of the tests).
void useOwnFolders() {
    static QTemporaryDir dir;
    static bool done = false;
    if (done) return;
    done = true;
    const std::filesystem::path root = dir.path().toStdString();
    calcu::settings::HostDirs dirs;
    dirs.frontend = calcu::settings::FrontendKind::Qt6;
    dirs.home = (root / "home").generic_string();
    dirs.settingsBase = (root / "settings").generic_string();
    std::filesystem::create_directories(dirs.home);
    AppPaths::useHostDirs(dirs);
}

} // namespace

Fixture::Fixture(const char* model, bool started) {
    useOwnFolders();
    AppPaths::useBundledExamplesDir(QStringLiteral(CALCU59_DIR "/examples"));
    AppPaths::file().set("startup", "model", model);
    window = std::make_unique<MainWindow>();
    calcu::qt6::ShellChoosers ch;
    ch.file = [this](QWidget*, const QString& title, const QString& start, const QString& filter) {
        fileAsks.push_back(title);
        fileStarts.push_back(start);
        fileFilters.push_back(filter);
        return fileAnswer;
    };
    ch.folder = [this](QWidget*, const QString& title, const QString&) {
        folderAsks.push_back(title);
        return folderAnswer;
    };
    ch.openUrl = [this](const QUrl& u) {
        opened.push_back(u);
        return true;
    };
    window->shell().setChoosers(std::move(ch));
    window->resize(1200, 900);
    window->show();
    window->start();
    if (!started) return;
    waitFor([this] { return !this->model().empty() && !busy(); }, 30000);
    run(500);   // the ROM's start-up
}

Fixture::~Fixture() {
    window.reset();
    run(10);
}

EmulatorController& Fixture::c() { return window->controller(); }

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

bool Fixture::busy() { return calcu::shell::busyOf(window->host().value(calcu::sessions::kBusyTopic)).operationRuns; }

calcu::sessions::ReplyMessage Fixture::command(const std::string& name, calcu::sessions::Value args) {
    auto got = std::make_shared<std::optional<calcu::sessions::ReplyMessage>>();
    window->host().send(calcu::sessions::CommandName{name}, std::move(args),
                        [got](const calcu::sessions::ReplyMessage& r) { *got = r; });
    waitFor([got] { return got->has_value(); }, 60000);
    run(100);
    return *got ? **got : calcu::sessions::ReplyMessage{};
}

QAction* Fixture::action(const std::string& itemId) const {
    return window->menuBar()->findChild<QAction*>(QString::fromStdString(calcu::shell::names::item(itemId)));
}

std::string Fixture::model() {
    return calcu::shell::read::text(window->host().value(calcu::control::kControlStateTopic), "model");
}

} // namespace qtfixture
