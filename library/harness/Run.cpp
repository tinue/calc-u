#include "harness/Run.hpp"
#include "files/Files.hpp"
#include "machine/Names.hpp"
#include "debugger/DebuggerPart.hpp"
#include "media/PresetMedia.hpp"
#include "harness/StatePart.hpp"
#include "paper/PaperPart.hpp"
#include "sessions/LibraryTopics.hpp"
#include "sessions/MachineSession.hpp"
#include "sessions/Operation.hpp"
#include <atomic>
#include <cctype>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <thread>
#include <ctime>
#include <filesystem>

namespace calcu::harness {

namespace fs = std::filesystem;

namespace {

bool digits(std::string_view s, std::size_t at, std::size_t n, int* out) {
    int v = 0;
    for (std::size_t i = at; i < at + n; ++i) {
        if (i >= s.size() || !std::isdigit(static_cast<unsigned char>(s[i]))) return false;
        v = v * 10 + (s[i] - '0');
    }
    *out = v;
    return true;
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

namespace {

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

} // namespace

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
        *error = "Cannot make the run folder " + files::fromPathNative(run) + ": " + ec.message();
        return nullptr;
    }
    r->runFolder_ = files::fromPathNative(run);
    if (o.saveFolder) {
        fs::create_directories(files::toPath(*o.saveFolder), ec);
        if (ec || !fs::is_directory(files::toPath(*o.saveFolder), ec)) {
            *error = "Cannot use the save folder " + *o.saveFolder + ".";
            return nullptr;
        }
        r->saveFolder_ = files::fromPathNative(fs::absolute(files::toPath(*o.saveFolder), ec));
    } else {
        r->saveFolder_ = files::fromPathNative(run / "save");
    }
    if (o.outputFolder) {
        fs::create_directories(files::toPath(*o.outputFolder), ec);
        if (ec || !fs::is_directory(files::toPath(*o.outputFolder), ec)) {
            *error = "Cannot use the output folder " + *o.outputFolder + ".";
            return nullptr;
        }
        r->outputFolder_ = files::fromPathNative(fs::absolute(files::toPath(*o.outputFolder), ec));
    } else {
        r->outputFolder_ = r->saveFolder_;
    }
    r->settings_ = std::make_unique<settings::SettingsFile>(
        o.settingsFile ? settings::SettingsFile::open(decl, *o.settingsFile) : settings::SettingsFile::defaultsOnly(decl));
    // The folders as the run passes them, the others
    // inside the run folder.
    r->dirs_.frontend = settings::FrontendKind::Given;
    r->dirs_.home = files::fromPathNative(run / "home");
    r->dirs_.settingsBase = files::fromPathNative(run / "settings");
    r->dirs_.saveFolder = r->saveFolder_;
    r->dirs_.traceFolder = r->outputFolder_;
    r->dirs_.screenshotFolder = r->outputFolder_;
    r->paths_ = std::make_unique<settings::Paths>(*r->settings_, r->dirs_);
    return r;
}

RunSetup::~RunSetup() {
    std::error_code ec;
    if (!runFolder_.empty()) fs::remove_all(files::toPath(runFolder_), ec);
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
    // The product's app layer on the run's settings and folders;
    // the ROM full set, or no start.
    const roms::LookupResult roms = product.prepare(*s->setup_);
    if (!roms.ok()) {
        *error = roms::describe(roms.missing);
        return nullptr;
    }
    s->state_ = std::make_unique<MachineStatePart>(std::string(product.name()), std::string(product.version()));
    sessions::AppSessionSetup setup;
    // The wall clock of a session hosted in virtual time paces nothing;
    // this one stands still.
    auto reads = s->clockReads_;
    setup.clock = [reads] {
        ++*reads;
        return pacing::WallTime{0};
    };
    setup.hostTime = s->setup_->hostTime();
    setup.hosting = sessions::Hosting::VirtualTime;
    // The one order (LibraryParts): the control part, the run's state part
    // among the host's, the library's and the product's parts.
    setup.parts = product.sessionParts({s->state_.get()});
    s->session_ = std::make_unique<sessions::AppSession>(std::move(setup));
    s->host_ = std::make_unique<HarnessHost>(*s->session_, product, *s->setup_);
    // The machine the product starts with, at key readiness.
    const sessions::ReplyMessage started = s->host_->command(sessions::Command{sessions::kStartCommand, {}});
    if (!started.done) {
        *error = "The machine was not built: " + started.reason;
        return nullptr;
    }

    addServeCommands(s->registry_, product.libraryParts(), product);
    return s;
}

void addServeCommands(CommandRegistry& r, parts::LibraryParts& parts, HarnessProduct& product) {
    addFirstCommands(r);
    addStartupCommands(r);
    addPresetCommands(r, &parts.presets().vocabulary());
    addPaperCommands(r);
    addDebugCommands(r);
    addMediaCommands(r, parts.media());
    addOpenCommand(r);
    product.addCommands(r);
}

Server::~Server() { finish(); }

void Server::finish() {
    if (finished_) return;
    finished_ = true;
    if (host_) host_->finish();
    host_.reset();
    if (session_) session_->close();   // the parts write what they have to save
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
    finish();   // at the end of the input: as after quit
    return 0;
}

// ── serve with add-ons ──────────────────────────────────────────────────────

struct Server::Inbox {
    struct Item {
        enum class Kind { Line, End, Task } kind = Kind::Line;
        std::string line;
        std::function<void()> task;
    };
    std::mutex mutex;
    std::condition_variable ready;
    std::deque<Item> items;
    bool quit = false;   // the loop's thread only

