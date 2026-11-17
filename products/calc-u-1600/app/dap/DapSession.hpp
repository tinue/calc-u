#pragma once
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <vector>

#include "Debug/ProgramLoader.hpp"
#include "Debug/SourceMap.hpp"
#include "app/dap/DapBreakpoints.hpp"
#include "app/dap/DapJson.hpp"
#include "app/dap/DapStepping.hpp"
#include "app/dap/DapTargets.hpp"
#include "sessions/Operation.hpp"
#include "sessions/SessionPart.hpp"

namespace calcu::debugger {
class DebuggerPart;
class RunControl;
} // namespace calcu::debugger
namespace calcu::typing {
class TypingPart;
}
namespace calcu::presets {
class PresetsPart;
class PresetBundle;
} // namespace calcu::presets
namespace calcu::startup {
class MachineControl;
}
namespace calcu::settings {
class SettingsFile;
}

// One DAP session (C1600-3 design §3.4, §3.6): the client's requests of
// AC-11 and AC-17 on the library's debugger (LIB-10) and the DAP's layer of
// app/dap/, the set-up of AC-4 to AC-7 as operations of the app session
// (D-1; DapSetup.cpp), and the events of AC-16. From
// frontends/qt6/app/debug/DapSession.cpp and DebugController.cpp of 0.8.1,
// with calcu::sessions::Value for QJsonObject. No Qt type; it names no
// machine class (TR).
//
// Identifiers handed to the client (as today):
//   thread ids   the debug targets in the machine's order, from 1
//   frame ids    thread * 1000 + index: 0 = now, 1..20 = history
//   variables    frame id * 4 + 1 registers / 2 flags / 3 banks
//   memory refs  a number: the address in its low 16 bits, the thread
//                above them when it is not 1, 0x100000 for ME1

namespace cu1600::dap {

// What the server reaches the machine through (§3.3): the library's parts
// and the product's settings, from SharpAppPart's library parts (the
// harness's run builds a SharpAppPart of its own, LIB-11 D-6).
struct DapDeps {
    calcu::debugger::DebuggerPart* debugger = nullptr;
    calcu::typing::TypingPart* typing = nullptr;
    calcu::presets::PresetsPart* presets = nullptr;
    calcu::startup::MachineControl* control = nullptr;
    calcu::settings::SettingsFile* settings = nullptr;
    const calcu::presets::PresetBundle* bundle = nullptr;
};

class DapSession {
public:
    struct Host {
        std::function<void(const std::string& body)> send;   // one message, framed by the part
        std::function<void()> dropClient;                    // after `disconnect`
        std::function<void()> quitProgram;                   // `calcu1600/quit`
    };
    DapSession(const DapDeps&, calcu::sessions::SessionContext&, Host);
    ~DapSession();
    DapSession(const DapSession&) = delete;
    DapSession& operator=(const DapSession&) = delete;

    // A message of the client (requests only; anything else is passed over).
    void handle(const Value& message);
    // After every host call and every message: output lines, a new machine
    // told once no operation runs, and a pause told as `stopped` (AC-16).
    void poll();
    void machineChanged();
    // An operation ended or was stopped: ours answers its request.
    void operationEnded();
    // AC-10: the session's breakpoints and watches go, a paused machine
    // resumes. Once.
    void end();
    // Settings ▸ Debugger ▸ Disconnect: the client is told first (AC-16).
    void toldToGo();
    void output(const std::string& text, const std::string& category = "console");
    // Its client is gone and another came: nothing more is sent.
    void detach() { host_ = Host{}; }

    bool active() const { return active_; }
    bool operationRuns() const { return opRunning_; }
    // `disconnect` or `calcu1600/quit` was answered: the part drops the client.
    bool closing() const { return closing_; }
    // The machine should run now: a session that has it running (the harness).
    bool wantsTime() const;

private:
    using Body = Value::Object;
    // ── Transport ──
    void respond(const Value& request, bool success, Body body, const std::string& message);
    void event(const std::string& name, Body body = {});
    void sendBreakpointChanges(const std::vector<BreakpointStatus>& changed);

