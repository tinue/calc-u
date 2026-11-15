// The product made for the shell's tests (LIB-15 design §2, T1): LIB-1's
// test machines, LIB-3's sessions and LIB-9's settings file behind the
// library's Web glue. Product names carry the prefix "tp.". Built once and
// shipped under two product names (Test-A, Test-B; AC-7) and in two
// releases (AC-11); the worker config names the product.
//
//   start            {model, preset}: the model by name (test-seg,
//                    test-dot), or a bundled preset's path in /presets whose
//                    first line is `model: <name>`; a name or a path that
//                    names nothing gives a notice and the start goes on as
//                    without it (spec AC-16, EC-4). Without a model: the
//                    settings' [startup] model, else test-seg.
//   tp.machine       state: {model, preset}
//   tp.files         state: {files: [...]}, the save folder's files
//   tp.write         {name, text}: a file in the save folder (AC-5, AC-6)
//   tp.rename        {from, to};  tp.delete {name}
//   tp.set-model     {model}: the settings' [startup] model (AC-5)
//   tp.ask           {service, args}: asks a host service by name; the
//                    answer goes to tp.answers (AC-4, AC-9)
//   tp.answers       stream: {service, given, value, reason}
//   tp.calls         request: {calls}, the host calls made (AC-1, AC-2)
//   tp.read          request {name}: {text}, a save-folder file
//   tp.services      request: {services: [...]}, what the page stated (AC-4)
//   tp.settings      request: {model, outcome, path}
//   tp.bundle        request {name}: {text}, a file in /bundle (AC-11)
// From LIB-8 (M-39): the machines come from LIB-1's paper test catalogue,
// with LIB-2's peripherals part (set-control) and LIB-8's paper part for the
// test printers TD-20 and TP-4.
//   tp.attach        {place, peripheral, online?}: plugs a peripheral in
//                    (online: TD-20's switch `online` on at the plug)
//   tp.detach        {place}: takes out what is plugged in there
//   tp.press         {key}: the machine key down and up again
//   tp.printer       state: {owner, name, controls: [{id, form, on}]}, the
//                    printer plugged in first (owner null when none)
// From LIB-10 (M-40): the machines carry the test processor
// (machine/tests/TestCpu.hpp) and the session LIB-10's DebuggerPart with
// `cpu` only, a trace format of the tests and no trace folder: an ended
// trace is handed over with keep-file, a download (AC-26).
// From LIB-14 (M-43): the shell's test part (library/shell/tests/
// TestShellPart.hpp: a scripted control state, run state, roms-missing and
// media, the preset list made for the tests, the shell's commands recorded,
// `tsp.op`, `tsp.notice`, `tsp.publish`, `tsp.plug`) and the library's
// SettingsPart over this product's settings, with [preset] keys per model and
// [trace] max-file-size-mb.

#include "debugger/DebuggerPart.hpp"
#include "machine/tests/TestMachines.hpp"
#include "paper/PaperPart.hpp"
#include "peripherals/Controls.hpp"
#include "peripherals/PeripheralsPart.hpp"
#include "sessions/AppSession.hpp"
#include "sessions/LibraryTopics.hpp"
#include "sessions/MachineSession.hpp"
#include "sessions/SessionPart.hpp"
#include "settings/Declaration.hpp"
#include "settings/Paths.hpp"
#include "settings/SettingsFile.hpp"
#include "shell/SettingsPart.hpp"
#include "shell/tests/TestShellPart.hpp"
#include "web/glue/Bind.hpp"
#include "web/glue/MessageCodec.hpp"
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <system_error>
#include <vector>

namespace fs = std::filesystem;
namespace s = calcu::sessions;
namespace st = calcu::settings;
namespace mt = calcu::machine::test;
namespace cm = calcu::machine;
using calcu::web::WebStart;

