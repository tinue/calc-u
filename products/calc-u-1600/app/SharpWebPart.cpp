#include "app/SharpWebPart.hpp"

#include <filesystem>
#include <fstream>

#include "Connector/PrintableBytes.hpp"
#include "Display/DisplayOutputText.hpp"
#include "Utf8.hpp"
#include "app/CardYamlDump.hpp"
#include "app/SharpShellProduct.hpp"
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

} // namespace

SharpWebPart::SharpWebPart(SharpAppPart& app, std::string uploadsDir, std::string version)
    : m_app(app), m_uploadsDir(std::move(uploadsDir)), m_version(std::move(version)) {
    // LIB-13 AC-8: several files of Upload Preset… become one folder here.
    m_app.openPart().setup().uploadFolder = m_uploadsDir;
}

SharpWebPart::~SharpWebPart() = default;

void SharpWebPart::attach(sessions::SessionSetup& setup) {
    m_printer = &setup.streamTopic(topics::kWebPrinter, sessions::History::None);

    using H = sessions::SessionSetup::Handler;
    auto cmd = [&setup](const char* name, H h) { setup.command(sessions::CommandName{name}, std::move(h)); };
    auto req = [&setup](const char* name, sessions::SessionSetup::RequestHandler h) {
        setup.request(sessions::RequestName{name}, std::move(h));
    };

    // ── The shell (LIB-14 Part D) ───────────────────────────────────────
    // The product's data for the React set's shell (D-32) and the
    // faceplate's hit map and LCD layout (D-31), from the tables Qt6 uses.
    req("c1600.shell-product", [this](const Value&, SessionContext&) {
        return sessions::replyGiven(
            sessions::valuePayload("c1600.shell-product", Value::object({{"product", shellProductValue(shellProduct(m_version, "", false))}, {"settingsSpec", dialogSpecValue(settingsSpec(false, false))}})));
    });
    req("c1600.faceplate-map", [](const Value& a, SessionContext&) {
        const auto model = sharpModel(textArg(a, "model"));
        if (!model) return sessions::replyRefused("No such model: " + textArg(a, "model"));
        return sessions::replyGiven(sessions::valuePayload("c1600.faceplate-map", faceplateMapValue(*model)));
    });

    // ── Opening files ───────────────────────────────────────────────────
    // A drop and Upload Preset… are the library's `open` (LIB-13, the open
    // part of SharpAppPart, with the uploads folder for several files).
    // Import Media… puts its files into the uploads folder first, so a tape
    // finds its WAV beside its definition (LIB-7 AC-43); the result is the
    // path.
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
    // Load BASIC Program / Load Machine Code: the page sends the loaders'
    // own commands (app/SharpLoaderPart, C1600-2 design §3.7).

    // ── Inspector ───────────────────────────────────────────────────────
    req("c1600.inspector-menu", [this](const Value& a, SessionContext&) {
        const bool pc1500 = !m_app.isPC1600();
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

    // ── Copy Screen (LIB-14 AC-41) ──────────────────────────────────────
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
}

void SharpWebPart::afterCall(SessionContext&, calcu::pacing::WallTime) { printer(); }

void SharpWebPart::printer() {
    if (!m_app.ce158Attached()) return;
    const std::vector<std::uint8_t> b = m_app.drainCE158PrinterOutput();
    if (b.empty()) return;
    m_printer->append(sessions::valuePayload("c1600.printer", Value::object({{"bytes", Value(Value::Bytes(b))},
                                                              {"text", printableBytes(b.data(), b.size())}})));
}

} // namespace shared
