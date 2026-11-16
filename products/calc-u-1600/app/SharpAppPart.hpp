#pragma once
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "Connector/AlpsPlotterMechanism.hpp"
#include "Debug/Inspect/Inspector.hpp"
#include "Display/LcdScreenshot.hpp"
#include "Display/LcdText.hpp"
#include "Job.hpp"
#include "Machine/SharpProductKinds.hpp"
#include "PC1500/PC1500Variant.hpp"
#include "Preset/PresetRomSet.hpp"
#include "Tape/TapeDeck.hpp"
#include "TraceTypes.hpp"
#include "app/AppPaths.hpp"
#include "app/ControlState.hpp"
#include "app/DisplayFrame.hpp"
#include "app/FloppyBay.hpp"
#include "app/ModuleBay.hpp"
#include "app/TapeBay.hpp"
#include "app/MachineModel.hpp"
#include "app/PacingProfiles.hpp"
#include "app/PlotterPaper.hpp"
#include "app/Settings.hpp"
#include "app/SharpLoaderPart.hpp"
#include "app/SharpMachineParts.hpp"
#include "app/SharpTopics.hpp"
#include "app/SharpTyping.hpp"
#include "machine/Catalogue.hpp"
#include "machine/Machine.hpp"
#include "peripherals/PeripheralsPart.hpp"
#include "roms/Lookup.hpp"
#include "sessions/AppSession.hpp"
#include "sessions/MachineSession.hpp"
#include "sessions/SessionPart.hpp"

class ExpansionCard;

namespace shared {


// What an operation reports: ok, or why not (a sentence for the user).
struct OpResult {
    bool ok = true;
    std::string error;
    static OpResult fail(std::string why) { return OpResult{false, std::move(why)}; }
};

// The display output as the frontends draw it today (adapter until M-49,
// LIB-14): every status symbol in kStatusSymbolNames order.
DisplayFrame displayFrameOf(const calcu::machine::DisplayOutput& output);

// ── SharpAppPart: Calc-U-1600's app layer in the library's app session ───
//
// The library's app session (LIB-3) holds the machine session, the pacer,
// the operations, busy, sound and the messages. This part brings what only
// Calc-U-1600 has (LIB-3 design §2, §4; docs/contracts/README.md L-1):
// - the machine of the selected model, built from sharpCatalogue() with the
//   ROM set of the one lookup at the start (LIB-12), and handed to the
//   machine session with its pacing profile; model and ROM switches (each
//   a rebuild and boot); the peripherals in their places; the tape deck,
//   plotters, CE-158, host drive, trace and inspector reads (through the
//   machine's capabilities and core/Machine/LegacyCalls.hpp);
// - the module, floppy and tape bays and their per-call duties;
// - the Sharp operations as product commands (`c1600.*`, SharpTopics.hpp)
//   that start an operation of the session;
// - the interim product topics (control state, machine, tapes, serial
//   links) and LIB-12's `roms-missing`;
// - the library's typing part (LIB-5) with the Sharp profiles and maps
//   (app/SharpTyping), first in sessionParts(); the adapter SharpDebuggerGate.
//
// The host builds it, hands sessionParts() to its AppSessionSetup (and the
// part to AppSessionSetup::owned), then sends `start`. Single-threaded: on
// the session's thread.
class SharpAppPart final : public calcu::sessions::SessionPart {
public:
    SharpAppPart(Settings& settings, AppPaths& paths);
    ~SharpAppPart();
    SharpAppPart(const SharpAppPart&) = delete;
    SharpAppPart& operator=(const SharpAppPart&) = delete;

    // The parts of the session, in order: the typing part, LIB-2's
    // peripherals part (the host links of the serial ports, the real-time
    // clocks), the loaders' part (C1600-2) and this one.
    std::vector<calcu::sessions::SessionPart*> sessionParts();
    // The real-time clocks of a machine built inside the start or an
    // operation are set from this; tests pin it.
    using HostTime = std::function<std::chrono::system_clock::time_point()>;

    Settings& settings() { return m_settings; }
    AppPaths& paths() { return m_paths; }
    ModuleBay& modules() { return *m_modules; }
    FloppyBay& floppy() { return *m_floppy; }
    TapeBay& tapes() { return *m_tapes; }
    const ModuleBay& modules() const { return *m_modules; }
    const FloppyBay& floppy() const { return *m_floppy; }
    const TapeBay& tapes() const { return *m_tapes; }
    SharpLoaderPart& loaderPart() { return m_loaderPart; }
    // The typing part (LIB-5): frontends send its commands (host-key-down,
    // screen-key-down, paste, ...); the debugger's auto-start types its line
    // through the engine directly (not a user key, so not dropped while busy).
    calcu::typing::TypingPart& typingPart() { return m_typingPart; }
    // The debugger (adapter SharpDebuggerGate, until M-45).
    void setDebugger(SharpDebugger* d) { m_gate.setDebugger(d); }

    // File ▸ Mount Directory is offered when the frontend states a folder
    // picker (LIB-3 AC-38: 0.8.1's hostDirectories; a browser has none).
    ControlState controlState() const {
        return controlStateOf(*this, m_ctx && m_ctx->hostOffers(calcu::sessions::HostService::FolderPicker));
    }

