// C59-9 AC-15 (design §6, T10; LIB-11 AC-24): the TI machines offer the
// machine state. Each case saves while a program runs, lets 2 s pass and
// takes the replies (the display, the ticks, the paper, the whole state);
// then restores in the same run, and in a second run set up the same way
// after another time, and compares.
#include "../app/HarnessProduct.hpp"
#include "TestHarness.hpp"
#include "harness/Run.hpp"
#include "machine/Machine.hpp"
#include "media/MediaPlace.hpp"
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <string>
#include <vector>

using namespace calcu::harness;

namespace {

bool romsThere() { return std::filesystem::exists("roms/TMC0571B.txt"); }

struct Run {
    std::unique_ptr<HarnessProduct> product = cu59::makeHarnessProduct();
    std::unique_ptr<Server> server;
    explicit Run(const std::string& saveFolder) {
        RunOptions o;
        o.saveFolder = saveFolder;
        std::string error;
        server = Server::start(*product, o, &error);
        if (!server) std::fprintf(stderr, "%s\n", error.c_str());
    }
    ~Run() {
        if (server) server->finish();
    }
    // `feed card-reader`: the session's media.feed of the armed card, as the
    // card window's tap does; it returns while the card passes.
    std::string operator()(const std::string& l) {
        if (l == "feed card-reader") {
            const auto r = server->host().command(calcu::sessions::Command{
                calcu::sessions::CommandName{"media.feed"},
                calcu::sessions::Value::object({{"place", "card-reader"}, {"side", static_cast<std::int64_t>(0)}})});
            return r.done ? "done\n" : "refused: " + r.reason + "\n";
        }
        return server->line(l).text;
    }
};

std::string folder(const std::string& name) {
    const auto p = std::filesystem::absolute("build/c59_9_state/" + name);
    std::filesystem::remove_all(p);
    std::filesystem::create_directories(p);
    return p.string();
}

std::vector<std::uint8_t> bytesOf(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}

// What the machine shows after `wait 2s`: the replies and the state then.
// The paper is LIB-8's output, not machine state: the lines printed in the
// 2 s are compared.
std::string after2s(Run& r, const std::string& dir, const std::string& name, bool paper) {
    const std::string before = paper ? r("paper text") : std::string();
    std::string s = r("wait 2s") + r("lcd") + r("display") + r("ticks");
    if (paper) {
        const std::string now = r("paper text");
        const auto body = [](const std::string& t) { return t.substr(std::min(t.size(), t.find('\n') + 1)); };
        const std::string b = body(before), n = body(now);
        s += n.rfind(b, 0) == 0 ? n.substr(b.size()) : "the paper does not go on from before:\n" + n;
    }
    r("state-save " + dir + "/" + name);
    const auto b = bytesOf(dir + "/" + name);
    return s + std::string(b.begin(), b.end());
}

// A setup's commands, then a save while the program runs; the replies 2 s
// later, after a restore in the same run, and in a second run restored after
// another time all equal.
void restoresIdentically(const std::string& name, const std::vector<std::string>& setup, bool paper) {
    const std::string dir = folder(name);
    std::string first, same, other;
    {
        Run r(dir);
        if (!r.server) return;
        for (const auto& l : setup) CHECK_EQ(r(l).rfind("refused", 0), std::string::npos);
        if (name == "card") {   // the card is in the reader, passing, at the save
            auto* m = r.server->host().session().context().machine();
            bool passing = false;
            for (const auto& e : m->capabilities(calcu::media::MediaPlace::kKind))
                if (auto* p = calcu::machine::capabilityAs<calcu::media::MediaPlace>(*m, e.id)) passing = passing || p->side().has_value();
            CHECK(passing);
        }
        CHECK_EQ(r("state-save " + dir + "/at.state"), std::string("done\n"));
        first = after2s(r, dir, "a.state", paper);
        r("wait 1s");
        CHECK_EQ(r("state-restore " + dir + "/at.state"), std::string("done\n"));
        same = after2s(r, dir, "b.state", paper);
        // A save right after a restore gives the bytes restored.
        CHECK_EQ(r("state-restore " + dir + "/at.state"), std::string("done\n"));
        r("state-save " + dir + "/again.state");
        CHECK(bytesOf(dir + "/again.state") == bytesOf(dir + "/at.state"));
    }
    {
        Run r(dir);
        if (!r.server) return;
        for (const auto& l : setup) r(l);
        r("wait 1337ms");
        CHECK_EQ(r("state-restore " + dir + "/at.state"), std::string("done\n"));
        other = after2s(r, dir, "c.state", paper);
    }
    CHECK(!first.empty());
    if (first != same) {
        std::size_t i = 0;
        while (i < first.size() && i < same.size() && first[i] == same[i]) ++i;
        std::fprintf(stderr, "%s: differs at %zu of %zu/%zu:\n%s\n---\n%s\n", name.c_str(), i, first.size(), same.size(),
                     first.substr(0, std::min<std::size_t>(first.size(), 600)).c_str(), same.substr(0, std::min<std::size_t>(same.size(), 600)).c_str());
    }
    CHECK(first == same);
    CHECK(first == other);
}

const char* kLoop = "ti59 program 000 76 11 42 00 76 12 97 00 12 91";   // Lbl A STO 00 Lbl B Dsz 0 B R/S

} // namespace

