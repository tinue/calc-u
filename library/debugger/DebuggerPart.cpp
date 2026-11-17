#include "debugger/DebuggerPart.hpp"
#include "files/Files.hpp"
#include "formats/Number.hpp"
#include "machine/Names.hpp"
#include "sessions/Args.hpp"
#include "sessions/LibraryTopics.hpp"
#include "sessions/MachineSession.hpp"
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <utility>

namespace calcu::debugger {

using machine::DebugTargetId;
using sessions::CommandOutcome;
using sessions::Done;
using sessions::Refused;
using sessions::ReplyMessage;
using sessions::SessionContext;
using sessions::Value;

namespace {

Value u64(std::uint64_t v) { return Value(static_cast<std::int64_t>(v)); }

std::optional<std::uint64_t> numberOf(const Value* v) {
    if (!v) return std::nullopt;
    if (const std::int64_t* i = v->integer()) return static_cast<std::uint64_t>(*i);
    if (const std::string* s = v->string()) {
        if (auto n = formats::parseNumber(*s); n && *n >= 0) return static_cast<std::uint64_t>(*n);
    }
    return std::nullopt;
}

sessions::PayloadPtr runStatePayload(const RunControl& rc) { return std::make_shared<RunState>(rc.state()); }

CommandOutcome fromResult(const RunControl::Result& r, sessions::PayloadPtr state) {
    if (!r.ok) return Refused{r.reason};
    if (r.deferred) return sessions::Deferred{};
    return Done{std::move(state)};
}

// The reply with the run state as it is after the call: the call is made
// before this runs (argument order is unspecified, so the state must not be
// a second argument evaluated beside the call).
CommandOutcome withRunState(const RunControl::Result& r, const RunControl& rc) {
    return fromResult(r, runStatePayload(rc));
}

// The target by name (exact, then in any case), or none.
std::optional<std::size_t> findTarget(const machine::Debugging& d, const std::string& name) {
    if (auto i = targetIndex(d, DebugTargetId{name})) return i;
    for (std::size_t i = 0; i < d.targetCount(); ++i)
        if (machine::lowerAscii(d.target(i).info().name.value) == machine::lowerAscii(name)) return i;
    return std::nullopt;
}

std::optional<std::size_t> findSpace(const machine::TargetInfo& info, const std::string& name) {
    for (std::size_t i = 0; i < info.spaces.size(); ++i)
        if (info.spaces[i].name == name || machine::lowerAscii(info.spaces[i].name) == machine::lowerAscii(name)) return i;
    return std::nullopt;
}

Value infoValue(const machine::DebugTarget& t) {
    const machine::TargetInfo& info = t.info();
    Value::Array regs;
    for (const auto& r : info.registers)
        regs.push_back(Value::object({{"name", r.name}, {"bits", r.bits}, {"writable", r.writable}}));
    Value::Array spaces;
    for (const auto& s : info.spaces) {
        Value::Array banked;
        for (const auto& w : s.banked)
            banked.push_back(Value::object({{"first", u64(w.first)}, {"count", u64(w.count)},
                                            {"banks", static_cast<std::int64_t>(w.banks)},
                                            {"vbanks", static_cast<std::int64_t>(w.vbanks)}}));
        spaces.push_back(Value::object({{"name", s.name}, {"cellBits", s.cellBits}, {"size", u64(s.size)},
                                        {"writtenView", s.writtenView}, {"banked", Value(std::move(banked))}}));
    }
    Value::Array qualifier;
    const std::vector<std::string> now = t.qualifierNow();
    for (std::size_t i = 0; i < info.offers.qualifier.size(); ++i) {
        Value::Array values;
        for (const auto& v : info.offers.qualifier[i].values) values.push_back(Value(v));
        qualifier.push_back(Value::object({{"name", info.offers.qualifier[i].name},
                                           {"values", Value(std::move(values))},
                                           {"now", i < now.size() ? Value(now[i]) : Value()}}));
    }
    Value offers = Value::object({{"watches", info.offers.watches},
                                  {"stepOverOut", info.offers.stepOverOut},
                                  {"decode", info.offers.decode},
                                  {"history", static_cast<std::int64_t>(info.offers.history)},
                                  {"qualifier", Value(std::move(qualifier))}});
    return Value::object({{"name", info.name.value},
                          {"registers", Value(std::move(regs))},
                          {"spaces", Value(std::move(spaces))},
                          {"codeSpace", info.spaces.empty() ? Value() : Value(info.spaces[info.codeSpace].name)},
                          {"offers", std::move(offers)}});
}

Value cellValue(std::uint64_t address, const machine::CellRead& c) {
    switch (c.kind) {
    case machine::CellRead::Value: return Value::object({{"address", u64(address)}, {"value", u64(c.value)}});
    case machine::CellRead::Unreadable: return Value::object({{"address", u64(address)}, {"unreadable", true}});
    case machine::CellRead::NoneWritten: return Value::object({{"address", u64(address)}, {"none", true}});
    }
    return Value();
}

// The bank and vertical bank of a peek or poke into `bank`; the reason when
// they are refused (AC-49: the range lies inside one bank window that has them).
std::optional<std::string> bankOf(const Value& a, const machine::SpaceInfo& space, std::uint64_t address,
                                  std::uint64_t count, machine::BankSelect* bank) {
    if (auto b = numberOf(a.find("bank"))) bank->bank = static_cast<std::uint32_t>(*b);
    if (auto v = numberOf(a.find("vbank"))) bank->vbank = static_cast<std::uint32_t>(*v);
    if (!bank->bank && !bank->vbank) return std::nullopt;
    const auto w = std::find_if(space.banked.begin(), space.banked.end(), [&](const machine::BankWindow& x) {
        return address >= x.first && address + count <= x.first + x.count;
    });
    if (w == space.banked.end()) return "The range does not lie inside one bank of " + space.name + ".";
    if (bank->bank && *bank->bank >= w->banks) return "The space " + space.name + " has no bank " + std::to_string(*bank->bank) + ".";
    if (bank->vbank && *bank->vbank >= w->vbanks)
        return "The space " + space.name + " has no vertical bank " + std::to_string(*bank->vbank) + ".";
    return std::nullopt;
}

} // namespace

DebuggerPart::DebuggerPart(DebuggerSetup s)
    : setup_(std::move(s)), trace_(std::exchange(setup_.trace, {})),
      control_(setup_.rules ? *setup_.rules : ownRules_, trace_) {}

DebuggerPart::~DebuggerPart() = default;

const SymbolTable* DebuggerPart::symbols(const DebugTargetId& t) const {
    const auto it = symbols_.find(t);
    return it == symbols_.end() ? nullptr : &it->second;
}

void DebuggerPart::machineChanged(SessionContext&) { control_.sync(); }

void DebuggerPart::machineRestarted(SessionContext&, sessions::Restart r) {
    control_.restarted(r == sessions::Restart::Reset ? RestartKind::Reset : RestartKind::ColdBoot);
}

void DebuggerPart::operationEnded(SessionContext& c) {
    if (trace_.running() && trace_.ofOperation()) trace_.end(c, TraceSession::Ending::OperationEnded);   // T3
}

std::optional<std::uint64_t> DebuggerPart::addressOf(const Value* v, const DebugTargetId& target, std::string* error) const {
    if (auto n = numberOf(v)) return n;
    if (v && v->string()) {   // AC-51: a name of the target's symbols
        if (const SymbolTable* t = symbols(target))
            if (auto a = t->address(*v->string())) return a;
        *error = "There is no symbol " + *v->string() + " for " + target.value + ".";
        return std::nullopt;
    }
    *error = "An address is a number or a symbol.";
    return std::nullopt;
}

void DebuggerPart::attach(sessions::SessionSetup& s) {
    ctx_ = &s.context();
    trace_.setTopic(&s.stateTopic(kTraceTopic));
    control_.bind(*ctx_, &s.stateTopic(kRunStateTopic));
    ctx_->machineSession().setGate(&control_);
    addCommands(s);
    addRequests(s);
}

// ── Commands (§3.6) ──────────────────────────────────────────────────────

void DebuggerPart::addCommands(sessions::SessionSetup& s) {
    RunControl& rc = control_;
    // The machine's targets, or the refusal.
    auto dbg = [&rc](SessionContext& c, std::string* why) -> machine::Debugging* {
        if (auto r = rc.refusal()) {
            *why = *r;
            return nullptr;
        }
        return c.machine()->debugging();
    };
    auto levelOf = [](const Value& a) {
        const std::string l = sessions::textArg(a, "level");
        return LevelName{l.empty() ? std::string("cpu") : machine::lowerAscii(l)};
    };

    s.command(kPauseCommand, [&rc, levelOf](const Value& a, SessionContext& c) -> CommandOutcome {
        return withRunState(rc.pause(levelOf(a), c.currentCall()), rc);
    });
    s.command(kResumeCommand, [&rc](const Value&, SessionContext&) -> CommandOutcome {
        return withRunState(rc.resume(), rc);
    });
    s.command(kStepCommand, [&rc, levelOf](const Value& a, SessionContext& c) -> CommandOutcome {
        const std::int64_t count = sessions::integerArg(a, "count", 1);
        if (count < 1 || count > kMaxStepCount)
            return Refused{"step takes a count from 1 to " + std::to_string(kMaxStepCount) + "."};
        const std::string k = sessions::textArg(a, "kind");
        const StepKind kind = k == "over" ? StepKind::Over : k == "out" ? StepKind::Out : StepKind::In;
        const std::string t = sessions::textArg(a, "target");
        std::optional<DebugTargetId> target;
        if (!t.empty()) {
            if (machine::Machine* m = c.machine(); m && m->debugging()) {
                if (auto i = findTarget(*m->debugging(), t)) target = m->debugging()->target(*i).info().name;
                else return Refused{"The machine has no debug target " + t + "."};
            }
        }
        return withRunState(rc.step(levelOf(a), static_cast<std::uint64_t>(count), target, kind, c.currentCall()), rc);
    });
    s.command(kPauseOnStartCommand, [&rc](const Value& a, SessionContext&) -> CommandOutcome {
        const std::string l = machine::lowerAscii(sessions::textArg(a, "level"));
        if (sessions::flagArg(a, "off") || l == "off") return fromResult(rc.pauseOnStart(std::nullopt), nullptr);
        if (l.empty()) return Refused{"pause-on-start takes a level, or off."};
        return fromResult(rc.pauseOnStart(LevelName{l}), nullptr);
    });
    s.command(kPauseAtStartCommand, [&rc](const Value&, SessionContext&) -> CommandOutcome {
        if (auto r = rc.refusal(); r && *r == kNoTarget) return Refused{*r};
        rc.pauseAtStart();
        return Done{};
    });

    s.command(kBreakCommand, [this, &rc, dbg](const Value& a, SessionContext& c) -> CommandOutcome {
        std::string why;
        machine::Debugging* d = dbg(c, &why);
        if (!d) return Refused{why};
        const auto t = findTarget(*d, sessions::textArg(a, "target"));
        if (!t) return Refused{"The machine has no debug target " + sessions::textArg(a, "target") + "."};
        const machine::TargetInfo& info = d->target(*t).info();
        const auto address = addressOf(a.find("address"), info.name, &why);
        if (!address) return Refused{why};
        std::vector<std::string> qualifier;
        if (const Value* q = a.find("qualifier"); q && q->members() && !q->members()->empty()) {
            qualifier.assign(info.offers.qualifier.size(), std::string());
            for (const auto& m : *q->members()) {
                const auto f = std::find_if(info.offers.qualifier.begin(), info.offers.qualifier.end(),
                                            [&](const machine::QualifierField& x) { return x.name == m.key; });
                if (f == info.offers.qualifier.end())
                    return Refused{"The target " + info.name.value + " has no qualifier field " + m.key + "."};
                const std::string value = m.value.string() ? *m.value.string()
                                          : m.value.integer() ? std::to_string(*m.value.integer())
                                                              : std::string();
                qualifier[static_cast<std::size_t>(f - info.offers.qualifier.begin())] = value;
            }
        }
        std::string setter = sessions::textArg(a, "setter");
        if (setter.empty()) setter = kHarnessSetter;
        auto r = rc.table().addBreakpoint(setter, *d, info.name, *address, std::move(qualifier));
        if (auto* e = std::get_if<std::string>(&r)) return Refused{*e};
        rc.table().apply(*d);
        return Done{sessions::valuePayload("breakpoint", Value::object({{"id", std::get<BreakpointId>(r)}}))};
    });
    auto off = [&rc](const Value& a, SessionContext& c) -> CommandOutcome {
        std::string setter = sessions::textArg(a, "setter");
        if (setter.empty()) setter = kHarnessSetter;
        std::optional<BreakpointId> id;
        if (const Value* v = a.find("id"); v && v->integer()) id = static_cast<BreakpointId>(*v->integer());
        rc.table().remove(setter, id);   // only the setter's own (AC-47)
        if (machine::Machine* m = c.machine(); m && m->debugging()) rc.table().apply(*m->debugging());
        return Done{};
    };
    s.command(kBreakOffCommand, off);
    s.command(kWatchOffCommand, off);
    s.command(kWatchCommand, [&rc, dbg](const Value& a, SessionContext& c) -> CommandOutcome {
        std::string why;
        machine::Debugging* d = dbg(c, &why);
        if (!d) return Refused{why};
        const auto t = findTarget(*d, sessions::textArg(a, "target"));
        if (!t) return Refused{"The machine has no debug target " + sessions::textArg(a, "target") + "."};
        const machine::TargetInfo& info = d->target(*t).info();
        if (!info.offers.watches) return Refused{"The target " + info.name.value + " offers no watches."};
        const auto sp = findSpace(info, sessions::textArg(a, "space"));
        if (!sp) return Refused{"The target " + info.name.value + " has no space " + sessions::textArg(a, "space") + "."};
        const auto address = numberOf(a.find("address"));
        const auto count = numberOf(a.find("count"));
        if (!address || !count) return Refused{"A watch needs an address and a count."};
        const std::string access = sessions::textArg(a, "access");
        if (access != "read" && access != "write" && access != "both") return Refused{"A watch is read, write or both."};
        std::string setter = sessions::textArg(a, "setter");
        if (setter.empty()) setter = kHarnessSetter;
        machine::WatchRange range{*sp, *address, *count, access != "write", access != "read"};
        auto r = rc.table().addWatch(setter, *d, info.name, range);
        if (auto* e = std::get_if<std::string>(&r)) return Refused{*e};
        rc.table().apply(*d);
        return Done{sessions::valuePayload("watch", Value::object({{"id", std::get<BreakpointId>(r)}}))};
    });

    s.command(kSetRegisterCommand, [dbg](const Value& a, SessionContext& c) -> CommandOutcome {
        std::string why;
        machine::Debugging* d = dbg(c, &why);
        if (!d) return Refused{why};
        const auto t = findTarget(*d, sessions::textArg(a, "target"));
        if (!t) return Refused{"The machine has no debug target " + sessions::textArg(a, "target") + "."};
        machine::DebugTarget& target = d->target(*t);
        const std::string name = sessions::textArg(a, "register");
        const auto& regs = target.info().registers;
        const auto r = std::find_if(regs.begin(), regs.end(), [&](const machine::RegisterInfo& x) {
            return x.name == name || machine::lowerAscii(x.name) == machine::lowerAscii(name);
        });
        if (r == regs.end()) return Refused{"The target " + target.info().name.value + " has no register " + name + "."};
        const auto value = numberOf(a.find("value"));
        if (!value) return Refused{"set-register needs a value."};
        if (!target.writeRegister(static_cast<std::size_t>(r - regs.begin()), *value))
            return Refused{"The register " + r->name + " cannot be written."};
        return Done{};
    });
    s.command(kPokeCommand, [this, dbg](const Value& a, SessionContext& c) -> CommandOutcome {
        std::string why;
        machine::Debugging* d = dbg(c, &why);
        if (!d) return Refused{why};
        const auto t = findTarget(*d, sessions::textArg(a, "target"));
        if (!t) return Refused{"The machine has no debug target " + sessions::textArg(a, "target") + "."};
        machine::DebugTarget& target = d->target(*t);
        const auto sp = findSpace(target.info(), sessions::textArg(a, "space"));
        if (!sp) return Refused{"The target " + target.info().name.value + " has no space " + sessions::textArg(a, "space") + "."};
        const machine::SpaceInfo& space = target.info().spaces[*sp];
        const auto address = addressOf(a.find("address"), target.info().name, &why);
        if (!address) return Refused{why};
        const Value* vs = a.find("values");
        if (!vs || !vs->elements() || vs->elements()->empty()) return Refused{"poke needs values."};
        const std::uint64_t count = vs->elements()->size();
        if (*address >= space.size || count > space.size - *address)   // EC-9
            return Refused{"The address is outside the space " + space.name + "."};
        machine::BankSelect bank;
        if (auto why = bankOf(a, space, *address, count, &bank)) return Refused{*why};
        std::vector<std::pair<std::uint64_t, machine::CellRead>> before;
        for (std::uint64_t i = 0; i < count; ++i) {
            const auto v = numberOf(&(*vs->elements())[i]);
            const std::uint64_t at = *address + i;
            const machine::CellRead old = target.peek(*sp, at, bank);
            if (!v || !target.poke(*sp, at, *v, bank)) {
                for (auto it = before.rbegin(); it != before.rend(); ++it)   // a refused poke changes nothing
                    if (it->second.kind == machine::CellRead::Value) target.poke(*sp, it->first, it->second.value, bank);
                char buf[32];
                std::snprintf(buf, sizeof buf, "0x%llx", static_cast<unsigned long long>(at));
                return Refused{std::string("The cell ") + buf + " of " + space.name + " cannot be written."};
            }
            before.emplace_back(at, old);
        }
        return Done{};
    });

    s.command(kTraceCommand, [this](const Value& a, SessionContext& c) -> CommandOutcome {
        if (!c.machine()) return Refused{sessions::kNoMachine};
        if (sessions::flagArg(a, "off") || sessions::textArg(a, "file") == "off") {
            trace_.end(c, TraceSession::Ending::Off);
            return Done{};
        }
        const bool button = sessions::flagArg(a, "button");
        const std::string file = sessions::textArg(a, "file");
        if (!button && file.empty()) return Refused{"trace takes a file name, or off."};
        if (auto why = trace_.start(c, file, button, false)) return Refused{*why};
        return Done{};
    });
    s.command(kProfileCommand, [&rc](const Value& a, SessionContext&) -> CommandOutcome {
        if (auto r = rc.refusal()) return Refused{*r};
        const bool on = sessions::flagArg(a, "on");
        if (on && !rc.profile().on()) rc.profile().clear();
        rc.profile().setOn(on);
        return Done{};
    });
    s.command(kSymbolsCommand, [this, dbg](const Value& a, SessionContext& c) -> CommandOutcome {
        std::string why;
        machine::Debugging* d = dbg(c, &why);
        if (!d) return Refused{why};
        std::size_t t = 0;
        if (const std::string name = sessions::textArg(a, "target"); !name.empty()) {
            const auto i = findTarget(*d, name);
            if (!i) return Refused{"The machine has no debug target " + name + "."};
            t = *i;
        }
        const std::string file = sessions::textArg(a, "file");
        std::optional<std::vector<std::uint8_t>> bytes = sessions::bytesArg(a, "bytes");
        if (!bytes) bytes = files::readBytes(files::toPath(file));
        if (!bytes) return Refused{"Cannot read " + file + "."};
        for (const SymbolReader* r : setup_.symbolReaders) {
            if (!r->reads(file, *bytes)) continue;
            std::string error;
            auto table = r->read(*bytes, &error);
            if (!table) return Refused{error};
            const std::size_t n = table->size();
            symbols_[d->target(t).info().name] = std::move(*table);
            return Done{sessions::valuePayload("symbols", Value::object({{"count", static_cast<std::int64_t>(n)}}))};
        }
        return Refused{"No assembler that " + std::string("this product supports wrote ") + file + "."};
    });
}

// ── Requests (§3.6) ──────────────────────────────────────────────────────

void DebuggerPart::addRequests(sessions::SessionSetup& s) {
    RunControl& rc = control_;
    auto dbg = [&rc](SessionContext& c, std::string* why) -> machine::Debugging* {
        if (auto r = rc.refusal()) {
            *why = *r;
            return nullptr;
        }
        return c.machine()->debugging();
    };
    // The named target, or the current one when no name is given.
    auto targetOf = [](machine::Debugging& d, const Value& a, std::string* why) -> std::optional<std::size_t> {
        const std::string name = sessions::textArg(a, "target");
        if (name.empty()) return d.current();
        const auto i = findTarget(d, name);
        if (!i) *why = "The machine has no debug target " + name + ".";
        return i;
    };

    s.request(kTargetsRequest, [dbg](const Value&, SessionContext& c) {
        std::string why;
        machine::Debugging* d = dbg(c, &why);
        if (!d) return sessions::replyRefused(why);
        Value::Array out;
        for (std::size_t i = 0; i < d->targetCount(); ++i) out.push_back(infoValue(d->target(i)));
        return sessions::replyGiven(sessions::valuePayload("debug-targets", Value::object({{"targets", Value(std::move(out))}})));
    });
    s.request(kRegistersRequest, [dbg, targetOf](const Value& a, SessionContext& c) {
        std::string why;
        machine::Debugging* d = dbg(c, &why);
        if (!d) return sessions::replyRefused(why);
        const auto t = targetOf(*d, a, &why);
        if (!t) return sessions::replyRefused(why);
        const machine::DebugTarget& target = d->target(*t);
        Value::Array regs;
        for (std::size_t r = 0; r < target.info().registers.size(); ++r) {
            const auto& info = target.info().registers[r];
            regs.push_back(Value::object({{"name", info.name}, {"bits", info.bits}, {"value", u64(target.readRegister(r))}}));
        }
        return sessions::replyGiven(sessions::valuePayload(
            "debug-registers", Value::object({{"target", target.info().name.value}, {"registers", Value(std::move(regs))}})));
    });
    s.request(kPeekRequest, [this, dbg](const Value& a, SessionContext& c) {
        std::string why;
        machine::Debugging* d = dbg(c, &why);
        if (!d) return sessions::replyRefused(why);
        const auto t = findTarget(*d, sessions::textArg(a, "target"));
        if (!t) return sessions::replyRefused("The machine has no debug target " + sessions::textArg(a, "target") + ".");
        const machine::DebugTarget& target = d->target(*t);
        const auto sp = findSpace(target.info(), sessions::textArg(a, "space"));
        if (!sp) return sessions::replyRefused("The target " + target.info().name.value + " has no space " + sessions::textArg(a, "space") + ".");
        const machine::SpaceInfo& space = target.info().spaces[*sp];
        const auto address = addressOf(a.find("address"), target.info().name, &why);
        if (!address) return sessions::replyRefused(why);
        const std::uint64_t count = numberOf(a.find("count")).value_or(1);
        if (count == 0) return sessions::replyRefused("peek needs a count of at least 1.");
        if (*address >= space.size || count > space.size - *address)   // EC-9
            return sessions::replyRefused("The address is outside the space " + space.name + ".");
        machine::BankSelect bank;
        if (auto why = bankOf(a, space, *address, count, &bank)) return sessions::replyRefused(*why);
        Value::Array cells;
        for (std::uint64_t i = 0; i < count; ++i) cells.push_back(cellValue(*address + i, target.peek(*sp, *address + i, bank)));
        return sessions::replyGiven(sessions::valuePayload(
            "debug-cells", Value::object({{"target", target.info().name.value}, {"space", space.name},
                                          {"cellBits", space.cellBits}, {"cells", Value(std::move(cells))}})));
    });
    s.request(kHistoryRequest, [this, dbg, targetOf](const Value& a, SessionContext& c) {
        std::string why;
        machine::Debugging* d = dbg(c, &why);
        if (!d) return sessions::replyRefused(why);
        const auto t = targetOf(*d, a, &why);
        if (!t) return sessions::replyRefused(why);
        const machine::DebugTarget& target = d->target(*t);
        if (target.info().offers.history == 0) return sessions::replyRefused("The target " + target.info().name.value + " keeps no history.");
        const std::int64_t n = sessions::integerArg(a, "count", static_cast<std::int64_t>(target.info().offers.history));
        const SymbolTable* syms = symbols(target.info().name);
        Value::Array entries;
        for (const auto& e : target.history(static_cast<std::size_t>(std::max<std::int64_t>(0, n)))) {
            Value::Array regs;
            for (std::uint64_t r : e.registers) regs.push_back(u64(r));
            Value::Object o{{"address", u64(e.address)}, {"registers", Value(std::move(regs))}};
            if (e.text) o.push_back({"text", *e.text});
            if (syms)
                if (auto name = syms->nameAt(e.address)) o.push_back({"symbol", *name});
            entries.push_back(Value(std::move(o)));
        }
        Value::Array names;
        for (const auto& r : target.info().registers) names.push_back(Value(r.name));
        return sessions::replyGiven(sessions::valuePayload(
            "debug-history", Value::object({{"target", target.info().name.value},
                                            {"registers", Value(std::move(names))},
                                            {"entries", Value(std::move(entries))}})));
    });
    s.request(kDisassembleRequest, [this, dbg](const Value& a, SessionContext& c) {
        std::string why;
        machine::Debugging* d = dbg(c, &why);
        if (!d) return sessions::replyRefused(why);
        const auto t = findTarget(*d, sessions::textArg(a, "target"));
        if (!t) return sessions::replyRefused("The machine has no debug target " + sessions::textArg(a, "target") + ".");
        const machine::DebugTarget& target = d->target(*t);
        if (!target.info().offers.decode) return sessions::replyRefused("The target " + target.info().name.value + " does not decode.");
        const auto address = addressOf(a.find("address"), target.info().name, &why);
        if (!address) return sessions::replyRefused(why);
        const std::uint64_t size = target.info().spaces[target.info().codeSpace].size;
        if (*address >= size) return sessions::replyRefused("The address is outside the space " + target.info().spaces[target.info().codeSpace].name + ".");
        const std::int64_t n = sessions::integerArg(a, "count", 1);
        const SymbolTable* syms = symbols(target.info().name);
        Value::Array out;
        std::uint64_t at = *address;
        for (std::int64_t i = 0; i < n && at < size; ++i) {
            const auto dec = target.decode(at);
            if (!dec) break;
            Value::Object o{{"address", u64(at)}, {"text", dec->text}, {"cells", static_cast<std::int64_t>(dec->cells)}};
            if (syms)
                if (auto name = syms->nameAt(at)) o.push_back({"symbol", *name});
            out.push_back(Value(std::move(o)));
            at += std::max<std::uint32_t>(1, dec->cells);
        }
        return sessions::replyGiven(sessions::valuePayload(
            "debug-disassembly", Value::object({{"target", target.info().name.value}, {"instructions", Value(std::move(out))}})));
    });
    auto list = [&rc](bool watches) {
        return [&rc, watches](const Value& a, SessionContext&) {
            const std::string setter = sessions::textArg(a, "setter");
            const Setter* only = setter.empty() ? nullptr : &setter;
            Value::Array out;
            if (watches) {
                for (const auto& w : rc.table().watches(only))
                    out.push_back(Value::object({{"id", w.id}, {"setter", w.setter}, {"target", w.target.value},
                                                 {"space", static_cast<std::int64_t>(w.range.space)},
                                                 {"address", u64(w.range.first)}, {"count", u64(w.range.count)},
                                                 {"access", w.range.read && w.range.write ? "both" : w.range.read ? "read" : "write"}}));
            } else {
                for (const auto& b : rc.table().breakpoints(only)) {
                    Value::Array q;
                    for (const auto& v : b.qualifier) q.push_back(Value(v));
                    out.push_back(Value::object({{"id", b.id}, {"setter", b.setter}, {"target", b.target.value},
                                                 {"address", u64(b.address)}, {"qualifier", Value(std::move(q))}}));
                }
            }
            return sessions::replyGiven(sessions::valuePayload(watches ? "debug-watches" : "debug-breakpoints",
                                                               Value::object({{"entries", Value(std::move(out))}})));
        };
    };
    s.request(kBreakpointsRequest, list(false));
    s.request(kWatchesRequest, list(true));
    s.request(kProfileReportRequest, [this, &rc](const Value& a, SessionContext& c) {
        if (auto r = rc.refusal()) return sessions::replyRefused(*r);
        const std::int64_t n = sessions::integerArg(a, "count", 20);   // D-7
        machine::Debugging* d = c.machine()->debugging();
        Value::Array out;
        for (const auto& e : rc.profile().top(static_cast<std::size_t>(std::max<std::int64_t>(0, n)))) {
            const DebugTargetId name = e.target < d->targetCount() ? d->target(e.target).info().name : DebugTargetId{"?"};
            Value::Object o{{"target", name.value}, {"address", u64(e.address)}, {"count", u64(e.count)}};
            if (const SymbolTable* syms = symbols(name))
                if (auto sym = syms->nameAt(e.address)) o.push_back({"symbol", *sym});
            out.push_back(Value(std::move(o)));
        }
        return sessions::replyGiven(sessions::valuePayload(
            "debug-profile", Value::object({{"entries", Value(std::move(out))},
                                            {"distinct", static_cast<std::int64_t>(rc.profile().distinct())}})));
    });
}

} // namespace calcu::debugger
