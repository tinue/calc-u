#pragma once
#include "Files.hpp"
#include "TempFolder.hpp"
#include "TestFrontend.hpp"
#include "TestMachines.hpp"
#include "TestParts.hpp"
#include "TestPresetRules.hpp"
#include "TestProductMachines.hpp"
#include "peripherals/PeripheralsPart.hpp"
#include "presets/PresetsPart.hpp"
#include "sessions/AppSession.hpp"
#include "sessions/LibraryTopics.hpp"
#include "sessions/MachineSession.hpp"
#include "settings/SettingsFile.hpp"
#include "startup/StartupPart.hpp"
#include "typing/TypingPart.hpp"
#include <chrono>
#include <memory>
#include <optional>
#include <string>
#include <vector>

// The window of the preset tests: an app session with PeripheralsPart,
// PresetsPart, StartupPart and TypingPart, in that order; the machine area's
// test machines with a program on test-seg the tests can start and stop; the
// rules and keywords of TestPresetRules.hpp; the sessions' test frontend; the
// startup area's test settings. In virtual time unless a case says Paced (the
// session tests' scripted clock, 1 ms per reading, host calls of 16 ms).
//
// Typing profiles: test-seg 10 ms down, 5 ms up, presses wait for
// key readiness (polled every 10 ticks), the characters 0-3, `clr` as ENTER;
// test-dot the same in its ticks, the characters a-c, `on` as ENTER.

