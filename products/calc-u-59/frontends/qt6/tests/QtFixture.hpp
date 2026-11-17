#pragma once
#include "MainWindow.hpp"
#include "sessions/Messages.hpp"
#include <QString>
#include <QUrl>
#include <functional>
#include <memory>
#include <string>
#include <vector>

class QAction;

// The fixture of CalcU59QtTests (LIB-14 design §10.9, M-57; as Calc-U-1600's
// QtFixture): the real window of Calc-U-59, put together from the shell,
// over the product's session on the ROMs compiled in as the app has them, on
// the real clock, offscreen, with the HOME CTest gives it (no settings of the
// user's) and the product's examples/ as the bundled examples. The file and
// folder dialogs and the browser are replaced by recorders.
namespace qtfixture {

bool romsPresent();

struct Fixture {
    // `model`: [startup] model before the start; `started`: wait for the
    // first control state and the end of the start.
    explicit Fixture(const char* model = "TI-59", bool started = true);
    ~Fixture();

    MainWindow& w() { return *window; }
    EmulatorController& c();
    void run(int ms);
    bool waitFor(const std::function<bool()>& done, int ms = 10000);
    bool busy();
    bool waitIdle(int ms = 30000) { return waitFor([this] { return !busy(); }, ms); }
    // A command through the host, waited for (an operation: until it ends).
    calcu::sessions::ReplyMessage command(const std::string& name, calcu::sessions::Value args = {});
    QAction* action(const std::string& itemId) const;
    template <class W> W* find(const std::string& name) const {
        return window->findChild<W*>(QString::fromStdString(name));
    }
    std::string model();   // the control state's

    std::unique_ptr<MainWindow> window;
    std::vector<QString> fileAsks, fileStarts, fileFilters, folderAsks;
    QString fileAnswer, folderAnswer;
    std::vector<QUrl> opened;
};

} // namespace qtfixture
