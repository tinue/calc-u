// The exit check of M-28 (LIB-11 design §6, T14): `calcu` drives a TI-59
// headless, through the library's app session (the product ACs of LIB-11,
// AC-14 to AC-17, are tested in M-59).
#include "../app/HarnessProduct.hpp"
#include "TestHarness.hpp"
#include "harness/Run.hpp"
#include <filesystem>
#include <string>

using namespace calcu::harness;

TEST(harness_drives_a_ti59) {
    SKIP_UNLESS(std::filesystem::exists("roms/TMC0571B.txt"), "the ROMs are not fetched (roms/fetch_roms.sh)");
    auto product = cu59::makeHarnessProduct();
    std::string error;
    auto server = Server::start(*product, {}, &error);
    CHECK(server != nullptr);
    if (!server) {
        std::fprintf(stderr, "%s\n", error.c_str());
        return;
    }
    auto reply = [&](const std::string& l) { return server->line(l).text; };
    CHECK(reply("wait 2s").rfind("done 2\nticks: ", 0) == 0);
    CHECK(reply("lcd").rfind("done 2\n          0 \nindicators:", 0) == 0);   // as ti59.py display() shows it
    CHECK_EQ(reply("key-down 7"), std::string("done\n"));
    reply("wait 100ms");
    CHECK_EQ(reply("key-up 7"), std::string("done\n"));
    reply("wait 500ms");
    CHECK_EQ(reply("lcd"), std::string("done 2\n          7 \nindicators:\n"));
    const std::string t = reply("ticks");
    CHECK(t.rfind("done 2\nticks: ", 0) == 0 && t.find("seconds: 2.6") != std::string::npos);
    CHECK_EQ(reply("audio x.wav"), std::string("refused: This machine has no sound.\n"));   // EC-6 on a TI
    server->finish();
}

// LIB-18 design P59 (LIB-11 AC-2 through a product): the harness switches
// the model with LIB-18's `model`.
TEST(harness_switches_to_a_ti58) {
    SKIP_UNLESS(std::filesystem::exists("roms/TMC0571B.txt"), "the ROMs are not fetched (roms/fetch_roms.sh)");
    auto product = cu59::makeHarnessProduct();
    std::string error;
    auto server = Server::start(*product, {}, &error);
    CHECK(server != nullptr);
    if (!server) {
        std::fprintf(stderr, "%s\n", error.c_str());
        return;
    }
    auto reply = [&](const std::string& l) { return server->line(l).text; };
    CHECK_EQ(reply("model TI-58"), std::string("done\n"));
    CHECK(reply("lcd").rfind("done 2\n          0 \nindicators:", 0) == 0);   // ready for keys
    CHECK(reply("capabilities").find("place printer PC-100C\n") != std::string::npos);   // the harness's default (D-17)
    const auto* m = server->host().session().context().machine();
    CHECK(m && m->model().value == "TI-58");
    CHECK_EQ(reply("model ti-59"), std::string("done\n"));
    CHECK_EQ(reply("model TI-99"), std::string("refused: There is no model \"TI-99\".\n"));
    server->finish();
}

// LIB-5 (design §6, L12 on a TI; AC-15, AC-16, AC-17, AC-26; D-8): `key`
// presses the names of preset.md §8.1 as the app types them and replies when
// the last key is up; a TI model has no `type`.
TEST(lib5_harness_key_on_a_ti59) {
    SKIP_UNLESS(std::filesystem::exists("roms/TMC0571B.txt"), "the ROMs are not fetched (roms/fetch_roms.sh)");
    auto product = cu59::makeHarnessProduct();
    std::string error;
    auto server = Server::start(*product, {}, &error);
    CHECK(server != nullptr);
    if (!server) return;
    auto reply = [&](const std::string& l) { return server->line(l).text; };
    CHECK_EQ(reply("key 2 x^2"), std::string("done\n"));
    reply("wait 300ms");
    CHECK_EQ(reply("lcd"), std::string("done 2\n          4. \nindicators:\n"));   // a result: x² of 2
    CHECK_EQ(reply("key 1 S+"), std::string("refused: Unknown key: S+.\n"));   // AC-26: an old name; nothing typed
    CHECK_EQ(reply("key CLR 2nd pi"), std::string("done\n"));                  // 2nd before a second function: once
    reply("wait 300ms");
    CHECK(reply("lcd").find("3.14159265") != std::string::npos);
    CHECK_EQ(reply("type 12"), std::string("refused: Unknown command: type.\n"));   // D-8
    server->finish();
}

