#pragma once
#include "opening/OpenRequest.hpp"
#include "pacing/WallClock.hpp"
#include "shell/ShellProduct.hpp"
#include <QMainWindow>
#include <QPointer>
#include <memory>

class CalculatorPanel;
class DebugPanel;
class EmulatorController;
class KeyboardWidget;
class QSplitter;
class QToolButton;
namespace calcu::qt6 {
class SessionHost;
class Shell;
} // namespace calcu::qt6

// The window, put together from the library's shell: the
// shell's menus (File, Machine, Help; no Edit), control bar, settings
// and About dialogs, notices, "Loading…" and "ROMs missing" over the
// session host of the EmulatorController it owns, with Calc-U-59's data
// (app/TIShellProduct). Its view follows the library's rule (shell::view
// on app/'s chrome): where the room beside the faceplate takes the
// printer, the calculator column (the faceplate and the shell's bar), the
// shell's panel area holding the paper panel, and the debug panel (where
// the room takes it too) side by side in a splitter whose dividers the
// user can move; else one page at a time, calculator or printer, switched
// with the chevron at the window's top corner. Drag-and-drop of any one file: every way a
// file reaches the program is the session's `open`.
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
    /// One page at a time (the window too narrow for the printer beside the
    /// faceplate), and the chevron that switches the pages.
    bool paged() const { return m_paged; }
    QToolButton* pageChevron() const { return m_chevron; }
    /// The host's timer and the session's `start`.
    void start();

    /// A file handed over: a drop, Open With (macOS), the
    /// file named at the start; the session's `open` (qt6::openFile, with the
    /// open part's limits: none for Calc-U-59), the window raised for
    /// every way but the start.
    void openFile(const QString& path, calcu::opening::Way way);

protected:
    void closeEvent(QCloseEvent* e) override;
    void dragEnterEvent(QDragEnterEvent* e) override;
    void dragMoveEvent(QDragMoveEvent* e) override;
    void dropEvent(QDropEvent* e) override;
    bool eventFilter(QObject* o, QEvent* e) override;

private:
    QPointer<QWidget> m_paperStyled;   // the paper panel in the chrome now
    calcu::shell::ShellProduct product();
    void restoreWindowState();
    void saveWindowState();
    void armCard();
    void armFreshCard();
    void selectAsmFile();
    void followPrinter();
    void applyView();

    std::unique_ptr<EmulatorController> m_controller;
    calcu::qt6::Shell* m_shell = nullptr;
    QSplitter* m_splitter = nullptr;
    CalculatorPanel* m_calculator = nullptr;
    DebugPanel* m_debug = nullptr;
    KeyboardWidget* m_keyboard = nullptr;
    QToolButton* m_chevron = nullptr;
    bool m_paged = false;
    bool m_printerPage = false;   // the page shown while paged
};
