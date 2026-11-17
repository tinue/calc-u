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
#include "Machine/SharpCatalogue.hpp"
#include "Machine/SharpProductKinds.hpp"
#include "PC1500/PC1500Variant.hpp"
#include "Tape/TapeDeck.hpp"
#include "TraceTypes.hpp"
#include "app/AppPaths.hpp"
#include "app/PacingProfiles.hpp"
#include "app/Settings.hpp"
#include "app/SharpControlRules.hpp"
#include "app/SharpFileKinds.hpp"
#include "app/SharpLoaderPart.hpp"
#include "app/SharpPresetRules.hpp"
#include "app/SharpProductMachines.hpp"
#include "control/ControlPart.hpp"
#include "debugger/DebuggerPart.hpp"
#include "app/SharpTopics.hpp"
#include "app/SharpTyping.hpp"
#include "machine/Catalogue.hpp"
#include "machine/Machine.hpp"
#include "media/MediaPart.hpp"
#include "opening/OpenPart.hpp"
#include "paper/PaperPart.hpp"
#include "parts/LibraryParts.hpp"
#include "peripherals/PeripheralsPart.hpp"
#include "presets/PresetsPart.hpp"
#include "presets/Source.hpp"
#include "startup/StartupPart.hpp"
#include "roms/Lookup.hpp"
#include "sessions/AppSession.hpp"
#include "sessions/MachineSession.hpp"
#include "sessions/SessionPart.hpp"

class ExpansionCard;

namespace shared::dap {
class DapPart;
}

namespace shared {


// What an operation reports: ok, or why not (a sentence for the user).
struct OpResult {
    bool ok = true;
    std::string error;
    static OpResult fail(std::string why) { return OpResult{false, std::move(why)}; }
};

// What differs by frontend (LIB-4 AC-18, D-12): a browser cannot mount a
// host folder.
struct SharpAppPartOptions {
    bool canMountHostFolder = true;
    // The harness (LIB-11 D-6): typing in emulated time, no serial port file
    // unless asked for (requestSerialLink; C1600-5 AC-9).
    calcu::sessions::Hosting hosting = calcu::sessions::Hosting::Paced;
    bool serialLinksOnRequest = false;
    // The Web: where several files of Upload Preset… become one folder
    // (LIB-13 AC-8); unset: none.
    std::optional<std::string> uploadFolder;
};

// ── SharpAppPart: Calc-U-1600's app layer in the library's app session ───
//
// The library's app session (LIB-3) holds the machine session, the pacer,
// the operations, busy, sound and the messages. This part brings the
// library's parts Calc-U-1600 uses and what only Calc-U-1600 has (LIB-3
// design §2, §4; docs/contracts/README.md L-1):
// - the library's parts (parts::LibraryParts, which builds, wires and
//   orders them) on Calc-U-1600's data: the typing profiles and maps
//   (app/SharpTyping), the serial port files, the preset rules
//   (app/SharpPresetRules) and the bundle file of examples/, the machines
//   (app/SharpProductMachines), the printers' papers (app/SharpPrinters), the
//   debugger (app/SharpDebuggerSetup), the kinds of medium
//   (app/SharpMediaKinds), the file kinds (app/SharpFileKinds), the rules of
//   the control state (app/SharpControlRules); among them, before the open
//   part, the loaders' part (C1600-2), and after them the DAP server
//   (C1600-3) and this part;
// - the product's commands that need its own data, each one
//   MachineControl::change (LIB-18 D-22):
//   `c1600.mount-host-drive`, `c1600.ce1600p-rom` (SharpTopics.hpp);
// - the product's topics (`c1600.lamps`, `c1600.serial-links`).
//
// The host builds it, hands sessionParts() (every part, LIB-4's ControlPart
// first, its own among them) to its AppSessionSetup (and the part to
// AppSessionSetup::owned), then sends
// `start`. Single-threaded: on
// the session's thread.
class SharpAppPart final : public calcu::sessions::SessionPart {
public:
    SharpAppPart(Settings& settings, AppPaths& paths, SharpAppPartOptions options = {});
    ~SharpAppPart();
    SharpAppPart(const SharpAppPart&) = delete;
    SharpAppPart& operator=(const SharpAppPart&) = delete;

