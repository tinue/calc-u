// The Calc-U-1600 peripherals on the machine interface: the places' pin-out
// rules, the combinations refused, the CE-158's link, the CE-161's switch
// and the real-time clocks. The serial links and the clock set by the
// session go through the product's machine session
// (app/tests/serial_links_tests.cpp).

#include <algorithm>

#include "SharpTestSupport.hpp"
#include "peripherals/Controls.hpp"
#include "peripherals/RealTimeClock.hpp"
#include "peripherals/SerialPort.hpp"

using namespace calcu::machine;
using namespace sharptest;
namespace pp = calcu::peripherals;

namespace {

bool haveAll() { return haveModelRoms("PC-1500A") && haveModelRoms("PC-1600"); }

const PinOutDescriptor* pinOutOf(const MachineDescriptor& d, const PlaceDescriptor& p) {
    const ConnectorDescriptor* c = d.connector(p.connector);
    if (!c) return nullptr;
    for (const auto& po : c->pinOuts)
        if (po.id == p.pinOut) return &po;
    return nullptr;
}

} // namespace

TEST(slots_one_chain_contact_level_power_cycle) {
    for (const char* model : {"PC-1500", "PC-1500A", "PC-1600"}) {
        const MachineDescriptor& d = *sharp::sharpCatalogue().model(ModelId{model});
        for (const PlaceDescriptor& p : d.places) {
            const PinOutDescriptor* po = pinOutOf(d, p);
            CHECK(po != nullptr);
            if (!po) continue;
            const bool slot = p.name.value.rfind("slot-", 0) == 0;
            CHECK(po->takes == (slot ? Takes::One : Takes::Chain));
            CHECK_EQ(p.takesAtOnce, p.name.value == "bus-rom" ? 1000 : 1);   // any number of bus ROMs
            CHECK(po->level == PinOutLevel::ContactLevel);
            CHECK(po->after == AfterChange::PowerCycle);
        }
    }
    SKIP_UNLESS(haveAll(), "Sharp ROMs missing");
    const Catalogue c = catalogueWithModules();
    for (const char* model : {"PC-1500A", "PC-1600"}) {
        // Not started yet: the change is made and nothing follows.
        auto fresh = buildSharp(c, model);
        CHECK(fresh->attach(request(c, "CE-150", "printer"), c).follows == AfterChange::Nothing);
        // Running: a power cycle follows an attach and a detach, on the
        // 60-pin connector and in a module slot.
        auto m = bootedSharp(c, model);
        const ChangeResult a = m->attach(request(c, "CE-158", "interface"), c);
        CHECK(!a.refused && a.follows == AfterChange::PowerCycle);
        const ChangeResult s = m->attach(moduleRequest("CE-161", "slot-1"), c);
        CHECK(!s.refused && s.follows == AfterChange::PowerCycle);
        CHECK(m->detach(place("interface"), PeripheralId{"CE-158"}, c).follows == AfterChange::PowerCycle);
        // Switched off: nothing follows, the change is made at once.
        m->keyDown(KeyId{"off"});
        m->advance(frameTicks(*m) * 4);
        m->keyUp(KeyId{"off"});
        CHECK(runUntil(*m, [&] { return !m->readDisplay().on; }, 3));
        CHECK(m->detach(place("slot-1"), PeripheralId{"CE-161"}, c).follows == AfterChange::Nothing);
        CHECK(sharptest::moduleIn(*m, 1) == nullptr);
    }
}

TEST(second_printer_refused) {
    SKIP_UNLESS(haveAll(), "Sharp ROMs missing");
    const Catalogue& c = sharp::sharpCatalogue();
    auto m = buildSharp(c, "PC-1600");
    CHECK(!m->attach(request(c, "CE-150", "printer"), c).refused);
    const ChangeResult r = m->attach(request(c, "CE-1600P", "printer"), c);
    CHECK(r.refused.has_value());
    if (r.refused) CHECK(!r.refused->reason.empty());
    const Plugged p = m->plugged();
    CHECK(p.byPlace.size() == 1 && p.byPlace[0].peripheral.value == "CE-150");
    CHECK(sharp::pc1600Of(*m)->ce150Attached() && !sharp::pc1600Of(*m)->ce1600pAttached());
    // The other way round, and a printer is changed with a replace.
    auto n = buildSharp(c, "PC-1600");
    CHECK(!n->attach(request(c, "CE-1600P", "printer"), c).refused);
    CHECK(n->attach(request(c, "CE-150", "printer"), c).refused.has_value());
    CHECK(!n->replace(PeripheralId{"CE-1600P"}, request(c, "CE-150", "printer"), c).refused);
    CHECK(sharp::pc1600Of(*n)->ce150Attached() && !sharp::pc1600Of(*n)->ce1600pAttached());
}

