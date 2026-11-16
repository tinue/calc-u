#include "app/SharpWebPart.hpp"

#include <filesystem>
#include <fstream>
#include <map>

#include "Connector/Ce158Card.hpp"
#include "Display/DisplayOutputText.hpp"
#include "DropFile.hpp"
#include "Utf8.hpp"
#include "app/CardYamlDump.hpp"
#include "app/KeyLayout/FaceplateLayout.hpp"
#include "app/LcdLayout.hpp"
#include "presets/BundleFile.hpp"
#include "presets/Source.hpp"
#include "app/Text.hpp"
#include "app/Utf8Path.hpp"
#include "sessions/Args.hpp"

namespace shared {

namespace sessions = calcu::sessions;
using sessions::CommandOutcome;
using sessions::Done;
using sessions::Refused;
using sessions::SessionContext;
using sessions::Value;
using sessions::bytesArg;
using sessions::flagArg;
using sessions::textArg;
using sessions::truncatedIntegerArg;

namespace {

Done result(std::string_view kind, Value v) { return Done{sessions::valuePayload(kind, std::move(v))}; }

Value strings(const std::vector<std::string>& items) {
    Value::Array out;
    for (const auto& s : items) out.emplace_back(s);
    return Value(std::move(out));
}

Value rgb(Rgb c) { return Value(Value::Array{Value(c.r), Value(c.g), Value(c.b)}); }


Value presetEntry(const calcu::presets::PresetListEntry& e) {
    return Value::object({{"group", e.group},
                          {"ref", e.bundlePath},
                          {"model", e.model.value},
                          {"name", e.name},
                          {"description", e.description},
                          {"label", e.description.empty() ? e.name : e.description}});
}

Value faceplate(int model) {
    const KeyLayout::Layout& layout = KeyLayout::layoutFor(static_cast<Model>(model));
    auto rect = [](const KeyLayout::KeyRect& r) {
        return Value::object({{"name", std::string(r.name ? r.name : "")}, {"x", r.x}, {"y", r.y}, {"w", r.w}, {"h", r.h}});
    };
    Value::Array keys;
    for (std::size_t i = 0; i < layout.keyCount; ++i) keys.push_back(rect(layout.keys[i]));
    return Value::object({{"width", layout.imageWidth},
                          {"height", layout.imageHeight},
                          {"lcd", rect(layout.lcdRect)},
                          {"keys", Value(std::move(keys))}});
}

Value lcdConstants() {
    using namespace lcd;
    return Value::object({{"p15Background", rgb(kP15BackgroundRgb)},
                          {"p16Background", rgb(kP16BackgroundRgb)},
                          {"ink", rgb(kInkRgb)},
                          {"p15IndicatorBarFraction", kP15IndicatorBarFraction},
                          {"p15IndicatorFontHeightFraction", kP15IndicatorFontHeightFraction},
                          {"p15IndicatorTopInsetFraction", kP15IndicatorTopInsetFraction},
                          {"p15ContentScale", kP15ContentScale},
                          {"p15MatrixTopGapFraction", kP15MatrixTopGapFraction},
                          {"p16IndicatorRowTop", kP16IndicatorRowTop},
                          {"p16LabelFontHeightFraction", kP16LabelFontHeightFraction},
                          {"p16RowHeightFraction", kP16RowHeightFraction},
                          {"p16MatrixLeft", kP16MatrixLeft},
                          {"p16MatrixRight", kP16MatrixRight},
                          {"p16MatrixTop", kP16MatrixTop},
                          {"p16MatrixBottom", kP16MatrixBottom}});
}

} // namespace

SharpWebPart::SharpWebPart(SharpAppPart& app, Settings& settings, AppPaths&, std::string uploadsDir)
    : m_app(app), m_settings(settings), m_uploadsDir(std::move(uploadsDir)) {}

SharpWebPart::~SharpWebPart() = default;

void SharpWebPart::attach(sessions::SessionSetup& setup) {
    m_ui = &setup.stateTopic(topics::kWebUi);
    m_lcd = &setup.stateTopic(topics::kWebLcd);
    m_lamps = &setup.stateTopic(topics::kWebLamps);
    m_printer = &setup.streamTopic(topics::kWebPrinter, sessions::History::None);

    using H = sessions::SessionSetup::Handler;
    auto cmd = [&setup](const char* name, H h) { setup.command(sessions::CommandName{name}, std::move(h)); };
    auto req = [&setup](const char* name, sessions::SessionSetup::RequestHandler h) {
        setup.request(sessions::RequestName{name}, std::move(h));
    };

    // Keys and typing are the library's typing part's commands (LIB-5:
    // host-key-down/up, screen-key-down/up, focus-lost, paste).

    // ── Opening files ───────────────────────────────────────────────────
    // A file the user picked or dropped goes to the uploads folder first (a
    // preset finds the files it names next to it); the result is its path.
    cmd("c1600.upload", [this](const Value& a, SessionContext&) -> CommandOutcome {
        const std::string path = m_uploadsDir + "/" + fromPath(toPath(textArg(a, "name")).filename());
        std::error_code ec;
        std::filesystem::create_directories(toPath(m_uploadsDir), ec);
        const std::vector<std::uint8_t> b = bytesArg(a, "bytes").value_or(std::vector<std::uint8_t>{});
        std::ofstream out(toPath(path), std::ios::binary | std::ios::trunc);
        out.write(reinterpret_cast<const char*>(b.data()), static_cast<std::streamsize>(b.size()));
        if (!out) return Refused{"The file could not be written: " + path};
        return result("c1600.path", Value(path));
    });
    req("c1600.drop-target", [](const Value& a, SessionContext&) {
        const char* t = "";
        switch (dropfile::classify(bytesArg(a, "bytes").value_or(std::vector<std::uint8_t>{}), textArg(a, "name"))) {
            case dropfile::Target::Preset: t = "preset"; break;
            case dropfile::Target::BasicProgram: t = "basic"; break;
            case dropfile::Target::MachineCode: t = "code"; break;
            case dropfile::Target::None: break;
        }
        return sessions::replyGiven(sessions::valuePayload("c1600.drop-target", Value(t)));
    });
    // Load BASIC Program / Load Machine Code: the page sends the loaders'
    // own commands (app/SharpLoaderPart, C1600-2 design §3.7).

    // ── Media ───────────────────────────────────────────────────────────
    // The pickers, Name & Save, the side, the tapes, the import and the
    // saved media are LIB-7's commands (`media.choose`, `media.turn`,
    // `media.name-and-save`, `media.start-new`, `media.import`,
    // `media.export`, `media.delete`; AC-43), drawn by the library's
    // components (CalcuMediaPicker, CalcuNameAndSaveDialog,
    // CalcuSavedMediaDialog).
    // ── Inspector ───────────────────────────────────────────────────────
    req("c1600.inspector-menu", [this](const Value& a, SessionContext&) {
        const bool pc1500 = m_app.currentModel() != Model::PC1600;
        Value::Array out;
        for (const inspect::MenuEntry& e : m_app.inspectorMenu(flagArg(a, "dumps")))
            out.push_back(Value::object({{"view", static_cast<int>(e.view)},
                                         {"title", std::string(inspect::viewTitle(e.view, pc1500))},
                                         {"enabled", e.enabled}}));
        return sessions::replyGiven(sessions::valuePayload("c1600.inspector-menu", Value(std::move(out))));
    });
    req("c1600.inspector-view", [this](const Value& a, SessionContext&) {
        return sessions::replyGiven(sessions::valuePayload(
            "c1600.inspector-view", Value(joinLines(m_app.inspectorView(static_cast<inspect::View>(truncatedIntegerArg(a, "view")))))));
    });
    req("c1600.card-yaml", [this](const Value&, SessionContext&) {
        return sessions::replyGiven(sessions::valuePayload("c1600.card-yaml", strings(cardYamlDump(m_app))));
    });

    // ── Copy Screen ─────────────────────────────────────────────────────
    req("c1600.screen-image", [this](const Value&, SessionContext& c) {
        const calcu::machine::DisplayOutput* d = c.display();
        if (!d) return sessions::replyRefused("No machine is running.");
        const GrayImage image = m_app.screenImage(*d);
        LcdText t;
        return sessions::replyGiven(sessions::valuePayload("c1600.screen-image",
                                            Value::object({{"width", image.width},
                                                           {"height", image.height},
                                                           {"dpi", image.dpi},
                                                           {"pixels", Value(Value::Bytes(image.pixels))},
                                                           {"text", m_app.lcdText(*d, &t) ? t.plainText() : std::string()}})));
    });
    req("c1600.lcd-text", [this](const Value&, SessionContext& c) {
        const calcu::machine::DisplayOutput* d = c.display();
        LcdText t;
        return sessions::replyGiven(sessions::valuePayload("c1600.lcd-text", Value(d && m_app.lcdText(*d, &t) ? t.text() : std::string())));
    });

    // ── Settings ────────────────────────────────────────────────────────
    req("c1600.settings-state", [this](const Value&, SessionContext&) {
        // The startup presets of the preset list per model (LIB-18 §10.4),
        // and the setting's own value when it is none of them.
        const std::vector<calcu::presets::PresetListEntry> entries = m_app.presetEntries();
        Value::Object presets, choices;
        for (Model m : {Model::PC1500, Model::PC1500A, Model::PC1600}) {
            const std::string current = m_settings.defaultPresetPath(m);
            presets.push_back({modelKey(m), Value(current)});
            Value::Array list;
            bool listed = current.empty();
            for (const auto& e : entries) {
                if (e.kind != calcu::presets::PresetListEntry::Kind::Startup || e.model.value != settingsModelName(m)) continue;
                listed = listed || e.bundlePath == current;
                list.push_back(presetEntry(e));
            }
            if (!listed) {
                calcu::presets::PresetListEntry own;
                own.bundlePath = current;
                own.name = calcu::presets::stemOf(current);
                list.push_back(presetEntry(own));
            }
            choices.push_back({modelKey(m), Value(std::move(list))});
        }
        return sessions::replyGiven(sessions::valuePayload("c1600.settings",
                                            Value::object({{"startupModel", m_settings.startupModelPreference()},
                                                           {"defaultPresets", Value(std::move(presets))},
                                                           {"presetChoices", Value(std::move(choices))}})));
    });
    cmd("c1600.set-startup-model", [this](const Value& a, SessionContext&) -> CommandOutcome {
        m_settings.setStartupModelPreference(textArg(a, "model"));
        return Done{};
    });
    cmd("c1600.set-default-preset", [this](const Value& a, SessionContext&) -> CommandOutcome {
        m_settings.setDefaultPresetPath(static_cast<Model>(truncatedIntegerArg(a, "model")), textArg(a, "path"));
        return Done{};
    });

    // ── Static data (adapter W16-1, until M-49) ─────────────────────────
    req("c1600.faceplate", [](const Value& a, SessionContext&) {
        return sessions::replyGiven(sessions::valuePayload("c1600.faceplate", faceplate(static_cast<int>(truncatedIntegerArg(a, "model")))));
    });
    req("c1600.lcd-constants", [](const Value&, SessionContext&) {
        return sessions::replyGiven(sessions::valuePayload("c1600.lcd-constants", lcdConstants()));
    });
}

void SharpWebPart::afterCall(SessionContext& c, calcu::pacing::WallTime) {
    lcd(c);
    lamps();
    printer();
    // The pickers read folders: twice a second, as a bay's change (a
    // preset's save, a recording) shows up; at once after a command.
    if (++m_calls % 30 == 0) m_ui->set(sessions::valuePayload("c1600.ui", ui()));
}

void SharpWebPart::machineChanged(SessionContext& c) { refresh(c); }
void SharpWebPart::operationEnded(SessionContext& c) { refresh(c); }

void SharpWebPart::refresh(SessionContext& c) {
    m_ui->set(sessions::valuePayload("c1600.ui", ui()));
    lcd(c);
    lamps();
}

Value SharpWebPart::ui() const {
    const ControlState s = m_app.controlState();
    auto peripheral = [](const PeripheralState& p) {
        return Value::object({{"offered", p.offered}, {"onControlBar", p.onControlBar}, {"attached", p.attached}, {"enabled", p.enabled}});
    };
    Value::Object ui{{"model", Value(static_cast<int>(s.model))},
                     {"slotCount", Value(s.slotCount)},
                     {"ce150", peripheral(s.ce150)},
                     {"ce1600p", peripheral(s.ce1600p)},
                     {"ce158", peripheral(s.ce158)},
                     {"floppyEnabled", Value(s.floppyEnabled)},
                     {"pc1500RomMenu", Value(s.pc1500RomMenu)},
                     {"pc1600RomMenu", Value(s.pc1600RomMenu)},
                     {"ce1600pRomMenu", Value(s.ce1600pRomMenu)},
                     {"paper", Value(s.paperOwner ? s.paperOwner->value : std::string())},   // P-A1
                     {"printerShown", Value(s.printerShown)},
                     {"pc1500Rom", Value(static_cast<int>(m_app.pc1500RomRevision()))},
                     {"pc1600Rom", Value(static_cast<int>(m_app.pc1600RomVersion()))},
                     {"ce1600pRom", Value(static_cast<int>(m_app.ce1600pRomVersion()))}};
    // The pickers of the slots, the drive and the cassette are the media
    // part's (LIB-7: the topic `media`, the commands `media.*`); here only
    // which of them the control bar shows.
    ui.push_back({"floppyShown", Value(s.floppyShown)});
    ui.push_back({"tapeShown", Value(s.tapeShown)});
    return Value(std::move(ui));
}

// The display output as the page's LCD painter takes it: the model, the
// dots (one byte each) and the indicators laid out (app/LcdLayout).
void SharpWebPart::lcd(SessionContext& c) {
    const calcu::machine::DisplayOutput* d = c.display();
    if (!d || !m_app.hasLiveMachine()) return;
    const DisplayFrame frame = displayFrameOf(*d);
    const Model model = m_app.currentModel();
    Value::Bytes dots(frame.pixels.size());
    for (std::size_t i = 0; i < dots.size(); ++i) dots[i] = frame.pixels[i] ? 1 : 0;
    Value::Array indicators;
    for (const lcd::Indicator& ind : lcd::indicators(model, frame))
        indicators.push_back(Value::object({{"text", ind.text},
                                            {"x", ind.x},
                                            {"scale", ind.scale},
                                            {"style", ind.style == lcd::Indicator::Style::Inverted ? "inverted"
                                                      : ind.style == lcd::Indicator::Style::Dot    ? "dot"
                                                                                                   : "text"}}));
    m_lcd->set(sessions::valuePayload("c1600.lcd", Value::object({{"model", static_cast<int>(model)},
                                                   {"cols", frame.cols},
                                                   {"rows", frame.rows},
                                                   {"poweredOn", frame.poweredOn},
                                                   {"dots", Value(std::move(dots))},
                                                   {"indicators", Value(std::move(indicators))}})));
}

// The floppy lamp and the tape counter.
void SharpWebPart::lamps() {
    const ControlState state = m_app.controlState();
    const TapeDeck::Status tape = m_app.tapeStatus();
    const calcu::media::Binding* held = m_app.mediaPart().bindingOf("tape");
    const std::string tapeName = held ? held->name : std::string();
    m_lamps->set(sessions::valuePayload(
        "c1600.lamps",
        Value::object({{"floppyMotor", state.floppyShown && m_app.floppyMotorOn()},
                       {"tapeMode", tape.mode == TapeDeck::Mode::Play ? "play" : tape.mode == TapeDeck::Mode::Record ? "record" : ""},
                       {"tapeMotor", tape.motor},
                       {"tapeSeconds", static_cast<int>(tape.position)},
                       {"tapeFile", tapeName}})));
}

void SharpWebPart::printer() {
    if (!m_app.ce158Attached()) return;
    const std::vector<std::uint8_t> b = m_app.drainCE158PrinterOutput();
    if (b.empty()) return;
    m_printer->append(sessions::valuePayload("c1600.printer", Value::object({{"bytes", Value(Value::Bytes(b))},
                                                              {"text", ce158PrintableText(b.data(), b.size())}})));
}

} // namespace shared
