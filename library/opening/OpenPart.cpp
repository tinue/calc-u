#include "opening/OpenPart.hpp"
#include "files/FileCache.hpp"
#include "files/Files.hpp"
#include "media/MediaPart.hpp"
#include "opening/Classify.hpp"
#include "presets/PresetsPart.hpp"
#include "sessions/Args.hpp"
#include "sessions/LibraryTopics.hpp"
#include "shell/Inputs.hpp"
#include "shell/SettingsPart.hpp"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <memory>
#include <system_error>
#include <utility>

namespace calcu::opening {

using sessions::CommandOutcome;
using sessions::Done;
using sessions::Refused;
using sessions::SessionContext;
using sessions::StartOperation;
using sessions::Value;

namespace {

constexpr std::string_view kAsks = "asks: ";   // media's question, not a refusal

bool asks(const std::string& reason) { return reason.rfind(kAsks, 0) == 0; }

// The operation an opener started, its result the open reply.
sessions::Task<sessions::OperationResult> withReply(sessions::OperationContext& c, sessions::Operation inner,
                                                    std::shared_ptr<const OpenReply> r) {
    sessions::OperationResult res = co_await inner(c);
    if (res.ok) res.result = std::move(r);
    co_return res;
}

// A refusal is a notice that names the file and says why; media's
// question (replace the medium of that name?) is no notice.
CommandOutcome finish(CommandOutcome o, OpenReply reply, SessionContext& c) {
    auto r = std::make_shared<const OpenReply>(std::move(reply));
    if (std::holds_alternative<Done>(o)) return Done{std::move(r)};
    if (auto* x = std::get_if<Refused>(&o)) {
        if (!asks(x->reason)) {
            c.notice(x->reason);
            c.appLog().warn("A file was not opened: " + x->reason);
        }
        return o;
    }
    if (auto* s = std::get_if<StartOperation>(&o)) {
        sessions::Operation inner = std::move(s->op);
        return StartOperation{[inner, r](sessions::OperationContext& oc) { return withReply(oc, inner, r); }};
    }
    return o;   // Deferred: the opener replies
}

CommandOutcome refuse(SessionContext& c, std::string sentence) {
    c.notice(sentence);
    c.appLog().warn("A file was not opened: " + sentence);
    return Refused{std::move(sentence)};
}

std::string joinedOr(const std::vector<std::string>& list) {
    std::string out;
    for (std::size_t i = 0; i < list.size(); ++i) {
        if (i > 0) out += i + 1 == list.size() ? " or " : ", ";
        out += list[i];
    }
    return out;
}


std::string text(const OpenedFile& f) { return f.bytes ? std::string(f.bytes->begin(), f.bytes->end()) : std::string(); }

// `media.open-file` for the file: where it lies, or (an upload) its bytes.
Value mediumArgs(const OpenedFile& f, const std::optional<std::string>& place, bool upload, bool confirmed) {
    Value::Object o{{"name", Value(fileNameOf(f))}};
    if (f.path) o.push_back({"path", Value(*f.path)});
    if (f.bytes) o.push_back({"bytes", Value(Value::Bytes(*f.bytes))});
    if (upload) o.push_back({"upload", Value(true)});
    if (confirmed) o.push_back({"confirmed", Value(true)});
    if (place) o.push_back({"place", Value(*place)});
    return Value(std::move(o));
}

// The file's name without any folder, for a file written into a folder.
std::string plainName(const OpenedFile& f) { return files::plainFileName(fileNameOf(f)); }

} // namespace

OpenPart::OpenPart(OpenSetup s) : setup_(std::move(s)) {}

void OpenPart::attach(sessions::SessionSetup& s) {
    s.command(kOpenCommand, [this](const Value& a, SessionContext& c) -> CommandOutcome {
        std::string why;
        auto req = OpenRequest::fromValue(a, &why);
        if (!req) return Refused{why};
        if (c.busy() || !c.machine()) {   // kept, the second in place of the first
            if (kept_)
                c.appLog().info("The file " + fileNameOf(kept_->files.front()) + " kept to be opened is replaced by " +
                                fileNameOf(req->files.front()) + ".");
            const std::string name = fileNameOf(req->files.front());
            kept_ = std::move(*req);
            return Done{std::make_shared<const OpenReply>(OpenedAs::Kept, name)};
        }
        return open(*req, c);
    });
    if (setup_.settings && setup_.presets)
        s.command(shell::commands::kChooseStartupPreset, [this](const Value& a, SessionContext& c) { return chooseStartup(a, c); });
    s.request(kWouldTakeRequest, [this](const Value& a, SessionContext&) {
        const Value* size = a.find("size");
        std::optional<std::uint64_t> bytes;
        if (size && size->integer() && *size->integer() >= 0) bytes = static_cast<std::uint64_t>(*size->integer());
        return sessions::replyGiven(std::make_shared<const WouldTake>(wouldTake(sessions::textArg(a, "name"), bytes, setup_)));
    });
}

void OpenPart::afterCall(SessionContext& c, pacing::WallTime) {
    if (!kept_ || c.busy() || !c.machine()) return;
    const OpenRequest req = std::move(*kept_);
    kept_.reset();
    CommandOutcome o = open(req, c);
    if (auto* r = std::get_if<Refused>(&o)) {   // replied already with Kept: the refusal is a notice
        if (asks(r->reason)) c.notice(fileNameOf(req.files.front()) + " was not opened: " + r->reason.substr(kAsks.size()));
    } else if (auto* st = std::get_if<StartOperation>(&o)) {
        if (!c.startOperation(std::move(st->op))) {
            const std::string sentence = fileNameOf(req.files.front()) + " was not opened: " + sessions::kAnotherOperationRuns;
            c.notice(sentence);
            c.appLog().warn(sentence);
        }
    }
}

CommandOutcome OpenPart::open(const OpenRequest& req, SessionContext& c) {
    // Every uploaded file but the preset: a preset loaded once is not kept.
    if (req.upload) keep(req.files, presetAmong(req.files), c);
    if (req.files.size() > 1) return openSeveral(req, c);
    // An uploaded preset alone goes into an upload folder as well, so that
    // a file it names is refused with the advice to choose it with it.
    if (req.upload && setup_.uploadFolder && setup_.presets &&
        classify(req.files.front(), req.way, setup_).what == Classified::What::Preset)
        return openSeveral(req, c);
    return openOne(req.files.front(), req, c);
}

CommandOutcome OpenPart::openOne(const OpenedFile& f, const OpenRequest& req, SessionContext& c) {
    const std::string name = fileNameOf(f);
    const Classified k = classify(f, req.way, setup_);
    switch (k.what) {
    case Classified::What::Refused: return refuse(c, k.refusal);
    case Classified::What::Preset: {
        // From its folder where the host can write it; an upload, or a file
        // without a path, as its content.
        const Value args = f.path && !req.upload ? Value::object({{"ref", *f.path}})
                                                 : Value::object({{"content", text(f)}, {"name", name}});
        CommandOutcome o = setup_.presets->loadPreset(args, c);
        return finish(std::move(o), OpenReply(OpenedAs::Preset, name), c);
    }
    case Classified::What::Medium: return openMedium(f, req, c);
    case Classified::What::Kind:
        return finish(k.kind->open(f, req.way, c), OpenReply(OpenedAs::FileKind, name, std::string(k.kind->name())), c);
    }
    return refuse(c, setup_.productName + " cannot open " + name + ".");
}

// A medium: read and checked, its one place now, then used where it lies
// (the file is the medium's instance from then on, written back there unless
// it is a template) or, an upload, imported into the save folder first. A
// name already taken there and not confirmed is media's `asks:` refusal.
CommandOutcome OpenPart::openMedium(const OpenedFile& f, const OpenRequest& req, SessionContext& c) {
    media::MediaPart& m = *setup_.media;
    const std::string name = fileNameOf(f);
    const media::MediaPart::OpenedMedium medium = m.readOpened(name, text(f), f.path.value_or(std::string()));
    if (medium.refusal) return refuse(c, *medium.refusal);   // with media's hint
    const machine::Machine& mc = *c.machine();
    const std::vector<std::string> places = m.placesFor(medium, mc);
    if (places.size() > 1)
        return refuse(c, name + " fits more than one place; choose it in the picker of the place it should go into.");
    if (places.empty()) {
        const auto where = m.placesInCatalogue(medium);
        std::string sentence = "The " + mc.descriptor().model.value + " has no place for " + name + " now";
        if (!where.empty() && medium.kind) sentence += "; a " + medium.kind->label + " goes into " + joinedOr(where);
        sentence += ".";
        if (!req.upload) return refuse(c, sentence);   // not copied
        // Imported only: it waits in the save folder for its picker.
        CommandOutcome o = m.openFile(mediumArgs(f, std::nullopt, true, req.confirmed), c);
        if (std::holds_alternative<Refused>(o)) return finish(std::move(o), OpenReply(OpenedAs::Medium, name), c);
        c.notice(sentence);
        return finish(std::move(o), OpenReply(OpenedAs::Medium, name, "imported"), c);
    }
    const std::string& place = places.front();
    if (auto r = m.refusesNow(place, mc)) return refuse(c, *r);   // nothing imported
    return finish(m.openFile(mediumArgs(f, place, req.upload, req.upload && req.confirmed), c),
                  OpenReply(OpenedAs::Medium, name, place), c);
}

// Several files from a Web file dialog (or one preset) are one folder; the
// preset among them is opened by its path there, as an upload: a file it
// names is looked up there by its name. Without an upload folder only the
// first file is opened, alone.
CommandOutcome OpenPart::openSeveral(const OpenRequest& req, SessionContext& c) {
    if (!setup_.uploadFolder || !setup_.presets) return openOne(req.files.front(), req, c);
    namespace fs = std::filesystem;
    std::error_code ec;
    fs::path folder;
    do {
        folder = files::toPath(*setup_.uploadFolder) / ("open-" + std::to_string(++folders_));
    } while (fs::exists(folder, ec));
    fs::create_directories(folder, ec);
    if (ec) return refuse(c, "The files could not be put together in " + files::fromPath(folder) + ": " + ec.message());
    for (const OpenedFile& f : req.files) {
        if (!f.bytes) continue;
        errno = 0;
        std::ofstream out(folder / files::toPath(plainName(f)), std::ios::binary | std::ios::trunc);
        out.write(reinterpret_cast<const char*>(f.bytes->data()), static_cast<std::streamsize>(f.bytes->size()));
        if (!out)
            return refuse(c, plainName(f) + " could not be written to " + files::fromPath(folder) + ": " +
                                 files::osReason(errno) + ".");
    }
    const OpenedFile* first = presetAmong(req.files);
    if (!first) return openOne(req.files.front(), req, c);
    const Classified k = classify(*first, req.way, setup_);
    if (k.what == Classified::What::Refused) return refuse(c, k.refusal);   // 1b
    const std::string path = files::fromPath(folder / files::toPath(plainName(*first)));
    return finish(setup_.presets->loadPreset(Value::object({{"ref", path}, {"upload", Value(true)}}), c),
                  OpenReply(OpenedAs::Preset, plainName(*first)), c);
}

// The preset among several files: the first by name of those with a preset
// extension, else of those whose text has a model line; null: none. Each
// must still pass test 1.
const OpenedFile* OpenPart::presetAmong(const std::vector<OpenedFile>& files) const {
    std::vector<const OpenedFile*> byExtension, byContent;
    for (const OpenedFile& f : files) {
        const std::string lower = lowerAscii(plainName(f));
        if (!f.bytes || lower.empty() || lower[0] == '.') continue;
        if (std::any_of(setup_.presetExtensions.begin(), setup_.presetExtensions.end(),
                        [&](const std::string& e) { return endsWith(lower, lowerAscii(e)); }))
            byExtension.push_back(&f);
        else if (looksLikePreset(*f.bytes))
            byContent.push_back(&f);
    }
    auto& list = byExtension.empty() ? byContent : byExtension;
    if (list.empty()) return nullptr;
    return *std::min_element(list.begin(), list.end(), [](const OpenedFile* a, const OpenedFile* b) {
        return lowerAscii(plainName(*a)) < lowerAscii(plainName(*b));
    });
}

// Into the file cache, flat by name; a file that cannot be kept is opened
// all the same (the app log says why it is not kept).
void OpenPart::keep(const std::vector<OpenedFile>& files, const OpenedFile* except, SessionContext& c) const {
    if (!setup_.cacheFolder) return;
    for (const OpenedFile& f : files) {
        if (&f == except || !f.bytes) continue;
        std::string why;
        if (!files::keepInCache(files::toPath(*setup_.cacheFolder), plainName(f), f.bytes->data(), f.bytes->size(), &why))
            c.appLog().warn("The file " + plainName(f) + " was not kept in the file cache: " + why + ".");
    }
}

CommandOutcome OpenPart::chooseStartup(const Value& a, SessionContext& c) {
    const std::string model = sessions::textArg(a, "model");
    settings::SettingsFile& f = *setup_.settings;
    if (!f.product().find("preset", model)) return Refused{"There is no startup preset for " + model + "."};
    // A choice that does not pass: the refusal, its notice and its app log line.
    auto notChosen = [&](std::string sentence) -> CommandOutcome {
        c.notice(sentence);
        c.appLog().warn("Settings: [preset] " + model + " not chosen: " + sentence);
        return Refused{std::move(sentence)};
    };
    std::string ref = sessions::textArg(a, "ref");
    std::string name = files::plainFileName(ref);
    if (const Value* list = a.find("files"); list && list->elements()) {
        if (!setup_.cacheFolder) return Refused{"choose-startup-preset takes files only where there is a file cache."};
        std::string why;
        auto req = OpenRequest::fromValue(Value::object({{"files", *list}, {"way", "dialog"}, {"upload", true}}), &why);
        if (!req) return Refused{why};
        const OpenedFile* preset = presetAmong(req->files);
        if (!preset) return notChosen("None of the chosen files is a preset.");
        name = plainName(*preset);
        keep(req->files, preset, c);
        const auto kept = files::keepInCache(files::toPath(*setup_.cacheFolder), name, preset->bytes->data(),
                                             preset->bytes->size(), &why);
        if (!kept) return notChosen("The preset " + name + " could not be kept in the file cache: " + why + ".");
        ref = files::fromPath(*kept);
    }
    if (ref.empty()) return Refused{"choose-startup-preset needs a ref or files."};
    if (auto why = setup_.presets->checkStartup(ref, machine::ModelId{model}))
        return notChosen("The preset " + name + " cannot be the startup preset of the " + model + ": " + *why);
    const settings::SettingsFile::WriteResult w = f.set("preset", model, ref);
    if (!w.ok) shell::tellNotSaved(c, "[preset] " + model, w);   // kept for this run, as set-setting keeps a value
    return Done{};
}

} // namespace calcu::opening
