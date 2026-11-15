#pragma once
#include "TestRoms.hpp"
#include "../app/TIAppPart.hpp"
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

inline std::string catalog() { return readFile("roms/solid-state/cuecards.txt").value_or(""); }

/// What a frontend hands the part (C59-5 D-12): the ROM set, the cue cards,
/// `model` first, the save folder and the traces in `folder`.
inline cu59::TIAppSetup appSetup(cu59::Model model, const fs::path& folder) {
    cu59::TIAppSetup s;
    s.roms = romSet();
    s.cuecards = catalog();
    s.startModel = model;
    s.saveFolder = folder.string();
    s.trace = {folder, 1'000'000};
    return s;
}

/// The files of the save folder, in memory (a FileIO of the test's).
struct MapFileIO final : cu59::FileIO {
    std::map<std::string, std::string> cards;   ///< by path; ti58c.mem is "cards/ti58c.mem"
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
};

struct SessionRig {
    using Value = calcu::sessions::Value;
    calcu::testing::TempFolder folder{"calcu59-session"};
    calcu::pacing::WallTime now{std::chrono::hours(1)};
    calcu::sessions::test::TestFrontend fe;
    calcu::sessions::ProductSession<cu59::TIAppPart> app;
    calcu::sessions::ListenerId me = 0;

    explicit SessionRig(RigOptions o) {
        cu59::TIAppSetup s = appSetup(o.model, folder.path());
        if (!o.saveFolder.empty()) s.saveFolder = o.saveFolder;
        if (!o.traceFolder.empty()) s.trace.dir = o.traceFolder;
        s.fileIO = std::move(o.fileIO);
        app = cu59::makeAppSession(std::move(s), [this] { return now += std::chrono::milliseconds(1); }, 59u,
                                   std::move(o.before), std::move(o.after));
        me = session().listen(fe, std::move(o.services));
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

    /// A key tapped as the on-screen keyboard does (100 ms held at least).
    void tap(int code) {
        post(cu59::commands::kKeyboardDown, Value::object({{"codes", Value(Value::Bytes{static_cast<uint8_t>(code)})}}));
        runFor(std::chrono::milliseconds(150));
        post(cu59::commands::kKeyboardUp);
        runFor(std::chrono::milliseconds(150));
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
    /// A preset as the Preset button loads it (the flow, not the operation).
    void load(const std::string& text) { part().loadStateFile(std::vector<uint8_t>(text.begin(), text.end())); }
};

} // namespace testroms
