// Shared/PresetCatalog over the repo's examples/bundled.yaml: every listed
// preset exists, has its own short name and description, and the names
// tell the presets of one model apart; plus the "examples:" refs.

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <set>

#include "TempFolder.hpp"
#include "TestSettings.hpp"
#include "app/AppPaths.hpp"
#include "app/PresetCatalog.hpp"
#include "TestHarness.hpp"

namespace {

namespace fs = std::filesystem;
using shared::Model;
using shared::PresetEntry;

struct Fixture {
    TestSettings testSettings;
    shared::Settings& settings = testSettings.settings;
    shared::AppPaths paths{settings, testHostDirs(fs::temp_directory_path().generic_string()), shared::BundleDirs{"roms", "examples"}};
    shared::PresetCatalog catalog{paths};
};

void printProblems(const std::vector<std::string>& problems) {
    for (const auto& p : problems) std::fprintf(stderr, "  bundled.yaml: %s\n", p.c_str());
}

// The bundle check: what Qt6 and the web client offer by name.
TEST(test_bundled_lists) {
    Fixture f;
    std::string error;
    const std::vector<PresetEntry> all = f.catalog.all(&error);
    if (!error.empty()) std::fprintf(stderr, "  bundled.yaml: %s\n", error.c_str());
    CHECK(error.empty());

    std::vector<PresetEntry> defaults;
    for (Model m : {Model::PC1500, Model::PC1500A, Model::PC1600}) {
        const auto forModel = f.catalog.defaults(m);
        CHECK(!forModel.empty());
        for (const auto& e : forModel) CHECK(e.model == m);
        defaults.insert(defaults.end(), forModel.begin(), forModel.end());
    }
    const std::vector<PresetEntry> examples = f.catalog.examples();
    CHECK(!examples.empty());
    CHECK(defaults.size() + examples.size() == all.size());

    auto problems = shared::presetNameProblems(defaults);
    printProblems(problems);
    CHECK(problems.empty());
    problems = shared::presetNameProblems(examples);
    printProblems(problems);
    CHECK(problems.empty());

    // A default preset is not an example too.
    std::set<std::string> defaultRefs;
    for (const auto& e : defaults) defaultRefs.insert(e.ref);
    for (const auto& e : examples) {
        CHECK(!defaultRefs.count(e.ref));
        CHECK(!e.group.empty());
    }
}

TEST(test_describe) {
    Fixture f;
    const auto hanoi = f.catalog.describe("examples:basic/hanoi.pc1600");
    CHECK(hanoi && hanoi->name == "Hanoi" && hanoi->model == Model::PC1600 && !hanoi->description.empty());
    CHECK(f.paths.resolvePresetRef("examples:basic/hanoi.pc1600") == "examples/basic/hanoi.pc1600");
    CHECK(f.paths.resolvePresetRef("/x/y.pc1600") == "/x/y.pc1600");

    // A preset without a name of its own: the file name stands in.
    const calcu::testing::TempFolder tmp("calcu1600-catalog");
    const fs::path plain = tmp.path() / "calcu-catalog-plain.pc1500a";
    std::ofstream(plain) << "format-version: 1\nmodel: PC-1500A\n";
    const auto e = f.catalog.describe(plain.generic_string());
    CHECK(e && e->name == "calcu-catalog-plain" && !e->hasName && e->description.empty());
    CHECK(!f.catalog.describe("examples:no/such.pc1600"));
}

TEST(test_default_choices) {
    Fixture f;
    const auto defaults = f.catalog.defaults(Model::PC1600);
    CHECK(!defaults.empty());
    CHECK(f.catalog.defaultChoices(Model::PC1600, "").size() == defaults.size());
    CHECK(f.catalog.defaultChoices(Model::PC1600, defaults.front().ref).size() == defaults.size());
    // The user's own file, unreadable here: listed by its file name.
    const auto choices = f.catalog.defaultChoices(Model::PC1600, "/no/such/mine.pc1600");
    CHECK(choices.size() == defaults.size() + 1);
    CHECK(choices.back().ref == "/no/such/mine.pc1600" && choices.back().label() == "mine");
}

TEST(test_name_problems) {
    PresetEntry a;
    a.ref = "a";
    a.model = Model::PC1600;
    a.name = "Same";
    a.description = "A";
    a.hasName = true;
    PresetEntry b = a;
    b.ref = "b";
    CHECK(shared::presetNameProblems({a}).empty());
    CHECK(shared::presetNameProblems({a, b}).size() == 1);  // same name, same model
    b.model = Model::PC1500;
    CHECK(shared::presetNameProblems({a, b}).empty());      // other model: fine
    b.name = "A name much too long";
    b.hasName = false;
    b.description.clear();
    CHECK(shared::presetNameProblems({b}).size() == 3);
}

} // namespace
