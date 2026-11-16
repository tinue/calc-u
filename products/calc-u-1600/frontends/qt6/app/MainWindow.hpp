#pragma once
#include <QByteArray>
#include <QElapsedTimer>
#include <QImage>
#include <QMainWindow>
#include <QString>
#include <functional>
#include <memory>
#include <optional>
#include <string>

#include "MachineController.hpp"
#include "opening/OpenRequest.hpp"
#include "qt6/HostKeys.hpp"
#include "qt6/Shell.hpp"
#include "sessions/Value.hpp"

class QKeyEvent;
class QCloseEvent;
class QDragEnterEvent;
class QDragMoveEvent;
class QDropEvent;
class FaceplateWidget;
class DebugPanel;
class DiskControl;
class Ce158PrinterWidget;
class AudioOutput;

// The window of Calc-U-1600 (LIB-14 Part D, D.3): a QMainWindow put together
// from the Qt6 component set's shell (calcu::qt6::Shell) over the session
// host of MachineController. The shell draws the menus, the control bar,
// the settings and About dialogs, the notices, "Loading…", "ROMs missing"
// and the panel area from the session's topics, and the pointer on the
// faceplate (the hit map of app/SharpShellProduct). The window keeps the
// product's: the layout of AC-40 (the faceplate, below it the bar, below it
// the row with the INSPECTOR, the paper panel and, while a CE-158 is
// plugged in, its printer panel), the faceplate's art and LCD, the host
// keys and the input method (LIB-5), the drop and Open With (LIB-13), the
// program flows of File ▸ Load BASIC Program… and Load Machine Code…
// (C1600-2), the host drive's items, Copy Screen (Copy Log Selection while
// text of the INSPECTOR is selected) and the SHIFT tap (LIB-5 AC-33).
class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

    // The session starts (LIB-18): after the window is up.
    void start();

    calcu::qt6::Shell& shell() { return *m_shell; }
    calcu::qt6::SessionHost& host() { return m_controller->host(); }
    MachineController* controller() const { return m_controller.get(); }
    FaceplateWidget* faceplate() const { return m_faceplate; }
    DebugPanel* inspector() const { return m_inspector; }
    // Copy Screen's picture (AC-41): the display at its physical size (dots
    // per metre set); null for an empty screen.
    QImage screenImage() const;
    static QImage toQImage(const GrayImage& screen);

    // A file to open (LIB-13): a drop, Open With (the Dock icon, Finder), the
    // file named at the start. Sent as the session's `open` with the way it
    // came; a refusal is the session's notice. A program file comes back as
    // the topic c1600.program-opened (onProgramOpened).
    void openFile(const QString& path, calcu::opening::Way way);
    // The tests: Copy Screen's picture and text go here, not to the
    // platform's clipboard.
    void setClipboardSink(std::function<void(const QImage&, const QString&)> sink) { m_clipboardSink = std::move(sink); }

protected:
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;
    void inputMethodEvent(QInputMethodEvent* event) override;
    QVariant inputMethodQuery(Qt::InputMethodQuery query) const override;
    void changeEvent(QEvent* event) override;
    void closeEvent(QCloseEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dropEvent(QDropEvent* event) override;

private:
    std::unique_ptr<MachineController> m_controller;
    calcu::qt6::Shell* m_shell = nullptr;
    FaceplateWidget* m_faceplate = nullptr;
    DebugPanel* m_inspector = nullptr;
    Ce158PrinterWidget* m_ce158Printer = nullptr;
    DiskControl* m_disk = nullptr;
    AudioOutput* m_audio = nullptr;
    std::function<void(const QImage&, const QString&)> m_clipboardSink;
    std::string m_model;   // the control state's running model (the faceplate's art)

    calcu::shell::ShellProduct product();
    void onTopic(const QString& topic);
    void followControlState();
    void applyCopyLabel();
    void copyScreen();
    void refreshSerialStatus();

    // C1600-2: the two program flows, behind File ▸ Load BASIC Program… and
    // Load Machine Code… (after their dialog, which remembers the folder) and
    // an opened program file (onProgramOpened). `file`: the file of a
    // cassette WAV already chosen (from 1); none: asked when it has several.
    void chooseProgram(bool code);
    void loadBasicProgram(const QString& name, const QByteArray& bytes, std::optional<int> file = std::nullopt);
    void loadMachineCode(const QString& name, const QByteArray& bytes, std::optional<int> file = std::nullopt);
    std::optional<QByteArray> readChosen(const QString& path, const QString& title);
    std::optional<calcu::sessions::Value::Object> programArgs(const QString& name, const QByteArray& bytes,
                                                              const QString& title, std::optional<int> file);
    void showMachineCodeLoaded(const shared::CodeLoadedReply& loaded);
    void onProgramOpened(const QString& name, const QByteArray& bytes, const QString& kind);
    bool m_programFlow = false;
    // File ▸ Mount Directory… / Unmount (the host drive, AC-9: its owner's items).
    void mountHostDirectory();
    void unmountHostDirectory();

    // Host keys (LIB-5): the keys held down, so each key-up pairs with its
    // host-key-down (LIB-5 EC-1).
    calcu::qt6::HostKeys m_hostKeys;
    void releaseHeldKeys();
    // A host Shift tapped on its own is one press of the machine's SHIFT
    // (LIB-5 AC-33; the wall-time rule stays here, D-12).
    bool m_shiftTapArmed = false;
    QElapsedTimer m_shiftTapClock;
    void armShiftTap();
    void disarmShiftTap();
};