TEST(c59_9_ac15_restore_continues_identically_ti59) {
    SKIP_UNLESS(romsThere(), "the ROMs are not fetched (roms/fetch_roms.sh)");
    restoresIdentically("ti59", {kLoop, "key 5 0 0 A", "wait 500ms"}, false);
}

TEST(c59_9_ac15_restore_continues_identically_ti58) {
    SKIP_UNLESS(romsThere(), "the ROMs are not fetched (roms/fetch_roms.sh)");
    restoresIdentically("ti58", {"model TI-58", kLoop, "key 5 0 0 A", "wait 500ms"}, false);
}

TEST(c59_9_ac15_restore_continues_identically_ti58c) {
    SKIP_UNLESS(romsThere(), "the ROMs are not fetched (roms/fetch_roms.sh)");
    // The TI-58C's constant memory is its RAM (C59-3): a data register written before.
    restoresIdentically("ti58c", {"model TI-58C", "ti59 registers 07=3.25", kLoop, "key 5 0 0 A", "wait 500ms"}, false);
}

TEST(c59_9_ac15_with_printer_module_and_card_in_reader) {
    SKIP_UNLESS(romsThere(), "the ROMs are not fetched (roms/fetch_roms.sh)");
    // A PC-100C printing in a loop (Lbl A Op 00 1 Prt GTO A), the ML module in slot-1.
    restoresIdentically("printer", {"attach printer PC-100C", "ti59 program 000 76 11 69 00 01 99 61 11",
                                    "key A", "wait 700ms"}, true);
    // A card passing the reader: a program written onto side 0 of a card.
    restoresIdentically("card", {"ti59 program 000 76 11 01 02 03 91", "arm { mem: X }", "key 1 Write", "feed card-reader",
                                 "wait 20ms"}, false);
}

TEST(c59_9_ac15_refused_for_other_peripherals) {
    SKIP_UNLESS(romsThere(), "the ROMs are not fetched (roms/fetch_roms.sh)");
    const std::string dir = folder("other");
    Run r(dir);
    if (!r.server) return;
    r("attach printer PC-100C");
    CHECK_EQ(r("state-save " + dir + "/p.state"), std::string("done\n"));
    r("detach printer");
    const std::string before = r("ticks");
    const std::string refused = r("state-restore " + dir + "/p.state");
    CHECK(refused.rfind("refused: ", 0) == 0);
    CHECK_EQ(r("ticks"), before);   // nothing changed
    r("model TI-58");
    CHECK(r("state-restore " + dir + "/p.state").rfind("refused: ", 0) == 0);
}
