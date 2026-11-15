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
#include <memory>
#include <string_view>
#include <vector>

// The interface a product implements for the harness (LIB-11 design §3.6).
// The library depends on no product (LIB-1 AC-24): each product registers
// a factory with calcu_harness_add_product() (§3.7), and the harness runs
// the library's app session on the product's machine through it (§4).

namespace calcu::harness {

class CommandRegistry;

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
    // A-1: the request of the product's start, until LIB-18's start exists.
    virtual machine::BuildRequest startupRequest(const settings::SettingsFile&, const roms::RomSet&) = 0;
    // A-1: what the product does with a machine it has just built (a
    // Sharp machine's first cold boot); nothing by default.
    virtual void startUp(machine::Machine&) {}
    // AC-10: the display as text, by the product's rule or font.
    virtual DisplayText readDisplay(const machine::DisplayOutput&, const machine::MachineDescriptor&) = 0;
    // AC-13; C59-9, C1600-5.
    virtual void addCommands(CommandRegistry&) {}
    // The product's app layer in the session: its parts, after the
    // library's (LIB-3 AC-12). Owned by the product.
    virtual std::vector<sessions::SessionPart*> sessionParts() { return {}; }
};

using ProductFactory = std::unique_ptr<HarnessProduct> (*)();

} // namespace calcu::harness
