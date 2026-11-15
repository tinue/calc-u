#pragma once
#include "TestRoms.hpp"
#include "../app/TIAppPart.hpp"
#include "sessions/LibraryTopics.hpp"
#include "sessions/TopicMirror.hpp"
#include <atomic>
#include <chrono>
#include <filesystem>
#include <memory>
#include <random>
#include <string>
#include <vector>

// Calc-U-59 on the library's app session as a frontend hosts it (LIB-3 59,
// C59-5): a test frontend that keeps a topic mirror and the raw messages,
// and a wall clock of the test's own (it moves when the test moves it, and
// 1 ms at every reading of the pacer, as LIB-3's ScriptedClock). No host
// clock is read.

namespace testroms {

namespace fs = std::filesystem;

inline std::string catalog() { return readFile("roms/solid-state/cuecards.txt").value_or(""); }

/// A folder of its own for one rig, removed at the end.
struct TempFolder {
    fs::path path;
    TempFolder() {
        static std::atomic<int> n{0};
        path = fs::temp_directory_path() / ("calcu59-session-" + std::to_string(std::random_device{}()) + "-" + std::to_string(++n));
        fs::remove_all(path);
        fs::create_directories(path);
    }
    ~TempFolder() {
        std::error_code ec;
        fs::remove_all(path, ec);
    }
};

struct SessionRig : calcu::sessions::MessageSink {
    using Value = calcu::sessions::Value;
    TempFolder folder;
    calcu::pacing::WallTime now{std::chrono::hours(1)};
    calcu::sessions::TopicMirror mirror;
    std::vector<calcu::sessions::Message> messages;
    std::vector<calcu::sessions::ReplyMessage> replies;
    std::unique_ptr<cu59::TIAppSession> app;
    calcu::sessions::ListenerId me = 0;

    explicit SessionRig(cu59::Model model = cu59::Model::TI59, calcu::sessions::HostServices services = {},
                        std::vector<calcu::sessions::SessionPart*> before = {}) {
        cu59::TIAppSetup s;
        s.roms = romSet();
        s.cuecards = catalog();
        s.startModel = model;
        s.saveFolder = folder.path.string();
        s.trace = {folder.path, 1'000'000};
        app = cu59::makeAppSession(std::move(s), [this] { return now += std::chrono::milliseconds(1); }, 59u,
                                   std::move(before));
        me = session().listen(*this, std::move(services));
    }

    calcu::sessions::AppSession& session() { return *app->session; }
    cu59::TIAppPart& part() { return *app->part; }
    calcu::machine::Machine& machine() { return *session().machineSession().machine(); }

    void deliver(const calcu::sessions::Message& m) override {
        messages.push_back(m);
        if (const auto* r = std::get_if<calcu::sessions::ReplyMessage>(&m)) replies.push_back(*r);
        mirror.deliver(m);
    }

    calcu::sessions::CallId post(const calcu::sessions::CommandName& name, Value args = Value::object({})) {
        return session().send(me, calcu::sessions::Command{name, std::move(args)});
    }
    /// The reply of `id`, once it came.
    const calcu::sessions::ReplyMessage* reply(calcu::sessions::CallId id) const {
        for (const auto& r : replies)
            if (r.call == id) return &r;
        return nullptr;
    }
    calcu::sessions::ReplyMessage send(const calcu::sessions::CommandName& name, Value args = Value::object({})) {
        const auto id = post(name, std::move(args));
        if (const auto* r = reply(id)) return *r;
        return calcu::sessions::ReplyMessage{id, false, "no reply yet", nullptr};
    }

    /// One host call, a 60th of a second after the last.
    void frame() {
        now += std::chrono::microseconds(16'667);
        session().call(now);
    }
    void runFor(std::chrono::milliseconds d) {
        const auto end = now + d;
        while (now < end) frame();
    }
    template <class F> bool until(F cond, std::chrono::milliseconds timeout = std::chrono::milliseconds(10'000)) {
        const auto end = now + timeout;
        while (now < end) {
            if (cond()) return true;
            frame();
        }
        return cond();
    }
    template <class P> const P* state(const calcu::sessions::TopicName& t) const {
        const calcu::sessions::PayloadPtr* p = mirror.state(t);
        return p ? calcu::sessions::payloadAs<P>(*p) : nullptr;
    }
    bool busy() const {
        const auto* b = state<calcu::sessions::BusyState>(calcu::sessions::kBusyTopic);
        return b && b->operationRuns;
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

} // namespace testroms
