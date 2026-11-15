#pragma once
#include "PresetRig.hpp"
#include "TestMachines.hpp"
#include "debugger/DebuggerPart.hpp"
#include "debugger/Levels.hpp"
#include "debugger/RunState.hpp"
#include "debugger/Symbols.hpp"
#include "debugger/TraceSession.hpp"
#include "presets/PresetsPart.hpp"
#include <chrono>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

// The debugger's parts built for the tests (LIB-10 design §6):
//   TestLevelRules   the test product's levels: `program` lands at the
//                    next program step boundary (the mark of a `nop`), one
//                    `program` step runs to the next boundary; `cpu` lands
//                    at once; the cpu repeats, the restart rule and the
//                    pause-on-start of `program` (the next boundary) are
//                    settable.
//   TestRecorder     a trace format of the tests: the header "TTRC", 0x01
//                    for the start of a session, 3 bytes per instruction
//                    (0x02, target, address), 0x03 for its end; the button's
//                    file name TEST_TRACE.bin.
//   TestSymbols      a table of the tests: a file `*.sym` of lines
//                    `name = <number>`.
//   DebugRig         an app session with PeripheralsPart, PresetsPart,
//                    StartupPart, TypingPart and DebuggerPart (the preset
//                    rig's parts, library/presets/tests/PresetRig.hpp) on the
//                    test machines with the test processor (TestCpu.hpp) and
//                    the program `defaultProgram()`; the debugger's steps in
//                    the preset vocabulary; a trace folder of its own.

