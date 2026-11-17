#include "app/dap/DapBreakpoints.hpp"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <set>
#include <utility>
#include <variant>

#include "Debug/SharpDebugging.hpp"

namespace cu1600::dap {

using namespace debug;

namespace {

std::string trim(const std::string& s) {
    size_t a = 0, b = s.size();
    while (a < b && std::isspace(static_cast<unsigned char>(s[a]))) a++;
    while (b > a && std::isspace(static_cast<unsigned char>(s[b - 1]))) b--;
    return s.substr(a, b - a);
}

std::string valueText(int64_t v) {
    char buf[24];
    if (v < 0) std::snprintf(buf, sizeof buf, "%lld", static_cast<long long>(v));
    else if (v <= 0xFF) std::snprintf(buf, sizeof buf, "0x%02llX", static_cast<unsigned long long>(v));
    else std::snprintf(buf, sizeof buf, "0x%04llX", static_cast<unsigned long long>(v));
    return buf;
}

BreakpointStatus verifiedStatus(int id, int thread, uint16_t addr) {
    BreakpointStatus st;
    st.id = id;
    st.verified = true;
    st.hasAddress = true;
    st.thread = thread;
    st.addr = addr;
    return st;
}

bool sameStatus(const BreakpointStatus& a, const BreakpointStatus& b) {
    return a.verified == b.verified && a.line == b.line && a.message == b.message && a.addr == b.addr &&
           a.thread == b.thread;
}

} // namespace

HitCondition HitCondition::parse(const std::string& text) {
    HitCondition h;
    std::string t = trim(text);
    if (t.empty()) return h;
    Op op = Eq;
    struct Spelling { const char* text; Op op; };
    static const Spelling kOps[] = {{"==", Eq}, {">=", Ge}, {"<=", Le}, {"!=", Ne}, {">", Gt}, {"<", Lt}, {"%", Mod}, {"=", Eq}};
    for (const Spelling& s : kOps) {
        const size_t n = std::char_traits<char>::length(s.text);
        if (t.compare(0, n, s.text) == 0) { op = s.op; t = trim(t.substr(n)); break; }
    }
    char* end = nullptr;
    const long long n = std::strtoll(t.c_str(), &end, 0);
    if (t.empty() || !end || *end != '\0') {
        h.valid = false; // an unreadable hit condition doesn't hide the breakpoint
        return h;
    }
    h.op = op;
    h.n = n;
    return h;
}

bool HitCondition::met(int hits) const {
    switch (op) {
        case Always: return true;
        case Eq: return hits == n;
        case Ne: return hits != n;
        case Ge: return hits >= n;
        case Le: return hits <= n;
        case Gt: return hits > n;
        case Lt: return hits < n;
        case Mod: return n > 0 && hits % n == 0;
    }
    return true;
}

bool hitConditionMet(const std::string& hitCondition, int hits, bool* valid) {
    const HitCondition h = HitCondition::parse(hitCondition);
    if (valid) *valid = h.valid;
    return h.met(hits);
}

LogTemplate LogTemplate::parse(const std::string& message) {
    LogTemplate t;
    std::string text;
    for (size_t i = 0; i < message.size(); i++) {
        if (message[i] != '{') { text += message[i]; continue; }
        const size_t close = message.find('}', i);
        if (close == std::string::npos) { text += message.substr(i); break; }
        if (!text.empty()) t.m_parts.push_back({text, false, {}});
        text.clear();
        const std::string expr = message.substr(i + 1, close - i - 1);
        t.m_parts.push_back({expr, true, CompiledExpression::compile(expr)});
        i = close;
    }
    if (!text.empty()) t.m_parts.push_back({text, false, {}});
    return t;
}

std::string LogTemplate::render(const ExpressionContext& ctx) const {
    std::string out;
    for (const Part& p : m_parts) {
        if (!p.expression) { out += p.text; continue; }
        const ExpressionResult r = p.compiled.run(ctx);
        out += r.ok ? valueText(r.value) : "{" + r.error + "}";
    }
    return out;
}

std::string interpolateLog(const std::string& message, const ExpressionContext& ctx) {
    return LogTemplate::parse(message).render(ctx);
}

std::shared_ptr<const DapBreakpoints::Compiled> DapBreakpoints::compile(const BreakpointSpec& spec) {
    auto c = std::make_shared<Compiled>();
    c->hasCondition = !trim(spec.condition).empty();
    if (c->hasCondition) c->condition = CompiledExpression::compile(spec.condition);
    c->hit = HitCondition::parse(spec.hitCondition);
    c->hasLog = !spec.logMessage.empty();
    if (c->hasLog) c->log = LogTemplate::parse(spec.logMessage);
    return c;
}

// ── Setting ───────────────────────────────────────────────────────────────

std::vector<BreakpointStatus> DapBreakpoints::resolveSource(Source& s, const SourceMap& map) {
    std::vector<BreakpointStatus> out;
    const std::set<int> ids(s.ids.begin(), s.ids.end());
    m_sourceArmed.erase(std::remove_if(m_sourceArmed.begin(), m_sourceArmed.end(),
                                       [&ids](const Armed& a) { return ids.count(a.id) > 0; }),
                        m_sourceArmed.end());
    for (size_t i = 0; i < s.requests.size(); i++) {
        const SourceRequest& r = s.requests[i];
        int resolved = 0;
        const std::vector<SourceMap::CodeAddress> addrs = map.addressesFor(s.file, r.line, &resolved);
        BreakpointStatus st;
        if (addrs.empty()) {
            st.id = s.ids[i];
            st.line = r.line;
            st.message = "No code at or after this line in a loaded listing";
        } else {
            st = verifiedStatus(s.ids[i], addrs[0].thread, addrs[0].addr);
            st.line = resolved;
            std::string banks;
            const std::shared_ptr<const Compiled> compiled = compile(r);
            for (const auto& a : addrs) {
                Armed armed;
                static_cast<BreakpointSpec&>(armed) = r;
                armed.compiled = compiled;
                armed.id = st.id;
                armed.thread = a.thread;
                armed.addr = a.addr;
                armed.key = a.key;
                m_sourceArmed.push_back(armed);
                if (a.key.any()) banks += (banks.empty() ? "" : "; ") + a.key.describe();
            }
            if (!banks.empty()) st.message = "Only while " + banks;
        }
        out.push_back(st);
    }
    return out;
}

std::vector<BreakpointStatus> DapBreakpoints::setSource(const std::string& file, const std::vector<SourceRequest>& requests,
                                                         const SourceMap& map) {
    auto it = std::find_if(m_sources.begin(), m_sources.end(), [&file](const Source& s) { return s.file == file; });
    if (it == m_sources.end()) {
        m_sources.push_back({file, {}, {}});
        it = m_sources.end() - 1;
    }
    // Drop the file's old breakpoints, then set the new ones with fresh ids.
    it->requests.clear();
    resolveSource(*it, map);
    for (int id : it->ids) forgetStatus(id);
    it->requests = requests;
    it->ids.clear();
    for (size_t i = 0; i < requests.size(); i++) it->ids.push_back(m_nextId++);
    std::vector<BreakpointStatus> out = resolveSource(*it, map);
    m_lastStatus.insert(m_lastStatus.end(), out.begin(), out.end());
    return out;
}

std::vector<BreakpointStatus> DapBreakpoints::setInstructions(const std::vector<InstructionRequest>& requests) {
    m_instructionArmed.clear();
    std::vector<BreakpointStatus> out;
    for (const InstructionRequest& r : requests) {
        Armed a;
        static_cast<BreakpointSpec&>(a) = r;
        a.compiled = compile(r);
        a.id = m_nextId++;
        a.thread = r.thread;
        a.addr = r.addr;
        m_instructionArmed.push_back(a);
        out.push_back(verifiedStatus(a.id, r.thread, r.addr));
    }
    return out;
}

std::vector<BreakpointStatus> DapBreakpoints::resolveFunctions(const SourceMap& map) {
    m_functionArmed.clear();
    std::vector<BreakpointStatus> out;
    for (const Function& f : m_functions) {
        SourceMap::SymbolInfo sym;
        if (map.findSymbol(f.request.name, &sym)) {
            Armed a;
            static_cast<BreakpointSpec&>(a) = f.request;
            a.compiled = compile(f.request);
            a.id = f.id;
            a.thread = sym.thread;
            a.addr = sym.value;
            a.key = sym.key;
            m_functionArmed.push_back(a);
            BreakpointStatus st = verifiedStatus(f.id, sym.thread, sym.value);
            if (sym.key.any()) st.message = "Only while " + sym.key.describe();
            out.push_back(st);
        } else {
            BreakpointStatus st;
            st.id = f.id;
            st.message = "Unknown symbol '" + f.request.name + "'";
            out.push_back(st);
        }
    }
    return out;
}

std::vector<BreakpointStatus> DapBreakpoints::setFunctions(const std::vector<FunctionRequest>& requests,
                                                            const SourceMap& map) {
    for (const Function& f : m_functions) forgetStatus(f.id);
    m_functions.clear();
    for (const FunctionRequest& r : requests) m_functions.push_back({r, m_nextId++});
    std::vector<BreakpointStatus> out = resolveFunctions(map);
    m_lastStatus.insert(m_lastStatus.end(), out.begin(), out.end());
    return out;
}

std::vector<BreakpointStatus> DapBreakpoints::setData(const std::vector<DataBreakpointSpec>& requests) {
    m_data.clear();
    std::vector<BreakpointStatus> out;
    for (const DataBreakpointSpec& r : requests) {
        Data d;
        static_cast<DataBreakpointSpec&>(d) = r;
        d.compiled = compile(r);
        d.id = m_nextId++;
        m_data.push_back(d);
        out.push_back(verifiedStatus(d.id, r.thread, r.addr));
    }
    return out;
}

void DapBreakpoints::forgetStatus(int id) {
    m_lastStatus.erase(std::remove_if(m_lastStatus.begin(), m_lastStatus.end(),
                                      [id](const BreakpointStatus& s) { return s.id == id; }),
                       m_lastStatus.end());
}

void DapBreakpoints::setEntry(int thread, uint16_t addr) {
    m_entryArmed = true;
    m_entryThread = thread;
    m_entryAddr = addr;
}

void DapBreakpoints::clear() {
    m_entryArmed = false;
    m_sources.clear();
    m_sourceArmed.clear();
    m_instructionArmed.clear();
    m_functions.clear();
    m_functionArmed.clear();
    m_data.clear();
    m_lastStatus.clear();
}

std::vector<BreakpointStatus> DapBreakpoints::reresolve(const SourceMap& map) {
    std::vector<BreakpointStatus> now;
    for (Source& s : m_sources) {
        std::vector<BreakpointStatus> r = resolveSource(s, map);
        now.insert(now.end(), r.begin(), r.end());
    }
    std::vector<BreakpointStatus> f = resolveFunctions(map);
    now.insert(now.end(), f.begin(), f.end());
    std::vector<BreakpointStatus> changed;
    for (const BreakpointStatus& st : now) {
        auto old = std::find_if(m_lastStatus.begin(), m_lastStatus.end(),
                                [&st](const BreakpointStatus& o) { return o.id == st.id; });
        if (old == m_lastStatus.end() || !sameStatus(*old, st)) changed.push_back(st);
    }
    m_lastStatus = now;
    return changed;
}

// ── Arming (LIB-10 AC-10 to AC-12, AC-47) ────────────────────────────────

void DapBreakpoints::apply(calcu::debugger::BreakpointTable& table, const DapTargets& targets, SymbolLookup symbols) {
    table.remove(kDapSetter);
    m_dapIdOf.clear();
    m_entryId.reset();
    calcu::machine::Debugging* d = targets.debugging();
    if (!targets.valid()) return;
    struct Applied {   // the targets get the table's addresses and ranges at the end
        calcu::debugger::BreakpointTable& t;
        calcu::machine::Debugging& d;
        ~Applied() { t.apply(d); }
    } applied{table, *d};
    // A breakpoint's test finds its entry by list and place again, so a
    // change in between (set anew, then applied anew) never reaches a stale one.
    std::vector<Armed>* lists[3] = {&m_sourceArmed, &m_instructionArmed, &m_functionArmed};
    for (std::vector<Armed>* list : lists) {
        for (std::size_t i = 0; i < list->size(); ++i) {
            const Armed& a = (*list)[i];
            const sharp::SharpDebugTarget& t = targets.target(a.thread);
            calcu::debugger::HitTest test = [this, list, i, id = a.id, symbols](const calcu::machine::DebugTarget& at) {
                if (i >= list->size() || (*list)[i].id != id) return false;
                Armed& x = (*list)[i];
                return passes(x, *x.compiled, x.hits, static_cast<const sharp::SharpDebugTarget&>(at), symbols);
            };
            const auto r = table.addBreakpoint(kDapSetter, *d, t.info().name, a.addr, t.qualifierOf(a.key), std::move(test));
            if (const auto* lib = std::get_if<calcu::debugger::BreakpointId>(&r)) m_dapIdOf[*lib] = a.id;
        }
    }
    if (m_entryArmed) {
        const sharp::SharpDebugTarget& t = targets.target(m_entryThread);
        calcu::debugger::HitTest once = [this](const calcu::machine::DebugTarget&) {
            if (!m_entryArmed) return false;
            m_entryArmed = false;   // it stops once, then disappears
            return true;
        };
        const auto r = table.addBreakpoint(kDapSetter, *d, t.info().name, m_entryAddr, {}, std::move(once));
        if (const auto* lib = std::get_if<calcu::debugger::BreakpointId>(&r)) m_entryId = *lib;
    }
    for (std::size_t i = 0; i < m_data.size(); ++i) {
        const Data& w = m_data[i];
        const sharp::SharpDebugTarget& t = targets.target(w.thread);
        calcu::machine::WatchRange range;
        range.space = w.space == kSpaceME1 ? 1 : 0;
        range.first = w.addr;
        range.count = w.length ? w.length : 1;
        range.read = w.access != DataAccess::Write;
        range.write = w.access != DataAccess::Read;
        calcu::debugger::HitTest test = [this, i, id = w.id, symbols](const calcu::machine::DebugTarget& at) {
            if (i >= m_data.size() || m_data[i].id != id) return false;
            Data& x = m_data[i];
            return passes(x, *x.compiled, x.hits, static_cast<const sharp::SharpDebugTarget&>(at), symbols);
        };
        const auto r = table.addWatch(kDapSetter, *d, t.info().name, range, std::move(test));
        if (const auto* lib = std::get_if<calcu::debugger::BreakpointId>(&r)) m_dapIdOf[*lib] = w.id;
    }
}

// ── Deciding ──────────────────────────────────────────────────────────────

bool DapBreakpoints::passes(const BreakpointSpec& spec, const Compiled& compiled, int& hits,
                            const sharp::SharpDebugTarget& target, const SymbolLookup& symbols) {
    // Only a condition or a log message evaluates anything.
    const ExpressionContext ctx =
        compiled.hasCondition || compiled.hasLog ? target.expressionContext(symbols) : ExpressionContext{};
    if (compiled.hasCondition) {
        const ExpressionResult r = compiled.condition.run(ctx);
        if (!r.ok) {
            // A broken condition stops, and says why, rather than silently never firing.
            m_output.push_back("Breakpoint condition '" + spec.condition + "': " + r.error);
            return true;
        }
        if (r.value == 0) return false;
    }
    hits++;
    if (!compiled.hit.met(hits)) return false;
    if (compiled.hasLog) {
        m_output.push_back(compiled.log.render(ctx));   // a log message writes its line and does not pause
        return false;
    }
    return true;
}

std::vector<int> DapBreakpoints::dapIds(const std::vector<calcu::debugger::RunState::Hit>& hits) const {
    std::vector<int> ids;
    for (const auto& h : hits) {
        if (h.setter != kDapSetter) continue;
        const auto it = m_dapIdOf.find(h.id);
        if (it != m_dapIdOf.end() && std::find(ids.begin(), ids.end(), it->second) == ids.end()) ids.push_back(it->second);
    }
    return ids;
}

bool DapBreakpoints::entryHit(const std::vector<calcu::debugger::RunState::Hit>& hits) const {
    for (const auto& h : hits)
        if (h.setter == kDapSetter && m_entryId && h.id == *m_entryId) return true;
    return false;
}

std::vector<std::string> DapBreakpoints::takeOutput() { return std::exchange(m_output, {}); }

} // namespace cu1600::dap
