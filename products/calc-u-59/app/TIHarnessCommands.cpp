#include "TIHarnessCommands.hpp"
#include "Bcd.hpp"
#include "KeyIds.hpp"
#include "ModuleImage.hpp"
#include "TIDebugPart.hpp"
#include "TIMnemonics.hpp"
#include "TIPresetCommands.hpp"
#include "TIPresetKeys.hpp"
#include "TIRunProbe.hpp"
#include "../core/PC100C.hpp"
#include "../core/TITypingSupport.hpp"
#include "debugger/DebuggerPart.hpp"
#include "files/Files.hpp"
#include "formats/Duration.hpp"
#include "harness/Commands.hpp"
#include "harness/HarnessHost.hpp"
#include "harness/Product.hpp"
#include "machine/Idle.hpp"
#include "paper/PaperTopics.hpp"
#include "parts/LibraryParts.hpp"
#include "sessions/LibraryTopics.hpp"
#include "typing/TypingTopics.hpp"
#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <map>
#include <optional>
#include <sstream>

namespace cu59 {

namespace {

using namespace calcu;
using harness::CommandContext;
using harness::Reply;
using harness::doneReply;
using harness::refusedReply;
using machine::Ticks;

using harness::words;

harness::Reply linesReply(const sessions::ReplyMessage& r) {
    if (!r.done) return refusedReply(r.reason);
    std::vector<std::string> out;
    const sessions::Value result = r.result ? r.result->toValue() : sessions::Value();
    if (const auto* a = result.elements())
        for (const auto& e : *a)
            if (e.string()) out.push_back(*e.string());
    if (out.empty()) return doneReply();
    harness::Value v = harness::Value::array();
    for (const auto& l : out) v.push(l);
    return doneReply(std::move(v), out);
}

// ── `c59 partition|program|registers`, sent as
// the product commands of ProgramMemoryPart.

void addProgramMemoryCommands(harness::CommandRegistry& r) {
    r.addProduct("c59", "partition", [](CommandContext& c) {
        const auto w = words(c.line.argText);
        if (w.size() > 1) return refusedReply("usage: c59 partition [<value>]");
        sessions::Value args = w.empty() ? sessions::Value::object({}) : sessions::Value::object({{"value", w[0]}});
        return linesReply(c.host.command(sessions::Command{commands::kPartition, std::move(args)}));
    }, "partition [<value>]  set the partition (479, 479.59) or read it");
    r.addProduct("c59", "program", [](CommandContext& c) {
        const auto w = words(c.line.argText);
        if (w.empty()) return refusedReply("usage: c59 program [<step>] <keycode>... | c59 program <step> count=<n>");
        sessions::Value::Object o;
        std::size_t i = 0;
        if (w[0].size() >= 3 && w[0].find('=') == std::string::npos) {
            o.push_back({"step", w[0]});
            i = 1;
        }
        if (i < w.size() && w[i].rfind("count=", 0) == 0) {
            if (i != 1 || w.size() != 2) return refusedReply("usage: c59 program <step> count=<n>");
            const std::string n = w[i].substr(6);
            bool digits = !n.empty() && n.size() < 5;
            for (char ch : n) digits = digits && ch >= '0' && ch <= '9';
            if (!digits) return refusedReply("count= is a number of steps, at least 1.");
            o.push_back({"count", static_cast<std::int64_t>(std::stoi(n))});
        } else {
            std::string codes;
            for (; i < w.size(); ++i) codes += (codes.empty() ? "" : " ") + w[i];
            if (codes.empty()) return refusedReply("usage: c59 program [<step>] <keycode>...");
            o.push_back({"keycodes", codes});
        }
        return linesReply(c.host.command(sessions::Command{commands::kProgram, sessions::Value(std::move(o))}));
    }, "program [<step>] <keycode>... | program <step> count=<n>  write keycodes from a step, or read n");
    r.addProduct("c59", "data-registers", [](CommandContext& c) {
        const auto w = words(c.line.argText);
        if (w.empty()) return refusedReply("usage: c59 data-registers <NN>=<value>... | c59 data-registers <NN>...");
        const bool set = w[0].find('=') != std::string::npos;
        sessions::Value::Array a;
        for (const std::string& x : w) {
            const std::size_t eq = x.find('=');
            if ((eq != std::string::npos) != set) return refusedReply("Either write registers (NN=value) or read them (NN), not both.");
            if (set) a.emplace_back(sessions::Value::Array{sessions::Value(x.substr(0, eq)), sessions::Value(x.substr(eq + 1))});
            else a.emplace_back(x);
        }
        sessions::Value args = sessions::Value::object({{set ? "set" : "get", sessions::Value(std::move(a))}});
        return linesReply(c.host.command(sessions::Command{commands::kDataRegisters, std::move(args)}));
    }, "data-registers <NN>=<value>... | data-registers <NN>...  write data registers (H00-H03 on the TI-58C) or read them");
}

// ── The debug panel's commands; no preset steps.

void addDebugPanelCommands(harness::CommandRegistry& r) {
    r.addProduct("c59", "asm", [](CommandContext& c) -> Reply {
        const auto w = words(c.line.argText);
        if (w.empty()) return refusedReply("usage: c59 asm load <file> | run | clear");
        if (w[0] == "load") {
            std::string path = c.line.argText.substr(4);   // after "load"
            path.erase(0, std::min(path.size(), path.find_first_not_of(" \t")));
            if (path.empty()) return refusedReply("usage: c59 asm load <file>");
            const std::optional<std::string> text = calcu::files::readText(calcu::files::toPath(path));
            sessions::Value::Object o{{"name", sessions::Value(std::filesystem::path(path).filename().string())}};
            if (text) o.push_back({"text", sessions::Value(*text)});
            const sessions::ReplyMessage m = c.host.command(sessions::Command{commands::kAsmLoad, sessions::Value(std::move(o))});
            if (!m.done) return refusedReply(m.reason);
            const sessions::Value v = m.result ? m.result->toValue() : sessions::Value();
            const std::string n = std::to_string(v.integer() ? *v.integer() : 0);
            return doneReply(harness::Value(n + " instructions"), {n + " instructions"});
        }
        if (w[0] == "run") {
            const sessions::ReplyMessage m = c.host.command(sessions::Command{commands::kAsmRun, sessions::Value::object({})});
            if (!m.done) return refusedReply(m.reason);
            const auto v = m.result ? m.result->toValue() : sessions::Value();
            const std::string status = v.string() ? *v.string() : std::string();
            return doneReply(harness::Value(status), {status});
        }
        if (w[0] == "clear") return linesReply(c.host.command(sessions::Command{commands::kAsmClear, sessions::Value::object({})}));
        return refusedReply("usage: c59 asm load <file> | run | clear");
    }, "asm load <file> | run | clear  the ASM overlay: put over the ROM, enter at 1800, take away");
    r.addProduct("c59", "dump", [](CommandContext& c) {
        const auto w = words(c.line.argText);
        if (w.size() != 1) return refusedReply("usage: c59 dump scom|memory|prog|vars");
        return linesReply(c.host.command(sessions::Command{commands::kDump, sessions::Value::object({{"what", w[0]}})}));
    }, "dump scom|memory|prog|vars  the lines of a dump (also into the log)");
    r.addProduct("c59", "log-level", [](CommandContext& c) {
        const auto w = words(c.line.argText);
        if (w.size() != 1) return refusedReply("usage: c59 log-level off|info|debug");
        return linesReply(c.host.command(sessions::Command{commands::kLogLevel, sessions::Value::object({{"level", w[0]}})}));
    }, "log-level off|info|debug  the level of the machine's own log lines");
    r.addProduct("c59", "heatmap", [](CommandContext& c) {
        const auto w = words(c.line.argText);
        if (w.size() != 1 || w[0] != "reset") return refusedReply("usage: c59 heatmap reset");
        return linesReply(c.host.command(sessions::Command{commands::kHeatmapReset, sessions::Value::object({})}));
    }, "heatmap reset  set the ROM heatmap's counts to zero");
}

// ── Coverage, timing and keycode functions ──────────────────────────────────

constexpr const char* kNoMachine = "There is no machine.";
constexpr Ticks kInstruction = 32;   // ticks of one instruction outside the idle state

machine::Machine* machineOf(CommandContext& c) { return c.host.session().machineSession().machine(); }

std::uint32_t rateOf(machine::Machine& m) { return m.descriptor().tickRate; }

std::string secondsText(Ticks t, std::uint32_t rate) { return harness::toText(harness::secondsOf(t, rate)); }

// A share of `n` notes in per mille, written as a percent with one decimal.
std::string share(std::uint64_t count, std::uint64_t n) {
    const std::uint64_t pm = n ? count * 1000 / n : 0;
    return std::to_string(pm / 10) + "." + std::to_string(pm % 10) + "%";
}

std::string three(int v) {
    char b[8];
    std::snprintf(b, sizeof b, "%03d", v);
    return b;
}

// `limit=<duration>` among the words, in ticks; the default without it.
std::optional<Ticks> limitOf(const std::vector<std::string>& w, double defaultSeconds, std::uint32_t rate,
                             std::string* error) {
    Ticks t = static_cast<Ticks>(defaultSeconds * rate);
    for (const auto& x : w) {
        if (x.rfind("limit=", 0) != 0) {
            *error = "usage: limit=<duration>";
            return std::nullopt;
        }
        const auto us = formats::parseDuration(x.substr(6), error);
        if (!us) return std::nullopt;
        t = harness::ticksFor(*us, rate);
    }
    return t;
}

bool hasPrinter(machine::Machine& m) {
    for (const auto& p : m.plugged().byPlace)
        if (p.peripheral.value == "PC-100C") return true;
    return false;
}

std::vector<std::string> paperLines(CommandContext& c) {
    const sessions::ReplyMessage r = c.host.ask(sessions::Request{paper::kPaperTextRequest, sessions::Value::object({})});
    std::vector<std::string> out;
    if (!r.done) return out;
    const sessions::Value v = r.result ? r.result->toValue() : sessions::Value();
    if (const sessions::Value* l = v.find("lines"); l && l->elements())
        for (const auto& e : *l->elements())
            if (e.string()) out.push_back(*e.string());
    return out;
}

// Advances `ticks` on the machine session (its slice hooks and the run
// control's gate, as every advance), without the app session's per-call
// duties: a command runs thousands of slices, and the parts follow once at
// its end (settle). False: the machine went or did not run (paused).
bool run(CommandContext& c, Ticks ticks) {
    return machineOf(c) != nullptr && c.host.session().machineSession().advanceUnpaced(ticks) > 0;
}

// The parts of the session follow what the slices did (the display read
// over the command's time, the paper, the topics), as after a `wait`: one
// instruction more through the app session.
Reply settled(CommandContext& c, Reply r) {
    c.host.advance(kInstruction);
    return r;
}

// ── `c59 coverage [all]` ──

Reply coverageCmd(CommandContext& c) {
    const auto w = words(c.line.argText);
    if (w.size() > 1 || (w.size() == 1 && w[0] != "all")) return refusedReply("usage: c59 coverage [all]");
    machine::Machine* m = machineOf(c);
    if (!m) return refusedReply(kNoMachine);
    debugger::DebuggerPart* d = &c.host.product().libraryParts().debugger();
    if (!d || !d->runControl().profile().on()) return refusedReply("Profile is off: send `profile on` first.");
    const debugger::Profile& p = d->runControl().profile();
    machine::Debugging* dbg = m->debugging();
    const auto range = idleAndKeyDetectRange(m->descriptor().model);
    std::vector<std::uint64_t> addresses;
    for (const auto& e : p.top(p.distinct())) {
        if (!dbg || e.target >= dbg->targetCount() || dbg->target(e.target).info().name.value != "TMC0501") continue;
        if (e.address > 0x17FF) continue;
        const bool keyReady = (e.address >= range[0] && e.address <= range[1]) || (e.address >= range[2] && e.address <= range[3]);
        if (keyReady && w.empty()) continue;
        addresses.push_back(e.address);
    }
    std::sort(addresses.begin(), addresses.end());
    std::vector<std::string> lines;
    harness::Value list = harness::Value::array();
    std::string line;
    for (std::size_t i = 0; i < addresses.size(); ++i) {
        char b[8];
        std::snprintf(b, sizeof b, "%04llX", static_cast<unsigned long long>(addresses[i]));
        list.push(std::string(b));
        line += (line.empty() ? "" : " ") + std::string(b);
        if (i % 16 == 15) {
            lines.push_back(line);
            line.clear();
        }
    }
    if (!line.empty()) lines.push_back(line);
    lines.push_back("reached: " + std::to_string(addresses.size()) + " of 6144");
    harness::Value v = harness::Value::object();
    v.set("addresses", std::move(list));
    v.set("reached", static_cast<std::int64_t>(addresses.size()));
    v.set("of", 6144);
    return doneReply(std::move(v), std::move(lines));
}

// ── `c59 line-times [limit=<d>]` ──

struct GapSummary {
    std::uint64_t n = 0;
    Ticks sum = 0, least = 0, most = 0;
    void add(Ticks g) {
        least = n ? std::min(least, g) : g;
        most = n ? std::max(most, g) : g;
        sum += g;
        ++n;
    }
};

Reply lineTimesCmd(CommandContext& c) {
    machine::Machine* m = machineOf(c);
    if (!m) return refusedReply(kNoMachine);
    if (!hasPrinter(*m)) return refusedReply("There is no PC-100C: attach printer PC-100C first.");
    const std::uint32_t rate = rateOf(*m);
    std::string error;
    const auto limit = limitOf(words(c.line.argText), 4000, rate, &error);
    if (!limit) return refusedReply(error);
    const Ticks slice = 200 * kInstruction;   // a printed line takes far longer: a line's time is exact to 200 instructions
    const Ticks quietNeeded = static_cast<Ticks>(15) * rate;
    const Ticks t0 = m->tickCount();
    std::size_t seen = paperLines(c).size();
    struct Printed {
        Ticks at, gap;
        std::string text;
    };
    std::vector<Printed> printed;
    Ticks quiet = 0, last = 0;
    bool limited = false;
    while (true) {
        if (!run(c, slice)) break;
        m = machineOf(c);
        const Ticks now = m->tickCount() - t0;
        const auto paper = paperLines(c);
        bool any = false;
        for (std::size_t i = seen; i < paper.size(); ++i) {
            printed.push_back({now, now - last, paper[i]});
            last = now;
            any = true;
        }
        seen = paper.size();
        quiet = (!any && calcu::machine::idleState(*m).ended) ? quiet + slice : 0;
        if (quiet >= quietNeeded) break;
        if (now >= *limit) {
            limited = true;
            break;
        }
    }
    std::vector<std::string> lines;
    harness::Value list = harness::Value::array();
    GapSummary feeds, other;
    for (const auto& p : printed) {
        const bool feed = p.text.find_first_not_of(' ') == std::string::npos;
        (feed ? feeds : other).add(p.gap);
        lines.push_back(secondsText(p.at, rate) + " " + secondsText(p.gap, rate) + " |" + p.text + "|");
        harness::Value e = harness::Value::object();
        e.set("time", harness::secondsOf(p.at, rate));
        e.set("gap", harness::secondsOf(p.gap, rate));
        e.set("text", p.text);
        list.push(std::move(e));
    }
    if (printed.empty()) lines.push_back("no lines printed");
    auto summary = [&](const char* name, const GapSummary& g, harness::Value& v) {
        v = harness::Value::object();
        v.set("n", static_cast<std::int64_t>(g.n));
        if (g.n) {
            v.set("mean", harness::secondsOf(g.sum / g.n, rate));
            v.set("min", harness::secondsOf(g.least, rate));
            v.set("max", harness::secondsOf(g.most, rate));
            v.set("sum", harness::secondsOf(g.sum, rate));
            lines.push_back(std::string(name) + ": n=" + std::to_string(g.n) + " mean " + secondsText(g.sum / g.n, rate) +
                            " min " + secondsText(g.least, rate) + " max " + secondsText(g.most, rate) + " sum " +
                            secondsText(g.sum, rate));
        } else {
            lines.push_back(std::string(name) + ": n=0");
        }
    };
    harness::Value fv, ov;
    summary("feeds", feeds, fv);
    summary("lines", other, ov);
    // The least time the printer needs, from measurements on a PC-100A.
    const Ticks floor = (other.n * PC100C::kLineInstructions + feeds.n * PC100C::kFeedInstructions) * kInstruction;
    lines.push_back("printer floor: " + secondsText(floor, rate));
    lines.push_back(std::string("limit: ") + (limited ? "yes" : "no"));
    harness::Value v = harness::Value::object();
    v.set("lines", std::move(list));
    v.set("feeds", std::move(fv));
    v.set("other", std::move(ov));
    v.set("floor", harness::secondsOf(floor, rate));
    v.set("limited", limited);
    return settled(c, doneReply(std::move(v), std::move(lines)));
}

// ── `c59 run-time <key>...` ──

Reply runTimeCmd(CommandContext& c) {
    const auto keys = words(c.line.argText);
    if (keys.empty()) return refusedReply("usage: c59 run-time <key> [<key> ...]");
    machine::Machine* m = machineOf(c);
    if (!m) return refusedReply(kNoMachine);
    const std::uint32_t rate = rateOf(*m);
    const Ticks slice = 100 * kInstruction;   // fine enough for a run's end, coarse enough to be quick
    const Ticks limit = static_cast<Ticks>(4000) * rate;
    const Ticks settleTicks = rate / 4;          // stopped for a quarter second, not a short gap
    std::vector<std::string> lines;
    harness::Value list = harness::Value::array();
    Ticks total = 0;
    for (const auto& key : keys) {
        m = machineOf(c);
        if (!m) return refusedReply(kNoMachine);
        const Ticks t0 = m->tickCount();
        sessions::Value a = sessions::Value::object({{"names", sessions::Value(sessions::Value::Array{sessions::Value(key)})}});
        const sessions::ReplyMessage r = c.host.command(sessions::Command{typing::kKey, std::move(a)}, "The keys were not taken within an hour.");
        if (!r.done) return refusedReply(r.reason);
        bool wentDark = false, ran = false, stopped = false, finished = false;
        std::optional<Ticks> onAt = t0;
        Ticks quiet = 0, time = 0;
        while ((m = machineOf(c)) && m->tickCount() - t0 < limit) {
            const Ticks t = m->tickCount();
            const RunNote n = noteRun(*m);
            const bool lit = n.displayOn;
            ran = ran || n.running;
            stopped = stopped && !n.running;
            if (!lit && !stopped) {
                wentDark = true;
                onAt.reset();
                quiet = 0;
            } else {
                if (!onAt) onAt = t;
                stopped = stopped || !n.running;
                quiet = stopped ? quiet + slice : 0;
                if (quiet >= settleTicks) {
                    finished = true;
                    time = wentDark ? *onAt - t0 : 0;
                    break;
                }
            }
            if (!run(c, slice)) break;
        }
        if (!finished) time = (m ? m->tickCount() : t0) - t0;
        if (!ran) time = finished ? 0 : time;
        total += time;
        lines.push_back(key + " " + secondsText(time, rate) + (finished ? "" : " not finished"));
        harness::Value e = harness::Value::object();
        e.set("key", key);
        e.set("seconds", harness::secondsOf(time, rate));
        e.set("finished", finished);
        list.push(std::move(e));
        if (!finished) break;   // the command ends there, done
    }
    m = machineOf(c);
    lines.push_back("total " + secondsText(total, m ? rateOf(*m) : rate));
    harness::Value v = harness::Value::object();
    v.set("keys", std::move(list));
    v.set("total", harness::secondsOf(total, rate));
    return settled(c, doneReply(std::move(v), std::move(lines)));
}

// ── `c59 hot-steps [limit=<d>]` ──

Reply hotStepsCmd(CommandContext& c) {
    machine::Machine* m = machineOf(c);
    if (!m) return refusedReply(kNoMachine);
    const std::uint32_t rate = rateOf(*m);
    std::string error;
    const auto limit = limitOf(words(c.line.argText), 200, rate, &error);
    if (!limit) return refusedReply(error);
    const Ticks slice = 97 * kInstruction;
    const Ticks t0 = m->tickCount();
    std::uint64_t n = 0, busy = 0, wait = 0, memory = 0, fast = 0, rom = 0, module = 0;
    std::map<int, std::uint64_t> steps;
    while ((m = machineOf(c)) && m->tickCount() - t0 < *limit) {
        if (!run(c, slice)) break;
        m = machineOf(c);
        if (!m) break;
        const RunNote note = noteRun(*m);
        ++n;
        busy += note.printerBusy;
        wait += note.printWait;
        ++steps[note.step];
        switch (programSourceFrom(note.source, note.pc)) {
        case ProgramSource::UserProgram: ++memory; break;
        case ProgramSource::FastMode: ++memory; ++fast; break;
        case ProgramSource::Rom: ++rom; break;
        case ProgramSource::SolidState:
        case ProgramSource::SolidStateReturn: ++module; break;
        }
    }
    std::vector<std::pair<int, std::uint64_t>> top(steps.begin(), steps.end());
    std::stable_sort(top.begin(), top.end(), [](const auto& a, const auto& b) { return a.second > b.second; });
    if (top.size() > 25) top.resize(25);
    std::vector<std::string> lines{"notes: " + std::to_string(n), "printer busy " + share(busy, n),
                                   "printer wait " + share(wait, n),
                                   "memory " + std::to_string(memory) + " (fast mode " + std::to_string(fast) + ")",
                                   "rom " + std::to_string(rom), "module " + std::to_string(module)};
    harness::Value list = harness::Value::array();
    for (const auto& [step, count] : top) {
        lines.push_back(three(step) + ": " + share(count, n));
        harness::Value e = harness::Value::object();
        e.set("step", step);
        e.set("share", share(count, n));
        list.push(std::move(e));
    }
    harness::Value sources = harness::Value::object();
    sources.set("memory", static_cast<std::int64_t>(memory));
    sources.set("fastMode", static_cast<std::int64_t>(fast));
    sources.set("rom", static_cast<std::int64_t>(rom));
    sources.set("module", static_cast<std::int64_t>(module));
    harness::Value v = harness::Value::object();
    v.set("notes", static_cast<std::int64_t>(n));
    v.set("printerBusy", share(busy, n));
    v.set("printerWait", share(wait, n));
    v.set("sources", std::move(sources));
    v.set("steps", std::move(list));
    return settled(c, doneReply(std::move(v), std::move(lines)));
}

// ── `c59 step-trace <from> <to>` ──

Reply stepTraceCmd(CommandContext& c) {
    const auto w = words(c.line.argText);
    if (w.size() != 2) return refusedReply("usage: c59 step-trace <from> <to>");
    machine::Machine* m = machineOf(c);
    if (!m) return refusedReply(kNoMachine);
    const std::uint32_t rate = rateOf(*m);
    std::string error;
    const auto from = formats::parseDuration(w[0], &error);
    if (!from) return refusedReply(error);
    const auto to = formats::parseDuration(w[1], &error);
    if (!to) return refusedReply(error);
    if (*to <= *from) return refusedReply("The second time must come after the first.");
    const Ticks a = harness::ticksFor(*from, rate), b = harness::ticksFor(*to, rate);
    const Ticks t0 = m->tickCount();
    std::vector<std::pair<int, std::uint64_t>> seq;
    while ((m = machineOf(c)) && m->tickCount() - t0 < b) {
        if (!run(c, 20 * kInstruction)) break;
        m = machineOf(c);
        if (!m) break;
        const Ticks t = m->tickCount() - t0;
        if (t > b) break;
        if (t < a) continue;
        const int step = noteRun(*m).step;
        if (!seq.empty() && seq.back().first == step) ++seq.back().second;
        else seq.push_back({step, 1});
    }
    std::string line;
    harness::Value list = harness::Value::array();
    for (const auto& [step, count] : seq) {
        line += (line.empty() ? "" : " ") + three(step) + ":" + std::to_string(count);
        harness::Value e = harness::Value::object();
        e.set("step", step);
        e.set("count", static_cast<std::int64_t>(count));
        list.push(std::move(e));
    }
    harness::Value v = harness::Value::object();
    v.set("steps", std::move(list));
    return settled(c, doneReply(std::move(v), line.empty() ? std::vector<std::string>{} : std::vector<std::string>{line}));
}

// ── `c59 step-cost <keycode>...` ──

// The entry of the program to measure: the SST method of fast mode and
// GTO 008 R/S, as matrix codes (row × 10 + column).
constexpr std::uint8_t kFastEntry[] = {82, 85, 83, 52, 82, 83, 94, 95, 61, 92, 92, 62, 41, 92, 42};
constexpr std::uint8_t kNormalEntry[] = {61, 92, 92, 63, 91};
constexpr int kReps = 30;

struct CostRun {
    std::optional<Ticks> ticks;
    std::string refusal;
};

bool keyEdge(CommandContext& c, std::uint8_t code, bool down) {
    const auto id = keyIdAt(code);
    if (!id) return false;
    const sessions::ReplyMessage r = c.host.command(sessions::Command{
        down ? sessions::kKeyDownCommand : sessions::kKeyUpCommand, sessions::Value::object({{"key", id->value}})});
    return r.done;
}

CostRun measure(CommandContext& c, const std::vector<std::uint8_t>& body, int reps, bool fast) {
    machine::Machine* m = machineOf(c);
    ProgramMemory* pm = m ? programMemoryOf(*m) : nullptr;
    if (!pm) return {std::nullopt, kNoMachine};
    const std::uint32_t rate = rateOf(*m);
    std::vector<std::uint8_t> prog{91, 0, 0, 0, 0, 0, 0, 86};
    for (int r = 0; r < reps; ++r) prog.insert(prog.end(), body.begin(), body.end());
    prog.push_back(25);
    prog.push_back(81);
    if (static_cast<int>(prog.size()) - 1 > pm->lastStep()) return {std::nullopt, "The program does not fit into the memory."};
    for (std::size_t i = 0; i < prog.size(); ++i) pm->writeStep(static_cast<int>(i), prog[i]);
    // The registers the measured program starts with.
    const std::pair<int, const char*> regs[] = {{0, "10"}, {10, "2.000311000312"}, {11, "2.000311000312"},
                                                {12, "2.000311000312"}, {22, "98.99"}, {4, "1000"}, {1, "5"}};
    for (const auto& [n, value] : regs)
        if (n < pm->dataRegisterCount()) pm->writeRegister(n, *parseDecimal(value));
    const std::uint8_t* keys = fast ? kFastEntry : kNormalEntry;
    const std::size_t count = fast ? std::size(kFastEntry) : std::size(kNormalEntry);
    auto idleNow = [&](bool printerToo) {
        machine::Machine* mm = machineOf(c);
        if (!mm) return false;
        const RunNote n = noteRun(*mm);
        return n.cpuIdle && !(printerToo && n.printerBusy);
    };
    for (std::size_t k = 0; k + 1 < count; ++k) {   // press, hold, release, settle (ti59.py press, settle)
        if (!keyEdge(c, keys[k], true)) return {std::nullopt, "The entry keys were refused."};
        run(c, 6000 * kInstruction);
        keyEdge(c, keys[k], false);
        const Ticks s0 = machineOf(c)->tickCount();
        int quiet = 0;
        while (quiet < 3 && machineOf(c)->tickCount() - s0 < static_cast<Ticks>(60) * rate) {
            if (!run(c, 20000 * kInstruction)) break;
            quiet = idleNow(true) ? quiet + 1 : 0;
        }
    }
    keyEdge(c, keys[count - 1], true);
    run(c, 3000 * kInstruction);
    keyEdge(c, keys[count - 1], false);
    const Ticks t0 = machineOf(c)->tickCount();
    int quiet = 0;
    while (quiet < 20) {
        if (!run(c, 25 * kInstruction)) return {std::nullopt, "The machine stopped running."};
        quiet = idleNow(false) ? quiet + 1 : 0;
        if (machineOf(c)->tickCount() - t0 > static_cast<Ticks>(300) * rate)
            return {std::nullopt, "The program still runs after 300 s: a jump that loops back?"};
    }
    return {machineOf(c)->tickCount() - t0, {}};
}

Reply stepCostCmd(CommandContext& c) {
    const auto w = words(c.line.argText);
    if (w.empty()) return refusedReply("usage: c59 step-cost <keycode> [<keycode> ...] (e.g. c59 step-cost 43 22)");
    std::vector<std::uint8_t> body;
    for (const auto& x : w) {
        if (x.size() != 2 || x.find_first_not_of("0123456789") != std::string::npos)
            return refusedReply("A keycode is two digits, 00 to 99: " + x + ".");
        body.push_back(static_cast<std::uint8_t>(std::stoi(x)));
    }
    machine::Machine* m = machineOf(c);
    if (!m) return refusedReply(kNoMachine);
    const std::uint32_t rate = rateOf(*m);
    // The entry keys are pressed as on a calculator that waits for keys; pressed into a running program they measure nothing.
    if (noteRun(*m).running) return refusedReply("A program runs: let it end first (`wait`).");
    const bool printer = hasPrinter(*m);
    std::vector<std::string> lines;
    harness::Value v = harness::Value::object();
    for (const bool fast : {true, false}) {
        const CostRun with = measure(c, body, kReps, fast);
        if (!with.ticks) return refusedReply(with.refusal);
        const CostRun without = measure(c, {}, 1, fast);
        if (!without.ticks) return refusedReply(without.refusal);
        const std::int64_t diff = static_cast<std::int64_t>(*with.ticks) - static_cast<std::int64_t>(*without.ticks);
        const std::int64_t den = static_cast<std::int64_t>(kInstruction) * kReps;
        const std::int64_t instructions = (diff >= 0 ? diff + den / 2 : diff - den / 2) / den;
        const std::int64_t rden = static_cast<std::int64_t>(rate) * kReps;
        const std::int64_t ms = (diff * 1000 + (diff >= 0 ? rden / 2 : -rden / 2)) / rden;
        const char* name = fast ? "fast" : "normal";
        lines.push_back(std::string(name) + " " + std::to_string(instructions) + " instructions " + std::to_string(ms) + " ms");
        harness::Value e = harness::Value::object();
        e.set("instructions", instructions);
        e.set("ms", ms);
        v.set(name, std::move(e));
    }
    lines.push_back(std::string("printer: ") + (printer ? "yes" : "no"));
    v.set("printer", printer);
    return settled(c, doneReply(std::move(v), std::move(lines)));
}

// ── `c59 keycodes list|disassemble|assemble` ──

std::optional<int> stepWord(const std::string& w) {
    if (w.size() < 3 || w.size() > 4 || w.find_first_not_of("0123456789") != std::string::npos) return std::nullopt;
    return std::stoi(w);
}

Reply keycodesCmd(CommandContext& c) {
    static const char* kUsage = "usage: c59 keycodes list|disassemble [<first> [<last>]] | c59 keycodes assemble <file>";
    const auto w = words(c.line.argText);
    if (w.empty()) return refusedReply(kUsage);
    machine::Machine* m = machineOf(c);
    ProgramMemory* pm = m ? programMemoryOf(*m) : nullptr;
    if (!pm) return refusedReply(kNoMachine);
    if (w[0] == "assemble") {
        std::string path = c.line.argText.substr(c.line.argText.find("assemble") + 8);
        path.erase(0, std::min(path.size(), path.find_first_not_of(" \t")));
        while (!path.empty() && (path.back() == ' ' || path.back() == '\t')) path.pop_back();
        if (path.empty()) return refusedReply("usage: c59 keycodes assemble <file>");
        const std::optional<std::string> text = calcu::files::readText(calcu::files::toPath(path));
        if (!text) return refusedReply("The file " + path + " cannot be read.");
        auto r = assemble(*text, pm->lastStep());
        if (const auto* e = std::get_if<KeycodeError>(&r))
            return refusedReply("Line " + std::to_string(e->lineIndex + 1) + ": " + e->message);
        const auto& steps = std::get<std::vector<ProgramStep>>(r);
        if (steps.empty()) return refusedReply("The file holds no instruction.");
        int lo = steps.front().step, hi = lo;
        for (const auto& s : steps) {
            pm->writeStep(s.step, s.keycode);
            lo = std::min(lo, s.step);
            hi = std::max(hi, s.step);
        }
        const std::string l = "steps: " + three(lo) + "-" + three(hi);
        harness::Value v = harness::Value::object();
        v.set("first", lo);
        v.set("last", hi);
        return doneReply(std::move(v), {l});
    }
    if (w[0] != "list" && w[0] != "disassemble") return refusedReply(kUsage);
    if (w.size() > 3) return refusedReply(kUsage);
    const int partitionLast = pm->partitionSteps() - 1;
    std::vector<std::uint8_t> steps;
    for (int i = 0; i <= pm->lastStep(); ++i) steps.push_back(pm->readStep(i));
    int first = 0;
    std::optional<int> last;
    if (w.size() >= 2) {
        const auto f = stepWord(w[1]);
        if (!f || *f > pm->lastStep()) return refusedReply("A step is three digits, 000 to " + three(pm->lastStep()) + ": " + w[1] + ".");
        first = *f;
    }
    if (w.size() == 3) {
        const auto l = stepWord(w[2]);
        if (!l || *l > pm->lastStep() || *l < first)
            return refusedReply("The last step is three digits, from the first to " + three(pm->lastStep()) + ": " + w[2] + ".");
        last = *l;
    }
    if (!last) {   // the last step that is not 00, within the partition
        std::vector<std::uint8_t> program(steps.begin(), steps.begin() + std::max(0, std::min(partitionLast, pm->lastStep()) + 1));
        last = lastUsedStep(program, first);
    }
    const bool list = w[0] == "list";
    const auto out = list ? listing(steps, first, *last) : disassemble(steps, first, *last);
    std::vector<std::string> lines;
    harness::Value arr = harness::Value::array();
    for (const auto& l : out) {
        lines.push_back(l.text);
        harness::Value e = harness::Value::object();
        e.set("step", l.step);
        if (list) {
            e.set("keycode", l.text.substr(4, 2));
            e.set("mnemonic", l.text.substr(7));
        } else {
            e.set("text", l.text);
        }
        arr.push(std::move(e));
    }
    harness::Value v = harness::Value::object();
    v.set(list ? "steps" : "lines", std::move(arr));
    return doneReply(std::move(v), std::move(lines));
}

} // namespace

void addTIHarnessCommands(harness::CommandRegistry& r) {
    addProgramMemoryCommands(r);
    addDebugPanelCommands(r);
    r.addProduct("c59", "coverage", coverageCmd,
                 "coverage [all]  the ROM addresses 0000-17FF carried out since `profile on` (without key readiness unless all)");
    r.addProduct("c59", "line-times", lineTimesCmd,
                 "line-times [limit=<d>]  run until idle 15 s; each printed line's time, the gaps, the printer's floor");
    r.addProduct("c59", "run-time", runTimeCmd,
                 "run-time <key>...  press each key; the time until the display is lit and no program runs");
    r.addProduct("c59", "hot-steps", hotStepsCmd,
                 "hot-steps [limit=<d>]  run 200 s (or the limit); printer shares, program sources, the 25 hottest steps");
    r.addProduct("c59", "step-trace", stepTraceCmd, "step-trace <from> <to>  the program steps between two times");
    r.addProduct("c59", "step-cost", stepCostCmd,
                 "step-cost <keycode>...  what one run of the sequence costs in fast and normal mode");
    r.addProduct("c59", "keycodes", keycodesCmd,
                 "keycodes list|disassemble [<first> [<last>]] | assemble <file>  the program as a listing or an assembly");
}

} // namespace cu59
