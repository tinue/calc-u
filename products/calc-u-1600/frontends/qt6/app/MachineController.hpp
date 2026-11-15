#pragma once
#include <QObject>
#include <QString>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "Connector/AlpsPlotterMechanism.hpp"
#include "Debug/Inspect/Inspector.hpp"
#include "Display/LcdScreenshot.hpp"
#include "Display/LcdText.hpp"
#include "KeyPaste.hpp"
#include "Tape/TapeDeck.hpp"
#include "TraceTypes.hpp"
#include "app/MachineModel.hpp"
#include "app/AppSession.hpp"
#include "app/MachineSession.hpp"

class MemoryModuleManager;
class DebugController;
class FloppyDiskManager;

using shared::Model;
using shared::PC1600RomVersion;
using shared::CE1600PRomVersion;

inline QString modelSettingsKey(Model model) { return QString::fromLatin1(shared::modelKey(model)); }

using shared::DisplayFrame;

// Runs `op(std::string* error)` and copies a failure's message into the
// optional `error` -- the Shared calls' error form, for the Qt widgets.
template <class Op>
bool withQtError(QString* error, Op&& op) {
    std::string err;
    const bool ok = op(&err);
    if (!ok && error) *error = QString::fromStdString(err);
    return ok;
}

// The Qt side of shared::AppSession (Shared/AppSession.hpp and
// Shared/MachineSession.hpp have the behaviour and the docs): owns the
// app session, turns its Platform calls into signals, message boxes and
// the debugger hooks, and keeps the QString-typed calls the widgets use.
// The machine is session(), the rest app().
class MachineController : public QObject, private shared::AppSession::Platform {
    Q_OBJECT
public:
    explicit MachineController(QObject* parent = nullptr);
    ~MachineController() override;

    shared::AppSession& app() { return *m_app; }
    shared::MachineSession& session() { return m_app->machine(); }
    const shared::MachineSession& session() const { return m_app->machine(); }

    // ---- Forwarded unchanged (see MachineSession) ----
    Model currentModel() const { return session().currentModel(); }
    PC1500RomRevision pc1500RomRevision() const { return session().pc1500RomRevision(); }
    PC1600RomVersion pc1600RomVersion() const { return session().pc1600RomVersion(); }
    CE1600PRomVersion ce1600pRomVersion() const { return session().ce1600pRomVersion(); }
    void seedClockFromHost() { session().seedClockFromHost(); }

    void pressKey(const std::string& name) { session().pressKey(name); }
    void releaseKey(const std::string& name) { session().releaseKey(name); }
    void setOnKeyPressed(bool pressed) { session().setOnKeyPressed(pressed); }
    void pasteText(const std::string& text) { session().pasteText(text); }
    void typeCommand(const std::string& line) { session().typeCommand(line); }
    void cancelPaste() { session().cancelPaste(); }

    GrayImage currentScreenImage() const { return session().currentScreenImage(); }
    DisplayFrame currentDisplay() const { return session().currentDisplay(); }
    bool lcdText(LcdText* out) const { return session().lcdText(out); }

    // The live machine (calcu::machine::Machine; C1600-1), or nullptr.
    calcu::machine::Machine* machine() const { return session().machine(); }
    bool hasLiveMachine() const { return session().hasLiveMachine(); }

    std::vector<std::string> inspectorView(inspect::View view) const { return session().inspectorView(view); }
    std::vector<inspect::MenuEntry> inspectorMenu(bool dumps) const { return session().inspectorMenu(dumps); }
    void endTrace() { session().endTrace(); }
    std::uint64_t traceBytes() const { return session().traceBytes(); }
    bool traceActive() const { return session().traceActive(); }

    std::vector<std::uint8_t> drainCE158PrinterOutput() { return session().drainCE158PrinterOutput(); }
    bool hostDriveAttached() const { return session().hostDriveAttached(); }
    std::vector<AlpsPlotterMechanism::FlatPoint> plotPoints(shared::PaperKind kind) const {
        return session().plotPoints(kind);
    }
    std::uint64_t plotRevision(shared::PaperKind kind) const { return session().plotRevision(kind); }
    void clearPaper(shared::PaperKind kind) { session().clearPaper(kind); }

    // ---- QString translations ----
    bool beginTrace(const QString& path) { return session().beginTrace(path.toStdString()); }
    bool tapeEject(QString* error = nullptr);
    void refreshSerialLinkDirectory() { session().refreshSerialLinkDirectory(); }
    QString serialLinkStatus() const { return QString::fromStdString(session().serialLinkStatus()); }
    QString ce158SerialLinkStatus() const { return QString::fromStdString(session().ce158SerialLinkStatus()); }

    // ---- Qt side ----
    // The debugger (DAP server + session), see debug/DebugController.
    DebugController* debugController() const { return m_debug.get(); }
    // Starts, stops or rebinds the DAP server after a Settings change.
    void refreshDebugServer();

signals:
    void modelChanged(Model model);
    // refreshSerialLinkDirectory() moved the PTY symlinks.
    void serialLinksMoved();
    // See shared::AppSession::Platform.
    void bayModuleChanged(int slot);
    void tapeListChanged();
    void bayErrorMessage(shared::Bay bay, const QString& text);
    void presetWasArmed();
    void peripheralAttachFailed(const QString& device, const QString& reason);
    // An AppSession operation started / finished (emitted from inside it).
    void appBusyChanged(bool busy);

private:
    // shared::AppSession::Platform
    void romMissing(const std::string& error, const std::string& searchedIn) override;
    void notice(shared::MachineSession::Notice notice, const std::string& detail) override;
    void machineAboutToChange() override;
    void machineReplaced() override;
    void modelSwitched(Model model) override { emit modelChanged(model); }
    bool runDebuggerSlice(std::uint64_t cycles) override;
    void serialLinksRelinked() override { emit serialLinksMoved(); }
    void moduleChanged(int slot) override { emit bayModuleChanged(slot); }
    void tapesChanged() override { emit tapeListChanged(); }
    void bayError(shared::Bay bay, const std::string& text) override;
    void presetArmed() override { emit presetWasArmed(); }
    void presetLog(const std::string& line) override;
    void attachFailed(const std::string& device, const std::string& reason) override;
    void busyChanged(bool busy) override { emit appBusyChanged(busy); }

    std::unique_ptr<shared::AppSession> m_app;
    // Declared after the session so it goes first on destruction: its
    // debug target still refers to the live machine.
    std::unique_ptr<DebugController> m_debug;
};
