#pragma once
#include "debugger/Levels.hpp"
#include "debugger/RunControl.hpp"
#include "debugger/RunState.hpp"
#include "debugger/Symbols.hpp"
#include "debugger/TraceSession.hpp"
#include "machine/Ids.hpp"
#include "sessions/SessionPart.hpp"
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

// The debugger's session part: the run control as the machine session's
// gate, the topics `run-state` and `trace`, and the session commands and
// requests of the debugger. One per app session; a product puts it among its
// parts, the harness's test product among its own. Without a machine every
// name but the topics is refused with the session's "No machine is
// running."; a machine without debug targets refuses
// them with kNoTarget.

namespace calcu::presets {
class Vocabulary;
}

namespace calcu::debugger {

struct DebuggerSetup {
    LevelRules* rules = nullptr;   // nullptr: CpuOnlyRules
    TraceSetup trace;
    std::vector<const SymbolReader*> symbolReaders;
    // Whether a model has the steps pause:, resume:,
    // step:, pause-on-start: and debug-panel: (the Sharp models: undefined);
    // unset: every model. `trace:` is defined for every model.
    std::function<bool(const machine::ModelId&)> runControlSteps;
    // The views of the product's debug panel for `debug-panel:`; and what
    // tells the frontend to show one; unset: none shown.
    std::function<std::vector<std::string>(const machine::ModelId&)> panelViews;
    std::function<void(sessions::SessionContext&, const std::string& view)> showPanel;
};

class DebuggerPart final : public sessions::SessionPart {
public:
    explicit DebuggerPart(DebuggerSetup);
    ~DebuggerPart();

    void attach(sessions::SessionSetup&) override;
    void machineChanged(sessions::SessionContext&) override;
    void machineRestarted(sessions::SessionContext&, sessions::Restart) override;
    void operationEnded(sessions::SessionContext&) override;                       // ends a preset's trace

    RunControl& runControl() { return control_; }
    const RunControl& runControl() const { return control_; }
    TraceSession& traceSession() { return trace_; }
    // The setup as given, but for `trace`, which the trace session holds.
    const DebuggerSetup& setup() const { return setup_; }
    // The symbols loaded for a target.
    const SymbolTable* symbols(const machine::DebugTargetId&) const;

private:
    void addCommands(sessions::SessionSetup&);
    void addRequests(sessions::SessionSetup&);
    // An address argument: a number, or a name of the target's symbols.
    std::optional<std::uint64_t> addressOf(const sessions::Value* v, const machine::DebugTargetId&, std::string* error) const;

    DebuggerSetup setup_;
    CpuOnlyRules ownRules_;
    TraceSession trace_;
    RunControl control_;
    sessions::SessionContext* ctx_ = nullptr;
    std::map<machine::DebugTargetId, SymbolTable> symbols_;
};

// The debugger's preset steps on the presets' step place (DebuggerSteps.cpp):
// added before the session is built, as the paper's addPaperStep.
void addDebuggerSteps(presets::Vocabulary&, DebuggerPart&);

} // namespace calcu::debugger
