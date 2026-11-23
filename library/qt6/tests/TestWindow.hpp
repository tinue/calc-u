#pragma once
#include "../../settings/tests/TestProduct.hpp"
#include "ScriptedClock.hpp"
#include "TestShellPart.hpp"
#include "qt6/SessionHost.hpp"
#include "qt6/Shell.hpp"
#include "sessions/AppSession.hpp"
#include "settings/Paths.hpp"
#include "settings/SettingsFile.hpp"
#include "shell/SettingsPart.hpp"
#include <QMainWindow>
#include <QString>
#include <functional>
#include <memory>
#include <string>
#include <vector>

class QAction;
class QWidget;

// The test window of the Qt6 shell: an app session with TestShellPart (the
// control state, preset list and product built for the tests) and
// SettingsPart over the settings' test declaration, the test machine
// test-seg, hosted by a SessionHost on a scripted wall clock (1 ms a
// reading), and a QMainWindow put together from the shell with a plain
// faceplate widget of 50 × 30 hit-map units at scale 4. No code of a product.
// The file and folder dialogs, the browser and the clipboard are replaced by
// recorders.

namespace calcu::qt6::test {

class TestWindow {
public:
    // `product`: changes the test product before the shell takes it.
    explicit TestWindow(bool withControlState = true, ShellOptions options = defaultOptions(),
                        const std::function<void(shell::ShellProduct&)>& product = {});
    ~TestWindow();

    static ShellOptions defaultOptions() { return {}; }

    void settle();                   // the event loop's pending work
    void step(int calls = 1);        // host calls, then settle()
    // Host calls until `ms` of scripted wall time passed.
    void runFor(int ms);
    sessions::ReplyMessage command(const std::string& name, sessions::Value args = {});

    QAction* action(const std::string& itemId) const;
    template <class W> W* find(const std::string& name) const {
        return window.findChild<W*>(QString::fromStdString(name));
    }

    calcu::testing::ScriptedClock clock;
    settings::test::TempDir home{"calcu-qt6-shell"};
    settings::SettingsFile file;
    std::unique_ptr<settings::Paths> paths;
    std::unique_ptr<shell::SettingsPart> settingsPart;
    shell::test::TestShellPart part;
    std::unique_ptr<sessions::AppSession> session;
    QMainWindow window;
    QWidget* faceplate = nullptr;
    std::unique_ptr<SessionHost> host;
    Shell* shell = nullptr;
    double scale = 4;

    // What the replaced dialogs were asked and give.
    std::vector<QString> fileAsks, folderAsks;
    QString fileAnswer, folderAnswer;
    std::vector<QUrl> opened;
    bool openSucceeds = true;
    QString clipboard = QStringLiteral("12+3");
    int productActs = 0;   // the product item `load-program` chosen
};

} // namespace calcu::qt6::test
