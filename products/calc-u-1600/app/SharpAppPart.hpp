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

#include "Debug/Inspect/Inspector.hpp"
#include "Display/LcdScreenshot.hpp"
#include "Display/LcdText.hpp"
#include "Job.hpp"
#include "Machine/SharpProductKinds.hpp"
#include "PC1500/PC1500Variant.hpp"
#include "Tape/TapeDeck.hpp"
#include "TraceTypes.hpp"
#include "app/AppPaths.hpp"
#include "app/ControlState.hpp"
#include "app/DisplayFrame.hpp"
#include "app/MachineModel.hpp"
#include "app/PacingProfiles.hpp"
#include "app/Settings.hpp"
#include "app/SharpFileKinds.hpp"
#include "app/SharpLoaderPart.hpp"
#include "app/SharpPresetRules.hpp"
#include "app/SharpProductMachines.hpp"
#include "debugger/DebuggerPart.hpp"
#include "app/SharpTopics.hpp"
#include "app/SharpTyping.hpp"
#include "machine/Catalogue.hpp"
#include "machine/Machine.hpp"
#include "media/MediaPart.hpp"
#include "opening/LibraryOpeners.hpp"
#include "opening/OpenPart.hpp"
#include "paper/PaperPart.hpp"
#include "peripherals/PeripheralsPart.hpp"
#include "presets/PresetsPart.hpp"
#include "presets/Source.hpp"
#include "startup/StartupPart.hpp"
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
// the operations, busy, sound and the messages. This part brings the
// library's parts Calc-U-1600 uses and what only Calc-U-1600 has (LIB-3
// design §2, §4; docs/contracts/README.md L-1):
// - the library's parts, in sessionParts() order: LIB-5's typing part
//   (the Sharp profiles and maps, app/SharpTyping), LIB-2's peripherals
//   part (the host links of the serial ports, the real-time clocks), LIB-6's
//   presets part (the preset loader on app/SharpPresetRules, the bundle
//   file of examples/), LIB-18's startup part (the start, the model and
//   variant switches, attach and detach, Reset, Cold boot, power cycle, on
//   app/SharpProductMachines), the loaders' part (C1600-2), LIB-8's paper
//   part, LIB-10's debugger part (LIB-6 §10.2, LIB-18 §10.2) and LIB-7's media
//   part (the modules, the floppy disk and the tape; app/SharpMediaKinds),
//   then LIB-13's open part (the command `open` of a drop, Open With, the
//   start argument and Load Preset…; the product's file kinds in
//   app/SharpFileKinds);
// - the product's commands that need its own data, each one
//   MachineControl::change (LIB-18 D-22):
//   `c1600.mount-host-drive`, `c1600.ce1600p-rom` (SharpTopics.hpp);
// - the interim product topics (control state, machine, serial
//   links) and LIB-12's `roms-missing`.
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

    std::vector<calcu::sessions::SessionPart*> sessionParts();

    Settings& settings() { return m_settings; }
    AppPaths& paths() { return m_paths; }
    // LIB-7: the media (modules, disks, tapes): the topic `media`, the
    // commands `media.*`, autosave (app/SharpMediaKinds for the formats).
    calcu::media::MediaPart& mediaPart() { return *m_media; }
    const calcu::media::MediaPart& mediaPart() const { return *m_media; }
    SharpLoaderPart& loaderPart() { return m_loaderPart; }
    // LIB-13: `open`, `open-would-take`; a Web host sets its upload folder
    // (setup().uploadFolder) before the session is built.
    calcu::opening::OpenPart& openPart() { return *m_open; }
    // The typing part (LIB-5): frontends send its commands (host-key-down,
    // screen-key-down, paste, ...); the debugger's auto-start types its line
    // through the engine directly (not a user key, so not dropped while busy).
    calcu::typing::TypingPart& typingPart() { return m_typingPart; }
    // The papers of the two plotters (LIB-8): kept from the start of the
    // session to its end, whatever machine runs.
    calcu::paper::PaperPart& paperPart() { return m_paper; }
    // The library's debugger (LIB-10): the run control, the trace session,
    // the topics run-state and trace. The DAP's layer (app/dap/) works on it.
    calcu::debugger::DebuggerPart& debuggerPart() { return m_debugger; }
    // LIB-6: the preset loader (load-preset, check-preset, preset-list).
    calcu::presets::PresetsPart& presetsPart() { return m_presets; }
    const calcu::presets::PresetBundle& presetBundle() const { return m_bundle; }
    // The preset list (LIB-6 AC-32): what `preset-list` replies, for the
    // menus and the Settings of a frontend on the session's thread.
    std::vector<calcu::presets::PresetListEntry> presetEntries() const {
        return calcu::presets::presetList(m_bundle, m_rules, m_catalogue, nullptr);
    }
    // LIB-18: the machine control (builds, plug changes).
    calcu::startup::MachineControl& machineControl() { return m_startup.control(); }

    // File ▸ Mount Directory is offered when the frontend states a folder
    // picker (LIB-3 AC-38: 0.8.1's hostDirectories; a browser has none).
    ControlState controlState() const {
        return controlStateOf(*this, hostFolders());
    }
    bool hostFolders() const { return m_ctx && m_ctx->hostOffers(calcu::sessions::HostService::FolderPicker); }

    // ---- SessionPart ----
    void attach(calcu::sessions::SessionSetup&) override;
    void afterCall(calcu::sessions::SessionContext&, calcu::pacing::WallTime) override;
    void machineChanged(calcu::sessions::SessionContext&) override;
    void operationEnded(calcu::sessions::SessionContext&) override;

    // ---- The machine ----
    calcu::machine::Machine* machine() const { return m_ms ? m_ms->machine() : nullptr; }
    bool hasLiveMachine() const { return machine() != nullptr; }
    Model currentModel() const { return m_model; }
    // The variants of the running machine, and the CE-1600P's ROM: the
    // plugged one's, else the choice kept for the next attach (AC-31, D-21).
    PC1500RomRevision pc1500RomRevision() const;
    PC1600RomVersion pc1600RomVersion() const;
    CE1600PRomVersion ce1600pRomVersion() const;
    double clockHz() const;
    const calcu::machine::Catalogue& catalogue() const { return m_catalogue; }
    const std::shared_ptr<const calcu::roms::RomSet>& romSet() const { return m_roms; }
    const calcu::roms::RomsMissing& romsMissing() const { return m_romsMissing; }

    // The LCD as text and as a screen image, from a display output of the
    // `display` topic (the machine supplies the font and the cursor RAM).
    bool lcdText(const calcu::machine::DisplayOutput& output, LcdText* out) const;
    GrayImage screenImage(const calcu::machine::DisplayOutput& output) const;

    // ---- Expansion slots ----
    int slotCount() const;
    const ExpansionCard* slotCard(int slot) const;

    // ---- Serial ports ----
    void refreshSerialLinkDirectory();
    std::string serialLinkStatus() const;
    std::string ce158SerialLinkStatus() const;

    // ---- Debug panel ----
    std::vector<std::string> inspectorView(inspect::View view) const;
    std::vector<inspect::MenuEntry> inspectorMenu(bool dumps) const;
    std::vector<std::uint8_t> debugSlotCardImage(int slot, int* bankCount) const;

    // ---- Peripherals (what is plugged in) ----
    bool ce150Attached() const;
    bool ce1600pAttached() const;
    bool ce158Attached() const;
    std::vector<std::uint8_t> drainCE158PrinterOutput();
    bool hostDriveAttached() const;
    std::string hostDriveDirectory() const;

    // ---- Cassette recorder and CE-1600F (the media: mediaPart()) ----
    bool tapeInterfaceAttached() const;
    TapeDeck::Status tapeStatus() const;   // the counter (AC-42)
    bool floppyMotorOn() const;            // the drive's lamp (AC-39)

