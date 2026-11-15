// `calcu attach` (C59-5 design §6, A6): the client against an endpoint whose
// inbox answers in the test (a stand-in for the app).

#include "TestHarness.hpp"
#include "harness/attach/AttachClient.hpp"
#include "harness/attach/AttachEndpoint.hpp"
#include <algorithm>
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
    void post(AttachEvent e) override {
        if (e.kind != AttachEvent::Kind::Line) return;
        {
            std::lock_guard<std::mutex> lock(m);
            lines.push_back(e.line);
        }
        if (e.line == "json on") return endpoint.write(e.client, {"{\"done\":true,\"result\":null,\"reason\":null}"});
        endpoint.write(e.client, {"message display x", "done 1", e.line});
    }
};

std::function<std::optional<std::string>()> input(std::vector<std::string> lines) {
    auto q = std::make_shared<std::deque<std::string>>(lines.begin(), lines.end());
    return [q]() -> std::optional<std::string> {
        if (q->empty()) return std::nullopt;
        std::string l = q->front();
        q->pop_front();
        return l;
    };
}

} // namespace

TEST(c59_5_ac10_calcu_attach_relays_lines) {
    EchoApp app;
    CHECK(app.endpoint.open().ok);
    const std::string port = std::to_string(app.endpoint.port());
    std::string out, err;
    const int status = runAttach({"--port", port}, input({"ticks", "lcd"}), [&](const std::string& s) { out += s; },
                                 [&](const std::string& s) { err += s; });
    CHECK_EQ(status, 0);
    CHECK_EQ(out, std::string("done 1\nticks\ndone 1\nlcd\n"));   // message lines dropped (D-1)
    out.clear();
    CHECK_EQ(runAttach({"--port", port, "--messages"}, input({"ticks"}), [&](const std::string& s) { out += s; },
                       [&](const std::string& s) { err += s; }),
             0);
    CHECK_EQ(out, std::string("message display x\ndone 1\nticks\n"));
    out.clear();
    runAttach({"--port", port, "--json"}, input({}), [&](const std::string& s) { out += s; },
              [&](const std::string& s) { err += s; });
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    std::lock_guard<std::mutex> lock(app.m);
    CHECK(std::find(app.lines.begin(), app.lines.end(), "json on") != app.lines.end());   // D-2
}

TEST(c59_5_ac11_options_that_do_not_apply_are_refused) {
    for (const char* o : {"--product", "--settings", "--save-folder", "--output-folder"}) {
        std::string out, err;
        const int status = runAttach({o, "x"}, input({}), [&](const std::string& s) { out += s; },
                                     [&](const std::string& s) { err += s; });
        CHECK_EQ(status, 2);
        CHECK(err.find("does not apply to an attached harness") != std::string::npos);
    }
}
