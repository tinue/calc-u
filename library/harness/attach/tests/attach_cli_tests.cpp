// `calcu attach`: the client against an endpoint whose
// inbox answers in the test (a stand-in for the app).

#include "LineInput.hpp"
#include "TestHarness.hpp"
#include "harness/attach/AttachClient.hpp"
#include "harness/attach/AttachEndpoint.hpp"
#include <algorithm>
#include <chrono>
#include <deque>
#include <memory>
#include <mutex>
#include <thread>

using namespace calcu::harness;

namespace {

// Answers every line with `done 1` and the line, after a message line.
struct EchoApp final : AttachInbox {
    AttachEndpoint endpoint{*this, 0};
    std::mutex m;
    std::vector<std::string> lines;
    int detached = 0;
    bool refuseJson = false;   // an app that does not know `json on`
    void post(AttachEvent e) override {
        if (e.kind == AttachEvent::Kind::Detached) {
            std::lock_guard<std::mutex> lock(m);
            ++detached;
        }
        if (e.kind != AttachEvent::Kind::Line) return;
        {
            std::lock_guard<std::mutex> lock(m);
            lines.push_back(e.line);
        }
        if (e.line == "json on" && refuseJson) return endpoint.write(e.client, {"refused: unknown command json"});
        if (e.line == "json on") return endpoint.write(e.client, {"{\"done\":true,\"result\":null,\"reason\":null}"});
        endpoint.write(e.client, {"message display x", "done 1", e.line});
    }
    // The endpoint takes one harness at a time: before the next client
    // connects, wait until it has dropped the one before (a race on CI).
    void waitDetached(int count) {
        for (int i = 0; i < 400; ++i) {
            {
                std::lock_guard<std::mutex> lock(m);
                if (detached >= count) return;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    }
};

} // namespace

TEST(calcu_attach_relays_lines) {
    EchoApp app;
    CHECK(app.endpoint.open().ok);
    const std::string port = std::to_string(app.endpoint.port());
    std::string out, err;
    const int status = runAttach({"--port", port}, input({"ticks", "display-text"}), [&](const std::string& s) { out += s; },
                                 [&](const std::string& s) { err += s; });
    CHECK_EQ(status, 0);
    CHECK_EQ(out, std::string("done 1\nticks\ndone 1\ndisplay-text\n"));   // message lines dropped without --messages
    out.clear();
    app.waitDetached(1);
    CHECK_EQ(runAttach({"--port", port, "--messages"}, input({"ticks"}), [&](const std::string& s) { out += s; },
                       [&](const std::string& s) { err += s; }),
             0);
    CHECK_EQ(out, std::string("message display x\ndone 1\nticks\n"));
    out.clear();
    app.waitDetached(2);
    runAttach({"--port", port, "--json"}, input({}), [&](const std::string& s) { out += s; },
              [&](const std::string& s) { err += s; });
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    std::lock_guard<std::mutex> lock(app.m);
    CHECK(std::find(app.lines.begin(), app.lines.end(), "json on") != app.lines.end());   // --json sends `json on` first
}

// An app that answers `json on` in plain: its replies would not be JSON.
TEST(calcu_attach_json_refused_by_the_app) {
    EchoApp app;
    app.refuseJson = true;
    CHECK(app.endpoint.open().ok);
    std::string out, err;
    const int status = runAttach({"--port", std::to_string(app.endpoint.port()), "--json"}, input({"ticks"}),
                                 [&](const std::string& s) { out += s; }, [&](const std::string& s) { err += s; });
    CHECK_EQ(status, 2);
    CHECK_EQ(err, std::string("The app did not switch to JSON: refused: unknown command json\n"));
    CHECK_EQ(out, std::string());
}

TEST(options_that_do_not_apply_are_refused) {
    for (const char* o : {"--product", "--settings", "--save-folder", "--output-folder"}) {
        std::string out, err;
        const int status = runAttach({o, "x"}, input({}), [&](const std::string& s) { out += s; },
                                     [&](const std::string& s) { err += s; });
        CHECK_EQ(status, 2);
        CHECK(err.find("does not apply to an attached harness") != std::string::npos);
    }
}
