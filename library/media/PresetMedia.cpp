#include "media/PresetMedia.hpp"
#include "formats/Flow.hpp"
#include "media/Names.hpp"
#include "presets/Engine.hpp"
#include "sessions/LibraryTopics.hpp"
#include "settings/AtomicFile.hpp"
#include <algorithm>
#include <set>

namespace calcu::media {

using formats::YamlNode;
using presets::After;
using presets::Item;
using presets::ItemResult;
using presets::ReadContext;
using presets::RunContext;
using sessions::Task;

namespace {

ItemResult failed(std::string why) { return ItemResult{false, std::move(why), {}}; }
std::string inQuotes(const std::string& s) { return "“" + s + "”"; }
std::string text(const YamlNode& n) { return formats::scalarText(n); }

std::shared_ptr<InFileSet> inFileSetOf(ReadContext& ctx) {
    if (auto kept = ctx.kept(kInFileKey))
        return std::const_pointer_cast<InFileSet>(std::static_pointer_cast<const InFileSet>(kept));
    auto set = std::make_shared<InFileSet>();
    ctx.keep(kInFileKey, set);
    return set;
}

// The sides the places of a kind have anywhere in the catalogue.
std::set<int> sidesOf(const machine::Catalogue& cat, const KindDecl& k) {
    std::set<int> out;
    auto add = [&](const std::vector<machine::MediaPlaceDescriptor>& v) {
        for (const auto& p : v)
            if (p.mediumKind == k.id) out.insert(p.sides.begin(), p.sides.end());
    };
    for (const auto& m : cat.models()) add(m.mediaPlaces);
    for (const auto& p : cat.peripherals()) add(p.mediaPlaces);
    return out;
}

// A reference (docs/formats/README.md §3; AC-21): a mapping with exactly one
// of name:, file: and (where the kind has in-file media) mem:, plus side:
// where the places have sides.
struct Reference {
    enum class By { Name, File, Mem } by = By::Name;
    std::string value;
    int side = 0;
};

std::optional<Reference> readReference(const std::string& verb, const YamlNode& v, ReadContext& ctx, const KindDecl& k,
                                       bool withSide) {
    const int line = v.line;
    if (!v.isMap()) {   // AC-21: a bare value is an error
        ctx.error(line, verb + ": takes a reference, as in { name: <name> } or { file: <path> }.");
        return std::nullopt;
    }
    const bool mem = !k.words.inFileKey.empty();
    Reference r;
    int ways = 0;
    for (const auto& [key, val] : v.map) {
        if (key == "name" || key == "file" || (mem && key == "mem")) {
            ++ways;
            r.by = key == "name" ? Reference::By::Name : key == "file" ? Reference::By::File : Reference::By::Mem;
            if (!val.isScalar() || text(val).empty()) {
                ctx.error(line, verb + ": " + key + ": needs a value.");
                return std::nullopt;
            }
            r.value = text(val);
        } else if (key == "side" && withSide) {
            const std::string s = val.isScalar() ? text(val) : std::string();
            if (s != "0" && s != "1") {
                ctx.error(line, verb + ": side: is 0 or 1.");
                return std::nullopt;
            }
            r.side = s == "1" ? 1 : 0;
        } else {
            ctx.error(line, verb + ": does not take " + key + ":.");
            return std::nullopt;
        }
    }
    if (ways != 1) {
        ctx.error(line, verb + ": a reference has exactly one of name:, file:" + std::string(mem ? " and mem:." : " and no other."));
        return std::nullopt;
    }
    return r;
}

// AC-23: a medium by name or file, resolved when the preset is read.
struct Found {
    Binding binding;
    Content content;
    MediumInfo info;
};

std::optional<Found> resolveByName(MediaPart& part, const KindDecl& k, const std::string& name, int line, ReadContext& ctx,
                                   const std::string& verb) {
    part.refreshShelf(nullptr);
    const ShelfEntry* e = part.shelf().find(k, name);
    if (!e) {
        ctx.error(line, verb + ": there is no " + k.label + " " + inQuotes(name) + ".");
        return std::nullopt;
    }
    if (e->outdatedHint) {
        ctx.error(line, verb + ": the " + k.label + " " + inQuotes(e->name) + " is outdated: " + *e->outdatedHint);
        return std::nullopt;
    }
    Found f{part.bindingFor(*e), e->info.content, e->info};
    return f;
}

std::optional<Found> resolveByFile(MediaPart& part, const KindDecl& k, const std::string& path, int line, ReadContext& ctx,
                                   const std::string& verb) {
    const std::string file = baseName(path);
    if (!hasSuffix(file, k)) {
        ctx.error(line, verb + ": the file " + file + " is not a " + k.label + " (" + k.suffix + ").");
        return std::nullopt;
    }
    const auto bytes = ctx.readFile(path, line);
    if (!bytes) return std::nullopt;
    const std::string b(bytes->begin(), bytes->end());
    const std::string resolved = ctx.source().resolvedName(path);
    const std::string dir = folderOfPath(path);
    const ReadOutcome r = k.codec->read(b, [&](const std::string& rel) -> std::optional<std::string> {
        auto c = ctx.source().readNamed(dir.empty() ? rel : dir + "/" + rel);
        if (auto* v = std::get_if<std::vector<std::uint8_t>>(&c)) return std::string(v->begin(), v->end());
        return std::nullopt;
    });
    if (r.unreadable()) {
        ctx.error(line, verb + ": the " + k.label + " file " + file + " cannot be read" + (r.problem.empty() ? "." : ": " + r.problem));
        return std::nullopt;
    }
    if (r.status == ReadOutcome::Status::Outdated) {
        ctx.error(line, verb + ": the " + k.label + " file " + file + " is outdated: " + Shelf::outdatedHint(k, r.info.formatVersion));
        return std::nullopt;
    }
    Found f;
    f.info = r.info;
    f.content = r.info.content;
    Binding& bd = f.binding;
    bd.kind = &k;
    bd.name = nameOf(r.info, file, k);
    bd.description = r.info.description;
    // AC-25: the file decides; a bundled file is a template (AC-1).
    const bool bundled = resolved.rfind("examples:", 0) == 0 ||
                         (!k.bundledFolder.empty() && folderOfPath(resolved) == k.bundledFolder);
    bd.isTemplate = r.info.isTemplate || bundled;
    bd.origin = bundled ? Origin::Bundled : Origin::Opened;
    if (part.shelf().byPath(resolved)) bd.origin = Origin::Saved;
    bd.path = resolved;
    bd.source = b;
    bd.hasWriteProtect = r.info.hasWriteProtect;
    bd.writeProtected = r.info.writeProtected;
    return f;
}

// ── The insert and arm steps ─────────────────────────────────────────────

class MediumItem final : public Item {
public:
    enum class Does { Insert, Arm };
    MediumItem(Does d, MediaPart& part, const KindDecl& k, std::string verb, Reference ref, std::optional<Found> found,
               std::shared_ptr<const InFileSet> set)
        : does_(d), part_(part), kind_(k), verb_(std::move(verb)), ref_(std::move(ref)), found_(std::move(found)),
          set_(std::move(set)) {}

