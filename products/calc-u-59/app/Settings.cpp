#include "Settings.hpp"
#include "files/Files.hpp"
#include "settings/OpenFolders.hpp"
#include "settings/Startup.hpp"
#include <charconv>
#include <filesystem>
#include <system_error>

namespace cu59::settings {

namespace cs = calcu::settings;
namespace fs = std::filesystem;

namespace {

cs::KeyDecl choice(const char* section, const char* key, std::vector<std::string> choices, std::string help) {
    cs::KeyDecl d;
    d.section = section;
    d.key = key;
    d.kind = cs::ValueKind::Choice;
    d.defaultValue = choices.front();
    d.choices = std::move(choices);
    d.help = std::move(help);
    return d;
}

// A [window] number without a default: empty until the app writes it.
cs::KeyDecl windowOptional(const char* key, const char* what) {
    cs::KeyDecl d;
    d.section = "window";
    d.key = key;
    d.kind = cs::ValueKind::Integer;
    d.writtenByApp = true;
    d.help = std::string("The ") + what + " (written by the app; Qt6).";
    return d;
}

cs::ProductDecl makeDecl() {
    cs::ProductDecl d;
    d.productName = "Calc-U-59";
    d.models = {"TI-59", "TI-58", "TI-58C"};
    d.fallbackModel = "TI-59";  // AC-20
    d.keys.push_back(cs::family::startupModel());
    d.keys.push_back(cs::family::startupLastModel());
    for (const std::string& m : d.models) d.keys.push_back(cs::family::presetFor(m));
    d.keys.push_back(cs::family::saveFolder());
    d.keys.push_back(cs::family::traceFolder());
    d.keys.push_back(cs::family::traceMaxFileSizeMb());
    d.keys.push_back(cs::family::traceLocation());
    d.keys.push_back(choice("display", "led-style", {"modernized", "classic"},
                            "The display's segments: modernized or classic."));
    d.keys.push_back(choice("keyboard", "feedback", {"off", "haptic", "click"},
                            "Feedback on a key press: off, haptic or click (Swift on iOS, Web)."));
    for (OpenFolder f : {OpenFolder::Presets, OpenFolder::AsmOverlays, OpenFolder::Other})
        d.keys.push_back(cs::family::openFolder(openFolderKind(f)));
    d.keys.push_back(windowOptional("x", "window's left edge"));
    d.keys.push_back(windowOptional("y", "window's top edge"));
    d.keys.push_back(cs::family::windowNumber("width", WindowState{}.width));
    d.keys.push_back(cs::family::windowNumber("height", WindowState{}.height));
    d.keys.push_back(windowOptional("calculator-width", "calculator panel's width"));
    d.keys.push_back(windowOptional("printer-width", "printer panel's width"));
    d.keys.push_back(windowOptional("debug-width", "debug panel's width"));
    cs::KeyDecl debugPage;
    debugPage.section = "window";
    debugPage.key = "portrait-debug-page";
    debugPage.kind = cs::ValueKind::Boolean;
    debugPage.defaultValue = "false";
    debugPage.writtenByApp = true;
    debugPage.help = "The debugger as a third page in portrait: true or false (Swift on iOS).";
    d.keys.push_back(debugPage);
    return d;
}

std::optional<long> number(const cs::SettingsFile& f, const char* key) {
    const std::string v = f.text("window", key);
    long out = 0;
    auto [p, ec] = std::from_chars(v.data(), v.data() + v.size(), out);
    if (v.empty() || ec != std::errc() || p != v.data() + v.size()) return std::nullopt;
    return out;
}

void setNumber(cs::SettingsFile& f, const char* key, std::optional<long> v) {
    f.set("window", key, v ? std::to_string(*v) : std::string());
}

} // namespace

const cs::ProductDecl& settingsDecl() {
    static const cs::ProductDecl decl = makeDecl();
    return decl;
}

const char* modelName(Model model) { return info(model).displayName; }

std::optional<Model> modelNamed(std::string_view name) {
    for (Model m : {Model::TI59, Model::TI58, Model::TI58C})
        if (name == modelName(m)) return m;
    return std::nullopt;
}

const char* openFolderKind(OpenFolder folder) {
    switch (folder) {
    case OpenFolder::Presets: return "presets";
    case OpenFolder::AsmOverlays: return "asm-overlays";
    case OpenFolder::Other: return "other";
    }
    return "other";
}

std::optional<Model> Settings::startupModelChoice() const { return modelNamed(m_file.text("startup", "model")); }

void Settings::setStartupModelChoice(std::optional<Model> model) {
    m_file.set("startup", "model", model ? modelName(*model) : "last");
}

Model Settings::startupModel() const { return modelNamed(cs::startupModel(m_file)).value_or(Model::TI59); }

std::optional<Model> Settings::lastUsedModel() const { return modelNamed(m_file.text("startup", "last-model")); }

void Settings::setLastUsedModel(Model model) { m_file.set("startup", "last-model", modelName(model)); }

std::string Settings::startupPreset(Model model) const { return cs::startupPreset(m_file, modelName(model)); }

int Settings::traceMaxFileSizeMB() const { return static_cast<int>(m_file.integer("trace", "max-file-size-mb")); }

void Settings::setTraceMaxFileSizeMB(int mb) { m_file.set("trace", "max-file-size-mb", std::to_string(mb)); }

std::string Settings::traceFolder() const { return m_file.text("trace", "folder"); }

void Settings::setTraceFolder(const std::string& dir) { m_file.set("trace", "folder", dir); }

TraceLocation Settings::traceLocation() const {
    const std::string v = m_file.text("trace", "location");
    if (v == "icloud") return TraceLocation::ICloud;
    if (v == "folder") return TraceLocation::Folder;
    return TraceLocation::AppSupport;
}

void Settings::setTraceLocation(TraceLocation location) {
    const char* v = location == TraceLocation::ICloud ? "icloud"
                    : location == TraceLocation::Folder ? "folder"
                                                        : "app-support";
    m_file.set("trace", "location", v);
}

std::string Settings::saveFolderOverride() const { return m_file.text("storage", "save-folder"); }

void Settings::setSaveFolderOverride(const std::string& dir) { m_file.set("storage", "save-folder", dir); }

LedStyle Settings::ledStyle() const {
    return m_file.text("display", "led-style") == "classic" ? LedStyle::Classic : LedStyle::Modernized;
}

void Settings::setLedStyle(LedStyle style) {
    m_file.set("display", "led-style", style == LedStyle::Classic ? "classic" : "modernized");
}

KeyboardFeedback Settings::keyboardFeedback() const {
    const std::string v = m_file.text("keyboard", "feedback");
    if (v == "haptic") return KeyboardFeedback::Haptic;
    if (v == "click") return KeyboardFeedback::Click;
    return KeyboardFeedback::Off;
}

void Settings::setKeyboardFeedback(KeyboardFeedback feedback) {
    const char* v = feedback == KeyboardFeedback::Haptic ? "haptic"
                    : feedback == KeyboardFeedback::Click ? "click"
                                                          : "off";
    m_file.set("keyboard", "feedback", v);
}

std::string Settings::openDir(OpenFolder folder) const { return cs::fixedOpenFolder(m_file, openFolderKind(folder)); }

void Settings::setOpenDir(OpenFolder folder, const std::string& dir) {
    cs::setFixedOpenFolder(m_file, openFolderKind(folder), dir);   // "": back to the folder last used
}

std::string Settings::openStartDir(OpenFolder folder, const std::string& home) const {
    return cs::openStartFolder(m_file, openFolderKind(folder), home);
}

void Settings::rememberOpenFile(OpenFolder folder, const std::string& filePath) {
    const fs::path file = calcu::files::toPath(filePath);
    std::error_code ec;
    fs::path abs = fs::absolute(file, ec);
    if (ec) abs = file;
    cs::rememberOpenFolder(m_file, openFolderKind(folder), calcu::files::fromPathNative(abs.parent_path()));
}

WindowState Settings::windowState() const {
    WindowState w;
    w.x = number(m_file, "x");
    w.y = number(m_file, "y");
    w.width = number(m_file, "width").value_or(w.width);
    w.height = number(m_file, "height").value_or(w.height);
    w.calculatorWidth = number(m_file, "calculator-width");
    w.printerWidth = number(m_file, "printer-width");
    w.debugWidth = number(m_file, "debug-width");
    return w;
}

void Settings::setWindowState(const WindowState& w) {
    setNumber(m_file, "x", w.x);
    setNumber(m_file, "y", w.y);
    setNumber(m_file, "width", w.width);
    setNumber(m_file, "height", w.height);
    setNumber(m_file, "calculator-width", w.calculatorWidth);
    setNumber(m_file, "printer-width", w.printerWidth);
    setNumber(m_file, "debug-width", w.debugWidth);
}

bool Settings::portraitDebugPage() const { return m_file.boolean("window", "portrait-debug-page"); }

void Settings::setPortraitDebugPage(bool on) { m_file.set("window", "portrait-debug-page", on ? "true" : "false"); }

} // namespace cu59::settings