namespace calcu::presets::test {

using machine::Ticks;
using sessions::CallId;
using sessions::Value;
using namespace std::chrono_literals;

inline typing::TypingProfile rigProfile(machine::TickRate rate, char first, char last, const char* enter) {
    typing::TypingProfile p;
    p.minHold = typing::ticksOf(0.010, rate);
    p.typed = typing::Cadence{typing::ticksOf(0.010, rate), typing::ticksOf(0.005, rate)};
    p.queuedWaitsForReadiness = true;
    p.readinessPoll = std::max<Ticks>(1, rate / 10'000);
    p.charKeys = [first, last](char32_t c, const typing::TypingSupport*) -> std::optional<std::vector<typing::Press>> {
        if (c < static_cast<char32_t>(first) || c > static_cast<char32_t>(last)) return std::nullopt;
        return std::vector<typing::Press>{typing::Press{machine::KeyId{std::string(1, static_cast<char>(c))}}};
    };
    p.enter = machine::KeyId{enter};
    return p;
}

class RigTyping final : public typing::TypingProduct {
public:
    RigTyping() : seg(rigProfile(mt::kSegRate, '0', '3', "clr")), dot(rigProfile(mt::kDotRate, 'a', 'c', "on")) {}
    const typing::TypingProfile* profile(const machine::ModelId& m) const override { return m == mt::kDot ? &dot : &seg; }
    const typing::KeyboardMap* keyboardMap(const machine::ModelId&) const override { return nullptr; }
    bool typesText() const override { return true; }
    typing::TypingProfile seg, dot;
};

// The builder of the rig: the test builder, then the program on test-seg, a
// start-up of its own, and the product rule of the test printer and the
// test UART refused together.
struct RigBuilder final : machine::MachineBuilder {
    mt::TestBuilder inner;
    Ticks program = 50'000;   // 0.5 s on test-seg
    std::optional<Ticks> startUp;
    machine::BuildResult build(const machine::BuildRequest& req, const machine::Catalogue& cat) override {
        bool printer = false, uart = false;
        for (const auto& p : req.peripherals) {
            printer = printer || p.peripheral == mt::kPrinter;
            uart = uart || p.peripheral == mt::kUart;
        }
        if (printer && uart) return machine::BuildResult{nullptr, {}, mt::kTogether};
        machine::BuildRequest r = req;
        if (!r.content && r.model == mt::kSeg) r.content = std::make_shared<mt::TestSegProgram>(program);
        machine::BuildResult b = inner.build(r, cat);
        if (b.machine && startUp) static_cast<mt::TestMachine&>(*b.machine).setStartUp(*startUp);
        return b;
    }
};

class RigMachines final : public startup::ProductMachines {
public:
    explicit RigMachines(const machine::Catalogue& c) : cat_(c) {}
    const machine::Catalogue& catalogue() const override { return cat_; }
    machine::MachineBuilder& builder() override { return builder_; }
    startup::MachinePlan builtInDefault(const machine::ModelId& m) override { return startup::MachinePlan{m, {}, {}, nullptr}; }
    pacing::PacingProfile pacingProfile(const machine::ModelId&) const override { return {std::chrono::microseconds(16'667), false}; }   // a frame per call
    int soundRate(const machine::ModelId& m) const override { return m == mt::kDot ? 8000 : 0; }
    machine::Seconds readinessBound(const machine::ModelId&) const override { return machine::Seconds(1.0); }
    RigBuilder builder_;

private:
    const machine::Catalogue& cat_;
};

struct RigOptions {
    sessions::Hosting hosting = sessions::Hosting::VirtualTime;
    Ticks program = 50'000;
    std::optional<Ticks> startUp;
    std::optional<std::string> settings;   // the settings file's text after format-version
    sessions::HostServices services;
    const PresetBundle* bundle = nullptr;
    bool screenshots = true;               // a folder for screenshots
    const machine::Catalogue* readCatalogue = nullptr;   // the reader's, when it differs
    bool start = true;
};

class Rig {
public:
    explicit Rig(RigOptions o = {})
        : opts_(o), folder("calcu-presets"), shots("calcu-shots"), machines(mt::testCatalogue()),
          typing(typingProduct, o.hosting) {
        machines.builder_.program = o.program;
        machines.builder_.startUp = o.startUp;
        if (o.settings) {
            const std::string ini = folder / "Startup-Test.ini";
            calcu::testing::writeText(ini, "format-version = 1\n" + *o.settings);
            settings = std::make_unique<settings::SettingsFile>(settings::SettingsFile::open(startup::test::testDecl(), ini));
        } else {
            settings = std::make_unique<settings::SettingsFile>(settings::SettingsFile::defaultsOnly(startup::test::testDecl()));
        }
        PresetsSetup ps;
        ps.catalogue = o.readCatalogue ? o.readCatalogue : &mt::testCatalogue();
        ps.rules = &rules;
        ps.typing = &typing;
        ps.bundle = o.bundle;
        ps.home = (folder / "home");
        if (o.screenshots) ps.screenshotFolder = [this]() -> std::optional<std::string> { return shots.str(); };
        ps.control = &machineControl;
        presets = std::make_unique<PresetsPart>(ps);
        startupPart = std::make_unique<startup::StartupPart>(
            startup::StartupSetup{machineControl, *settings, presets.get()});
        sessions::AppSessionSetup s;
        s.clock = clock.fn();
        s.hostTime = [] { return std::chrono::system_clock::time_point(std::chrono::hours(24 * 365 * 50)); };
        s.parts = {&peripherals, presets.get(), startupPart.get(), &typing};
        s.hosting = o.hosting;
        s.jitterSeed = 1;
        session = std::make_unique<sessions::AppSession>(std::move(s));
        id = session->listen(fe, o.services);
        if (o.start) {
            startCall = send("start");
            callWhileBusy();
        }
    }
    Rig(const Rig&) = delete;
    Rig& operator=(const Rig&) = delete;

    CallId send(const std::string& name, Value args = Value()) {
        return session->send(id, sessions::Command{sessions::CommandName{name}, std::move(args)});
    }
    sessions::ReplyMessage ask(const std::string& name, Value args = Value()) {
        const CallId c = session->ask(id, sessions::Request{sessions::RequestName{name}, std::move(args)});
        return *fe.reply(c);
    }
    // The preset `text` written as `file` in the rig's folder, and loaded.
    std::string write(const std::string& text, const std::string& file = "t.test-seg") {
        folder.write(file, text);
        return folder / file;
    }
    CallId load(const std::string& text, const std::string& file = "t.test-seg", bool buildOnly = false) {
        const std::string path = write(text, file);
        Value args = Value::object({{"ref", path}});
        if (buildOnly) args = Value::object({{"ref", path}, {"buildOnly", true}});
        return send("load-preset", std::move(args));
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
    bool done(CallId c) const { return fe.reply(c) && fe.reply(c)->done; }
    bool replied(CallId c) const { return fe.reply(c) != nullptr; }
    std::string reason(CallId c) const { return fe.reply(c) ? fe.reply(c)->reason : std::string("(no reply)"); }
    mt::TestMachine* machine() { return static_cast<mt::TestMachine*>(session->context().machine()); }
    Ticks ticks() { return machine() ? machine()->tickCount() : 0; }
    std::vector<std::string> logLines() {
        std::vector<std::string> out;
        for (const auto& p : fe.mirror.takeAppended(sessions::kLogTopic))
            if (const auto* l = sessions::payloadAs<sessions::LogLine>(p)) out.push_back(l->text);
        return out;
    }
    std::vector<std::string> notices() { return fe.noticeSentences(); }
    const sessions::BusyState* busy() const { return fe.busy(); }
    // The ticks of the machine's key events of `key` going down.
    std::vector<Ticks> downs(const char* key) {
        std::vector<Ticks> out;
        if (machine())
            for (const auto& e : machine()->keyEvents())
                if (e.down && e.key == machine::KeyId{key}) out.push_back(e.tick);
        return out;
    }

    RigOptions opts_;
    calcu::testing::TempFolder folder, shots;
    sessions::test::ScriptedClock clock;
    sessions::test::TestFrontend fe;
    RigMachines machines;
    std::unique_ptr<settings::SettingsFile> settings;
    TestPresetRules rules;
    RigTyping typingProduct;
    typing::TypingPart typing;
    peripherals::PeripheralsPart peripherals;
    startup::MachineControl machineControl{machines, startup::test::testRoms()};
    std::unique_ptr<PresetsPart> presets;
    std::unique_ptr<startup::StartupPart> startupPart;
    std::unique_ptr<sessions::AppSession> session;
    sessions::ListenerId id = 0;
    CallId startCall = 0;
};

// The reader alone (no session): the library's and the test product's
// keywords, the rig's typing profiles.
struct Reading {
    Reading() {
        addLibraryKeywords(vocabulary);
        rules.addKeywords(vocabulary);
    }
    ReadSetup setup() { return ReadSetup{vocabulary, rules, mt::testCatalogue(), &typingProduct}; }
    // `text` as the content of `name`, in the folder `dir` when given.
    std::variant<LoadPlan, ReadError> read(const std::string& text, const std::string& name = "t.test-seg") {
        auto src = contentSource(text, name, home);
        return readPreset(*src, setup());
    }
    std::variant<LoadPlan, ReadError> readFile(const std::string& path) {
        auto src = folderSource(std::filesystem::path(std::u8string(path.begin(), path.end())), home);
        return readPreset(*src, setup());
    }
    // The error's message, or "" when the preset was read.
    std::string error(const std::string& text) {
        auto r = read(text);
        if (auto* e = std::get_if<ReadError>(&r)) return e->message;
        return "";
    }
    Vocabulary vocabulary;
    TestPresetRules rules;
    RigTyping typingProduct;
    std::string home = "/nonexistent-home";
};

// Whether a line of `lines` contains `text`.
inline bool hasLine(const std::vector<std::string>& lines, const std::string& text) {
    for (const auto& l : lines)
        if (l.find(text) != std::string::npos) return true;
    return false;
}

// The header of a preset of the tests on `model`.
inline std::string header(const std::string& model = "test-seg") { return "format-version: 2\nmodel: " + model + "\n"; }

} // namespace calcu::presets::test
