#pragma once
#include "TestRoms.hpp"
#include "../app/KeyIds.hpp"
#include "../app/TIAppPart.hpp"
#include "../app/Settings.hpp"
#include "presets/PresetsPart.hpp"
#include "startup/StartupPart.hpp"
#include "typing/TypingTopics.hpp"
#include "Pump.hpp"
#include "TestFrontend.hpp"
#include "sessions/LibraryTopics.hpp"
#include <chrono>
#include <filesystem>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

// Calc-U-59 on the library's app session as a frontend hosts it (LIB-3 59,
// C59-5; M-34: what SessionController's tests drove through a FakePlatform):
// the product's session (makeAppSession), the library's test frontend
// (topic mirror and the raw messages, library/testing/), a folder of the
// rig's own for the save folder and the traces, and a wall clock of the
// test's own (it moves when the test moves it, and 1 ms at every reading of
// the pacer). No host clock is read.

namespace testroms {

namespace fs = std::filesystem;

/// What a frontend hands the part (C59-5 D-12): the ROM set, the cue cards,
/// the settings (`[startup] model`: `model`, LIB-18 AC-17), the bundled
/// examples, the save folder and the traces in `folder`.
inline cu59::TIAppSetup appSetup(calcu::settings::SettingsFile& settings, cu59::Model model, const fs::path& folder) {
    cu59::TIAppSetup s;
    s.roms = romSet();
    settings.set("startup", "model", cu59::modelId(model).value);
    s.settings = &settings;
    s.examplesFolder = "examples";
    s.saveFolder = folder.string();
    s.trace = {folder, 50'000'000};   // the apps' default limit (50 MiB for the library's trace session)
    return s;
}

/// The files of the save folder, in memory (a FileIO of the test's).
struct MapFileIO final : cu59::FileIO {
    std::map<std::string, std::string> cards;   ///< by path; the constant memory is "cards/ti58c.memory.yaml"
    std::set<std::string> unreadableCards;      ///< paths that are there but can't be read
    std::map<std::string, int> writes;          ///< how often each path was written
    Read read(const std::string& path) override {
        if (unreadableCards.count(path)) return {std::nullopt, true};
        auto it = cards.find(path);
        if (it == cards.end()) return {};
        return {std::vector<uint8_t>(it->second.begin(), it->second.end())};
    }
    bool write(const std::string& path, const std::string& text) override {
        cards[path] = text;
        ++writes[path];
        return true;
    }
};

struct RigOptions {
    cu59::Model model = cu59::Model::TI59;
    calcu::sessions::HostServices services;
    std::vector<calcu::sessions::SessionPart*> before;   ///< parts in front of the product's
    std::vector<calcu::sessions::SessionPart*> after;    ///< parts behind it
    std::shared_ptr<cu59::FileIO> fileIO;                ///< null: plain files in the rig's folder
    std::string saveFolder;                              ///< empty: the rig's folder
    fs::path traceFolder;                                ///< empty: the rig's folder
    bool noTraceFolder = false;                          ///< the Web: none, traces handed over (LIB-10 AC-26)
    std::uint64_t traceMaxBytes = 0;                     ///< 0: the apps' default
    calcu::settings::SettingsFile* settings = nullptr;   ///< null: the rig's own (defaults, in memory)
    std::string examplesFolder;                          ///< empty: examples/
    bool start = true;                                   ///< makeAppSession sends LIB-18's start (the Web's page sends its own)
};

struct SessionRig {
    using Value = calcu::sessions::Value;
    calcu::testing::TempFolder folder{"calcu59-session"};
    calcu::pacing::WallTime now{std::chrono::hours(1)};
    calcu::settings::SettingsFile settings = calcu::settings::SettingsFile::defaultsOnly(cu59::settings::settingsDecl());
    calcu::sessions::test::TestFrontend fe;
    calcu::sessions::ProductSession<cu59::TIAppPart> app;
    calcu::sessions::ListenerId me = 0;

