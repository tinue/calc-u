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
#include "roms/RomSet.hpp"
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
// in one place (docs/reviews/d-simplification.md, P-2): every app layer
// (Calc-U-1600's SharpAppPart, Calc-U-59's from Part E), the harness's
// products and the library's test rigs get them from here instead of
// putting them together themselves.
//
// What it wires: LIB-18's machine control is made first and handed to every
// part that builds or reads a build (presets, startup, paper, media); the
// steps of paper, debugger and media go into LIB-6's vocabulary; LIB-13's
// `open` takes the presets and media parts it has. A part the product has no
// data for is not made (no typing product, no preset rules, no printers, no
// debugger, no kinds of medium).
//
// The order of the parts in the session (their designs' rules, in one
// place): LIB-4's ControlPart first, before the host's own parts (LIB-4
// D-13; controlPart()); then sessionParts(): LIB-5's TypingPart, LIB-2's
// PeripheralsPart before StartupPart (the clocks at a build, LIB-18 §3.5),
// LIB-6's PresetsPart before StartupPart (LIB-6 §3.8), StartupPart, LIB-7's
// MediaPart after it (LIB-7 §3.7), the product's parts the file kinds use,
// LIB-13's OpenPart after them, LIB-8's PaperPart, LIB-10's DebuggerPart; the
// product's own parts follow. Single-threaded, on the session's thread.

namespace calcu::parts {

struct LibraryPartsSetup {
    // Always: the product's catalogue, machines (LIB-18) and settings (LIB-9).
    const machine::Catalogue* catalogue = nullptr;
    startup::ProductMachines* machines = nullptr;
    settings::SettingsFile* settings = nullptr;
    // LIB-12: the full ROM set; without it the start is refused with
    // `romsMissing` (AC-5).
    std::shared_ptr<const roms::RomSet> roms;
    std::string romsMissing;

    // LIB-5: the product's typing; none: no TypingPart.
    const typing::TypingProduct* typing = nullptr;
    sessions::Hosting hosting = sessions::Hosting::Paced;
    // LIB-2: which serial port gets a serial port file, and their folder.
    peripherals::HostLinkRules hostLinks;
    std::optional<std::string> serialLinkFolder;
    // LIB-6: the product's preset rules; none: no PresetsPart (and no steps).
    presets::ProductRules* presetRules = nullptr;
    const presets::PresetBundle* bundle = nullptr;
    std::string home;                                                // `~/`
    std::function<std::optional<std::string>()> screenshotFolder;   // AC-25
    // LIB-8: the papers of the product's printers; none: no PaperPart.
    std::vector<paper::PrinterPaper> papers;
    // LIB-10: the product's debugger; none: no DebuggerPart.
    std::optional<debugger::DebuggerSetup> debugger;
    // LIB-7: the product's kinds of medium; none: no MediaPart.
    std::vector<media::KindDecl> mediaKinds;
    const settings::Paths* mediaPaths = nullptr;   // the save folder and the tape folder
    media::ModuleListing moduleListing;
    // LIB-13: `open` (the product's name for its sentences, preset
    // extensions, file kinds in the order they are asked, limits per way).
    std::string productName;
    std::vector<std::string> presetExtensions;
    std::vector<opening::FileKind*> fileKinds;
    std::map<opening::Way, std::uint64_t> openLimits;
    // LIB-4: the printers that may own the paper panel and the product's rules.
    std::vector<machine::PeripheralId> printers;
    std::vector<const control::ControlRules*> controlRules;
};

class LibraryParts {
public:
    explicit LibraryParts(LibraryPartsSetup);
    ~LibraryParts();
    LibraryParts(const LibraryParts&) = delete;
    LibraryParts& operator=(const LibraryParts&) = delete;

    // The parts in the session's order (above), `fileKindParts` (the
    // product's parts its file kinds use) before OpenPart; ControlPart not
    // among them.
    std::vector<sessions::SessionPart*> sessionParts(const std::vector<sessions::SessionPart*>& fileKindParts = {});
    control::ControlPart& controlPart() { return *control_; }

    startup::MachineControl& control() { return machineControl_; }
    startup::StartupPart& startup() { return *startup_; }
    peripherals::PeripheralsPart& peripherals() { return *peripherals_; }
    const peripherals::PeripheralsPart& peripherals() const { return *peripherals_; }
    opening::OpenPart& open() { return *open_; }
    // Null where the product has none (above).
    typing::TypingPart* typing() { return typing_.get(); }
    presets::PresetsPart* presets() { return presets_.get(); }
    paper::PaperPart* paper() { return paper_.get(); }
    debugger::DebuggerPart* debugger() { return debugger_.get(); }
    media::MediaPart* media() { return media_.get(); }

    const machine::Catalogue& catalogue() const { return *setup_.catalogue; }
    settings::SettingsFile& settings() { return *setup_.settings; }
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
    std::unique_ptr<opening::PresetsOpener> presetsOpener_;
    std::unique_ptr<opening::MediaOpener> mediaOpener_;
    std::unique_ptr<opening::OpenPart> open_;
    std::unique_ptr<control::ControlPart> control_;
};

} // namespace calcu::parts
