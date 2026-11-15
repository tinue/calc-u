#include "app/SharpWebPart.hpp"

#include <cstring>
#include <filesystem>
#include <fstream>
#include <map>

#include "Connector/Ce158Card.hpp"
#include "Display/DisplayOutputText.hpp"
#include "DropFile.hpp"
#include "Utf8.hpp"
#include "app/CardYamlDump.hpp"
#include "app/HostKeyMap.hpp"
#include "app/KeyLayout/FaceplateLayout.hpp"
#include "app/LcdLayout.hpp"
#include "app/PlotterPaper.hpp"
#include "app/PresetCatalog.hpp"
#include "app/Text.hpp"
#include "app/Utf8Path.hpp"

namespace shared {

namespace sessions = calcu::sessions;
using sessions::CommandOutcome;
using sessions::Done;
using sessions::Refused;
using sessions::SessionContext;
using sessions::Value;

namespace {

// A payload that is its value: the page's own forms.
class ValuePayload final : public sessions::Payload {
public:
    ValuePayload(sessions::PayloadKind k, Value v) : m_kind(k), m_value(std::move(v)) {}
    sessions::PayloadKind kind() const override { return m_kind; }
    bool equals(const sessions::Payload& o) const override { return o.kind() == m_kind && o.toValue() == m_value; }
    Value toValue() const override { return m_value; }

private:
    sessions::PayloadKind m_kind;
    Value m_value;
};

sessions::PayloadPtr payload(std::string_view kind, Value v) {
    return std::make_shared<ValuePayload>(sessions::PayloadKind{kind}, std::move(v));
}
Done result(std::string_view kind, Value v) { return Done{payload(kind, std::move(v))}; }

std::string text(const Value& a, const char* key) {
    const Value* v = a.find(key);
    return v && v->string() ? *v->string() : std::string();
}
std::int64_t integer(const Value& a, const char* key, std::int64_t fallback = 0) {
    const Value* v = a.find(key);
    if (v && v->integer()) return *v->integer();
    if (v && v->real()) return static_cast<std::int64_t>(*v->real());
    return fallback;
}
double real(const Value& a, const char* key) {
    const Value* v = a.find(key);
    if (v && v->real()) return *v->real();
    if (v && v->integer()) return static_cast<double>(*v->integer());
    return 0;
}
bool flag(const Value& a, const char* key) {
    const Value* v = a.find(key);
    return v && v->boolean() && *v->boolean();
}
std::vector<std::uint8_t> bytes(const Value& a, const char* key) {
    const Value* v = a.find(key);
    return v && v->bytes() ? *v->bytes() : std::vector<std::uint8_t>{};
}

Value strings(const std::vector<std::string>& items) {
    Value::Array out;
    for (const auto& s : items) out.emplace_back(s);
    return Value(std::move(out));
}

Value rgb(Rgb c) { return Value(Value::Array{Value(c.r), Value(c.g), Value(c.b)}); }

// A picker's groups plus `descriptions` (name -> description) for its tooltips.
template <typename Entry>
void pickerGroups(Value::Object& v, std::initializer_list<std::pair<const char*, const std::vector<Entry>*>> groups,
                  std::string Entry::*name) {
    Value::Object descriptions;
    for (const auto& [key, entries] : groups) {
        Value::Array names;
        for (const Entry& e : *entries) {
            names.emplace_back(e.*name);
            if (!e.description.empty()) descriptions.push_back({e.*name, Value(e.description)});
        }
        v.push_back({key, Value(std::move(names))});
    }
    v.push_back({"descriptions", Value(std::move(descriptions))});
}

template <class T> Value::Bytes rawBytes(const std::vector<T>& v) {
    Value::Bytes out(v.size() * sizeof(T));
    if (!v.empty()) std::memcpy(out.data(), v.data(), out.size());
    return out;
}

// Per pen: {coords: float32 x,y pairs as bytes, starts: uint32 polyline starts as bytes}.
Value strokes(const PaperStrokes& s) {
    Value::Array pens;
    for (int i = 0; i < kPenCount; ++i) {
        std::vector<float> coords;
        std::vector<std::uint32_t> starts;
        for (const Polyline& line : s[static_cast<std::size_t>(i)]) {
            starts.push_back(static_cast<std::uint32_t>(coords.size() / 2));
            for (const PaperPoint& p : line) {
                coords.push_back(static_cast<float>(p.x));
                coords.push_back(static_cast<float>(p.y));
            }
        }
        starts.push_back(static_cast<std::uint32_t>(coords.size() / 2));
        pens.push_back(Value::object({{"coords", Value(rawBytes(coords))}, {"starts", Value(rawBytes(starts))}}));
    }
    return Value(std::move(pens));
}

HostKey hostKeyFromCode(const std::string& code) {
    using K = HostKey;
    static const std::map<std::string, K> keys = {
        {"Backspace", K::Backspace}, {"Enter", K::Return}, {"NumpadEnter", K::Enter}, {"Tab", K::Tab},
        {"Delete", K::Delete},       {"ArrowUp", K::Up},   {"ArrowDown", K::Down},    {"ArrowLeft", K::Left},
        {"ArrowRight", K::Right},    {"F1", K::F1},        {"F2", K::F2},             {"F3", K::F3},
        {"F4", K::F4},               {"F5", K::F5},        {"F6", K::F6},             {"ScrollLock", K::ScrollLock},
        {"Insert", K::Insert},       {"Home", K::Home},    {"End", K::End},           {"PageUp", K::PageUp},
        {"PageDown", K::PageDown},
    };
    const auto it = keys.find(code);
    return it == keys.end() ? K::Other : it->second;
}

char32_t singleCodePoint(const std::string& utf8) {
    std::size_t i = 0;
    char32_t cp = 0;
    return decodeUtf8(utf8, i, cp) && i == utf8.size() ? cp : 0;
}

Value presetEntry(const PresetEntry& e) {
    return Value::object({{"group", e.group},
                          {"ref", e.ref},
                          {"model", std::string(modelKey(e.model))},
                          {"name", e.name},
                          {"description", e.description},
                          {"label", e.label()}});
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
    Value::Array pens;
    for (int i = 0; i < kPenCount; ++i) pens.push_back(rgb(kPenColors[static_cast<std::size_t>(i)]));
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
                          {"p16MatrixBottom", kP16MatrixBottom},
                          {"penColors", Value(std::move(pens))},
                          {"penWidth", kPenWidthPt}});
}

} // namespace