namespace calcu::debugger::test {

namespace mt = calcu::machine::test;
namespace pt = calcu::presets::test;
using machine::Ticks;
using sessions::CallId;
using sessions::Value;
using namespace std::chrono_literals;

class TestLevelRules final : public LevelRules {
public:
    bool hasProgramLevel() const override { return program; }
    bool pauseLandsAtOnce(const LevelName& l) const override { return l == kCpu; }
    bool pauseLandsAfter(const LevelName&, const machine::InstructionEvent& e) override {
        return (e.marks & mt::cpu::kProgramStep) != 0;
    }
    void pauseAsked(const LevelName&, machine::Machine&) override { ++pausesAsked; }
    bool programStepDoneAfter(const machine::InstructionEvent& e) override { return (e.marks & mt::cpu::kProgramStep) != 0; }
    int cpuStepRepeats() const override { return repeats; }
    bool startFiresAfter(const LevelName& l, const machine::InstructionEvent& e, const machine::Machine& m) override {
        if (l == kProgram) return (e.marks & mt::cpu::kProgramStep) != 0;
        return LevelRules::startFiresAfter(l, e, m);
    }
    bool startFiresAtRestart(const LevelName& l, RestartKind k) const override {
        return l == kCpu && k == RestartKind::ColdBoot && coldBootFires;
    }
    bool program = true;
    int repeats = 0;
    bool coldBootFires = false;
    int pausesAsked = 0;
};

class TestRecorder final : public TraceRecorder {
public:
    explicit TestRecorder(mt::TestMachine& m) : m_(&m) {}
    void begin(TraceFile& f) override {
        const std::uint8_t head[] = {'T', 'T', 'R', 'C', 0x01};
        f.write(head);
        if (m_ && m_->cpu()) {
            m_->cpu()->recording = true;
            m_->cpu()->recorded.clear();
        }
    }
    bool pump(TraceFile& f, std::uint64_t budget) override {
        if (!m_ || !m_->cpu()) return true;
        auto& rec = m_->cpu()->recorded;
        std::size_t n = 0;
        bool all = true;
        for (; n < rec.size(); ++n) {
            if (budget < 3) {
                all = false;
                break;
            }
            const std::uint8_t r[] = {0x02, rec[n].first, rec[n].second};
            f.write(r);
            budget -= 3;
        }
        rec.erase(rec.begin(), rec.begin() + static_cast<std::ptrdiff_t>(n));
        return all;
    }
    std::uint64_t endBytes() const override { return 1; }
    void end(TraceFile& f) override {
        const std::uint8_t e[] = {0x03};
        f.write(e);
        if (m_ && m_->cpu()) m_->cpu()->recording = false;
    }
    std::string buttonFileName() const override { return "TEST_TRACE.bin"; }
    void machineGone() override { m_ = nullptr; }

private:
    mt::TestMachine* m_;
};

class TestSymbols final : public SymbolReader {
public:
    bool reads(std::string_view name, const std::vector<std::uint8_t>&) const override {
        return name.size() > 4 && name.substr(name.size() - 4) == ".sym";
    }
    std::optional<SymbolTable> read(const std::vector<std::uint8_t>& bytes, std::string* error) const override {
        SymbolTable t;
        std::istringstream in(std::string(bytes.begin(), bytes.end()));
        for (std::string line; std::getline(in, line);) {
            if (line.empty()) continue;
            const auto eq = line.find('=');
            if (eq == std::string::npos) {
                *error = "A symbol line is name = number.";
                return std::nullopt;
            }
            auto trim = [](std::string s) {
                while (!s.empty() && s.front() == ' ') s.erase(s.begin());
                while (!s.empty() && s.back() == ' ') s.pop_back();
                return s;
            };
            const auto n = formats::parseNumber(trim(line.substr(eq + 1)));
            if (!n) {
                *error = "A symbol line is name = number.";
                return std::nullopt;
            }
            t.add(trim(line.substr(0, eq)), static_cast<std::uint64_t>(*n));
        }
        return t;
    }
};

// The program of the tests (TestCpu.hpp): a loop with a program step
// boundary, a store, a call and a return.
//   00 nop             ; boundary
//   01 ld a, #5
//   03 st a, [0x30]
//   05 call 0x10
//   07 nop             ; boundary
//   08 ld a, [0x31]
//   0a jmp 0x00
//   10 ld a, [0x30]    ; the routine
//   12 out 0x02, a
//   14 ret
inline mt::TestProgram defaultProgram() {
    using namespace mt::cpu;
    std::vector<std::uint8_t> p(0x20, 0);
    const std::uint8_t main[] = {kNop, kLdA, 5, kSt, 0x30, kCall, 0x10, kNop, kLdMem, 0x31, kJmp, 0x00};
    std::copy(std::begin(main), std::end(main), p.begin());
    const std::uint8_t routine[] = {kLdMem, 0x30, kOut, 0x02, kRet};
    std::copy(std::begin(routine), std::end(routine), p.begin() + 0x10);
    return mt::TestProgram(p, {0, 7});
}

// test-seg builds with the program of LIB-6's rig (key `1` starts a
// program of `segProgram` ticks) and the test processor.
struct DebugBuilder final : machine::MachineBuilder {
    mt::TestBuilder inner;
    mt::TestProgram program = defaultProgram();
    Ticks segProgram = 50'000;
    bool cpu = true;
    machine::BuildResult build(const machine::BuildRequest& req, const machine::Catalogue& cat) override {
        machine::BuildRequest r = req;
        if (!r.content && r.model == mt::kSeg) r.content = std::make_shared<mt::TestSegProgram>(segProgram);
        machine::BuildResult b = inner.build(r, cat);
        if (b.machine && cpu) static_cast<mt::TestMachine&>(*b.machine).enableCpu(program);
        return b;
    }
};

class DebugMachines final : public startup::ProductMachines {
public:
    const machine::Catalogue& catalogue() const override { return mt::testCatalogue(); }
    machine::MachineBuilder& builder() override { return builder_; }
    startup::MachinePlan builtInDefault(const machine::ModelId& m) override { return startup::MachinePlan{m, {}, {}, nullptr}; }
    pacing::PacingProfile pacingProfile(const machine::ModelId&) const override { return {}; }
    int soundRate(const machine::ModelId& m) const override { return m == mt::kDot ? 8000 : 0; }
    machine::Seconds readinessBound(const machine::ModelId&) const override { return machine::Seconds(1.0); }
    DebugBuilder builder_;
};

struct DebugOptions {
    sessions::Hosting hosting = sessions::Hosting::VirtualTime;
    std::string model = "test-seg";
    bool cpu = true;
    enum class Folder { Writable, None, NotWritable } folder = Folder::Writable;
    bool handOver = false;
    sessions::HostServices services;
    long limitMb = 0;
    std::optional<mt::TestProgram> program;
};

class DebugRig {
public:
    explicit DebugRig(DebugOptions o = {})
        : opts(o), folder("calcu-debug"), traces("calcu-traces"),
          settings(settings::SettingsFile::defaultsOnly(startup::test::testDecl())),
          typing(typingProduct, o.hosting), debugger(debuggerSetup()) {
        machines.builder_.cpu = o.cpu;
        if (o.program) machines.builder_.program = *o.program;
        presets::PresetsSetup ps;
        ps.catalogue = &mt::testCatalogue();
        ps.rules = &presetRules;
        ps.typing = &typing;
        ps.home = folder / "home";
        presetsPart = std::make_unique<presets::PresetsPart>(ps);
        addDebuggerSteps(presetsPart->vocabulary(), debugger);
        startupPart = std::make_unique<startup::StartupPart>(
            startup::StartupSetup{machines, startup::test::testRoms(), settings, presetsPart.get()});
        presetsPart->setControl(&startupPart->control());
        sessions::AppSessionSetup s;
        s.clock = clock.fn();
        s.hostTime = [this] { return hostNow; };
        s.parts = {&peripherals, presetsPart.get(), startupPart.get(), &typing, &debugger};
        s.hosting = o.hosting;
        s.jitterSeed = 1;
        session = std::make_unique<sessions::AppSession>(std::move(s));
        id = session->listen(fe, o.services);
        send("start", Value::object({{"model", o.model}}));
        callWhileBusy();
    }
    DebugRig(const DebugRig&) = delete;
    DebugRig& operator=(const DebugRig&) = delete;