    // ---- SessionPart ----
    void attach(calcu::sessions::SessionSetup&) override;
    void afterCall(calcu::sessions::SessionContext&, calcu::pacing::WallTime) override;
    void machineChanged(calcu::sessions::SessionContext&) override;
    void operationEnded(calcu::sessions::SessionContext&) override;

    // ---- Operations (the commands of SharpTopics.hpp start them) ----
    job::Task<OpResult> startJob(std::string modelOrEmpty, std::string presetOrEmpty);
    job::Task<OpResult> loadPresetJob(std::string ref, bool armOnly = false);
    job::Task<OpResult> loadDefaultPresetJob(std::string ref, Model model);
    job::Task<OpResult> resetJob(bool allReset);
    job::Task<OpResult> selectModelJob(Model model);
    job::Task<OpResult> defaultPresetJob();
    job::Task<OpResult> pickModelJob(Model model);
    job::Task<OpResult> selectModuleJob(int slot, std::string moduleNameOrEmpty);
    job::Task<OpResult> selectPC1500RomJob(PC1500RomRevision revision);
    job::Task<OpResult> selectPC1600RomJob(PC1600RomVersion version);
    job::Task<OpResult> selectCE1600PRomJob(CE1600PRomVersion version);
    job::Task<OpResult> togglePeripheralJob(Peripheral p);
    job::Task<OpResult> mountHostDriveJob(std::string dirOrEmpty);

    // ---- The machine ----
    calcu::machine::Machine* machine() const { return m_ms ? m_ms->machine() : nullptr; }
    bool hasLiveMachine() const { return machine() != nullptr; }
    Model currentModel() const { return m_model; }
    PC1500RomRevision pc1500RomRevision() const { return m_pc1500RomRevision; }
    PC1600RomVersion pc1600RomVersion() const { return m_pc1600RomVersion; }
    CE1600PRomVersion ce1600pRomVersion() const { return m_ce1600pRomVersion; }
    double clockHz() const;
    const calcu::machine::Catalogue& catalogue() const { return m_catalogue; }
    const std::shared_ptr<const calcu::roms::RomSet>& romSet() const { return m_roms; }
    const calcu::roms::RomsMissing& romsMissing() const { return m_romsMissing; }
    PresetRoms presetRoms() const { return {m_roms, m_hostDriveRom}; }
    sharp::FloppyDriveAccess* floppyDrive() const;

    // The LCD as text and as a screen image, from a display output of the
    // `display` topic (the machine supplies the font and the cursor RAM).
    bool lcdText(const calcu::machine::DisplayOutput& output, LcdText* out) const;
    GrayImage screenImage(const calcu::machine::DisplayOutput& output) const;

    // ---- Expansion slots ----
    int slotCount() const;
    const ExpansionCard* slotCard(int slot) const;
    void setSlotWriteProtected(int slot, bool protect);
    void attachSlotCard(int slot, std::unique_ptr<ExpansionCard> card);
    void detachSlot(int slot);
    void detachSlots();

    // ---- Serial ports ----
    void refreshSerialLinkDirectory();
    std::string serialLinkStatus() const;
    std::string ce158SerialLinkStatus() const;

    // ---- Debug panel ----
    std::vector<std::string> inspectorView(inspect::View view) const;
    std::vector<inspect::MenuEntry> inspectorMenu(bool dumps) const;
    std::vector<std::uint8_t> debugSlotCardImage(int slot, int* bankCount) const;
    bool beginTrace(const std::string& path);
    void endTrace();
    std::uint64_t traceBytes() const;
    bool traceActive() const;

    // ---- Peripherals ----
    bool attachCE150(std::string* error = nullptr);
    void detachCE150();
    bool ce150Attached() const;
    bool attachCE1600P(std::string* error = nullptr);  // PC-1600 only
    void detachCE1600P();
    bool swapCE1600PRom(CE1600PRomVersion version, std::string* error = nullptr);
    bool ce1600pAttached() const;
    bool attachCE158(std::string* error = nullptr);
    void detachCE158();
    bool ce158Attached() const;
    std::vector<std::uint8_t> drainCE158PrinterOutput();
    bool attachHostDrive(const std::string& dir, std::string* error = nullptr);
    void detachHostDrive();
    bool hostDriveAttached() const;
    std::string hostDriveDirectory() const;

    // ---- Cassette recorder ----
    bool tapeInterfaceAttached() const;
    bool tapePlay(const std::string& path, std::string* error = nullptr);
    void tapeRecord(const std::string& path);
    bool tapeEject(std::string* error = nullptr);
    TapeDeck::Status tapeStatus() const;
    std::string tapeTakeLastError();

    // ---- Plotters ----
    std::vector<AlpsPlotterMechanism::FlatPoint> plotPoints(PaperKind kind) const;
    std::uint64_t plotRevision(PaperKind kind) const;
    void clearPaper(PaperKind kind);