    // Every part of the session in the one order (parts::LibraryParts): the
    // control part, the host's `host`, the library's with the loaders before
    // the open part, the DAP server and this part, then the host's `after`.
    std::vector<calcu::sessions::SessionPart*> sessionParts(std::vector<calcu::sessions::SessionPart*> host = {},
                                                            std::vector<calcu::sessions::SessionPart*> after = {});

    Settings& settings() { return m_settings; }
    AppPaths& paths() { return m_paths; }
    // LIB-7: the media (modules, disks, tapes): the topic `media`, the
    // commands `media.*`, autosave (app/SharpMediaKinds for the formats).
    calcu::media::MediaPart& mediaPart() { return m_lib.media(); }
    SharpLoaderPart& loaderPart() { return m_loaderPart; }
    // LIB-13: `open`, `open-would-take` (a Web host's upload folder:
    // SharpAppPartOptions::uploadFolder).
    calcu::opening::OpenPart& openPart() { return m_lib.open(); }
    // The typing part (LIB-5): frontends send its commands (host-key-down,
    // screen-key-down, paste, ...); the debugger's auto-start types its line
    // through the engine directly (not a user key, so not dropped while busy).
    calcu::typing::TypingPart& typingPart() { return m_lib.typing(); }
    // The papers of the two plotters (LIB-8): kept from the start of the
    // session to its end, whatever machine runs.
    calcu::paper::PaperPart& paperPart() { return m_lib.paper(); }
    // The library's debugger (LIB-10): the run control, the trace session,
    // the topics run-state and trace. The DAP's layer (app/dap/) works on it.
    calcu::debugger::DebuggerPart& debuggerPart() { return m_lib.debugger(); }
    // LIB-6: the preset loader (load-preset, check-preset, preset-list).
    calcu::presets::PresetsPart& presetsPart() { return m_lib.presets(); }
    const calcu::presets::PresetBundle& presetBundle() const { return m_bundle; }
    SharpPresetRules& presetRules() { return m_rules; }
    // C1600-3: the DAP server (app/dap/DapPart); nullptr in a Web build,
    // which has none (AC-1). The host hands it its socket (setTransport).
    dap::DapPart* dapPart() { return m_dap.get(); }
    // LIB-18: the machine control (builds, plug changes).
    calcu::startup::MachineControl& machineControl() { return m_lib.control(); }
    // The library's parts (the harness takes them from the app layer, LIB-11 D-6).
    calcu::parts::LibraryParts& libraryParts() { return m_lib; }
    calcu::peripherals::PeripheralsPart& peripheralsPart() { return m_lib.peripherals(); }
    SharpProductMachines& productMachines() { return m_machines; }
    // C1600-5 AC-7, AC-9: with serialLinksOnRequest, the serial port file of
    // `fileName` ("calcu1600-ce158.serial", "calcu1600-rs232c.serial") is made
    // from now on; its path, or empty where the host has no pseudo-terminals.
    std::string requestSerialLink(const std::string& fileName);
    bool serialLinked(const std::string& fileName) const;

    // The place host-drive can be used (AC-18, D-12): not in a browser.
    bool canMountHostFolder() const { return m_options.canMountHostFolder; }

    // ---- SessionPart ----
    void attach(calcu::sessions::SessionSetup&) override;
    void afterCall(calcu::sessions::SessionContext&, calcu::pacing::WallTime) override;
    void machineChanged(calcu::sessions::SessionContext&) override;
    void operationEnded(calcu::sessions::SessionContext&) override;

