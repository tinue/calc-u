// A product's option of `serve`. Its tasks
// come through the loop's inbox, in order with standard input's lines, and
// emulated time passes, unpaced, only while it wants time.
#include "TestHarness.hpp"
#include "TestProduct.hpp"
#include "harness/Run.hpp"
#include "harness/ServeAddOn.hpp"
#include "sessions/AppSession.hpp"
#include "sessions/MachineSession.hpp"
#include <atomic>
#include <future>
#include <memory>
#include <optional>
#include <string>
#include <thread>
#include <vector>

using namespace calcu::harness;
using namespace calcu::harness::test;

namespace {

calcu::machine::Ticks ticksOf(calcu::sessions::AppSession& s) {
    const calcu::machine::Machine* m = s.machineSession().machine();
    return m ? m->tickCount() : 0;
}

// Wants time until `want` ticks have passed since its first task, then
// posts the end of the run.
struct TestAddOn final : ServeAddOn {
    std::string_view option() const override { return "--test"; }
    std::string_view help() const override { return "a test"; }
    std::optional<std::string> start(std::string_view value, calcu::sessions::AppSession& s, ServeLoop l) override {
        if (value == "no") return std::string("It cannot start.");
        session = &s;
        loop = std::move(l);
        poster = std::thread([this] {   // another thread, as an I/O thread
            for (int i = 0; i < 3; ++i)
                loop.post([this, i] {
                    log.push_back("task " + std::to_string(i));
                    if (i == 2) {
                        from = ticksOf(*session);
                        wanting = true;
                    }
                });
        });
        return std::nullopt;
    }
    bool wantsTime() const override {
        if (!wanting) return false;
        if (ticksOf(*session) - from < want) return true;
        wanting = false;
        loop.post([self = const_cast<TestAddOn*>(this)] {
            self->ran = ticksOf(*self->session) - self->from;
            self->loop.quit();
        });
        return false;
    }
    void stop() override {
        stopped = true;
        if (poster.joinable()) poster.join();
    }
    calcu::sessions::AppSession* session = nullptr;
    ServeLoop loop;
    std::thread poster;
    std::vector<std::string> log;
    mutable bool wanting = false;
    calcu::machine::Ticks from = 0, want = 5000, ran = 0;
    bool stopped = false;
};

} // namespace

TEST(serve_addon_events_and_time_while_wanted) {
    TestRun run({}, std::nullopt);
    CHECK(run.server != nullptr);
    if (!run.server) return;
    TestAddOn addOn;
    CHECK(!addOn.start("yes", run.server->host().session(), run.server->serveLoop()));
    // Standard input stays open until the end of the test (its reader is left).
    auto open = std::make_shared<std::promise<void>>();
    std::shared_future<void> closed = open->get_future().share();
    std::vector<std::string> written;
    const int readsBefore = run.server->wallClockReads();
    const int status = run.server->serveWith(
        {&addOn}, [closed]() -> std::optional<std::string> {
            closed.wait();
            return std::nullopt;
        },
        [&written](const std::string& s) { written.push_back(s); });
    open->set_value();
    CHECK_EQ(status, 0);
    CHECK_EQ(addOn.log.size(), std::size_t{3});
    CHECK(addOn.log.size() == 3 && addOn.log[0] == "task 0" && addOn.log[2] == "task 2");   // in order
    CHECK(addOn.ran >= addOn.want);                       // time passed while it wanted it
    CHECK(addOn.ran < addOn.want + 100000);               // in pieces, looked at between them
    CHECK(addOn.stopped);
    CHECK_EQ(run.server->wallClockReads(), readsBefore);   // no wall clock read for it
}

TEST(serve_addon_lines_between_tasks_and_cannot_start) {
    TestRun run({}, std::nullopt);
    CHECK(run.server != nullptr);
    if (!run.server) return;
    TestAddOn refused;
    CHECK_EQ(refused.start("no", run.server->host().session(), run.server->serveLoop()).value_or(""),
             std::string("It cannot start."));
    // Without wanting time, standard input's lines are answered as in serve.
    struct Idle final : ServeAddOn {
        std::string_view option() const override { return "--idle"; }
        std::string_view help() const override { return ""; }
        std::optional<std::string> start(std::string_view, calcu::sessions::AppSession&, ServeLoop) override { return std::nullopt; }
        bool wantsTime() const override { return false; }
        void stop() override {}
    } idle;
    std::vector<std::string> lines{"ticks", "ticks"};
    std::size_t at = 0;
    std::vector<std::string> written;
    const int status = run.server->serveWith(
        {&idle}, [&]() -> std::optional<std::string> { return at < lines.size() ? std::optional<std::string>(lines[at++]) : std::nullopt; },
        [&written](const std::string& s) { written.push_back(s); });
    CHECK_EQ(status, 0);
    CHECK_EQ(written.size(), std::size_t{2});
    CHECK(written.size() == 2 && written[0] == written[1]);   // no time passed between them
}