namespace {

std::string textArg(const s::Value& args, const char* key) {
    const s::Value* v = args.find(key);
    return v && v->string() ? *v->string() : std::string();
}

std::optional<std::string> readText(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return std::nullopt;
    std::ostringstream out;
    out << in.rdbuf();
    return out.str();
}

// LIB-10: the trace format of the tests ("TTRC", 0x01, 3 bytes per
// instruction, 0x03), as library/debugger/tests/TestDebugParts.hpp has it.
class TraceOfTests final : public calcu::debugger::TraceRecorder {
public:
    explicit TraceOfTests(mt::TestMachine& m) : m_(&m) {}
    void begin(calcu::debugger::TraceFile& f) override {
        const std::uint8_t head[] = {'T', 'T', 'R', 'C', 0x01};
        f.write(head);
        m_->cpu()->recording = true;
        m_->cpu()->recorded.clear();
    }
    bool pump(calcu::debugger::TraceFile& f, std::uint64_t budget) override {
        if (!m_) return true;
        auto& rec = m_->cpu()->recorded;
        std::size_t n = 0;
        for (; n < rec.size() && budget >= 3; ++n, budget -= 3) {
            const std::uint8_t r[] = {0x02, rec[n].first, rec[n].second};
            f.write(r);
        }
        const bool all = n == rec.size();
        rec.erase(rec.begin(), rec.begin() + static_cast<std::ptrdiff_t>(n));
        return all;
    }
    std::uint64_t endBytes() const override { return 1; }
    void end(calcu::debugger::TraceFile& f) override {
        const std::uint8_t e[] = {0x03};
        f.write(e);
        if (m_) m_->cpu()->recording = false;
    }
    std::string buttonFileName() const override { return "TEST_TRACE.bin"; }
    void machineGone() override { m_ = nullptr; }

private:
    mt::TestMachine* m_;
};

calcu::debugger::DebuggerSetup debuggerSetup() {
    calcu::debugger::DebuggerSetup d;
    d.trace.recorder = [](cm::Machine& m) -> std::unique_ptr<calcu::debugger::TraceRecorder> {
        auto* tm = dynamic_cast<mt::TestMachine*>(&m);
        return tm && tm->cpu() ? std::make_unique<TraceOfTests>(*tm) : nullptr;
    };
    d.trace.handOver = true;   // no folder: kept in memory and downloaded (D-10)
    return d;
}

struct Machine final : s::PayloadOf<Machine> {
    static constexpr s::PayloadKind kKind{"tp.machine"};
    Machine(std::string m, std::string p) : model(std::move(m)), preset(std::move(p)) {}
    std::string model, preset;
    bool same(const Machine& o) const { return model == o.model && preset == o.preset; }
    s::Value toValue() const override { return s::Value::object({{"model", model}, {"preset", preset}}); }
};

struct Files final : s::PayloadOf<Files> {
    static constexpr s::PayloadKind kKind{"tp.files"};
    std::vector<std::string> names;
    bool same(const Files& o) const { return names == o.names; }
    s::Value toValue() const override {
        s::Value::Array a;
        for (const auto& n : names) a.push_back(s::Value(n));
        return s::Value::object({{"files", s::Value(std::move(a))}});
    }
};

struct Given final : s::PayloadOf<Given> {   // a request's answer or a stream item: a Value as it is
    static constexpr s::PayloadKind kKind{"tp.given"};
    explicit Given(s::Value x) : v(std::move(x)) {}
    s::Value v;
    bool same(const Given& o) const { return v == o.v; }
    s::Value toValue() const override { return v; }
};

st::ProductDecl declFor(const std::string& product) {
    st::ProductDecl d;
    d.productName = product;
    d.models = {mt::kSeg.value, mt::kDot.value};
    d.fallbackModel = mt::kSeg.value;
    d.keys = {st::family::startupModel(), st::family::startupLastModel(), st::family::presetFor(mt::kSeg.value),
              st::family::presetFor(mt::kDot.value), st::family::traceMaxFileSizeMb()};
    return d;
}

class TestProductPart final : public s::SessionPart {
public:
    explicit TestProductPart(const WebStart& w)
        : start_(w), decl_(declFor(w.product)),
          settings_(st::SettingsFile::defaultsOnly(decl_)), paths_(settings_, w.dirs) {
        // Only a store that is kept is written (design §3.3); a second copy
        // works in memory and so on: the settings are read from it but the
        // file system is not written over.
        settings_ = st::SettingsFile::open(decl_, paths_.settingsFile());
    }

