// The exit check of M-28 (LIB-11 design §6, T14; D-1): `calcu` drives the
// Sharp machine of the default start, the PC-1500A, headless through the
// library's app session (the PC-1600 follows with LIB-18's model switch;
// the product ACs of LIB-11, AC-18 to AC-20, are tested in M-51).
#include "../app/HarnessProduct.hpp"
#include "TestHarness.hpp"
#include "harness/Run.hpp"
#include "TempFolder.hpp"
#include <cstdio>
#include <memory>
#include <filesystem>
#include <string>

using namespace calcu::harness;

TEST(harness_drives_a_sharp_machine) {
    SKIP_UNLESS(std::filesystem::exists("roms/PC-1500_A04.ROM"), "the ROMs are not fetched (roms/fetch_roms.sh)");
    auto product = shared::makeHarnessProduct();
    std::string error;
    auto server = Server::start(*product, {}, &error);
    CHECK(server != nullptr);
    if (!server) {
        std::fprintf(stderr, "%s\n", error.c_str());
        return;
    }
    auto reply = [&](const std::string& l) { return server->line(l).text; };
    CHECK(reply("wait 2s").rfind("done 2\nticks: ", 0) == 0);
    const std::string boot = reply("lcd");
    CHECK(boot.find("NEW0?") != std::string::npos);   // a fresh RAM asks first
    CHECK(boot.find("\nnot text: 0\n") != std::string::npos);
    for (const char* k : {"cl", "n", "e", "w", "0", "enter"}) {
        CHECK_EQ(reply(std::string("key-down ") + k), std::string("done\n"));
        reply("wait 70ms");
        reply(std::string("key-up ") + k);
        reply("wait 70ms");
    }
    reply("wait 1s");
    const std::string after = reply("lcd");
    CHECK(after.find("NEW0?") == std::string::npos);
    CHECK(after.find("indicators:") != std::string::npos);
    CHECK(reply("ticks").rfind("done 2\nticks: ", 0) == 0);
    CHECK(reply("display").rfind("done 10\n", 0) == 0);   // 7 rows of dots, indicators, names, on
    server->finish();
}

// LIB-18 design P16 (the PC-1600 of the M-28 exit check, LIB-11 D-1): the
// harness switches to the PC-1600 with LIB-18's `model`, and reads its
// display with the PC-1600's font.
TEST(harness_drives_a_pc1600) {
    SKIP_UNLESS(std::filesystem::exists("roms/PC1600-P0-B0-new.bin"), "the ROMs are not fetched (roms/fetch_roms.sh)");
    auto product = shared::makeHarnessProduct();
    std::string error;
    auto server = Server::start(*product, {}, &error);
    CHECK(server != nullptr);
    if (!server) {
        std::fprintf(stderr, "%s\n", error.c_str());
        return;
    }
    auto reply = [&](const std::string& l) { return server->line(l).text; };
    CHECK_EQ(reply("model PC-1600"), std::string("done\n"));
    const std::string lcd = reply("lcd");
    CHECK(lcd.rfind("done ", 0) == 0);
    CHECK(lcd.find(">") != std::string::npos);   // the PC-1600's prompt, read with its font
    CHECK(lcd.find("\nnot text: 0\n") != std::string::npos);
    CHECK(reply("capabilities").find("place host-drive empty\n") != std::string::npos);   // a PC-1600 place
    const auto* m = server->host().session().context().machine();
    CHECK(m && m->model().value == "PC-1600");
    server->finish();
}

TEST(harness_variant_switch_old) {
    SKIP_UNLESS(std::filesystem::exists("roms/PC1600-P0-B0-old.bin"), "the ROMs are not fetched (roms/fetch_roms.sh)");
    auto product = shared::makeHarnessProduct();
    std::string error;
    auto server = Server::start(*product, {}, &error);
    CHECK(server != nullptr);
    if (!server) return;
    auto reply = [&](const std::string& l) { return server->line(l).text; };
    CHECK_EQ(reply("model PC-1600"), std::string("done\n"));
    CHECK_EQ(reply("variant old"), std::string("done\n"));   // any case (LIB-18 D-16)
    const auto* m = server->host().session().context().machine();
    CHECK(m && m->model().value == "PC-1600" && m->variant() && m->variant()->value == "Old");
    CHECK(reply("lcd").find(">") != std::string::npos);
    CHECK_EQ(reply("variant Old"), std::string("done\n"));   // the running variant: nothing to do
    server->finish();
}

// LIB-8 design §10.6, P16-8 (AC-32 with LIB-8 AC-23): the harness's paper
// commands on the CE-150 of a PC-1500A.
namespace {
struct PaperHarness {
    std::unique_ptr<HarnessProduct> product = shared::makeHarnessProduct();
    std::unique_ptr<Server> server;
    PaperHarness() {
        std::string error;
        server = Server::start(*product, {}, &error);
        if (!server) std::fprintf(stderr, "%s\n", error.c_str());
    }
    std::string reply(const std::string& l) { return server->line(l).text; }
    // Clears the fresh RAM's NEW0? question, plugs the CE-150 in and prints.
    void printOnCe150(const std::string& line) {
        reply("key cl");
        reply("type NEW0");
        reply("wait 1s");
        reply("attach printer CE-150");
        reply("type " + line);
        reply("wait 3s");
    }
};
} // namespace