TEST(ce1600p_ce158_refused_both_ways) {
    SKIP_UNLESS(haveAll(), "Sharp ROMs missing");
    const Catalogue& c = sharp::sharpCatalogue();
    const std::string why = "The CE-1600P and the CE-158 cannot be used together.";
    {
        auto m = buildSharp(c, "PC-1600");
        CHECK(!m->attach(request(c, "CE-1600P", "printer"), c).refused);
        const ChangeResult r = m->attach(request(c, "CE-158", "interface"), c);
        CHECK(r.refused && r.refused->reason == why);
        CHECK(m->canAttach(request(c, "CE-158", "interface"), c).has_value());
        CHECK(sharp::pc1600Of(*m)->ce1600pAttached() && !sharp::pc1600Of(*m)->ce158Attached());
    }
    {
        auto m = buildSharp(c, "PC-1600");
        CHECK(!m->attach(request(c, "CE-158", "interface"), c).refused);
        const ChangeResult r = m->attach(request(c, "CE-1600P", "printer"), c);
        CHECK(r.refused && r.refused->reason == why);
        CHECK(sharp::pc1600Of(*m)->ce158Attached() && !sharp::pc1600Of(*m)->ce1600pAttached());
        CHECK_EQ(m->plugged().byPlace.size(), std::size_t{1});
    }
}

TEST(ce150_and_ce158_together) {
    SKIP_UNLESS(haveAll(), "Sharp ROMs missing");
    const Catalogue& c = sharp::sharpCatalogue();
    for (const char* model : {"PC-1500A", "PC-1600"}) {
        auto m = buildSharp(c, model);
        CHECK(!m->attach(request(c, "CE-150", "printer"), c).refused);
        CHECK(!m->attach(request(c, "CE-158", "interface"), c).refused);
        const Plugged p = m->plugged();
        CHECK_EQ(p.byPlace.size(), std::size_t{2});
        CHECK(p.byConnector.size() == 1 && p.byConnector[0].connector.value == "60-pin" &&
              p.byConnector[0].plugged.size() == 2);
        if (PC1500Machine* core = sharp::pc1500Of(*m)) CHECK(core->ce150Attached() && core->ce158Attached());
        if (PC1600Machine* core = sharp::pc1600Of(*m)) CHECK(core->ce150Attached() && core->ce158Attached());
        boot(*m);
        CHECK(runUntilKeyReady(*m, 10));
    }
}

namespace {

// The emulated times (seconds) at which the CE-158's serial port gave each
// byte of a program's `LPRINT` (SETDEV PO), sampled every 0.25 ms.
std::vector<double> ce158SendTimes(const char* model) {
    const Catalogue& c = sharp::sharpCatalogue();
    auto m = bootedSharp(c, model, {request(c, "CE-158", "interface")});
    if (!m) return {};
    const bool p16 = std::string(model) == "PC-1600";
    toMode(*m, "PRO");
    if (p16) {   // PC-1500 BASIC on the LH5803 (MODE 1), as the CE-158 needs
        typeLine(*m, "NEW");
        typeLine(*m, "MODE1", 2);
        toMode(*m, "PRO");
    }
    typeLine(*m, "10 SETDEV PO");
    typeLine(*m, "20 OUTSTAT 0");
    typeLine(*m, "30 LPRINT 1234567890");
    toMode(*m, "RUN");
    pp::SerialPort* port = calcu::machine::first<pp::SerialPort>(*m);
    if (p16)   // the PC-1600's RS-232C port comes first; the CE-158's is at `interface`
        for (const auto& e : m->capabilities(pp::SerialPort::kKind))
            if (std::holds_alternative<PeripheralOwner>(e.owner)) port = capabilityAs<pp::SerialPort>(*m, e.id);
    for (char ch : std::string("run")) tap(*m, std::string(1, ch));
    std::uint8_t drop[256];
    while (port && port->takeSent(drop) > 0) {}
    m->keyDown(KeyId{"enter"});   // the program starts from here; sampled from the first slice on
    std::vector<double> times;
    const Ticks slice = m->descriptor().tickRate / 4000;
    for (int i = 0; i < 4000 * 3 && times.size() < 11; ++i) {
        m->advance(slice);
        std::uint8_t b[16];
        const std::size_t n = port ? port->takeSent(b) : 0;
        for (std::size_t k = 0; k < n; ++k) times.push_back(m->elapsed().seconds().count());
        if (i == 8 * 4000 / 60) m->keyUp(KeyId{"enter"});
    }
    return times;
}

} // namespace

TEST(ce158_same_emulated_time_pc1500_pc1600) {
    SKIP_UNLESS(haveAll(), "Sharp ROMs missing");
    const std::vector<double> a = ce158SendTimes("PC-1500A");
    const std::vector<double> b = ce158SendTimes("PC-1600");
    CHECK(a.size() >= 11 && b.size() >= 11);
    if (a.size() < 11 || b.size() < 11) return;
    // The same bytes at the same baud rate take the same emulated time on
    // both, although the tick rates differ (P-2): byte to byte, and over the
    // whole line.
    for (std::size_t i = 1; i < 11; ++i) {
        const double da = a[i] - a[i - 1], db = b[i] - b[i - 1];
        if (std::abs(da - db) > 0.0006) std::fprintf(stderr, "  byte %zu: %.5f s vs %.5f s\n", i, da, db);
        CHECK(std::abs(da - db) <= 0.0006);
    }
    CHECK(std::abs((a[10] - a[0]) - (b[10] - b[0])) <= 0.0006);
}