    // ---- The machine ----
    calcu::machine::Machine* machine() const { return m_ms ? m_ms->machine() : nullptr; }
    bool hasLiveMachine() const { return machine() != nullptr; }
    // The running model (the startup model before the first build).
    calcu::machine::ModelId model() const;
    bool isPC1600() const { return model().value == sharp::kPC1600; }
    // The CE-1600P's variant ("New", "Old"): the plugged one's, else the
    // choice kept for the next attach (LIB-18 AC-31, D-21).
    std::string ce1600pVariant() const;
    double clockHz() const;
    const calcu::machine::Catalogue& catalogue() const { return m_catalogue; }
    const std::shared_ptr<const calcu::roms::RomSet>& romSet() const { return m_found.set; }
    // The one lookup at the start (LIB-12).
    const calcu::roms::LookupResult& romLookup() const { return m_found; }

    // The LCD as text and as a screen image, from a display output of the
    // `display` topic (the machine supplies the font and the cursor RAM).
    bool lcdText(const calcu::machine::DisplayOutput& output, LcdText* out) const;
    GrayImage screenImage(const calcu::machine::DisplayOutput& output) const;

    // ---- Expansion slots ----
    int slotCount() const;
    // The module in a slot (from 1), by its name; empty when none.
    std::string slotModuleName(int slot) const;

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
    calcu::parts::LibraryPartsSetup librarySetup();   // the constructor's, on the members before m_lib
    // A plug change of a product command as an operation (LIB-18 D-22).
    CommandOutcome change(calcu::startup::PlugChange ch, std::function<void()> after = {});
    void refreshTopics();
    void notice(const std::string& sentence);
    void log(const std::string& line);
    // The model and the counters of the machine topic follow a new machine.
    void followNewMachine();
    std::string pluggedAt(const char* place) const;
    template <class K> K* kind() const { return machine() ? calcu::machine::first<K>(*machine()) : nullptr; }

    Settings& m_settings;
    AppPaths& m_paths;

    SharpAppPartOptions m_options;
    std::string m_lastModel;   // the model of the last build (followNewMachine)
    // AC-31, D-21: the CE-1600P's ROM chosen while none is plugged in, until
    // the product quits or the model is switched.
    std::string m_ce1600pChoice = "New";

    // The one lookup, at the start (LIB-12 AC-4, AC-13): the whole full set
    // or nothing. The host drive's ROM is bundled beside it (roms.md Q-1).
    calcu::roms::LookupResult m_found;
    std::shared_ptr<const std::vector<std::uint8_t>> m_hostDriveRom;
    // The catalogue with the module definitions (the bundled ones and the
    // set's CE-502B; LIB-12 AC-14).
    calcu::machine::Catalogue m_catalogue;

    calcu::sessions::SessionContext* m_ctx = nullptr;
    calcu::sessions::MachineSession* m_ms = nullptr;
    std::vector<std::string> m_requestedLinks;   // serialLinksOnRequest: the files asked for
    std::uint64_t m_seenGeneration = 0;

    // The product's data for the library's parts.
    SharpProductMachines m_machines;
    SharpPresetRules m_rules;
    calcu::presets::DirectoryPresetBundle m_bundle;
    BasicProgramKind m_basicKind;
    MachineCodeKind m_codeKind;
    SharpControlRules m_controlRules;
    calcu::parts::LibraryParts m_lib;
    // C1600-2: the loaders' commands (app/SharpLoaderPart).
    SharpLoaderPart m_loaderPart{m_lib.typing()};
    // C1600-3: the DAP server, after the debugger among the parts; not in
    // an Emscripten build.
    std::shared_ptr<dap::DapPart> m_dap;   // shared: no complete type where Emscripten leaves it out

    // The topics (attach).
    calcu::sessions::StateTopic* m_lampsTopic = nullptr;
    calcu::sessions::StateTopic* m_serialTopic = nullptr;
    std::uint64_t m_serialMoves = 0;
};

} // namespace shared
