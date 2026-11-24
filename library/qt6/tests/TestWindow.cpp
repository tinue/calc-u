#include "TestWindow.hpp"
#include "TestMachines.hpp"
#include "qt6/PanelArea.hpp"
#include "sessions/MachineSession.hpp"
#include "shell/ObjectNames.hpp"
#include <QAction>
#include <QCoreApplication>
#include <QMenuBar>
#include <QVBoxLayout>
#include <QWidget>
#include <chrono>

namespace calcu::qt6::test {

using namespace std::chrono_literals;

TestWindow::TestWindow(bool withControlState, ShellOptions options,
                       const std::function<void(shell::ShellProduct&)>& change, bool paperWindow)
    : file(settings::SettingsFile::open(settings::test::testProduct(), home.str() + "/settings/Test-Product.ini")),
      part(withControlState) {
    clock.step = 1ms;   // the pacer's full speed ends when its allowance is used up (ScriptedClock.hpp)
    settings::HostDirs dirs;
    dirs.home = home.str();
    dirs.settingsBase = home.str() + "/settings";
    paths = std::make_unique<settings::Paths>(file, dirs);
    shell::SettingsPartSetup ss;
    ss.file = &file;
    ss.paths = paths.get();
    settingsPart = std::make_unique<shell::SettingsPart>(std::move(ss));
    sessions::AppSessionSetup a;
    a.clock = clock.fn();
    a.hostTime = [] { return std::chrono::system_clock::time_point{}; };
    a.parts = {settingsPart.get(), &part};
    a.hosting = sessions::Hosting::Paced;
    session = std::make_unique<sessions::AppSession>(std::move(a));
    session->machineSession().setMachine(machine::test::buildTestMachine(machine::test::kSeg), pacing::PacingProfile{}, 0);

    auto* central = new QWidget(&window);
    auto* column = new QVBoxLayout(central);
    faceplate = new QWidget(central);
    faceplate->setObjectName(QStringLiteral("test-faceplate"));
    faceplate->setFixedSize(static_cast<int>(50 * scale), static_cast<int>(30 * scale));
    window.setCentralWidget(central);
    sessions::HostServices services;
    services.offered = {sessions::HostService::OpenUrl, sessions::HostService::ClipboardText};
    host = std::make_unique<SessionHost>(*session, services, clock.fn());
    auto product = shell::test::product();
    product.items[0].act = [this] { ++productActs; };
    if (change) change(product);
    if (paperWindow) options.windowSettings = &file;
    shell = new Shell(window, *host, std::move(product), shell::familySections(settings::test::testProduct()), options);
    column->addWidget(faceplate);
    column->addWidget(shell->romsMissingView());
    column->addWidget(shell->controlBar());
    column->addWidget(shell->panelArea());
    shell->setFaceplate(faceplate, shell::test::hitMap(), [this] { return scale; });
    ShellChoosers c;
    c.file = [this](QWidget*, const QString& title, const QString&, const QString&) {
        fileAsks.push_back(title);
        return fileAnswer;
    };
    c.folder = [this](QWidget*, const QString& title, const QString&) {
        folderAsks.push_back(title);
        return folderAnswer;
    };
    c.openUrl = [this](const QUrl& u) {
        opened.push_back(u);
        return openSucceeds;
    };
    c.clipboardText = [this] { return clipboard; };
    shell->setChoosers(std::move(c));
    window.resize(1000, 700);
    window.show();
    settle();
    step();
}

TestWindow::~TestWindow() {
    // The window's widgets go before the host they send to, the host before
    // the session.
    delete window.centralWidget();
    delete shell;
    shell = nullptr;
    host.reset();
}

void TestWindow::settle() {
    for (int i = 0; i < 6; ++i) {
        QCoreApplication::processEvents();
        // deleteLater is not done by processEvents outside an event loop.
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    }
}

void TestWindow::step(int calls) {
    for (int i = 0; i < calls; ++i) {
        clock.now += 16ms;
        host->call();
    }
    settle();
}

void TestWindow::runFor(int ms) {
    const auto end = clock.now + std::chrono::milliseconds(ms);
    while (clock.now < end) {
        clock.now += 16ms;
        host->call();
        QCoreApplication::processEvents();
    }
    settle();
}

sessions::ReplyMessage TestWindow::command(const std::string& name, sessions::Value args) {
    sessions::ReplyMessage got;
    host->send(sessions::CommandName{name}, std::move(args), [&got](const sessions::ReplyMessage& r) { got = r; });
    settle();
    return got;
}

QAction* TestWindow::action(const std::string& itemId) const {
    return window.menuBar()->findChild<QAction*>(QString::fromStdString(shell::names::item(itemId)));
}

} // namespace calcu::qt6::test
