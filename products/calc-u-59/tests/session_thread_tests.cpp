// Calc-U-59 on the library's native threaded host, as the Swift frontend
// hosts it (the host itself has its tests in library/shell/tests/).

#include "TestHarness.hpp"
#include "SessionRig.hpp"
#include "shell/SessionThread.hpp"
#include "sessions/LibraryTopics.hpp"
#include <atomic>
#include <chrono>
#include <thread>
#include <vector>

using namespace cu59;
using namespace std::chrono_literals;
using calcu::shell::Delivery;
using calcu::shell::SessionThread;
using Clock = std::chrono::steady_clock;

namespace {

template <class F> bool waitFor(F f, std::chrono::milliseconds t = 3000ms) {
    const auto end = Clock::now() + t;
    while (Clock::now() < end) {
        if (f()) return true;
        std::this_thread::sleep_for(1ms);
    }
    return f();
}

} // namespace

// The real session on the thread: Calc-U-59 runs, and a key reaches it.
TEST(ti59_on_the_session_thread) {
    calcu::testing::TempFolder folder;
    static calcu::settings::SettingsFile settings = calcu::settings::SettingsFile::defaultsOnly(settings::settingsDecl());
    static std::unique_ptr<calcu::settings::Paths> paths;
    paths = testroms::rigPaths(settings, folder.path().string(), folder.path().string());
    const TIAppSetup setup = testroms::appSetup(settings, ModelId{kTI59}, *paths);
    std::atomic<std::uint64_t> ticks{0};
    auto t = std::make_unique<SessionThread>(
        [setup] { return makeAppSession(setup, steadyWallTime).session; },
        calcu::sessions::HostServices{}, steadyWallTime, [] {});
    std::this_thread::sleep_for(500ms);
    t->post([&](calcu::sessions::AppSession& a) { ticks = a.machineSession().machine()->tickCount(); });
    CHECK(waitFor([&] { return ticks.load() > 0; }));
    std::vector<Delivery> got = t->drain();
    bool led = false;
    for (auto& d : got)
        if (const auto* m = std::get_if<calcu::sessions::StateMessage>(&d.message)) led |= m->topic == topics::kCalcIndicator;
    CHECK(led);
}