    void push(Item i) {
        {
            std::lock_guard<std::mutex> lock(mutex);
            items.push_back(std::move(i));
        }
        ready.notify_one();
    }
};

ServeLoop Server::serveLoop() {
    if (!inbox_) inbox_ = std::make_shared<Inbox>();
    std::weak_ptr<Inbox> weak = inbox_;
    ServeLoop loop;
    loop.post = [weak](std::function<void()> task) {
        if (auto in = weak.lock()) in->push(Inbox::Item{Inbox::Item::Kind::Task, {}, std::move(task)});
    };
    loop.quit = [weak] {
        if (auto in = weak.lock()) in->quit = true;
    };
    return loop;
}

int Server::serveWith(const std::vector<ServeAddOn*>& addOns, std::function<std::optional<std::string>()> readLine,
                      const std::function<void(const std::string&)>& write) {
    if (addOns.empty()) return serve(readLine, write);
    if (!inbox_) inbox_ = std::make_shared<Inbox>();
    std::shared_ptr<Inbox> inbox = inbox_;
    auto readerDone = std::make_shared<std::atomic<bool>>(false);
    std::thread reader([inbox, readLine, readerDone] {
        while (std::optional<std::string> l = readLine()) inbox->push(Inbox::Item{Inbox::Item::Kind::Line, std::move(*l), {}});
        inbox->push(Inbox::Item{Inbox::Item::Kind::End, {}, {}});
        *readerDone = true;
    });
    const auto wantsTime = [&addOns] {
        for (ServeAddOn* a : addOns)
            if (a->wantsTime()) return true;
        return false;
    };
    for (;;) {
        std::optional<Inbox::Item> item;
        {
            std::lock_guard<std::mutex> lock(inbox->mutex);
            if (!inbox->items.empty()) {
                item = std::move(inbox->items.front());
                inbox->items.pop_front();
            }
        }
        if (!item) {   // an add-on may post from wantsTime(): asked without the lock
            const machine::Machine* m = session_->machineSession().machine();
            if (m && wantsTime()) {   // emulated time, no wall clock
                host_->advance(sessions::OperationContext::runPiece(m->descriptor().tickRate));
                continue;
            }
            std::unique_lock<std::mutex> lock(inbox->mutex);
            inbox->ready.wait(lock, [&] { return !inbox->items.empty(); });
            item = std::move(inbox->items.front());
            inbox->items.pop_front();
        }
        if (item->kind == Inbox::Item::Kind::End) break;
        if (item->kind == Inbox::Item::Kind::Task) {
            item->task();
            if (inbox->quit) break;
            continue;
        }
        Result r = line(item->line);
        if (r.quit) break;
        if (!r.text.empty()) write(r.text);
    }
    for (ServeAddOn* a : addOns) a->stop();
    finish();   // at the end of the input: as after quit
    // Standard input may still be open: its reader is left to the end of the program.
    if (*readerDone) reader.join();
    else reader.detach();
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
    const std::string want = machine::lowerAscii(name);
    for (const auto& p : products)
        if (machine::lowerAscii(p->name()) == want || machine::lowerAscii(p->shortName()) == want) return p.get();
    return nullptr;
}

} // namespace calcu::harness
