// The set-up of a DAP session (C1600-3 design §3.4; AC-4 to AC-7, AC-17,
// AC-24, AC-25; EC-2 to EC-5): `attach`/`launch`, `restart`,
// `calcu1600/load` and `calcu1600/reset`, each one operation of the app
// session (D-1), answered at its end. So the messages that arrive meanwhile
// wait (AC-8), and the rules of operations hold inside: no breakpoint or
// watch pauses it (LIB-10 AC-13), a reset is over when the machine stands
// at the reset vector (AC-24). 0.8.1 did the same in a nested event loop of
// the Qt frontend (DebugController::runAndWait).
//
// GCC 13: no temporaries inside a co_await expression (named locals).

#include <cstdio>
#include <functional>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <utility>

#include "Debug/AttachKeys.hpp"
#include "Debug/Listing/Listing.hpp"
#include "Debug/ProgramLoader.hpp"
#include "app/dap/DapConfig.hpp"
#include "app/dap/DapSession.hpp"
#include "debugger/DebuggerPart.hpp"
#include "debugger/RunControl.hpp"
#include "machine/Machine.hpp"
#include "presets/PresetsPart.hpp"
#include "sessions/MachineSession.hpp"
#include "settings/Startup.hpp"
#include "startup/MachineControl.hpp"
#include "typing/TypingPart.hpp"

