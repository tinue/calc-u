#pragma once
#include "machine/Catalogue.hpp"
#include "machine/Machine.hpp"
#include "paper/Paper.hpp"
#include "paper/PaperTopics.hpp"
#include "sessions/SessionPart.hpp"
#include "sessions/Topics.hpp"
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <vector>

// The papers of a product in its app session: one paper
// for each type of printer the product declares, from the start of the
// session until it ends, whatever machine runs and whether or not that
// printer is plugged in. A product without printers adds no part, so it
// has no paper topic.
//
// Binding: before and after every host call, on a new machine and
// before every paper command and request, the part lists the machine's
// paper-output capabilities and binds each to the paper of its printer
// type. Binding sends nothing. A capability of a printer that is
// not declared, or a second one of a type, is logged once and left
// unbound. A new machine built by a user's preset load (BuildOrigin::Preset)
// empties the papers of the printers the preset names first; any other
// build, such as the startup preset at a model switch, leaves them.

namespace calcu::startup {
class MachineControl;
}
namespace calcu::presets {
class Vocabulary;
}

namespace calcu::paper {

struct PrinterPaper {
    machine::PeripheralId printer;
    PaperDescription description;
};

class PaperPart final : public sessions::SessionPart {
public:
    // The owner's name of each printer is its descriptor's name in the
    // catalogue. `control`: the origin of the last build; it outlives
    // the part.
    PaperPart(const machine::Catalogue&, std::vector<PrinterPaper>, const startup::MachineControl& control);

    void attach(sessions::SessionSetup&) override;
    void beforeCall(sessions::SessionContext&, pacing::WallTime) override;
    void afterCall(sessions::SessionContext&, pacing::WallTime) override;
    void machineChanged(sessions::SessionContext&) override;

    // The steps `cut-paper` and `copy-paper`: the paper of the printer
    // that is plugged in, as the GUI's Cut and Copy, where the host takes
    // the clipboard from a step (hostCopiesInSteps); elsewhere, such as the
    // Web, the cut only cuts and the copy does nothing. Nothing without a
    // printer; neither ever fails.
    void cutPlugged(sessions::SessionContext&);
    void copyPlugged(sessions::SessionContext&);

    // A paper by its owner's name in any case, the one of the printer that
    // is plugged in without a name. `refusal` says why there is none.
    struct Found {
        Paper* paper = nullptr;
        std::string owner;
        std::string refusal;
    };
    Found find(sessions::SessionContext&, const std::optional<std::string>& owner);
    // The papers in the order declared, with their owner's names (tests, the harness).
    std::vector<std::pair<std::string, const Paper*>> papers() const;
    // Binds and sends what changed (the start of every command and request).
    void sync(sessions::SessionContext&);

private:
    struct Entry {
        machine::PeripheralId printer;
        std::string owner;
        std::unique_ptr<Paper> paper;
        sessions::StreamTopic* topic = nullptr;
    };
    void bindAll(sessions::SessionContext&);
    void publish();
    Entry* entryOf(const machine::PeripheralId&);
    Entry* plugged(sessions::SessionContext&);
    Entry* entryOf(const Paper*);
    sessions::CommandOutcome copy(sessions::SessionContext&, Entry&, bool cutAfter);

    const startup::MachineControl& control_;
    std::vector<Entry> entries_;
    sessions::StateTopic* papersTopic_ = nullptr;
    sessions::StreamTopic* copiesTopic_ = nullptr;
    std::optional<std::uint64_t> generation_;
    std::set<std::string> logged_;   // each unbound capability is logged once
};

// The steps `- cut-paper:` and `- copy-paper:` (no value, no emulated time,
// also while paused; the harness has its own commands, PaperCommands.cpp).
// The product, which owns both parts, adds them.
void addPaperSteps(presets::Vocabulary&, PaperPart&);

// Conformance: the printer's capability is there, its description
// equals the one declared, its standing is 0 after a plug. The problem, or
// none.
std::optional<std::string> checkPaperOutput(machine::Machine&, const machine::PeripheralId&, const PaperDescription&);

} // namespace calcu::paper