    explicit SessionRig(RigOptions o) {
        cu59::TIAppSetup s = appSetup(o.settings ? *o.settings : settings, o.model, folder.path());
        if (!o.examplesFolder.empty()) s.examplesFolder = o.examplesFolder;
        if (!o.saveFolder.empty()) s.saveFolder = o.saveFolder;
        if (!o.traceFolder.empty()) s.trace.dir = o.traceFolder;
        if (o.noTraceFolder) s.trace.dir.clear();
        if (o.traceMaxBytes) s.trace.maxBytes = o.traceMaxBytes;
        s.fileIO = std::move(o.fileIO);
        app = cu59::makeAppSession(std::move(s), [this] { return now += std::chrono::milliseconds(1); }, 59u,
                                   std::move(o.before), std::move(o.after), o.start);
        me = session().listen(fe, std::move(o.services));
        // LIB-18's start (makeAppSession sent it): until the machine is ready for keys.
        if (o.start) until([this] { return session().machineSession().machine() && !session().context().busy(); });
    }
    explicit SessionRig(cu59::Model model = cu59::Model::TI59, calcu::sessions::HostServices services = {},
                        std::vector<calcu::sessions::SessionPart*> before = {})
        : SessionRig(RigOptions{.model = model, .services = std::move(services), .before = std::move(before)}) {}

    calcu::sessions::AppSession& session() { return *app.session; }
    cu59::TIAppPart& part() { return *app.part; }
    calcu::machine::Machine& machine() { return *session().machineSession().machine(); }

    /// A command as a frontend sends it.
    calcu::sessions::CallId post(const calcu::sessions::CommandName& name, Value args = Value::object({})) {
        return session().send(me, calcu::sessions::Command{name, std::move(args)});
    }
    /// The reply of `id`, once it came.
    const calcu::sessions::ReplyMessage* reply(calcu::sessions::CallId id) const { return fe.reply(id); }
    calcu::sessions::ReplyMessage send(const calcu::sessions::CommandName& name, Value args = Value::object({})) {
        const auto id = post(name, std::move(args));
        if (const auto* r = reply(id)) return *r;
        return calcu::sessions::ReplyMessage{id, false, "no reply yet", nullptr};
    }
    /// A command that may start an operation (LIB-18's model, reset, attach,
    /// …; LIB-6's run-step): sent, then host calls until its reply came, at
    /// most `limit` of wall time.
    calcu::sessions::ReplyMessage operation(const calcu::sessions::CommandName& name, Value args = Value::object({}),
                                            std::chrono::milliseconds limit = std::chrono::minutes(2)) {
        const auto id = post(name, std::move(args));
        until([&] { return reply(id) != nullptr; }, limit);
        if (const auto* r = reply(id)) return *r;
        return calcu::sessions::ReplyMessage{id, false, "no reply", nullptr};
    }
    /// One preset step on the running machine (LIB-6's run-step): `step("key", "1 2nd Prt")`.
    calcu::sessions::ReplyMessage step(const std::string& verb, const std::string& value = {},
                                       std::chrono::milliseconds limit = std::chrono::minutes(2)) {
        return operation(calcu::presets::kRunStepCommand, Value::object({{"verb", verb}, {"value", value}}), limit);
    }
    /// LIB-18's commands (LIB-18 §11.2: the library's start, model, resets, attach, detach).
    calcu::sessions::ReplyMessage model(const std::string& name) {
        return operation(calcu::startup::kModelCommand, Value::object({{"name", name}}));
    }
    calcu::sessions::ReplyMessage attach(const std::string& place, const std::string& peripheral) {
        return operation(calcu::startup::kAttachCommand, Value::object({{"place", place}, {"peripheral", peripheral}}));
    }
    calcu::sessions::ReplyMessage detach(const std::string& place) {
        return operation(calcu::startup::kDetachCommand, Value::object({{"place", place}}));
    }
    calcu::sessions::ReplyMessage reset() { return operation(calcu::startup::kResetCommand); }
    calcu::sessions::ReplyMessage coldBoot() { return operation(calcu::startup::kColdBootCommand); }

