#pragma once
#include "control/Compute.hpp"
#include "harness/DisplayText.hpp"
#include "machine/Build.hpp"
#include "machine/Catalogue.hpp"
#include "machine/Descriptor.hpp"
#include "machine/Display.hpp"
#include "media/Kind.hpp"
#include "opening/OpenRequest.hpp"
#include "roms/Lookup.hpp"
#include "roms/RomSet.hpp"
#include "sessions/SessionPart.hpp"
#include "settings/Declaration.hpp"
#include "settings/SettingsFile.hpp"
#include "startup/ProductMachines.hpp"
#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

// The interface a product implements for the harness (LIB-11 design §3.6).
// The library depends on no product (LIB-1 AC-24): each product registers
// a factory with calcu_harness_add_product() (§3.7), and the harness runs
// the library's app session on the product's machine through it (§4).

namespace calcu::presets {
class ProductRules;
class PresetBundle;
} // namespace calcu::presets
namespace calcu::typing {
class TypingPart;
}
namespace calcu::paper {
class PaperPart;
}
namespace calcu::debugger {
class DebuggerPart;
}
namespace calcu::opening {
class FileKind;
}
namespace calcu::presets {
class PresetsPart;
}
namespace calcu::startup {
class MachineControl;
class StartupPart;
}
namespace calcu::peripherals {
class PeripheralsPart;
}
namespace calcu::control {
class ControlPart;
}
namespace calcu::media {
class MediaPart;
}
namespace calcu::opening {
class OpenPart;
}

namespace calcu::harness {

class CommandRegistry;
class ServeAddOn;
class RunSetup;

// LIB-11 D-6 (Part D, D.3; D-15): the library's parts as a product's app
// layer brings them, the same set its frontends host. A run whose product
// gives them builds none of these itself, so no part is in the session twice.
// The product has set them up (the paper and debugger steps, the media steps,
// the machine control of each); the run hands the debugger its trace folder.
// Every member is set.
struct AppLayerParts {
    peripherals::PeripheralsPart* peripherals = nullptr;
    presets::PresetsPart* presets = nullptr;
    startup::StartupPart* startup = nullptr;
    control::ControlPart* control = nullptr;
    media::MediaPart* media = nullptr;
    opening::OpenPart* open = nullptr;
};

// What a run makes of the library's parts, handed to the product before its
// session is built (C1600-3 design §3.5: the DAP's set-up loads presets,
// cold boots and reads the settings).
struct RunParts {
    presets::PresetsPart* presets = nullptr;     // when the product has preset rules
    startup::MachineControl* control = nullptr;
    settings::SettingsFile* settings = nullptr;  // the run's
};

class HarnessProduct {
public:
    virtual ~HarnessProduct() = default;
    virtual std::string_view name() const = 0;          // "Calc-U-59" (AC-1)
    virtual std::string_view shortName() const = 0;     // "59"
    virtual std::string_view commandWord() const = 0;   // "ti59", "pc1600" (AC-13)
    virtual std::string_view version() const = 0;       // in the header of a state file (AC-24)
    virtual const machine::Catalogue& catalogue() const = 0;
    virtual machine::MachineBuilder& builder() = 0;
    // The product's settings declaration (LIB-9), for --settings.
    virtual const settings::ProductDecl& settingsDecl() const = 0;
    // The product's lookup over its bundled ROM files, as a frontend finds
    // them (LIB-12; O-2). On failure the run does not start (EC-1).
    virtual roms::LookupResult lookUpRoms() const = 0;
    // LIB-18 (design §3.6): what the start, the model switch and the other
    // machine-control commands build with; the harness's start is LIB-18's
    // StartupPart (LIB-11 AC-2).
    virtual startup::ProductMachines& machines() = 0;
    // LIB-4 (design §7): the printers and rules of the control state the
    // harness session carries; by default the catalogue alone, no printers
    // and no rules (the products name theirs in their column steps).
    virtual control::ControlSetup controlSetup() const { return control::ControlSetup{&catalogue(), {}, {}}; }
    // AC-10: the display as text, by the product's rule or font.
    virtual DisplayText readDisplay(const machine::DisplayOutput&, const machine::MachineDescriptor&) = 0;
    // AC-13; C59-9, C1600-5.
    virtual void addCommands(CommandRegistry&) {}
    // The product's app layer in the session: its parts, after the
    // library's (LIB-3 AC-12). Owned by the product.
    virtual std::vector<sessions::SessionPart*> sessionParts() { return {}; }
    // LIB-6 (design §2): the product's preset rules and bundled presets; the
    // harness puts a PresetsPart into the session when there are rules. None
    // until the product's column (M-46, M-54): `preset` is refused then.
    virtual presets::ProductRules* presetRules() { return nullptr; }
    virtual const presets::PresetBundle* presetBundle() const { return nullptr; }
    // LIB-5's part among sessionParts(), for the preset steps `key` and `type`.
    virtual typing::TypingPart* typingPart() { return nullptr; }
    // LIB-8's part among sessionParts(), when the product has printers: the
    // harness hands it the machine control (the origin of a build, A-3) and
    // adds the step `cut-paper` to the preset vocabulary (A-2).
    virtual paper::PaperPart* paperPart() { return nullptr; }
    // LIB-10's part among sessionParts(): the harness hands it the run's
    // trace folder and limit and adds the debugger's steps to the preset
    // vocabulary. None until the product's column (M-45, M-53).
    virtual debugger::DebuggerPart* debuggerPart() { return nullptr; }
    // LIB-7 (design §3.11): the product's kinds of medium, with their bundled
    // folders, and the listing of its modules that are not media; the harness
    // puts a MediaPart on the run's save folder into the session when there
    // are kinds. None until the product's column (M-47, M-55).
    virtual std::vector<media::KindDecl> mediaKinds() { return {}; }
    virtual media::ModuleListing moduleListing() const { return {}; }
    // LIB-13 (design §5, §7): what `open` takes besides presets and media.
    // The product's preset extensions (".ti59"; none: a preset is told by
    // its model line only), its file kinds in the order they are asked, and
    // the largest file per way in (none: no limit). None until the
    // product's column (M-48, M-56): `open` takes presets and media and
    // refuses the rest.
    virtual std::vector<std::string> presetExtensions() const { return {}; }
    virtual std::vector<opening::FileKind*> fileKinds() { return {}; }
    virtual std::map<opening::Way, std::uint64_t> openLimits() const { return {}; }
    // C1600-3 (design §3.5): the run's library parts, before the session is
    // built; and the product's options of `serve` (`--dap`), owned by it.
    virtual void runParts(const RunParts&) {}
    virtual std::vector<ServeAddOn*> serveAddOns() { return {}; }
    // LIB-11 D-11 (Part D, D.3): called by Server::start once the run's
    // settings and folders exist and before any other member that reads the
    // session's parts (lookUpRoms() included); the product builds its app
    // layer on them here.
    virtual void prepare(RunSetup&) {}
    // LIB-11 D-6, D-15: the library's parts of the product's app layer, or
    // none (the run builds its own). Asked after prepare(); the product's
    // sessionParts() then holds them, in the order its frontends use, and the
    // run puts its control part first, as the frontends' session builders do.
    virtual std::optional<AppLayerParts> appLayerParts() { return std::nullopt; }
};

using ProductFactory = std::unique_ptr<HarnessProduct> (*)();

} // namespace calcu::harness
