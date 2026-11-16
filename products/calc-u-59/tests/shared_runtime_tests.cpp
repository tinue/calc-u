// Shared/ runtime: C-indicator smoothing, the display as the eye sees it and
// a live session with KEYSTROKES playback. The armed freezes, the queued
// steps and the trace consumers went with the freeze in M-53: they are the
// run control's, debugger_tests.cpp (LIB-10 59). 2.3.0's
// batch rules (Pacing.hpp: cycle-weighted batches, the sleep rule) went with
// CooperativePacing in M-34; the library's pacer has its own tests (LIB-17).

#include "SessionRig.hpp"
#include "TestHarness.hpp"
#include "TestRoms.hpp"
#include "../core/TIRomRanges.hpp"
#include "../app/EmulatorSession.hpp"
#include "../app/FrameSampler.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>

using namespace cu59;
using namespace std::chrono_literals;

TEST(calc_indicator_smoothing) {
    FrameSampler s;
    CHECK(std::fabs(s.updateCalcIndicator(1.0f) - 0.65) < 1e-9);  // one bit: 0.65
    CHECK(std::fabs(s.updateCalcIndicator(0.0f) - 0.65) < 1e-9);  // one zero frame: hold
    CHECK(std::fabs(s.updateCalcIndicator(0.0f) - 0.30) < 1e-9);  // then decay 0.35
    CHECK(std::fabs(s.updateCalcIndicator(0.0f) - 0.0) < 1e-9);   // 0.70 → clamped at 0
    CHECK_EQ(s.updateCalcIndicator(4.0f), 1.0);                   // several bits: full brightness
}

// A real TI-59's C during an error blink with the C lit: normal in the dark phase,
// very weak in the lit phase, changing abruptly. The core's drive there: 1, then 0.09.
TEST(calc_indicator_blink_changes_abruptly) {
    FrameSampler s;
    CHECK(std::fabs(s.updateCalcIndicator(1.0f) - 0.65) < 1e-9);
    const double weak = s.updateCalcIndicator(0.09f);  // at once
    CHECK(std::fabs(weak - std::pow(0.09, 1.0 / 2.2) * 0.65) < 1e-6);
    CHECK(weak > 0.15 && weak < 0.3);                  // very weak, but visible
    CHECK(std::fabs(s.updateCalcIndicator(1.0f) - 0.65) < 1e-9);  // and back at once
}

// ── Display: persistence of vision (Shared/DisplayPov) ──

TEST(display_pov_channel_hold_and_fade) {
    const double frame = 1.0 / 60;
    DisplayPov::Channel c;  // the whole display darkens alike: best duty = own duty
    CHECK_EQ(c.step(1.0, 1.0, frame), 1.0);                  // driven: on at once
    CHECK_EQ(c.step(0.4, 0.4, frame), 1.0);                  // part of a frame dark: within the hold
    CHECK_EQ(c.step(1.0, 1.0, frame), 1.0);
    CHECK_EQ(c.step(0.0, 0.0, frame), 1.0);                  // 17 ms dark …
    CHECK_EQ(c.step(0.0, 0.0, frame), 1.0);                  // … 33 ms: still held
    double fading = c.step(0.0, 0.0, frame);                 // 50 ms
    fading = c.step(0.0, 0.0, frame);                        // 67 ms: fading
    CHECK(fading < 1.0 && fading > DisplayPov::kCutoff);
    for (int i = 0; i < 6; ++i) c.step(0.0, 0.0, frame);     // 167 ms: out
    CHECK_EQ(c.level, 0.0);
    CHECK_EQ(c.brightness(1.0), 0.0);
}

// A position that gets less of the multiplex than the others — alternately
// 54% and 81% of the best-driven one, as the 60 Hz polls catch the error
// blink's short sweeps — is steadily dimmer, not flickering and not fading:
// a 67.5% share looks like √0.675 = 82%.
TEST(display_pov_channel_share_is_steady) {
    const double frame = 1.0 / 60;
    DisplayPov::Channel c;
    double lo = 1, hi = 0;
    for (int i = 0; i < 120; ++i) {
        c.step(i % 2 ? 0.54 * 0.9 : 0.81 * 0.9, 0.9, frame);
        if (i >= 30) { lo = std::min(lo, c.brightness(1.0)); hi = std::max(hi, c.brightness(1.0)); }
    }
    CHECK_EQ(c.level, 1.0);
    CHECK(lo > 0.79 && hi < 0.85);
    CHECK(hi - lo < 0.04);
}