SharpWebPart::SharpWebPart(SharpAppPart& app, Settings& settings, AppPaths& paths, std::string uploadsDir)
    : m_app(app), m_settings(settings), m_paths(paths), m_uploadsDir(std::move(uploadsDir)),
      m_library(std::make_unique<MediaLibrary>(app)) {}

SharpWebPart::~SharpWebPart() = default;

void SharpWebPart::attach(sessions::SessionSetup& setup) {
    m_ui = &setup.stateTopic(topics::kWebUi);
    m_lcd = &setup.stateTopic(topics::kWebLcd);
    m_lamps = &setup.stateTopic(topics::kWebLamps);
    m_paper = &setup.stateTopic(topics::kWebPaper);
    m_printer = &setup.streamTopic(topics::kWebPrinter, sessions::History::None);

    using H = sessions::SessionSetup::Handler;
    auto cmd = [&setup](const char* name, H h) { setup.command(sessions::CommandName{name}, std::move(h)); };
    auto req = [&setup](const char* name, sessions::SessionSetup::RequestHandler h) {
        setup.request(sessions::RequestName{name}, std::move(h));
    };
    // Every command that changes what the page shows refreshes the topics.
    auto refreshed = [this](CommandOutcome o, SessionContext& c) {
        refresh(c);
        return o;
    };
    auto errorOutcome = [](const std::string& error) -> CommandOutcome {
        if (error.empty()) return Done{};
        return Refused{error};
    };

    // ── Keys and typing (SharpTypingHook) ───────────────────────────────
    cmd("c1600.face-key", [this](const Value& a, SessionContext&) -> CommandOutcome {
        const std::string name = text(a, "name");
        const bool down = flag(a, "down");
        if (name == "on") m_app.typing().setOnKeyPressed(down);
        else if (down) m_app.typing().pressKey(name);
        else m_app.typing().releaseKey(name);
        return Done{};
    });
    // A host key down: the calculator key the page must release on its keyup
    // ("" when nothing needs releasing).
    cmd("c1600.host-key-down", [this](const Value& a, SessionContext&) -> CommandOutcome {
        HostModifiers mods;
        mods.shift = flag(a, "shift");
        mods.alt = flag(a, "alt");
        const bool mac = flag(a, "macHost");
        if (mac) {
            mods.command = flag(a, "meta");
            mods.control = flag(a, "control");
        } else {
            mods.control = flag(a, "control");
            mods.meta = flag(a, "meta");
        }
        const auto resolved = resolveHostKey(hostKeyFromCode(text(a, "code")), mods, singleCodePoint(text(a, "key")),
                                             m_app.currentModel() == Model::PC1600, mac);
        std::string release;
        if (resolved && m_app.typing().hostKeyDown(*resolved, /*physical=*/true)) release = resolved->baseKey;
        return result("c1600.key", Value(release));
    });
    cmd("c1600.host-key-up", [this](const Value& a, SessionContext&) -> CommandOutcome {
        m_app.typing().hostKeyUp(text(a, "key"));
        return Done{};
    });
    cmd("c1600.host-shift-tap", [this](const Value&, SessionContext&) -> CommandOutcome {
        m_app.typing().hostShiftTap();
        return Done{};
    });
    cmd("c1600.paste", [this](const Value& a, SessionContext&) -> CommandOutcome {
        m_app.typing().pasteText(text(a, "text"));
        return Done{};
    });

    // ── Opening files ───────────────────────────────────────────────────
    // A file the user picked or dropped goes to the uploads folder first (a
    // preset finds the files it names next to it); the result is its path.
    cmd("c1600.upload", [this](const Value& a, SessionContext&) -> CommandOutcome {
        const std::string path = m_uploadsDir + "/" + fromPath(toPath(text(a, "name")).filename());
        std::error_code ec;
        std::filesystem::create_directories(toPath(m_uploadsDir), ec);
        const std::vector<std::uint8_t> b = bytes(a, "bytes");
        std::ofstream out(toPath(path), std::ios::binary | std::ios::trunc);
        out.write(reinterpret_cast<const char*>(b.data()), static_cast<std::streamsize>(b.size()));
        if (!out) return Refused{"The file could not be written: " + path};
        return result("c1600.path", Value(path));
    });
    req("c1600.drop-target", [](const Value& a, SessionContext&) {
        const char* t = "";
        switch (dropfile::classify(bytes(a, "bytes"), text(a, "name"))) {
            case dropfile::Target::Preset: t = "preset"; break;
            case dropfile::Target::BasicProgram: t = "basic"; break;
            case dropfile::Target::MachineCode: t = "code"; break;
            case dropfile::Target::None: break;
        }
        return sessions::replyGiven(payload("c1600.drop-target", Value(t)));
    });
    // Load BASIC Program / Load Machine Code, step 1: the images in the file.
    cmd("c1600.open-program", [this](const Value& a, SessionContext&) -> CommandOutcome {
        m_program = programChoice(bytes(a, "bytes"));
        m_programName = text(a, "name");
        return result("c1600.program", Value::object({{"names", strings(m_program.names)}}));
    });
    // Step 2, BASIC: the picked image (0 when there was no choice) into the running machine.
    cmd("c1600.load-basic-picked", [this, refreshed](const Value& a, SessionContext& c) -> CommandOutcome {
        const auto index = integer(a, "index");
        if (index < 0 || index >= static_cast<std::int64_t>(m_program.images.size())) return Refused{"No program picked."};
        const OpResult r = m_app.loadBasicProgram(m_program.images[static_cast<std::size_t>(index)], m_programName);
        return refreshed(r.ok ? CommandOutcome(Done{}) : CommandOutcome(Refused{r.error}), c);
    });
    // Step 2, machine code: the plan.
    cmd("c1600.plan-machine-code", [this](const Value& a, SessionContext&) -> CommandOutcome {
        const auto index = integer(a, "index");
        if (index < 0 || index >= static_cast<std::int64_t>(m_program.images.size()))
            return result("c1600.plan", Value::object({{"error", "No program picked."}}));
        m_code = MachineCodeLoad::plan(m_app, m_program.images[static_cast<std::size_t>(index)]);
        return result("c1600.plan", Value::object({{"error", m_code->error()},
                                                   {"needsAddress", m_code->needsAddress()},
                                                   {"defaultAddress", m_code->defaultAddressText()},
                                                   {"placeholder", m_code->addressPlaceholder()},
                                                   {"cpuNote", m_code->cpuNote()},
                                                   {"length", static_cast<double>(m_code->length())}}));
    });
    cmd("c1600.check-address", [this](const Value& a, SessionContext&) -> CommandOutcome {
        if (!m_code) return result("c1600.address", Value::object({}));
        const auto check = m_code->setAddress(text(a, "text"));
        return result("c1600.address", Value::object({{"valid", check.valid}, {"hint", check.hint}}));
    });
    // Step 3: load it; the page shows the summary and types the CALL.
    cmd("c1600.load-planned-machine-code", [this, refreshed](const Value&, SessionContext& c) -> CommandOutcome {
        if (!m_code) return Refused{"No program planned."};
        const OpResult r = m_app.loadMachineCode(m_code->request());
        if (!r.ok) return Refused{r.error};
        const auto loaded = m_code->loaded(m_app);
        return refreshed(result("c1600.loaded", Value::object({{"summary", loaded.summary},
                                                               {"newCommand", loaded.advice.newCommand},
                                                               {"newNote", loaded.advice.newNote},
                                                               {"callCommand", loaded.advice.callCommand},
                                                               {"callNote", loaded.advice.callNote}})),
                         c);
    });

    // ── Bays ────────────────────────────────────────────────────────────
    cmd("c1600.name-save-card", [this, refreshed, errorOutcome](const Value& a, SessionContext& c) -> CommandOutcome {
        std::string error;
        m_app.modules().nameAndSave(static_cast<int>(integer(a, "slot", 1)), text(a, "name"), &error, flag(a, "writeProtect"));
        return refreshed(errorOutcome(error), c);
    });
    cmd("c1600.select-disk", [this, refreshed](const Value& a, SessionContext& c) -> CommandOutcome {
        m_app.floppy().selectDisk(text(a, "name"));
        return refreshed(Done{}, c);
    });
    cmd("c1600.toggle-disk-side", [this, refreshed](const Value&, SessionContext& c) -> CommandOutcome {
        m_app.floppy().toggleSide();
        return refreshed(Done{}, c);
    });
    cmd("c1600.name-save-disk", [this, refreshed, errorOutcome](const Value& a, SessionContext& c) -> CommandOutcome {
        std::string error;
        m_app.floppy().nameAndSave(text(a, "name"), &error);
        return refreshed(errorOutcome(error), c);
    });
    cmd("c1600.select-tape", [this, refreshed, errorOutcome](const Value& a, SessionContext& c) -> CommandOutcome {
        std::string error;
        m_app.tapes().selectForPlay(text(a, "name"), &error);
        return refreshed(errorOutcome(error), c);
    });
    req("c1600.tape-exists", [this](const Value& a, SessionContext&) {
        return sessions::replyGiven(payload("c1600.exists", Value(m_app.tapes().exists(text(a, "name")))));
    });
    cmd("c1600.record-tape", [this, refreshed, errorOutcome](const Value& a, SessionContext& c) -> CommandOutcome {
        std::string error;
        m_app.tapes().recordNew(text(a, "name"), &error);
        return refreshed(errorOutcome(error), c);
    });

    // ── Saved media ─────────────────────────────────────────────────────
    req("c1600.media-items", [this](const Value&, SessionContext&) {
        Value::Array items;
        for (const MediaItem& item : m_library->items())
            items.push_back(Value::object({{"type", item.type == MediaType::Card   ? "card"
                                                    : item.type == MediaType::Disk ? "disk"
                                                                                   : "tape"},
                                           {"name", item.name},
                                           {"description", item.description},
                                           {"fileName", item.fileName},
                                           {"path", item.path},
                                           {"isTemplate", item.isTemplate},
                                           {"inUse", item.inUse}}));
        return sessions::replyGiven(payload("c1600.media", Value(std::move(items))));
    });
    req("c1600.export-media", [this](const Value& a, SessionContext&) {
        std::vector<std::uint8_t> b;
        if (!m_library->exportBytes(text(a, "path"), &b)) return sessions::replyRefused("The file could not be read.");
        return sessions::replyGiven(payload("c1600.bytes", Value(std::move(b))));
    });
    cmd("c1600.import-media", [this, refreshed](const Value& a, SessionContext& c) -> CommandOutcome {
        const OpResult r = m_library->importFile(text(a, "name"), bytes(a, "bytes"));
        return refreshed(r.ok ? CommandOutcome(Done{}) : CommandOutcome(Refused{r.error}), c);
    });
    cmd("c1600.remove-media", [this, refreshed](const Value& a, SessionContext& c) -> CommandOutcome {
        const OpResult r = m_library->remove(text(a, "path"));
        return refreshed(r.ok ? CommandOutcome(Done{}) : CommandOutcome(Refused{r.error}), c);
    });

    // ── Inspector ───────────────────────────────────────────────────────
    req("c1600.inspector-menu", [this](const Value& a, SessionContext&) {
        const bool pc1500 = m_app.currentModel() != Model::PC1600;
        Value::Array out;
        for (const inspect::MenuEntry& e : m_app.inspectorMenu(flag(a, "dumps")))
            out.push_back(Value::object({{"view", static_cast<int>(e.view)},
                                         {"title", std::string(inspect::viewTitle(e.view, pc1500))},
                                         {"enabled", e.enabled}}));
        return sessions::replyGiven(payload("c1600.inspector-menu", Value(std::move(out))));
    });
    req("c1600.inspector-view", [this](const Value& a, SessionContext&) {
        return sessions::replyGiven(payload(
            "c1600.inspector-view", Value(joinLines(m_app.inspectorView(static_cast<inspect::View>(integer(a, "view")))))));
    });
    req("c1600.card-yaml", [this](const Value&, SessionContext&) {
        return sessions::replyGiven(payload("c1600.card-yaml", strings(cardYamlDump(m_app))));
    });

    // ── Copy Screen ─────────────────────────────────────────────────────
    req("c1600.screen-image", [this](const Value&, SessionContext& c) {
        const calcu::machine::DisplayOutput* d = c.display();
        if (!d) return sessions::replyRefused("No machine is running.");
        const GrayImage image = m_app.screenImage(*d);
        LcdText t;
        return sessions::replyGiven(payload("c1600.screen-image",
                                            Value::object({{"width", image.width},
                                                           {"height", image.height},
                                                           {"dpi", image.dpi},
                                                           {"pixels", Value(Value::Bytes(image.pixels))},
                                                           {"text", m_app.lcdText(*d, &t) ? t.plainText() : std::string()}})));
    });
    req("c1600.lcd-text", [this](const Value&, SessionContext& c) {
        const calcu::machine::DisplayOutput* d = c.display();
        LcdText t;
        return sessions::replyGiven(payload("c1600.lcd-text", Value(d && m_app.lcdText(*d, &t) ? t.text() : std::string())));
    });

    // ── Plotter paper ───────────────────────────────────────────────────
    // The pane width in CSS pixels the page draws the paper at.
    cmd("c1600.paper-width", [this](const Value& a, SessionContext&) -> CommandOutcome {
        const double w = real(a, "width");
        if (w != m_paperWidth) {
            m_paperWidth = w;
            paper(true);
        }
        return Done{};
    });
    req("c1600.paper-export", [this](const Value&, SessionContext&) {
        const PaperKind kind = m_app.controlState().paper;
        const auto points = m_app.plotPoints(kind);
        if (points.empty()) return sessions::replyGiven(payload("c1600.paper-export", Value()));
        const PaperGeometry geom = PaperGeometry::forKind(kind);
        const auto [lower, upper] = penYRange(points);
        const PaperExportSize size = paperExportSize(geom, lower, upper);
        return sessions::replyGiven(payload(
            "c1600.paper-export",
            Value::object({{"width", size.widthPx},
                           {"height", size.heightPx},
                           {"scale", size.scale},
                           {"paneWidth", size.paneWidthPt},
                           {"contentHeight", size.contentHeightPt},
                           {"dotsPerMeter", size.dotsPerMeter},
                           {"strokes", strokes(buildPaperStrokes(geom, size.paneWidthPt, points, lower))}})));
    });
    cmd("c1600.clear-paper", [this](const Value&, SessionContext&) -> CommandOutcome {
        m_app.clearPaper(m_app.controlState().paper);
        paper(true);
        return Done{};
    });

    // ── Settings ────────────────────────────────────────────────────────
    req("c1600.settings-state", [this](const Value&, SessionContext&) {
        const PresetCatalog catalog(m_paths);
        Value::Object presets, choices;
        for (Model m : {Model::PC1500, Model::PC1500A, Model::PC1600}) {
            const std::string current = m_settings.defaultPresetPath(m);
            presets.push_back({modelKey(m), Value(current)});
            Value::Array list;
            for (const auto& e : catalog.defaultChoices(m, current)) list.push_back(presetEntry(e));
            choices.push_back({modelKey(m), Value(std::move(list))});
        }
        return sessions::replyGiven(payload("c1600.settings",
                                            Value::object({{"startupModel", m_settings.startupModelPreference()},
                                                           {"defaultPresets", Value(std::move(presets))},
                                                           {"presetChoices", Value(std::move(choices))}})));
    });
    req("c1600.preset-examples", [this](const Value&, SessionContext&) {
        Value::Array out;
        for (const auto& e : PresetCatalog(m_paths).examples()) out.push_back(presetEntry(e));
        return sessions::replyGiven(payload("c1600.examples", Value(std::move(out))));
    });
    cmd("c1600.set-startup-model", [this](const Value& a, SessionContext&) -> CommandOutcome {
        m_settings.setStartupModelPreference(text(a, "model"));
        return Done{};
    });
    cmd("c1600.set-default-preset", [this](const Value& a, SessionContext&) -> CommandOutcome {
        m_settings.setDefaultPresetPath(static_cast<Model>(integer(a, "model")), text(a, "path"));
        return Done{};
    });

    // ── Static data (adapter W16-1, until M-49) ─────────────────────────
    req("c1600.faceplate", [](const Value& a, SessionContext&) {
        return sessions::replyGiven(payload("c1600.faceplate", faceplate(static_cast<int>(integer(a, "model")))));
    });
    req("c1600.lcd-constants", [](const Value&, SessionContext&) {
        return sessions::replyGiven(payload("c1600.lcd-constants", lcdConstants()));
    });
}