// LIB-8 AC-23 (design §11.6, E8-7): the harness's `paper` commands on the
// PC-100C's paper since M-53.
TEST(lib8_ac23_harness_paper_text_of_the_pc100c) {
    SKIP_UNLESS(std::filesystem::exists("roms/TMC0571B.txt"), "the ROMs are not fetched (roms/fetch_roms.sh)");
    auto product = cu59::makeHarnessProduct();
    std::string error;
    auto server = Server::start(*product, {}, &error);
    CHECK(server != nullptr);
    if (!server) return;
    auto reply = [&](const std::string& l) { return server->line(l).text; };
    CHECK_EQ(reply("paper text"), std::string("done\n"));   // empty paper (EC-2)
    reply("key 7");
    reply("key 2nd Prt");
    reply("wait 1s");
    CHECK_EQ(reply("paper text"), std::string("done 1\n          7.        \n"));
    CHECK_EQ(reply("paper text PC-100C"), std::string("done 1\n          7.        \n"));
    server->finish();
}

TEST(lib8_ac23_harness_paper_marks_and_png) {
    SKIP_UNLESS(std::filesystem::exists("roms/TMC0571B.txt"), "the ROMs are not fetched (roms/fetch_roms.sh)");
    auto product = cu59::makeHarnessProduct();
    std::string error;
    auto server = Server::start(*product, {}, &error);
    CHECK(server != nullptr);
    if (!server) return;
    auto reply = [&](const std::string& l) { return server->line(l).text; };
    reply("key 7");
    reply("key 2nd Prt");
    reply("wait 1s");
    const std::string marks = reply("paper marks");
    CHECK(marks.rfind("done ", 0) == 0);
    CHECK(marks.find("\nreach 0 10.18\n") != std::string::npos);
    CHECK(marks.find("\ndot ") != std::string::npos);
    const std::filesystem::path png = std::filesystem::absolute(std::filesystem::path("build") / "lib8-ac23.png");
    std::filesystem::remove(png);
    CHECK_EQ(reply("paper png " + png.string()), std::string("done\n"));
    CHECK(std::filesystem::exists(png));
    CHECK_EQ(reply("cut-paper"), std::string("done\n"));
    CHECK_EQ(reply("paper text"), std::string("done\n"));
    server->finish();
}

// LIB-10 AC-31 (E10-4): a breakpoint set through the harness pauses a TI.
TEST(lib10_ac31_harness_breakpoint_pauses_the_ti) {
    SKIP_UNLESS(std::filesystem::exists("roms/TMC0571B.txt"), "the ROMs are not fetched (roms/fetch_roms.sh)");
    auto product = cu59::makeHarnessProduct();
    std::string error;
    auto server = Server::start(*product, {}, &error);
    CHECK(server != nullptr);
    if (!server) return;
    auto reply = [&](const std::string& l) { return server->line(l).text; };
    CHECK(reply("targets").find("TMC0501") != std::string::npos);
    CHECK(reply("pause").rfind("done", 0) == 0);
    const std::string regs = reply("registers");
    const auto at = regs.find("\npc = ");
    CHECK(at != std::string::npos);
    if (at == std::string::npos) {
        std::fprintf(stderr, "%s\n", regs.c_str());
        return;
    }
    CHECK(reply("resume").rfind("done", 0) == 0);
    reply("wait 100ms");
    const std::string pc = regs.substr(at + 6, regs.find('\n', at + 1) - at - 6);
    CHECK(reply("break TMC0501 " + pc).rfind("done", 0) == 0);
    reply("wait 100ms");
    const std::string state = reply("state run-state");
    CHECK(state.find("breakpoint") != std::string::npos);
    if (state.find("breakpoint") == std::string::npos) std::fprintf(stderr, "%s\n", state.c_str());
    CHECK(reply("history TMC0501 3").rfind("done", 0) == 0);
    server->finish();
}
