#include "media/MediaPart.hpp"
#include "files/Files.hpp"
#include "media/Names.hpp"
#include "media/Writer.hpp"
#include "peripherals/Controls.hpp"
#include "sessions/Args.hpp"
#include "sessions/LibraryTopics.hpp"
#include "settings/AtomicFile.hpp"
#include <algorithm>
#include <atomic>
#include <cctype>
#include <chrono>
#include <cstdio>
#include <filesystem>

namespace calcu::media {

using machine::Machine;
using sessions::CommandOutcome;
using sessions::OperationContext;
using sessions::OperationResult;
using sessions::Refused;
using sessions::SessionContext;
using sessions::Task;
using sessions::Value;

namespace {

constexpr pacing::WallTime kAutosaveDelay = std::chrono::milliseconds(500);   // a change is written at most this late

std::string inQuotes(const std::string& s) { return "“" + s + "”"; }

bool endsWithNoCase(const std::string& s, const std::string& end) {
    if (end.size() > s.size()) return false;
    for (std::size_t i = 0; i < end.size(); ++i)
        if (std::tolower(static_cast<unsigned char>(s[s.size() - end.size() + i])) !=
            std::tolower(static_cast<unsigned char>(end[i])))
            return false;
    return true;
}

std::string labelled(const Binding& b) { return b.kind->label + " " + inQuotes(b.name); }

sessions::Done done() { return sessions::Done{}; }

// A media place word as a refusal names it.
std::string noPlace(const std::string& word) { return "There is no place " + word + " for a medium now."; }

Task<OperationResult> runChange(OperationContext& op, startup::MachineControl* control, startup::PlugChange ch) {
    co_await control->change(op, std::move(ch));
    co_return OperationResult{};
}

Task<OperationResult> runPowerCycle(OperationContext& op, MediaPart* part) {
    co_await part->powerCycle(op);
    co_return OperationResult{};
}

} // namespace

InFileDefs::InFileDefs() {
    static std::atomic<std::uint64_t> next{0};
    serial = ++next;
}

MediaPart::MediaPart(MediaSetup s)
    : shelf_(s.kinds, s.paths, std::move(s.files)), listing_(std::move(s.listing)), catalogue_(s.catalogue), control_(s.control) {
    startup::MachineControl* c = control_;
    if (!c) return;
    // A variant switch carries a module medium's content over.
    c->addCarry([this](machine::PeripheralRequest& r, const machine::PlugEntry& e, Machine& m) {
        for (const Place& p : places_) {
            if (!p.slot || p.path != e.place) continue;
            const auto it = bindings_.find(p.word);
            ModuleMedium* mm = moduleCap(m, p);
            if (it == bindings_.end() || !mm) return;
            r.content = mm->content();
            if (const auto* ctl = peripherals::controlsCapability(m, machine::PeripheralOwner{e.peripheral, e.place}))
                for (const auto& info : ctl->list())
                    if (info.form == machine::ControlForm::Switch)
                        r.switches.push_back(machine::SwitchPosition{info.id, info.on});
            registerModule(r.content, it->second);
            return;
        }
    });
}

// ── The session part ─────────────────────────────────────────────────────

void MediaPart::attach(sessions::SessionSetup& s) {
    topic_ = &s.stateTopic(kMediaTopic);
    refreshShelf(&s.context());   // the lines of the first scan at the start

    s.command(kRefreshCommand, [this](const Value&, SessionContext& c) -> CommandOutcome {
        refreshShelf(&c);
        return done();
    });
    s.command(kChooseCommand, [this](const Value& a, SessionContext& c) -> CommandOutcome {
        return choose(c, sessions::textArg(a, "place"), sessions::textArg(a, "entry"));
    });
    s.command(kEjectCommand, [this](const Value& a, SessionContext& c) -> CommandOutcome {
        return choose(c, sessions::textArg(a, "place"), "");
    });
    s.command(kTurnCommand, [this](const Value& a, SessionContext& c) -> CommandOutcome {
        const std::string word = sessions::textArg(a, "place");
        sync(c);
        const Place* p = place(word);
        Machine* m = c.machine();
        MediaPlace* cap = p && m && !p->slot ? placeCap(*m, *p) : nullptr;
        if (!cap) return Refused{noPlace(word)};
        if (p->sides.size() < 2) return Refused{"The medium in " + word + " has one side."};
        const auto it = bindings_.find(word);
        if (it == bindings_.end() || !cap->side()) return Refused{"There is no medium in " + word + "."};
        flush(c, word);
        Binding b = it->second;
        const int other = *cap->side() == p->sides[0] ? p->sides[1] : p->sides[0];
        const Content content = cap->content();
        if (auto r = insert(c, word, b, content, other)) return Refused{*r};
        return done();
    });
    s.command(kArmCommand, [this](const Value& a, SessionContext& c) -> CommandOutcome {
        const std::string word = sessions::textArg(a, "place");
        sync(c);
        const Place* p = place(word);
        if (!p || !p->passThrough) return Refused{"A medium is armed only for a pass-through place."};
        Resolved r = resolveEntry(*p, sessions::textArg(a, "entry"));
        if (r.refusal) return Refused{*r.refusal};
        if (auto e = arm(c, word, std::move(r.binding), std::move(r.content))) return Refused{*e};
        return done();
    });
    s.command(kFeedCommand, [this](const Value& a, SessionContext& c) -> CommandOutcome {
        const int side = static_cast<int>(sessions::truncatedIntegerArg(a, "side", 0));
        if (auto r = feed(c, sessions::textArg(a, "place"), side)) return Refused{*r};
        return done();
    });
    s.command(kNameAndSaveCommand, [this](const Value& a, SessionContext& c) -> CommandOutcome {
        const std::string word = sessions::textArg(a, "place");
        sync(c);
        const auto it = bindings_.find(word);
        if (!place(word) || it == bindings_.end()) return Refused{"There is no medium in " + word + "."};
        if (it->second.instance())
            return Refused{"The " + labelled(it->second) + " is saved already; Name & Save is for a medium that is not."};
        if (it->second.writeProtected) return Refused{"The " + labelled(it->second) + " is write-protected."};
        if (!it->second.kind->offersNameAndSave)
            return Refused{"A " + it->second.kind->label + " is not saved by Name & Save."};
        if (c.busy()) return Refused{sessions::kAnotherOperationRuns};
        SaveRequest r;
        r.by = SaveBy::NameAndSave;
        r.target = sessions::textArg(a, "name");
        r.replaceConfirmed = sessions::flagArg(a, "confirmed");
        const SaveResult res = save(c, word, r, sessions::flagArg(a, "protect"));
        if (!res.ok) return Refused{res.reason};
        if (res.powerCycle) return sessions::StartOperation{[this](OperationContext& op) { return runPowerCycle(op, this); }};
        return done();
    });
    s.command(kStartNewCommand, [this](const Value& a, SessionContext& c) -> CommandOutcome {
        const std::string word = sessions::textArg(a, "place");
        sync(c);
        const Place* p = place(word);
        if (!p || p->slot) return Refused{noPlace(word)};
        Resolved r = resolveEntry(*p, sessions::textArg(a, "template"));
        if (r.refusal) return Refused{*r.refusal};
        if (!r.binding.isTemplate) return Refused{"A new medium is started from a template."};
        refreshShelf(&c);
        SaveRequest q;
        q.kind = p->kind;
        q.by = SaveBy::NameAndSave;
        q.target = sessions::textArg(a, "name");
        q.replaceConfirmed = sessions::flagArg(a, "confirmed");
        q.from = word;
        const Plan plan = planSave(q, shelf_, [this](const std::string& path) { return inPlace(path); });
        if (plan.verdict != Plan::Verdict::Write) return Refused{plan.refusal};
        Binding b = r.binding;   // Name & Save done up front
        b.name = plan.name;
        b.path = plan.path;
        b.isTemplate = false;
        b.origin = Origin::Saved;
        b.started = true;
        b.unsaved = true;
        if (p->passThrough) {   // fitted, for the user to feed
            if (auto e = arm(c, word, std::move(b), std::move(r.content))) return Refused{*e};
            return done();
        }
        if (auto e = insert(c, word, std::move(b), std::move(r.content), p->sides.empty() ? 0 : p->sides[0]))
            return Refused{*e};
        return done();
    });
    s.command(kOpenFileCommand, [this](const Value& a, SessionContext& c) { return openFile(a, c); });
    s.command(kImportCommand, [this](const Value& a, SessionContext& c) -> CommandOutcome {
        const Value* paths = a.find("paths");
        if (paths && paths->elements()) {
            std::string refusals;
            for (const Value& v : *paths->elements()) {
                if (!v.string()) continue;
                const Outcome r = importFile(c, *v.string());
                if (!r.ok) refusals += (refusals.empty() ? "" : " ") + r.reason;
            }
            if (!refusals.empty()) return Refused{refusals};
            return done();
        }
        std::string accept;
        for (const KindDecl& k : kinds()) {
            accept += (accept.empty() ? "" : ",") + k.suffix;
            for (const std::string& s : k.companionSuffixes) accept += "," + s;
        }
        const auto asked = c.askHost(sessions::HostService::FilePicker,
                                     Value::object({{"accept", accept}, {"multiple", true}}),
                                     [this, &c](const sessions::ServiceAnswer& ans) {
                                         if (!ans.given) return;
                                         const auto* list = ans.result.elements();
                                         if (!list) return;
                                         // A companion (a tape's sound) picked with media is
                                         // read beside them, not imported on its own.
                                         auto companion = [this](const std::string& path) {
                                             for (const KindDecl& k : kinds())
                                                 for (const std::string& s : k.companionSuffixes)
                                                     if (endsWithNoCase(path, s)) return true;
                                             return false;
                                         };
                                         bool media = false;
                                         for (const Value& v : *list) media = media || (v.string() && !companion(*v.string()));
                                         for (const Value& v : *list) {
                                             if (!v.string() || (media && companion(*v.string()))) continue;
                                             if (const Outcome r = importFile(c, *v.string()); !r.ok) c.notice(r.reason);
                                         }
                                     });
        if (!asked) return Refused{"No file picker is offered."};
        return done();
    });
    s.command(kExportCommand, [this](const Value& a, SessionContext& c) -> CommandOutcome {
        const KindDecl* k = kind(sessions::textArg(a, "kind"));
        if (!k) return Refused{"There is no kind of medium " + sessions::textArg(a, "kind") + "."};
        std::string reason;
        const auto files = exportFiles(*k, sessions::textArg(a, "name"), &reason);
        if (!files) return Refused{reason};
        if (!c.hostOffers(sessions::HostService::KeepFile)) return Refused{"Nobody keeps a file here."};
        for (const FileOut& f : *files)   // the medium's file first
            c.askHost(sessions::HostService::KeepFile,
                      Value::object({{"name", f.relativePath},
                                     {"bytes", Value::Bytes(f.bytes.begin(), f.bytes.end())},
                                     {"mediaType", hasSuffix(f.relativePath, *k) ? "application/yaml" : "application/octet-stream"}}),
                      [](const sessions::ServiceAnswer&) {});
        return done();
    });
    s.command(kDeleteCommand, [this](const Value& a, SessionContext& c) -> CommandOutcome {
        const KindDecl* k = kind(sessions::textArg(a, "kind"));
        if (!k) return Refused{"There is no kind of medium " + sessions::textArg(a, "kind") + "."};
        const Outcome r = remove(c, *k, sessions::textArg(a, "name"), sessions::flagArg(a, "confirmed"));
        if (!r.ok) return Refused{r.reason};
        return done();
    });
}

void MediaPart::beforeCall(SessionContext&, pacing::WallTime now) {
    callStart_ = lastCall_;   // a change is dated at the start of the call that shows it
    gap_ = now - lastCall_;
    lastCall_ = now;
}

void MediaPart::afterCall(SessionContext& c, pacing::WallTime now) {
    sync(c);
    autosave(c, c.virtualTime(), now);
}

void MediaPart::beforeFlush(SessionContext& c) {
    sync(c);
    autosave(c, c.virtualTime(), lastCall_);   // in virtual time: at the end of every command
    updateTopic(c);
}

void MediaPart::beforeOperation(SessionContext& c) { flushAll(c); }

void MediaPart::beforeMachineChange(SessionContext& c) {
    flushAll(c);
    // What the media places hold, for a variant switch.
    Machine* m = c.machine();
    carried_.clear();
    if (!m) return;
    for (const Place& p : places_) {
        if (p.slot) continue;
        const auto it = bindings_.find(p.word);
        if (it == bindings_.end()) continue;
        Binding b = it->second;
        if (MediaPlace* cap = placeCap(*m, p); cap && cap->content()) {
            b.content = cap->content();
            b.side = cap->side();
        }
        carried_.emplace_back(p.word, std::move(b));
    }
}

void MediaPart::machineChanged(SessionContext& c) { sync(c); }

void MediaPart::operationEnded(SessionContext& c) {
    sync(c);
    pendingSlot_.clear();
}

void MediaPart::suspending(SessionContext& c) { flushAll(c); }
void MediaPart::closing(SessionContext& c) { flushAll(c); }   // quit

void MediaPart::refreshShelf(SessionContext* c) {
    const std::vector<std::string> lines = shelf_.reread();
    if (c)
        for (const auto& l : lines) c->log(l);
}

// ── Places and bindings ─────────────────────────────────────────────────

void MediaPart::computePlaces(const Machine& m) {
    places_.clear();
    for (const auto& e : m.capabilities(MediaPlace::kKind)) {
        const MediaPlace* cap = machine::capabilityAs<MediaPlace>(m, e.id);
        if (!cap) continue;
        Place p;
        p.id = cap->id();
        const std::vector<machine::MediaPlaceDescriptor>* list = &m.descriptor().mediaPlaces;
        if (const auto* owner = std::get_if<machine::PeripheralOwner>(&e.owner)) {
            p.path = owner->place;
            const machine::PeripheralDescriptor* d = catalogue_ ? catalogue_->peripheral(owner->peripheral) : nullptr;
            list = d ? &d->mediaPlaces : nullptr;
        }
        if (!list) continue;
        const auto d = std::find_if(list->begin(), list->end(), [&](const machine::MediaPlaceDescriptor& x) { return x.id == p.id; });
        if (d == list->end()) continue;
        p.kind = shelf_.kind(d->mediumKind);
        if (!p.kind || p.kind->heldBy != HeldBy::MediaPlace) continue;
        p.passThrough = d->passage == machine::MediaPassage::PassThrough;
        p.sides = d->sides;
        places_.push_back(std::move(p));
    }
    for (Place& p : places_) {
        const auto same = std::count_if(places_.begin(), places_.end(), [&](const Place& o) { return o.kind == p.kind; });
        p.word = same == 1 && !p.kind->words.ejectWord.empty() ? p.kind->words.ejectWord : p.id.value;
    }
    if (!catalogue_) return;
    for (const auto& pl : m.descriptor().places) {
        const auto info = catalogue_->place(m.model(), pl.name);
        if (!info) continue;
        for (const KindDecl& k : kinds()) {
            if (k.heldBy != HeldBy::ModuleSlot || !k.isMedium) continue;
            const bool any = std::any_of(info->fitting.begin(), info->fitting.end(), [&](const machine::PeripheralId& id) {
                const machine::PeripheralDescriptor* d = catalogue_->peripheral(id);
                return d && k.isMedium(*d);
            });
            if (!any) continue;
            Place p;
            p.word = pl.name.value;
            p.kind = &k;
            p.slot = true;
            p.path = machine::PlacePath{{pl.name}};
            places_.push_back(std::move(p));
            break;
        }
    }
    // A module slot that no kind of medium fits (Calc-U-59's slot-1).
    for (const machine::PlaceName& name : listing_.moduleSlots) {
        if (!m.descriptor().place(name) || place(name.value)) continue;
        Place p;
        p.word = name.value;
        p.slot = true;
        p.path = machine::PlacePath{{name}};
        places_.push_back(std::move(p));
    }
}

const MediaPart::Place* MediaPart::place(const std::string& word) const {
    for (const Place& p : places_)
        if (p.word == word) return &p;
    return nullptr;
}

template <class M> MediaPart::PlaceCap<M>* MediaPart::placeCap(M& m, const Place& p) {
    if (p.slot) return nullptr;
    for (const auto& e : m.capabilities(MediaPlace::kKind)) {
        const auto* owner = std::get_if<machine::PeripheralOwner>(&e.owner);
        if ((owner ? owner->place : machine::PlacePath{}) != p.path) continue;
        if (auto* cap = machine::capabilityAs<MediaPlace>(m, e.id); cap && cap->id() == p.id) return cap;
    }
    return nullptr;
}

ModuleMedium* MediaPart::moduleCap(Machine& m, const Place& p) const {
    if (!p.slot) return nullptr;
    for (const auto& e : m.capabilities(ModuleMedium::kKind)) {
        const auto* owner = std::get_if<machine::PeripheralOwner>(&e.owner);
        if (owner && owner->place == p.path) return machine::capabilityAs<ModuleMedium>(m, e.id);
    }
    return nullptr;
}

std::optional<machine::PeripheralId> MediaPart::pluggedAt(const Machine& m, const Place& p) const {
    for (const auto& e : m.plugged().byPlace)
        if (e.place == p.path) return e.peripheral;
    return std::nullopt;
}

std::uint64_t MediaPart::revisionOf(Machine& m, const Place& p) const {
    if (p.slot) {
        ModuleMedium* mm = moduleCap(m, p);
        return mm ? mm->revision() : 0;
    }
    MediaPlace* cap = placeCap(m, p);
    return cap ? cap->revision() : 0;
}

Content MediaPart::contentOf(Machine& m, const Place& p, const Binding& b) const {
    if (p.slot) {
        ModuleMedium* mm = moduleCap(m, p);
        return mm ? mm->content() : b.content;
    }
    if (MediaPlace* cap = placeCap(m, p); cap && cap->content()) return cap->content();
    return b.content;
}

std::optional<std::string> MediaPart::inPlace(const std::string& path) const {
    if (path.empty()) return std::nullopt;
    for (const auto& [word, b] : bindings_)
        if (b.path == path) return word;
    return std::nullopt;
}

Binding MediaPart::bindingFor(const ShelfEntry& e) const {
    Binding b;
    b.kind = e.kind;
    b.name = e.name;
    b.description = e.description;
    b.origin = e.origin;
    b.isTemplate = e.isTemplate;
    b.path = e.path;
    if (!e.bytes.empty()) b.source = e.bytes;
    b.hasWriteProtect = e.hasWriteProtect;
    b.writeProtected = e.writeProtected;
    return b;
}

void MediaPart::registerModule(const Content& c, Binding b) {
    if (!c) return;
    registry_.erase(std::remove_if(registry_.begin(), registry_.end(), [](const auto& x) { return x.first.expired(); }),
                    registry_.end());
    registry_.emplace_back(c, std::move(b));
}

void MediaPart::newMachine(SessionContext& c) {
    const std::uint64_t gen = c.machineSession().generation();
    if (gen == generation_) return;
    generation_ = gen;
    const auto origin = control_ && control_->lastBuild() ? std::optional(control_->lastBuild()->origin) : std::nullopt;
    // A fitted medium that never passed through is logged, unless a variant
    // switch carries it over. `places_` are still the old machine's.
    if (!origin || *origin != startup::BuildOrigin::VariantSwitch)
        for (const auto& [word, b] : bindings_)
            if (const Place* p = place(word); p && p->passThrough && !b.passed)
                c.log("The " + labelled(b) + " was armed and never fed.");
    passed_.clear();
    // In-file media go at a load and at a model switch.
    if (origin && *origin != startup::BuildOrigin::VariantSwitch) {
        inFile_.clear();
        inFileSerial_ = 0;
    }
    bindings_.clear();   // the new machine holds nothing yet
    fromPlan_ = true;
    if (Machine* m = c.machine()) computePlaces(*m);
    // A variant switch puts the media back.
    if (origin && *origin == startup::BuildOrigin::VariantSwitch) {
        for (auto& [word, b] : carried_) {
            const Place* p = place(word);
            Machine* m = c.machine();
            MediaPlace* cap = p && m ? placeCap(*m, *p) : nullptr;
            if (!cap) continue;
            if (b.content && !p->passThrough) {
                if (!cap->insert(b.content, b.side.value_or(0)).ok) continue;
                b.written = cap->revision();
            }
            bindings_[word] = b;
        }
    }
    carried_.clear();
}

void MediaPart::sync(SessionContext& c) {
    Machine* m = c.machine();
    if (!m) {
        places_.clear();
        return;
    }
    newMachine(c);
    computePlaces(*m);
    bindSlots(c);
    for (auto it = bindings_.begin(); it != bindings_.end();)   // a place that went (a peripheral detached)
        it = place(it->first) ? std::next(it) : bindings_.erase(it);
}

void MediaPart::bindSlots(SessionContext& c) {
    Machine& m = *c.machine();
    for (const Place& p : places_) {
        if (!p.slot) continue;
        const auto plugged = pluggedAt(m, p);
        const auto it = bindings_.find(p.word);
        if (it != bindings_.end() && (!plugged || plugged->value != it->second.peripheral)) bindings_.erase(it);
        if (const auto pend = pendingSlot_.find(p.word); pend != pendingSlot_.end() && plugged &&
                                                          plugged->value == pend->second.peripheral) {
            Binding b = pend->second;
            b.written = revisionOf(m, p);
            bindings_[p.word] = std::move(b);
            pendingSlot_.erase(pend);
            continue;
        }
        if (!plugged || bindings_.count(p.word) || !moduleCap(m, p)) continue;
        std::optional<Binding> found;
        if (fromPlan_ && control_ && control_->lastBuild())
            for (const auto& r : control_->lastBuild()->plan.peripherals) {
                if (r.place != p.path || !r.content) continue;
                for (const auto& [w, b] : registry_)
                    if (w.lock() == r.content) found = b;
            }
        if (!found) {   // a module of the catalogue: a bundled template of the kind
            Binding b;
            b.kind = p.kind;
            const machine::PeripheralDescriptor* d = catalogue_ ? catalogue_->peripheral(*plugged) : nullptr;
            b.name = d && !d->name.empty() ? d->name : plugged->value;
            b.origin = Origin::Bundled;
            b.isTemplate = true;
            found = b;
        }
        found->peripheral = plugged->value;
        found->written = revisionOf(m, p);
        if (const auto* ctl = peripherals::controlsCapability(m, machine::PeripheralOwner{*plugged, p.path})) {
            for (const auto& info : ctl->list())
                if (info.id == kWriteProtect) {
                    found->hasWriteProtect = true;
                    found->writeProtected = found->writeProtected || info.on;
                }
        }
        bindings_[p.word] = std::move(*found);
    }
    fromPlan_ = false;
}

// ── Autosave ────────────────────────────────────────────────────────────

bool MediaPart::write(SessionContext& c, const Place*, Binding& b, Content content, std::uint64_t revision) {
    if (!b.instance()) return false;   // a template is never written
    MediumCodec::Write w;
    w.name = b.name;
    w.fileStem = stemOf(baseName(b.path), *b.kind);
    w.asTemplate = false;
    if (b.hasWriteProtect) w.writeProtected = b.writeProtected;
    w.content = std::move(content);
    w.source = b.source;
    w.savedAt = isoTime(c.hostTime());
    WriteResult r;
    if (b.kind->rereadBeforeWrite && !b.started) {   // what the file holds now
        const FileAccess::Read now = shelf_.files().read(b.path);
        if (now.unreadable) r.problem = "the file cannot be read.";
        else w.source = now.bytes;
    }
    if (r.problem.empty()) r = writeMedium(*b.kind, w, b.path, shelf_);
    b.written = revision;
    b.pending = false;
    if (!r.ok) {   // one notice until a write succeeds
        if (!b.failed) c.notice("The " + labelled(b) + " could not be saved: " + r.problem);
        b.failed = true;
        return false;
    }
    b.failed = false;
    b.unsaved = false;
    b.started = false;
    b.source = r.bytes;
    c.log("Saved " + labelled(b) + ".");   // kind and name tag, never the path
    refreshShelf(&c);
    return true;
}

void MediaPart::changed(SessionContext& c, const Place& p, Binding& b, std::uint64_t revision, Content content) {
    if (!b.instance()) {   // a template: what the machine wrote lives in the place
        b.unsaved = true;
        b.written = revision;
        return;
    }
    write(c, &p, b, std::move(content), revision);
}

void MediaPart::givenBack(SessionContext& c, const Place& p, Binding& b) {
    Machine* m = c.machine();
    MediaPlace* cap = m ? placeCap(*m, p) : nullptr;
    if (!cap) return;
    auto g = cap->takeGivenBack();
    if (!g) return;
    passed_[p.word] = true;
    b.passed = true;
    if (g->content) b.content = g->content;   // the fitted medium as it came back: the next feed inserts it
    if (!g->written) return;
    if (!b.inFile.empty())   // writes stay in the store
        for (InFileItem& i : inFile_)
            if (i.kind == b.kind->id && i.name == b.inFile) i.content = b.content;
    changed(c, p, b, cap->revision(), b.content);   // given back: written at once
}

void MediaPart::autosave(SessionContext& c, bool atOnce, pacing::WallTime now) {
    Machine* m = c.machine();
    if (!m) return;
    for (const Place& p : places_) {
        if (p.passThrough) {
            const auto it = bindings_.find(p.word);
            if (it != bindings_.end()) givenBack(c, p, it->second);
            else if (MediaPlace* cap = placeCap(*m, p)) {
                if (cap->takeGivenBack()) passed_[p.word] = true;
            }
            continue;
        }
        const auto it = bindings_.find(p.word);
        if (it == bindings_.end()) continue;
        Binding& b = it->second;
        const std::uint64_t rev = revisionOf(*m, p);
        MediaPlace* cap = placeCap(*m, p);
        const bool now4 = cap && cap->takeWriteNow();   // written at once (a tape whose motor stopped)
        if (rev == b.written && !now4) continue;
        if (!b.instance()) {
            changed(c, p, b, rev, nullptr);
            continue;
        }
        if (cap && cap->writesWhenAsked() && !atOnce && !now4) continue;   // only at its moments (a tape)
        if (!b.pending) {   // dated at the start of the call that showed it
            b.pending = true;
            b.dated = callStart_;
        }
        if (atOnce || now4 || now + gap_ >= b.dated + kAutosaveDelay) write(c, &p, b, contentOf(*m, p, b), rev);
    }
}

void MediaPart::flush(SessionContext& c, const std::string& word) {
    sync(c);
    Machine* m = c.machine();
    const Place* p = place(word);
    const auto it = bindings_.find(word);
    if (!m || !p || it == bindings_.end()) return;
    Binding& b = it->second;
    if (p->passThrough) {
        givenBack(c, *p, b);
        return;
    }
    const std::uint64_t rev = revisionOf(*m, *p);
    if (rev != b.written) changed(c, *p, b, rev, contentOf(*m, *p, b));
}

void MediaPart::flushAll(SessionContext& c) {   // the writes that do not wait
    sync(c);
    std::vector<std::string> words;
    for (const auto& [w, b] : bindings_) words.push_back(w);
    for (const auto& w : words) flush(c, w);
}

// ── Inserting, ejecting, arming ─────────────────────────────────────────

std::optional<std::string> MediaPart::placeOfKind(SessionContext& c, const KindDecl& k) {
    sync(c);
    for (const Place& p : places_)
        if (p.kind == &k || (p.kind && p.kind->id == k.id)) return p.word;
    return std::nullopt;
}

bool MediaPart::hasPlace(SessionContext& c, const std::string& word) {
    sync(c);
    return place(word) != nullptr;
}

const Binding* MediaPart::bindingOf(const std::string& word) const {
    const auto it = bindings_.find(word);
    return it == bindings_.end() ? nullptr : &it->second;
}

std::optional<std::string> MediaPart::insert(SessionContext& c, const std::string& word, Binding b, Content content, int side) {
    flush(c, word);   // the old medium written first
    const Place* p = place(word);
    Machine* m = c.machine();
    if (!p || !m) return noPlace(word);
    if (p->slot) return "A module is changed in its slot with the machine control.";
    MediaPlace* cap = placeCap(*m, *p);
    if (!cap) return noPlace(word);
    if (!p->sides.empty() && std::find(p->sides.begin(), p->sides.end(), side) == p->sides.end())
        return "The " + p->kind->label + " has no side " + std::to_string(side) + ".";
    if (p->passThrough) {   // fitted, then fed (a `card:` step)
        if (auto e = arm(c, word, std::move(b), std::move(content))) return e;
        return feed(c, word, side);
    }
    if (cap->side()) cap->eject();
    bindings_.erase(word);
    passed_.erase(word);
    const MediaPlace::Inserted r = cap->insert(content, side);
    if (!r.ok) return r.refusal.empty() ? "The " + labelled(b) + " was not taken." : r.refusal;
    b.side = side;
    b.content = std::move(content);
    b.written = cap->revision();
    b.pending = false;
    bindings_[word] = std::move(b);
    return std::nullopt;
}

void MediaPart::eject(SessionContext& c, const std::string& word) {
    flush(c, word);   // written first
    const Place* p = place(word);
    Machine* m = c.machine();
    if (!p || !m) return;
    MediaPlace* cap = placeCap(*m, *p);
    if (cap && cap->side()) {
        cap->eject();
        if (p->passThrough) return;   // out of the reader, still fitted
    }
    bindings_.erase(word);   // an empty place stays empty
}

std::optional<std::string> MediaPart::passingNow(const Machine& m, const Place& p) const {
    if (const MediaPlace* cap = placeCap(m, p); cap && p.passThrough && cap->side())
        return "A " + p.kind->label + " is passing through " + p.word + " now.";
    return std::nullopt;
}

std::optional<std::string> MediaPart::arm(SessionContext& c, const std::string& word, Binding b, Content content) {
    flush(c, word);   // what the medium fitted before gave back
    const Place* p = place(word);
    Machine* m = c.machine();
    if (!p || !p->passThrough) return "A medium is armed only for a pass-through place.";
    MediaPlace* cap = m ? placeCap(*m, *p) : nullptr;
    if (!cap) return noPlace(word);
    if (auto why = passingNow(*m, *p)) return why;
    b.content = std::move(content);
    b.side.reset();
    b.passed = false;
    b.armed = ++armSerial_;
    b.pending = false;
    b.written = cap->revision();
    passed_.erase(word);
    bindings_[word] = std::move(b);   // in the place of the one fitted before
    return std::nullopt;
}

std::optional<std::string> MediaPart::feed(SessionContext& c, const std::string& word, int side) {
    flush(c, word);   // what the last pass gave back first
    const Place* p = place(word);
    Machine* m = c.machine();
    if (!p || !p->passThrough) return "A medium is fed only into a pass-through place.";
    MediaPlace* cap = m ? placeCap(*m, *p) : nullptr;
    if (!cap) return noPlace(word);
    const auto it = bindings_.find(word);
    if (it == bindings_.end()) return "Nothing is fitted for " + word + ".";
    if (auto why = passingNow(*m, *p)) return why;
    if (!p->sides.empty() && std::find(p->sides.begin(), p->sides.end(), side) == p->sides.end())
        return "The " + p->kind->label + " has no side " + std::to_string(side) + ".";
    Binding& b = it->second;
    passed_.erase(word);
    const MediaPlace::Inserted r = cap->insert(b.content, side);
    if (!r.ok) return r.refusal.empty() ? "The " + labelled(b) + " was not taken." : r.refusal;
    b.side = side;
    b.written = cap->revision();
    b.pending = false;
    return std::nullopt;   // it stays fitted
}

bool MediaPart::takePassed(SessionContext& c, const std::string& word) {
    flush(c, word);
    if (!bindings_.count(word)) autosave(c, true, lastCall_);
    const auto it = passed_.find(word);
    if (it == passed_.end()) return false;
    passed_.erase(it);
    return true;
}

bool MediaPart::passing(SessionContext& c, const std::string& word) {
    sync(c);
    const Place* p = place(word);
    Machine* m = c.machine();
    MediaPlace* cap = p && m ? placeCap(*m, *p) : nullptr;
    return cap && cap->side().has_value();
}

// ── Saving ──────────────────────────────────────────────────────────────

MediaPart::SaveResult MediaPart::save(SessionContext& c, const std::string& word, SaveRequest r, bool protect) {
    SaveResult out;
    flush(c, word);
    const Place* p = place(word);
    const auto it = bindings_.find(word);
    Machine* m = c.machine();
    if (!p || !m || it == bindings_.end()) {
        out.reason = "There is no medium in " + word + ".";
        return out;
    }
    Binding& b = it->second;
    if (protect && !b.hasWriteProtect) {
        out.reason = "The " + b.kind->label + " has no write-protect switch.";
        return out;
    }
    refreshShelf(&c);
    r.kind = b.kind;
    r.from = word;
    const Plan plan = planSave(r, shelf_, [this](const std::string& path) { return inPlace(path); });
    if (plan.verdict != Plan::Verdict::Write) {
        out.reason = plan.refusal;
        return out;
    }
    MediumCodec::Write w;
    w.name = plan.name;
    w.fileStem = stemOf(baseName(plan.path), *b.kind);
    w.asTemplate = r.by == SaveBy::SaveasTemplate;
    if (protect) w.writeProtected = true;
    else if (b.hasWriteProtect) w.writeProtected = b.writeProtected;
    w.content = contentOf(*m, *p, b);
    w.source = b.source;
    w.savedAt = isoTime(c.hostTime());
    const WriteResult res = writeMedium(*b.kind, w, plan.path, shelf_);
    if (!res.ok) {
        out.reason = "The " + b.kind->label + " " + inQuotes(plan.name) + " could not be saved: " + res.problem;
        return out;
    }
    const std::string oldInFile = b.inFile;
    b.name = plan.name;
    b.path = plan.path;
    b.origin = Origin::Saved;
    b.isTemplate = r.by == SaveBy::SaveasTemplate;
    b.unsaved = false;
    b.started = false;
    b.pending = false;
    b.failed = false;
    b.source = res.bytes;
    b.written = revisionOf(*m, *p);
    if (!b.isTemplate && !oldInFile.empty()) {   // that instance from then on
        for (InFileItem& i : inFile_)
            if (i.kind == b.kind->id && i.name == oldInFile) {
                i.instance = plan.path;
                i.instanceName = plan.name;
            }
        b.inFile.clear();
    }
    c.log("Saved " + labelled(b) + ".");
    if (protect) {
        b.writeProtected = true;
        if (p->slot) {
            if (const auto plugged = pluggedAt(*m, *p))
                if (auto* ctl = peripherals::controlsCapability(*m, machine::PeripheralOwner{*plugged, p->path}))
                    ctl->set(kWriteProtect, true);
        } else if (MediaPlace* cap = placeCap(*m, *p)) {
            cap->setWriteProtected();
        }
        out.powerCycle = true;
    }
    refreshShelf(&c);
    out.ok = true;
    return out;
}

MediaPart::SaveResult MediaPart::saveInFile(SessionContext& c, const KindDecl& k, const std::string& name, SaveRequest r,
                                            bool protect) {
    SaveResult out;
    flushAll(c);
    if (protect) {
        out.reason = "The " + k.label + " has no write-protect switch.";
        return out;
    }
    refreshShelf(&c);
    r.kind = &k;
    const Plan plan = planSave(r, shelf_, [this](const std::string& path) { return inPlace(path); });
    if (plan.verdict != Plan::Verdict::Write) {
        out.reason = plan.refusal;
        return out;
    }
    MediumCodec::Write w;
    w.name = plan.name;
    w.fileStem = stemOf(baseName(plan.path), k);
    w.asTemplate = r.by == SaveBy::SaveasTemplate;
    w.content = inFileContent(k, name);
    w.savedAt = isoTime(c.hostTime());
    const WriteResult res = writeMedium(k, w, plan.path, shelf_);
    if (!res.ok) {
        out.reason = "The " + k.label + " " + inQuotes(plan.name) + " could not be saved: " + res.problem;
        return out;
    }
    c.log("Saved " + k.label + " " + inQuotes(plan.name) + ".");
    if (r.by != SaveBy::SaveasTemplate) {   // `as: live` binds it
        for (InFileItem& i : inFile_)
            if (i.kind == k.id && i.name == name) {
                i.instance = plan.path;
                i.instanceName = plan.name;
            }
        for (auto& [word, b] : bindings_)
            if (b.kind->id == k.id && b.inFile == name) {
                b.inFile.clear();
                b.isTemplate = false;
                b.origin = Origin::Saved;
                b.path = plan.path;
                b.name = plan.name;
                b.source = res.bytes;
                b.unsaved = false;
            }
    }
    refreshShelf(&c);
    out.ok = true;
    return out;
}

Task<bool> MediaPart::powerCycle(OperationContext& op) {
    Machine* m = op.session().machine();
    if (!m) co_return false;
    op.session().machineSession().dropInput();
    m->powerCycle();
    if (control_) co_return co_await control_->untilKeyReady(op);
    co_return true;
}

// ── In-file media ───────────────────────────────────────────────────────

void MediaPart::adoptInFile(SessionContext& c, const InFileDefs& defs) {
    sync(c);
    if (defs.serial == inFileSerial_) return;
    inFileSerial_ = defs.serial;
    inFile_.clear();
    for (const InFileDef& d : defs.items) inFile_.push_back(InFileItem{d.kind, d.name, d.content, std::nullopt, {}});
}

Content MediaPart::inFileContent(const KindDecl& k, const std::string& name) {
    for (const InFileItem& i : inFile_)
        if (i.kind == k.id && i.name == name) return i.content;
    Content fresh = !k.implicitTemplates.empty() && k.implicitTemplates.front().make ? k.implicitTemplates.front().make() : nullptr;
    inFile_.push_back(InFileItem{k.id, name, fresh, std::nullopt, {}});   // a fresh in-file medium
    return fresh;
}

std::optional<std::string> MediaPart::inFileInstance(const KindDecl& k, const std::string& name) const {
    for (const InFileItem& i : inFile_)
        if (i.kind == k.id && i.name == name && i.instance) return i.instance;
    return std::nullopt;
}

// ── Import, export, delete ──────────────────────────────────────────────

MediaPart::Outcome MediaPart::importBytes(SessionContext& c, const std::string& fileName, const std::string& bytes,
                                          const MediumCodec::Companion& companion, bool replaceConfirmed) {
    Outcome out;
    refreshShelf(&c);
    ImportPlan ip = planImport(fileName, bytes, companion, shelf_);
    if (ip.plan.verdict != Plan::Verdict::Write) {
        // An upload of a medium whose name a saved one has: replaced when confirmed.
        const KindDecl* k = ip.kind;
        const ShelfEntry* same = k && ip.info.content ? shelf_.find(*k, nameOf(ip.info, fileName, *k)) : nullptr;
        if (!replaceConfirmed || !same || same->origin != Origin::Saved) {
            out.reason = ip.plan.refusal;
            return out;
        }
        ip.plan.verdict = Plan::Verdict::Write;
        ip.plan.path = same->path;
        ip.plan.name = same->name;
    }
    std::vector<FileOut> files{FileOut{baseName(ip.plan.path), bytes}};
    for (const std::string& rel : ip.info.companions)
        if (const auto b = companion ? companion(rel) : std::nullopt) files.push_back(FileOut{rel, *b});
    const WriteResult w = writeFiles(*ip.kind, files, ip.plan.path, shelf_);
    if (!w.ok) {
        out.reason = "The " + ip.kind->label + " " + inQuotes(ip.plan.name) + " could not be imported: " + w.problem;
        return out;
    }
    c.log("Imported " + ip.kind->label + " " + inQuotes(ip.plan.name) + ".");
    refreshShelf(&c);
    out.ok = true;
    out.path = ip.plan.path;
    return out;
}

MediaPart::Outcome MediaPart::importFile(SessionContext& c, const std::string& path) {
    const auto bytes = shelf_.files().read(path).bytes;
    if (!bytes) return Outcome{false, "The file " + baseName(path) + " cannot be read.", {}};
    return importBytes(c, baseName(path), *bytes, companionsBeside(path));
}

std::optional<std::vector<FileOut>> MediaPart::exportFiles(const KindDecl& k, const std::string& name, std::string* reason) {
    shelf_.reread();
    const ShelfEntry* e = shelf_.find(k, name);
    if (!e || e->path.empty()) {
        if (reason) *reason = "The " + k.label + " " + inQuotes(name) + " is no longer there.";
        return std::nullopt;
    }
    std::vector<FileOut> out{FileOut{e->fileName, e->bytes}};
    for (const std::string& rel : e->info.companions)
        if (const auto b = settings::readWholeFile(joinFile(folderOfPath(e->path), rel))) out.push_back(FileOut{rel, *b});
    return out;
}

MediaPart::Outcome MediaPart::remove(SessionContext& c, const KindDecl& k, const std::string& name, bool confirmed) {
    Outcome out;
    refreshShelf(&c);   // read again first
    const Plan plan = planDelete(k, name, confirmed, shelf_, [this](const std::string& path) { return inPlace(path); });
    if (plan.verdict != Plan::Verdict::Write) {
        out.reason = plan.refusal;
        return out;
    }
    std::vector<std::string> companions;
    if (const ShelfEntry* e = shelf_.byPath(plan.path)) companions = e->info.companions;
    std::error_code ec;
    namespace fs = std::filesystem;
    fs::remove(files::toPath(plan.path), ec);
    if (ec) {
        out.reason = "The " + k.label + " " + inQuotes(plan.name) + " could not be deleted: " + ec.message();
        return out;
    }
    for (const auto& rel : companions) fs::remove(files::toPath(joinFile(folderOfPath(plan.path), rel)), ec);
    c.log("Deleted " + k.label + " " + inQuotes(plan.name) + ".");
    refreshShelf(&c);
    out.ok = true;
    return out;
}

// ── Opened files ────────────────────────────────────────────────────────

MediaPart::OpenedMedium MediaPart::readOpened(std::string_view name, const std::string& bytes,
                                              const std::string& path) const {
    OpenedMedium out;
    out.kind = shelf_.kindOfFile(baseName(name));
    if (!out.kind || !out.kind->codec) {
        out.refusal = "The file " + baseName(name) + " is not a medium of this product.";
        return out;
    }
    const ReadOutcome r = out.kind->codec->read(bytes, path.empty() ? MediumCodec::Companion{} : companionsBeside(path));
    if (r.unreadable())
        out.refusal = "The " + out.kind->label + " file " + baseName(name) + " cannot be read" +
                      (r.problem.empty() ? "." : ": " + r.problem);
    out.isTemplate = r.info.isTemplate;
    out.isModule = out.kind->heldBy == HeldBy::ModuleSlot;
    return out;
}

std::vector<std::string> MediaPart::placesFor(const OpenedMedium& o, const Machine&) const {
    std::vector<std::string> out;
    for (const Place& p : places_)
        if (o.kind && p.kind && p.kind->id == o.kind->id) out.push_back(p.word);
    return out;
}

std::vector<std::string> MediaPart::placesInCatalogue(const OpenedMedium& o) const {
    std::vector<std::string> out;
    if (!catalogue_ || !o.kind) return out;
    for (const auto& m : catalogue_->models()) {
        for (const auto& mp : m.mediaPlaces)
            if (mp.mediumKind == o.kind->id) out.push_back("the " + mp.id.value + " of the " + m.model.value);
        if (o.isModule)
            for (const auto& pl : m.places) {
                const auto info = catalogue_->place(m.model, pl.name);
                if (info && std::any_of(info->fitting.begin(), info->fitting.end(), [&](const machine::PeripheralId& id) {
                        const auto* d = catalogue_->peripheral(id);
                        return d && o.kind->isMedium && o.kind->isMedium(*d);
                    }))
                    out.push_back(pl.name.value + " of the " + m.model.value);
            }
    }
    return out;
}

std::optional<std::string> MediaPart::refusesNow(const std::string& word, const Machine& cm) const {
    const Place* p = place(word);
    if (!p) return noPlace(word);
    return passingNow(cm, *p);
}

CommandOutcome MediaPart::openFile(const Value& a, SessionContext& c) {
    const std::string name = sessions::textArg(a, "name");
    const std::string path = sessions::textArg(a, "path");
    std::string bytes;
    if (const auto b = sessions::bytesArg(a, "bytes")) bytes.assign(b->begin(), b->end());
    else if (const auto f = shelf_.files().read(path).bytes) bytes = *f;
    else return Refused{"The file " + baseName(name.empty() ? path : name) + " cannot be read."};
    const std::string fileName = baseName(name.empty() ? path : name);
    const OpenedMedium o = readOpened(fileName, bytes, sessions::flagArg(a, "upload") ? std::string() : path);
    if (o.refusal) return Refused{*o.refusal};
    const KindDecl& k = *o.kind;
    Binding b;
    Content content;
    MediumInfo info;
    if (sessions::flagArg(a, "upload")) {   // a Web frontend imports it first
        refreshShelf(&c);
        const ReadOutcome r = k.codec->read(bytes, {});
        const ShelfEntry* same = shelf_.find(k, nameOf(r.info, fileName, k));
        if (same && same->origin == Origin::Saved && !sessions::flagArg(a, "confirmed"))
            return Refused{std::string(kAsks) + replaceQuestionFor(k, same->name)};
        const Outcome imp = importBytes(c, fileName, bytes, {}, sessions::flagArg(a, "confirmed"));
        if (!imp.ok) return Refused{imp.reason};
        const ShelfEntry* e = shelf_.byPath(imp.path);
        if (!e) return Refused{"The " + k.label + " was imported but cannot be read back."};
        b = bindingFor(*e);
        info = e->info;
    } else {   // used and written where it lies
        const ReadOutcome r = k.codec->read(bytes, path.empty() ? MediumCodec::Companion{} : companionsBeside(path));
        info = r.info;
        b.kind = &k;
        b.name = nameOf(r.info, fileName, k);
        b.description = r.info.description;
        b.origin = Origin::Opened;
        b.isTemplate = r.info.isTemplate;
        b.path = path;
        b.source = bytes;
        b.hasWriteProtect = r.info.hasWriteProtect;
        b.writeProtected = r.info.writeProtected;
        if (path.empty()) b.isTemplate = true;   // nowhere to write it back
    }
    content = info.content;
    const std::string word = sessions::textArg(a, "place");
    if (word.empty()) return done();   // no place: only imported
    sync(c);
    const Place* p = place(word);
    if (!p) return Refused{noPlace(word)};
    if (!p->kind || p->kind->id != k.id) return Refused{"The " + k.label + " does not go into " + word + "."};
    if (p->slot) {
        if (!k.moduleRequest) return Refused{"The " + k.label + " cannot be plugged in."};
        auto req = k.moduleRequest(info, p->path);
        if (!req) return Refused{"The " + k.label + " cannot be plugged in."};
        b.peripheral = req->peripheral.value;
        return changeSlot(c, *p, std::move(req), std::move(b));
    }
    if (p->passThrough) {   // fitted, not fed
        if (auto e = arm(c, word, std::move(b), std::move(content))) return Refused{*e};
        return done();
    }
    if (auto e = insert(c, word, std::move(b), std::move(content), p->sides.empty() ? 0 : p->sides[0])) return Refused{*e};
    return done();
}

// ── Choosing ────────────────────────────────────────────────────────────

MediaPart::Resolved MediaPart::resolveEntry(const Place& p, const std::string& entry) const {
    Resolved out;
    static const KindDecl kNone;   // a slot that no kind of medium fits
    const KindDecl& k = p.kind ? *p.kind : kNone;
    const std::size_t colon = entry.find(':');
    const std::string what = entry.substr(0, colon), rest = colon == std::string::npos ? std::string() : entry.substr(colon + 1);
    if (what == "implicit") {
        for (const ImplicitTemplate& t : k.implicitTemplates)
            if (t.name == rest) {
                out.binding.kind = &k;
                out.binding.name = t.name;
                out.binding.description = t.description;
                out.binding.origin = Origin::Implicit;
                out.binding.isTemplate = true;
                out.content = t.make ? t.make() : nullptr;
                return out;
            }
    } else if (what == "bundled" || what == "saved") {
        for (const ShelfEntry* e : shelf_.entries(k)) {
            if (e->fileName != rest || (what == "bundled") != (e->origin == Origin::Bundled)) continue;
            std::string bytes;   // read again when it is chosen
            const auto r = shelf_.readFile(k, e->path, &bytes);
            if (!r || r->status != ReadOutcome::Status::Read) {
                out.refusal = "The " + k.label + " " + inQuotes(e->name) + " cannot be read" +
                              (r && !r->problem.empty() ? ": " + r->problem : std::string("."));
                return out;
            }
            out.binding = bindingFor(*e);
            out.binding.source = bytes;
            out.content = r->info.content;
            if (p.slot) {
                out.module = k.moduleRequest ? k.moduleRequest(r->info, p.path) : std::nullopt;
                if (!out.module) out.refusal = "The " + k.label + " " + inQuotes(e->name) + " cannot be plugged in.";
                else out.binding.peripheral = out.module->peripheral.value;
            }
            return out;
        }
    } else if (what == "module" && p.slot && catalogue_) {
        const machine::PeripheralDescriptor* d = catalogue_->peripheral(machine::PeripheralId{rest});
        if (d) {
            machine::PeripheralRequest r;
            r.place = p.path;
            r.peripheral = d->id;
            r.content = d->productData;
            out.module = r;
            if (k.isMedium && k.isMedium(*d)) {
                out.binding.kind = &k;
                out.binding.name = d->name.empty() ? d->id.value : d->name;
                out.binding.origin = Origin::Bundled;
                out.binding.isTemplate = true;
                out.binding.peripheral = d->id.value;
            }
            return out;
        }
    }
    out.refusal = "There is no entry " + entry + " for " + p.word + ".";
    return out;
}

CommandOutcome MediaPart::changeSlot(SessionContext& c, const Place& p, std::optional<machine::PeripheralRequest> req,
                                     std::optional<Binding> b) {
    Machine* m = c.machine();
    if (!m) return Refused{sessions::kNoMachine};
    if (!control_) return Refused{"The modules cannot be changed here."};
    const auto plugged = pluggedAt(*m, p);
    startup::PlugChange ch;
    ch.place = p.path;
    if (!req) {
        if (!plugged) return done();   // an empty slot stays empty
        ch.kind = startup::PlugChange::Kind::Detach;
    } else {
        ch.kind = plugged ? startup::PlugChange::Kind::Replace : startup::PlugChange::Kind::Attach;
        ch.request = std::move(*req);
        ch.request.place = p.path;
    }
    if (auto refused = control_->check(*m, ch)) return Refused{refused->reason};
    if (b && ch.kind != startup::PlugChange::Kind::Detach) pendingSlot_[p.word] = *b;
    else pendingSlot_.erase(p.word);
    startup::MachineControl* control = control_;
    return sessions::StartOperation{[control, ch](OperationContext& op) { return runChange(op, control, ch); }};
}

CommandOutcome MediaPart::choose(SessionContext& c, const std::string& word, const std::string& entry) {
    sync(c);
    const Place* p = place(word);
    if (!p) return Refused{noPlace(word)};
    if (c.busy()) return Refused{sessions::kAnotherOperationRuns};
    if (Machine* m = c.machine(); m && !entry.empty())   // nothing is fitted while a medium passes
        if (auto why = passingNow(*m, *p)) return Refused{*why};
    flush(c, word);   // what the old medium has to save first
    p = place(word);
    if (!p) return Refused{noPlace(word)};
    if (entry.empty()) {
        if (p->slot) return changeSlot(c, *p, std::nullopt, std::nullopt);
        eject(c, word);
        return done();
    }
    Resolved r = resolveEntry(*p, entry);
    if (r.refusal) {
        if (r.refusal->rfind("There is no entry", 0) == 0) return Refused{*r.refusal};
        if (!p->slot) eject(c, word);   // the place is empty and a notice says why
        c.notice(*r.refusal);
        if (p->slot) return Refused{*r.refusal};
        return done();
    }
    if (p->slot) {
        std::optional<Binding> b;
        if (r.binding.kind) b = r.binding;
        return changeSlot(c, *p, std::move(r.module), std::move(b));
    }
    if (p->passThrough) {   // fitted, not fed
        if (auto e = arm(c, word, std::move(r.binding), std::move(r.content))) c.notice(*e);
        return done();
    }
    if (auto e = insert(c, word, std::move(r.binding), std::move(r.content), p->sides.empty() ? 0 : p->sides[0])) {
        eject(c, word);
        c.notice(*e);
    }
    return done();
}

// ── The topic ───────────────────────────────────────────────────────────

std::vector<PickerEntry> MediaPart::picker(SessionContext& c, const Place& p) const {
    std::vector<PickerEntry> empty, bundled, user, instances, modules;
    empty.push_back(PickerEntry{PickerEntry::Group::Empty, "", "", std::nullopt, true, std::nullopt});
    static const KindDecl kNone;   // a slot that no kind of medium fits: no shelf entries
    const KindDecl& k = p.kind ? *p.kind : kNone;
    auto tip = [](const std::string& d) { return d.empty() ? std::nullopt : std::optional<std::string>(d); };
    Machine* atMachine = c.machine();
    const machine::PlaceDescriptor* slotPlace =
        p.slot && atMachine ? atMachine->descriptor().place(p.path.steps.front()) : nullptr;
    std::vector<std::string> shelfNames;   // a catalogue module of that name is the same template
    for (const ShelfEntry* e : p.kind ? shelf_.entries(k) : std::vector<const ShelfEntry*>{}) {
        // A module slot lists the media that fit it.
        if (slotPlace && k.fitsPinOut && !k.fitsPinOut(e->info, slotPlace->pinOut)) continue;
        shelfNames.push_back(foldName(e->name));
        PickerEntry x;
        x.label = e->name;
        x.tooltip = tip(e->description);
        switch (e->origin) {
        case Origin::Bundled:
            x.group = PickerEntry::Group::BundledTemplate;
            x.id = "bundled:" + e->fileName;
            break;
        case Origin::Implicit:
            x.group = PickerEntry::Group::BundledTemplate;
            x.id = "implicit:" + e->name;
            break;
        default:
            x.group = e->isTemplate ? PickerEntry::Group::UserTemplate : PickerEntry::Group::Instance;
            x.id = "saved:" + e->fileName;
            break;
        }
        (x.group == PickerEntry::Group::BundledTemplate ? bundled : x.group == PickerEntry::Group::UserTemplate ? user : instances)
            .push_back(std::move(x));
    }
    Machine* m = c.machine();
    if (p.slot && catalogue_ && m) {
        if (const auto info = catalogue_->place(m->model(), p.path.steps.front())) {
            std::vector<const machine::PeripheralDescriptor*> others;
            for (const auto& id : info->fitting) {
                const machine::PeripheralDescriptor* d = catalogue_->peripheral(id);
                if (!d) continue;
                bool medium = false;
                for (const KindDecl& kk : kinds())
                    medium = medium || (kk.heldBy == HeldBy::ModuleSlot && kk.isMedium && kk.isMedium(*d));
                if (medium) {
                    const std::string name = foldName(d->name.empty() ? d->id.value : d->name);
                    const bool onShelf = std::find(shelfNames.begin(), shelfNames.end(), name) != shelfNames.end();
                    if (k.isMedium && k.isMedium(*d) && !onShelf)
                        bundled.push_back(PickerEntry{PickerEntry::Group::BundledTemplate, "module:" + d->id.value,
                                                      d->name.empty() ? d->id.value : d->name, std::nullopt, true, std::nullopt});
                } else {
                    others.push_back(d);
                }
            }
            // The listing's order, equal keys in the catalogue's order.
            std::stable_sort(others.begin(), others.end(), [&](const auto* a, const auto* b) {
                if (listing_.sortKey) return listing_.sortKey(*a) < listing_.sortKey(*b);
                return foldName(a->name) < foldName(b->name);
            });
            for (const auto* d : others)
                modules.push_back(PickerEntry{PickerEntry::Group::Module, "module:" + d->id.value,
                                              listing_.label ? listing_.label(*d) : (d->name.empty() ? d->id.value : d->name),
                                              listing_.tooltip ? listing_.tooltip(*d) : std::nullopt, true, std::nullopt});
        }
    }
    auto byName = [](std::vector<PickerEntry>& v) {
        std::stable_sort(v.begin(), v.end(), [](const PickerEntry& a, const PickerEntry& b) {
            const std::string fa = foldName(a.label), fb = foldName(b.label);
            return fa != fb ? fa < fb : a.label < b.label;
        });
    };
    byName(bundled);
    byName(user);
    byName(instances);
    std::vector<PickerEntry> out = empty;
    for (auto* v : {&bundled, &user, &instances, &modules}) out.insert(out.end(), v->begin(), v->end());
    if (c.busy())   // nothing is chosen while an operation runs
        for (PickerEntry& e : out)
            if (e.canChoose) {
                e.canChoose = false;
                e.hint = sessions::kAnotherOperationRuns;
            }
    return out;
}

std::shared_ptr<const MediaState> MediaPart::state(SessionContext& c) {
    sync(c);
    auto s = std::make_shared<MediaState>();
    for (const KindDecl& k : kinds()) {
        KindState ks;
        ks.kind = k.id;
        ks.label = k.label;
        ks.suffix = k.suffix;
        for (const ShelfEntry* e : shelf_.entries(k)) {
            KindState::Item i{e->name, e->description, e->fileName, e->origin, e->isTemplate, {}};
            for (const auto& [word, b] : bindings_)
                if ((!e->path.empty() && b.path == e->path) ||
                    (e->path.empty() && b.origin == e->origin && b.kind->id == k.id && b.name == e->name))
                    i.inPlace = word;
            (e->isTemplate ? ks.templates : ks.instances).push_back(std::move(i));
        }
        for (const auto& [word, b] : bindings_)   // an opened medium while it is in a place
            if (b.origin == Origin::Opened && b.kind->id == k.id)
                (b.isTemplate ? ks.templates : ks.instances)
                    .push_back(KindState::Item{b.name, b.description, baseName(b.path), Origin::Opened, b.isTemplate, word});
        s->kinds.push_back(std::move(ks));
    }
    Machine* m = c.machine();
    for (const Place& p : places_) {
        PlaceState ps;
        ps.place = p.word;
        ps.kind = p.kind ? p.kind->id : std::string();
        ps.slot = p.slot;
        ps.passThrough = p.passThrough;
        ps.sides = p.sides;
        MediaPlace* cap = m ? placeCap(*m, p) : nullptr;
        if (p.passThrough && cap) ps.passing = cap->side();
        const auto it = bindings_.find(p.word);
        if (it != bindings_.end()) {
            const Binding& b = it->second;
            ps.holds = PlaceState::Held{b.name, p.passThrough ? ps.passing : b.side, b.isTemplate, b.unsaved, b.writeProtected};
            ps.nameAndSave = b.kind->offersNameAndSave && !b.instance() && !b.writeProtected;   // a medium whose switch is on cannot change
            ps.askWriteProtect = ps.nameAndSave && b.hasWriteProtect;
        } else if (p.slot && m) {
            if (const auto plugged = pluggedAt(*m, p)) {   // a module that is not a medium
                const machine::PeripheralDescriptor* d = catalogue_ ? catalogue_->peripheral(*plugged) : nullptr;
                ps.holds = PlaceState::Held{d && !d->name.empty() ? d->name : plugged->value, std::nullopt, false, false, false};
            }
        }
        if (cap) ps.waits = cap->waitsForMedium();
        ps.picker = picker(c, p);
        s->places.push_back(std::move(ps));
    }
    return s;
}

void MediaPart::updateTopic(SessionContext& c) {
    if (topic_) topic_->set(state(c));
}

} // namespace calcu::media
