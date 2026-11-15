#pragma once
// A test frontend for Calc-U-1600 on the library's app session (LIB-3
// AC-29's kind): the product's part through SharpHost, a scripted wall
// clock, the topics kept in a TopicMirror, and helpers that run a command
// to its end one host call at a time. ROMs and the bundled cards are
// copied into a temporary bundle folder; nothing of the user's is touched.

#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

#include "TestSettings.hpp"
#include "app/SharpHost.hpp"
#include "sessions/LibraryTopics.hpp"

namespace sessionfixture {

namespace fs = std::filesystem;
namespace sessions = calcu::sessions;

inline bool romsPresent() { return std::ifstream("roms/PC1600-P0-B0-new.bin").good(); }

struct SessionFixture {
    fs::path root;
    TestSettings testSettings;
    shared::Settings& settings = testSettings.settings;
    std::unique_ptr<shared::AppPaths> paths;
    calcu::pacing::WallTime now{1'000'000'000'000LL};
    std::unique_ptr<shared::SharpHost> host;
    std::vector<sessions::Message> messages;   // every message, in order
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

    explicit SessionFixture(const char* name = "calcu-session-test")
        : root(fs::temp_directory_path() / name) {
        fs::remove_all(root);
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
        host.reset();
        std::error_code ec;
        fs::remove_all(root, ec);
    }

    // Builds the session (not yet started).
    void open() {
        shared::SharpHost::Options o;
        o.clock = [this] {
            now += readCost;
            return now;
        };
        if (fixedHostTime) {
            const auto t = fixedTime;
            o.hostTime = [t] { return t; };
        }
        o.hostDirectories = hostDirectories;
        o.services = services;
        host = std::make_unique<shared::SharpHost>(settings, *paths, std::move(o));
        host->onMessage = [this](const sessions::Message& m) { messages.push_back(m); };
    }

    shared::SharpAppPart& part() { return host->part(); }
    sessions::AppSession& session() { return host->session(); }

    // One host call, `ms` of wall time after the last.
    void call(double ms = 1000.0 / 60) {
        now += std::chrono::nanoseconds(static_cast<std::int64_t>(ms * 1e6));
        session().call(now);
        ++calls;
    }
    // A command run to its end, one host call at a time.
    sessions::ReplyMessage run(const sessions::CommandName& name, sessions::Value args = {}) {
        return host->runToEnd(name, std::move(args), [this] { call(); });
    }
    bool ok(const sessions::CommandName& name, sessions::Value args = {}) {
        const sessions::ReplyMessage r = run(name, std::move(args));
        if (!r.done) std::fprintf(stderr, "  %s refused: %s\n", name.value.c_str(), r.reason.c_str());
        return r.done;
    }
    // Opens the session and starts `model` (a settings name: "PC-1500A").
    bool start(const char* model = nullptr) {
        if (model) settings.setStartupModelPreference(model);
        if (!host) open();
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
        const calcu::machine::DisplayOutput* d = host->display();
        return d && part().lcdText(*d, &text) ? text.text() : std::string();
    }
    template <class P> const P* state(const sessions::TopicName& t) const { return host->state<P>(t); }
    std::vector<std::string> notices() {
        std::vector<std::string> out;
        for (const auto& p : host->mirror().takeAppended(sessions::kNoticesTopic))
            if (const auto* n = sessions::payloadAs<sessions::Notice>(p)) out.push_back(n->sentence);
        return out;
    }
};

} // namespace sessionfixture
