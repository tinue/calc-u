// LIB-17's Calc-U-59 column (design §6, P59-1 to P59-3): the TI machines on
// the library's pacer, through the app session as the frontends host it.

#include "TestHarness.hpp"
#include "AppSessionRig.hpp"
#include "../app/PacingProfiles.hpp"
#include <algorithm>

using namespace cu59;
using namespace std::chrono_literals;
using namespace calcu::sessions;
using testroms::SessionRig;

namespace {

// Emulated seconds against wall seconds over `wall` of host calls at 60 Hz.
double rateOver(SessionRig& r, std::chrono::milliseconds wall) {
    const auto t0 = r.ticks();
    const auto w0 = r.now;
    r.runFor(wall);
    return r.seconds(r.ticks() - t0) / std::chrono::duration<double>(r.now - w0).count();
}

void program(SessionRig& r, const std::string& keycodes) {
    const CallId id = r.post(commands::kLoadState, Value::object({{"bytes", r.bytes("PROGRAM:\n" + keycodes + "\n")}}));
    r.until([&] { return r.reply(id) != nullptr; });
    r.until([&] { return r.machine().isKeyReady(); });
}

// A part with one command: a stretch at full speed whose operation reports
// "not waiting" (a preset's `wait:` with a duration, LIB-6 AC-20).
struct FastWaitPart final : SessionPart {
    void attach(SessionSetup& s) override {
        s.command(CommandName{"test.fast-wait"}, [](const Value&, SessionContext&) -> CommandOutcome {
            return StartOperation{[](OperationContext& op) -> Task<OperationResult> {
                op.setReportsWaiting(false);
                const auto rate = op.session().machine()->descriptor().tickRate;
                co_await op.run(rate);   // one emulated second
                op.setReportsWaiting(true);
                co_return OperationResult{};
            }};
        });
    }
};

} // namespace

TEST(lib17_ac15_pacing_profiles_ti) {
    for (Model m : {Model::TI59, Model::TI58, Model::TI58C}) {
        const auto p = pacingProfile(m);
        CHECK(p.slice == std::optional<std::chrono::microseconds>(20ms));
        CHECK(p.jitter);
        CHECK(p.fullSpeedPacedWhileWaiting);
    }
    // The session hands it to the pacer with the machine: a 20 ms slice at the
    // model's rate (9,100 / 7,680 / 7,880 ticks a slice, design R3) keeps time.
    SessionRig r;
    CHECK(r.until([&] { return r.machine().isKeyReady(); }));
    const double rate = rateOver(r, 2s);
    CHECK(rate > 0.99 && rate < 1.01);
}

TEST(lib17_ac16_full_speed_paced_at_key_readiness) {
    SessionRig r;
    CHECK(r.until([&] { return r.machine().isKeyReady(); }));
    r.send(kSpeedCommand, Value::object({{"full", true}}));
    const double rate = rateOver(r, 2s);
    CHECK(rate > 0.95 && rate < 1.05);   // idle: the pace of regular speed
}

TEST(lib17_ac16_full_speed_races_a_computation) {
    SessionRig r;
    CHECK(r.until([&] { return r.machine().isKeyReady(); }));
    program(r, "76 11 61 00 00");   // Lbl A GTO 000: computes forever
    r.tap(11);                      // A
    const double regular = rateOver(r, 1s);
    r.send(kSpeedCommand, Value::object({{"full", true}}));
    const double full = rateOver(r, 1s);
    CHECK(regular > 0.95 && regular < 1.05);
    CHECK(full > 2.0 * regular);
}

TEST(lib17_ac16_printer_wait_is_not_waiting) {
    SessionRig r;
    CHECK(r.until([&] { return r.machine().isKeyReady(); }));
    program(r, "76 11 01 99 61 00 02");   // Lbl A 1 Prt GTO 002: prints forever
    r.tap(11);
    r.send(kSpeedCommand, Value::object({{"full", true}}));
    r.runFor(200ms);
    const double full = rateOver(r, 1s);
    CHECK(full > 1.5);
    CHECK(!r.state<TIPrinterState>(topics::kPrinter)->lines.empty());
}

TEST(lib17_ac16_card_wait_is_waiting) {
    SessionRig r;
    CHECK(r.until([&] { return r.machine().isKeyReady(); }));
    program(r, "76 11 04 96 91");   // Lbl A 4 Write R/S: waits for a card
    r.tap(11);
    r.send(kSpeedCommand, Value::object({{"full", true}}));
    r.runFor(200ms);
    CHECK(testroms::card(r.machine()).isInCardWait() && testroms::card(r.machine()).isWaitingForCard());
    const double rate = rateOver(r, 1s);
    CHECK(rate > 0.95 && rate < 1.05);
}

TEST(lib17_ac16_preset_wait_at_full_speed_on_idle_is_not_paced) {
    FastWaitPart fast;
    SessionRig r(Model::TI59, {}, {&fast});
    CHECK(r.until([&] { return r.machine().isKeyReady(); }));
    const auto w0 = r.now;
    const CallId id = r.post(CommandName{"test.fast-wait"});
    CHECK(r.until([&] { return r.reply(id) != nullptr; }));
    CHECK(r.reply(id)->done);
    const double wall = std::chrono::duration<double>(r.now - w0).count();
    CHECK(wall < 0.5);   // one emulated second, not paced
}

// AC-17 (port of cooperative_runner_batch_ends_do_not_lock_onto_a_loop): the
// TI-58's error blink seen at 50 slice ends takes more than one PC.
TEST(lib17_ac17_ti58_error_blink_pc_varies_over_50_slice_ends) {
    SessionRig r(Model::TI58);
    CHECK(r.until([&] { return r.machine().isKeyReady(); }));
    r.tap(92);   // 0
    r.tap(35);   // 1/x: error, the display blinks
    std::vector<uint16_t> ends;
    for (int i = 0; i < 150 && ends.size() < 50; ++i) {
        r.frame();
        ends.push_back(testroms::dbg(r.machine()).pc());
    }
    std::sort(ends.begin(), ends.end());
    CHECK(std::unique(ends.begin(), ends.end()) - ends.begin() > 1);
}