namespace {

// A machine driven frame by frame (60 Hz of emulated time) through FrameSampler,
// as the apps do.
struct PovRig {
    Model model;
    std::unique_ptr<calcu::machine::Machine> m;
    FrameSampler sampler;
    double hz;
    double acc = 0;
    explicit PovRig(Model md) : model(md), m(testroms::makeMachine(md)), hz(testroms::instructionHz(md)) {
        testroms::runToScanLoop(*m, md);
        m->readDisplay();
    }
    FrameUpdate frame() {
        while (acc < hz / 60) acc += static_cast<double>(m->advance(1) / 32);   // one instruction: its weight
        acc -= hz / 60;
        return sampler.tick(*m, false);
    }
    static int brightest(const FrameUpdate& f) {
        int mx = 0;
        for (uint8_t l : f.led.digitLevel) mx = std::max(mx, int(l));
        return mx;
    }
    /// Press the last key of `keys` (the others are tapped first) and return the
    /// dimmest brightest-digit level over the next second.
    int dimmestDuring(std::initializer_list<int> setup, std::initializer_list<int> keys) {
        for (int k : setup) testroms::tap(*m, model, k);
        std::vector<int> v(keys);
        for (size_t i = 0; i + 1 < v.size(); ++i) testroms::tap(*m, model, v[i]);
        for (int i = 0; i < 10; ++i) frame();
        testroms::press(*m, v.back());
        int dim = 255;
        for (int i = 0; i < 60; ++i) {
            if (i == 6) testroms::release(*m, v.back());
            dim = std::min(dim, brightest(frame()));
        }
        return dim;
    }
};

} // namespace

// Calibrated on a real TI-59: a digit key's ≈ 11 ms of dark display is
// invisible, sin (≈ 350 ms) visibly darkens.
TEST(display_pov_keys_as_the_eye_sees_them) {
    CHECK_EQ(PovRig(Model::TI59).dimmestDuring({82, 83}, {84}), 255);    // 1 2 → 3
    CHECK_EQ(PovRig(Model::TI59).dimmestDuring({84, 92}, {21, 33}), 0);  // 30 sin
}

// In the error blink each lit pass leaves IDLE before the sweep reaches the
// right-hand digits every time, so they get less of the multiplex. On a real
// TI-59 they look a little dimmer than the left ones, steadily — no flicker.
// Mid-phase (frames well inside a lit phase) every position must hold its
// brightness frame to frame, the rightmost must be dimmer than the leftmost
// but never dark (with the old per-poll brightness it flickered between 60%
// and 87% on the TI-58, and before that dropped to dark).
TEST(display_pov_error_blink_is_steady) {
    for (Model model : {Model::TI58, Model::TI59}) {
        PovRig r(model);
        for (int k : {92, 35}) testroms::tap(*r.m, model, k);  // 0 1/x → error blink
        std::vector<FrameUpdate> f;
        for (int i = 0; i < 600; ++i) f.push_back(r.frame());
        // "9.9999999 99": position 10 is the leftmost lit one (not fading).
        auto lit = [&](size_t i) { return f[i].led.digitLevel[10] >= 200; };
        int midFrames = 0, worstSpread = 0, dimmest = 255, phases = 0;
        int lo[12], hi[12];
        bool inPhase = false;
        for (size_t i = 3; i + 3 < f.size(); ++i) {
            bool mid = true;
            for (size_t j = i - 3; j <= i + 3; ++j) mid = mid && lit(j);
            if (!mid) {
                inPhase = false;
                continue;
            }
            if (!inPhase) {  // a new lit phase: compare frames within it only
                for (int p = 0; p < 12; ++p) lo[p] = hi[p] = f[i].led.digitLevel[p];
                inPhase = true;
                ++phases;
            }
            if (phases == 1) continue;  // the shares still settle from the display before the blink
            ++midFrames;
            for (int p = 0; p < 12; ++p) {
                lo[p] = std::min(lo[p], int(f[i].led.digitLevel[p]));
                hi[p] = std::max(hi[p], int(f[i].led.digitLevel[p]));
                worstSpread = std::max(worstSpread, hi[p] - lo[p]);
            }
            CHECK(f[i].led.digitLevel[10] >= 245);
            dimmest = std::min(dimmest, int(f[i].led.digitLevel[0]));
        }
        CHECK(midFrames > 100);
        CHECK(worstSpread <= 13);              // ≤ 5%: no visible flicker
        CHECK(dimmest > 150 && dimmest < 245); // a little dimmer, never dark
    }
}


