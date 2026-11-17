#pragma once
#include <QMainWindow>

class EmulatorController;
class KeyboardWidget;
class QSplitter;

// The window: calculator | printer | debug side by side (the Swift app's Mac
// layout, CalculatorView), menus for presets, cards, settings and About, and
// drag-and-drop of any one file: every way a file reaches the program is
// LIB-13's `open` (M-56), the session tells what the file is.
class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(EmulatorController& controller, QWidget* parent = nullptr);

    /// A file handed over by Open With (macOS) or named at the start (LIB-13 AC-14).
    void openFile(const QString& path, bool atStart);

protected:
    void closeEvent(QCloseEvent* e) override;
    void dragEnterEvent(QDragEnterEvent* e) override;
    void dropEvent(QDropEvent* e) override;

private:
    void buildMenus();
    void restoreWindowState();
    void saveWindowState();
    void openPreset();
    void loadCard();
    void newCard();
    void selectAsmFile();

    EmulatorController& m_controller;
    QSplitter* m_splitter;
    KeyboardWidget* m_keyboard = nullptr;
};
