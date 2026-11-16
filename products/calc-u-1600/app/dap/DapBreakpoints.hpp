#pragma once

#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "Debug/CpuRegisters.hpp"
#include "Debug/DebugExpression.hpp"
#include "Debug/SourceMap.hpp"
#include "app/dap/DapTargets.hpp"
#include "debugger/Breakpoints.hpp"
#include "debugger/RunState.hpp"

namespace sharp {
class SharpDebugTarget;
}

// ── The DAP's breakpoints (C1600-3 design §3.7, AC-14; LIB-10 design §10.3,
// D-15; built in M-45 from core/Debug/BreakpointTable) ─────────────────────
//
// Everything the DAP client sets -- source-line, instruction, function
// (symbol) and data breakpoints, each with an optional condition, hit
// condition and (not data) log message -- as breakpoints and watches of the
// library's table (LIB-10 AC-10, AC-11) with the setter `dap` (AC-47), the
// qualifier of their binding, and a HitTest (AC-12) that decides here
// whether a hit really pauses: a condition that is false, a hit condition
// not met, a log message (its line is written, no pause) or a bank that is
// not switched in (the library passes it, qualifierMatches) go on; a
// condition that cannot be worked out pauses and says why.
//
// IDs are the DAP's own; dapIds() maps the run state's hits back. apply()
// hands the table everything after each change (and to a new machine).

namespace shared::dap {

inline const calcu::debugger::Setter kDapSetter = "dap";

struct BreakpointSpec {
    std::string condition;     ///< expression; empty = always
    std::string hitCondition;  ///< "5", "== 5", ">= 5", "% 3"; empty = always
    std::string logMessage;    ///< non-empty: a logpoint (never stops); {expr} is interpolated
};

/// A parsed hit condition: "5" / "== 5", ">= 5", "% 3", ... An unreadable
/// one is `valid == false` and always met, so it doesn't hide the
/// breakpoint.
struct HitCondition {
    enum Op : uint8_t { Always, Eq, Ne, Ge, Le, Gt, Lt, Mod };
    Op op = Always;
    long long n = 0;
    bool valid = true;

    static HitCondition parse(const std::string& text);
    /// `hits` counts this hit.
    bool met(int hits) const;
};

/// A log message split once into text and compiled {expr} parts.
class LogTemplate {
public:
    static LogTemplate parse(const std::string& message);
    /// Unevaluable parts become "{error}".
    std::string render(const debug::ExpressionContext& ctx) const;

private:
    struct Part {
        std::string text;             ///< literal text, or the expression's source
        bool expression = false;
        debug::CompiledExpression compiled;
    };
    std::vector<Part> m_parts;
};

/// What the client gets back for each breakpoint it set.
struct BreakpointStatus {
    int id = 0;
    bool verified = false;
    int line = 0;              ///< source breakpoints: the line it resolved to
    std::string message;       ///< why it's unverified, or which bank it's bound to
    bool hasAddress = false;
    int thread = 0;
    uint16_t addr = 0;
};

enum class DataAccess : uint8_t { Read, Write, ReadWrite };

struct DataBreakpointSpec : BreakpointSpec {
    int thread = 1;
    debug::Space space = debug::kSpaceMain;
    uint16_t addr = 0;
    uint16_t length = 1;
    DataAccess access = DataAccess::Write;
};

class DapBreakpoints {
public:
    using SymbolLookup = std::function<bool(const std::string& name, int64_t* value)>;

    struct SourceRequest : BreakpointSpec { int line = 0; };
    struct InstructionRequest : BreakpointSpec { int thread = 1; uint16_t addr = 0; };
    struct FunctionRequest : BreakpointSpec { std::string name; };

