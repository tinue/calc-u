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