void SharpWebPart::afterCall(SessionContext& c, calcu::pacing::WallTime) {
    lcd(c);
    lamps();
    paper(false);
    printer();
    // The pickers read folders: twice a second, as a bay's change (a
    // preset's save, a recording) shows up; at once after a command.
    if (++m_calls % 30 == 0) m_ui->set(payload("c1600.ui", ui()));
}

void SharpWebPart::machineChanged(SessionContext& c) { refresh(c); }
void SharpWebPart::operationEnded(SessionContext& c) { refresh(c); }

void SharpWebPart::refresh(SessionContext& c) {
    m_ui->set(payload("c1600.ui", ui()));
    lcd(c);
    lamps();
    paper(true);
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
                     {"paper", Value(paperName(s.paper))},
                     {"printerShown", Value(s.printerShown)},
                     {"pc1500Rom", Value(static_cast<int>(m_app.pc1500RomRevision()))},
                     {"pc1600Rom", Value(static_cast<int>(m_app.pc1600RomVersion()))},
                     {"ce1600pRom", Value(static_cast<int>(m_app.ce1600pRomVersion()))}};
    Value::Array slots;
    for (int slot = 1; slot <= s.slotCount; ++slot) {
        const auto lists = m_app.modules().moduleLists(ModuleBay::hostForModel(slot, s.model));
        Value::Object v;
        pickerGroups(v, {{"templates", &lists.templates}, {"instances", &lists.instances}, {"roms", &lists.roms}},
                     &ModuleBay::ModuleEntry::moduleName);
        v.push_back({"selected", Value(m_app.modules().selectedModuleName(slot))});
        v.push_back({"canSave", Value(m_app.modules().canNameAndSave(slot))});
        v.push_back({"canWriteProtect", Value(m_app.modules().canWriteProtect(slot))});
        slots.emplace_back(std::move(v));
    }
    ui.push_back({"slots", Value(std::move(slots))});
    if (s.floppyShown) {
        const auto disks = m_app.floppy().diskLists();
        Value::Object v;
        pickerGroups(v, {{"templates", &disks.templates}, {"instances", &disks.instances}}, &FloppyBay::DiskEntry::diskName);
        v.push_back({"selected", Value(m_app.floppy().selectedDiskName())});
        v.push_back({"side", Value(m_app.floppy().side())});
        v.push_back({"canSave", Value(m_app.floppy().canNameAndSave())});
        ui.push_back({"floppy", Value(std::move(v))});
    }
    if (s.tapeShown)
        ui.push_back({"tape", Value::object({{"names", strings(m_app.tapes().pickerNames())},
                                            {"current", m_app.tapes().currentName()}})});
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
    m_lcd->set(payload("c1600.lcd", Value::object({{"model", static_cast<int>(model)},
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
    m_lamps->set(payload(
        "c1600.lamps",
        Value::object({{"floppyMotor", state.floppyShown && m_app.floppy().motorOn()},
                       {"tapeMode", tape.mode == TapeDeck::Mode::Play ? "play" : tape.mode == TapeDeck::Mode::Record ? "record" : ""},
                       {"tapeMotor", tape.motor},
                       {"tapeSeconds", static_cast<int>(tape.position)},
                       {"tapeFile", fromPath(toPath(tape.path).filename())}})));
}

// The strokes at the page's pane width, when the plot changed.
void SharpWebPart::paper(bool force) {
    const PaperKind kind = m_app.controlState().paper;
    if (kind == PaperKind::None || m_paperWidth <= 0) return;
    const std::uint64_t rev = m_app.plotRevision(kind);
    if (!force && rev == m_paperRevision && kind == m_paperKind) return;
    m_paperRevision = rev;
    m_paperKind = kind;
    const auto points = m_app.plotPoints(kind);
    const PaperGeometry geom = PaperGeometry::forKind(kind);
    const auto [lower, upper] = penYRange(points);
    m_paper->set(payload("c1600.paper", Value::object({{"kind", paperName(kind)},
                                                       {"empty", points.empty()},
                                                       {"contentHeight", geom.contentHeight(0, lower, upper, m_paperWidth)},
                                                       {"width", m_paperWidth},
                                                       {"revision", static_cast<std::int64_t>(rev)},
                                                       {"strokes", strokes(buildPaperStrokes(geom, m_paperWidth, points, lower))}})));
}

void SharpWebPart::printer() {
    if (!m_app.ce158Attached()) return;
    const std::vector<std::uint8_t> b = m_app.drainCE158PrinterOutput();
    if (b.empty()) return;
    m_printer->append(payload("c1600.printer", Value::object({{"bytes", Value(Value::Bytes(b))},
                                                              {"text", ce158PrintableText(b.data(), b.size())}})));
}

} // namespace shared
