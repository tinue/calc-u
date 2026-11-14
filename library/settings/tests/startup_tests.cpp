#include "TestHarness.hpp"
#include "settings/tests/TestProduct.hpp"
#include "settings/OpenFolders.hpp"
#include "settings/Startup.hpp"

// LIB-9 AC-9, AC-10, EC-1, and the library part of AC-29 (open folders);
// design §6, T-12, T-13. AC-11 (the trace size) is in settings_file_tests.cpp.

using namespace calcu::settings;
using namespace calcu::settings::test;

namespace {
SettingsFile withValues(const TempDir& dir, const std::string& body) {
    const std::string ini = dir / "Test-Product/Test-Product.ini";
    writeText(ini, "format-version = 1\n" + body);
    return SettingsFile::open(testProduct(), ini);
}
} // namespace

TEST(ac9_model_last_or_name_else_last) {
    TempDir dir;
    // The default is `last`.
    CHECK_EQ(SettingsFile::defaultsOnly(testProduct()).text("startup", "model"), std::string("last"));
    // A model's name.
    CHECK_EQ(startupModel(withValues(dir, "[startup]\nmodel = Model-A\nlast-model = Model-B\n")),
             std::string("Model-A"));
    // `last` takes the model used last.
    CHECK_EQ(startupModel(withValues(dir, "[startup]\nmodel = last\nlast-model = Model-A\n")),
             std::string("Model-A"));
    // A value that is neither counts as `last`.
    for (const char* v : {"TI-59", "model-a", "", "first"})
        CHECK_EQ(startupModel(withValues(dir, std::string("[startup]\nmodel = ") + v + "\nlast-model = Model-A\n")),
                 std::string("Model-A"));
    // `last` and no model was used before: the product's fallback.
    CHECK_EQ(startupModel(withValues(dir, "[startup]\nmodel = last\n")), std::string("Model-B"));
    CHECK_EQ(startupModel(SettingsFile::defaultsOnly(testProduct())), std::string("Model-B"));
}

TEST(ec1_last_model_not_a_model_gives_fallback) {
    TempDir dir;
    for (const char* v : {"PC-1500A", "Model-C", "model-a", "3"})
        CHECK_EQ(startupModel(withValues(dir, std::string("[startup]\nmodel = last\nlast-model = ") + v + "\n")),
                 std::string("Model-B"));
}

TEST(ac10_preset_per_model_empty_means_none) {
    TempDir dir;
    const SettingsFile f =
        withValues(dir, "[preset]\nModel-A = examples/hello.preset.yaml\nModel-B =\n");
    CHECK_EQ(startupPreset(f, "Model-A"), std::string("examples/hello.preset.yaml"));
    CHECK_EQ(startupPreset(f, "Model-B"), std::string(""));  // empty: no startup preset
    // One key per model, named after the model.
    const SettingsFile none = withValues(dir, "");
    CHECK_EQ(startupPreset(none, "Model-A"), std::string(""));  // missing: none
    CHECK(testProduct().find("preset", "Model-A") && testProduct().find("preset", "Model-B"));
    CHECK(testProduct().find("PRESET", "model-a"));
}

TEST(ac29_open_folder_fixed_else_last_used_else_home) {
    TempDir dir;
    SettingsFile f = withValues(dir, "");
    // Nothing set: the home folder.
    CHECK_EQ(openStartFolder(f, "presets", "/home/u"), std::string("/home/u"));
    // A pick is remembered, per kind.
    CHECK(rememberOpenFolder(f, "presets", "/home/u/presets").ok);
    CHECK_EQ(openStartFolder(f, "presets", "/home/u"), std::string("/home/u/presets"));
    CHECK_EQ(openStartFolder(f, "programs", "/home/u"), std::string("/home/u"));
    CHECK(!hasFixedOpenFolder(f, "presets"));
    // A fixed folder: picks do not change it.
    CHECK(setFixedOpenFolder(f, "presets", "~/fixed").ok);
    CHECK(hasFixedOpenFolder(f, "presets"));
    CHECK_EQ(f.text("folders", "presets"), std::string("fixed:~/fixed"));
    CHECK_EQ(openStartFolder(f, "presets", "/home/u"), std::string("/home/u/fixed"));
    CHECK(rememberOpenFolder(f, "presets", "/elsewhere").ok);
    CHECK_EQ(openStartFolder(f, "presets", "/home/u"), std::string("/home/u/fixed"));
    // Cleared: back to the folder last used, which the next pick sets.
    CHECK(setFixedOpenFolder(f, "presets", "").ok);
    CHECK(!hasFixedOpenFolder(f, "presets"));
    CHECK(rememberOpenFolder(f, "presets", "/picked").ok);
    CHECK_EQ(openStartFolder(f, "presets", "/home/u"), std::string("/picked"));
    // It is written to the file, one key per kind.
    const SettingsFile again = SettingsFile::open(testProduct(), dir / "Test-Product/Test-Product.ini");
    CHECK(again.raw("folders", "presets") == std::optional<std::string>("/picked"));
}