    void attach(s::SessionSetup& setup) override {
        machine_ = &setup.stateTopic(s::TopicName{"tp.machine"});
        files_ = &setup.stateTopic(s::TopicName{"tp.files"});
        answers_ = &setup.streamTopic(s::TopicName{"tp.answers"}, s::History::SinceReset);
        printer_ = &setup.stateTopic(s::TopicName{"tp.printer"});
        printer_->set(std::make_shared<Given>(s::Value::object({{"owner", s::Value()}, {"name", ""}, {"controls", s::Value(s::Value::Array{})}})));
        setup.command(s::CommandName{"tp.attach"}, [this](const s::Value& a, s::SessionContext& c) -> s::CommandOutcome {
            cm::Machine* m = c.machine();
            if (!m) return s::Refused{"No machine."};
            const auto& cat = mt::paperTestCatalogue();
            cm::PeripheralRequest r = mt::peripheralAt(cat, textArg(a, "place"), cm::PeripheralId{textArg(a, "peripheral")});
            const s::Value* online = a.find("online");
            if (online && online->boolean() && *online->boolean()) r.switches.push_back(cm::SwitchPosition{cm::ControlId{"online"}, true});
            const cm::ChangeResult res = m->attach(std::move(r), cat);
            if (res.refused) return s::Refused{res.refused->reason};
            publishPrinter(c);
            return s::Done{};
        });
        setup.command(s::CommandName{"tp.detach"}, [this](const s::Value& a, s::SessionContext& c) -> s::CommandOutcome {
            cm::Machine* m = c.machine();
            if (!m) return s::Refused{"No machine."};
            const cm::PlacePath place{{cm::PlaceName{textArg(a, "place")}}};
            for (const auto& p : m->plugged().byPlace)
                if (p.place == place) {
                    const cm::ChangeResult res = m->detach(place, p.peripheral, mt::paperTestCatalogue());
                    if (res.refused) return s::Refused{res.refused->reason};
                    publishPrinter(c);
                    return s::Done{};
                }
            return s::Refused{"Nothing is plugged in there."};
        });
        setup.command(s::CommandName{"tp.press"}, [](const s::Value& a, s::SessionContext& c) -> s::CommandOutcome {
            cm::Machine* m = c.machine();
            if (!m) return s::Refused{"No machine."};
            const cm::KeyId k{textArg(a, "key")};
            if (m->keyDown(k) != cm::KeyCall::Done) return s::Refused{"No such key."};
            m->keyUp(k);
            return s::Done{};
        });
        setup.command(s::kStartCommand, [this](const s::Value& a, s::SessionContext& c) -> s::CommandOutcome {
            return startMachine(a, c);
        }, true);
        setup.command(s::CommandName{"tp.write"}, [this](const s::Value& a, s::SessionContext&) -> s::CommandOutcome {
            const auto ensured = paths_.ensure(st::Folder::Save);
            if (!ensured.ok) return s::Refused{ensured.problem};
            std::ofstream out(paths_.saveFolderFile(textArg(a, "name")), std::ios::binary | std::ios::trunc);
            out << textArg(a, "text");
            if (!out) return s::Refused{"The file could not be written."};
            out.close();
            listFiles();
            return s::Done{};
        });
        setup.command(s::CommandName{"tp.rename"}, [this](const s::Value& a, s::SessionContext&) -> s::CommandOutcome {
            std::error_code ec;
            fs::rename(paths_.saveFolderFile(textArg(a, "from")), paths_.saveFolderFile(textArg(a, "to")), ec);
            if (ec) return s::Refused{ec.message()};
            listFiles();
            return s::Done{};
        });
        setup.command(s::CommandName{"tp.delete"}, [this](const s::Value& a, s::SessionContext&) -> s::CommandOutcome {
            std::error_code ec;
            if (!fs::remove(paths_.saveFolderFile(textArg(a, "name")), ec)) return s::Refused{"No such file."};
            listFiles();
            return s::Done{};
        });
        setup.command(s::CommandName{"tp.set-model"}, [this](const s::Value& a, s::SessionContext&) -> s::CommandOutcome {
            const auto r = settings_.set("startup", "model", textArg(a, "model"));
            if (!r.ok) return s::Refused{r.problem};
            return s::Done{};
        });
        setup.command(s::CommandName{"tp.ask"}, [this](const s::Value& a, s::SessionContext& c) -> s::CommandOutcome {
            const std::string name = textArg(a, "service");
            const auto service = s::serviceNamed(name);
            if (!service) return s::Refused{"No such service: " + name + "."};
            const s::Value* args = a.find("args");
            const auto id = c.askHost(*service, args ? *args : s::Value(), [this, name](const s::ServiceAnswer& ans) {
                answers_->append(std::make_shared<Given>(s::Value::object(
                    {{"service", name}, {"given", ans.given}, {"value", ans.result}, {"reason", ans.reason}})));
            });
            if (!id) return s::Refused{"Nobody offers " + name + "."};
            return s::Done{};
        });
        setup.request(s::RequestName{"tp.calls"}, [this](const s::Value&, s::SessionContext&) {
            return s::replyGiven(std::make_shared<Given>(s::Value::object({{"calls", calls_}})));
        });
        setup.request(s::RequestName{"tp.read"}, [this](const s::Value& a, s::SessionContext&) {
            const auto t = readText(paths_.saveFolderFile(textArg(a, "name")));
            if (!t) return s::replyRefused("No such file.");
            return s::replyGiven(std::make_shared<Given>(s::Value::object({{"text", *t}})));
        });
        setup.request(s::RequestName{"tp.bundle"}, [this](const s::Value& a, s::SessionContext&) {
            const auto t = readText(start_.bundleDir + "/" + textArg(a, "name"));
            if (!t) return s::replyRefused("No such file.");
            return s::replyGiven(std::make_shared<Given>(s::Value::object({{"text", *t}})));
        });
        setup.request(s::RequestName{"tp.services"}, [](const s::Value&, s::SessionContext& c) {
            s::Value::Array names;
            for (int i = 0; i <= static_cast<int>(s::HostService::Haptics); ++i) {
                const auto svc = static_cast<s::HostService>(i);
                if (c.hostOffers(svc)) names.push_back(s::Value(s::serviceName(svc)));
            }
            return s::replyGiven(std::make_shared<Given>(s::Value::object({{"services", s::Value(std::move(names))}})));
        });
        setup.request(s::RequestName{"tp.settings"}, [this](const s::Value&, s::SessionContext&) {
            return s::replyGiven(std::make_shared<Given>(s::Value::object(
                {{"model", settings_.text("startup", "model")},
                 {"outcome", static_cast<int>(settings_.outcome())},
                 {"path", settings_.path()}})));
        });
    }

