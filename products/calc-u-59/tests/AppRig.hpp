#pragma once
#include "../app/Settings.hpp"
#include "../app/TIAppPart.hpp"
#include "../app/TIPresetKeys.hpp"
#include "Pump.hpp"
#include "TempFolder.hpp"
#include "TestFrontend.hpp"
#include "TestRoms.hpp"
#include "presets/PresetsPart.hpp"
#include "sessions/LibraryTopics.hpp"
#include "startup/StartupPart.hpp"
#include "typing/TypingTopics.hpp"
#include <chrono>
#include <functional>
#include <map>
#include <memory>
#include <string>

// The app session as a native frontend hosts it, for the M-54 tests of
// LIB-18 59 and C59-3 (tests/machine_control_tests.cpp,
// tests/constant_memory_tests.cpp): the settings set up before the start
// (`configure`), a save folder with a FileIO the test can make fail, a wall
// clock of the test's own (1 ms per reading), and LIB-18's `start` sent once
// after the rig listens (its notices are seen). Unlike SessionRig it can be made again on the same save
// folder, which a "quit and start again" needs.

namespace apprig {

using Value = calcu::sessions::Value;

/// The save folder in memory; `failWrites` makes every write fail (C59-3 EC-4).
struct Files final : cu59::FileIO {
    std::map<std::string, std::string> files;
    std::map<std::string, int> writes;
    bool failWrites = false;
    Read read(const std::string& path) override {
        auto it = files.find(path);
        if (it == files.end()) return {};
        return {std::vector<uint8_t>(it->second.begin(), it->second.end())};
    }
    bool write(const std::string& path, const std::string& text) override {
        if (failWrites) return false;
        files[path] = text;
        ++writes[path];
        return true;
    }
};

inline const std::string kMemoryPath = "save/ti58c.memory.yaml";

struct Rig {
    calcu::settings::SettingsFile settings = calcu::settings::SettingsFile::defaultsOnly(cu59::settings::settingsDecl());
    calcu::pacing::WallTime now{std::chrono::hours(1)};
    calcu::sessions::test::TestFrontend fe;
    calcu::sessions::ProductSession<cu59::TIAppPart> app;
    calcu::sessions::ListenerId me = 0;
    std::shared_ptr<Files> files;

    explicit Rig(std::string model = "TI-59", std::shared_ptr<Files> f = std::make_shared<Files>(),
                 const std::function<void(calcu::settings::SettingsFile&)>& configure = {})
        : files(std::move(f)) {
        if (!model.empty()) settings.set("startup", "model", model);
        if (configure) configure(settings);
        cu59::TIAppSetup s;
        s.roms = testroms::romSet();
        s.cuecards = testroms::readFile("roms/solid-state/cuecards.txt").value_or("");
        s.settings = &settings;
        s.examplesFolder = "examples";
        s.saveFolder = "save";
        s.fileIO = files;
        app = cu59::makeAppSession(std::move(s), [this] { return now += std::chrono::milliseconds(1); }, 59u, {}, {},
                                   /*start=*/false);
        me = session().listen(fe, {});
        send(calcu::sessions::kStartCommand);   // LIB-18's start, once, as a host sends it
    }
    ~Rig() {
        if (app.session) session().close();
    }
    calcu::sessions::AppSession& session() { return *app.session; }
    cu59::TIAppPart& part() { return *app.part; }
    calcu::machine::Machine* machine() { return session().machineSession().machine(); }
    template <class F> bool until(F cond, std::chrono::milliseconds limit = std::chrono::seconds(60)) {
        return calcu::testing::pumpUntil(session(), now, cond, limit);
    }
    void runFor(std::chrono::milliseconds d) { calcu::testing::pumpFor(session(), now, d); }
    /// Until no operation runs and a machine is there.
    void settle() { until([this] { return machine() && !session().context().busy(); }); }
    /// A command; its reply once it came (an operation: at its end).
    calcu::sessions::ReplyMessage send(const calcu::sessions::CommandName& name, Value args = Value::object({})) {
        const auto id = session().send(me, calcu::sessions::Command{name, std::move(args)});
        until([&] { return fe.reply(id) != nullptr; }, std::chrono::minutes(10));
        settle();
        if (const auto* r = fe.reply(id)) return *r;
        return calcu::sessions::ReplyMessage{id, false, "no reply", nullptr};
    }
    calcu::sessions::ReplyMessage load(const std::string& text, const std::string& name = "t.ti59") {
        return send(calcu::presets::kLoadPresetCommand, Value::object({{"content", text}, {"name", name}}));
    }
    std::vector<std::string> notices() const {
        std::vector<std::string> out;
        for (const auto& m : fe.log)
            if (const auto* s = std::get_if<calcu::sessions::StreamMessage>(&m); s && s->topic == calcu::sessions::kNoticesTopic)
                for (const auto& n : s->appended)
                    if (const auto* x = calcu::sessions::payloadAs<calcu::sessions::Notice>(n)) out.push_back(x->sentence);
        return out;
    }
    std::string plugged(const char* place) {
        if (!machine()) return {};
        for (const auto& e : machine()->plugged().byPlace)
            if (e.place.steps.size() == 1 && e.place.steps[0].value == place) return e.peripheral.value;
        return {};
    }
    cu59::ProgramMemory& memory() { return *cu59::programMemoryOf(*machine()); }
    /// Keys as the on-screen keyboard presses them, each held 150 ms.
    void keys(std::initializer_list<const char*> ids) {
        for (const char* k : ids) {
            session().send(me, calcu::sessions::Command{calcu::typing::kScreenKeyDown, Value::object({{"key", k}, {"pointer", 1}})});
            runFor(std::chrono::milliseconds(150));
            session().send(me, calcu::sessions::Command{calcu::typing::kScreenKeyUp, Value::object({{"key", k}, {"pointer", 1}})});
            runFor(std::chrono::milliseconds(200));
        }
    }
};

inline Value attach(const char* place, const char* peripheral) {
    return Value::object({{"place", place}, {"peripheral", peripheral}});
}
inline Value detach(const char* place) { return Value::object({{"place", place}}); }

} // namespace apprig
