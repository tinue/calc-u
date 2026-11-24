#include "Settings.hpp"
#include <charconv>
#include <system_error>

namespace cu59::settings {

namespace cs = calcu::settings;

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
    d.fallbackModel = "TI-59";
    d.keys.push_back(cs::family::startupModel());
    d.keys.push_back(cs::family::startupLastModel());
    // A fresh install starts each model with its bundled startup preset
    // (the Master Library and the PC-100C); the TI-58C with none, as a
    // preset cold boots it and its constant memory would not come back.
    d.keys.push_back(cs::family::presetFor("TI-59", "examples:startup/default-ti59.ti59"));
    d.keys.push_back(cs::family::presetFor("TI-58", "examples:startup/default-ti58.ti58"));
    d.keys.push_back(cs::family::presetFor("TI-58C"));
    d.keys.push_back(cs::family::saveFolder());
    d.keys.push_back(cs::family::traceFolder());
    d.keys.push_back(cs::family::traceMaxFileSizeMb());
    d.keys.push_back(cs::family::traceLocation());
    d.keys.push_back(cs::family::appLogLevel());
    d.keys.push_back(choice("display", "led-style", {"modernized", "classic"},
                            "The display's segments: modernized or classic."));
    d.keys.push_back(choice("keyboard", "feedback", {"off", "haptic", "click"},
                            "Feedback on a key press: off, haptic or click (Swift on iOS, Web)."));
    for (OpenFolder f : {OpenFolder::Presets, OpenFolder::AsmOverlays})
        d.keys.push_back(cs::family::openFolder(openFolderKind(f)));
    for (cs::KeyDecl& k : cs::family::windowPlace("", WindowState{}.place.width, WindowState{}.place.height))
        d.keys.push_back(std::move(k));
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
    d.keys.push_back(cs::family::windowFlag(cs::kPaperOwnWindowKey,
                                            "The printer's paper in a window of its own: true or false (Qt6)."));
    for (cs::KeyDecl& k : cs::family::windowPlace(cs::kPaperWindowPrefix, cs::kPaperWindowWidth, cs::kPaperWindowHeight))
        d.keys.push_back(std::move(k));
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

const char* openFolderKind(OpenFolder folder) {
    switch (folder) {
    case OpenFolder::Presets: return "presets";
    case OpenFolder::AsmOverlays: return "asm-overlays";
    }
    return "presets";
}

WindowState Settings::windowState() const {
    WindowState w;
    w.place = cs::windowPlace(m_file, "");
    w.calculatorWidth = number(m_file, "calculator-width");
    w.printerWidth = number(m_file, "printer-width");
    w.debugWidth = number(m_file, "debug-width");
    return w;
}

void Settings::setWindowState(const WindowState& w) {
    cs::setWindowPlace(m_file, "", w.place);
    setNumber(m_file, "calculator-width", w.calculatorWidth);
    setNumber(m_file, "printer-width", w.printerWidth);
    setNumber(m_file, "debug-width", w.debugWidth);
}

} // namespace cu59::settings