    void afterCall(s::SessionContext& c, calcu::pacing::WallTime) override {
        ++calls_;
        publishPrinter(c);
    }

    // LIB-14: the library's SettingsPart reads and writes them.
    st::SettingsFile& settingsFile() { return settings_; }
    const st::Paths& paths() const { return paths_; }

private:
    s::CommandOutcome startMachine(const s::Value& a, s::SessionContext& c) {
        std::string model = textArg(a, "model");
        const std::string preset = textArg(a, "preset");
        std::string loaded;
        if (!model.empty() && !decl_.isModel(model)) {   // EC-4
            c.notice("There is no model " + model + ".");
            model.clear();
        }
        if (!preset.empty()) {
            const auto t = preset.find("..") == std::string::npos ? readText(start_.presetsDir + "/" + preset)
                                                                  : std::nullopt;
            const std::string prefix = "model: ";
            if (!t || t->rfind(prefix, 0) != 0) {
                c.notice("There is no bundled preset " + preset + ".");   // AC-16, EC-4
            } else {
                std::string m = t->substr(prefix.size());
                m.erase(std::find_if(m.begin(), m.end(), [](char ch) { return ch == '\n' || ch == '\r'; }), m.end());
                if (decl_.isModel(m)) {
                    model = m;
                    loaded = preset;
                }
            }
        }
        if (model.empty()) {
            const std::string fromSettings = settings_.text("startup", "model");
            model = decl_.isModel(fromSettings) ? fromSettings : decl_.fallbackModel;
        }
        mt::TestBuilder builder;
        const auto& cat = mt::paperTestCatalogue();
        auto built = cm::build(cat, builder, mt::requestFor(cat, cm::ModelId{model}));
        auto m = std::move(built.machine);
        if (!m) return s::Refused{"The machine could not be built."};
        // LIB-10: the test processor; a loop of nop; ld a, #5; st a, [0x30]; jmp 0.
        static_cast<mt::TestMachine&>(*m).enableCpu(mt::TestProgram({0x00, 0x01, 0x05, 0x03, 0x30, 0x06, 0x00}));
        c.machineSession().setMachine(std::move(m), calcu::pacing::PacingProfile{}, 8000);
        machine_->set(std::make_shared<Machine>(model, loaded));
        listFiles();
        return s::Done{};
    }