TEST(lib8_ac32_harness_paper_marks_of_the_ce150) {
    SKIP_UNLESS(std::filesystem::exists("roms/CE-150.ROM"), "the ROMs are not fetched (roms/fetch_roms.sh)");
    PaperHarness h;
    CHECK(h.server != nullptr);
    if (!h.server) return;
    h.printOnCe150("LPRINT 123");
    const std::string marks = h.reply("paper marks CE-150");
    CHECK(marks.rfind("done ", 0) == 0);
    CHECK(marks.find("\nreach ") != std::string::npos);
    CHECK(marks.find("\nstroke ") != std::string::npos);
    CHECK_EQ(h.reply("paper text CE-150"), std::string("refused: The CE-150 has no text layer.\n"));
    h.server->finish();
}

TEST(lib8_ac32_harness_cut_paper_and_png) {
    SKIP_UNLESS(std::filesystem::exists("roms/CE-150.ROM"), "the ROMs are not fetched (roms/fetch_roms.sh)");
    calcu::testing::TempFolder t("calcu-1600-paper-png");
    PaperHarness h;
    CHECK(h.server != nullptr);
    if (!h.server) return;
    h.printOnCe150("LPRINT 1");
    const std::string png = (t.path() / "ce150.png").string();
    CHECK_EQ(h.reply("paper png " + png), std::string("done\n"));
    CHECK(std::filesystem::exists(png) && std::filesystem::file_size(png) > 100);
    CHECK_EQ(h.reply("cut-paper"), std::string("done\n"));
    CHECK_EQ(h.reply("paper png " + png), std::string("refused: The paper is empty.\n"));
    h.server->finish();
}

// LIB-10 design §10.10, D16-6 and D16-3 (AC-40, AC-52): the harness's run
// control and memory commands on the Sharp targets.
TEST(lib10_ac40_harness_pause_step_resume) {
    SKIP_UNLESS(std::filesystem::exists("roms/PC1600-P0-B0-new.bin"), "the ROMs are not fetched (roms/fetch_roms.sh)");
    PaperHarness h;
    CHECK(h.server != nullptr);
    if (!h.server) return;
    CHECK_EQ(h.reply("model PC-1600"), std::string("done\n"));
    CHECK(h.reply("pause").find("paused, owner cpu, reason pause, target SC7852") != std::string::npos);
    const std::string t0 = h.reply("ticks");
    h.reply("wait 100ms");
    CHECK_EQ(h.reply("ticks"), t0);   // paused: no emulated time passes
    CHECK(h.reply("step { level: cpu, count: 3 }").find("reason step") != std::string::npos);
    CHECK(h.reply("step-over").find("reason step") != std::string::npos);
    CHECK(h.reply("resume").find("running") != std::string::npos);
    h.reply("wait 100ms");
    CHECK(h.reply("ticks") != t0);
    h.server->finish();
}

TEST(lib10_ac40_harness_breakpoint_and_watch) {
    SKIP_UNLESS(std::filesystem::exists("roms/PC-1500_A04.ROM"), "the ROMs are not fetched (roms/fetch_roms.sh)");
    PaperHarness h;
    CHECK(h.server != nullptr);
    if (!h.server) return;
    h.reply("wait 1s");
    // The PC-1500A's ROM goes through its keyboard scan all the time.
    const std::string hist = h.reply("pause");
    CHECK(hist.find("paused") != std::string::npos);
    const std::string regs = h.reply("registers");
    const std::size_t at = regs.find("p = 0x");
    CHECK(at != std::string::npos);
    const std::string pc = regs.substr(at + 4, 6);
    CHECK(h.reply("break LH5801 " + pc).rfind("done", 0) == 0);
    CHECK(h.reply("resume").find("running") != std::string::npos);
    h.reply("wait 1s");
    CHECK(h.reply("state run-state").find("breakpoint") != std::string::npos);
    CHECK(h.reply("break off").rfind("done", 0) == 0);
    // A write watch on the RAM the ROM keeps writing while idle.
    CHECK(h.reply("watch LH5801 me0 0x7600 0x600 write").rfind("done", 0) == 0);
    CHECK(h.reply("resume").find("running") != std::string::npos);
    h.reply("wait 1s");
    CHECK(h.reply("state run-state").find("watch") != std::string::npos);
    CHECK(h.reply("watch off").rfind("done", 0) == 0);
    h.server->finish();
}

TEST(lib10_ac52_harness_peek_bank_and_vbank) {
    SKIP_UNLESS(std::filesystem::exists("roms/PC1600-P0-B0-new.bin"), "the ROMs are not fetched (roms/fetch_roms.sh)");
    PaperHarness h;
    CHECK(h.server != nullptr);
    if (!h.server) return;
    CHECK_EQ(h.reply("model PC-1600"), std::string("done\n"));
    CHECK(h.reply("peek SC7852 memory 0x8000 2 bank=6").rfind("done 2\n0x8000: 0x", 0) == 0);
    CHECK(h.reply("peek SC7852 io-written 0x31").rfind("done 1\n0x31: 0x", 0) == 0);
    CHECK(h.reply("peek SC7852 io 0x31").rfind("done 1\n0x31: 0x", 0) == 0);
    CHECK(h.reply("peek LH5803 me1-written 0x0000").find("none") != std::string::npos);
    CHECK(h.reply("peek SC7852 memory 0x8000 1 bank=2 vbank=1").rfind("done 1", 0) == 0);
    h.server->finish();
}
