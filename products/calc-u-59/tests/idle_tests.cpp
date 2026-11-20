// The cases of docs/developer/Idle.md, through the harness: each brings the
// machine into its case with timed waits only, then reads the answer of
// `idle-state` (A takes keys, W waits, the program has ended) and checks
// what the steps do with it (`wait` returns once settled, `wait-finished`
// once the program has ended, keys that do not wait go in at once).
#include "../app/HarnessProduct.hpp"
#include "TestHarness.hpp"
#include "harness/Run.hpp"
#include <filesystem>
#include <memory>
#include <string>

using namespace calcu::harness;

namespace {

bool romsPresent() { return std::filesystem::exists("roms/TMC0571B.txt"); }

struct Idle {
    bool takesKeys = false, waits = false, ended = false;
    bool read = false;   // the reply had the three lines
};

class Ti {
public:
    explicit Ti(const std::string& model) {
        product_ = cu59::makeHarnessProduct();
        std::string error;
        server_ = Server::start(*product_, {}, &error);
        if (!server_) {
            std::fprintf(stderr, "calcu did not start: %s\n", error.c_str());
            return;
        }
        line("model " + model);
        line("wait 1s");
    }
    ~Ti() {
        if (server_) server_->finish();
    }
    bool ok() const { return server_ != nullptr; }
    std::string line(const std::string& l) { return server_->line(l).text; }

    Idle idle() {
        const std::string r = line("idle-state");
        Idle i;
        auto yes = [&](const std::string& key, bool* out) {
            const auto at = r.find("\n" + key + ": ");
            if (at == std::string::npos) return false;
            *out = r.compare(at + key.size() + 3, 3, "yes") == 0;
            return true;
        };
        i.read = yes("takes keys", &i.takesKeys) && yes("waits", &i.waits) && yes("ended", &i.ended);
        return i;
    }
    // The emulated seconds a `wait`-like reply took; -1 without them.
    static double seconds(const std::string& reply) {
        const auto at = reply.find("\nseconds: ");
        return at == std::string::npos ? -1.0 : std::stod(reply.substr(at + 10));
    }
    // Program steps in LRN mode, then back to RUN mode at step 000.
    void program(const std::string& keys) {
        line("key LRN " + keys + " LRN");
        line("wait 300ms");
        line("key RST");
        line("wait 300ms");
    }

private:
    std::unique_ptr<HarnessProduct> product_;
    std::unique_ptr<Server> server_;
};

// The row of a case: A, W, ended.
void expectRow(const Idle& i, bool a, bool w, bool ended) {
    CHECK(i.read);
    CHECK_EQ(i.takesKeys, a);
    CHECK_EQ(i.waits, w);
    CHECK_EQ(i.ended, ended);
}

// A settled machine: `wait` returns after the stretch, well within a second.
void expectSettles(Ti& t) {
    const double s = Ti::seconds(t.line("wait"));
    CHECK(s >= 0.0 && s < 1.0);
}

} // namespace

// ── At the scan loop ─────────────────────────────────────────────────────

TEST(idle_scan_loop_of_every_model_with_and_without_printer) {
    SKIP_UNLESS(romsPresent(), "the ROMs are not fetched (roms/fetch_roms.sh)");
    for (const char* model : {"TI-59", "TI-58", "TI-58C"}) {
        for (bool printer : {false, true}) {
            Ti t(model);
            CHECK(t.ok());
            if (!t.ok()) return;
            if (printer) {
                t.line("attach printer PC-100C");
                t.line("wait 1s");
            }
            expectRow(t.idle(), true, true, true);
            expectSettles(t);
            const double s = Ti::seconds(t.line("wait-finished"));
            CHECK(s >= 0.0 && s < 1.0);
        }
    }
}

TEST(idle_error_blink_and_pending_operand_take_keys) {
    SKIP_UNLESS(romsPresent(), "the ROMs are not fetched (roms/fetch_roms.sh)");
    Ti t("TI-59");
    if (!t.ok()) return;
    t.line("key 0 1/x");   // an error: the display flashes
    t.line("wait 500ms");
    expectRow(t.idle(), true, true, true);
    t.line("key CLR STO");   // STO waits for its register
    t.line("wait 300ms");
    expectRow(t.idle(), true, true, true);
}

TEST(idle_a_held_key_keeps_the_release_gate) {
    SKIP_UNLESS(romsPresent(), "the ROMs are not fetched (roms/fetch_roms.sh)");
    Ti t("TI-59");
    if (!t.ok()) return;
    t.line("key-down 7");
    t.line("wait 300ms");   // taken; the ROM waits for the release
    expectRow(t.idle(), false, false, false);
    t.line("key-up 7");
    t.line("wait 300ms");
    expectRow(t.idle(), true, true, true);
}