    DebuggerSetup debuggerSetup() {
        DebuggerSetup d;
        d.rules = &rules;
        d.trace.recorder = [](machine::Machine& m) -> std::unique_ptr<TraceRecorder> {
            auto* tm = dynamic_cast<mt::TestMachine*>(&m);
            return tm && tm->cpu() ? std::make_unique<TestRecorder>(*tm) : nullptr;
        };
        d.trace.folder = [this]() -> std::optional<settings::Paths::Ensured> {
            switch (opts.folder) {
            case DebugOptions::Folder::None: return std::nullopt;
            case DebugOptions::Folder::NotWritable: return settings::Paths::Ensured{false, traces.str(), "it is read-only."};
            case DebugOptions::Folder::Writable: break;
            }
            return settings::Paths::Ensured{true, traces.str(), {}};
        };
        d.trace.limitMb = [this] { return opts.limitMb; };
        d.trace.handOver = opts.handOver;
        d.symbolReaders = {&symbolReader};
        d.runControlSteps = [](const machine::ModelId& m) { return m != mt::kDot; };   // test-dot: as a Sharp model
        d.panelViews = [](const machine::ModelId& m) {
            return m == mt::kDot ? std::vector<std::string>{} : std::vector<std::string>{"cpu", "log"};
        };
        d.showPanel = [this](sessions::SessionContext&, const std::string& v) { shownViews.push_back(v); };
        return d;
    }

    CallId send(const std::string& name, Value args = Value()) {
        return session->send(id, sessions::Command{sessions::CommandName{name}, std::move(args)});
    }
    sessions::ReplyMessage ask(const std::string& name, Value args = Value()) {
        const CallId c = session->ask(id, sessions::Request{sessions::RequestName{name}, std::move(args)});
        return *fe.reply(c);
    }
    CallId load(const std::string& text, const std::string& file = "t.test-seg") {
        folder.write(file, text);
        return send("load-preset", Value::object({{"ref", folder / file}}));
    }
    void call(pacing::WallTime frame = 16ms) {
        clock.now += frame;
        session->call(clock.now);
    }
    int callWhileBusy(int max = 100'000) {
        int n = 0;
        while (session->busy() && n < max) {
            call();
            ++n;
        }
        return n;
    }
    Ticks advance(Ticks t) { return session->advanceUnpaced(t); }
    bool replied(CallId c) const { return fe.reply(c) != nullptr; }
    bool done(CallId c) const { return fe.reply(c) && fe.reply(c)->done; }
    std::string reason(CallId c) const { return fe.reply(c) ? fe.reply(c)->reason : std::string("(no reply)"); }
    const RunState* replyState(CallId c) const {
        return fe.reply(c) ? sessions::payloadAs<RunState>(fe.reply(c)->result) : nullptr;
    }
    const RunState* runState() const { return fe.state<RunState>(kRunStateTopic); }
    const TraceState* traceState() const { return fe.state<TraceState>(kTraceTopic); }
    bool paused() { return session->machineSession().paused(); }
    mt::TestMachine* machine() { return static_cast<mt::TestMachine*>(session->context().machine()); }
    mt::TestCpu* cpu() { return machine() ? machine()->cpu() : nullptr; }
    machine::DebugTarget& target(std::size_t i = 0) { return machine()->debugging()->target(i); }
    std::uint64_t pc(std::size_t i = 0) { return target(i).nextAddress(); }
    Ticks ticks() { return machine() ? machine()->tickCount() : 0; }
    std::vector<std::string> logLines() {
        std::vector<std::string> out;
        for (const auto& p : fe.mirror.takeAppended(sessions::kLogTopic))
            if (const auto* l = sessions::payloadAs<sessions::LogLine>(p)) out.push_back(l->text);
        return out;
    }
    std::vector<std::string> notices() { return fe.noticeSentences(); }
    // Runs single instructions (virtual time) until the target stands at `address`.
    bool runTo(std::uint64_t address, std::size_t target = 0, int max = 10'000) {
        for (int i = 0; i < max; ++i) {
            if (pc(target) == address) return true;
            if (advance(1) == 0) return false;
        }
        return false;
    }
    sessions::Value breakAt(std::uint64_t address, const std::string& target = "test-cpu",
                            const std::string& setter = "harness") {
        const CallId c = send("break", Value::object({{"setter", setter}, {"target", target}, {"address", static_cast<std::int64_t>(address)}}));
        const auto* r = fe.reply(c);
        return r && r->done && r->result ? r->result->toValue() : Value();
    }

    DebugOptions opts;
    calcu::testing::TempFolder folder, traces;
    sessions::test::ScriptedClock clock;
    sessions::test::TestFrontend fe;
    std::chrono::system_clock::time_point hostNow{std::chrono::hours(24 * 365 * 50)};
    DebugMachines machines;
    settings::SettingsFile settings;
    pt::TestPresetRules presetRules;
    pt::RigTyping typingProduct;
    typing::TypingPart typing;
    peripherals::PeripheralsPart peripherals;
    TestLevelRules rules;
    TestSymbols symbolReader;
    std::vector<std::string> shownViews;
    DebuggerPart debugger;
    std::unique_ptr<presets::PresetsPart> presetsPart;
    std::unique_ptr<startup::StartupPart> startupPart;
    std::unique_ptr<sessions::AppSession> session;
    sessions::ListenerId id = 0;
};

} // namespace calcu::debugger::test
