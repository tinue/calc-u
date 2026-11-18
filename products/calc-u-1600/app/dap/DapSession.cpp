#include "app/dap/DapSession.hpp"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <utility>

#include "Debug/CpuRegisters.hpp"
#include "Debug/Listing/Listing.hpp"
#include "app/SharpDisplayLines.hpp"
#include "harness/DisplayText.hpp"
#include "debugger/DebuggerPart.hpp"
#include "debugger/RunControl.hpp"
#include "debugger/RunState.hpp"
#include "machine/Machine.hpp"
#include "machine/Names.hpp"
#include "sessions/MachineSession.hpp"
#include "typing/TypingPart.hpp"

namespace cu1600::dap {

namespace {

constexpr int kHistoryFrames = 20;
constexpr int kVarRegisters = 1, kVarFlags = 2, kVarBanks = 3;

int frameThread(int frameId) { return frameId / 1000; }
int frameIndex(int frameId) { return frameId % 1000; }

std::string hex(std::uint32_t v, int digits) {
    char buf[16];
    std::snprintf(buf, sizeof buf, "0x%0*X", digits, v);
    return buf;
}
std::string hexBare(std::uint32_t v, int digits) {
    char buf[16];
    std::snprintf(buf, sizeof buf, "%0*X", digits, v);
    return buf;
}

std::string upper(std::string s) {
    for (char& c : s) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    return s;
}

std::string fileName(const std::string& path) {
    const std::size_t at = path.find_last_of("/\\");
    return at == std::string::npos ? path : path.substr(at + 1);
}

// The condition / hit condition / log message every breakpoint kind shares.
void readSpec(const Value& o, BreakpointSpec* spec) {
    spec->condition = textOf(o, "condition");
    spec->hitCondition = textOf(o, "hitCondition");
    spec->logMessage = textOf(o, "logMessage");
}

std::string registerValue(const debug::Register& r) {
    if (r.bits == 1) return std::to_string(r.value);
    return hex(r.value, r.bits == 8 ? 2 : 4);
}

std::string stopReason(calcu::debugger::RunState::Reason r, bool entry, bool entryHit) {
    using R = calcu::debugger::RunState::Reason;
    switch (r) {
    case R::Breakpoint: return entryHit ? "entry" : "breakpoint";
    case R::Watch: return "data breakpoint";
    case R::Step: return "step";
    case R::Entry: return "entry";
    case R::Pause:
    case R::PauseOnStart: return entry ? "entry" : "pause";   // D-7
    }
    return "pause";
}

const Value::Array kNoElements;
const Value::Array& elementsOf(const Value& v, std::string_view key) {
    const Value* m = v.find(key);
    return m && m->elements() ? *m->elements() : kNoElements;
}

} // namespace

std::string memoryReference(int thread, std::uint16_t addr, bool me1) {
    return hex(std::uint32_t(addr) | (thread > 1 ? std::uint32_t(thread) << 16 : 0u) | (me1 ? 0x100000u : 0u), 4);
}

bool parseMemoryReference(const std::string& ref, int* thread, std::uint16_t* addr, bool* me1) {
    std::string a = ref;
    while (!a.empty() && std::isspace(static_cast<unsigned char>(a.front()))) a.erase(a.begin());
    while (!a.empty() && std::isspace(static_cast<unsigned char>(a.back()))) a.pop_back();
    if (a.size() < 3 || a[0] != '0' || (a[1] != 'x' && a[1] != 'X')) return false;
    char* end = nullptr;
    const unsigned long long v = std::strtoull(a.c_str() + 2, &end, 16);
    if (!end || *end != '\0' || v > 0x1FFFFF) return false;
    *addr = std::uint16_t(v & 0xFFFF);
    *thread = int((v >> 16) & 0xF);
    if (*thread == 0) *thread = 1;
    *me1 = (v & 0x100000) != 0;
    return true;
}

DapSession::DapSession(const DapDeps& deps, calcu::sessions::SessionContext& ctx, Host host)
    : deps_(deps), ctx_(ctx), host_(std::move(host)) {
    symbols_ = [this](const std::string& name, std::int64_t* value) {
        std::uint16_t v = 0;
        if (!map_.symbolValue(name, &v)) return false;
        *value = v;
        return true;
    };
}

DapSession::~DapSession() = default;

// ── Transport ─────────────────────────────────────────────────────────────

void DapSession::respond(const Value& request, bool success, Body body, const std::string& message) {
    Value::Object r;
    r.push_back({"seq", Value(seq_++)});
    r.push_back({"type", Value("response")});
    const Value* rs = request.find("seq");
    r.push_back({"request_seq", rs ? *rs : Value()});
    const Value* cmd = request.find("command");
    r.push_back({"command", cmd ? *cmd : Value()});
    r.push_back({"success", Value(success)});
    if (!body.empty()) r.push_back({"body", Value(std::move(body))});
    if (!success) r.push_back({"message", Value(message)});
    if (host_.send) host_.send(writeJson(Value(std::move(r))));
}

void DapSession::event(const std::string& name, Body body) {
    Value::Object e;
    e.push_back({"seq", Value(seq_++)});
    e.push_back({"type", Value("event")});
    e.push_back({"event", Value(name)});
    if (!body.empty()) e.push_back({"body", Value(std::move(body))});
    if (host_.send) host_.send(writeJson(Value(std::move(e))));
}

void DapSession::output(const std::string& text, const std::string& category) {
    std::string line = text;
    if (line.empty() || line.back() != '\n') line += '\n';
    event("output", Body{{"category", Value(category)}, {"output", Value(std::move(line))}});
}

void DapSession::sendBreakpointChanges(const std::vector<BreakpointStatus>& changed) {
    for (const auto& st : changed) event("breakpoint", Body{{"reason", Value("changed")}, {"breakpoint", statusObject(st)}});
}

// ── Dispatch ──────────────────────────────────────────────────────────────

void DapSession::handle(const Value& request) {
    if (textOf(request, "type") != "request") return;
    const std::string command = textOf(request, "command");
    const Value* argsPtr = request.find("arguments");
    const Value args = argsPtr && argsPtr->members() ? *argsPtr : Value(Value::Object{});
    Body body;
    std::string error;
    if (command == "attach" || command == "launch" || command == "restart" || command == "c1600/load" ||
        command == "c1600/reset") {
        if (startOperation(request, &error)) return;   // answered at its end
        respond(request, false, {}, error);
        return;
    }
    if (command == "initialize") initialize(&body, &error);
    else if (command == "configurationDone") configurationDone(&error);
    else if (command == "disconnect") end();
    else if (command == "threads") threads(&body, &error);
    else if (command == "stackTrace") stackTrace(args, &body, &error);
    else if (command == "scopes") scopes(args, &body, &error);
    else if (command == "variables") variables(args, &body, &error);
    else if (command == "setVariable") setVariable(args, &body, &error);
    else if (command == "setBreakpoints") setBreakpoints(args, &body, &error);
    else if (command == "setFunctionBreakpoints") setFunctionBreakpoints(args, &body, &error);
    else if (command == "setInstructionBreakpoints") setInstructionBreakpoints(args, &body, &error);
    else if (command == "dataBreakpointInfo") dataBreakpointInfo(args, &body, &error);
    else if (command == "setDataBreakpoints") setDataBreakpoints(args, &body, &error);
    else if (command == "setExceptionBreakpoints") body.push_back({"breakpoints", Value(Value::Array{})});
    else if (command == "continue") continueRequest(&body, &error);
    else if (command == "pause") pause(&error);
    else if (command == "next") step(args, StepKind::Over, &error);
    else if (command == "stepIn") step(args, StepKind::In, &error);
    else if (command == "stepOut") step(args, StepKind::Out, &error);
    else if (command == "disassemble") disassemble(args, &body, &error);
    else if (command == "readMemory") readMemory(args, &body, &error);
    else if (command == "writeMemory") writeMemory(args, &body, &error);
    else if (command == "evaluate") evaluate(args, &body, &error);
    else if (command == "c1600/display-text") displayText(&body, &error);
    else if (command == "c1600/quit") end();
    else error = "Unsupported request '" + command + "'";
    respond(request, error.empty(), std::move(body), error);

    if (command == "initialize" && error.empty()) event("initialized");
    if (command == "disconnect") {
        closing_ = true;
        if (host_.dropClient) host_.dropClient();
    }
    // Scripted runs (tools/dap_smoke.py) end the program this way: a normal
    // quit, so macOS does not take it for a crash.
    if (command == "c1600/quit") {
        closing_ = true;
        if (host_.quitProgram) host_.quitProgram();
    }
}

calcu::debugger::RunControl& DapSession::runControl() const { return deps_.debugger->runControl(); }

DapTargets DapSession::targets() const {
    calcu::machine::Machine* m = ctx_.machine();
    return DapTargets(m ? m->debugging() : nullptr);
}

bool DapSession::ready(std::string* error) const {
    if (active_ && targets().valid()) return true;
    *error = "No machine is running";
    return false;
}

int DapSession::threadOf(const Value& args) const {
    const int t = static_cast<int>(intOf(args, "threadId", 1));
    return t >= 1 ? t : 1;
}

int DapSession::threadForCpu(const std::string& cpu) const {
    const std::string c = calcu::machine::lowerAscii(cpu);
    if (c == "lh5803") return 2;
    const DapTargets t = targets();
    for (const auto& th : t.threads())
        if ((c == "z80" && th.kind == debug::CpuKind::Z80) || (c == "lh5801" && th.kind == debug::CpuKind::LH5801)) return th.id;
    return 1;
}

void DapSession::applyBreakpoints() {
    breakpoints_.apply(runControl().table(), targets(), symbols_);
}

std::vector<BreakpointStatus> DapSession::rebindListings() {
    const DapTargets t = targets();
    if (!t.valid()) return {};
    for (const auto& b : map_.bindings()) map_.verify(b.id, t.bankMatch(), t.codePeek());
    auto changed = breakpoints_.reresolve(map_);
    applyBreakpoints();
    return changed;
}

void DapSession::typeCommand(const std::string& line) {
    deps_.typing->engine().typeText(line, /*enter=*/true, /*refuseUnknown=*/false, calcu::typing::Origin::Typed);
}

void DapSession::resume() {
    noteRunning();
    runControl().resume();
}

void DapSession::pauseMachine(bool entry) {
    noteRunning();
    entryPending_ = entry;
    runControl().pause(calcu::debugger::kCpu);
}

bool DapSession::wantsTime() const {
    if (!active_ || ended_) return false;
    return opRunning_ || !runControl().paused();
}

// ── Events (AC-9, AC-16) ──────────────────────────────────────────────────

void DapSession::machineChanged() {
    if (!active_ || ended_) return;
    const std::uint64_t generation = ctx_.machineSession().generation();
    if (generation == seenGeneration_) return;   // told late (virtual time) of a build the set-up made
    seenGeneration_ = generation;
    if (opRunning_) return;   // the set-up binds the listings itself
    replacedPending_ = true;
}

void DapSession::poll() {
    if (!active_ || ended_) return;
    if (replacedPending_ && !ctx_.busy() && ctx_.machine()) {
        replacedPending_ = false;
        sendBreakpointChanges(rebindListings());
        event("invalidated", Body{{"areas", Value(Value::Array{Value("all")})}});
        output("The machine was rebuilt; breakpoints re-applied.");
    }
    for (std::string& line : breakpoints_.takeOutput()) output(line);
    if (opRunning_) return;   // the pauses of the set-up are not told
    const calcu::debugger::RunState s = runControl().state();
    if (s.paused && !lastPaused_ && s.reason) {
        Body body;
        body.push_back({"reason", Value(stopReason(*s.reason, entryPending_, breakpoints_.entryHit(s.hits)))});
        int thread = 1;
        const DapTargets t = targets();
        if (t.valid() && s.target)
            for (const auto& th : t.threads())
                if (t.targetId(th.id) == *s.target) thread = th.id;
        body.push_back({"threadId", Value(thread)});
        body.push_back({"allThreadsStopped", Value(true)});
        const std::vector<int> ids = breakpoints_.dapIds(s.hits);
        if (!ids.empty()) {
            Value::Array a;
            for (int id : ids) a.push_back(Value(id));
            body.push_back({"hitBreakpointIds", Value(std::move(a))});
        }
        entryPending_ = false;
        event("stopped", std::move(body));
    }
    lastPaused_ = s.paused;
}

void DapSession::end() {
    if (ended_) return;
    ended_ = true;
    if (active_ && ctx_.machine()) {
        breakpoints_.clear();
        applyBreakpoints();
        if (runControl().paused()) runControl().resume();
    }
    active_ = false;
    map_.clear();
}

void DapSession::toldToGo() {
    output("Disconnected from the app (Settings > Debugger).", "important");
    event("terminated");
}

// ── Session ───────────────────────────────────────────────────────────────

void DapSession::initialize(Body* body, std::string* error) {
    if (!ctx_.machine()) {
        *error = "No machine is running";
        return;
    }
    // AC-4: a session starts on a paused machine.
    map_.clear();
    breakpoints_.clear();
    active_ = true;
    seenGeneration_ = ctx_.machineSession().generation();
    applyBreakpoints();
    runControl().pause(calcu::debugger::kCpu);
    lastPaused_ = true;   // the pause of initialize is not told
    *body = Body{
        {"supportsConfigurationDoneRequest", Value(true)},
        {"supportsConditionalBreakpoints", Value(true)},
        {"supportsHitConditionalBreakpoints", Value(true)},
        {"supportsLogPoints", Value(true)},
        {"supportsFunctionBreakpoints", Value(true)},
        {"supportsInstructionBreakpoints", Value(true)},
        {"supportsDataBreakpoints", Value(true)},
        {"supportsDisassembleRequest", Value(true)},
        {"supportsSteppingGranularity", Value(true)},
        {"supportsReadMemoryRequest", Value(true)},
        {"supportsWriteMemoryRequest", Value(true)},
        {"supportsSetVariable", Value(true)},
        {"supportsEvaluateForHovers", Value(true)},
        {"supportsValueFormattingOptions", Value(false)},
        {"supportsRestartRequest", Value(true)},
    };
}

void DapSession::configurationDone(std::string* error) {
    if (!ready(error)) return;
    if (stopOnEntry_) pauseMachine(/*entry=*/true);
    else resume();
}

// ── Threads, frames, variables ────────────────────────────────────────────

void DapSession::threads(Body* body, std::string* error) {
    if (!ready(error)) return;
    const DapTargets t = targets();
    Value::Array list;
    const auto all = t.threads();
    for (const Thread& th : all) {
        std::string name = th.name;
        if (all.size() > 1 && th.id == t.busOwner()) name += " [bus]";
        list.push_back(Value::object({{"id", Value(th.id)}, {"name", Value(name)}}));
    }
    body->push_back({"threads", Value(std::move(list))});
}

std::string DapSession::instructionText(int thread, std::uint16_t pc) const {
    const debug::SourceMap& map = map_;
    return targets().decode(thread, pc, [&map, thread](std::uint16_t a) { return map.symbolAt(thread, a); }).text;
}

Value DapSession::sourceObject(const std::string& path) const {
    return Value::object({{"name", Value(fileName(path))}, {"path", Value(path)}});
}

bool DapSession::memoryArgs(const Value& args, int* thread, std::uint16_t* addr, bool* me1, std::string* error) const {
    if (!parseMemoryReference(textOf(args, "memoryReference"), thread, addr, me1)) {
        *error = "Bad memory reference";
        return false;
    }
    *addr = std::uint16_t(*addr + intOf(args, "offset", 0));
    return true;
}

void DapSession::stackTrace(const Value& args, Body* body, std::string* error) {
    if (!ready(error)) return;
    const DapTargets t = targets();
    const int thread = threadOf(args);
    const int start = static_cast<int>(intOf(args, "startFrame", 0));
    const int levels = static_cast<int>(intOf(args, "levels", 0));
    const int history = int(std::min<std::uint32_t>(t.historySize(thread), kHistoryFrames));
    const int total = 1 + history;
    Value::Array frames;
    const debug::SourceMap& map = map_;
    const disasm::SymbolFn symbols = [&map, thread](std::uint16_t a) { return map.symbolAt(thread, a); };
    for (int i = start; i < total && (levels <= 0 || int(frames.size()) < levels); i++) {
        Value::Object f{{"id", Value(thread * 1000 + i)}, {"column", Value(0)}, {"line", Value(0)}};
        std::uint16_t pc = 0;
        if (i == 0) {
            pc = t.pc(thread);
            put(f, "name", Value(hexBare(pc, 4) + "  " + instructionText(thread, pc)));
            debug::SourceLocation loc;
            if (map_.lookup(thread, pc, t.bankMatch(), &loc, t.codePeek())) {
                put(f, "source", sourceObject(loc.file));
                put(f, "line", Value(loc.line));
            }
        } else {
            const debug::HistoryEntry h = t.history(thread, std::uint32_t(i - 1));
            pc = h.pc;
            std::string text;
            if (h.interrupt) {
                text = "interrupt at " + hexBare(pc, 4);
            } else {
                text = hexBare(pc, 4) + "  " + t.decode(thread, h, symbols).text;
                debug::SourceLocation loc;
                if (map_.lookup(thread, pc, t.bankMatch(), &loc)) {
                    put(f, "source", sourceObject(loc.file));
                    put(f, "line", Value(loc.line));
                }
            }
            put(f, "name", Value(text));
            put(f, "presentationHint", Value("subtle"));
        }
        put(f, "instructionPointerReference", Value(memoryReference(thread, pc)));
        frames.push_back(Value(std::move(f)));
    }
    body->push_back({"stackFrames", Value(std::move(frames))});
    body->push_back({"totalFrames", Value(total)});
}

void DapSession::scopes(const Value& args, Body* body, std::string* error) {
    if (!ready(error)) return;
    const int frameId = static_cast<int>(intOf(args, "frameId"));
    Value::Array list;
    list.push_back(Value::object({{"name", Value("Registers")},
                                  {"presentationHint", Value("registers")},
                                  {"variablesReference", Value(frameId * 4 + kVarRegisters)},
                                  {"expensive", Value(false)}}));
    body->push_back({"scopes", Value(std::move(list))});
}

std::vector<debug::Register> DapSession::frameRegisters(int frameId) const {
    const DapTargets t = targets();
    const int thread = frameThread(frameId), index = frameIndex(frameId);
    if (index == 0) return t.registers(thread);
    if (std::uint32_t(index - 1) >= t.historySize(thread)) return {};
    return t.history(thread, std::uint32_t(index - 1)).registers;
}

Value::Array DapSession::registerVariables(int frameId) const {
    Value::Array list;
    for (const debug::Register& r : frameRegisters(frameId)) {
        Value::Object v{{"name", Value(upper(r.name))}, {"value", Value(registerValue(r))}, {"variablesReference", Value(0)}};
        if (r.bits == 16) put(v, "memoryReference", Value(memoryReference(frameThread(frameId), std::uint16_t(r.value))));
        list.push_back(Value(std::move(v)));
    }
    const Value::Array flags = flagVariables(frameId);
    if (!flags.empty()) {
        std::string summary;
        for (const Value& f : flags) {
            if (!summary.empty()) summary += ' ';
            summary += textOf(f, "name") + textOf(f, "value");
        }
        list.push_back(Value::object({{"name", Value("Flags")},
                                      {"value", Value(summary)},
                                      {"variablesReference", Value(frameId * 4 + kVarFlags)}}));
    }
    // The live frame's bank state, expandable like Flags.
    if (frameIndex(frameId) == 0) {
        std::string parts;
        for (const Value& b : bankVariables(frameId)) {
            if (!parts.empty()) parts += ", ";
            parts += textOf(b, "name").substr(0, 6) + " " + textOf(b, "value");
        }
        list.push_back(Value::object({{"name", Value("Banks")},
                                      {"value", Value(parts)},
                                      {"variablesReference", Value(frameId * 4 + kVarBanks)}}));
    }
    return list;
}

Value::Array DapSession::flagVariables(int frameId) const {
    Value::Array list;
    const debug::CpuKind kind = targets().kindOf(frameThread(frameId));
    std::uint8_t status = 0;
    if (!debug::statusFlags(frameRegisters(frameId), &status)) return list;
    const auto& names = debug::flagNames(kind);
    for (std::size_t i = names.size(); i-- > 0;)
        if (!names[i].empty())
            list.push_back(Value::object({{"name", Value(upper(names[i]))},
                                          {"value", Value(std::to_string((status >> i) & 1))},
                                          {"variablesReference", Value(0)}}));
    return list;
}

Value::Array DapSession::bankVariables(int frameId) const {
    Value::Array list;
    for (const debug::BankField& b : targets().bankState(frameThread(frameId)))
        list.push_back(Value::object({{"name", Value(b.name)}, {"value", Value(b.value)}, {"variablesReference", Value(0)}}));
    return list;
}

void DapSession::variables(const Value& args, Body* body, std::string* error) {
    if (!ready(error)) return;
    const int ref = static_cast<int>(intOf(args, "variablesReference"));
    const int frameId = ref / 4;
    Value::Array list;
    switch (ref % 4) {
    case kVarRegisters: list = registerVariables(frameId); break;
    case kVarFlags: list = flagVariables(frameId); break;
    case kVarBanks: list = bankVariables(frameId); break;
    default: break;
    }
    body->push_back({"variables", Value(std::move(list))});
}

debug::ExpressionContext DapSession::frameContext(int frameId) const {
    const int thread = std::max(1, frameThread(frameId));
    debug::ExpressionContext ctx = targets().expressionContext(thread, symbols_);
    if (frameIndex(frameId) > 0) {
        // A history frame: its own registers first.
        const std::vector<debug::Register> regs = frameRegisters(frameId);
        const auto fallback = ctx.lookup;
        ctx.lookup = [regs, fallback](const std::string& name, std::int64_t* value) {
            for (const auto& r : regs)
                if (r.name == name) {
                    *value = r.value;
                    return true;
                }
            return fallback(name, value);
        };
    }
    return ctx;
}

void DapSession::setVariable(const Value& args, Body* body, std::string* error) {
    if (!ready(error)) return;
    const int ref = static_cast<int>(intOf(args, "variablesReference"));
    const int frameId = ref / 4;
    if (frameIndex(frameId) != 0) {   // EC-6
        *error = "Only the live frame's registers can be changed";
        return;
    }
    const int thread = frameThread(frameId);
    const debug::ExpressionResult r = debug::evaluate(textOf(args, "value"), frameContext(frameId));
    if (!r.ok) {
        *error = r.error;
        return;
    }
    std::string name = calcu::machine::lowerAscii(textOf(args, "name"));
    if (ref % 4 == kVarFlags) name += "f";
    const DapTargets t = targets();
    if (ref % 4 == kVarBanks || !t.writeRegister(thread, name, std::uint32_t(r.value))) {
        *error = "Can't set " + textOf(args, "name");
        return;
    }
    // The value as the register holds it now (a flag as 0/1).
    std::uint32_t now = 0;
    t.readRegister(thread, name, &now);
    debug::Register reg{name, now, std::uint8_t(ref % 4 == kVarFlags ? 1 : 8)};
    for (const auto& x : t.registers(thread))
        if (x.name == name) reg.bits = x.bits;
    body->push_back({"value", Value(registerValue(reg))});
}

// ── Breakpoints ───────────────────────────────────────────────────────────

Value DapSession::statusObject(const BreakpointStatus& st) const {
    Value::Object o{{"id", Value(st.id)}, {"verified", Value(st.verified)}};
    if (st.line > 0) put(o, "line", Value(st.line));
    if (!st.message.empty()) put(o, "message", Value(st.message));
    if (st.hasAddress) put(o, "instructionReference", Value(memoryReference(st.thread, st.addr)));
    return Value(std::move(o));
}

void DapSession::setBreakpoints(const Value& args, Body* body, std::string* error) {
    if (!ready(error)) return;
    const Value* source = args.find("source");
    const std::string path = source ? textOf(*source, "path") : std::string();
    std::vector<DapBreakpoints::SourceRequest> requests;
    for (const Value& o : elementsOf(args, "breakpoints")) {
        DapBreakpoints::SourceRequest r;
        r.line = static_cast<int>(intOf(o, "line"));
        readSpec(o, &r);
        requests.push_back(r);
    }
    const auto statuses = breakpoints_.setSource(debug::absolutePath(path), requests, map_);
    applyBreakpoints();
    Value::Array list;
    for (const auto& st : statuses) {
        Value::Object o = membersOf(statusObject(st));
        put(o, "source", sourceObject(path));
        list.push_back(Value(std::move(o)));
    }
    body->push_back({"breakpoints", Value(std::move(list))});
}

void DapSession::setFunctionBreakpoints(const Value& args, Body* body, std::string* error) {
    if (!ready(error)) return;
    std::vector<DapBreakpoints::FunctionRequest> requests;
    for (const Value& o : elementsOf(args, "breakpoints")) {
        DapBreakpoints::FunctionRequest r;
        r.name = textOf(o, "name");
        readSpec(o, &r);
        requests.push_back(r);
    }
    const auto statuses = breakpoints_.setFunctions(requests, map_);
    applyBreakpoints();
    Value::Array list;
    for (const auto& st : statuses) list.push_back(statusObject(st));
    body->push_back({"breakpoints", Value(std::move(list))});
}

void DapSession::setInstructionBreakpoints(const Value& args, Body* body, std::string* error) {
    if (!ready(error)) return;
    std::vector<DapBreakpoints::InstructionRequest> requests;
    std::vector<bool> valid;
    for (const Value& o : elementsOf(args, "breakpoints")) {
        DapBreakpoints::InstructionRequest r;
        int thread = 1;
        std::uint16_t addr = 0;
        bool me1 = false;
        const bool ok = parseMemoryReference(textOf(o, "instructionReference"), &thread, &addr, &me1);
        valid.push_back(ok);
        if (!ok) continue;
        r.thread = thread;
        r.addr = std::uint16_t(addr + intOf(o, "offset", 0));
        readSpec(o, &r);
        requests.push_back(r);
    }
    const auto statuses = breakpoints_.setInstructions(requests);
    applyBreakpoints();
    Value::Array list;
    std::size_t next = 0;
    for (bool ok : valid) {
        if (ok) list.push_back(statusObject(statuses[next++]));
        else list.push_back(Value::object({{"verified", Value(false)}, {"message", Value("Unknown instruction reference")}}));
    }
    body->push_back({"breakpoints", Value(std::move(list))});
}

void DapSession::dataBreakpointInfo(const Value& args, Body* body, std::string* error) {
    if (!ready(error)) return;
    const std::string name = textOf(args, "name");
    if (intOf(args, "variablesReference", 0) != 0) {
        body->push_back({"dataId", Value()});
        body->push_back({"description", Value("Registers can't be watched; use a conditional breakpoint")});
        return;
    }
    // An address, symbol or expression.
    int thread = 1;
    std::uint16_t addr = 0;
    bool me1 = false;
    if (!parseMemoryReference(name, &thread, &addr, &me1)) {
        const int frameId = static_cast<int>(intOf(args, "frameId", 1000));
        const debug::ExpressionResult r = debug::evaluate(name, frameContext(frameId));
        if (!r.ok || r.value < 0 || r.value > 0xFFFF) {
            body->push_back({"dataId", Value()});
            body->push_back({"description", Value("Not a memory address")});
            return;
        }
        thread = std::max(1, frameThread(frameId));
        addr = std::uint16_t(r.value);
    }
    const int length = std::max<int>(1, static_cast<int>(intOf(args, "bytes", 1)));
    body->push_back({"dataId", Value(std::to_string(thread) + ":" + (me1 ? "1" : "0") + ":" + std::to_string(addr) + ":" +
                                     std::to_string(length))});
    body->push_back({"description", Value(std::string(me1 ? "#" : "") + hex(addr, 4) + " (" + std::to_string(length) +
                                          " byte" + (length > 1 ? "s" : "") + ")")});
    body->push_back({"accessTypes", Value(Value::Array{Value("read"), Value("write"), Value("readWrite")})});
    body->push_back({"canPersist", Value(true)});
}

void DapSession::setDataBreakpoints(const Value& args, Body* body, std::string* error) {
    if (!ready(error)) return;
    std::vector<DataBreakpointSpec> requests;
    for (const Value& o : elementsOf(args, "breakpoints")) {
        const std::string id = textOf(o, "dataId");
        std::vector<std::string> parts;
        std::size_t from = 0;
        for (std::size_t at; (at = id.find(':', from)) != std::string::npos; from = at + 1) parts.push_back(id.substr(from, at - from));
        parts.push_back(id.substr(from));
        if (parts.size() != 4) continue;
        DataBreakpointSpec d;
        d.thread = std::atoi(parts[0].c_str());
        d.space = std::atoi(parts[1].c_str()) ? debug::kSpaceME1 : debug::kSpaceMain;
        d.addr = std::uint16_t(std::strtoul(parts[2].c_str(), nullptr, 10));
        d.length = std::uint16_t(std::max(1, std::atoi(parts[3].c_str())));
        const std::string access = textOf(o, "accessType", "write");
        d.access = access == "read" ? DataAccess::Read : access == "readWrite" ? DataAccess::ReadWrite : DataAccess::Write;
        readSpec(o, &d);
        requests.push_back(d);
    }
    const auto statuses = breakpoints_.setData(requests);
    applyBreakpoints();
    Value::Array list;
    for (const auto& st : statuses) list.push_back(statusObject(st));
    body->push_back({"breakpoints", Value(std::move(list))});
}

// ── Execution ─────────────────────────────────────────────────────────────

void DapSession::continueRequest(Body* body, std::string* error) {
    if (!ready(error)) return;
    resume();
    body->push_back({"allThreadsContinued", Value(true)});
}

void DapSession::pause(std::string* error) {
    if (!ready(error)) return;
    pauseMachine(false);
}

void DapSession::step(const Value& args, StepKind kind, std::string* error) {
    if (!ready(error)) return;
    const bool instruction = textOf(args, "granularity") == "instruction";
    const DapTargets t = targets();
    if (cu1600::dap::step(runControl(), t, map_, threadOf(args), kind, !instruction).ok) noteRunning();
}

// ── Code and memory ───────────────────────────────────────────────────────

void DapSession::disassemble(const Value& args, Body* body, std::string* error) {
    if (!ready(error)) return;
    int thread = 1;
    std::uint16_t base = 0;
    bool me1 = false;
    if (!memoryArgs(args, &thread, &base, &me1, error)) return;
    const DapTargets t = targets();
    const int instructionOffset = static_cast<int>(intOf(args, "instructionOffset", 0));
    const int count = static_cast<int>(intOf(args, "instructionCount", 0));

    // Instruction boundaries before `base`: decode forward from a little
    // earlier and keep a start that lands exactly on `base`.
    std::uint16_t start = base;
    if (instructionOffset < 0) {
        const int back = -instructionOffset;
        std::vector<std::uint16_t> best;
        for (int slack = 0; slack < 6 && best.size() < std::size_t(back); slack++) {
            std::vector<std::uint16_t> starts;
            std::uint16_t a = std::uint16_t(base - back * 4 - slack);
            while (a != base && std::uint16_t(base - a) <= std::uint16_t(back * 4 + slack)) {
                starts.push_back(a);
                a = std::uint16_t(a + t.decode(thread, a).len);
            }
            if (a == base && starts.size() > best.size()) best = starts;
        }
        if (best.size() >= std::size_t(back)) start = best[best.size() - std::size_t(back)];
        else start = best.empty() ? base : best.front();
    } else {
        for (int i = 0; i < instructionOffset; i++) start = std::uint16_t(start + t.decode(thread, start).len);
    }

    const debug::SourceMap& map = map_;
    Value::Array list;
    std::string lastFile;
    std::uint16_t a = start;
    for (int i = 0; i < count; i++) {
        const disasm::Decoded d = t.decode(thread, a, [&map, thread](std::uint16_t x) { return map.symbolAt(thread, x); });
        std::string bytes;
        for (int k = 0; k < d.len; k++) {
            std::uint8_t b = 0;
            t.peek(thread, debug::kSpaceMain, std::uint16_t(a + k), &b);
            if (!bytes.empty()) bytes += ' ';
            bytes += hexBare(b, 2);
        }
        Value::Object ins{{"address", Value(memoryReference(thread, a))},
                          {"instructionBytes", Value(bytes)},
                          {"instruction", Value(d.text)}};
        const std::string label = map.symbolAt(thread, a);
        if (!label.empty()) put(ins, "symbol", Value(label));
        debug::SourceLocation loc;
        if (map_.lookup(thread, a, t.bankMatch(), &loc, t.codePeek())) {
            if (loc.file != lastFile) put(ins, "location", sourceObject(loc.file));
            lastFile = loc.file;
            put(ins, "line", Value(loc.line));
        }
        list.push_back(Value(std::move(ins)));
        a = std::uint16_t(a + d.len);
    }
    body->push_back({"instructions", Value(std::move(list))});
}

void DapSession::readMemory(const Value& args, Body* body, std::string* error) {
    if (!ready(error)) return;
    int thread = 1;
    std::uint16_t addr = 0;
    bool me1 = false;
    if (!memoryArgs(args, &thread, &addr, &me1, error)) return;
    const int count = static_cast<int>(std::min<std::int64_t>(intOf(args, "count", 0), 0x10000));
    const DapTargets t = targets();
    std::string data;
    int unreadable = 0;
    for (int i = 0; i < count; i++) {
        std::uint8_t b = 0;
        if (!t.peek(thread, me1 ? debug::kSpaceME1 : debug::kSpaceMain, std::uint16_t(addr + i), &b)) {
            if (data.empty()) {   // leading unreadable bytes are reported, not sent
                unreadable++;
                continue;
            }
            break;
        }
        data += static_cast<char>(b);
    }
    body->push_back({"address", Value(memoryReference(thread, std::uint16_t(addr + unreadable), me1))});
    body->push_back({"data", Value(base64(data))});
    if (unreadable) body->push_back({"unreadableBytes", Value(unreadable)});
}

void DapSession::writeMemory(const Value& args, Body* body, std::string* error) {
    if (!ready(error)) return;
    int thread = 1;
    std::uint16_t addr = 0;
    bool me1 = false;
    if (!memoryArgs(args, &thread, &addr, &me1, error)) return;
    const std::string data = unbase64(textOf(args, "data"));
    const DapTargets t = targets();
    int written = 0;
    for (const char c : data) {
        if (!t.poke(thread, me1 ? debug::kSpaceME1 : debug::kSpaceMain, std::uint16_t(addr + written), std::uint8_t(c))) break;
        written++;
    }
    body->push_back({"bytesWritten", Value(written)});
    if (written < int(data.size()) && written == 0) *error = "Memory at " + hex(addr, 4) + " isn't writable";
    else
        event("memory", Body{{"memoryReference", Value(memoryReference(thread, addr, me1))},
                             {"offset", Value(0)},
                             {"count", Value(written)}});
}

void DapSession::evaluate(const Value& args, Body* body, std::string* error) {
    if (!ready(error)) return;
    const int frameId = static_cast<int>(intOf(args, "frameId", targets().busOwner() * 1000));
    const debug::ExpressionResult r = debug::evaluate(textOf(args, "expression"), frameContext(frameId));
    if (!r.ok) {
        *error = r.error;
        return;
    }
    const bool address = r.value >= 0 && r.value <= 0xFFFF;
    const std::string text =
        address ? hex(std::uint32_t(r.value), r.value > 0xFF ? 4 : 2) + " (" + std::to_string(r.value) + ")" : std::to_string(r.value);
    body->push_back({"result", Value(text)});
    body->push_back({"variablesReference", Value(0)});
    if (address) body->push_back({"memoryReference", Value(memoryReference(std::max(1, frameThread(frameId)), std::uint16_t(r.value)))});
}

// The display as text, as the harness's `display-text` replies with --json
// (LIB-11 AC-10, AC-19; harness/DisplayText.hpp): {on, rows, indicators:
// [{name, brightness}], notText}, written by the harness's own serialiser.
// Needs no DAP session, only a machine.
void DapSession::displayText(Body* body, std::string* error) {
    calcu::machine::Machine* m = ctx_.machine();
    const calcu::machine::DisplayOutput* d = ctx_.display();
    if (!m || !d) {
        *error = "No machine is running";
        return;
    }
    const std::string json = calcu::harness::writeJson(calcu::harness::displayTextValue(displayTextNow(*m, *d)));
    const std::optional<Value> v = parseJson(json);
    if (v) *body = membersOf(*v);
}

} // namespace cu1600::dap
