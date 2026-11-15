// The key sequencer (KeySequencer) on virtual time, advanced in 20 ms
// slices as the machine session's slice ends advance it (TIKeySequencerHook):
// holds, waits for the scan loop, cancels, reloads and preset cards. The
// product's flows over it are part_flows_tests.cpp.

#include "TestHarness.hpp"
#include "TestRoms.hpp"
#include "../app/KeyboardLayout.hpp"
#include "../app/TextUtil.hpp"
#include "../app/KeySequencer.hpp"
#include "../app/StateFile.hpp"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <deque>
#include <functional>
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

using namespace cu59;
using namespace std::chrono_literals;
using Clock = std::chrono::steady_clock;

namespace {

// Records every call with the time it came at; key ready when `idle`.
struct FakeHost : KeySequencer::Host {
    std::mutex mutex;
    std::vector<std::pair<std::string, Clock::time_point>> calls;
    std::atomic<bool> idle{true};
    std::atomic<bool> full{false};
    std::function<Clock::time_point()> clock = [] { return Clock::now(); };

    void note(std::string s) { std::lock_guard<std::mutex> lock(mutex); calls.emplace_back(std::move(s), clock()); }
    void pressKey(uint8_t c) override { note("press " + std::to_string(c)); }
    void releaseKey(uint8_t c) override { note("release " + std::to_string(c)); }
    bool isKeyReady() override { return idle; }
    std::atomic<bool> frozen{false};
    bool isFrozen() override { return frozen; }
    void heldKeyChanged(int id) override { note("held " + std::to_string(id)); }
    void setFullSpeed(bool on) override { full = on; note(on ? "full" : "regular"); }
    void scriptedTrace(const std::optional<std::string>&) override {}
    void zeroElapseTime() override { note("zero"); }
    void reportElapseTime() override { note("report"); }
    void playbackStarted() override { note("started"); }
    void playbackFinished() override { note("finished"); }
    std::atomic<int> pending{0};
    void applyReload(const std::shared_ptr<const ReloadStage>&) override { ++pending; note("reload"); }
    void queueCard(const KeystrokeEvent& e) override {
        if (e.kind == KeystrokeEvent::Kind::InsertCard) ++pending;
        note("card " + e.cardName);
    }
    void control(const KeystrokeEvent&) override { ++pending; note("control"); }
    bool hostPending() override { return pending > 0; }

    std::vector<std::string> names() {
        std::lock_guard<std::mutex> lock(mutex);
        std::vector<std::string> v;
        for (auto& c : calls) v.push_back(c.first);
        return v;
    }
    Clock::time_point at(const std::string& name) {
        std::lock_guard<std::mutex> lock(mutex);
        for (auto& c : calls) if (c.first == name) return c.second;
        return {};
    }
};

// A sequencer on virtual time, advanced in 20 ms slices as the machine
// session's slice ends advance it.
struct VirtualRig {
    FakeHost host;
    Clock::time_point t = Clock::time_point{} + 1h;
    KeySequencer keys{host};
    VirtualRig() { host.clock = [this] { return t; }; keys.advance(t); }
    void run(Clock::duration d, Clock::duration slice = 20ms) {
        auto end = t + d;
        while (t < end) {
            t += slice;
            keys.advance(t);
        }
    }
};

std::vector<KeystrokeEvent> keyEvents(std::initializer_list<uint8_t> codes) {
    std::vector<KeystrokeEvent> v;
    for (uint8_t c : codes) {
        KeystrokeEvent e;
        e.code = c;
        v.push_back(e);
    }
    return v;
}

KeystrokeEvent event(KeystrokeEvent::Kind kind, double seconds = 0) {
    KeystrokeEvent e;
    e.kind = kind;
    e.seconds = seconds;
    return e;
}

} // namespace

TEST(keys_script_holds_each_key_and_waits_for_the_scan_loop) {
    VirtualRig r;
    r.host.idle = false;  // still computing: the first key waits
    r.keys.play(keyEvents({25, 83}));
    r.run(200ms);
    CHECK(r.keys.isPlaying());
    CHECK(r.host.at("press 25") == Clock::time_point{});
    r.host.idle = true;
    auto idleAt = r.t;
    r.run(20ms);
    CHECK(r.host.at("press 25") == idleAt + 20ms);  // noticed at the next batch boundary
    r.host.idle = false;  // the key starts a computation
    r.run(200ms);
    CHECK(r.host.at("release 25") - r.host.at("press 25") == 100ms);
    CHECK(r.host.at("press 83") == Clock::time_point{});  // waits for the scan loop
    r.host.idle = true;
    idleAt = r.t;
    r.run(200ms);
    CHECK(r.host.at("press 83") == idleAt + 20ms);
    CHECK(!r.keys.isPlaying());
    CHECK(r.host.names() == (std::vector<std::string>{"started", "press 25", "release 25", "press 83", "release 83", "finished"}));
}

