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

// The papers of a product in its app session (LIB-8 design §3.6; spec AC-6,
// AC-8, AC-9, AC-16 to AC-19, AC-22, EC-1 to EC-5, EC-7, EC-8): one paper
// for each type of printer the product declares, from the start of the
// session until it ends, whatever machine runs and whether or not that
// printer is plugged in. A product without printers adds no part, so it
// has no paper topic (EC-2).
//
// Binding (A-1): before and after every host call, on a new machine and
// before every paper command and request, the part lists the machine's
// paper-output capabilities and binds each to the paper of its printer
// type. Binding sends nothing (AC-9). A capability of a printer that is
// not declared, or a second one of a type, is logged and left unbound
// (D-10). A new machine built by a user's preset load (BuildOrigin::Preset)
// empties the papers of the printers the preset names first (A-3; AC-8
// table; LIB-6 AC-41); any other build leaves them.

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
    // catalogue. `control`, for the origin of the last build (A-3), may be
    // set later (the harness builds it after the product's parts).
    PaperPart(const machine::Catalogue&, std::vector<PrinterPaper>, const startup::MachineControl* control = nullptr);
    void setControl(const startup::MachineControl* c) { control_ = c; }

    void attach(sessions::SessionSetup&) override;
    void beforeCall(sessions::SessionContext&, pacing::WallTime) override;
    void afterCall(sessions::SessionContext&, pacing::WallTime) override;
    void machineChanged(sessions::SessionContext&) override;

    // The step `cut-paper` (AC-22): the paper of the printer that is plugged
    // in, without a copy; nothing without a printer.
    void cutPlugged(sessions::SessionContext&);

    // A paper by its owner's name in any case, the one of the printer that
    // is plugged in without a name (EC-8). `refusal` says why there is none.
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

    const startup::MachineControl* control_;
    std::vector<Entry> entries_;
    sessions::StateTopic* papersTopic_ = nullptr;
    sessions::StreamTopic* copiesTopic_ = nullptr;
    std::optional<std::uint64_t> generation_;
    std::set<std::string> logged_;   // D-10: each unbound capability once
};

// A-2: the step `- cut-paper:` (no value, no emulated time, also while
// paused; not a harness command of the same name, PaperCommands.cpp has its
// own). The product, which owns both parts, adds it.
void addPaperStep(presets::Vocabulary&, PaperPart&);

// Conformance (§3.1): the printer's capability is there, its description
// equals the one declared, its standing is 0 after a plug. The problem, or
// none.
std::optional<std::string> checkPaperOutput(machine::Machine&, const machine::PeripheralId&, const PaperDescription&);

} // namespace calcu::paper
