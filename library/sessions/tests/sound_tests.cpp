// The audio policy.
#include "TestHarness.hpp"
#include "TestParts.hpp"
#include <algorithm>
#include <array>

using namespace calcu::sessions;
using namespace calcu::sessions::test;
namespace mt = calcu::machine::test;

namespace {
std::size_t samplesIn(const TestFrontend& fe, std::size_t from = 0) {
    std::size_t n = 0;
    for (std::size_t i = from; i < fe.log.size(); ++i)
        if (const auto* s = std::get_if<SoundSamples>(&fe.log[i])) n += s->mono.size();
    return n;
}
// The samples the test machine makes for `ticks` at `soundRate` (TestMachines.cpp).
std::size_t madeFor(Ticks ticks, std::uint64_t soundRate) { return (ticks * soundRate + mt::kDotRate - 1) / mt::kDotRate; }

RigOptions withSink() {
    RigOptions o;
    o.services.offered = {HostService::AudioSink};
    return o;
}
} // namespace

TEST(sink_told_rate_and_gets_regular_speed_sound) {
    Rig r(withSink());
    r.build(mt::kDot, 8000);
    for (int i = 0; i < 5; ++i) r.call();
    std::vector<Message> sound;
    for (const auto& m : r.fe.log)
        if (!std::holds_alternative<StateMessage>(m) && !std::holds_alternative<StreamMessage>(m)) sound.push_back(m);
    CHECK(sound.size() >= 3);
    if (sound.size() < 3) return;
    CHECK(std::holds_alternative<SoundStart>(sound[0]) && std::get<SoundStart>(sound[0]).sampleRate == 8000);
    // Primed with 40 ms of silence when it opens (as AudioPump), then the
    // machine's sound as it was made.
    const auto* prime = std::get_if<SoundSamples>(&sound[1]);
    CHECK(prime && prime->mono.size() == 320 && prime->mono[0] == 0);
    CHECK_EQ(samplesIn(r.fe) - 320, madeFor(r.ticks(), 8000));
    CHECK_EQ(r.fe.count<SoundStart>(), 1);
    CHECK_EQ(r.fe.count<SoundStop>(), 0);
}

TEST(full_speed_and_operation_sound_thrown_away) {
    Rig r(withSink());
    r.build(mt::kDot, 8000);
    r.call();
    std::size_t mark = r.fe.log.size();
    r.send("speed", Value::object({{"full", true}}));
    for (int i = 0; i < 3; ++i) r.call();
    CHECK_EQ(samplesIn(r.fe, mark), std::size_t(0));
    r.send("speed", Value::object({{"full", false}}));
    r.part.operation = runningFor(2'000'000);
    r.send("tp.op");
    CHECK(r.callWhileBusy() > 1);
    CHECK_EQ(samplesIn(r.fe, mark), std::size_t(0));
    // Back at regular speed: only the sound of the new call, no backlog.
    const Ticks t0 = r.ticks();
    mark = r.fe.log.size();
    r.call();
    const std::size_t got = samplesIn(r.fe, mark);
    CHECK(got > 0);
    CHECK(got <= madeFor(r.ticks() - t0, 8000) + 1);
}

TEST(full_speed_sound_thrown_away_while_the_machine_waits) {
    // A waiting machine runs at full speed at the pace of regular speed; its
    // sound goes to the sink no more than that of an unpaced slice.
    Rig r(withSink());
    r.build(mt::kDot, 8000);
    r.waits(true);
    r.call();
    r.call();
    CHECK(r.session->machineSession().waitsForInput());
    CHECK(samplesIn(r.fe) > 0);   // the sink is open
    const std::size_t mark = r.fe.log.size();
    r.send("speed", Value::object({{"full", true}}));
    Ticks ran = 0;
    for (int i = 0; i < 5; ++i) {
        const Ticks t0 = r.ticks();
        r.call();
        ran = r.ticks() - t0;
    }
    CHECK(ran > 0 && ran < 2 * mt::kDotRate / 60);   // paced: one frame's ticks
    CHECK_EQ(samplesIn(r.fe, mark), std::size_t(0));
}

TEST(no_sink_thrown_away) {
    Rig r;   // the frontend states no audio sink
    r.build(mt::kDot, 8000);
    for (int i = 0; i < 3; ++i) r.call();
    CHECK_EQ(r.fe.count<SoundStart>() + r.fe.count<SoundSamples>() + r.fe.count<SoundStop>(), 0);
    std::array<std::int16_t, 16> buf{};
    CHECK_EQ(r.m->takeSound(buf), std::size_t(0));   // drained all the same
}

TEST(recorder_gets_all) {
    RigOptions o;
    o.services.recordsSound = true;   // the harness's `audio <file>`
    Rig r(o);
    r.build(mt::kDot, 8000);
    r.call();
    r.send("speed", Value::object({{"full", true}}));
    r.call();
    r.send("speed", Value::object({{"full", false}}));
    r.part.operation = runningFor(500'000);
    r.send("tp.op");
    r.callWhileBusy();
    r.session->advanceUnpaced(10'000);
    CHECK_EQ(r.fe.count<SoundStart>(), 1);
    CHECK(std::get<SoundStart>(*std::find_if(r.fe.log.begin(), r.fe.log.end(), [](const Message& m) {
              return std::holds_alternative<SoundStart>(m);
          })).sampleRate == 8000);
    CHECK_EQ(samplesIn(r.fe), madeFor(r.ticks(), 8000));   // every sample, whatever the speed
}

TEST(session_states_machine_rate_whatever_device) {
    // The sink is told the machine's rate; matching a device of another rate
    // is the frontend's (Web/audio-worklet.js today).
    for (int rate : {8000, 11025, 48000}) {
        Rig r(withSink());
        r.build(mt::kDot, rate);
        r.call();
        CHECK_EQ(r.fe.count<SoundStart>(), 1);
        for (const auto& m : r.fe.log)
            if (const auto* s = std::get_if<SoundStart>(&m)) CHECK_EQ(s->sampleRate, rate);
    }
}