TEST(keys_wait_runs_at_the_speed_set_before_it) {
    VirtualRig r;
    r.keys.play({event(KeystrokeEvent::Kind::FullSpeed), event(KeystrokeEvent::Kind::Wait, 0.5),
                 event(KeystrokeEvent::Kind::RegularSpeed), event(KeystrokeEvent::Kind::ZeroElapseTime)});
    r.run(400ms);
    CHECK(r.host.full.load());
    r.run(200ms);
    CHECK(!r.host.full.load());
    CHECK(r.host.at("regular") - r.host.at("full") == 500ms);
    CHECK(r.host.names() == (std::vector<std::string>{"started", "full", "regular", "zero", "finished"}));
}

TEST(keys_cancel_ends_a_wait_but_not_a_hold) {
    VirtualRig r;
    auto events = keyEvents({25});
    events.push_back(event(KeystrokeEvent::Kind::Wait, 10));
    events.push_back(event(KeystrokeEvent::Kind::ZeroElapseTime));
    r.keys.play(events);
    r.run(40ms);
    r.keys.cancel();
    r.run(40ms);
    CHECK(r.host.at("release 25") == Clock::time_point{});  // the hold runs on
    r.run(40ms);
    CHECK(r.host.at("release 25") - r.host.at("press 25") == 100ms);
    r.keys.play(keyEvents({83}));  // queued after a cancel: runs
    r.run(1s);
    CHECK(r.host.names() == (std::vector<std::string>{"started", "press 25", "release 25", "finished",
                                                      "started", "press 83", "release 83", "finished"}));
}

TEST(keys_physical_key_keeps_the_minimum_hold) {
    VirtualRig r;
    r.keys.keyDown({83});
    r.keys.keyUp();  // in the same frame: the release is deferred
    CHECK(r.host.names() == (std::vector<std::string>{"held 37", "press 83", "held -1"}));
    r.run(80ms);
    CHECK(r.host.at("release 83") == Clock::time_point{});
    r.run(40ms);
    CHECK(r.host.at("release 83") - r.host.at("press 83") == 100ms);
}

TEST(keys_two_code_binding_taps_both_keys) {
    VirtualRig r;
    r.keys.keyDown({21, 11});  // 2nd, then A
    r.run(500ms);
    CHECK(r.host.names() == (std::vector<std::string>{"held 5", "press 21", "release 21", "held 0", "press 11",
                                                      "release 11", "held -1"}));
    r.keys.keyDown({21, 11});
    r.run(20ms);
    r.keys.keyDown({83});  // cancels the sequence after its first tap
    r.run(500ms);
    auto n = r.host.names();
    CHECK(std::count(n.begin(), n.end(), "press 11") == 1);
}


TEST(keys_reload_waits_for_the_scan_loop_and_the_host) {
    VirtualRig r;
    auto events = keyEvents({25});
    KeystrokeEvent reload = event(KeystrokeEvent::Kind::Reload);
    reload.reload = std::make_shared<ReloadStage>();
    events.push_back(reload);
    events.push_back(keyEvents({83})[0]);
    r.keys.play(events);
    r.run(40ms);
    CHECK(r.host.at("press 25") != Clock::time_point{});
    r.host.idle = false;  // the key starts a run: the reload waits
    r.run(300ms);
    CHECK(r.host.at("reload") == Clock::time_point{});
    r.host.idle = true;
    r.run(40ms);
    CHECK(r.host.at("reload") != Clock::time_point{});
    r.run(300ms);  // the host hasn't applied it yet: the next key waits
    CHECK(r.host.at("press 83") == Clock::time_point{});
    r.host.pending = 0;
    r.run(200ms);
    CHECK(r.host.at("press 83") != Clock::time_point{});
    CHECK(!r.keys.isPlaying());
}

TEST(keys_arm_card_does_not_wait_insert_card_does) {
    VirtualRig r;
    KeystrokeEvent arm = event(KeystrokeEvent::Kind::ArmCard);
    arm.cardName = "mem:A";
    KeystrokeEvent insert = event(KeystrokeEvent::Kind::InsertCard);
    insert.cardName = "B";
    std::vector<KeystrokeEvent> events{arm, insert};
    events.push_back(keyEvents({83})[0]);
    r.keys.play(events);
    r.run(100ms);
    CHECK(r.host.names() == (std::vector<std::string>{"started", "card mem:A", "card B"}));
    r.host.pending = 0;  // the inserted card has passed
    r.run(200ms);
    CHECK(r.host.at("press 83") != Clock::time_point{});
}

TEST(keys_script_waits_out_a_freeze_but_runs_its_directives) {
    VirtualRig r;
    r.host.frozen = true;
    std::vector<KeystrokeEvent> events{event(KeystrokeEvent::Kind::Resume)};
    events.push_back(keyEvents({83})[0]);
    r.keys.play(events);
    r.run(100ms);
    CHECK(r.host.at("control") != Clock::time_point{});  // Resume: runs while frozen
    r.host.pending = 0;                                  // … and is done
    r.run(300ms);
    CHECK(r.host.at("press 83") == Clock::time_point{});  // the key waits for the machine
    r.host.frozen = false;
    r.run(200ms);
    CHECK(r.host.at("press 83") != Clock::time_point{});
}