    // ---- Lower-level machine jobs (the operations above use them) ----
    void resetRomSelectionsToDefault();
    job::Task<void> switchModelJob(Model model, bool keepPeripherals = false);
    job::Task<void> setPC1500RomRevisionJob(PC1500RomRevision revision);
    job::Task<void> setPC1600RomVersionJob(PC1600RomVersion version);
    void setCE1600PRomVersion(CE1600PRomVersion version) { m_ce1600pRomVersion = version; }
    job::Task<void> resetToPromptJob(bool allReset);
    job::Task<void> powerCycleAroundJob(std::function<void()> change);
    calcu::machine::Machine& resetBareForPreset(Model model, PC1600RomVersion version, CE1600PRomVersion ce1600pVersion);
    void finishPresetLoad(Model model);
    void seedClockFromHost();

private:
    using CommandOutcome = calcu::sessions::CommandOutcome;
    // A Sharp job as an operation of the session: pending card and disk
    // writes go first; the session replies at its end (LIB-3 AC-18).
    CommandOutcome operation(std::function<job::Task<OpResult>()> make);
    static calcu::sessions::Task<calcu::sessions::OperationResult> runOperation(SharpAppPart* self,
                                                                               calcu::sessions::OperationContext* ctx,
                                                                               std::function<job::Task<OpResult>()> make);
    void refreshTopics();
    void frameDuties();
    void notice(const std::string& sentence);
    void log(const std::string& line);

    // Drops the live machine before a rebuild: ejects the tape (saving a
    // recording), ends a TRACE capture, cancels typing, takes it away.
    void discardMachine();
    // A just-built machine joins the session (with its pacing profile).
    void installMachine(std::unique_ptr<calcu::machine::Machine> m);
    void buildMachine(Model model);
    [[noreturn]] void romMissing();
    struct Peripherals {
        bool ce150 = false;
        bool ce1600p = false;
        bool ce158 = false;
        std::string hostDriveDir;
    };
    Peripherals attachedPeripherals() const;
    void reattach(const Peripherals& p);
    template <class F> auto busChange(F&& change) {
        if (ce1600pAttached()) flushFloppyBay();
        return std::forward<F>(change)();
    }
    void flushFloppyBay();
    void insertSelectedDisk();
    bool peripheralImages(const calcu::machine::PeripheralRequest& req, std::vector<calcu::machine::RomImage>* out,
                          std::string* error) const;
    bool attachAt(const char* place, const char* peripheral, std::optional<calcu::machine::VariantId> variant,
                  std::shared_ptr<const calcu::machine::ProductData> content, std::string* error);
    void detachAt(const char* place, const char* peripheral);
    void noteChange(const calcu::machine::ChangeResult& r);
    std::string pluggedAt(const char* place) const;
    bool isPC1600() const { return machine() && m_model == Model::PC1600; }
    template <class K> K* kind() const { return machine() ? sharp::firstOf<K>(*machine()) : nullptr; }
    template <class K> K* kindOf(const char* peripheral) const;

    Settings& m_settings;
    AppPaths& m_paths;

    Model m_model = Model::PC1500A;
    PC1500RomRevision m_pc1500RomRevision = PC1500RomRevision::A04;
    PC1600RomVersion m_pc1600RomVersion = PC1600RomVersion::New;
    CE1600PRomVersion m_ce1600pRomVersion = CE1600PRomVersion::New;

    calcu::machine::Catalogue m_catalogue;
    std::shared_ptr<const calcu::roms::RomSet> m_roms;
    calcu::roms::RomsMissing m_romsMissing;
    std::shared_ptr<const std::vector<std::uint8_t>> m_hostDriveRom;   // roms.md Q-1: not looked up

    // LIB-2's part: the host links outlive the machines (a stable serial
    // port file doesn't come and go with a model switch).
    calcu::peripherals::PeripheralsPart m_peripherals;
    calcu::sessions::SessionContext* m_ctx = nullptr;
    calcu::sessions::MachineSession* m_ms = nullptr;
    calcu::typing::TypingPart m_typingPart{typingProduct()};
    // C1600-2: the loaders' commands (app/SharpLoaderPart).
    SharpLoaderPart m_loaderPart{m_typingPart};
    // The context of the operation that runs now (runOperation), for the
    // typing hand-off of a preset's typed program (C1600-2 design §3.10).
    calcu::sessions::OperationContext* m_opCtx = nullptr;
    SharpDebuggerGate m_gate;
    bool m_powerCycleFollows = false;

    std::unique_ptr<ModuleBay> m_modules;
    std::unique_ptr<FloppyBay> m_floppy;
    std::unique_ptr<TapeBay> m_tapes;

    // The topics (attach).
    calcu::sessions::StateTopic* m_controlTopic = nullptr;
    calcu::sessions::StateTopic* m_machineTopic = nullptr;
    calcu::sessions::StateTopic* m_tapesTopic = nullptr;
    calcu::sessions::StateTopic* m_serialTopic = nullptr;
    calcu::sessions::StateTopic* m_romsMissingTopic = nullptr;
    MachinePayload m_machineState;
    std::uint64_t m_tapeListChanges = 0;
    std::uint64_t m_serialMoves = 0;
    TapeDeck::Status m_tapeStatus;
};

} // namespace shared
