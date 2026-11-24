#pragma once
#include "Files.hpp"
#include "TestParts.hpp"
#include "TempFolder.hpp"
#include "TestFrontend.hpp"
#include "TestKinds.hpp"
#include "TestMachines.hpp"
#include "TestPresetRules.hpp"
#include "TestProductMachines.hpp"
#include "control/ControlPart.hpp"
#include "formats/Flow.hpp"
#include "media/MediaPart.hpp"
#include "media/PresetMedia.hpp"
#include "peripherals/PeripheralsPart.hpp"
#include "presets/PresetsPart.hpp"
#include "sessions/AppSession.hpp"
#include "sessions/LibraryTopics.hpp"
#include "settings/Paths.hpp"
#include "settings/SettingsFile.hpp"
#include "startup/StartupPart.hpp"
#include <chrono>
#include <memory>
#include <optional>
#include <string>
#include <vector>

// The window of the media tests: an app session with PeripheralsPart,
// PresetsPart, StartupPart, ControlPart and MediaPart, in that order; the
// media test catalogue (the test machines with their media places and
// test-memory-module); the kinds of TestKinds.hpp with a bundled folder for
// disks and modules; a save folder of the test's own; the sessions' test
// frontend; the startup's test settings. In virtual time unless a case says
// Paced (host calls of 16 ms on the scripted clock). The preset header key
// `test-cards:` defines in-file test cards:
//   test-cards:
//     - { name: A, content: abc }