// ── A program ────────────────────────────────────────────────────────────

TEST(idle_a_running_program_neither_takes_keys_nor_waits) {
    SKIP_UNLESS(romsPresent(), "the ROMs are not fetched (roms/fetch_roms.sh)");
    Ti t("TI-59");
    if (!t.ok()) return;
    t.program("2nd Lbl A GTO A");
    t.line("key A");
    t.line("wait 500ms");
    expectRow(t.idle(), false, false, false);
}

TEST(idle_a_pause_neither_takes_keys_nor_waits) {
    SKIP_UNLESS(romsPresent(), "the ROMs are not fetched (roms/fetch_roms.sh)");
    Ti t("TI-59");
    if (!t.ok()) return;
    t.program("2nd Lbl A 2nd Pause GTO A");
    t.line("key A");
    for (int i = 0; i < 4; ++i) {   // within a Pause and between two
        t.line("wait 130ms");
        expectRow(t.idle(), false, false, false);
    }
}

TEST(idle_r_s_goes_in_while_a_program_runs) {
    SKIP_UNLESS(romsPresent(), "the ROMs are not fetched (roms/fetch_roms.sh)");
    Ti t("TI-59");
    if (!t.ok()) return;
    t.program("2nd Lbl A GTO A");
    t.line("key A");
    t.line("wait 500ms");
    // R/S does not wait for A: it goes down at once and stays until the run
    // loop has sampled it; the program stops.
    CHECK_EQ(t.line("key R/S"), std::string("done\n"));
    t.line("wait 500ms");
    expectRow(t.idle(), true, true, true);
}

TEST(idle_a_key_takes_its_own_hold) {
    SKIP_UNLESS(romsPresent(), "the ROMs are not fetched (roms/fetch_roms.sh)");
    Ti t("TI-59");
    if (!t.ok()) return;
    t.program("2nd Lbl A GTO A");
    t.line("key A");
    t.line("wait 500ms");
    CHECK_EQ(t.line("key R/S hold=300ms"), std::string("done\n"));
    t.line("wait 500ms");
    expectRow(t.idle(), true, true, true);
    CHECK_EQ(t.line("key 7 hold=x"), std::string("refused: hold= takes a duration: x.\n"));
}

TEST(idle_wait_finished_lasts_until_the_program_has_ended) {
    SKIP_UNLESS(romsPresent(), "the ROMs are not fetched (roms/fetch_roms.sh)");
    Ti t("TI-59");
    if (!t.ok()) return;
    t.program("2nd Lbl A 2nd Pause 2nd Pause R/S");   // two Pauses, 432 ms each
    t.line("key A");
    const double s = Ti::seconds(t.line("wait-finished"));
    CHECK(s >= 0.8 && s < 2.0);
    expectRow(t.idle(), true, true, true);
}

// ── Cards and printer ────────────────────────────────────────────────────

TEST(idle_write_from_the_keyboard_waits_for_a_card) {
    SKIP_UNLESS(romsPresent(), "the ROMs are not fetched (roms/fetch_roms.sh)");
    Ti t("TI-59");
    if (!t.ok()) return;
    t.line("key 1 2nd Write");
    t.line("wait 300ms");
    expectRow(t.idle(), false, true, true);   // a card is awaited, no program runs
    expectSettles(t);
}

TEST(idle_write_in_a_program_waits_for_a_card_but_has_not_ended) {
    SKIP_UNLESS(romsPresent(), "the ROMs are not fetched (roms/fetch_roms.sh)");
    Ti t("TI-59");
    if (!t.ok()) return;
    t.program("2nd Lbl A 1 2nd Write R/S");
    t.line("key A");
    t.line("wait 500ms");
    expectRow(t.idle(), false, true, false);
    expectSettles(t);
}

TEST(idle_printing_neither_takes_keys_nor_waits) {
    SKIP_UNLESS(romsPresent(), "the ROMs are not fetched (roms/fetch_roms.sh)");
    Ti t("TI-59");
    if (!t.ok()) return;
    t.line("attach printer PC-100C");
    t.line("wait 1s");
    t.line("key-down 7");
    t.line("wait 150ms");
    t.line("key-up 7");
    t.line("wait 150ms");
    t.line("key-down 2nd");
    t.line("wait 150ms");
    t.line("key-up 2nd");
    t.line("wait 150ms");
    t.line("key-down +/-");   // 2nd +/- is Prt
    t.line("wait 150ms");
    t.line("key-up +/-");
    t.line("wait 30ms");   // the line is printing
    expectRow(t.idle(), false, false, false);
    expectSettles(t);   // settled only after the paper is out
    CHECK_EQ(t.line("paper text"), std::string("done 1\n          7.        \n"));
}