    Task<ItemResult> carryOut(RunContext& run) override {
        sessions::SessionContext& s = run.session();
        Binding b;
        Content content;
        if (ref_.by == Reference::By::Mem) {   // AC-22: an in-file medium of this load
            if (run.plan() && set_) part_.adoptInFile(s, set_->defs);
            content = part_.inFileContent(kind_, ref_.value);
            b.kind = &kind_;
            b.name = ref_.value;
            b.origin = Origin::InFile;
            b.isTemplate = true;
            b.inFile = ref_.value;
            if (const auto inst = part_.inFileInstance(kind_, ref_.value)) {   // saved `as: live` before
                b.isTemplate = false;
                b.origin = Origin::Saved;
                b.path = *inst;
                b.inFile.clear();
                if (const ShelfEntry* e = part_.shelf().byPath(*inst)) {
                    b.name = e->name;
                    b.source = e->bytes;
                }
            }
        } else {
            b = found_->binding;
            content = found_->content;
            // Read again: what the file holds now (AC-25, a preset run again).
            if (!b.path.empty() && b.instance())
                if (const auto bytes = settings::readWholeFile(b.path)) {
                    const ReadOutcome r = kind_.codec->read(*bytes, companionsBeside(b.path));
                    if (r.status == ReadOutcome::Status::Read) {
                        content = r.info.content;
                        b.source = *bytes;
                    }
                }
        }
        const auto word = part_.placeOfKind(s, kind_);
        if (!word) co_return failed("There is no place for a " + kind_.label + " now.");
        if (does_ == Does::Arm) {   // fitted for the user to feed (C59-7 AC-17)
            if (auto r = part_.arm(s, *word, std::move(b), std::move(content))) co_return failed(*r);
            co_return ItemResult{};
        }
        if (auto r = part_.insert(s, *word, std::move(b), std::move(content), ref_.side)) co_return failed(*r);
        if (!kind_.words.insertNeedsTime) co_return ItemResult{};
        // A pass-through place: fitted and fed; until the medium has passed (and
        // stays fitted), within timeout: (AC-26; LIB-6 AC-57).
        machine::Machine* m = s.machine();
        if (!m) co_return failed(sessions::kNoMachine);
        const machine::TickRate rate = m->descriptor().tickRate;
        const machine::Ticks start = m->tickCount();
        // timeout: (LIB-6 AC-57); on its own (run-step) at most an hour, as LIB-6's waits.
        const machine::Ticks limit = run.deadline().value_or(static_cast<machine::Ticks>(rate) * 3600);
        while (!part_.takePassed(s, *word)) {
            m = s.machine();
            if (!m) co_return failed(sessions::kNoMachine);
            if (m->tickCount() - start >= limit)
                co_return failed("The " + kind_.label + " " + inQuotes(logName()) + " has not passed after the timeout.");
            co_await run.op().run(sessions::OperationContext::runPiece(rate));
        }
        co_return ItemResult{};
    }
    After after() const override {
        return does_ == Does::Insert && kind_.words.insertNeedsTime ? After::Own : After::TakenEffect;
    }
    bool needsRunningMachine() const override { return does_ == Does::Insert && kind_.words.insertNeedsTime; }
    std::string logText() const override { return verb_ + ": " + kind_.label + " " + inQuotes(logName()); }   // AC-25

private:
    std::string logName() const { return ref_.by == Reference::By::Mem || !found_ ? ref_.value : found_->binding.name; }
    Does does_;
    MediaPart& part_;
    const KindDecl& kind_;
    std::string verb_;
    Reference ref_;
    std::optional<Found> found_;
    std::shared_ptr<const InFileSet> set_;
};

// LIB-6 AC-47 (M-47): a medium step needs a place of its kind -- the
// model's own, or one of a peripheral the header (or the running machine)
// plugs in; else the error names the peripherals that have one.
bool placeFor(const KindDecl& k, const std::string& verb, int line, ReadContext& ctx) {
    const machine::MachineDescriptor& model = ctx.model();
    for (const auto& p : model.mediaPlaces)
        if (p.mediumKind == k.id) return true;
    std::vector<std::string> needs;
    for (const auto& d : ctx.catalogue().peripherals()) {
        if (std::none_of(d.mediaPlaces.begin(), d.mediaPlaces.end(), [&](const auto& p) { return p.mediumKind == k.id; }))
            continue;
        for (const auto& place : model.places) {
            const auto info = ctx.catalogue().place(model.model, place.name);
            if (!info || std::find(info->fitting.begin(), info->fitting.end(), d.id) == info->fitting.end()) continue;
            if (ctx.plugged(place.name, d.id)) return true;
            needs.push_back(d.name.empty() ? d.id.value : d.name);
            break;
        }
    }
    std::string list;
    for (std::size_t i = 0; i < needs.size(); ++i) list += (i == 0 ? "" : i + 1 == needs.size() ? " or " : ", ") + needs[i];
    ctx.error(line, verb + ": the " + model.model.value + " has no place for a " + k.label +
                        (list.empty() ? std::string(".") : " without the " + list + "."));
    return false;
}

std::unique_ptr<Item> readMedium(MediumItem::Does does, MediaPart& part, const KindDecl& k, const std::string& verb,
                                 const YamlNode& v, ReadContext& ctx) {
    if (!placeFor(k, verb, v.line, ctx)) return nullptr;
    const bool sides = does == MediumItem::Does::Insert && sidesOf(ctx.catalogue(), k).size() > 1;
    auto ref = readReference(verb, v, ctx, k, sides);
    if (!ref) return nullptr;
    std::optional<Found> found;
    std::shared_ptr<const InFileSet> set;
    switch (ref->by) {
    case Reference::By::Name: found = resolveByName(part, k, ref->value, v.line, ctx, verb); break;
    case Reference::By::File: found = resolveByFile(part, k, ref->value, v.line, ctx, verb); break;
    case Reference::By::Mem: set = inFileSetOf(ctx); break;   // AC-23: a name not defined is fresh
    }
    if (ref->by != Reference::By::Mem && !found) return nullptr;
    return std::make_unique<MediumItem>(does, part, k, verb, std::move(*ref), std::move(found), std::move(set));
}

// ── eject: ───────────────────────────────────────────────────────────────

class EjectItem final : public Item {
public:
    EjectItem(MediaPart& p, std::string w) : part_(p), word_(std::move(w)) {}
    Task<ItemResult> carryOut(RunContext& run) override {
        if (!part_.hasPlace(run.session(), word_)) co_return failed("There is no place " + word_ + " now.");
        part_.eject(run.session(), word_);   // EC-6: an empty place, nothing
        co_return ItemResult{};
    }
    After after() const override { return After::TakenEffect; }
    bool needsRunningMachine() const override { return false; }
    std::string logText() const override { return "eject: " + word_; }

private:
    MediaPart& part_;
    std::string word_;
};

// ── saveas: (preset.md §5.2.1) ───────────────────────────────────────────

class SaveasItem final : public Item {
public:
    SaveasItem(MediaPart& p, std::string place, const KindDecl* inFileKind, std::string inFile, SaveRequest r, bool protect,
               std::shared_ptr<const InFileSet> set)
        : part_(p), place_(std::move(place)), inFileKind_(inFileKind), inFile_(std::move(inFile)), request_(std::move(r)),
          protect_(protect), set_(std::move(set)) {}
    Task<ItemResult> carryOut(RunContext& run) override {
        sessions::SessionContext& s = run.session();
        MediaPart::SaveResult r;
        if (inFileKind_) {
            if (run.plan() && set_) part_.adoptInFile(s, set_->defs);
            r = part_.saveInFile(s, *inFileKind_, inFile_, request_, protect_);
        } else {
            if (!part_.hasPlace(s, place_)) co_return failed("There is no place " + place_ + " now.");
            r = part_.save(s, place_, request_, protect_);
        }
        if (!r.ok) co_return failed(r.reason);   // LIB-6 AC-17
        if (r.powerCycle) co_await part_.powerCycle(run.op());   // AC-19
        co_return ItemResult{};
    }
    After after() const override { return After::TakenEffect; }
    bool needsRunningMachine() const override { return false; }
    std::string logText() const override {
        return "saveas: " + (inFileKind_ ? inFileKind_->words.inFileKey + " " + inFile_ : place_) + " as " +
               (request_.by == SaveBy::SaveasTemplate ? "template " : "live ") + inQuotes(request_.target);
    }

private:
    MediaPart& part_;
    std::string place_;
    const KindDecl* inFileKind_;
    std::string inFile_;
    SaveRequest request_;
    bool protect_;
    std::shared_ptr<const InFileSet> set_;
};

std::unique_ptr<Item> readSaveas(MediaPart& part, const YamlNode& v, ReadContext& ctx) {
    const int line = v.line;
    if (!v.isMap()) {
        ctx.error(line, "saveas: takes a mapping, as in { place: slot-1, as: live, name: My module }.");
        return nullptr;
    }
    std::string place, as, name, file, inFile;
    const KindDecl* inFileKind = nullptr;
    bool protect = false;
    for (const auto& [key, val] : v.map) {
        const std::string t = val.isScalar() ? text(val) : std::string();
        const KindDecl* k = nullptr;
        for (const KindDecl& x : part.kinds())
            if (!x.words.inFileKey.empty() && x.words.inFileKey == key) k = &x;
        if (key == "place") place = t;
        else if (key == "as") as = t;
        else if (key == "name") name = t;
        else if (key == "file") file = t;
        else if (key == "protected") {
            if (t != "true" && t != "false") {
                ctx.error(line, "saveas: protected: is true or false.");
                return nullptr;
            }
            protect = t == "true";
        } else if (k) {
            inFileKind = k;
            inFile = t;
        } else {
            ctx.error(line, "saveas: does not take " + key + ":.");
            return nullptr;
        }
        if (key != "protected" && t.empty()) {
            ctx.error(line, "saveas: " + key + ": needs a value.");
            return nullptr;
        }
    }
    if (place.empty() == !inFileKind) {
        ctx.error(line, "saveas: has exactly one of place: and an in-file medium.");
        return nullptr;
    }
    if (as != "live" && as != "template") {
        ctx.error(line, "saveas: as: is live or template.");
        return nullptr;
    }
    if (name.empty() == file.empty()) {
        ctx.error(line, "saveas: has exactly one of name: and file:.");
        return nullptr;
    }
    SaveRequest r;
    r.by = as == "live" ? SaveBy::SaveasLive : SaveBy::SaveasTemplate;
    r.byFile = !file.empty();
    r.target = file.empty() ? name : ctx.source().resolvedName(file);
    std::shared_ptr<const InFileSet> set = inFileKind ? inFileSetOf(ctx) : nullptr;
    return std::make_unique<SaveasItem>(part, place, inFileKind, inFile, std::move(r), protect, std::move(set));
}

// ── Slot keys (LIB-6 AC-3; AC-21, AC-23, AC-24) ─────────────────────────

const KindDecl* moduleKindFor(MediaPart& part, const ReadContext& ctx, const machine::PlaceName& place) {
    const auto info = ctx.catalogue().place(ctx.model().model, place);
    if (!info) return nullptr;
    for (const KindDecl& k : part.kinds()) {
        if (k.heldBy != HeldBy::ModuleSlot || !k.isMedium) continue;
        for (const auto& id : info->fitting)
            if (const auto* d = ctx.catalogue().peripheral(id); d && k.isMedium(*d)) return &k;
    }
    return nullptr;
}

presets::PlaceResolution resolvePlace(MediaPart& part, const machine::PlaceName& place, const YamlNode& v, int line,
                                      ReadContext& ctx) {
    using How = presets::PlaceResolution::How;
    const KindDecl* k = moduleKindFor(part, ctx, place);
    if (!k) return {};
    const std::string key = place.value;
    std::optional<Found> found;
    if (const YamlNode* n = v.find("name")) {
        part.refreshShelf(nullptr);
        // AC-5: one list of names; a module of the catalogue is the reader's.
        const ShelfEntry* e = nullptr;
        for (const KindDecl& x : part.kinds())
            if (!e && x.heldBy == HeldBy::ModuleSlot) e = part.shelf().find(x, text(*n));
        if (!e) return {};
        if (e->outdatedHint) {
            ctx.error(line, key + ": the " + e->kind->label + " " + inQuotes(e->name) + " is outdated: " + *e->outdatedHint);
            return presets::PlaceResolution{How::Error, {}};
        }
        k = e->kind;
        found = Found{part.bindingFor(*e), e->info.content, e->info};
    } else if (const YamlNode* f = v.find("file")) {
        const std::string path = text(*f);
        const KindDecl* byFile = nullptr;
        for (const KindDecl& x : part.kinds())
            if (x.heldBy == HeldBy::ModuleSlot && hasSuffix(baseName(path), x)) byFile = &x;
        if (!byFile) {
            ctx.error(line, key + ": the file " + baseName(path) + " is not a module medium of this product.");
            return presets::PlaceResolution{How::Error, {}};
        }
        k = byFile;
        found = resolveByFile(part, *k, path, line, ctx, key);
        if (!found) return presets::PlaceResolution{How::Error, {}};
    } else {
        return {};
    }
    const machine::PlacePath at{{place}};
    auto req = k->moduleRequest ? k->moduleRequest(found->info, at) : std::nullopt;
    if (!req) {
        ctx.error(line, key + ": the " + k->label + " " + inQuotes(found->binding.name) + " cannot be plugged in.");
        return presets::PlaceResolution{How::Error, {}};
    }
    req->place = at;
    found->binding.peripheral = req->peripheral.value;
    part.registerModule(req->content, found->binding);   // S-4: bound after the build (AC-24)
    return presets::PlaceResolution{How::Done, std::move(*req)};
}

} // namespace

void defineInFile(ReadContext& ctx, const KindDecl& k, std::string name, Content content) {
    inFileSetOf(ctx)->defs.items.push_back(InFileDef{k.id, std::move(name), std::move(content)});
}

void addMediaToPresets(presets::Vocabulary& v, MediaPart& part) {
    MediaPart* p = &part;
    bool any = false;
    for (const KindDecl& k : part.kinds()) {
        if (k.heldBy != HeldBy::MediaPlace) continue;
        const KindDecl* kp = &k;
        if (!k.words.insertStep.empty()) {
            any = true;
            v.add(presets::StepDef{k.words.insertStep, presets::StepDef::Value::Any,
                                   [p, kp](const YamlNode& n, ReadContext& c) {
                                       return readMedium(MediumItem::Does::Insert, *p, *kp, kp->words.insertStep, n, c);
                                   },
                                   true, false});
        }
        if (!k.words.armStep.empty())
            v.add(presets::StepDef{k.words.armStep, presets::StepDef::Value::Any,
                                   [p, kp](const YamlNode& n, ReadContext& c) {
                                       return readMedium(MediumItem::Does::Arm, *p, *kp, kp->words.armStep, n, c);
                                   },
                                   true, false});
    }
    // D-20: eject: and saveas: are the library's.
    v.add(presets::StepDef{"eject", presets::StepDef::Value::Scalar,
                           [p](const YamlNode& n, ReadContext& c) -> std::unique_ptr<Item> {
                               const std::string word = text(n);
                               std::string words;
                               for (const KindDecl& k : p->kinds()) {
                                   if (k.words.ejectWord.empty()) continue;
                                   if (k.words.ejectWord == word) return std::make_unique<EjectItem>(*p, word);
                                   words += (words.empty() ? "" : " or ") + k.words.ejectWord;
                               }
                               for (const auto& mp : c.model().mediaPlaces)
                                   if (mp.id.value == word) return std::make_unique<EjectItem>(*p, word);
                               c.error(n.line, "eject: takes " + (words.empty() ? std::string("a media place") : words) + ".");
                               return nullptr;
                           },
                           true, false});
    v.add(presets::StepDef{"saveas", presets::StepDef::Value::Any,
                           [p](const YamlNode& n, ReadContext& c) { return readSaveas(*p, n, c); }, true, false});
    (void)any;
    v.setPlaceResolver([p](const machine::PlaceName& place, const YamlNode& ref, int line, ReadContext& c) {
        return resolvePlace(*p, place, ref, line, c);
    });
    // bundle.md §3: a file a preset names may be a bundled medium of the product.
    v.setBundledMedium([p](std::string_view named) {
        const std::string file = baseName(named);
        for (const KindDecl& k : p->kinds())
            if (!k.bundledFolder.empty() && hasSuffix(file, k) && settings::fileExists(joinFile(k.bundledFolder, file)))
                return true;
        return false;
    });
}

} // namespace calcu::media