    // ── Requests answered at once ──
    void initialize(Body* body, std::string* error);
    void configurationDone(std::string* error);
    void threads(Body* body, std::string* error);
    void stackTrace(const Value& args, Body* body, std::string* error);
    void scopes(const Value& args, Body* body, std::string* error);
    void variables(const Value& args, Body* body, std::string* error);
    void setVariable(const Value& args, Body* body, std::string* error);
    void setBreakpoints(const Value& args, Body* body, std::string* error);
    void setFunctionBreakpoints(const Value& args, Body* body, std::string* error);
    void setInstructionBreakpoints(const Value& args, Body* body, std::string* error);
    void dataBreakpointInfo(const Value& args, Body* body, std::string* error);
    void setDataBreakpoints(const Value& args, Body* body, std::string* error);
    void continueRequest(Body* body, std::string* error);
    void pause(std::string* error);
    void step(const Value& args, StepKind kind, std::string* error);
    void disassemble(const Value& args, Body* body, std::string* error);
    void readMemory(const Value& args, Body* body, std::string* error);
    void writeMemory(const Value& args, Body* body, std::string* error);
    void evaluate(const Value& args, Body* body, std::string* error);
    void screen(Body* body, std::string* error);

    // ── Requests answered at the end of an operation (DapSetup.cpp, D-1) ──
    // attach/launch, restart, calcu1600/load, calcu1600/reset: false when it
    // is refused before anything starts (`error`).
    bool startOperation(const Value& request, std::string* error);
    calcu::sessions::Task<calcu::sessions::OperationResult> runRequest(calcu::sessions::OperationContext&, Value request);
    // S1 to S8 with `config`; false with `error`.
    calcu::sessions::Task<bool> setUp(calcu::sessions::OperationContext&, Value config, std::string* error);
    // S7: Build & Load of `program`; the reply's body into `body` (may be null).
    calcu::sessions::Task<bool> buildAndLoad(calcu::sessions::OperationContext&, Value program, bool inSetup, Body* body,
                                             std::string* error);
    // S7's clean start: from the configuration's preset, else the model's
    // startup preset (EC-4), else a Cold boot to key readiness.
    calcu::sessions::Task<bool> cleanStart(calcu::sessions::OperationContext&, std::string* how, std::string* error);
    // S4: Reset or Cold boot without the run to key readiness, standing at
    // the reset vector (D-2).
    bool resetBare(bool coldBoot, std::string* error);
    // S8: pause cpu, landed between two instructions.
    calcu::sessions::Task<void> pauseNow(calcu::sessions::OperationContext&);
    void bindStatic(const Value& item, bool isListing);

    // ── Helpers ──
    calcu::debugger::RunControl& runControl() const;
    DapTargets targets() const;
    bool ready(std::string* error) const;
    void applyBreakpoints();
    std::vector<BreakpointStatus> rebindListings();
    void typeCommand(const std::string& line);
    void resume();
    void pauseMachine(bool entry);
    void noteRunning() {
        lastPaused_ = false;
        entryPending_ = false;
    }
    int threadOf(const Value& args) const;
    int threadForCpu(const std::string& cpu) const;
    Value sourceObject(const std::string& path) const;
    Value statusObject(const BreakpointStatus& st) const;
    Value::Array registerVariables(int frameId) const;
    Value::Array flagVariables(int frameId) const;
    Value::Array bankVariables(int frameId) const;
    std::vector<debug::Register> frameRegisters(int frameId) const;
    debug::ExpressionContext frameContext(int frameId) const;
    std::string instructionText(int thread, std::uint16_t pc) const;
    bool memoryArgs(const Value& args, int* thread, std::uint16_t* addr, bool* me1, std::string* error) const;

    DapDeps deps_;
    calcu::sessions::SessionContext& ctx_;
    Host host_;
    std::int64_t seq_ = 1;
    bool active_ = false;
    bool closing_ = false;
    bool ended_ = false;

    debug::SourceMap map_;
    DapBreakpoints breakpoints_;
    DapBreakpoints::SymbolLookup symbols_;
    bool stopOnEntry_ = false;
    Value launchArgs_;     // the client's attach arguments, for restart
    Value attachConfig_;   // effectiveConfig() of them

    // Events (AC-16).
    bool lastPaused_ = false;
    bool entryPending_ = false;
    bool replacedPending_ = false;
    std::uint64_t seenGeneration_ = 0;   // the machine the session knows (the set-up's own builds are known)

    // The operation of a request (D-1).
    bool opRunning_ = false;
    std::optional<Value> pendingRequest_;
    bool tellStopAtEnd_ = false;   // the end leaves a pause to be told (entry)
    bool resumeAtEnd_ = false;     // the end resumes the machine
};

// Memory references are plain numbers: VS Code's disassembly view parses
// them (and every instruction address) as one.
std::string memoryReference(int thread, std::uint16_t addr, bool me1 = false);
bool parseMemoryReference(const std::string& ref, int* thread, std::uint16_t* addr, bool* me1);

} // namespace cu1600::dap
