#pragma once
#include "TestRoms.hpp"
#include "../app/TIAppPart.hpp"
#include "sessions/LibraryTopics.hpp"
#include <chrono>
#include <filesystem>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

// The product's flows on the library's app session (M-34: what
// SessionController's tests drove through a FakePlatform). The part's flows
// are called directly, as the commands call them, and the session is called
// at 60 Hz on a clock of the test's own. The card and constant-memory files
// are a map (a FileIO of the test's): the save folder is "cards", so a
// preset's card "X" is "cards/X". Errors are the session's notices.

namespace testroms {

/// The files of the save folder, in memory.
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

struct PartRig : calcu::sessions::MessageSink {
    std::shared_ptr<MapFileIO> io = std::make_shared<MapFileIO>();
    MapFileIO& p = *io;
    calcu::pacing::WallTime now{std::chrono::hours(1)};
    calcu::sessions::ProductSession<cu59::TIAppPart> app;
    std::vector<std::string> notices;
    std::vector<cu59::DebugPanel> panels;                 ///< KEYSTROKES DebugPanel: (c59.show-debug-panel)
    std::vector<calcu::sessions::TopicName> stateSets;    ///< every state topic sent, in order

    explicit PartRig(cu59::Model model = cu59::Model::TI59) {
        cu59::TIAppSetup s;
        s.roms = romSet();
        s.cuecards = readFile("roms/solid-state/cuecards.txt").value_or("");
        s.startModel = model;
        s.saveFolder = "cards";
        s.fileIO = io;
        s.trace = {std::filesystem::temp_directory_path(), 1'000'000};
        app = cu59::makeAppSession(std::move(s), [this] { return now += std::chrono::milliseconds(1); }, 59u);
        me = app.session->listen(*this, {});
    }
    calcu::sessions::ListenerId me = 0;
    /// A command as a frontend sends it.
    calcu::sessions::CallId post(const calcu::sessions::CommandName& name,
                                 calcu::sessions::Value args = calcu::sessions::Value::object({})) {
        return app.session->send(me, calcu::sessions::Command{name, std::move(args)});
    }
    bool busy() const {
        const auto* b = busyState.get();
        return b && b->operationRuns;
    }
    std::shared_ptr<const calcu::sessions::BusyState> busyState;
    cu59::TIAppPart& part() { return *app.part; }
    calcu::machine::Machine& machine() { return *app.session->machineSession().machine(); }

    void deliver(const calcu::sessions::Message& m) override {
        if (const auto* st = std::get_if<calcu::sessions::StateMessage>(&m)) {
            stateSets.push_back(st->topic);
            if (st->topic == calcu::sessions::kBusyTopic)
                busyState = std::dynamic_pointer_cast<const calcu::sessions::BusyState>(st->state);
        }
        if (const auto* s = std::get_if<calcu::sessions::StreamMessage>(&m); s && s->topic == cu59::topics::kShowDebugPanel)
            for (const auto& n : s->appended)
                if (const auto* x = calcu::sessions::payloadAs<cu59::TIShowDebugPanel>(n)) panels.push_back(x->panel);
        if (const auto* s = std::get_if<calcu::sessions::StreamMessage>(&m); s && s->topic == calcu::sessions::kNoticesTopic)
            for (const auto& n : s->appended)
                if (const auto* x = calcu::sessions::payloadAs<calcu::sessions::Notice>(n)) notices.push_back(x->sentence);
    }
    /// The notices so far (what a direct call said goes out at the next call).
    const std::vector<std::string>& errors() {
        app.session->call(now);
        return notices;
    }

    void run(std::chrono::milliseconds d) {
        for (auto end = now + d; now < end;) {
            now += std::chrono::microseconds(16'667);
            app.session->call(now);
        }
    }
    template <class F> bool runUntil(F cond, std::chrono::milliseconds limit = std::chrono::seconds(20)) {
        for (auto end = now + limit; now < end;) {
            if (cond()) return true;
            run(std::chrono::milliseconds(100));
        }
        return cond();
    }
    /// A preset as the Preset button loads it (the flow, not the operation).
    void load(const std::string& text) { part().loadStateFile(std::vector<uint8_t>(text.begin(), text.end())); }
};

} // namespace testroms
