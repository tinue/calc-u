#pragma once
#include "harness/DisplayText.hpp"
#include "machine/Build.hpp"
#include "machine/Catalogue.hpp"
#include "machine/Descriptor.hpp"
#include "machine/Display.hpp"
#include "roms/Lookup.hpp"
#include "roms/RomSet.hpp"
#include "sessions/SessionPart.hpp"
#include "settings/Declaration.hpp"
#include "settings/SettingsFile.hpp"
#include "startup/ProductMachines.hpp"
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

// The interface a product implements for the harness (LIB-11 design §3.6).
// The library depends on no product (LIB-1 AC-24): each product registers
// a factory with calcu_harness_add_product() (§3.7), and the harness runs
// the library's app session on the product's machine through it (§4).

namespace calcu::parts {
class LibraryParts;
}

namespace calcu::harness {

class CommandRegistry;
class ServeAddOn;
class RunSetup;

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
    // AC-10: the display as text, by the product's rule or font.
    virtual DisplayText readDisplay(const machine::DisplayOutput&, const machine::MachineDescriptor&) = 0;
    // AC-13; C59-9, C1600-5.
    virtual void addCommands(CommandRegistry&) {}
    // LIB-11 D-11 (Part D, D.3): called by Server::start once the run's
    // settings and folders exist and before any other member that reads the
    // session's parts (lookUpRoms() included); the product builds its app
    // layer on them here.
    virtual void prepare(RunSetup&) {}
    // LIB-11 D-6, D-15: the library's parts of the run's session, as the
    // product's app layer brings them (parts::LibraryParts, on what every
    // run's parts take from the run: RunSetup::partsSetup), built in
    // prepare(). The run puts their control part first, its own state part
    // next, then sessionParts().
    virtual parts::LibraryParts& libraryParts() = 0;
    // The session's parts after the control part: the library's in their
    // order (LibraryParts::sessionParts), then the product's own (LIB-3
    // AC-12). Owned by the product.
    virtual std::vector<sessions::SessionPart*> sessionParts() = 0;
    // C1600-3 (design §3.5): the product's options of `serve` (`--dap`), owned by it.
    virtual std::vector<ServeAddOn*> serveAddOns() { return {}; }
};

using ProductFactory = std::unique_ptr<HarnessProduct> (*)();

} // namespace calcu::harness
