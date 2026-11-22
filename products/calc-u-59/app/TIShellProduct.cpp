#include "TIShellProduct.hpp"
#include "KeyIds.hpp"
#include "KeyboardLayout.hpp"
#include "Settings.hpp"
#include "../core/TICatalogue.hpp"
#include "shell/ValueForms.hpp"
#include <utility>
#include <vector>

namespace cu59 {

namespace shell = calcu::shell;
using calcu::sessions::Value;

shell::ShellProduct shellProduct(std::string version, std::string build) {
    shell::ShellProduct p;
    p.name = "Calc-U-59";
    p.version = std::move(version);
    p.build = std::move(build);
    p.helpUrl = kHelpUrl;
    // The texts of every frontend.
    p.aboutLines = {"Calc-U-59 Calculator Emulation",
                    "Cycle-accurate TI-59, TI-58, and TI-58C emulation with magnetic cards, paper tape printer, "
                    "presets, and debugger tools."};
    p.aboutLinks = {{"GitHub", "https://github.com/tinue/calc-u"},
                    {"Support", "https://github.com/tinue/calc-u/issues"},
                    {"Privacy", "https://github.com/tinue/calc-u/blob/main/products/calc-u-59/PRIVACY.md"}};
    p.barModelLabels = {{"TI-59", "59"}, {"TI-58", "58"}, {"TI-58C", "58C"}};
    p.barSpread = true;          // model and module left, the cards in the middle, the rest right
    p.barShortPickers = true;    // "59", "ML": the bar fits beside the faceplate
    p.presetFilter = "Presets (*.ti59 *.ti58 *.ti58c)";
    // What a preset can name: modules, keystroke programs and magnetic cards
    // (.module.yaml, .keystrokes.yaml, .magnetcard.yaml).
    p.presetUploads = ".ti59,.ti58,.ti58c,.yaml";
    // Neither Copy Screen nor Paste Text, so no Edit menu.
    p.moduleSlotPinOuts = {"module-slot"};
    p.placesOnPaper = {kTIPrinterPlace.value};   // the PC-100C's button over its paper
    p.mobileBarMedia = {"magnetic-card"};
    for (const auto& [id, label] : {std::pair{kItemArmCard, "Arm Card…"}, std::pair{kItemArmFreshCard, "Arm Fresh Card…"}}) {
        shell::ProductItem i;
        i.group = shell::MenuGroup::FileMedia;
        i.id = id;
        i.label = label;
        i.mediaPlace = kCardReaderPlace;
        i.onBar = true;
        p.items.push_back(std::move(i));
    }
    return p;
}

namespace {

// The start folders of the dedicated loaders, in the declaration's order,
// labelled as Calc-U-1600 labels its own.
const std::vector<std::string> kFolderKeys{"folders.presets", "folders.asm-overlays"};

void labelFolders(shell::DialogSpec& spec) {
    for (auto& s : spec.sections) {
        if (s.title != "Folders") continue;
        for (auto& r : s.rows) {
            if (r.key == settings::openFolderKind(settings::OpenFolder::Presets)) r.label = "Presets folder";
            else if (r.key == settings::openFolderKind(settings::OpenFolder::AsmOverlays)) r.label = "ASM overlay folder";
        }
    }
}

// The rows and labels of the Swift
// settings (frontends/swift/App/Views/SettingsView.swift at 7bdd51c).
shell::DialogSpec swiftSpec(const std::string& versionLine) {
    using W = shell::DialogRow::Widget;
    // No folder rows: the iOS file importer takes no start folder.
    const std::vector<std::string> keys{"startup.model", "preset.TI-59", "preset.TI-58", "preset.TI-58C"};
    shell::DialogSpec spec = shell::familySections(settings::settingsDecl(), keys);
    for (auto& s : spec.sections)
        if (s.title == "General") s.rows.push_back({W::Note, "general", "version", versionLine});
    shell::DialogRow style{W::Choice, "display", "led-style", "Segment Style"};
    style.choices = {{"modernized", "Modernized"}, {"classic", "Classic"}};
    spec.sections.push_back({"Display", {style}});
    shell::DialogRow feedback{W::Choice, "keyboard", "feedback", "Keyboard Feedback"};
    feedback.choices = {{"off", "Off"}, {"haptic", "Haptic"}, {"click", "Click Sound"}};
    spec.sections.push_back({"Keyboard", {feedback}});
    // The values true and false, drawn as a Toggle.
    shell::DialogRow page{W::Choice, "window", "portrait-debug-page", "Debug Page in Portrait"};
    page.choices = {{"true", "On"}, {"false", "Off"}};
    spec.sections.push_back({"Layout", {page}});
    shell::DialogRow location{W::Choice, "trace", "location", "Save Location"};
    location.choices = {{"app-support", "Local (App Library)"}, {"icloud", "iCloud"}};
    shell::DialogSection tracing{"Tracing", {location}};
    tracing.rows.push_back({W::Integer, "trace", "max-file-size-mb", "Maximum file size (MB)"});
    spec.sections.push_back(std::move(tracing));
    return spec;
}

} // namespace

shell::DialogSpec settingsSpec(ShellFrontend f, const std::string& versionLine) {
    using W = shell::DialogRow::Widget;
    if (f == ShellFrontend::SwiftIOS) return swiftSpec(versionLine);
    std::vector<std::string> keys{"startup.model", "preset.TI-59", "preset.TI-58", "preset.TI-58C"};
    if (f == ShellFrontend::Qt6) {
        keys.push_back("storage.save-folder");
        keys.insert(keys.end(), kFolderKeys.begin(), kFolderKeys.end());
        for (const char* k : {"trace.folder", "trace.max-file-size-mb"}) keys.push_back(k);
    }
    if (f == ShellFrontend::WebWide) keys.push_back("trace.max-file-size-mb");
    shell::DialogSpec spec = shell::familySections(settings::settingsDecl(), keys);
    labelFolders(spec);
    for (auto& s : spec.sections)
        if (s.title == "General") s.rows.push_back({W::Note, "general", "version", versionLine});
    if (f == ShellFrontend::Qt6) {
        shell::DialogRow style{W::Choice, "display", "led-style", "Segment Style"};
        style.choices = {{"modernized", "Modernized"}, {"classic", "Classic"}};
        spec.sections.push_back({"Display", {style}});
    }
    if (f == ShellFrontend::WebPortrait) {
        shell::DialogRow feedback{W::Choice, "keyboard", "feedback", "Keyboard Feedback"};
        feedback.choices = {{"off", "Off"}, {"click", "Click Sound"}};
        spec.sections.push_back({"Keyboard", {feedback}});
    }
    return spec;
}

shell::HitMap faceplateHitMap(const calcu::machine::ModelId&) {
    shell::HitMap m;
    for (int row = 0; row < 9; ++row)
        for (int col = 0; col < 5; ++col) {
            const auto id = keyIdAt(matrixCode(row, col));
            if (!id) continue;
            const NRect r = expandedKeyRect(row, col);
            m.keys.push_back({*id, shell::Rect{r.x * kCanvasWidth, (kKeyboardYStart + r.y * kKeyboardYScale) * kCanvasHeight,
                                               r.w * kCanvasWidth, r.h * kKeyboardYScale * kCanvasHeight}});
        }
    m.display = shell::Rect{kDisplayRect.x * kCanvasWidth, kDisplayRect.y * kCanvasHeight, kDisplayRect.w * kCanvasWidth,
                            kDisplayRect.h * kCanvasHeight};
    m.distance = 0;
    return m;
}

Value faceplateMapValue(const calcu::machine::ModelId& model) {
    return Value::object({{"map", shell::toValue(faceplateHitMap(model))},
                          {"card", shell::toValue(shell::Rect{kCardRect.x * kCanvasWidth, kCardRect.y * kCanvasHeight,
                                                              kCardRect.w * kCanvasWidth, kCardRect.h * kCanvasHeight})},
                          {"canvas", Value::object({{"w", kCanvasWidth}, {"h", kCanvasHeight}})}});
}

} // namespace cu59