    /// Replaces the breakpoints of one source file.
    std::vector<BreakpointStatus> setSource(const std::string& file, const std::vector<SourceRequest>& requests,
                                            const debug::SourceMap& map);
    std::vector<BreakpointStatus> setInstructions(const std::vector<InstructionRequest>& requests);
    /// Function breakpoints resolve their name through the source map's
    /// symbols; each is armed on the CPU of the binding that defines it,
    /// with that binding's bank qualifier.
    std::vector<BreakpointStatus> setFunctions(const std::vector<FunctionRequest>& requests, const debug::SourceMap& map);
    std::vector<BreakpointStatus> setData(const std::vector<DataBreakpointSpec>& requests);
    void clear();
    /// A one-shot stop at `addr` (Build & Load's stopOnEntry): it stops
    /// once (reason "entry"), then disappears.
    void setEntry(int thread, uint16_t addr);

    /// Re-resolves source and function breakpoints after the source map
    /// changed (a new load, a stale listing). Returns the ones whose status
    /// changed, for "breakpoint changed" events.
    std::vector<BreakpointStatus> reresolve(const debug::SourceMap& map);

    /// Hands the library's table every breakpoint and watch with the setter
    /// `dap` (the ones of before go first). `symbols` for conditions and log
    /// messages. Nothing without targets.
    void apply(calcu::debugger::BreakpointTable& table, const DapTargets& targets, SymbolLookup symbols);

    /// The DAP IDs of a pause's hits with the setter `dap`, and whether the
    /// entry stop is among them.
    std::vector<int> dapIds(const std::vector<calcu::debugger::RunState::Hit>& hits) const;
    bool entryHit(const std::vector<calcu::debugger::RunState::Hit>& hits) const;
    /// The lines of log messages and of conditions that could not be worked
    /// out, since the last call (the session sends them as `output`).
    std::vector<std::string> takeOutput();

private:
    /// A spec's condition, hit condition and log message, parsed once when
    /// the breakpoint is set; shared by every address it arms.
    struct Compiled {
        bool hasCondition = false;
        debug::CompiledExpression condition;
        HitCondition hit;
        bool hasLog = false;
        LogTemplate log;
    };
    static std::shared_ptr<const Compiled> compile(const BreakpointSpec& spec);

    struct Armed : BreakpointSpec {
        int id = 0;
        int thread = 1;
        uint16_t addr = 0;
        debug::BankKey key;
        int hits = 0;
        std::shared_ptr<const Compiled> compiled;
    };
    struct Source {
        std::string file;
        std::vector<SourceRequest> requests;
        std::vector<int> ids; // one per request, stable across re-resolves
    };
    struct Function {
        FunctionRequest request;
        int id = 0;
    };
    struct Data : DataBreakpointSpec {
        int id = 0;
        int hits = 0;
        std::shared_ptr<const Compiled> compiled;
    };

    std::vector<BreakpointStatus> resolveSource(Source& s, const debug::SourceMap& map);
    std::vector<BreakpointStatus> resolveFunctions(const debug::SourceMap& map);
    void forgetStatus(int id);
    /// Whether a hit pauses (AC-12, AC-14): the condition, the hit count,
    /// the log message.
    bool passes(const BreakpointSpec& spec, const Compiled& compiled, int& hits, const sharp::SharpDebugTarget& target,
                const SymbolLookup& symbols);

    std::vector<Source> m_sources;
    std::vector<Armed> m_sourceArmed;       // from m_sources
    std::vector<Armed> m_instructionArmed;
    std::vector<Function> m_functions;
    std::vector<Armed> m_functionArmed;
    std::vector<Data> m_data;
    std::vector<BreakpointStatus> m_lastStatus; // source + function, for reresolve() diffs
    int m_nextId = 1;
    bool m_entryArmed = false;
    int m_entryThread = 1;
    uint16_t m_entryAddr = 0;

    std::map<calcu::debugger::BreakpointId, int> m_dapIdOf;   // the library's ID -> the DAP's
    std::optional<calcu::debugger::BreakpointId> m_entryId;
    std::vector<std::string> m_output;
};

/// Hit-condition check: `hits` is the count including this hit.
bool hitConditionMet(const std::string& hitCondition, int hits, bool* valid = nullptr);
/// Expands {expr} in a log message; unevaluable parts become "{error}".
std::string interpolateLog(const std::string& message, const debug::ExpressionContext& ctx);

} // namespace shared::dap