namespace cu1600::dap {

using calcu::sessions::OperationContext;
using calcu::sessions::OperationResult;
using calcu::sessions::Task;

namespace {

std::string hex4(std::uint16_t v) {
    char buf[8];
    std::snprintf(buf, sizeof buf, "%04X", v);
    return buf;
}

std::string fileName(const std::string& path) {
    const std::size_t at = path.find_last_of("/\\");
    return at == std::string::npos ? path : path.substr(at + 1);
}

} // namespace

// ── Starting and ending ───────────────────────────────────────────────────

bool DapSession::startOperation(const Value& request, std::string* error) {
    if (!ready(error)) return false;
    if (ctx_.busy()) {   // the part holds messages while one runs (AC-8); a safety only
        *error = "Another operation is running.";
        return false;
    }
    opRunning_ = true;
    tellStopAtEnd_ = false;
    resumeAtEnd_ = false;
    pendingRequest_ = request;
    const bool started = ctx_.startOperation([this, request](OperationContext& op) { return runRequest(op, request); });
    if (!started) {
        opRunning_ = false;
        pendingRequest_.reset();
        *error = "Another operation is running.";
        return false;
    }
    return true;
}

void DapSession::operationEnded() {
    if (!opRunning_) return;
    opRunning_ = false;
    seenGeneration_ = ctx_.machineSession().generation();   // its builds are no news
    if (pendingRequest_) {   // stopped before its end (a model switch, a Reset of the menu)
        const Value request = std::move(*pendingRequest_);
        pendingRequest_.reset();
        respond(request, false, {}, "The operation was stopped.");
        lastPaused_ = runControl().paused();
        return;
    }
    if (resumeAtEnd_) resume();
    else if (tellStopAtEnd_) noteRunning();   // the pause it left is told (entry)
    else lastPaused_ = runControl().paused();  // the set-up's pause is not told
    if (tellStopAtEnd_) entryPending_ = true;
    poll();
}

Task<OperationResult> DapSession::runRequest(OperationContext& op, Value request) {
    const std::string command = textOf(request, "command");
    const Value* argsPtr = request.find("arguments");
    const Value args = argsPtr && argsPtr->members() ? *argsPtr : Value(Value::Object{});
    std::string error;
    Body body;
    bool ok = true;
    if (command == "attach" || command == "launch") {
        launchArgs_ = args;
        ok = effectiveConfig(args, deps_.presets->readSetup(), deps_.bundle, &attachConfig_, &error);
        if (ok) {
            stopOnEntry_ = boolOf(attachConfig_, "stopOnEntry");
            const Value config = attachConfig_;
            ok = co_await setUp(op, config, &error);
        }
    } else if (command == "restart") {
        // A restart carries the (possibly edited) configuration and reads
        // the project again; `reset: none` is taken as `reset`.
        const Value* again = args.find("arguments");
        if (again && again->members() && !again->members()->empty()) launchArgs_ = *again;
        ok = effectiveConfig(launchArgs_, deps_.presets->readSetup(), deps_.bundle, &attachConfig_, &error);
        if (ok) {
            Value::Object config = membersOf(attachConfig_);
            if (textOf(attachConfig_, "reset", "none") == "none") put(config, "reset", Value("reset"));
            stopOnEntry_ = boolOf(attachConfig_, "stopOnEntry");
            const Value configValue(std::move(config));
            ok = co_await setUp(op, configValue, &error);
            if (ok && stopOnEntry_) tellStopAtEnd_ = true;
            else if (ok) resumeAtEnd_ = true;
        }
    } else if (command == "calcu1600/load") {
        // The project may have changed since the attach.
        ok = effectiveConfig(launchArgs_, deps_.presets->readSetup(), deps_.bundle, &attachConfig_, &error);
        const Value* configured = attachConfig_.find("program");
        if (ok && (args.find("bin") || (configured && configured->members()))) {
            const Value program = args.find("bin") ? args : *configured;
            ok = co_await buildAndLoad(op, program, /*inSetup=*/false, &body, &error);
        } else if (ok) {
            // AC-25: no program (a ROM project): the set-up again, then as at
            // configurationDone.
            stopOnEntry_ = boolOf(attachConfig_, "stopOnEntry");
            const Value config = attachConfig_;
            ok = co_await setUp(op, config, &error);
            if (ok && stopOnEntry_) tellStopAtEnd_ = true;
            else if (ok) resumeAtEnd_ = true;
        }
    } else if (command == "calcu1600/reset") {
        const bool all = textOf(args, "kind") == "allReset";
        ok = resetBare(all, &error);
        if (ok && boolOf(args, "stop", true)) tellStopAtEnd_ = true;
        else if (ok) resumeAtEnd_ = true;
    }
    pendingRequest_.reset();
    respond(request, ok, std::move(body), error);
    co_return OperationResult{};
}

// ── S1 to S8 ──────────────────────────────────────────────────────────────

void DapSession::bindStatic(const Value& v, bool isListing) {
    Value item = v;
    if (v.string()) item = Value::object({{"path", v}});
    const std::string path = textOf(item, "path");
    debug::Listing listing;
    std::vector<std::string> warnings;
    const bool ok = isListing ? debug::loadListingWithSymbols(path, textOf(item, "source"), {}, &listing, &warnings)
                              : debug::loadListingWithSymbols({}, {}, {path}, &listing, &warnings);
    for (const std::string& w : warnings) output(w, ok ? "console" : "important");
    if (ok) map_.addStatic(threadForCpu(textOf(item, "cpu")), std::move(listing), bankKeyOf(item), path);
}

Task<bool> DapSession::setUp(OperationContext& op, Value config, std::string* error) {
    if (!checkAttach(config, debug::attach::kBlockKeys, error)) co_return false;
    calcu::debugger::RunControl& rc = runControl();
    const bool bootDebug = config.find("boot") != nullptr;
    const std::string preset = textOf(config, "preset");
    const Value* programPtr = config.find("program");
    const bool hasProgram = programPtr && programPtr->members() && !programPtr->members()->empty();
    const std::string command = textOf(config, "command");
    // S2
    if (bootDebug && (hasProgram || !command.empty())) {
        *error = "\"boot\": \"debug\" runs the boot under the debugger; it takes no \"program\" or \"command\"";
        co_return false;
    }
    // S3: a preset builds the machine; with a program, the clean start of S7
    // takes it instead. `boot: debug` builds only, paused before the first
    // instruction (LIB-6 AC-55, LIB-10 AC-46).
    if (!preset.empty() && !hasProgram) {
        if (bootDebug) rc.pauseAtStart();
        const std::optional<calcu::machine::ModelId> anyModel;
        const OperationResult r = co_await deps_.presets->loadWithin(op, preset, bootDebug, anyModel);
        if (!r.ok) {
            *error = "Preset failed: " + r.message;
            co_return false;
        }
    }
    if (!ctx_.machine()) {
        *error = "No machine is running";
        co_return false;
    }
    // S4: Reset or Cold boot, standing at the reset vector (AC-24, D-2).
    const std::string reset = textOf(config, "reset", "none");
    if (reset == "reset" || reset == "allReset") {
        if (!resetBare(reset == "allReset", error)) co_return false;
    }
    // S5: the listings and symbols of the configuration.
    map_.clear();
    const Value* listings = config.find("listings");
    if (listings && listings->elements())
        for (const Value& v : *listings->elements()) bindStatic(v, true);
    const Value* symbols = config.find("symbols");
    if (symbols && symbols->elements())
        for (const Value& v : *symbols->elements()) bindStatic(v, false);
    sendBreakpointChanges(rebindListings());
    for (const auto& b : map_.bindings())
        if (b.stale)
            output(b.name + ": " + std::to_string(b.mismatched) + " of " + std::to_string(b.checked) +
                       " lines don't match memory -- not used for source",
                   "important");
    // S6, S7
    if (hasProgram) {
        const Value program = *programPtr;
        const bool loaded = co_await buildAndLoad(op, program, /*inSetup=*/true, nullptr, error);
        if (!loaded) co_return false;
    } else if (!command.empty()) {
        typeCommand(command);   // typed when the machine runs
    }
    // S8: the session goes on with a paused machine (AC-4).
    co_await pauseNow(op);
    co_return true;
}

Task<void> DapSession::pauseNow(OperationContext& op) {
    calcu::debugger::RunControl& rc = runControl();
    if (rc.paused() || !ctx_.machine()) co_return;
    rc.pause(calcu::debugger::kCpu);
    calcu::sessions::MachineSession& ms = ctx_.machineSession();
    // Machine requests of their own, not OperationContext::run: that one gives
    // the host its call once the machine is paused (LIB-10 G12), which in
    // virtual time would leave the operation unfinished.
    for (int i = 0; i < 1000 && !rc.paused() && ms.machine(); ++i) {
        const calcu::machine::Ticks piece = OperationContext::runPiece(ms.machine()->descriptor().tickRate);
        std::function<std::uint64_t()> call = [&ms, piece]() -> std::uint64_t {
            bool stopped = false;
            return ms.advance(piece, stopped);
        };
        co_await calcu::sessions::request(std::move(call));
    }
}

bool DapSession::resetBare(bool coldBoot, std::string* error) {
    calcu::machine::Machine* m = ctx_.machine();
    if (!m) {
        *error = "No machine is running";
        return false;
    }
    // Without the run to key readiness (D-2), but like any reset: input and
    // typing dropped; the clocks are set when the operation ends (LIB-2).
    ctx_.machineSession().dropInput();
    deps_.typing->engine().focusLost();
    runControl().pauseAtStart();
    if (coldBoot) m->coldBoot();
    else m->reset();
    ctx_.tellRestarted(coldBoot ? calcu::sessions::Restart::ColdBoot : calcu::sessions::Restart::Reset);
    return true;
}

// ── S7: Build & Load ──────────────────────────────────────────────────────

Task<bool> DapSession::cleanStart(OperationContext& op, std::string* how, std::string* error) {
    const std::string preset = textOf(attachConfig_, "preset");
    if (!preset.empty()) {
        *how = "preset " + preset;
        const std::optional<calcu::machine::ModelId> anyModel;
        const OperationResult r = co_await deps_.presets->loadWithin(op, preset, false, anyModel);
        if (!r.ok) *error = r.message;
        co_return r.ok;
    }
    calcu::machine::Machine* m = ctx_.machine();
    if (!m) {
        *error = "No machine is running";
        co_return false;
    }
    const calcu::machine::ModelId model = m->model();
    const std::string startup = deps_.settings ? calcu::settings::startupPreset(*deps_.settings, model.value) : std::string();
    if (!startup.empty()) {
        // The model's startup preset, as LIB-18 loads it; one for another
        // model is refused (EC-4).
        *how = "startup preset " + startup;
        const std::optional<calcu::machine::ModelId> forModel = model;
        const OperationResult r = co_await deps_.presets->loadWithin(op, startup, false, forModel);
        if (!r.ok) *error = r.message;
        co_return r.ok;
    }
    // No preset: the machine as Cold boot leaves it, at key readiness.
    *how = "Cold boot (no startup preset for this model)";
    ctx_.machineSession().dropInput();
    m->coldBoot();
    ctx_.tellRestarted(calcu::sessions::Restart::ColdBoot);
    co_await deps_.control->untilKeyReady(op);
    co_return true;
}

Task<bool> DapSession::buildAndLoad(OperationContext& op, Value d, bool inSetup, Body* body, std::string* error) {
    if (!checkAttach(d, debug::attach::kProgramKeys, error)) co_return false;
    if (d.find("slot")) {   // EC-2
        *error = "Bad \"slot\": it was removed: the loader places code by the machine's MODE, the program area TITLE "
                 "selects, and the address. Set MODE / TITLE on the calculator first.";
        co_return false;
    }
    const std::string bin = textOf(d, "bin");
    if (bin.empty()) {   // EC-2
        *error = "Bad \"bin\": it is missing";
        co_return false;
    }
    debug::LoadRequest req;
    req.bin = debug::absolutePath(bin);
    if (const std::string listing = textOf(d, "listing"); !listing.empty()) req.listing = debug::absolutePath(listing);
    if (const std::string source = textOf(d, "source"); !source.empty()) req.source = debug::absolutePath(source);
    if (const Value* s = d.find("symbols"); s && s->elements())
        for (const Value& p : *s->elements())
            if (p.string()) req.symbols.push_back(debug::absolutePath(*p.string()));
    req.thread = threadForCpu(textOf(d, "cpu"));
    req.key = bankKeyOf(d);
    if (const Value* a = d.find("address")) req.hasAddress = numberOf(*a, 0xFFFF, &req.address);
    // A number is the entry; text is a symbol of the listing or an address,
    // which the loader resolves once the listing is read.
    if (const Value* entry = d.find("entry")) {
        if (entry->integer() || entry->real()) {
            std::uint32_t e = 0;
            if (!numberOf(*entry, 0xFFFF, &e)) {
                *error = "Bad \"entry\": an address from 0 to &FFFF, or a symbol of the listing";
                co_return false;
            }
            req.hasEntry = true;
            req.entry = std::uint16_t(e);
        } else if (entry->string()) {
            std::string s = *entry->string();
            while (!s.empty() && s.back() == ' ') s.pop_back();
            while (!s.empty() && s.front() == ' ') s.erase(s.begin());
            req.entrySymbol = s;
        }
    }
    const std::string after = textOf(d, "after", "none");   // OQ-2: none by default
    // A clean machine first, unless the load says otherwise.
    if (boolOf(d, "cleanStart", true)) {
        std::string how, why;
        const bool clean = co_await cleanStart(op, &how, &why);
        if (!clean) {
            *error = "Clean start failed: " + why;
            co_return false;
        }
        output("Clean start: " + how);
    }
    calcu::machine::Machine* m = ctx_.machine();
    const DapTargets t = targets();
    if (!m || !t.valid()) {
        *error = "No machine is running";
        co_return false;
    }
    debug::LoadResult r = debug::loadProgram(*m, req, map_, t.bankMatch(), t.codePeek());
    if (!r.ok) {
        *error = "Load failed: " + r.error;
        co_return false;
    }
    if (after != "none") {
        if (const std::string command = textOf(attachConfig_, "command"); !command.empty()) r.callCommand = command;
        if (r.callCommand.empty()) {
            r.warnings.push_back("no BASIC command starts this code; load it, then call it from your own code");
        } else {
            if (after == "stopOnEntry") breakpoints_.setEntry(req.thread, r.entry);
            applyBreakpoints();
            typeCommand(r.callCommand);   // typed and entered as the machine runs
            // In the set-up the program runs from configurationDone (AC-7,
            // ledger 1600-20); else at the end of this operation.
            if (!inSetup) resumeAtEnd_ = true;
        }
    }
    for (const std::string& w : r.warnings) output(w, "important");
    std::string message = "Loaded " + fileName(req.bin) + " at " + hex4(r.lo) + "-" + hex4(r.hi);
    if (!r.callCommand.empty()) message += "; start with " + r.callCommand;
    output(message);
    if (body) {
        body->push_back({"start", Value(memoryReference(req.thread, r.lo))});
        body->push_back({"end", Value(memoryReference(req.thread, r.hi))});
        body->push_back({"entry", Value(memoryReference(req.thread, r.entry))});
        body->push_back({"call", Value(r.callCommand)});
    }
    sendBreakpointChanges(rebindListings());
    co_return true;
}

} // namespace cu1600::dap
