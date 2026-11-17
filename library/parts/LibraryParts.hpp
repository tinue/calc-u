#pragma once
#include "control/ControlPart.hpp"
#include "debugger/DebuggerPart.hpp"
#include "machine/Catalogue.hpp"
#include "media/MediaPart.hpp"
#include "opening/LibraryOpeners.hpp"
#include "opening/OpenPart.hpp"
#include "paper/PaperPart.hpp"
#include "peripherals/PeripheralsPart.hpp"
#include "presets/PresetsPart.hpp"
#include "roms/Lookup.hpp"
#include "sessions/SessionPart.hpp"
#include "settings/Paths.hpp"
#include "settings/SettingsFile.hpp"
#include "startup/MachineControl.hpp"
#include "startup/ProductMachines.hpp"
#include "startup/StartupPart.hpp"
#include "typing/TypingPart.hpp"
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

// The library's session parts of a product's app session, built and wired
// in one place (docs/reviews/d-simplification.md, P-2; Part E, E-1): every
// app layer (Calc-U-1600's SharpAppPart, Calc-U-59's TIAppPart), the
// harness's products and the library's test rigs get them from here instead
// of putting them together themselves.
//
// What it wires: LIB-18's machine control is made first and handed to every
// part that builds or reads a build (presets, startup, paper, media); the
// steps of paper, debugger and media go into LIB-6's vocabulary; LIB-13's
// `open` takes the presets and media parts. Every part is made: a product
// without printers has an inert PaperPart, one without kinds of medium an
// inert MediaPart. The host passes its LIB-9 places (`paths`) and LIB-12's
// lookup (`roms`); from them come the trace folder and limit of the
// debugger (LIB-10), the folder for screenshots (LIB-6 AC-25), the save and
// tape folders of the media (LIB-7), `~/` of a preset, and LIB-12's topic
// `roms-missing` (StartupPart publishes it).
//
// The order of the parts in the session (their designs' rules, in one
// place; sessionParts()): LIB-4's ControlPart first, before the host's own
// parts (LIB-4 D-13); then LIB-5's TypingPart, LIB-2's PeripheralsPart
// before StartupPart (the clocks at a build, LIB-18 §3.5), LIB-6's
// PresetsPart before StartupPart (LIB-6 §3.8), StartupPart, LIB-7's
// MediaPart after it (LIB-7 §3.7), the product's parts the file kinds use,
// LIB-13's OpenPart after them, LIB-8's PaperPart, LIB-10's DebuggerPart;
// then the product's own parts and the host's parts behind them.
// Single-threaded, on the session's thread.

namespace calcu::parts {

struct LibraryPartsSetup {
    // Always: the product's machines (LIB-18; their catalogue is the
    // session's), settings and places (LIB-9; both outlive the parts) and the
    // ROM lookup (LIB-12: without the full set the start is refused, AC-5).
    startup::ProductMachines* machines = nullptr;
    settings::SettingsFile* settings = nullptr;
    const settings::Paths* paths = nullptr;
    roms::LookupResult roms;
    // Whether the trace folder can be used now (the Swift frontend's iCloud,
    // C59-4 AC-12); null: always.
    std::function<bool()> traceAvailable;
    // Calc-U-59 (its behaviour before Part E, a question for the user:
    // LIB-10's Part E simplification E-1): the places as they are when the
    // parts are made (the save and tape folders of the media, the folder for
    // screenshots, the trace folder, made when a trace starts and not
    // checked by a write) and the trace limit of that moment; false: each
    // read when used (Calc-U-1600).
    bool placesAtStart = false;
    // Set: the folder for screenshots instead of the places' (Calc-U-59's
    // Web: its save folder, where the places have none, LIB-9 AC-17; a
    // question for the user: LIB-6's Part E simplification E-1).
    std::optional<std::string> screenshotFolder;

