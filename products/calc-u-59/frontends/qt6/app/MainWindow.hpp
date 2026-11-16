#pragma once
#include <QMainWindow>

class EmulatorController;
class KeyboardWidget;
class QSplitter;

// The window: calculator | printer | debug side by side (the Swift app's Mac
// layout, CalculatorView), menus for presets, cards, settings and About, and
// drag-and-drop of presets and cards.
class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(EmulatorController& controller, QWidget* parent = nullptr);

    void openStateFile(const QString& path);

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
