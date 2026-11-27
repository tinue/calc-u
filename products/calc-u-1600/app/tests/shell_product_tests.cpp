// What Calc-U-1600 gives the shell, checked against each model's control
// state, and the help address and the About texts.
#include "SessionFixture.hpp"
#include "TestHarness.hpp"
#include "app/SharpChrome.hpp"
#include "app/SharpShellProduct.hpp"
#include "control/ControlState.hpp"
#include "shell/ValueForms.hpp"

#include <cstdio>
#include <string>
#include <vector>

using sessionfixture::SessionFixture;
using sessionfixture::romsPresent;

TEST(shell_product_complete) {
    SKIP_UNLESS(romsPresent(), "no ROMs in roms/");
    const calcu::shell::ShellProduct p = cu1600::shellProduct("1.0", "test", true, true);
    SessionFixture f;
    std::vector<calcu::sessions::Value> states;
    for (const char* model : {"PC-1500", "PC-1500A", "PC-1600"}) {
        CHECK(model == std::string("PC-1500") ? f.start(model) : f.model(model));
        if (std::string(model) == "PC-1600") CHECK(f.attach("CE-1600P"));
        const auto* cs = f.state<calcu::control::ControlState>(calcu::control::kControlStateTopic);
        CHECK(cs != nullptr);
        if (!cs) continue;
        const std::vector<std::string> problems = calcu::shell::checkShellProduct(p, cs->toValue());
        for (const auto& s : problems) std::fprintf(stderr, "  %s: %s\n", model, s.c_str());
        CHECK(problems.empty());
        states.push_back(cs->toValue());
    }
    // Across the models: every media place an item names is listed.
    const std::vector<std::string> across = calcu::shell::checkShellProduct(p, states);
    for (const auto& s : across) std::fprintf(stderr, "  %s\n", s.c_str());
    CHECK(across.empty());
    CHECK(p.nameAndSave.slotButtons);
    CHECK_EQ(calcu::shell::nameAndSavePrompt(p, "slot-1"), std::string("Instance name:"));
    CHECK_EQ(calcu::shell::nameAndSavePrompt(p, "floppy"), std::string("Disk name:"));
    CHECK_EQ(calcu::shell::nameAndSavePrompt(p, "cassette"), std::string("Cassette name:"));
    std::vector<std::string> items;
    for (const auto& i : p.items) items.push_back(i.label);
    CHECK(items == (std::vector<std::string>{"Load BASIC Program…", "Load Machine Code…", "Host Drive…", "Unmount", "Trace", "Run Log"}));
    CHECK(cu1600::shellProduct("1.0", "", false, false).items.size() == 2);   // no host folders and no trace on the Web
}

TEST(help_address_and_about_texts) {
    const calcu::shell::ShellProduct p = cu1600::shellProduct("0.9.0", "abc123", true, true);
    CHECK_EQ(p.helpUrl, std::string("https://calc-u.org/1600/"));
    const calcu::shell::AboutModel a = calcu::shell::about(p);
    CHECK_EQ(a.name, std::string("Calc-U-1600"));
    CHECK_EQ(a.version, std::string("Version 0.9.0 (abc123)"));
    CHECK(a.lines.size() == 2 && a.lines[0] == "A Sharp PC-1500/1500A/1600 pocket-computer emulator.");
    CHECK(p.aboutLabel == "About Calc-U-1600…");
    CHECK_EQ(a.links.size(), std::size_t{2});
    CHECK(a.links.size() == 2 && a.links[0].first == "GitHub" && a.links[0].second == "https://github.com/tinue/Calc-U");
    CHECK(a.links.size() == 2 && a.links[1].first == "Support" &&
          a.links[1].second == "https://github.com/tinue/Calc-U/issues");
}

// The chrome: every colour the GUIs name is in both sets.
TEST(sharp_chrome_complete) {
    const calcu::shell::Chrome& c = cu1600::sharpChrome();
    const auto problems = calcu::shell::checkChrome(c, cu1600::sharpChromeNames());
    for (const auto& p : problems) std::printf("  %s\n", p.c_str());
    CHECK(problems.empty());
    CHECK(c.dark.has_value());
    CHECK(calcu::shell::toValue(c).find("dark") != nullptr);
}