    /// One host call, a 60th of a second after the last.
    void frame() { calcu::testing::pumpFrame(session(), now); }
    void runFor(std::chrono::milliseconds d) { calcu::testing::pumpFor(session(), now, d); }
    /// Host calls until `cond` holds, at most `limit` (20 s) of wall time.
    template <class F> bool until(F cond, std::chrono::milliseconds limit = std::chrono::seconds(20)) {
        return calcu::testing::pumpUntil(session(), now, cond, limit);
    }
    template <class P> const P* state(const calcu::sessions::TopicName& t) const { return fe.state<P>(t); }
    bool busy() const {
        const auto* b = fe.busy();
        return b && b->operationRuns;
    }
    /// Every notice so far, in order.
    std::vector<std::string> notices() const {
        std::vector<std::string> out;
        for (const auto& m : fe.log)
            if (const auto* s = std::get_if<calcu::sessions::StreamMessage>(&m); s && s->topic == calcu::sessions::kNoticesTopic)
                for (const auto& n : s->appended)
                    if (const auto* x = calcu::sessions::payloadAs<calcu::sessions::Notice>(n)) out.push_back(x->sentence);
        return out;
    }
    /// KEYSTROKES DebugPanel: (c59.show-debug-panel), in order.
    std::vector<cu59::DebugPanel> panels() const {
        std::vector<cu59::DebugPanel> out;
        for (const auto& m : fe.log)
            if (const auto* s = std::get_if<calcu::sessions::StreamMessage>(&m); s && s->topic == cu59::topics::kShowDebugPanel)
                for (const auto& n : s->appended)
                    if (const auto* x = calcu::sessions::payloadAs<cu59::TIShowDebugPanel>(n)) out.push_back(x->panel);
        return out;
    }
    /// Every state topic sent from position `from` of the log on, in order.
    std::vector<calcu::sessions::TopicName> stateSets(std::size_t from = 0) const {
        std::vector<calcu::sessions::TopicName> out;
        for (std::size_t i = from; i < fe.log.size(); ++i)
            if (const auto* s = std::get_if<calcu::sessions::StateMessage>(&fe.log[i])) out.push_back(s->topic);
        return out;
    }

    /// A key tapped as the on-screen keyboard does (the library's
    /// screen-key-down / -up, LIB-5; 100 ms held at least).
    void tap(int code) {
        const std::string key = cu59::keyIdAt(static_cast<uint8_t>(code)).value_or(calcu::machine::KeyId{}).value;
        post(calcu::typing::kScreenKeyDown, Value::object({{"key", key}, {"pointer", 1}}));
        runFor(std::chrono::milliseconds(150));
        post(calcu::typing::kScreenKeyUp, Value::object({{"key", key}, {"pointer", 1}}));
        runFor(std::chrono::milliseconds(150));
    }
    /// The PC-100C's paper as the PaperPart keeps it (LIB-8; empty paper: none).
    const calcu::paper::Paper* pc100cPaper() const {
        for (const auto& [owner, paper] : app.part->paperPart()->papers())
            if (owner == "PC-100C") return paper;
        return nullptr;
    }
    /// Its text lines (the printout).
    std::vector<std::string> printedText() const {
        std::vector<std::string> out;
        if (const auto* p = pc100cPaper())
            for (const auto& l : p->text()) out.push_back(l.text);
        return out;
    }
    Value bytes(const std::string& s) { return Value(Value::Bytes(s.begin(), s.end())); }
    calcu::machine::Ticks ticks() { return machine().tickCount(); }
    double seconds(calcu::machine::Ticks t) { return double(t) / machine().descriptor().tickRate; }
};

/// The product's flows with the card and constant-memory files in memory:
/// the save folder is "cards", so a preset's card "X" is "cards/X". The
/// flows are called directly, as the commands call them; errors are the
/// session's notices.
struct PartRig : SessionRig {
    std::shared_ptr<MapFileIO> io;
    MapFileIO& p;
    explicit PartRig(cu59::Model model = cu59::Model::TI59, std::shared_ptr<MapFileIO> files = std::make_shared<MapFileIO>())
        : SessionRig(RigOptions{.model = model, .fileIO = files, .saveFolder = "cards"}), io(std::move(files)), p(*io) {}

    /// The notices so far (what a direct call said goes out at the next call).
    const std::vector<std::string>& errors() {
        session().call(now);
        seen = notices();
        return seen;
    }
    std::vector<std::string> seen;   ///< errors()'s last answer
    void run(std::chrono::milliseconds d) { runFor(d); }
    template <class F> bool runUntil(F cond, std::chrono::milliseconds limit = std::chrono::seconds(20)) {
        return until(cond, limit);
    }
    /// A preset as a frontend loads one by its content (LIB-6's
    /// `load-preset {content, name}`); the reply of the command (refused at
    /// once with the first error, else when the preset has been carried out).
    calcu::sessions::ReplyMessage load(const std::string& text, const std::string& name = "test.ti59") {
        const auto id = post(calcu::presets::kLoadPresetCommand, Value::object({{"content", text}, {"name", name}}));
        until([&] { return reply(id) != nullptr; }, std::chrono::minutes(30));
        if (const auto* r = reply(id)) return *r;
        return calcu::sessions::ReplyMessage{id, false, "no reply", nullptr};
    }
};

} // namespace testroms