namespace calcu::media::test {

using machine::Ticks;
using sessions::CallId;
using sessions::Value;
using namespace std::chrono_literals;

// The test builder with the media places on, and a hook on every machine built.
struct FixtureBuilder final : machine::MachineBuilder {
    mt::TestBuilder inner;
    std::function<void(mt::TestMachine&)> onBuild;
    machine::BuildResult build(const machine::BuildRequest& r, const machine::Catalogue& c) override {
        machine::BuildResult b = inner.build(r, c);
        if (b.machine && onBuild) onBuild(static_cast<mt::TestMachine&>(*b.machine));
        return b;
    }
};

class FixtureMachines final : public startup::ProductMachines {
public:
    FixtureMachines() { builder_.inner.media = true; }
    const machine::Catalogue& catalogue() const override { return mt::mediaTestCatalogue(); }
    machine::MachineBuilder& builder() override { return builder_; }
    startup::MachinePlan builtInDefault(const machine::ModelId& m) override {
        startup::MachinePlan p{m, {}, {}, nullptr};
        if (defaults) defaults(p);
        return p;
    }
    pacing::PacingProfile pacingProfile(const machine::ModelId&) const override { return {}; }
    int soundRate(const machine::ModelId& m) const override { return m == mt::kDot ? 8000 : 0; }
    machine::Seconds readinessBound(const machine::ModelId&) const override { return machine::Seconds(1.0); }
    std::function<void(startup::MachinePlan&)> defaults;
    FixtureBuilder builder_;
};

class MediaFixture;

struct FixtureOptions {
    sessions::Hosting hosting = sessions::Hosting::VirtualTime;
    std::string model = "test-seg";
    std::function<void(startup::MachinePlan&)> defaults;   // the built-in default machine
    std::function<void(calcu::testing::TempFolder& save, calcu::testing::TempFolder& bundled)> prepare;   // before the start
    sessions::HostServices services;
    bool saveFolderMissing = false;   // the save folder is not made before
    ModuleListing listing;
    // The settings file the session reads (empty: defaults
    // only), and parts after MediaPart, made once the fixture's parts are.
    std::string settingsIni;
    std::function<std::vector<sessions::SessionPart*>(MediaFixture&)> moreParts;
    // The kinds changed before the part is made (what Calc-U-59's magnetic
    // card needs), and the host's file access.
    std::function<void(std::vector<KindDecl>&)> kinds;
    std::shared_ptr<const FileAccess> files;
    bool start = true;   // false: no machine until the case sends `start`
};

class MediaFixture {
public:
    explicit MediaFixture(FixtureOptions o = {})
        : saveRoot("calcu-media-save"), bundled("calcu-media-bundled"), presetsDir("calcu-media-presets") {
        machines.defaults = o.defaults;
        saveFolder = o.saveFolderMissing ? saveRoot / "not-yet" : saveRoot.str();
        settings = std::make_unique<settings::SettingsFile>(
            o.settingsIni.empty() ? settings::SettingsFile::defaultsOnly(startup::test::testDecl())
                                  : settings::SettingsFile::open(startup::test::testDecl(), o.settingsIni));
        settings::HostDirs dirs;
        dirs.frontend = settings::FrontendKind::Given;   // the save folder as passed
        dirs.home = saveRoot / "home";
        dirs.settingsBase = saveRoot / "settings";
        dirs.saveFolder = saveFolder;
        paths = std::make_unique<settings::Paths>(*settings, dirs);
        bundled.write("disks/.keep", "");
        bundled.write("modules/.keep", "");
        if (o.prepare) o.prepare(saveRoot, bundled);
        TestKindFolders folders{bundled / "disks", bundled / "modules", ""};
        MediaSetup ms;
        ms.kinds = testKinds(folders);
        if (o.kinds) o.kinds(ms.kinds);
        ms.files = o.files;
        ms.paths = paths.get();
        ms.catalogue = &mt::mediaTestCatalogue();
        ms.listing = o.listing;
        ms.control = &machineControl;
        media = std::make_unique<MediaPart>(std::move(ms));
        presets::PresetsSetup ps;
        ps.catalogue = &mt::mediaTestCatalogue();
        ps.rules = &rules;
        ps.home = saveRoot / "home";
        ps.control = &machineControl;
        presetsPart = std::make_unique<presets::PresetsPart>(ps);
        startupPart = std::make_unique<startup::StartupPart>(
            startup::StartupSetup{machineControl, *settings, presetsPart.get()});
        addMediaToPresets(presetsPart->vocabulary(), *media);
        MediaPart* part = media.get();
        presetsPart->vocabulary().add(presets::HeaderKeyDef{
            "test-cards",
            [part](const formats::YamlNode& v, int line, presets::ReadContext& ctx) {
                if (!v.isSeq()) {
                    ctx.error(line, "test-cards: is a list.");
                    return;
                }
                for (const auto& item : v.seq) {
                    const formats::YamlNode* n = item.find("name");
                    const formats::YamlNode* c = item.find("content");
                    if (!n) {
                        ctx.error(item.line, "A test card has a name.");
                        continue;
                    }
                    defineInFile(ctx, *part->kind("test-card"), formats::scalarText(*n),
                                 contentFrom(c ? formats::scalarText(*c) : std::string()));
                }
            }});
        control = std::make_unique<control::ControlPart>(control::ControlSetup{&mt::mediaTestCatalogue(), {}, nullptr});
        sessions::AppSessionSetup s;
        s.clock = clock.fn();
        s.hostTime = [] { return std::chrono::system_clock::time_point(std::chrono::hours(24 * 365 * 50)); };
        s.appLog = &appLog;
        s.parts = {&peripherals, presetsPart.get(), startupPart.get(), control.get(), media.get()};
        if (o.moreParts)
            for (sessions::SessionPart* p : o.moreParts(*this)) s.parts.push_back(p);
        s.hosting = o.hosting;
        s.jitterSeed = 1;
        session = std::make_unique<sessions::AppSession>(std::move(s));
        id = session->listen(fe, o.services);
        if (!o.start) return;
        startCall = send("start", Value::object({{"model", o.model}}));
        callWhileBusy();
    }
    MediaFixture(const MediaFixture&) = delete;
    MediaFixture& operator=(const MediaFixture&) = delete;

