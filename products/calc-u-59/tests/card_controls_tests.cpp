// The card controls of the bar as app/TIControlRules declares them: the
// declaration itself, and the product entry `c59.card-controls` the app
// session's control state carries while it lists the card reader.

#include "TestHarness.hpp"
#include "CardRig.hpp"
#include "../app/TIControlRules.hpp"
#include "../app/TIShellProduct.hpp"
#include "control/ControlState.hpp"
#include "sessions/LibraryTopics.hpp"

using namespace cu59;
using namespace std::chrono_literals;
using testroms::CardRig;
using calcu::sessions::Value;

namespace {

bool roms() { return testroms::romSet() != nullptr; }
#define NEEDS_ROMS SKIP_UNLESS(roms(), "the ROMs are not fetched (roms/fetch_roms.sh)")

std::vector<CardControl> controlsOf(CardRig& r) {
    const auto* cs = r.state<calcu::control::ControlState>(calcu::control::kControlStateTopic);
    return cs ? cardControlsOf(*cs) : std::vector<CardControl>{};
}
bool listed(CardRig& r) {
    const auto* cs = r.state<calcu::control::ControlState>(calcu::control::kControlStateTopic);
    return cs && cs->productEntry(kCardControlsEntry) != nullptr;
}
std::vector<std::string> ids(const std::vector<CardControl>& controls) {
    std::vector<std::string> out;
    for (const auto& c : controls) out.push_back(c.id);
    return out;
}

} // namespace

TEST(card_controls_arm_while_no_card_passes) {
    const auto c = cardControls(false, {});
    CHECK(ids(c) == (std::vector<std::string>{kItemArmCard, kItemArmFreshCard}));
    if (c.size() != 2) return;
    CHECK(c[0].label == "Crd" && c[1].label == "Crd");   // the icon and the name tell them apart
    CHECK(c[0].name == "Arm Card" && c[1].name == "Arm Fresh Card");
    CHECK(c[0].icon != c[1].icon && !c[0].icon.empty());
    CHECK(c[0].available && c[1].available && c[0].reason.empty());
}

TEST(card_controls_unavailable_with_the_insert_reason) {
    const auto c = cardControls(false, {false, calcu::sessions::kAnotherOperationRuns});
    CHECK_EQ(c.size(), std::size_t{2});
    for (const auto& x : c) {
        CHECK(!x.available);
        CHECK_EQ(x.reason, std::string(calcu::sessions::kAnotherOperationRuns));
    }
}

TEST(card_controls_eject_while_a_card_passes) {
    const auto c = cardControls(true, {false, "ignored"});
    CHECK(ids(c) == (std::vector<std::string>{kCardControlEject}));
    if (c.empty()) return;
    CHECK(c[0].label == "Eject Card" && c[0].name == "Eject Card" && c[0].available);
}

TEST(card_controls_entry_follows_the_session) {
    NEEDS_ROMS;
    CardRig r;
    r.runFor(50ms);
    CHECK(ids(controlsOf(r)) == (std::vector<std::string>{kItemArmCard, kItemArmFreshCard}));
    CHECK(!controlsOf(r).empty() && controlsOf(r)[0].available);
    // An operation runs: Arm cannot be used, the reason the control state's.
    const auto load = r.post(calcu::presets::kLoadPresetCommand,
                             Value::object({{"content", "format-version: 2\nmodel: TI-59\nkeys:\n  - wait: 3s\n"}, {"name", "busy.ti59"}}));
    CHECK(r.until([&] {
        const auto c = controlsOf(r);
        return !c.empty() && !c[0].available && c[0].reason == calcu::sessions::kAnotherOperationRuns;
    }, 5s));
    r.until([&] { return r.reply(load) != nullptr; }, 30s);
    // A card passes: Eject Card alone; after the pass Arm again.
    CHECK(r.load(CardRig::preset(testroms::kProgramA)).done);
    CHECK(r.keyWrite(1));
    CHECK(r.armFresh("Controls").done);
    CHECK(r.feed(0).done);
    CHECK(r.until([&] { return ids(controlsOf(r)) == std::vector<std::string>{kCardControlEject}; }, 5s));
    CHECK(r.untilPassed());
    CHECK(r.until([&] {
        const auto c = controlsOf(r);
        return ids(c) == std::vector<std::string>{kItemArmCard, kItemArmFreshCard} && c[0].available;
    }, 5s));
    // No card reader, no card controls.
    CHECK(r.model("TI-58").done);
    r.runFor(50ms);
    CHECK(!listed(r));
}
