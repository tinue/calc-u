#include "app/SharpWebPart.hpp"

#include <filesystem>
#include <fstream>

#include "Connector/PrintableBytes.hpp"
#include "Display/DisplayOutputText.hpp"
#include "Utf8.hpp"
#include "app/CardYamlDump.hpp"
#include "app/SharpShellProduct.hpp"
#include "app/Text.hpp"
#include "files/Files.hpp"
#include "sessions/Args.hpp"
#include "shell/ValueForms.hpp"

namespace cu1600 {

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
    : m_app(app), m_uploadsDir(std::move(uploadsDir)), m_version(std::move(version)) {}

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
            sessions::valuePayload("c1600.shell-product", Value::object({{"product", calcu::shell::toValue(shellProduct(m_version, "", false))},
                                                         {"settingsSpec", calcu::shell::toValue(settingsSpec(false, false))}})));
    });
    req("c1600.faceplate-map", [](const Value& a, SessionContext&) {
        const auto model = sharpModel(textArg(a, "model"));
        if (!model) return sessions::replyRefused("No such model: " + textArg(a, "model"));
        return sessions::replyGiven(sessions::valuePayload("c1600.faceplate-map", faceplateMapValue(*model)));
    });

    // ── Opening files ───────────────────────────────────────────────────
    // A drop, Upload Preset… and Import Media… go through the library
    // (LIB-13's `open` with the uploads folder for several files, LIB-7's
    // `media.import`); the page sends no `c1600.upload`. It writes one file
    // into the uploads folder and replies its path; the Web tests hand a
    // preset over with it (`session.spec.js`; LIB-13, LIB-7 and C1600-2's
    // designs keep it). A test aid: only with CALCU_WEB_TEST_AIDS, so a
    // release Web build (`build.sh --release`) has none (LIB-15 AC-29).
#ifdef CALCU_WEB_TEST_AIDS
    cmd("c1600.upload", [this](const Value& a, SessionContext&) -> CommandOutcome {
        const std::string path = m_uploadsDir + "/" + calcu::files::fromPath(calcu::files::toPath(textArg(a, "name")).filename());
        std::error_code ec;
        std::filesystem::create_directories(calcu::files::toPath(m_uploadsDir), ec);
        const std::vector<std::uint8_t> b = bytesArg(a, "bytes").value_or(std::vector<std::uint8_t>{});
        std::ofstream out(calcu::files::toPath(path), std::ios::binary | std::ios::trunc);
        out.write(reinterpret_cast<const char*>(b.data()), static_cast<std::streamsize>(b.size()));
        if (!out) return Refused{"The file could not be written: " + path};
        return result("c1600.path", Value(path));
    });
#endif
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
#ifdef CALCU_WEB_TEST_AIDS
    // The display as text, for the Web tests (the harness's `display-text`);
    // a test aid, as `c1600.upload`.
    req("c1600.display-text", [this](const Value&, SessionContext& c) {
        const calcu::machine::DisplayOutput* d = c.display();
        LcdText t;
        return sessions::replyGiven(sessions::valuePayload("c1600.display-text", Value(d && m_app.lcdText(*d, &t) ? t.text() : std::string())));
    });
#endif
}

void SharpWebPart::afterCall(SessionContext&, calcu::pacing::WallTime) { printer(); }

void SharpWebPart::printer() {
    if (!m_app.ce158Attached()) return;
    const std::vector<std::uint8_t> b = m_app.drainCE158PrinterOutput();
    if (b.empty()) return;
    m_printer->append(sessions::valuePayload("c1600.printer", Value::object({{"bytes", Value(Value::Bytes(b))},
                                                              {"text", printableBytes(b.data(), b.size())}})));
}

} // namespace cu1600