private:
    using CommandOutcome = calcu::sessions::CommandOutcome;
    // A plug change of a product command as an operation (LIB-18 D-22).
    CommandOutcome change(calcu::startup::PlugChange ch, std::function<void()> after = {});
    void refreshTopics();
    void notice(const std::string& sentence);
    void log(const std::string& line);
    // The model and the counters of the machine topic follow a new machine.
    void followNewMachine();
    std::string pluggedAt(const char* place) const;
    template <class K> K* kind() const { return machine() ? sharp::firstOf<K>(*machine()) : nullptr; }

    Settings& m_settings;
    AppPaths& m_paths;

    Model m_model = Model::PC1500A;
    // AC-31, D-21: the CE-1600P's ROM chosen while none is plugged in, until
    // the product quits or the model is switched.
    CE1600PRomVersion m_ce1600pChoice = CE1600PRomVersion::New;

    calcu::machine::Catalogue m_catalogue;
    // The one lookup, at the start (LIB-12 AC-4, AC-13): the whole full set
    // or nothing. The host drive's ROM is bundled beside it (roms.md Q-1).
    calcu::roms::LookupResult m_found;
    std::shared_ptr<const calcu::roms::RomSet> m_roms;
    calcu::roms::RomsMissing m_romsMissing;
    std::shared_ptr<const std::vector<std::uint8_t>> m_hostDriveRom;

    calcu::paper::PaperPart m_paper;
    calcu::debugger::DebuggerPart m_debugger;
    // LIB-2's part: the host links outlive the machines (a stable serial
    // port file doesn't come and go with a model switch).
    calcu::peripherals::PeripheralsPart m_peripherals;
    calcu::sessions::SessionContext* m_ctx = nullptr;
    calcu::sessions::MachineSession* m_ms = nullptr;
    calcu::typing::TypingPart m_typingPart{typingProduct()};
    // C1600-2: the loaders' commands (app/SharpLoaderPart).
    SharpLoaderPart m_loaderPart{m_typingPart};
    // LIB-18 and LIB-6 (§10.2 of their designs).
    SharpProductMachines m_machines;
    SharpPresetRules m_rules;
    calcu::presets::DirectoryPresetBundle m_bundle;
    calcu::presets::PresetsPart m_presets;
    calcu::startup::StartupPart m_startup;
    std::uint64_t m_seenGeneration = 0;
    // LIB-7 (§10.2): after StartupPart among the parts.
    std::unique_ptr<calcu::media::MediaPart> m_media;
    // LIB-13 (Part D): the open part over the presets and the media, with
    // the two file kinds, after the loaders among the parts.
    std::unique_ptr<calcu::opening::PresetsOpener> m_presetsOpener;
    std::unique_ptr<calcu::opening::MediaOpener> m_mediaOpener;
    BasicProgramKind m_basicKind;
    MachineCodeKind m_codeKind;
    std::unique_ptr<calcu::opening::OpenPart> m_open;

    // The topics (attach).
    calcu::sessions::StateTopic* m_controlTopic = nullptr;
    calcu::sessions::StateTopic* m_machineTopic = nullptr;
    calcu::sessions::StateTopic* m_serialTopic = nullptr;
    calcu::sessions::StateTopic* m_romsMissingTopic = nullptr;
    MachinePayload m_machineState;
    std::uint64_t m_serialMoves = 0;
};

} // namespace shared