    // The printer plugged in first (TD-20 or TP-4), and its controls.
    void publishPrinter(s::SessionContext& c) {
        s::Value owner, name = s::Value(std::string());
        s::Value::Array controls;
        if (cm::Machine* m = c.machine())
            for (const auto& p : m->plugged().byPlace) {
                if (p.peripheral != mt::kDotter && p.peripheral != mt::kPlotter) continue;
                owner = s::Value(p.peripheral.value);
                if (const auto* d = mt::paperTestCatalogue().peripheral(p.peripheral)) name = s::Value(d->name);
                for (const auto& ci : calcu::peripherals::controlsOf(*m, cm::PeripheralOwner{p.peripheral, p.place}))
                    controls.push_back(s::Value::object({{"id", ci.id.value},
                                                         {"form", ci.form == cm::ControlForm::Key ? "key" : "switch"},
                                                         {"on", ci.on}}));
                break;
            }
        printer_->set(std::make_shared<Given>(s::Value::object({{"owner", owner}, {"name", name}, {"controls", s::Value(std::move(controls))}})));
    }

    void listFiles() {
        auto f = std::make_shared<Files>();
        std::error_code ec;
        for (fs::directory_iterator it(paths_.saveFolder(), ec), end; !ec && it != end; it.increment(ec))
            if (it->is_regular_file()) f->names.push_back(it->path().filename().string());
        std::sort(f->names.begin(), f->names.end());
        files_->set(f);
    }

    WebStart start_;
    st::ProductDecl decl_;
    st::SettingsFile settings_;
    st::Paths paths_;
    s::StateTopic *machine_ = nullptr, *files_ = nullptr, *printer_ = nullptr;
    s::StreamTopic* answers_ = nullptr;
    std::int64_t calls_ = 0;
};

// The parts the session owns: LIB-2's peripherals part, LIB-8's paper part
// with the two test printers, the product's part.
struct Parts {
    explicit Parts(const WebStart& w)
        : paper(mt::paperTestCatalogue(), {{mt::kDotter, mt::td20Description()}, {mt::kPlotter, mt::tp4Description()}}),
          product(w), debugger(debuggerSetup()), settings(settingsSetup(product)) {}
    calcu::peripherals::PeripheralsPart peripherals;
    calcu::paper::PaperPart paper;
    TestProductPart product;
    calcu::debugger::DebuggerPart debugger;   // LIB-10
    static calcu::shell::SettingsPartSetup settingsSetup(TestProductPart& p) {
        calcu::shell::SettingsPartSetup s;
        s.file = &p.settingsFile();
        s.paths = &p.paths();
        s.frontend = st::FrontendKind::Web;
        return s;
    }
    calcu::shell::SettingsPart settings;          // LIB-14
    calcu::shell::test::TestShellPart shellPart{true, false};  // LIB-14; run control: the DebuggerPart's
};

std::unique_ptr<s::AppSession> makeTestSession(const WebStart& w) {
    auto parts = std::make_shared<Parts>(w);
    s::AppSessionSetup setup;
    setup.clock = calcu::web::wallTimeNow;
    setup.hostTime = [] { return std::chrono::system_clock::now(); };
    setup.parts = {w.shellPart, &parts->peripherals, &parts->paper, &parts->product, &parts->debugger, &parts->settings, &parts->shellPart};
    setup.hosting = s::Hosting::Paced;
    setup.owned = std::move(parts);
    return std::make_unique<s::AppSession>(std::move(setup));
}

} // namespace

CALCU_WEB_PRODUCT(CalcUTestHost, makeTestSession)
