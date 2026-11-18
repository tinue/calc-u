// LIB-14 1600 Q1 (design Part D, D.5, D.8): what Calc-U-1600 gives the
// shell, checked against each model's control state, and the help address
// and the About texts (AC-39).
#include "SessionFixture.hpp"
#include "TestHarness.hpp"
#include "app/SharpShellProduct.hpp"
#include "control/ControlState.hpp"
#include "shell/ValueForms.hpp"

#include <cstdio>
#include <string>
#include <vector>

using sessionfixture::SessionFixture;
using sessionfixture::romsPresent;

TEST(lib14_ac34_shell_product_complete) {
    SKIP_UNLESS(romsPresent(), "no ROMs in roms/");
    const calcu::shell::ShellProduct p = cu1600::shellProduct("1.0", "test", true);
    SessionFixture f;
    for (const char* model : {"PC-1500", "PC-1500A", "PC-1600"}) {
        CHECK(model == std::string("PC-1500") ? f.start(model) : f.model(model));
        if (std::string(model) == "PC-1600") CHECK(f.attach("CE-1600P"));
        const auto* cs = f.state<calcu::control::ControlState>(calcu::control::kControlStateTopic);
        CHECK(cs != nullptr);
        if (!cs) continue;
        const std::vector<std::string> problems = calcu::shell::checkShellProduct(p, cs->toValue());
        for (const auto& s : problems) std::fprintf(stderr, "  %s: %s\n", model, s.c_str());
        CHECK(problems.empty());
    }
    std::vector<std::string> items;
    for (const auto& i : p.items) items.push_back(i.label);
    CHECK(items == (std::vector<std::string>{"Load BASIC Program…", "Load Machine Code…", "Host Drive…", "Unmount"}));
    CHECK(cu1600::shellProduct("1.0", "", false).items.size() == 2);   // AC-35: no host folders on the Web
}

TEST(lib14_ac39_help_address_and_about_texts) {
    const calcu::shell::ShellProduct p = cu1600::shellProduct("0.9.0", "abc123", true);
    CHECK_EQ(p.helpUrl, std::string("https://calc-u.org/1600/"));
    const calcu::shell::AboutModel a = calcu::shell::about(p);
    CHECK_EQ(a.name, std::string("Calc-U-1600"));
    CHECK_EQ(a.version, std::string("Version 0.9.0 (abc123)"));
    CHECK(a.lines.size() == 2 && a.lines[0] == "A Sharp PC-1500/1500A/1600 pocket-computer emulator.");
    CHECK(p.aboutLabel == "About Calc-U-1600…");
}

// LIB-14 Part E L13 (design §10.3.3, D-37): the page's forms of Calc-U-1600
// stay as they were before M-57: the members and their order of M-49's form,
// `copyScreen` true (the placeholder act), no `aboutLinks`, items with
// exactly {group, id, label, entry}, and no `choices` in the settings spec.
TEST(lib14_calc_u_1600_forms_unchanged) {
    using calcu::sessions::Value;
    const Value v = calcu::shell::toValue(cu1600::shellProduct("1.0", "", false));
    std::vector<std::string> keys;
    for (const auto& m : *v.members()) keys.push_back(m.key);
    CHECK(keys == (std::vector<std::string>{"name", "helpUrl", "aboutLabel", "aboutLines", "variantLabels", "presetFilter",
                                            "copyScreen", "pasteText", "moduleSlotPinOuts", "placesWithoutControl", "items"}));
    CHECK(*v.find("copyScreen")->boolean());
    for (const Value& item : *v.find("items")->elements()) {
        std::vector<std::string> fields;
        for (const auto& m : *item.members()) fields.push_back(m.key);
        CHECK(fields == (std::vector<std::string>{"group", "id", "label", "entry"}));
    }
    for (bool qt6 : {false, true}) {
        const Value spec = calcu::shell::toValue(cu1600::settingsSpec(qt6, true));
        for (const Value& s : *spec.elements())
            for (const Value& r : *s.find("rows")->elements()) {
                std::vector<std::string> fields;
                for (const auto& m : *r.members()) fields.push_back(m.key);
                CHECK(fields == (std::vector<std::string>{"widget", "section", "key", "label"}));
            }
    }
}
