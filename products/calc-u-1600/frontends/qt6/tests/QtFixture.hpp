#pragma once
#include <QImage>
#include <QString>
#include <QUrl>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "MainWindow.hpp"
#include "sessions/Messages.hpp"

class QAction;
class QWidget;

// The fixture of CalcU1600QtTests: the real window of
// Calc-U-1600, put together from the shell, over the product's session on
// the real ROMs, on the real clock, offscreen. Once per process: a temporary
// home and settings folder (nothing of the user's is read or written) and a
// bundle folder of its own with the ROM set, the module definitions and the
// host drive's ROM (with CALCU1600_QT_ROMS_MISSING one ROM file is left
// out). The file and folder dialogs, the browser and the clipboard are
// replaced by recorders.
namespace qtfixture {

bool romsPresent();
bool romsMissingRun();   // CALCU1600_QT_ROMS_MISSING is set

struct Fixture {
    explicit Fixture(const char* startupModel = "PC-1500A", bool started = true);
    ~Fixture();

    MainWindow& w() { return *window; }
    // Events for `ms` of wall time (the host's timer calls the session).
    void run(int ms);
    // Events until `done` or `ms` passed; whether it came.
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
    // The replaced dialogs, the browser and the clipboard.
    std::vector<QString> fileAsks, fileStarts, fileFilters, folderAsks;
    QString fileAnswer, folderAnswer;
    std::vector<QUrl> opened;
    QImage clipboardImage;
    QString clipboardText;
};

} // namespace qtfixture
