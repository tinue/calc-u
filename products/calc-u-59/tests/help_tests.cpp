// LIB-16, Calc-U-59 column (design §13.9: P59-2, P59-5, P59-6): what the
// help tool cannot read itself. The settings keys the product declares
// reach `tools/help` through help/settings-keys.txt, which this test
// compares with settingsDecl() (CALCU59_HELP_KEYS=1 writes it anew); the
// words a shared part may not use (help/help.yaml's `names:`) cover the
// catalogue; the six presets of the preset tutorial load (help/downloads/).
// In CalcU59Tests (design D-13).

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <set>
#include <sstream>
#include <string>

#include "TestHarness.hpp"
#include "TIPresetRig.hpp"
#include "../app/Settings.hpp"
#include "../core/TICatalogue.hpp"
#include "settings/Declaration.hpp"

namespace {

namespace cs = calcu::settings;

const char* kKeysFile = "help/settings-keys.txt";
const char* kManifest = "help/help.yaml";

std::string readHelpFile(const char* path) {
    std::ifstream in(path, std::ios::binary);
    return std::string((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
}

// The family's keys by section and name; a section whose names the product
// gives ([preset] <model>, [folders] <kind>, [window] <key>) by its section.
struct FamilyKeys {
    std::set<std::string> fixed;          // "[section] key"
    std::set<std::string> namedSections;  // "preset", "folders", "window"
    std::string general(const std::string& section) const {
        if (section == "preset") return "<model>";
        if (section == "folders") return "<kind>";
        return "<key>";
    }
};

FamilyKeys familyKeys() {
    FamilyKeys f;
    for (const cs::KeyDecl& d : {cs::family::startupModel(), cs::family::startupLastModel(), cs::family::saveFolder(),
                                 cs::family::tapeFolder(), cs::family::traceFolder(), cs::family::traceMaxFileSizeMb(),
                                 cs::family::traceLocation(), cs::family::serialLinkFolder()})
        f.fixed.insert("[" + d.section + "] " + d.key);
    f.namedSections = {cs::family::presetFor("x").section, cs::family::openFolder("x").section,
                       cs::family::windowNumber("x", 0).section};
    return f;
}

// One line per key: `family [section] key` for the family's (a named
// section once, in its general form), `product [section] key` for the
// product's own and for the names it gives a named section.
std::string keysFileText() {
    const FamilyKeys family = familyKeys();
    std::ostringstream out;
    out << "# The settings keys Calc-U-59 declares (app/Settings.cpp, settingsDecl()),\n"
           "# for tools/help's check C7 (LIB-16 AC-14): each `[section] key` is described\n"
           "# in chapter 8, the family's in library/help/08-settings.md, the product's in\n"
           "# help/08-settings.md. Written by CalcU59Tests with CALCU59_HELP_KEYS=1.\n";
    std::set<std::string> generalDone;
    for (const cs::KeyDecl& d : cu59::settings::settingsDecl().keys) {
        const std::string name = "[" + d.section + "] " + d.key;
        if (family.fixed.count(name)) {
            out << "family " << name << "\n";
        } else if (family.namedSections.count(d.section)) {
            if (generalDone.insert(d.section).second)
                out << "family [" << d.section << "] " << family.general(d.section) << "\n";
            out << "product " << name << "\n";
        } else {
            out << "product " << name << "\n";
        }
    }
    return out.str();
}

// The `names:` list of help/help.yaml (one flow sequence on its line).
std::set<std::string> manifestNames() {
    std::istringstream in(readHelpFile(kManifest));
    std::string line;
    std::set<std::string> names;
    while (std::getline(in, line)) {
        if (line.rfind("names:", 0) != 0) continue;
        const auto open = line.find('[');
        const auto close = line.find(']');
        if (open == std::string::npos || close == std::string::npos) break;
        std::istringstream items(line.substr(open + 1, close - open - 1));
        std::string item;
        while (std::getline(items, item, ',')) {
            const auto b = item.find_first_not_of(" \"");
            const auto e = item.find_last_not_of(" \"");
            if (b != std::string::npos) names.insert(item.substr(b, e - b + 1));
        }
    }
    return names;
}

} // namespace

TEST(lib16_ac14_settings_keys_file_matches_declaration) {
    const std::string want = keysFileText();
    if (const char* write = std::getenv("CALCU59_HELP_KEYS"); write && std::string(write) == "1") {
        std::ofstream(kKeysFile, std::ios::binary) << want;
    }
    const std::string have = readHelpFile(kKeysFile);
    if (have != want) std::printf("%s differs from settingsDecl(); expected:\n%s", kKeysFile, want.c_str());
    CHECK(have == want);
}

TEST(lib16_ac14_every_declared_key_is_in_the_keys_file) {
    const std::string text = keysFileText();
    for (const cs::KeyDecl& d : cu59::settings::settingsDecl().keys) {
        const std::string name = "[" + d.section + "] " + d.key;
        CHECK(text.find(" " + name + "\n") != std::string::npos);
    }
    CHECK(text.find("product [display] led-style\n") != std::string::npos);
    CHECK(text.find("family [startup] model\n") != std::string::npos);
    CHECK(text.find("family [preset] <model>\nproduct [preset] TI-59\n") != std::string::npos);
}

TEST(lib16_ac2_help_names_cover_the_catalogue) {
    const std::set<std::string> names = manifestNames();
    CHECK(names.count("Calc-U-59") == 1);
    CHECK(names.count("TMC0501") == 1);
    const calcu::machine::Catalogue& catalogue = tiCatalogue();
    CHECK(catalogue.models().size() == 3);
    for (const auto& m : catalogue.models()) {
        if (!names.count(m.model.value)) std::printf("help.yaml names: lacks the model %s\n", m.model.value.c_str());
        CHECK(names.count(m.model.value) == 1);
    }
    CHECK(catalogue.peripherals().size() == 15);
    for (const auto& p : catalogue.peripherals()) {
        const std::string& name = p.name.empty() ? p.id.value : p.name;
        if (!names.count(name)) std::printf("help.yaml names: lacks the peripheral %s\n", name.c_str());
        CHECK(names.count(name) == 1);
    }
}

// P59-2 (AC-9, D-15): the six presets of the preset tutorial, as help/downloads/
// offers them, load on their model without a refusal, and what the
// tutorial's text says the calculator shows after a step holds: 5 A gives
// 15 (step 4 on), RCL 29 gives 3.000000000004 (step 5 on), and the finished
// file ends with the fraction 4e-12 after subtracting 3.
TEST(lib16_ac9_tutorial_presets_load) {
    SKIP_UNLESS(tirig::haveRoms(), "the ROMs are not fetched (roms/fetch_roms.sh)");
    const char* files[] = {"tutorial-1-setup.ti59",   "tutorial-2-partition.ti59", "tutorial-3-cuecard.ti59",
                           "tutorial-4-program.ti59", "tutorial-5-registers.ti59", "tutorial.ti59"};
    for (int i = 0; i < 6; ++i) {
        tirig::Calcu c;
        CHECK(static_cast<bool>(c));
        if (!c) continue;
        const std::string path = std::filesystem::absolute(std::string("help/downloads/") + files[i]).string();
        const std::string reply = c("preset " + path);
        if (reply != "done\n") std::printf("%s: %s", files[i], reply.c_str());
        CHECK_EQ(reply, std::string("done\n"));
        CHECK_EQ(c.machine()->model().value, std::string("TI-59"));
        c("wait");
        // Step 2 on: 720 steps, 30 data registers.
        CHECK_EQ(c("ti59 partition"), std::string(i == 0 ? "done 1\n479\n" : "done 1\n719\n"));
        if (i == 5) {   // the finished file typed 5 A, RCL 29, - 3 = itself
            CHECK_EQ(c("expect 4e-12"), std::string("done\n"));
            continue;
        }
        if (i >= 3) {   // the program: 5 A sums 1..5
            c("key 5 A");
            c("wait");
            CHECK_EQ(c("expect 15"), std::string("done\n"));
        }
        if (i == 4) {   // the register, in full precision, then the fraction
            c("key RCL 2 9");
            c("wait");
            CHECK_EQ(c("expect 3.000000000004"), std::string("done\n"));
            c("key - 3 =");
            c("wait");
            CHECK_EQ(c("expect 4e-12"), std::string("done\n"));
        }
    }
}
