#pragma once
#include "Files.hpp"
#include "TestMachines.hpp"
#include "TestParts.hpp"
#include "TestRomFiles.hpp"
#include "peripherals/PeripheralsPart.hpp"
#include "roms/FullSet.hpp"
#include "roms/Lookup.hpp"
#include "sessions/AppSession.hpp"
#include "settings/Declaration.hpp"
#include "settings/SettingsFile.hpp"
#include "startup/ProductMachines.hpp"
#include "startup/StartupPart.hpp"
#include <chrono>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

// The startup tests' product: ProductMachines over the test
// catalogue (library/machine/tests/TestMachines.hpp) with a build counter, a
// settable readiness bound and a built-in default per test model; the
// settings declared for it; and the rig the startup tests share: an app
// session with PeripheralsPart, StartupPart and the sessions' test part
// (for an operation that runs), a test frontend listening.

namespace calcu::startup::test {

namespace mt = calcu::machine::test;
using machine::ModelId;
using machine::PeripheralRequest;
using machine::PlacePath;
using machine::Ticks;
using machine::VariantId;
using mt::TestMachine;
using sessions::CallId;
using sessions::Value;

inline PlacePath at(std::initializer_list<const char*> steps) {
    PlacePath p;
    for (const char* s : steps) p.steps.push_back(machine::PlaceName{s});
    return p;
}

inline PeripheralRequest plug(const char* place, const machine::PeripheralId& id,
                              std::optional<VariantId> variant = std::nullopt) {
    PeripheralRequest r;
    r.place = at({place});
    r.peripheral = id;
    r.variant = std::move(variant);
    return r;
}

class TestProductMachines final : public ProductMachines {
public:
    TestProductMachines() {
        // The built-in defaults of the tests: test-seg with the printer in
        // `printer`, test-dot bare.
        defaults[mt::kSeg] = MachinePlan{mt::kSeg, std::nullopt, {plug("printer", mt::kPrinter)}, nullptr};
        defaults[mt::kDot] = MachinePlan{mt::kDot, std::nullopt, {}, nullptr};
    }
    const machine::Catalogue& catalogue() const override { return mt::testCatalogue(); }
    machine::MachineBuilder& builder() override { return builder_; }
    MachinePlan builtInDefault(const ModelId& m) override {
        ++defaultsAsked;
        return defaults[m];
    }
    pacing::PacingProfile pacingProfile(const ModelId&) const override { return {}; }
    int soundRate(const ModelId& m) const override { return m == mt::kDot ? 8000 : 0; }
    machine::Seconds readinessBound(const ModelId& m) const override {
        const auto it = bounds.find(m);
        return it == bounds.end() ? machine::Seconds(1.0) : it->second;
    }

    int builds() const { return builder_.builds; }

    std::map<ModelId, MachinePlan> defaults;
    std::map<ModelId, machine::Seconds> bounds;
    int defaultsAsked = 0;

private:
    mt::TestBuilder builder_;
};

// The settings of the test product: models test-seg and test-dot, fallback
// test-seg, the startup keys.
inline const settings::ProductDecl& testDecl() {
    static const settings::ProductDecl d = [] {
        settings::ProductDecl x;
        x.productName = "Startup-Test";
        x.models = {"test-seg", "test-dot"};
        x.fallbackModel = "test-seg";
        x.keys = {settings::family::startupModel(), settings::family::startupLastModel(),
                  settings::family::presetFor("test-seg"), settings::family::presetFor("test-dot")};
        return x;
    }();
    return d;
}

// The full ROM set of the test catalogue, through the one lookup.
inline std::shared_ptr<const roms::RomSet> testRoms() {
    static const auto set = [] {
        roms::test::TempFolder dir("calcu-startup-roms");
        const roms::FullSet full = roms::fullSet(mt::testCatalogue());
        roms::test::writeFullSet(dir, full);
        roms::test::TestTextReader reader;
        return roms::lookUp(full, roms::DirectoryBundle(dir.path()), &reader).set;
    }();
    return set;
}

struct RigOptions {
    sessions::Hosting hosting = sessions::Hosting::VirtualTime;
    // The settings file's text after `format-version = 1`; nullopt: defaults only.
    std::optional<std::string> settings;
    std::string formatVersion = "1";   // another: the file is not writable in the run
    std::optional<std::string> iniPath;   // an existing settings file to open instead
    PresetLoader* loader = nullptr;
    bool start = true;                 // send `start` at once
    std::function<void(TestProductMachines&)> prepare;   // before the start
    sessions::SliceHook* hook = nullptr;                  // on the machine session (where the typing engine goes)
};

class Rig {
public:
    explicit Rig(RigOptions o = {}) : folder_("calcu-startup") {
        if (o.iniPath) {
            iniPath = *o.iniPath;
            settings = std::make_unique<settings::SettingsFile>(settings::SettingsFile::open(testDecl(), iniPath));
        } else if (o.settings) {
            iniPath = folder_ / "Startup-Test.ini";
            calcu::testing::writeText(iniPath, "format-version = " + o.formatVersion + "\n" + *o.settings);
            settings = std::make_unique<settings::SettingsFile>(settings::SettingsFile::open(testDecl(), iniPath));
        } else {
            settings = std::make_unique<settings::SettingsFile>(settings::SettingsFile::defaultsOnly(testDecl()));
        }
        startup = std::make_unique<StartupPart>(StartupSetup{control, *settings, o.loader});
        sessions::AppSessionSetup s;
        s.clock = clock.fn();
        s.hostTime = [] { return std::chrono::system_clock::time_point(std::chrono::hours(24 * 365 * 50)); };
        s.parts = {&peripherals, startup.get(), &part};
        s.hosting = o.hosting;
        s.jitterSeed = 1;
        session = std::make_unique<sessions::AppSession>(std::move(s));
        id = session->listen(fe);
        if (o.hook) session->machineSession().addHook(*o.hook);
        if (o.prepare) o.prepare(product);
        if (o.start) startCall = send("start");
    }
    Rig(const Rig&) = delete;
    Rig& operator=(const Rig&) = delete;

    CallId send(const std::string& name, Value args = Value()) {
        return session->send(id, sessions::Command{sessions::CommandName{name}, std::move(args)});
    }
    void call() {
        clock.now += std::chrono::milliseconds(16);
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
    const sessions::ReplyMessage* reply(CallId c) const { return fe.reply(c); }
    bool done(CallId c) const { return fe.reply(c) && fe.reply(c)->done; }
    std::string reason(CallId c) const { return fe.reply(c) ? fe.reply(c)->reason : std::string("(no reply)"); }
    TestMachine* machine() { return static_cast<TestMachine*>(session->context().machine()); }
    std::vector<std::string> logLines() {
        std::vector<std::string> out;
        for (const auto& p : fe.mirror.takeAppended(sessions::kLogTopic))
            if (const auto* l = sessions::payloadAs<sessions::LogLine>(p)) out.push_back(l->text);
        return out;
    }
    std::vector<std::string> notices() { return fe.noticeSentences(); }

    calcu::testing::TempFolder folder_;
    std::string iniPath;
    TestProductMachines product;
    MachineControl control{product, testRoms()};
    std::unique_ptr<settings::SettingsFile> settings;
    peripherals::PeripheralsPart peripherals;
    sessions::test::TestPart part;
    std::unique_ptr<StartupPart> startup;
    sessions::test::ScriptedClock clock;
    sessions::test::TestFrontend fe;
    std::unique_ptr<sessions::AppSession> session;
    sessions::ListenerId id = 0;
    CallId startCall = 0;
};

} // namespace calcu::startup::test
