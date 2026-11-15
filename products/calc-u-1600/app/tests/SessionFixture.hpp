#pragma once
// A test frontend for Calc-U-1600 on the library's app session (LIB-3
// AC-29's kind): the product's session (makeSharpSession), a scripted wall
// clock, a Listener that keeps the topics, the replies by call, and helpers
// that run a command to its end one host call at a time. ROMs and the bundled cards are
// copied into a temporary bundle folder of the fixture's own (library/testing/
// TempFolder: a unique name, so parallel runs do not collide); nothing of the
// user's is touched.

#include <chrono>
#include <cstdio>
#include <filesystem>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "TempFolder.hpp"
#include "TestFrontend.hpp"
#include "TestSettings.hpp"
#include "app/SharpSession.hpp"
#include "sessions/Listener.hpp"
#include "sessions/LibraryTopics.hpp"

namespace sessionfixture {

namespace fs = std::filesystem;
namespace sessions = calcu::sessions;

inline bool romsPresent() { return fullRomSetPresent(); }

struct SessionFixture {
    calcu::testing::TempFolder folder;
    fs::path root = folder.path();
    TestSettings testSettings;
    shared::Settings& settings = testSettings.settings;
    std::unique_ptr<shared::AppPaths> paths;
    calcu::pacing::WallTime now{1'000'000'000'000LL};
    sessions::ProductSession<shared::SharpAppPart> app;
    sessions::Listener listener;               // stops listening before the session goes
    std::vector<sessions::Message> messages;   // every message, in order
    std::map<sessions::CallId, sessions::ReplyMessage> replies;   // not taken yet
    std::chrono::system_clock::time_point fixedTime{};
    bool fixedHostTime = false;
    bool hostDirectories = true;
    sessions::HostServices services;
    int calls = 0;
    // Wall time each read of the clock takes: the host's work inside a call
    // (an operation at full speed runs until its allowance is used up).
    // Without it, a call at full speed (an operation) would never see its
    // allowance pass on this clock: the fixture sets 100 us by default.
    std::chrono::nanoseconds readCost{100'000};

    explicit SessionFixture(const char* name = "calcu-session-test") : folder(name) {
        const fs::path bundle = root / "bundle";
        fs::create_directories(bundle);
        for (const auto& dir : {fs::path("roms"), fs::path("frontends/qt6/resources/cards")})
            for (const auto& f : fs::directory_iterator(dir))
                if (f.is_regular_file()) fs::copy_file(f.path(), bundle / f.path().filename());
        paths = std::make_unique<shared::AppPaths>(
            settings, testHostDirs((root / "home").generic_string()),
            shared::BundleDirs{bundle.generic_string(), fs::absolute("examples").generic_string()});
    }
    ~SessionFixture() {
        listener.stop();
        app = {};
    }

    // Builds the session (not yet started).
    void open() {
        shared::SharpSessionOptions o;
        o.clock = [this] {
            now += readCost;
            return now;
        };
        if (fixedHostTime) {
            const auto t = fixedTime;
            o.hostTime = [t] { return t; };
        }
        app = shared::makeSharpSession(settings, *paths, std::move(o));
        listener.onMessage = [this](const sessions::Message& m) {
            messages.push_back(m);
            if (const auto* r = std::get_if<sessions::ReplyMessage>(&m)) replies[r->call] = *r;
        };
        sessions::HostServices offered = services;
        if (hostDirectories) offered.offered.insert(sessions::HostService::FolderPicker);   // File ▸ Mount Directory
        listener.listen(*app.session, std::move(offered));
    }

    shared::SharpAppPart& part() { return *app.part; }
    sessions::AppSession& session() { return *app.session; }
    bool busy() const { return app.session->busy(); }

    // A command; its reply, once it came, from takeReply().
    sessions::CallId send(const sessions::CommandName& name, sessions::Value args = {}) {
        auto sent = listener.send(name, std::move(args));
        if (sent.reply) {
            messages.push_back(*sent.reply);
            replies[sent.call] = *sent.reply;
        }
        return sent.call;
    }
    std::optional<sessions::ReplyMessage> takeReply(sessions::CallId call) {
        const auto it = replies.find(call);
        if (it == replies.end()) return std::nullopt;
        sessions::ReplyMessage r = std::move(it->second);
        replies.erase(it);
        return r;
    }

    // One host call, `ms` of wall time after the last.
    void call(double ms = 1000.0 / 60) {
        now += std::chrono::nanoseconds(static_cast<std::int64_t>(ms * 1e6));
        session().call(now);
        ++calls;
    }
    // A command run to its end, one host call at a time.
    sessions::ReplyMessage run(const sessions::CommandName& name, sessions::Value args = {}) {
        return listener.runToEnd(name, std::move(args), [this] { call(); });
    }
    bool ok(const sessions::CommandName& name, sessions::Value args = {}) {
        const sessions::ReplyMessage r = run(name, std::move(args));
        if (!r.done) std::fprintf(stderr, "  %s refused: %s\n", name.value.c_str(), r.reason.c_str());
        return r.done;
    }
    // Opens the session and starts `model` (a settings name: "PC-1500A").
    bool start(const char* model = nullptr) {
        if (model) settings.setStartupModelPreference(model);
        if (!app.session) open();
        return ok(shared::commands::kStart);
    }
    // `seconds` of wall time at regular speed.
    void runFor(double seconds) {
        const int n = static_cast<int>(seconds * 60);
        for (int i = 0; i < n; ++i) call();
    }
    // Until the typing hook has typed everything, at most `capSeconds`.
    bool runUntilTyped(double capSeconds) {
        const int n = static_cast<int>(capSeconds * 60);
        for (int i = 0; i < n; ++i) {
            if (part().typing().idle()) return true;
            call();
        }
        return part().typing().idle();
    }
    std::string screen() {
        LcdText text;
        const calcu::machine::DisplayOutput* d = listener.display();
        return d && part().lcdText(*d, &text) ? text.text() : std::string();
    }
    template <class P> const P* state(const sessions::TopicName& t) const { return listener.state<P>(t); }
    std::vector<std::string> notices() { return sessions::test::noticeSentences(listener.mirror()); }
};

} // namespace sessionfixture
