// The place for other areas and products; the product's rules.
#include "PresetRig.hpp"
#include "TestHarness.hpp"

using namespace calcu::presets;
using namespace calcu::presets::test;
namespace machine = calcu::machine;
namespace peripherals = calcu::peripherals;

namespace {
bool traceOn(mt::TestMachine* m) {
    const machine::PeripheralOwner owner{mt::kPrinter, machine::PlacePath{{machine::PlaceName{"printer"}}}};
    for (const auto& c : peripherals::controlsOf(*m, owner))
        if (c.id == machine::ControlId{"trace"}) return c.on;
    return false;
}
} // namespace

TEST(header_key_section_step_of_a_product_read_checked_carried_out) {
    Reading rd;
    const auto read = rd.read(header() + "printer: test-printer\ntest-header:\n  level: 7\n  note: product's own\n");
    const LoadPlan* p = std::get_if<LoadPlan>(&read);
    CHECK(p && p->productHeader.count("test-header") == 1);
    if (p) CHECK_EQ(static_cast<const TestHeader*>(p->productHeader.at("test-header").get())->level, std::int64_t(7));
    Rig r;
    const CallId c = r.load(header() +
                            "keys:\n  - printer-trace: on\n"
                            "test-memory:\n  at: 2\n  bytes: |\n    C0 DE\n"
                            "printer: test-printer\n"   // the product's step needs it, wherever it stands
                            "test-header: { level: 1 }\n");
    CHECK(r.done(c));
    CHECK(traceOn(r.machine()));
    CHECK(r.machine()->memory()->bytes[2] == 0xC0 && r.machine()->memory()->bytes[3] == 0xDE);
}

TEST(product_keyword_errors_with_line) {
    Reading rd;
    CHECK_EQ(rd.error(header() + "test-header:\n  level: high\n"), std::string("t.test-seg, line 4: level: is a number."));
    CHECK_EQ(rd.error(header() + "test-memory:\n  at: 0\n  bytes: |\n    0G\n"),
             std::string("t.test-seg, line 6: bytes: are hex pairs."));
    CHECK_EQ(rd.error(header() + "keys:\n  - printer-trace: on\n"),
             std::string("t.test-seg, line 4: printer-trace: needs printer: test-printer."));
    CHECK_EQ(rd.error(header() + "printer: test-printer\nkeys:\n  - printer-trace: maybe\n"),
             std::string("t.test-seg, line 5: printer-trace: takes on or off."));
}

TEST(idle_after_build_picture_from_the_rules) {
    Rig r;
    const int before = r.rules.seg.idleAsked;
    CHECK(r.done(r.load(header() + "keys:\n  - key: 2\n  - screenshot: a.png\n  - key: 3\n")));
    CHECK(r.rules.seg.idleAsked > before);   // idle by the product's rule
    CHECK_EQ(r.rules.seg.afterBuilds, 1);    // after the build, before the first section
    CHECK_EQ(r.rules.seg.pictures, 1);       // the product's picture
    // The rules decide: a stretch of 50 ms makes the next key wait that long.
    r.rules.seg.rule.stretch = calcu::machine::Seconds(0.050);
    CHECK(r.done(r.load(header() + "keys:\n  - key: 2\n  - key: 3\n")));
    CHECK(r.downs("3")[0] >= r.downs("2")[0] + 5'000);
    // An after-build step that fails stops the preset before its first item.
    r.rules.seg.after = [](RunContext&) -> calcu::sessions::Task<ItemResult> {
        co_return ItemResult{false, "NEW0? :CHECK was not answered.", {}};
    };
    const CallId c = r.load(header() + "keys:\n  - key: 2\n");
    CHECK(!r.done(c));
    CHECK(hasLine({r.reason(c)}, "t.test-seg, line 2: NEW0? :CHECK was not answered."));
    CHECK(r.downs("2").empty());
}

TEST(absent_keyword_default_nothing_carried_over) {
    Rig r;
    CHECK(r.done(r.load(header() + "printer: test-printer\ntest-memory: { at: 0, bytes: FF }\nkeys:\n"
                                   "  - printer-trace: on\n  - key: 2\n")));
    CHECK(traceOn(r.machine()));
    CHECK(r.machine()->memory()->bytes[0] == 0xFF);
    CHECK(r.done(r.load(header() + "printer: test-printer\nkeys:\n  - key: 2\n")));
    CHECK(!traceOn(r.machine()));                       // printer-trace absent: off
    CHECK(r.machine()->memory()->bytes[0] == 0);        // the cold boot's state
    CHECK_EQ(r.machine()->presses(), std::uint64_t(1));
}
