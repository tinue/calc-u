#pragma once
#include "control/ControlPart.hpp"
#include "debugger/DebuggerPart.hpp"
#include "machine/Catalogue.hpp"
#include "media/MediaPart.hpp"
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
// in one place: every app layer (Calc-U-1600's SharpAppPart, Calc-U-59's
// TIAppPart), the harness's products and the library's test rigs get them
// from here instead of putting them together themselves.
//
// What it wires: the machine control is made first and handed to every
// part that builds or reads a build (presets, startup, paper, media); the
// steps of paper, debugger and media go into the presets' vocabulary;
// `open` takes the presets and media parts. Every part is made: a product
// without printers has an inert PaperPart, one without kinds of medium an
// inert MediaPart. The host passes its places (`paths`) and the ROM lookup
// (`roms`); from them come the trace folder and limit of the debugger, the
// folder for screenshots, the save and cassette folders of the media, `~/`
// of a preset, and the topic `roms-missing` (StartupPart publishes it).
// Every place is read when it is used, so a change in Settings applies at
// once.
//
// The order of the parts in the session (sessionParts()): ControlPart
// first, before the host's own parts (the library adds it, so every
// session builder and the harness have one); then TypingPart,
// PeripheralsPart before StartupPart (the clocks at a build), PresetsPart
// before StartupPart, StartupPart, MediaPart after it, the product's parts
// the file kinds use, OpenPart after them, PaperPart, DebuggerPart; then
// the product's own parts and the host's parts behind them.
// Single-threaded, on the session's thread.

namespace calcu::parts {

struct LibraryPartsSetup {
    // Always: the product's machines (their catalogue is the session's),
    // settings and places (both outlive the parts) and the ROM lookup
    // (without the full set the start is refused).
    startup::ProductMachines* machines = nullptr;
    settings::SettingsFile* settings = nullptr;
    const settings::Paths* paths = nullptr;
    roms::LookupResult roms;

    // The product's typing.
    const typing::TypingProduct* typing = nullptr;
    sessions::Hosting hosting = sessions::Hosting::Paced;
    // Which serial port gets a serial port file, and their folder.
    peripherals::HostLinkRules hostLinks;
    std::optional<std::string> serialLinkFolder;
    // The product's preset rules and bundled presets.
    presets::ProductRules* presetRules = nullptr;
    const presets::PresetBundle* bundle = nullptr;
    // The papers of the product's printers; these printers may own the
    // paper panel.
    std::vector<paper::PrinterPaper> papers;
    // The product's debugger; its trace folder and limit come from
    // `paths` and `settings` ([trace] max-file-size-mb).
    debugger::DebuggerSetup debugger;
    // The product's kinds of medium.
    std::vector<media::KindDecl> mediaKinds;
    media::ModuleListing moduleListing;
    std::shared_ptr<const media::FileAccess> mediaFiles;   // null: plain (Swift: coordinated iCloud access)
    // `open` (the product's name for its sentences, preset
    // extensions, file kinds in the order they are asked, limits per way).
    std::string productName;
    std::vector<std::string> presetExtensions;
    std::vector<opening::FileKind*> fileKinds;
    std::map<opening::Way, std::uint64_t> openLimits;
    // Where several files become one folder (the Web hosts'
    // uploads folder); none: several files are opened one by one.
    std::optional<std::string> uploadFolder;
    // The Web's file cache (files/FileCache); none on the native hosts.
    std::optional<std::string> cacheFolder;
    // The product's control rules (null: none).
    const control::ControlRules* controlRules = nullptr;
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
    startup::MachineControl machineControl_;
    std::unique_ptr<typing::TypingPart> typing_;
    std::unique_ptr<peripherals::PeripheralsPart> peripherals_;
    std::unique_ptr<presets::PresetsPart> presets_;
    std::unique_ptr<startup::StartupPart> startup_;
    std::unique_ptr<paper::PaperPart> paper_;
    std::unique_ptr<debugger::DebuggerPart> debugger_;
    std::unique_ptr<media::MediaPart> media_;
    std::unique_ptr<opening::OpenPart> open_;
    std::unique_ptr<control::ControlPart> control_;
};

} // namespace calcu::parts
