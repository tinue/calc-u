#pragma once
#include "opening/OpenRequest.hpp"
#include "pacing/WallClock.hpp"
#include "shell/ShellProduct.hpp"
#include <QMainWindow>
#include <memory>

class CalculatorPanel;
class DebugPanel;
class EmulatorController;
class KeyboardWidget;
class QSplitter;
namespace calcu::qt6 {
class SessionHost;
class Shell;
} // namespace calcu::qt6

// The window, put together from LIB-14's shell (M-57; design §10.4): the
// shell's menus (File, Machine, Help; no Edit, AC-26), control bar, settings
// and About dialogs, notices, "Loading…" and "ROMs missing" over the
// session host of the EmulatorController it owns, with Calc-U-59's data
// (app/TIShellProduct). Its layout is today's (AC-33): the calculator column
// (the faceplate and the shell's bar), the shell's panel area holding the
// paper panel, and the debug panel side by side in a splitter whose
// dividers the user can move. Drag-and-drop of any one file: every way a
// file reaches the program is LIB-13's `open` (M-56).
class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr, calcu::pacing::WallClock clock = calcu::pacing::steadyWallTime);
    ~MainWindow() override;

    EmulatorController& controller() { return *m_controller; }
    calcu::qt6::SessionHost& host();
    calcu::qt6::Shell& shell() { return *m_shell; }
    KeyboardWidget* keyboard() const { return m_keyboard; }
    QSplitter* splitter() const { return m_splitter; }
    /// The host's timer and LIB-18's `start`.
    void start();

    /// A file handed over (LIB-13 AC-14): a drop, Open With (macOS), the
    /// file named at the start; the session's `open` (qt6::openFile, with the
    /// open part's limits: none for Calc-U-59, AC-11), the window raised for
    /// every way but the start (AC-24).
    void openFile(const QString& path, calcu::opening::Way way);

protected:
    void closeEvent(QCloseEvent* e) override;
    void dragEnterEvent(QDragEnterEvent* e) override;
    void dragMoveEvent(QDragMoveEvent* e) override;
    void dropEvent(QDropEvent* e) override;

private:
    calcu::shell::ShellProduct product();
    void restoreWindowState();
    void saveWindowState();
    void armCard();
    void armFreshCard();
    void selectAsmFile();
    void followPrinter();

    std::unique_ptr<EmulatorController> m_controller;
    calcu::qt6::Shell* m_shell = nullptr;
    QSplitter* m_splitter = nullptr;
    CalculatorPanel* m_calculator = nullptr;
    DebugPanel* m_debug = nullptr;
    KeyboardWidget* m_keyboard = nullptr;
};