    // LIB-5: the product's typing.
    const typing::TypingProduct* typing = nullptr;
    sessions::Hosting hosting = sessions::Hosting::Paced;
    // LIB-2: which serial port gets a serial port file, and their folder.
    peripherals::HostLinkRules hostLinks;
    std::optional<std::string> serialLinkFolder;
    // LIB-6: the product's preset rules and bundled presets.
    presets::ProductRules* presetRules = nullptr;
    const presets::PresetBundle* bundle = nullptr;
    // LIB-8: the papers of the product's printers; LIB-4: these printers may
    // own the paper panel.
    std::vector<paper::PrinterPaper> papers;
    // LIB-10: the product's debugger; its trace folder and limit come from
    // `paths` and `settings` ([trace] max-file-size-mb).
    debugger::DebuggerSetup debugger;
    // LIB-7: the product's kinds of medium.
    std::vector<media::KindDecl> mediaKinds;
    media::ModuleListing moduleListing;
    std::shared_ptr<const media::FileAccess> mediaFiles;   // null: plain (D-30)
    // LIB-13: `open` (the product's name for its sentences, preset
    // extensions, file kinds in the order they are asked, limits per way).
    std::string productName;
    std::vector<std::string> presetExtensions;
    std::vector<opening::FileKind*> fileKinds;
    std::map<opening::Way, std::uint64_t> openLimits;
    // LIB-13 AC-8: where several files become one folder (the Web hosts'
    // uploads folder); none: several files are opened one by one.
    std::optional<std::string> uploadFolder;
    // LIB-4: the product's rules.
    std::vector<const control::ControlRules*> controlRules;
};

// The parts of others, in their places of the session's order (above).
struct SessionSlots {
    std::vector<sessions::SessionPart*> host;        // the host's own, after ControlPart
    std::vector<sessions::SessionPart*> fileKinds;   // the product's parts its file kinds use, before OpenPart
    std::vector<sessions::SessionPart*> product;     // the product's own, after DebuggerPart
    std::vector<sessions::SessionPart*> after;       // the host's, behind the product's
};

class LibraryParts {
public:
    explicit LibraryParts(LibraryPartsSetup);
    ~LibraryParts();
    LibraryParts(const LibraryParts&) = delete;
    LibraryParts& operator=(const LibraryParts&) = delete;

    // Every part of the session in its order (above), ControlPart first.
    std::vector<sessions::SessionPart*> sessionParts(const SessionSlots&);
    control::ControlPart& controlPart() { return *control_; }

    startup::MachineControl& control() { return machineControl_; }
    startup::StartupPart& startup() { return *startup_; }
    peripherals::PeripheralsPart& peripherals() { return *peripherals_; }
    const peripherals::PeripheralsPart& peripherals() const { return *peripherals_; }
    opening::OpenPart& open() { return *open_; }
    typing::TypingPart& typing() { return *typing_; }
    const typing::TypingPart& typing() const { return *typing_; }
    presets::PresetsPart& presets() { return *presets_; }
    const presets::PresetsPart& presets() const { return *presets_; }
    paper::PaperPart& paper() { return *paper_; }
    const paper::PaperPart& paper() const { return *paper_; }
    debugger::DebuggerPart& debugger() { return *debugger_; }
    const debugger::DebuggerPart& debugger() const { return *debugger_; }
    media::MediaPart& media() { return *media_; }
    const media::MediaPart& media() const { return *media_; }

    const machine::Catalogue& catalogue() const { return setup_.machines->catalogue(); }
    settings::SettingsFile& settings() { return *setup_.settings; }
    const settings::Paths& paths() const { return *setup_.paths; }
    const presets::PresetBundle* bundle() const { return setup_.bundle; }

private:
    LibraryPartsSetup setup_;
    std::unique_ptr<settings::Paths> startPlaces_;   // placesAtStart
    startup::MachineControl machineControl_;
    std::unique_ptr<typing::TypingPart> typing_;
    std::unique_ptr<peripherals::PeripheralsPart> peripherals_;
    std::unique_ptr<presets::PresetsPart> presets_;
    std::unique_ptr<startup::StartupPart> startup_;
    std::unique_ptr<paper::PaperPart> paper_;
    std::unique_ptr<debugger::DebuggerPart> debugger_;
    std::unique_ptr<media::MediaPart> media_;
    std::unique_ptr<opening::PresetsOpener> presetsOpener_;
    std::unique_ptr<opening::MediaOpener> mediaOpener_;
    std::unique_ptr<opening::OpenPart> open_;
    std::unique_ptr<control::ControlPart> control_;
};

} // namespace calcu::parts
