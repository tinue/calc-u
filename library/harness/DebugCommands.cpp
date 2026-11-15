// The debugger's commands (LIB-10 design §3.9; spec AC-20, AC-50, AC-51,
// EC-9): the steps of AC-19 but `debug-panel:` (pause, resume, step,
// pause-on-start, trace) and targets, registers, set-register, peek, poke,
// break, watch, history, disassemble, profile, step-over, step-out,
// pause-at-start, expect-register, expect-memory and symbols, each turned
// into the session command or request of LIB-10's DebuggerPart with the
// setter `harness` (AC-47). A product without a DebuggerPart in its session
// refuses them with "The machine offers no debug target." (design §4).
// Numbers are read as decimal, 0x… or $… and written as 0x with the digits
// of their width (D-4); a name loaded by `symbols` stands for an address.
#include "debugger/RunState.hpp"
#include "formats/Flow.hpp"
#include "formats/Number.hpp"
#include "harness/Commands.hpp"
#include "harness/HarnessHost.hpp"
#include "harness/Run.hpp"
#include "sessions/LibraryTopics.hpp"
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <map>
#include <sstream>

namespace calcu::harness {

namespace dbg = calcu::debugger;
using sessions::ReplyMessage;

namespace {

std::vector<std::string> words(const std::string& text) {
    std::vector<std::string> out;
    std::istringstream in(text);
    for (std::string w; in >> w;) out.push_back(w);
    return out;
}

std::string hexOf(std::uint64_t v, int bits) {
    const int digits = std::max(1, (bits + 3) / 4);
    char buf[32];
    std::snprintf(buf, sizeof buf, "0x%0*llx", digits, static_cast<unsigned long long>(v));
    return buf;
}

// The bits an address of a space of `size` cells needs.
int addressBits(std::uint64_t size) {
    int bits = 4;
    while (bits < 64 && size > 1 && (size - 1) >> bits) bits += 4;
    return bits;
}

std::uint64_t u64(const sessions::Value* v) { return v && v->integer() ? static_cast<std::uint64_t>(*v->integer()) : 0; }
std::string text(const sessions::Value* v) { return v && v->string() ? *v->string() : std::string(); }

// A number as the commands read it (D-4), or the word as it is (a symbol).
sessions::Value numberOrName(const std::string& w) {
    if (auto n = formats::parseNumber(w)) return sessions::Value(*n);
    return sessions::Value(w);
}

bool hasDebugger(CommandContext& c) {
    for (const auto& t : c.host.topics())
        if (t.name == dbg::kRunStateTopic) return true;
    return false;
}

Reply noTarget() { return refusedReply(dbg::kNoTarget); }
Reply needs(const char* usage) { return refusedReply(std::string("usage: ") + usage); }

std::string runStateLine(const sessions::Value& v) {
    if (!v.find("paused") || !*v.find("paused")->boolean()) {
        std::string l = "running";
        if (const auto* a = v.find("armed"); a && a->string()) l += ", armed " + *a->string();
        return l;
    }
    std::string l = "paused, owner " + text(v.find("owner")) + ", reason " + text(v.find("reason"));
    if (const auto* t = v.find("target"); t && t->string()) l += ", target " + *t->string();
    if (const auto* h = v.find("hits"); h && h->elements())
        for (const auto& x : *h->elements())
            l += ", hit " + std::to_string(u64(x.find("id"))) + " (" + text(x.find("setter")) + ")";
    return l;
}

Reply runStateReply(const ReplyMessage& r) {
    if (!r.done) return refusedReply(r.reason);
    if (const auto* s = sessions::payloadAs<dbg::RunState>(r.result)) {
        const sessions::Value v = s->toValue();
        return doneReply(fromSession(v), {runStateLine(v)});
    }
    return doneReply();
}

Reply fromCommand(const ReplyMessage& r) { return r.done ? doneReply() : refusedReply(r.reason); }

ReplyMessage send(CommandContext& c, const sessions::CommandName& n, sessions::Value args) {
    return c.host.command(sessions::Command{n, std::move(args)}, "The machine did not pause.");   // D-6
}
ReplyMessage ask(CommandContext& c, const sessions::RequestName& n, sessions::Value args = {}) {
    return c.host.ask(sessions::Request{n, std::move(args)});
}
sessions::Value valueOf(const ReplyMessage& r) { return r.result ? r.result->toValue() : sessions::Value(); }

// The target's info from `targets`, by name (any case), or the first.
struct TargetView {
    std::string name;
    std::vector<std::pair<std::string, int>> registers;               // name, bits
    std::map<std::string, std::pair<int, std::uint64_t>> spaces;      // name: cell bits, size
    std::string codeSpace;
};
std::optional<TargetView> targetView(CommandContext& c, const std::string& name) {
    const ReplyMessage r = ask(c, dbg::kTargetsRequest);
    if (!r.done) return std::nullopt;
    const sessions::Value v = valueOf(r);
    const auto* ts = v.find("targets");
    if (!ts || !ts->elements()) return std::nullopt;
    for (const auto& t : *ts->elements()) {
        std::string n = text(t.find("name")), a = n, b = name;
        std::transform(a.begin(), a.end(), a.begin(), ::tolower);
        std::transform(b.begin(), b.end(), b.begin(), ::tolower);
        if (!name.empty() && a != b) continue;
        TargetView out;
        out.name = n;
        for (const auto& reg : *t.find("registers")->elements())
            out.registers.emplace_back(text(reg.find("name")), static_cast<int>(u64(reg.find("bits"))));
        for (const auto& sp : *t.find("spaces")->elements())
            out.spaces[text(sp.find("name"))] = {static_cast<int>(u64(sp.find("cellBits"))), u64(sp.find("size"))};
        out.codeSpace = text(t.find("codeSpace"));
        return out;
    }
    return std::nullopt;
}

// The bits of an address of the target's space, 16 when unknown.
int spaceBits(const std::optional<TargetView>& view, const std::string& space) {
    if (!view) return 16;
    const auto it = view->spaces.find(space);
    return it == view->spaces.end() ? 16 : addressBits(it->second.second);
}

// The `key=value` words (bank=, vbank=, file=) taken out of `w`.
std::map<std::string, std::string> takeOptions(std::vector<std::string>& w) {
    std::map<std::string, std::string> o;
    w.erase(std::remove_if(w.begin(), w.end(),
                           [&](const std::string& x) {
                               const auto eq = x.find('=');
                               if (eq == std::string::npos) return false;
                               o[x.substr(0, eq)] = x.substr(eq + 1);
                               return true;
                           }),
            w.end());
    return o;
}

void addBank(sessions::Value::Object& args, const std::map<std::string, std::string>& o) {
    for (const char* k : {"bank", "vbank"})
        if (const auto it = o.find(k); it != o.end()) args.push_back({k, numberOrName(it->second)});
}

std::filesystem::path pathOf(const std::string& s) { return std::filesystem::path(std::u8string(s.begin(), s.end())); }

// ── Run control ──────────────────────────────────────────────────────────

Reply pauseCmd(CommandContext& c) {
    if (!hasDebugger(c)) return noTarget();
    const std::string level = c.line.argText.empty() ? std::string("cpu") : c.line.argText;
    return runStateReply(send(c, dbg::kPauseCommand, sessions::Value::object({{"level", level}})));
}

Reply resumeCmd(CommandContext& c) {
    if (!hasDebugger(c)) return noTarget();
    return runStateReply(send(c, dbg::kResumeCommand, {}));
}

// `step`, `step { level: cpu, count: 3 }`, `step cpu 3`.
Reply stepCmd(CommandContext& c) {
    if (!hasDebugger(c)) return noTarget();
    std::string level = "cpu";
    std::int64_t count = 1;
    const std::string& a = c.line.argText;
    if (!a.empty() && a.front() == '{') {
        formats::YamlNode n;
        std::string error;
        if (!formats::parseValue(a, &n, &error) || !n.isMap()) return refusedReply("step takes { level: program|cpu, count: n }.");
        for (const auto& [k, v] : n.map) {
            const std::string x = formats::scalarText(v);
            if (k == "level") level = x;
            else if (k == "count") {
                const auto n2 = formats::parseNumber(x);
                if (!n2) return refusedReply("step takes a count from 1 to 100000.");
                count = *n2;
            } else return refusedReply("Unknown key " + k + " in step.");
        }
    } else {
        auto w = words(a);
        if (!w.empty() && formats::parseNumber(w[0])) w.insert(w.begin(), "cpu");   // `step 3`
        if (!w.empty()) level = w[0];
        if (w.size() > 1) {
            const auto n2 = formats::parseNumber(w[1]);
            if (!n2) return refusedReply("step takes a count from 1 to 100000.");
            count = *n2;
        }
    }
    if (count < 1 || count > 100000) return refusedReply("step takes a count from 1 to 100000.");
    return runStateReply(send(c, dbg::kStepCommand,
                              sessions::Value::object({{"level", level}, {"count", count}, {"kind", "in"}})));
}

Handler overOut(const char* kind) {
    return [kind](CommandContext& c) {
        if (!hasDebugger(c)) return noTarget();
        sessions::Value::Object o{{"level", "cpu"}, {"count", 1}, {"kind", kind}};
        if (!c.line.argText.empty()) o.push_back({"target", c.line.argText});
        return runStateReply(send(c, dbg::kStepCommand, sessions::Value(std::move(o))));
    };
}

Reply pauseOnStartCmd(CommandContext& c) {
    if (!hasDebugger(c)) return noTarget();
    if (c.line.argText.empty()) return needs("pause-on-start <level>|off");
    return fromCommand(send(c, dbg::kPauseOnStartCommand, sessions::Value::object({{"level", c.line.argText}})));
}

Reply pauseAtStartCmd(CommandContext& c) {
    if (!hasDebugger(c)) return noTarget();
    return fromCommand(send(c, dbg::kPauseAtStartCommand, {}));
}

Reply traceCmd(CommandContext& c) {
    if (!hasDebugger(c)) return noTarget();
    if (c.line.argText.empty()) return needs("trace <file>|off");
    if (c.line.argText == "off") return fromCommand(send(c, dbg::kTraceCommand, sessions::Value::object({{"off", true}})));
    return fromCommand(send(c, dbg::kTraceCommand, sessions::Value::object({{"file", c.line.argText}})));
}

// ── Targets, registers, memory ───────────────────────────────────────────

Reply targetsCmd(CommandContext& c) {
    if (!hasDebugger(c)) return noTarget();
    const ReplyMessage r = ask(c, dbg::kTargetsRequest);
    if (!r.done) return refusedReply(r.reason);
    const sessions::Value v = valueOf(r);
    std::vector<std::string> lines;
    for (const auto& t : *v.find("targets")->elements()) {   // one block per target
        lines.push_back(text(t.find("name")));
        std::string regs = "  registers:";
        for (const auto& reg : *t.find("registers")->elements())
            regs += " " + text(reg.find("name")) + "/" + std::to_string(u64(reg.find("bits"))) +
                    (*reg.find("writable")->boolean() ? "" : " (read-only)");
        lines.push_back(regs);
        for (const auto& sp : *t.find("spaces")->elements()) {
            std::string l = "  space " + text(sp.find("name")) + ": " + std::to_string(u64(sp.find("size"))) + " cells of " +
                            std::to_string(u64(sp.find("cellBits"))) + " bits";
            if (*sp.find("writtenView")->boolean()) l += ", the values last written";
            for (const auto& w : *sp.find("banked")->elements())
                l += ", banked " + hexOf(u64(w.find("first")), addressBits(u64(sp.find("size")))) + "+" +
                     std::to_string(u64(w.find("count"))) + ": " + std::to_string(u64(w.find("banks"))) + " banks" +
                     (u64(w.find("vbanks")) ? ", " + std::to_string(u64(w.find("vbanks"))) + " vertical banks" : "");
            lines.push_back(l);
        }
        lines.push_back("  code space: " + text(t.find("codeSpace")));
        const sessions::Value* o = t.find("offers");
        std::string offers = "  offers:";
        if (*o->find("watches")->boolean()) offers += " watches";
        if (*o->find("stepOverOut")->boolean()) offers += " step-over-out";
        if (*o->find("decode")->boolean()) offers += " decode";
        if (u64(o->find("history"))) offers += " history " + std::to_string(u64(o->find("history")));
        lines.push_back(offers);
        for (const auto& q : *o->find("qualifier")->elements()) {
            std::string l = "  qualifier " + text(q.find("name")) + ":";
            for (const auto& x : *q.find("values")->elements()) l += " " + text(&x);
            l += " (now " + text(q.find("now")) + ")";
            lines.push_back(l);
        }
    }
    return doneReply(fromSession(v), std::move(lines));
}

Reply registersCmd(CommandContext& c) {
    if (!hasDebugger(c)) return noTarget();
    sessions::Value::Object o;
    if (!c.line.argText.empty()) o.push_back({"target", c.line.argText});
    const ReplyMessage r = ask(c, dbg::kRegistersRequest, sessions::Value(std::move(o)));
    if (!r.done) return refusedReply(r.reason);
    const sessions::Value v = valueOf(r);
    std::vector<std::string> lines;
    Value out = Value::object();
    for (const auto& reg : *v.find("registers")->elements()) {
        const std::string h = hexOf(u64(reg.find("value")), static_cast<int>(u64(reg.find("bits"))));
        lines.push_back(text(reg.find("name")) + " = " + h);
        out.set(text(reg.find("name")), h);
    }
    return doneReply(std::move(out), std::move(lines));
}

Reply setRegisterCmd(CommandContext& c) {
    if (!hasDebugger(c)) return noTarget();
    const auto w = words(c.line.argText);
    if (w.size() != 3) return needs("set-register <target> <register> <value>");
    return fromCommand(send(c, dbg::kSetRegisterCommand,
                            sessions::Value::object({{"target", w[0]}, {"register", w[1]}, {"value", numberOrName(w[2])}})));
}

Reply peekCmd(CommandContext& c) {
    if (!hasDebugger(c)) return noTarget();
    auto w = words(c.line.argText);
    const auto opts = takeOptions(w);
    if (w.size() < 3 || w.size() > 4) return needs("peek <target> <space> <address> [<count>] [bank=<b>] [vbank=<v>] [file=<file>]");
    sessions::Value::Object o{{"target", w[0]}, {"space", w[1]}, {"address", numberOrName(w[2])}};
    if (w.size() == 4) o.push_back({"count", numberOrName(w[3])});
    addBank(o, opts);
    const ReplyMessage r = ask(c, dbg::kPeekRequest, sessions::Value(std::move(o)));
    if (!r.done) return refusedReply(r.reason);
    const sessions::Value v = valueOf(r);
    const int bits = static_cast<int>(u64(v.find("cellBits")));
    const auto view = targetView(c, w[0]);
    const int abits = spaceBits(view, text(v.find("space")));
    const auto& cells = *v.find("cells")->elements();
    if (const auto f = opts.find("file"); f != opts.end()) {   // AC-50: bytes into the file
        std::filesystem::path p = pathOf(f->second);
        if (p.is_relative() && !c.host.attached()) p = pathOf(c.host.setup().traceFolder()) / p;   // the run's folder (LIB-11 AC-8)
        std::ofstream out(p, std::ios::binary | std::ios::trunc);
        const int perCell = std::max(1, (bits + 7) / 8);
        for (const auto& cell : cells) {
            const std::uint64_t x = u64(cell.find("value"));   // an unreadable cell as 0
            for (int i = 0; i < perCell; ++i) out.put(static_cast<char>((x >> (8 * i)) & 0xff));   // little-endian
        }
        out.close();
        if (!out) return refusedReply("Cannot write " + f->second + ".");
        Value res = Value::object();
        res.set("count", static_cast<long long>(cells.size()));
        return doneReply(std::move(res), {std::to_string(cells.size()) + " cells written to " + f->second});
    }
    std::vector<std::string> lines;
    Value arr = Value::array();
    for (const auto& cell : cells) {
        const std::string a = hexOf(u64(cell.find("address")), abits);
        std::string val;
        if (cell.find("unreadable")) val = "unreadable";
        else if (cell.find("none")) val = "none";
        else val = hexOf(u64(cell.find("value")), bits);
        lines.push_back(a + ": " + val);
        Value e = Value::object();
        e.set("address", a);
        e.set("value", val);
        arr.push(std::move(e));
    }
    return doneReply(std::move(arr), std::move(lines));
}

Reply pokeCmd(CommandContext& c) {
    if (!hasDebugger(c)) return noTarget();
    auto w = words(c.line.argText);
    const auto opts = takeOptions(w);
    if (w.size() < 4) return needs("poke <target> <space> <address> <value>… [bank=<b>] [vbank=<v>]");
    sessions::Value::Array values;
    for (std::size_t i = 3; i < w.size(); ++i) values.push_back(numberOrName(w[i]));
    sessions::Value::Object o{{"target", w[0]}, {"space", w[1]}, {"address", numberOrName(w[2])}, {"values", sessions::Value(std::move(values))}};
    addBank(o, opts);
    return fromCommand(send(c, dbg::kPokeCommand, sessions::Value(std::move(o))));
}

// ── Breakpoints, watches ─────────────────────────────────────────────────

Reply listReply(const ReplyMessage& r, bool watches) {
    if (!r.done) return refusedReply(r.reason);
    const sessions::Value v = valueOf(r);
    std::vector<std::string> lines;
    for (const auto& e : *v.find("entries")->elements()) {
        std::string l = std::to_string(u64(e.find("id"))) + " " + text(e.find("setter")) + " " + text(e.find("target"));
        if (watches)
            l += " space " + std::to_string(u64(e.find("space"))) + " " + hexOf(u64(e.find("address")), 16) + "+" +
                 std::to_string(u64(e.find("count"))) + " " + text(e.find("access"));
        else
            l += " " + hexOf(u64(e.find("address")), 16);
        lines.push_back(l);
    }
    return doneReply(fromSession(v), std::move(lines));
}

Reply breakCmd(CommandContext& c) {
    if (!hasDebugger(c)) return noTarget();
    auto w = words(c.line.argText);
    if (w.size() == 1 && w[0] == "list") return listReply(ask(c, dbg::kBreakpointsRequest, sessions::Value::object({{"setter", "harness"}})), false);
    if (!w.empty() && w[0] == "off") {
        if (w.size() == 1) return fromCommand(send(c, dbg::kBreakOffCommand, sessions::Value::object({{"setter", "harness"}})));
        if (w.size() != 3) return needs("break off [<target> <address>]");
        // The harness's own at that address of that target.
        const ReplyMessage l = ask(c, dbg::kBreakpointsRequest, sessions::Value::object({{"setter", "harness"}}));
        const auto view = targetView(c, w[1]);
        if (!view) return refusedReply("The machine has no debug target " + w[1] + ".");
        const auto n = formats::parseNumber(w[2]);
        std::optional<std::uint64_t> at = n ? std::optional<std::uint64_t>(static_cast<std::uint64_t>(*n)) : std::nullopt;
        if (!at) {   // a symbol: as the session reads it
            const ReplyMessage d = ask(c, dbg::kDisassembleRequest,
                                       sessions::Value::object({{"target", view->name}, {"address", w[2]}, {"count", 1}}));
            if (!d.done) return refusedReply(d.reason);
            at = u64((*valueOf(d).find("instructions")->elements())[0].find("address"));
        }
        const sessions::Value list = valueOf(l);
        for (const auto& e : *list.find("entries")->elements())
            if (text(e.find("target")) == view->name && u64(e.find("address")) == *at)
                send(c, dbg::kBreakOffCommand, sessions::Value::object({{"setter", "harness"}, {"id", e.find("id")->integer() ? *e.find("id")->integer() : 0}}));
        return doneReply();
    }
    if (w.size() < 2) return needs("break <target> <address> [<field>=<value>…] | break off [<target> <address>] | break list");
    sessions::Value::Object q;
    for (std::size_t i = 2; i < w.size(); ++i) {
        const auto eq = w[i].find('=');
        if (eq == std::string::npos) return refusedReply("A qualifier is written as name=value.");
        q.push_back({w[i].substr(0, eq), w[i].substr(eq + 1)});
    }
    sessions::Value::Object o{{"setter", "harness"}, {"target", w[0]}, {"address", numberOrName(w[1])}};
    if (!q.empty()) o.push_back({"qualifier", sessions::Value(std::move(q))});
    const ReplyMessage r = send(c, dbg::kBreakCommand, sessions::Value(std::move(o)));
    if (!r.done) return refusedReply(r.reason);
    const std::uint64_t id = u64(valueOf(r).find("id"));
    Value res = Value::object();
    res.set("id", static_cast<long long>(id));
    return doneReply(std::move(res), {"breakpoint " + std::to_string(id)});
}

Reply watchCmd(CommandContext& c) {
    if (!hasDebugger(c)) return noTarget();
    const auto w = words(c.line.argText);
    if (w.size() == 1 && w[0] == "list") return listReply(ask(c, dbg::kWatchesRequest, sessions::Value::object({{"setter", "harness"}})), true);
    if (w.size() == 1 && w[0] == "off") return fromCommand(send(c, dbg::kWatchOffCommand, sessions::Value::object({{"setter", "harness"}})));
    if (w.size() != 5) return needs("watch <target> <space> <address> <count> read|write|both | watch off | watch list");
    const ReplyMessage r = send(c, dbg::kWatchCommand,
                                sessions::Value::object({{"setter", "harness"}, {"target", w[0]}, {"space", w[1]},
                                                         {"address", numberOrName(w[2])}, {"count", numberOrName(w[3])},
                                                         {"access", w[4]}}));
    if (!r.done) return refusedReply(r.reason);
    const std::uint64_t id = u64(valueOf(r).find("id"));
    Value res = Value::object();
    res.set("id", static_cast<long long>(id));
    return doneReply(std::move(res), {"watch " + std::to_string(id)});
}

// ── History, disassemble, profile, symbols ───────────────────────────────

Reply historyCmd(CommandContext& c) {
    if (!hasDebugger(c)) return noTarget();
    const auto w = words(c.line.argText);
    sessions::Value::Object o;
    for (const auto& x : w) {
        if (auto n = formats::parseNumber(x)) o.push_back({"count", *n});
        else o.push_back({"target", x});
    }
    const ReplyMessage r = ask(c, dbg::kHistoryRequest, sessions::Value(std::move(o)));
    if (!r.done) return refusedReply(r.reason);
    const sessions::Value v = valueOf(r);
    const auto view = targetView(c, text(v.find("target")));
    const int abits = spaceBits(view, view->codeSpace);
    std::vector<std::string> lines;
    for (const auto& e : *v.find("entries")->elements()) {
        std::string l = hexOf(u64(e.find("address")), abits);
        if (const auto* s = e.find("symbol")) l += " <" + text(s) + ">";
        if (const auto* t = e.find("text")) l += "  " + text(t);
        const auto& regs = *e.find("registers")->elements();
        for (std::size_t i = 0; view && i < regs.size() && i < view->registers.size(); ++i)
            l += (i == 0 ? "  " : " ") + view->registers[i].first + "=" + hexOf(u64(&regs[i]), view->registers[i].second);
        lines.push_back(l);
    }
    return doneReply(fromSession(v), std::move(lines));
}

Reply disassembleCmd(CommandContext& c) {
    if (!hasDebugger(c)) return noTarget();
    const auto w = words(c.line.argText);
    if (w.size() < 2 || w.size() > 3) return needs("disassemble <target> <address> [<count>]");
    sessions::Value::Object o{{"target", w[0]}, {"address", numberOrName(w[1])}};
    if (w.size() == 3) o.push_back({"count", numberOrName(w[2])});
    const ReplyMessage r = ask(c, dbg::kDisassembleRequest, sessions::Value(std::move(o)));
    if (!r.done) return refusedReply(r.reason);
    const sessions::Value v = valueOf(r);
    const auto view = targetView(c, w[0]);
    const int abits = spaceBits(view, view->codeSpace);
    std::vector<std::string> lines;
    for (const auto& e : *v.find("instructions")->elements()) {
        std::string l = hexOf(u64(e.find("address")), abits);
        if (const auto* s = e.find("symbol")) l += " <" + text(s) + ">";
        lines.push_back(l + "  " + text(e.find("text")));
    }
    return doneReply(fromSession(v), std::move(lines));
}

Reply profileCmd(CommandContext& c) {
    if (!hasDebugger(c)) return noTarget();
    const auto w = words(c.line.argText);
    if (w.size() == 1 && (w[0] == "on" || w[0] == "off"))
        return fromCommand(send(c, dbg::kProfileCommand, sessions::Value::object({{"on", w[0] == "on"}})));
    if (w.empty() || w[0] != "report" || w.size() > 2) return needs("profile on|off|report [<count>]");
    sessions::Value::Object o;
    if (w.size() == 2) o.push_back({"count", numberOrName(w[1])});
    const ReplyMessage r = ask(c, dbg::kProfileReportRequest, sessions::Value(std::move(o)));
    if (!r.done) return refusedReply(r.reason);
    const sessions::Value v = valueOf(r);
    std::vector<std::string> lines;
    for (const auto& e : *v.find("entries")->elements()) {
        const auto view = targetView(c, text(e.find("target")));
        const int abits = spaceBits(view, view->codeSpace);
        std::string l = text(e.find("target")) + " " + hexOf(u64(e.find("address")), abits);
        if (const auto* s = e.find("symbol")) l += " <" + text(s) + ">";
        lines.push_back(l + " " + std::to_string(u64(e.find("count"))));
    }
    lines.push_back(std::to_string(u64(v.find("distinct"))) + " different addresses");
    return doneReply(fromSession(v), std::move(lines));
}

Reply symbolsCmd(CommandContext& c) {
    if (!hasDebugger(c)) return noTarget();
    const auto w = words(c.line.argText);
    if (w.empty() || w.size() > 2) return needs("symbols <file> [<target>]");
    sessions::Value::Object o{{"file", w[0]}};
    if (w.size() == 2) o.push_back({"target", w[1]});
    const ReplyMessage r = send(c, dbg::kSymbolsCommand, sessions::Value(std::move(o)));
    if (!r.done) return refusedReply(r.reason);
    const std::uint64_t n = u64(valueOf(r).find("count"));
    Value res = Value::object();
    res.set("count", static_cast<long long>(n));
    return doneReply(std::move(res), {std::to_string(n) + " symbols"});
}

// ── expect-register, expect-memory (AC-50) ───────────────────────────────

Reply expectRegisterCmd(CommandContext& c) {
    if (!hasDebugger(c)) return noTarget();
    const auto w = words(c.line.argText);
    if (w.size() != 3) return needs("expect-register <target> <register> <value>");
    const auto want = formats::parseNumber(w[2]);
    if (!want) return refusedReply("expect-register needs a number.");
    const ReplyMessage r = ask(c, dbg::kRegistersRequest, sessions::Value::object({{"target", w[0]}}));
    if (!r.done) return refusedReply(r.reason);
    std::string lw = w[1];
    std::transform(lw.begin(), lw.end(), lw.begin(), ::tolower);
    const sessions::Value regs = valueOf(r);
    for (const auto& reg : *regs.find("registers")->elements()) {
        std::string n = text(reg.find("name"));
        std::transform(n.begin(), n.end(), n.begin(), ::tolower);
        if (n != lw) continue;
        const std::uint64_t got = u64(reg.find("value"));
        if (got == static_cast<std::uint64_t>(*want)) return doneReply();
        return refusedReply("The register " + text(reg.find("name")) + " is " +
                            hexOf(got, static_cast<int>(u64(reg.find("bits")))) + ".");
    }
    return refusedReply("The target " + w[0] + " has no register " + w[1] + ".");
}

Reply expectMemoryCmd(CommandContext& c) {
    if (!hasDebugger(c)) return noTarget();
    auto w = words(c.line.argText);
    const auto opts = takeOptions(w);
    if (w.size() < 4) return needs("expect-memory <target> <space> <address> <value>… [bank=<b>] [vbank=<v>]");
    std::vector<std::uint64_t> want;
    for (std::size_t i = 3; i < w.size(); ++i) {
        const auto n = formats::parseNumber(w[i]);
        if (!n) return refusedReply("expect-memory needs numbers.");
        want.push_back(static_cast<std::uint64_t>(*n));
    }
    sessions::Value::Object o{{"target", w[0]}, {"space", w[1]}, {"address", numberOrName(w[2])},
                              {"count", static_cast<std::int64_t>(want.size())}};
    addBank(o, opts);
    const ReplyMessage r = ask(c, dbg::kPeekRequest, sessions::Value(std::move(o)));
    if (!r.done) return refusedReply(r.reason);
    const sessions::Value v = valueOf(r);
    const int bits = static_cast<int>(u64(v.find("cellBits")));
    const auto view = targetView(c, w[0]);
    const int abits = spaceBits(view, text(v.find("space")));
    const auto& cells = *v.find("cells")->elements();
    for (std::size_t i = 0; i < cells.size() && i < want.size(); ++i) {
        const auto& cell = cells[i];
        const std::string at = hexOf(u64(cell.find("address")), abits);
        if (cell.find("unreadable")) return refusedReply("The cell " + at + " of " + text(v.find("space")) + " is unreadable.");
        if (cell.find("none")) return refusedReply("The cell " + at + " of " + text(v.find("space")) + " holds none.");
        if (u64(cell.find("value")) != want[i])
            return refusedReply("The cell " + at + " of " + text(v.find("space")) + " is " + hexOf(u64(cell.find("value")), bits) + ".");
    }
    return doneReply();
}

} // namespace

void addDebugCommands(CommandRegistry& r) {
    r.add("pause", pauseCmd, "pause [<level>]          pause the machine (cpu without a level)");
    r.add("resume", resumeCmd, "resume                   run the paused machine again");
    r.add("step", stepCmd, "step [{ level: program|cpu, count: n }]  carry out steps of a paused machine");
    r.add("step-over", overOut("over"), "step-over [<target>]     an instruction, a call with its routine");
    r.add("step-out", overOut("out"), "step-out [<target>]      until the routine has returned");
    r.add("pause-on-start", pauseOnStartCmd, "pause-on-start <level>|off  pause at the next start of the level");
    r.add("pause-at-start", pauseAtStartCmd, "pause-at-start           pause before the first instruction after the next start");
    r.add("trace", traceCmd, "trace <file>|off         record every instruction into a trace file");
    r.add("targets", targetsCmd, "targets                  the debug targets");
    r.add("registers", registersCmd, "registers [<target>]     every register with its value");
    r.add("set-register", setRegisterCmd, "set-register <target> <register> <value>");
    r.add("peek", peekCmd, "peek <target> <space> <address> [<count>] [bank=<b>] [vbank=<v>] [file=<file>]");
    r.add("poke", pokeCmd, "poke <target> <space> <address> <value>… [bank=<b>] [vbank=<v>]");
    r.add("break", breakCmd, "break <target> <address> [<field>=<value>…] | off [<target> <address>] | list");
    r.add("watch", watchCmd, "watch <target> <space> <address> <count> read|write|both | off | list");
    r.add("history", historyCmd, "history [<target>] [<count>]  the last instructions");
    r.add("disassemble", disassembleCmd, "disassemble <target> <address> [<count>]");
    r.add("profile", profileCmd, "profile on|off|report [<count>]");
    r.add("expect-register", expectRegisterCmd, "expect-register <target> <register> <value>");
    r.add("expect-memory", expectMemoryCmd, "expect-memory <target> <space> <address> <value>… [bank=<b>] [vbank=<v>]");
    r.add("symbols", symbolsCmd, "symbols <file> [<target>]  names for the target's addresses");
}

} // namespace calcu::harness
