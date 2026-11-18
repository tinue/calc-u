// The controls kind. The test printer's
// switches `online`, `trace` and key `feed`.
#include "TestHarness.hpp"
#include "TestMachines.hpp"
#include "peripherals/Controls.hpp"

using namespace calcu::machine;
using namespace calcu::machine::test;
using namespace calcu::peripherals;

namespace {
const PlacePath kAt{{PlaceName{"printer"}}};
const Owner kOwner = PeripheralOwner{kPrinter, kAt};

std::unique_ptr<TestMachine> withPrinter(std::vector<SwitchPosition> switches = {}) {
    PeripheralRequest r = peripheralAt(testCatalogue(), "printer", kPrinter);
    r.switches = std::move(switches);
    return buildTestMachine(kSeg, std::nullopt, {r});
}

bool stateOf(Machine& m, const char* id) {
    for (const auto& c : controlsOf(m, kOwner))
        if (c.id == ControlId{id}) return c.on;
    return false;
}
} // namespace

TEST(controls_listed_with_id_form_state) {
    auto m = withPrinter();
    CHECK(m != nullptr);
    if (!m) return;
    const auto entries = m->capabilities(Controls::kKind);
    CHECK(entries.size() == 1 && entries[0].owner == kOwner && entries[0].id == CapabilityId{"printer/controls"});
    const auto list = controlsOf(*m, kOwner);
    const std::vector<ControlInfo> want{{ControlId{"online"}, ControlForm::Switch, false},
                                        {ControlId{"feed"}, ControlForm::Key, false},
                                        {ControlId{"trace"}, ControlForm::Switch, false}};
    CHECK(list == want);   // the descriptor's order, IDs, forms, up and off
    // A control is named by owner and ID: another owner has its own.
    const Catalogue& cat = testCatalogue();
    auto dot = buildTestMachine(kDot);
    CHECK(dot && !dot->attach(peripheralAt(cat, "a", kHub), cat).refused);
    PeripheralRequest one = peripheralAt(cat, "a", kPrinter), two = one;
    one.place = PlacePath{{PlaceName{"a"}, PlaceName{"jack"}}};
    two.place = one.place;
    CHECK(dot && !dot->attach(one, cat).refused && !dot->attach(two, cat).refused);
    if (!dot) return;
    CHECK(controlsOf(*dot, PeripheralOwner{kPrinter, one.place}).size() == 3);
    // Without the peripheral, no controls.
    auto bare = buildTestMachine(kSeg);
    CHECK(bare && bare->capabilities(Controls::kKind).empty() && controlsOf(*bare, kOwner).empty());
}

TEST(control_press_release_at_tick_counts) {
    auto m = withPrinter();
    CHECK(m != nullptr);
    if (!m) return;
    Controls* c = controlsCapability(*m, kOwner);
    CHECK(c != nullptr);
    if (!c) return;
    m->advance(1000);
    const Ticks down = m->tickCount();
    CHECK(c->press(ControlId{"feed"}) == ControlCall::Done);
    CHECK(stateOf(*m, "feed"));
    m->advance(2500);
    const Ticks up = m->tickCount();
    CHECK(c->release(ControlId{"feed"}) == ControlCall::Done);
    CHECK(!stateOf(*m, "feed"));
    const std::vector<ControlEvent> want{{kAt, ControlId{"feed"}, true, down}, {kAt, ControlId{"feed"}, false, up}};
    CHECK(m->controlEvents() == want);
}

TEST(switch_position_seen_and_reported) {
    auto m = withPrinter();
    CHECK(m != nullptr);
    if (!m) return;
    Controls* c = controlsCapability(*m, kOwner);
    CHECK(c != nullptr);
    if (!c) return;
    CHECK(c->set(ControlId{"trace"}, true) == ControlCall::Done);
    CHECK(stateOf(*m, "trace") && !stateOf(*m, "online"));
    CHECK(m->controlEvents().size() == 1 && m->controlEvents()[0].id == ControlId{"trace"} && m->controlEvents()[0].on);
    CHECK(c->set(ControlId{"trace"}, false) == ControlCall::Done);
    CHECK(!stateOf(*m, "trace") && m->controlEvents().size() == 2 && !m->controlEvents()[1].on);
}

TEST(restore_from_request) {
    // The positions handed in with the attach: where the peripheral starts.
    auto m = withPrinter({SwitchPosition{ControlId{"trace"}, true}, SwitchPosition{ControlId{"feed"}, true},
                          SwitchPosition{ControlId{"no-such"}, true}});
    CHECK(m != nullptr);
    if (!m) return;
    CHECK(stateOf(*m, "trace") && !stateOf(*m, "online"));
    CHECK(!stateOf(*m, "feed"));   // a key is not a position
    CHECK(m->controlEvents().empty());
    // The same through attach, after a detach.
    const Catalogue& cat = testCatalogue();
    CHECK(!m->detach(kAt, kPrinter, cat).refused);
    PeripheralRequest r = peripheralAt(cat, "printer", kPrinter);
    r.switches = {SwitchPosition{ControlId{"online"}, true}};
    CHECK(!m->attach(r, cat).refused);
    CHECK(stateOf(*m, "online") && !stateOf(*m, "trace"));
}

TEST(unknown_control_refused) {
    auto m = withPrinter();
    CHECK(m != nullptr);
    if (!m) return;
    Controls* c = controlsCapability(*m, kOwner);
    CHECK(c != nullptr);
    if (!c) return;
    const auto before = c->list();
    CHECK(c->press(ControlId{"PRINT"}) == ControlCall::UnknownControl);
    CHECK(c->release(ControlId{"PRINT"}) == ControlCall::UnknownControl);
    CHECK(c->set(ControlId{"write-protect"}, true) == ControlCall::UnknownControl);
    CHECK(c->list() == before && m->controlEvents().empty());
}

TEST(wrong_form_refused) {
    auto m = withPrinter();
    CHECK(m != nullptr);
    if (!m) return;
    Controls* c = controlsCapability(*m, kOwner);
    CHECK(c != nullptr);
    if (!c) return;
    const auto before = c->list();
    CHECK(c->press(ControlId{"trace"}) == ControlCall::WrongForm);
    CHECK(c->set(ControlId{"feed"}, true) == ControlCall::WrongForm);
    CHECK(c->list() == before && m->controlEvents().empty());
}
