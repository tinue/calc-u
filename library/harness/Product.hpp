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

// The interface a product implements for the harness. The library depends
// on no product: each product registers a factory with
// calcu_harness_add_product(), and the harness runs the library's app
// session on the product's app layer through it.

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
    virtual std::string_view name() const = 0;          // "Calc-U-59"
    virtual std::string_view shortName() const = 0;     // the product word, "c59" (glossary)
    virtual std::string_view version() const = 0;       // in the header of a state file
    // The product's settings declaration, for --settings.
    virtual const settings::ProductDecl& settingsDecl() const = 0;
    // The display as text, by the product's rule or font.
    virtual DisplayText readDisplay(const machine::DisplayOutput&, const machine::MachineDescriptor&) = 0;
    // The product's commands, under its product word.
    virtual void addCommands(CommandRegistry&) {}
    // Called by Server::start once the run's
    // settings and folders exist and before any other member that reads the
    // session's parts; the product builds its app layer on them here, with
    // its lookup over its bundled ROM files as a frontend finds them, which
    // it returns. Without the full set the run does not start.
    virtual roms::LookupResult prepare(RunSetup&) = 0;
    // The library's parts of the run's session, as the
    // product's app layer brings them (parts::LibraryParts), built in prepare().
    virtual parts::LibraryParts& libraryParts() = 0;
    // Every part of the session in the one order (LibraryParts::sessionParts):
    // the control part, `host` (the run's own), the library's, the product's
    // Owned by the product but `host`.
    virtual std::vector<sessions::SessionPart*> sessionParts(const std::vector<sessions::SessionPart*>& host) = 0;
    // The product's options of `serve` (`--dap`), owned by it.
    virtual std::vector<ServeAddOn*> serveAddOns() { return {}; }
};

using ProductFactory = std::unique_ptr<HarnessProduct> (*)();

} // namespace calcu::harness
