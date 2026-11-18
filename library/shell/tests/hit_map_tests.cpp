// The hit rule and PointerTracker. `hit_rule_cases_of_the_file` reads every line of
// hit_cases.txt, which shell_model.test.js reads as well.
#include "TestHarness.hpp"
#include "TestShellPart.hpp"
#include "shell/HitMap.hpp"
#include <fstream>
#include <sstream>
#include <string>

#ifndef CALCU_SHELL_FIXTURES
#error "CALCU_SHELL_FIXTURES is the folder of hit_cases.txt (library/CMakeLists.txt)"
#endif

using namespace calcu;
using namespace calcu::shell;
namespace st = calcu::shell::test;
using sessions::Value;

namespace {

std::string describe(const PointerTracker::Out& o) {
    std::string s;
    for (const auto& c : o.commands) s += (s.empty() ? "" : " ") + c.name.value + " " + toText(c.args);
    return s;
}

} // namespace

TEST(hit_rule_cases_of_the_file) {
    std::ifstream in(std::string(CALCU_SHELL_FIXTURES) + "/hit_cases.txt");
    CHECK(in.good());
    HitMap map;
    std::string line;
    int cases = 0, lineNo = 0;
    while (std::getline(in, line)) {
        ++lineNo;
        std::istringstream words(line);
        std::string verb;
        if (!(words >> verb) || verb[0] == '#') continue;
        if (verb == "map") {
            map = HitMap{};
            words >> map.distance;
            std::string key;
            while (words >> key) {
                const auto colon = key.find(':');
                Rect r;
                char comma = 0;
                std::istringstream nums(key.substr(colon + 1));
                nums >> r.x >> comma >> r.y >> comma >> r.w >> comma >> r.h;
                map.keys.emplace_back(machine::KeyId{key.substr(0, colon)}, r);
            }
        } else if (verb == "at") {
            double scale = 1, px = 0, py = 0;
            std::string expected;
            words >> scale >> px >> py >> expected;
            const auto k = keyAt(map, scale, px, py);
            const std::string got = k ? k->value : "-";
            if (got != expected) std::fprintf(stderr, "hit_cases.txt:%d: %s gives %s\n", lineNo, line.c_str(), got.c_str());
            CHECK_EQ(got, expected);
            ++cases;
        }
    }
    CHECK(cases >= 25);
}

TEST(several_pointers_hold_keys) {
    PointerTracker t(st::hitMap());
    // Key 1 at (5, 25), key 2 at (25, 25), in hit-map units at scale 1.
    CHECK_EQ(describe(t.down(1, 5, 25)), std::string("screen-key-down {\"key\":\"1\",\"pointer\":1}"));
    CHECK_EQ(describe(t.down(2, 25, 25)), std::string("screen-key-down {\"key\":\"2\",\"pointer\":2}"));
    CHECK_EQ(describe(t.down(3, 6, 26)), std::string("screen-key-down {\"key\":\"1\",\"pointer\":3}"));   // two hold key 1
    CHECK_EQ(describe(t.move(1, 45, 25)), std::string(""));   // a move changes no key
    CHECK_EQ(describe(t.up(1)), std::string("screen-key-up {\"key\":\"1\",\"pointer\":1}"));
    CHECK_EQ(describe(t.up(1)), std::string(""));   // once
    auto rest = t.releaseAll();   // the window loses focus
    CHECK_EQ(describe(rest), std::string("screen-key-up {\"key\":\"2\",\"pointer\":2} screen-key-up {\"key\":\"1\",\"pointer\":3}"));
    // The scale of the faceplate as shown now.
    double scale = 2;
    PointerTracker big(st::hitMap(), [&] { return scale; });
    CHECK_EQ(describe(big.down(7, 50, 50)), std::string("screen-key-down {\"key\":\"2\",\"pointer\":7}"));
}

TEST(display_hold_full_speed_until_up_or_leave) {
    PointerTracker t(st::hitMap());
    CHECK_EQ(describe(t.down(1, 10, 5)), std::string("speed {\"full\":true}"));
    CHECK_EQ(describe(t.move(1, 20, 6)), std::string(""));   // still on the display
    CHECK_EQ(describe(t.up(1)), std::string("speed {\"full\":false}"));
    // Leaving the display ends it; coming back does not start it again.
    t.down(2, 10, 5);
    CHECK_EQ(describe(t.move(2, 10, 40)), std::string("speed {\"full\":false}"));
    CHECK_EQ(describe(t.move(2, 10, 5)), std::string(""));
    CHECK_EQ(describe(t.up(2)), std::string(""));
    // Two pointers on the display: regular speed when the last lets go.
    CHECK_EQ(describe(t.down(3, 10, 5)), std::string("speed {\"full\":true}"));
    CHECK_EQ(describe(t.down(4, 30, 5)), std::string(""));
    CHECK_EQ(describe(t.up(3)), std::string(""));
    CHECK_EQ(describe(t.up(4)), std::string("speed {\"full\":false}"));
}

TEST(no_key_at_down_nothing_on_move) {
    PointerTracker t(st::hitMap());
    CHECK_EQ(describe(t.down(1, 15, 40)), std::string(""));   // below the keys, out of reach
    CHECK_EQ(describe(t.move(1, 5, 25)), std::string(""));    // onto key 1: still nothing
    CHECK_EQ(describe(t.up(1)), std::string(""));
}
