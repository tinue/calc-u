// The exit check of M-28 (LIB-11 design §6, T14; D-1): `calcu` drives the
// Sharp machine of the default start, the PC-1500A, headless through the
// library's app session (the PC-1600 follows with LIB-18's model switch;
// the product ACs of LIB-11, AC-18 to AC-20, are tested in M-51).
#include "../app/HarnessProduct.hpp"
#include "TestHarness.hpp"
#include "harness/Run.hpp"
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
