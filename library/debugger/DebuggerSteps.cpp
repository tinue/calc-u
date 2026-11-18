// The debugger's preset steps: pause:, resume:, step:, pause-on-start:,
// debug-panel: and trace: on the presets' step place. Each is checked when the
// preset is read; none needs a running machine before it is carried out.
// pause: and step: have finished when the pause has landed (After::Own);
// the others when they took effect. They are no harness commands of the
// same name (`command = false`): the harness has its own, with other rules.
#include "debugger/DebuggerPart.hpp"
#include "formats/Number.hpp"
#include "presets/Engine.hpp"
#include "presets/Reader.hpp"
#include "presets/Vocabulary.hpp"
#include "sessions/MachineSession.hpp"
#include "sessions/Operation.hpp"
#include <algorithm>

namespace calcu::debugger {

using formats::YamlNode;
using presets::After;
using presets::Item;
using presets::ItemResult;
using presets::ReadContext;
using presets::RunContext;
using sessions::Task;

namespace {

std::string scalarOf(const YamlNode& n) {
    if (n.rawLine) return n.scalar;
    std::string s, e;
    n.asString(&s, &e);
    return s;
}

ItemResult failed(std::string why) { return ItemResult{false, std::move(why), {}}; }

// On a model without the run-control steps (the Sharp models) the step is
// [undefined] and refuses the preset.
bool defined(DebuggerPart& p, ReadContext& ctx, const char* verb, int line) {
    const auto& f = p.setup().runControlSteps;
    if (!f || f(ctx.model().model)) return true;
    ctx.error(line, std::string(verb) + " is not defined yet for the " + ctx.model().model.value + " by the preset format.");
    return false;
}

std::optional<LevelName> levelOf(DebuggerPart& p, ReadContext& ctx, const std::string& written, const char* verb, int line) {
    std::string l = presets::lowerAscii(written);
    if (p.runControl().rules().hasLevel(LevelName{l})) return LevelName{l};
    ctx.error(line, std::string(verb) + " takes a level the " + ctx.model().model.value + " has: " +
                        (p.runControl().rules().hasProgramLevel() ? "program or cpu." : "cpu."));
    return std::nullopt;
}

// Runs the machine until the run control has paused it: raw machine calls,
// which end at once while it is paused (at most 3600 s of
// emulated time). True when it paused.
Task<bool> untilPaused(RunContext& run) {
    sessions::MachineSession& ms = run.session().machineSession();
    const std::uint64_t gen = ms.generation();
    machine::Ticks ran = 0;
    for (;;) {
        if (ms.paused()) co_return true;
        const machine::Machine* m = ms.machine();
        if (!m || ms.generation() != gen) co_return false;
        const machine::TickRate rate = m->descriptor().tickRate;
        if (ran >= static_cast<machine::Ticks>(rate) * 3600) co_return false;
        const machine::Ticks piece = sessions::OperationContext::runPiece(rate);
        const machine::Ticks r = co_await sessions::request([&ms, piece] {
            bool stopped = false;
            return ms.advance(piece, stopped);
        });
        if (r == 0 && !ms.paused()) co_return false;
        ran += r;
    }
}

class PauseItem final : public Item {
public:
    PauseItem(DebuggerPart& p, LevelName l) : part_(p), level_(std::move(l)) {}
    Task<ItemResult> carryOut(RunContext& run) override {
        const RunControl::Result r = part_.runControl().pause(level_);
        if (!r.ok) co_return failed(r.reason);
        if (!co_await untilPaused(run)) co_return failed("The machine did not pause.");   // finished when landed
        co_return ItemResult{};
    }
    After after() const override { return After::Own; }
    bool needsRunningMachine() const override { return false; }
    std::string logText() const override { return "pause: " + level_.value; }

private:
    DebuggerPart& part_;
    LevelName level_;
};

class ResumeItem final : public Item {
public:
    explicit ResumeItem(DebuggerPart& p) : part_(p) {}
    Task<ItemResult> carryOut(RunContext&) override {
        const RunControl::Result r = part_.runControl().resume();
        co_return r.ok ? ItemResult{} : failed(r.reason);
    }
    After after() const override { return After::TakenEffect; }
    bool needsRunningMachine() const override { return false; }
    std::string logText() const override { return "resume"; }

private:
    DebuggerPart& part_;
};

class StepItem final : public Item {
public:
    StepItem(DebuggerPart& p, LevelName l, std::uint64_t n) : part_(p), level_(std::move(l)), count_(n) {}
    Task<ItemResult> carryOut(RunContext& run) override {
        RunControl& rc = part_.runControl();
        if (!rc.paused()) {   // a line in the log, and the next item
            run.log("step: ignored, the machine is not paused");
            co_return ItemResult{};
        }
        const RunControl::Result r = rc.step(level_, count_, std::nullopt, StepKind::In);
        if (!r.ok) co_return failed(r.reason);
        if (!co_await untilPaused(run)) co_return failed("The machine did not pause.");
        co_return ItemResult{};
    }
    After after() const override { return After::Own; }
    bool needsRunningMachine() const override { return false; }
    std::string logText() const override { return "step: " + level_.value + " " + std::to_string(count_); }

private:
    DebuggerPart& part_;
    LevelName level_;
    std::uint64_t count_;
};

class PauseOnStartItem final : public Item {
public:
    PauseOnStartItem(DebuggerPart& p, std::optional<LevelName> l) : part_(p), level_(std::move(l)) {}
    Task<ItemResult> carryOut(RunContext&) override {
        const RunControl::Result r = part_.runControl().pauseOnStart(level_);
        co_return r.ok ? ItemResult{} : failed(r.reason);
    }
    After after() const override { return After::TakenEffect; }
    bool needsRunningMachine() const override { return false; }
    std::string logText() const override { return "pause-on-start: " + (level_ ? level_->value : std::string("off")); }

private:
    DebuggerPart& part_;
    std::optional<LevelName> level_;
};

class PanelItem final : public Item {   // a frontend step: never fails
public:
    PanelItem(DebuggerPart& p, std::string v) : part_(p), view_(std::move(v)) {}
    Task<ItemResult> carryOut(RunContext& run) override {
        if (part_.setup().showPanel) part_.setup().showPanel(run.session(), view_);
        co_return ItemResult{};
    }
    After after() const override { return After::TakenEffect; }
    bool needsRunningMachine() const override { return false; }
    std::string logText() const override { return "debug-panel: " + view_; }

private:
    DebuggerPart& part_;
    std::string view_;
};

class TraceItem final : public Item {
public:
    TraceItem(DebuggerPart& p, std::optional<std::string> f) : part_(p), file_(std::move(f)) {}
    Task<ItemResult> carryOut(RunContext& run) override {
        TraceSession& t = part_.traceSession();
        if (!file_) {
            t.end(run.session(), TraceSession::Ending::Off);
            co_return ItemResult{};
        }
        // The trace is the running operation's; its end ends it.
        if (auto why = t.start(run.session(), *file_, false, true)) co_return failed(*why);
        co_return ItemResult{};
    }
    After after() const override { return After::TakenEffect; }
    bool needsRunningMachine() const override { return false; }
    std::string logText() const override { return "trace: " + (file_ ? *file_ : std::string("off")); }

private:
    DebuggerPart& part_;
    std::optional<std::string> file_;
};

} // namespace

void addDebuggerSteps(presets::Vocabulary& v, DebuggerPart& part) {
    DebuggerPart* p = &part;
    using presets::StepDef;
    v.add(StepDef{"pause", StepDef::Value::Scalar,
                  [p](const YamlNode& n, ReadContext& ctx) -> std::unique_ptr<Item> {
                      if (!defined(*p, ctx, "pause:", n.line)) return nullptr;
                      auto l = levelOf(*p, ctx, scalarOf(n), "pause:", n.line);
                      if (!l) return nullptr;
                      return std::make_unique<PauseItem>(*p, *l);
                  },
                  false, false});
    v.add(StepDef{"resume", StepDef::Value::None,
                  [p](const YamlNode& n, ReadContext& ctx) -> std::unique_ptr<Item> {
                      if (!defined(*p, ctx, "resume:", n.line)) return nullptr;
                      return std::make_unique<ResumeItem>(*p);
                  },
                  false, false});
    v.add(StepDef{"step", StepDef::Value::Any,
                  [p](const YamlNode& n, ReadContext& ctx) -> std::unique_ptr<Item> {
                      if (!defined(*p, ctx, "step:", n.line)) return nullptr;
                      std::string level = "cpu";
                      std::uint64_t count = 1;
                      if (n.isMap()) {
                          for (const auto& [k, x] : n.map) {
                              const int line = x.line ? x.line : n.line;
                              const std::string value = scalarOf(x);
                              if (k == "level") {
                                  level = value;
                              } else if (k == "count") {
                                  const auto c = formats::parseNumber(value);
                                  if (!c || *c < 1 || *c > kMaxStepCount) {
                                      ctx.error(line, "step: takes a count from 1 to 100000.");
                                      return nullptr;
                                  }
                                  count = static_cast<std::uint64_t>(*c);
                              } else {
                                  ctx.error(line, "Unknown key " + k + " in step:.");
                                  return nullptr;
                              }
                          }
                      } else if (!n.scalar.empty() || n.type != YamlNode::Type::Null) {
                          ctx.error(n.line, "step: takes { level: program|cpu, count: n }.");
                          return nullptr;
                      }
                      auto l = levelOf(*p, ctx, level, "step:", n.line);
                      if (!l) return nullptr;
                      return std::make_unique<StepItem>(*p, *l, count);
                  },
                  false, false});
    v.add(StepDef{"pause-on-start", StepDef::Value::Scalar,
                  [p](const YamlNode& n, ReadContext& ctx) -> std::unique_ptr<Item> {
                      if (!defined(*p, ctx, "pause-on-start:", n.line)) return nullptr;
                      const std::string w = presets::lowerAscii(scalarOf(n));
                      if (w == "off") return std::make_unique<PauseOnStartItem>(*p, std::nullopt);
                      auto l = levelOf(*p, ctx, w, "pause-on-start:", n.line);
                      if (!l) return nullptr;
                      return std::make_unique<PauseOnStartItem>(*p, *l);
                  },
                  false, false});
    v.add(StepDef{"debug-panel", StepDef::Value::Scalar,
                  [p](const YamlNode& n, ReadContext& ctx) -> std::unique_ptr<Item> {
                      if (!defined(*p, ctx, "debug-panel:", n.line)) return nullptr;
                      const std::string view = presets::lowerAscii(scalarOf(n));
                      const auto& f = p->setup().panelViews;
                      const std::vector<std::string> views = f ? f(ctx.model().model) : std::vector<std::string>{};
                      if (std::find(views.begin(), views.end(), view) == views.end()) {
                          std::string list;
                          for (const auto& x : views) list += (list.empty() ? "" : ", ") + x;
                          ctx.error(n.line, "debug-panel: takes a view of the debug panel" +
                                                (list.empty() ? std::string(", and the ") + ctx.model().model.value + " has none."
                                                              : ": " + list + "."));
                          return nullptr;
                      }
                      return std::make_unique<PanelItem>(*p, view);
                  },
                  false, true});
    v.add(StepDef{"trace", StepDef::Value::Scalar,
                  [p](const YamlNode& n, ReadContext& ctx) -> std::unique_ptr<Item> {
                      const std::string f = scalarOf(n);
                      if (presets::lowerAscii(f) == "off") return std::make_unique<TraceItem>(*p, std::nullopt);
                      if (auto why = traceNameRefusal(f)) {   // refused when the preset is read
                          ctx.error(n.line, *why);
                          return nullptr;
                      }
                      return std::make_unique<TraceItem>(*p, f);
                  },
                  false, false});
}

} // namespace calcu::debugger
