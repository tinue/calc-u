#include "harness/Run.hpp"
#include "debugger/DebuggerPart.hpp"
#include "media/PresetMedia.hpp"
#include "harness/StatePart.hpp"
#include "paper/PaperPart.hpp"
#include "sessions/LibraryTopics.hpp"
#include "sessions/MachineSession.hpp"
#include <atomic>
#include <cctype>
#include <ctime>
#include <filesystem>

namespace calcu::harness {

namespace fs = std::filesystem;

namespace {
std::string pathText(const fs::path& p) {
    const std::u8string u = p.u8string();
    return std::string(u.begin(), u.end());
}
fs::path pathOf(const std::string& s) { return fs::path(std::u8string(s.begin(), s.end())); }

bool digits(std::string_view s, std::size_t at, std::size_t n, int* out) {
    int v = 0;
    for (std::size_t i = at; i < at + n; ++i) {
        if (i >= s.size() || !std::isdigit(static_cast<unsigned char>(s[i]))) return false;
        v = v * 10 + (s[i] - '0');
    }
    *out = v;
    return true;
}

std::string lower(std::string_view s) {
    std::string out(s);
    for (char& c : out) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return out;
}

void replaceAll(std::string& s, const std::string& what, const std::string& with) {
    if (what.empty()) return;
    for (std::size_t at = s.find(what); at != std::string::npos; at = s.find(what, at + with.size()))
        s.replace(at, what.size(), with);
}
} // namespace

std::optional<LocalDateTime> parseDateTime(std::string_view s) {
    LocalDateTime t;
    if (s.size() != 19 || s[4] != '-' || s[7] != '-' || s[10] != 'T' || s[13] != ':' || s[16] != ':') return std::nullopt;
    if (!digits(s, 0, 4, &t.year) || !digits(s, 5, 2, &t.month) || !digits(s, 8, 2, &t.day) ||
        !digits(s, 11, 2, &t.hour) || !digits(s, 14, 2, &t.minute) || !digits(s, 17, 2, &t.second))
        return std::nullopt;
    if (t.year < 1970 || t.month < 1 || t.month > 12 || t.day < 1 || t.day > 31 || t.hour > 23 || t.minute > 59 ||
        t.second > 59)
        return std::nullopt;
    return t;
}

std::chrono::system_clock::time_point localInstant(const LocalDateTime& t) {
    std::tm tm{};
    tm.tm_year = t.year - 1900;
    tm.tm_mon = t.month - 1;
    tm.tm_mday = t.day;
    tm.tm_hour = t.hour;
    tm.tm_min = t.minute;
    tm.tm_sec = t.second;
    tm.tm_isdst = -1;   // the host's rule for that date
    return std::chrono::system_clock::from_time_t(std::mktime(&tm));
}

// ── RunSetup ────────────────────────────────────────────────────────────

std::unique_ptr<RunSetup> RunSetup::make(const RunOptions& o, const settings::ProductDecl& decl, std::string* error) {
    std::unique_ptr<RunSetup> r(new RunSetup());
    r->options_ = o;
    static std::atomic<int> n{0};
    std::error_code ec;
    const fs::path base = fs::temp_directory_path(ec);
    if (ec) {
        *error = "There is no folder for temporary files: " + ec.message();
        return nullptr;
    }
    const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
    const fs::path run = base / ("calcu-run-" + std::to_string(stamp) + "-" + std::to_string(n++));
    fs::create_directories(run / "save", ec);
    if (ec) {
        *error = "Cannot make the run folder " + pathText(run) + ": " + ec.message();
        return nullptr;
    }
    r->runFolder_ = pathText(run);
    if (o.saveFolder) {
        fs::create_directories(pathOf(*o.saveFolder), ec);
        if (ec || !fs::is_directory(pathOf(*o.saveFolder), ec)) {
            *error = "Cannot use the save folder " + *o.saveFolder + ".";
            return nullptr;
        }
        r->saveFolder_ = pathText(fs::absolute(pathOf(*o.saveFolder), ec));
    } else {
        r->saveFolder_ = pathText(run / "save");
    }
    if (o.outputFolder) {
        fs::create_directories(pathOf(*o.outputFolder), ec);
        if (ec || !fs::is_directory(pathOf(*o.outputFolder), ec)) {
            *error = "Cannot use the output folder " + *o.outputFolder + ".";
            return nullptr;
        }
        r->outputFolder_ = pathText(fs::absolute(pathOf(*o.outputFolder), ec));
    } else {
        r->outputFolder_ = r->saveFolder_;
    }
    r->settings_ = std::make_unique<settings::SettingsFile>(
        o.settingsFile ? settings::SettingsFile::open(decl, *o.settingsFile) : settings::SettingsFile::defaultsOnly(decl));
    // LIB-9 AC-18, D-11: Qt6's rules on folders inside the run folder.
    r->dirs_.frontend = settings::FrontendKind::Qt6;
    r->dirs_.home = pathText(run / "home");
    r->dirs_.settingsBase = pathText(run / "settings");
    r->dirs_.saveFolder = r->saveFolder_;
    return r;
}

RunSetup::~RunSetup() {
    std::error_code ec;
    if (!runFolder_.empty()) fs::remove_all(pathOf(runFolder_), ec);
}

std::function<std::chrono::system_clock::time_point()> RunSetup::hostTime() const {
    switch (options_.clock) {
    case ClockMode::Host: return [] { return std::chrono::system_clock::now(); };
    case ClockMode::Given: {
        const auto t = localInstant(options_.given);
        return [t] { return t; };
    }
    case ClockMode::Fixed: break;
    }
    const auto t = localInstant(LocalDateTime{});
    return [t] { return t; };
}

std::string RunSetup::scrub(std::string s) const {
    replaceAll(s, saveFolder_, "<save folder>");
    replaceAll(s, runFolder_, "<run folder>");
    return s;
}

// ── Server ──────────────────────────────────────────────────────────────

std::unique_ptr<Server> Server::start(HarnessProduct& product, const RunOptions& options, std::string* error) {
    std::unique_ptr<Server> s(new Server());
    s->product_ = &product;
    s->setup_ = RunSetup::make(options, product.settingsDecl(), error);
    if (!s->setup_) return nullptr;

    // EC-1 (LIB-12 AC-7): the full set, or no start.
    roms::LookupResult roms = product.lookUpRoms();
    if (!roms.ok()) {
        *error = roms::describe(roms.missing);
        return nullptr;
    }
    s->peripherals_ = std::make_unique<peripherals::PeripheralsPart>();
    s->state_ = std::make_unique<MachineStatePart>(std::string(product.name()), std::string(product.version()));
    sessions::AppSessionSetup setup;
    // AC-6: the wall clock of a session hosted in virtual time paces nothing;
    // this one stands still.
    auto reads = s->clockReads_;
    setup.clock = [reads] {
        ++*reads;
        return pacing::WallTime{0};
    };
    setup.hostTime = s->setup_->hostTime();   // AC-21 (LIB-3 A-7)
    setup.hosting = sessions::Hosting::VirtualTime;
    // LIB-6 (design §2): the loader of preset format 2 when the product has
    // rules, before StartupPart, with the run's folder for screenshots
    // (LIB-11 AC-8); LIB-18's PresetLoader (LIB-18 D-9).
    if (presets::ProductRules* rules = product.presetRules()) {
        presets::PresetsSetup ps;
        ps.catalogue = &product.catalogue();
        ps.rules = rules;
        ps.typing = product.typingPart();
        ps.bundle = product.presetBundle();
        ps.home = s->setup_->hostDirs().home;
        RunSetup* run = s->setup_.get();
        ps.screenshotFolder = [run]() -> std::optional<std::string> { return run->screenshotFolder(); };
        s->presets_ = std::make_unique<presets::PresetsPart>(std::move(ps));
    }
    // LIB-18 §3.6: the start is StartupPart's, after PeripheralsPart (the
    // clocks at a build and at the end of every operation, AC-1, AC-13);
    // the run's own settings (LIB-9 AC-18).
    s->startup_ = std::make_unique<startup::StartupPart>(
        startup::StartupSetup{product.machines(), roms.set, s->setup_->settings(), s->presets_.get()});
    if (s->presets_) s->presets_->setControl(&s->startup_->control());
    // LIB-8 (A-2, A-3): the paper part's machine control and the step `cut-paper`.
    if (paper::PaperPart* paper = product.paperPart()) {
        paper->setControl(&s->startup_->control());
        if (s->presets_) paper::addPaperStep(s->presets_->vocabulary(), *paper);
    }
    // LIB-10: the debugger's trace folder (the run's, AC-8), its limit
    // ([trace] max-file-size-mb of the run's settings) and its steps.
    if (debugger::DebuggerPart* dp = product.debuggerPart()) {
        RunSetup* run = s->setup_.get();
        dp->setTraceFolder([run]() -> std::optional<settings::Paths::Ensured> {
            std::error_code ec;
            fs::create_directories(pathOf(run->traceFolder()), ec);
            if (ec) return settings::Paths::Ensured{false, run->traceFolder(), ec.message()};
            return settings::Paths::Ensured{true, run->traceFolder(), {}};
        });
        dp->setTraceLimit([run] { return run->settings().integer("trace", "max-file-size-mb"); });
        if (s->presets_) debugger::addDebuggerSteps(s->presets_->vocabulary(), *dp);
    }
    // LIB-4 §7: the control state, among the library's parts.
    s->control_ = std::make_unique<control::ControlPart>(product.controlSetup());
    // LIB-7 §3.11: the media part on the run's save folder (LIB-11 AC-8),
    // the product's bundled media always there; the media steps on LIB-6's place.
    if (std::vector<media::KindDecl> kinds = product.mediaKinds(); !kinds.empty()) {
        settings::HostDirs dirs = s->setup_->hostDirs();
        dirs.frontend = settings::FrontendKind::Swift;   // the save folder as the run passes it
        s->mediaPaths_ = std::make_unique<settings::Paths>(s->setup_->settings(), dirs);
        media::MediaSetup ms;
        ms.kinds = std::move(kinds);
        ms.paths = s->mediaPaths_.get();
        ms.listing = product.moduleListing();
        ms.catalogue = &product.catalogue();
        s->media_ = std::make_unique<media::MediaPart>(std::move(ms));
        s->media_->setControl(&s->startup_->control());
        if (s->presets_) media::addMediaToPresets(s->presets_->vocabulary(), *s->media_);
    }
    setup.parts = {s->peripherals_.get(), s->state_.get()};
    if (s->presets_) setup.parts.push_back(s->presets_.get());
    for (sessions::SessionPart* p : {static_cast<sessions::SessionPart*>(s->startup_.get()),
                                     static_cast<sessions::SessionPart*>(s->control_.get())})
        setup.parts.push_back(p);
    if (s->media_) setup.parts.push_back(s->media_.get());
    for (sessions::SessionPart* p : product.sessionParts()) setup.parts.push_back(p);
    s->session_ = std::make_unique<sessions::AppSession>(std::move(setup));
    s->host_ = std::make_unique<HarnessHost>(*s->session_, product, *s->setup_);
    // LIB-11 AC-2: the machine the product starts with, at key readiness.
    const sessions::ReplyMessage started = s->host_->command(sessions::Command{sessions::kStartCommand, {}});
    if (!started.done) {
        *error = "The machine was not built: " + started.reason;
        return nullptr;
    }

    addFirstCommands(s->registry_);
    addStartupCommands(s->registry_);
    addPresetCommands(s->registry_, s->presets_ ? &s->presets_->vocabulary() : nullptr);
    addPaperCommands(s->registry_);
    addDebugCommands(s->registry_);
    addMediaCommands(s->registry_, s->media_.get());
    product.addCommands(s->registry_);
    return s;
}

Server::~Server() { finish(); }

void Server::finish() {
    if (finished_) return;
    finished_ = true;
    if (host_) host_->finish();
    host_.reset();
    if (session_) session_->close();   // LIB-7 AC-13: the parts write what they have to save
    session_.reset();
    setup_.reset();   // removes the run folder
}

Server::Result Server::line(std::string_view text) {
    Result res;
    auto parsed = parseLine(text, registry_);
    Reply reply;
    bool quit = false;
    if (auto* refusal = std::get_if<std::string>(&parsed)) {
        reply = refusedReply(*refusal);
    } else {
        const CommandLine& cl = std::get<CommandLine>(parsed);
        if (cl.path.empty()) return res;   // an empty line
        const Handler* h = registry_.find(cl.path);
        CommandContext ctx{*host_, cl};
        reply = (*h)(ctx);
        quit = ctx.quit;
    }
    res.done = reply.done && !reply.failsRun;
    res.quit = quit;
    if (!quit) res.text = setup_->scrub(setup_->options().json ? writeJson(reply) : writePlain(reply));
    return res;
}

int Server::serve(const std::function<std::optional<std::string>()>& readLine,
                  const std::function<void(const std::string&)>& write) {
    while (auto l = readLine()) {
        Result r = line(*l);
        if (r.quit) break;
        if (!r.text.empty()) write(r.text);
    }
    finish();   // EC-3: as after quit
    return 0;
}

int Server::oneShot(const std::vector<std::string>& lines, const std::function<void(const std::string&)>& write) {
    int status = 0;
    for (const auto& l : lines) {
        Result r = line(l);
        if (r.quit) break;
        if (!r.text.empty()) write(r.text);
        if (!r.done) {
            status = 1;
            break;
        }
    }
    finish();
    return status;
}

HarnessProduct* findProduct(const std::vector<std::unique_ptr<HarnessProduct>>& products, std::string_view name) {
    const std::string want = lower(name);
    for (const auto& p : products)
        if (lower(p->name()) == want || lower(p->shortName()) == want) return p.get();
    return nullptr;
}

} // namespace calcu::harness
