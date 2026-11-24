#include "Settings.hpp"

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
    d.keys.push_back(cs::family::windowOptional("calculator-width", "The calculator panel's width (written by the app; Qt6)."));
    d.keys.push_back(cs::family::windowOptional("printer-width", "The printer panel's width (written by the app; Qt6)."));
    d.keys.push_back(cs::family::windowOptional("debug-width", "The debug panel's width (written by the app; Qt6)."));
    d.keys.push_back(cs::family::windowFlag("portrait-debug-page",
                                            "The debugger as a third page in portrait: true or false (Swift on iOS)."));
    for (cs::KeyDecl& k : cs::family::paperWindow()) d.keys.push_back(std::move(k));
    return d;
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
    w.calculatorWidth = cs::windowNumber(m_file, "calculator-width");
    w.printerWidth = cs::windowNumber(m_file, "printer-width");
    w.debugWidth = cs::windowNumber(m_file, "debug-width");
    return w;
}

void Settings::setWindowState(const WindowState& w) {
    cs::setWindowPlace(m_file, "", w.place);
    cs::setWindowNumber(m_file, "calculator-width", w.calculatorWidth);
    cs::setWindowNumber(m_file, "printer-width", w.printerWidth);
    cs::setWindowNumber(m_file, "debug-width", w.debugWidth);
}

} // namespace cu59::settings