TEST(ce161_switch_blocks_writes) {
    SKIP_UNLESS(haveAll(), "Sharp ROMs missing");
    const Catalogue c = catalogueWithModules();
    for (const char* model : {"PC-1500A", "PC-1600"}) {
        auto m = bootedSharp(c, model, {moduleRequest("CE-161", "slot-1")});
        const auto entries = m->capabilities(pp::Controls::kKind);
        CHECK_EQ(entries.size(), std::size_t{1});
        if (entries.empty()) continue;
        const auto* owner = std::get_if<PeripheralOwner>(&entries[0].owner);
        CHECK(owner && owner->peripheral.value == "CE-161" && owner->place == place("slot-1"));
        pp::Controls* controls = capabilityAs<pp::Controls>(*m, entries[0].id);
        CHECK(controls != nullptr);
        if (!controls) continue;
        const auto list = controls->list();
        CHECK(list.size() == 1 && list[0].id.value == "write-protect" && list[0].form == ControlForm::Switch);
        CHECK(controls->press(ControlId{"write-protect"}) == pp::ControlCall::WrongForm);
        CHECK(scribbleModule(*m, 1));   // off: writes land
        CHECK(controls->set(ControlId{"write-protect"}, true) == pp::ControlCall::Done);
        CHECK(sharptest::moduleIn(*m, 1)->writeProtected());
        CHECK(!scribbleModule(*m, 1));   // on: every write blocked
        CHECK(controls->set(ControlId{"write-protect"}, false) == pp::ControlCall::Done);
        CHECK(scribbleModule(*m, 1));
        // A module without the switch has no controls.
        auto n = buildSharp(c, model, std::nullopt, {moduleRequest("CE-163F", "slot-1")});
        CHECK(n->capabilities(pp::Controls::kKind).empty());
    }
}

TEST(rtc_on_three_models) {
    const Catalogue& c = sharp::sharpCatalogue();
    int built = 0;
    for (const char* model : {"PC-1500", "PC-1500A", "PC-1600"}) {
        auto m = buildSharp(c, model);
        if (!m) continue;
        ++built;
        const auto entries = m->capabilities(pp::RealTimeClock::kKind);
        CHECK_EQ(entries.size(), std::size_t{1});
        if (entries.empty()) continue;
        const auto* owner = std::get_if<ComponentOwner>(&entries[0].owner);
        CHECK(owner && owner->component.value == "rtc");
        CHECK(entries[0].id.value == "rtc/real-time-clock");
        pp::RealTimeClock* clock = capabilityAs<pp::RealTimeClock>(*m, entries[0].id);
        CHECK(clock != nullptr);
        if (!clock) continue;
        // The kept fields only: no year on either chip.
        clock->set(pp::ClockFields{2026, 11, 15, 0, 8, 9, 10});
        const pp::ClockFields f = clock->read();
        CHECK(!f.year.has_value());
        CHECK(f.month == 11 && f.day == 15 && f.hour == 8 && f.minute == 9 && f.second == 10);
        // setClocksFromHost reaches it.
        pp::setClocksFromHost(*m, pp::HostDateTime{2027, 3, 4, 4, 5, 6, 7, 0});
        const pp::ClockFields g = clock->read();
        CHECK(g.month == 3 && g.day == 4 && g.hour == 5 && g.minute == 6 && g.second == 7);
    }
    SKIP_UNLESS(built > 0, "Sharp ROMs missing");
}

TEST(clock_runs_on_emulated_time) {
    SKIP_UNLESS(haveAll(), "Sharp ROMs missing");
    const Catalogue& c = sharp::sharpCatalogue();
    for (const char* model : {"PC-1500A", "PC-1600"}) {
        auto m = bootedSharp(c, model);
        pp::RealTimeClock* clock = calcu::machine::first<pp::RealTimeClock>(*m);
        clock->set(pp::ClockFields{std::nullopt, 6, 1, std::nullopt, 10, 20, 30}, 0);
        // No advance, no change (wall time does not count).
        CHECK(clock->read().second == 30);
        m->advance(secondsTicks(*m, 5.0));
        const pp::ClockFields f = clock->read();
        CHECK(f.minute == 20 && f.second && *f.second >= 34 && *f.second <= 36);
        // While the machine is off, too.
        m->keyDown(KeyId{"off"});
        m->advance(frameTicks(*m) * 4);
        m->keyUp(KeyId{"off"});
        CHECK(runUntil(*m, [&] { return !m->readDisplay().on; }, 3));
        const int before = *clock->read().second;
        m->advance(secondsTicks(*m, 4.0));
        const int after = *clock->read().second;
        CHECK(((after - before + 60) % 60) >= 3 && ((after - before + 60) % 60) <= 5);
    }
}
