// The Sharp pacing profiles (LIB-17 AC-20) and the sink side of the sound
// (SoundFeed). 0.8.1's Pacer tests (test_absolute_deadline,
// test_stall_and_restart) moved to the library's pacer tests in M-31
// (library/pacing/tests/pacer_tests.cpp: call_slice_keeps_wall_time_at_60_hz,
// lag_over_limit_dropped_whole, start_afresh_no_lag_nothing_to_wait,
// new_machine_new_rate_and_profile).

#include <cstdio>
#include <string>
#include <vector>

#include "app/AudioSink.hpp"
#include "app/PacingProfiles.hpp"
#include "TestHarness.hpp"

namespace {

// LIB-17 AC-20: the PC-1500, PC-1500A and PC-1600 each have a profile whose
// slice is the call, with no jitter, and full speed never paced.
TEST(lib17_ac20_pacing_profiles_sharp) {
    for (calcu::machine::ModelId m : {calcu::machine::ModelId{"PC-1500"}, calcu::machine::ModelId{"PC-1500A"}, calcu::machine::ModelId{"PC-1600"}}) {
        const calcu::pacing::PacingProfile p = cu1600::pacingProfile(m);
        CHECK(!p.slice.has_value());
        CHECK(!p.jitter);
        CHECK(!p.fullSpeedPacedWhileWaiting);
    }
}

struct FakeSink : cu1600::AudioSink {
    bool open = false;
    int starts = 0;
    int rate = 0;
    std::size_t roomLeft = 100000;
    std::vector<std::int16_t> written;
    bool start(int r, int) override {
        open = true;
        rate = r;
        ++starts;
        return true;
    }
    void stop() override { open = false; }
    std::size_t room() const override { return roomLeft; }
    void write(const std::int16_t* s, std::size_t n) override {
        written.insert(written.end(), s, s + n);
        roomLeft -= std::min(roomLeft, n);
    }
    bool ranDry() const override { return false; }
};

// The session decides what is heard (LIB-3 design §3.6); the sink side
// opens on SoundStart at its rate, plays what it is given, keeps what the
// device has no room for up to a cap, and closes on SoundStop.
TEST(test_sound_feed) {
    using namespace calcu::sessions;
    FakeSink sink;
    std::vector<std::string> warnings;
    cu1600::SoundFeed feed(sink, [&](const std::string& w) { warnings.push_back(w); });
    const std::vector<std::int16_t> tone(800, 5000);

    feed.deliver(SoundSamples{tone});
    CHECK(!sink.open);   // nothing before SoundStart
    feed.deliver(SoundStart{48000});
    CHECK(sink.open && sink.starts == 1 && sink.rate == 48000);
    feed.deliver(SoundSamples{tone});
    CHECK(sink.written.size() == tone.size());

    // No room: the backlog is trimmed once it passes 50 ms.
    sink.roomLeft = 0;
    for (int i = 0; i < 4; ++i) feed.deliver(SoundSamples{tone});
    CHECK(!warnings.empty());

    feed.deliver(SoundStop{});
    CHECK(!sink.open);
}

} // namespace
