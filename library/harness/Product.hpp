#pragma once
#include "harness/DisplayText.hpp"
#include "machine/Descriptor.hpp"
#include "machine/Display.hpp"
#include "roms/Lookup.hpp"
#include "sessions/SessionPart.hpp"
#include "settings/Declaration.hpp"
#include <memory>
#include <string_view>
#include <vector>

// The interface a product implements for the harness (LIB-11 design §3.6;
// Part E simplification, E-1). The library depends on no product (LIB-1
// AC-24): each product registers a factory with calcu_harness_add_product()
// (§3.7), and the harness runs the library's app session on the product's
// app layer through it (§4, D-11).

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
    virtual std::string_view version() const = 0;       // in the header of a state file (AC-24)
    // The product's settings declaration (LIB-9), for --settings.
    virtual const settings::ProductDecl& settingsDecl() const = 0;
    // AC-10: the display as text, by the product's rule or font.
    virtual DisplayText readDisplay(const machine::DisplayOutput&, const machine::MachineDescriptor&) = 0;
    // AC-13; C59-9, C1600-5.
    virtual void addCommands(CommandRegistry&) {}
    // LIB-11 D-11 (Part D, D.3): called by Server::start once the run's
    // settings and folders exist and before any other member that reads the
    // session's parts; the product builds its app layer on them here, with
    // its lookup over its bundled ROM files as a frontend finds them (LIB-12;
    // O-2), which it returns. Without the full set the run does not start
    // (EC-1).
    virtual roms::LookupResult prepare(RunSetup&) = 0;
    // LIB-11 D-6, D-15: the library's parts of the run's session, as the
    // product's app layer brings them (parts::LibraryParts), built in prepare().
    virtual parts::LibraryParts& libraryParts() = 0;
    // Every part of the session in the one order (LibraryParts::sessionParts):
    // the control part, `host` (the run's own), the library's, the product's
    // (LIB-3 AC-12). Owned by the product but `host`.
    virtual std::vector<sessions::SessionPart*> sessionParts(const std::vector<sessions::SessionPart*>& host) = 0;
    // C1600-3 (design §3.5): the product's options of `serve` (`--dap`), owned by it.
    virtual std::vector<ServeAddOn*> serveAddOns() { return {}; }
};

using ProductFactory = std::unique_ptr<HarnessProduct> (*)();

} // namespace calcu::harness
