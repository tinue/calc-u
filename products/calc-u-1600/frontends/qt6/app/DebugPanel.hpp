#pragma once
#include <QWidget>
#include <cstdint>
#include <string>
#include <vector>

#include "Debug/Inspect/Inspector.hpp"

class QMenu;
class QPlainTextEdit;
class QPushButton;
class QLabel;
class MachineController;

// INSPECTOR: the docked machine inspector -- a header, a scrolling
// monospaced output log, and a button bar of pills (Pointers, the Memory ▾
// and Dump ▾ menus, Module YAML, Clear) in the product's chrome
// (app/SharpChrome), lined up with the panels beside it. The
// views themselves are built in Core (core/Debug/Inspect, via
// MachineController::inspectorView()); stepping and breakpoints are the
// debugger's (help/07-debugger.md), not this panel's. Always visible under the
// control bar (see MainWindow).
//
// A plain QWidget (not a QDialog, unlike SettingsDialog) so it docks inline
// rather than floating -- see MainWindow's m_debugRow, which also reserves
// the layout slot a future plotter-paper widget will share it with at a
// 2:1 width split.
class DebugPanel : public QWidget {
    Q_OBJECT
public:
    explicit DebugPanel(MachineController* controller, QWidget* parent = nullptr);
    ~DebugPanel();

    // Text selected (with the mouse) in the output area, "" if none.
    // `outputSelectionChanged` fires when that changes -- Edit > Copy then
    // copies it instead of the screen.
    QString selectedOutputText() const;

signals:
    void outputSelectionChanged(bool hasSelection);

protected:
    // The system's light/dark appearance changed (PaletteChange,
    // ApplicationPaletteChange): the chrome follows.
    void changeEvent(QEvent* event) override;

private:
    MachineController* m_controller;               // not owned

    QLabel* m_header = nullptr;
    QWidget* m_buttonBar = nullptr;
    QPlainTextEdit* m_output = nullptr;
    QPushButton* m_pointersButton = nullptr;
    QPushButton* m_memoryButton = nullptr; // Memory ▾
    QPushButton* m_dumpButton = nullptr;   // Dump ▾
    QPushButton* m_moduleYamlButton = nullptr;
    QPushButton* m_clearButton = nullptr;

    // ── Ring buffer / paging ─────────────────────────────────────────────
    static constexpr int kRingBufCap = 2000;
    static constexpr int kDisplayLinesPerPage = 40;
    static constexpr int kDisplayPageCap = 10;
    std::vector<std::string> m_ringBuf = std::vector<std::string>(kRingBufCap);
    int m_ringHead = 0;
    int m_debugLastDisplayHead = 0;
    void ringWrite(const std::string& line);
    void ringWriteAll(const std::vector<std::string>& lines);
    void refreshDebugDisplay();

    // ── Inspector views and Module YAML ───────────────────────────────
    void fillMenu(QMenu* menu, bool dumps);
    void showView(inspect::View view);
    void debugDumpModuleAsYaml();
    void clearDebug();

    // The chrome's light or dark colours as the window's appearance is,
    // again at each palette change (changeEvent).
    void applyChrome();
};