    CallId send(const std::string& name, Value args = Value()) {
        return session->send(id, sessions::Command{sessions::CommandName{name}, std::move(args)});
    }
    bool ok(const std::string& name, Value args = Value()) {
        const CallId c = send(name, std::move(args));
        callWhileBusy();
        return done(c);
    }
    std::string refusal(const std::string& name, Value args = Value()) {
        const CallId c = send(name, std::move(args));
        callWhileBusy();
        return reason(c);
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
    // A preset in the presets folder, loaded.
    std::string writePreset(const std::string& text, const std::string& file = "t.test-seg") {
        presetsDir.write(file, text);
        return presetsDir / file;
    }
    CallId load(const std::string& text, const std::string& file = "t.test-seg") {
        const std::string path = writePreset(text, file);
        const CallId c = send("load-preset", Value::object({{"ref", path}}));
        callWhileBusy();
        return c;
    }
    std::string readError(const std::string& text) {
        const std::string path = writePreset(text, "check.test-seg");
        const CallId c = session->ask(id, sessions::Request{sessions::RequestName{"check-preset"}, Value::object({{"ref", path}})});
        const auto* r = fe.reply(c);
        return r && !r->done ? r->reason : std::string();
    }
    // A step on the running machine (run-step, as the harness sends it).
    CallId step(const std::string& verb, const std::string& value) {
        const CallId c = send("run-step", Value::object({{"verb", verb}, {"value", value}}));
        callWhileBusy();
        return c;
    }
    bool done(CallId c) const { return fe.reply(c) && fe.reply(c)->done; }
    std::string reason(CallId c) const { return fe.reply(c) ? fe.reply(c)->reason : std::string("(no reply)"); }
    mt::TestMachine* machine() { return static_cast<mt::TestMachine*>(session->context().machine()); }
    mt::TestMediaPlace* reader() { return machine()->mediaPlace("reader"); }
    mt::TestMediaPlace* drive() { return machine()->mediaPlace("drive"); }
    mt::TestModuleMedium* module(const char* slot = "slot-1") { return machine()->moduleAt(startup::test::at({slot})); }
    const MediaState* state() const { return fe.state<MediaState>(kMediaTopic); }
    const PlaceState* place(const std::string& word) const { return state() ? state()->place(word) : nullptr; }
    std::vector<std::string> logLines() {
        std::vector<std::string> out;
        for (const auto& p : fe.mirror.takeAppended(sessions::kLogTopic))
            if (const auto* l = sessions::payloadAs<sessions::LogLine>(p)) out.push_back(l->text);
        return out;
    }
    // The app log's lines since the last call, every level.
    std::vector<std::string> appLogLines() {
        std::vector<std::string> out;
        for (const auto& l : appLog.lines()) out.push_back(l.text);
        appLog.clear();
        return out;
    }
    std::vector<std::string> notices() { return fe.noticeSentences(); }
    std::string saved(const std::string& file) const { return calcu::testing::readText(saveRoot.path() / file); }
    bool exists(const std::string& file) const { return std::filesystem::exists(saveRoot.path() / file); }
    void writeSaved(const std::string& file, const std::string& text) const { saveRoot.write(file, text); }
    // A file of a test kind in the save folder.
    void saveFile(const std::string& file, TestFile t) const { saveRoot.write(file, t.text()); }

    calcu::testing::TempFolder saveRoot, bundled, presetsDir;
    std::string saveFolder;
    sessions::test::ScriptedClock clock;
    applog::MemoryAppLog appLog{applog::AppLogLevel::Debug};   // the session's app log, every level
    sessions::test::TestFrontend fe;
    FixtureMachines machines;
    std::unique_ptr<settings::SettingsFile> settings;
    std::unique_ptr<settings::Paths> paths;
    presets::test::TestPresetRules rules;
    peripherals::PeripheralsPart peripherals;
    startup::MachineControl machineControl{machines, startup::test::testRoms()};
    std::unique_ptr<presets::PresetsPart> presetsPart;
    std::unique_ptr<startup::StartupPart> startupPart;
    std::unique_ptr<control::ControlPart> control;
    std::unique_ptr<MediaPart> media;
    std::unique_ptr<sessions::AppSession> session;
    sessions::ListenerId id = 0;
    CallId startCall = 0;
};

// Whether a line of `lines` contains `text`.
inline bool hasLine(const std::vector<std::string>& lines, const std::string& text) {
    for (const auto& l : lines)
        if (l.find(text) != std::string::npos) return true;
    return false;
}

inline std::string header(const std::string& model = "test-seg") { return "format-version: 2\nmodel: " + model + "\n"; }

inline TestFile disk(std::string name, std::string content, bool isTemplate = false) {
    TestFile t;
    t.format = "test-disk";
    t.name = std::move(name);
    t.content = std::move(content);
    t.isTemplate = isTemplate;
    return t;
}
inline TestFile card(std::string name, std::string content, bool isTemplate = false) {
    TestFile t = disk(std::move(name), std::move(content), isTemplate);
    t.format = "test-card";
    return t;
}
inline TestFile moduleFile(std::string name, std::string content, bool isTemplate = false, bool protect = false) {
    TestFile t = disk(std::move(name), std::move(content), isTemplate);
    t.format = "test-module";
    t.writeProtect = protect;
    return t;
}
// The value of a TestFile's line `key: `, "" when it has none.
inline std::string field(const std::string& text, const std::string& key) {
    const std::string k = key + ": ";
    std::size_t at = text.rfind("\n" + k);
    if (at == std::string::npos) {
        if (text.rfind(k, 0) != 0) return {};
        at = 0;
    } else {
        ++at;
    }
    const std::size_t end = text.find('\n', at);
    return text.substr(at + k.size(), end == std::string::npos ? std::string::npos : end - at - k.size());
}

} // namespace calcu::media::test
