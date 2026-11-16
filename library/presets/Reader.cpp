#include "presets/Reader.hpp"
#include "formats/Number.hpp"
#include <algorithm>
#include <cstdlib>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <set>

namespace calcu::presets {

using formats::YamlNode;

std::string lowerAscii(std::string_view s) {
    std::string out(s);
    for (char& c : out) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return out;
}

std::string errorText(const std::string& name, int line, const std::string& what) {
    if (line <= 0) return name + ": " + what;
    return name + ", line " + std::to_string(line) + ": " + what;
}

// ── ReadContext ─────────────────────────────────────────────────────────

ReadContext::ReadContext(const ReadSetup& setup, const PresetSource& source, const machine::MachineDescriptor& model)
    : setup_(setup), source_(source), model_(model), rules_(&setup.rules.forModel(model.model)) {}

const typing::TypingProfile* ReadContext::typingProfile() const {
    return setup_.typing ? setup_.typing->profile(model_.model) : nullptr;
}

bool ReadContext::plugged(const machine::PlaceName& place, const machine::PeripheralId& id) const {
    const auto p = plugged_ ? plugged_(place) : std::nullopt;
    return p && *p == id;
}

bool ReadContext::pluggedAny(const machine::PlaceName& place) const {
    return plugged_ && plugged_(place).has_value();
}

std::optional<std::vector<std::uint8_t>> ReadContext::readFile(std::string_view path, int line) {
    auto r = source_.readNamed(path);
    if (auto* bytes = std::get_if<std::vector<std::uint8_t>>(&r)) return std::move(*bytes);
    error(line, std::get<std::string>(r));
    return std::nullopt;
}

void ReadContext::keep(std::string key, std::shared_ptr<const ProductValue> v) { kept_[std::move(key)] = std::move(v); }

void ReadContext::error(int line, std::string message) { errors_.push_back(ReadError{line, std::move(message)}); }

std::optional<ReadError> ReadContext::firstError() const {
    if (errors_.empty()) return std::nullopt;
    // D-3: the lowest line; the file as a whole (line 0) first; the first given among equals.
    const ReadError* best = &errors_.front();
    for (const auto& e : errors_)
        if (e.line < best->line) best = &e;
    return ReadError{best->line, errorText(source_.name(), best->line, best->message)};
}

namespace {

// ── The file split at its top-level items (D-3) ──────────────────────────

struct Chunk {
    int line = 0;
    std::string key;
    YamlNode value;
    std::string firstLine;     // as written, for the form of a value (`description: |`)
    std::optional<ReadError> syntax;   // line and message without the name
};

std::optional<ReadError> yamlError(const std::string& e) {
    // "line N: what"
    ReadError out;
    if (e.rfind("line ", 0) == 0) {
        const std::size_t colon = e.find(':');
        out.line = std::atoi(e.substr(5, colon - 5).c_str());
        out.message = e.substr(colon + 2);
    } else {
        out.message = e;
    }
    if (!out.message.empty()) out.message[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(out.message[0])));
    if (!out.message.empty() && out.message.back() != '.') out.message += ".";
    return out;
}

std::vector<Chunk> splitChunks(const std::string& text, const std::vector<std::string>& rawVerbs) {
    std::vector<std::string> lines;
    std::size_t at = 0;
    while (at <= text.size()) {
        std::size_t end = text.find('\n', at);
        if (end == std::string::npos) end = text.size();
        lines.push_back(text.substr(at, end - at));
        at = end + 1;
    }
    auto blank = [](const std::string& l) {
        const std::size_t f = l.find_first_not_of(" \r");
        return f == std::string::npos || l[f] == '#';
    };
    std::vector<Chunk> out;
    formats::YamlOptions opts;
    opts.rawStepVerbs = rawVerbs;
    std::size_t i = 0;
    // Lines before the first top-level item: blank or comments only.
    while (i < lines.size() && (blank(lines[i]) || lines[i][0] == ' ')) {
        if (!blank(lines[i])) {
            Chunk c;
            c.line = static_cast<int>(i) + 1;
            c.syntax = ReadError{c.line, "A top-level key was expected here, without indentation."};
            out.push_back(std::move(c));
        }
        ++i;
    }
    while (i < lines.size()) {
        const std::size_t start = i++;
        while (i < lines.size() && (blank(lines[i]) || lines[i][0] == ' ' || lines[i][0] == '\t')) ++i;
        std::string chunk(start, '\n');   // keeps the line numbers
        for (std::size_t k = start; k < i; ++k) chunk += lines[k] + "\n";
        Chunk c;
        c.line = static_cast<int>(start) + 1;
        c.firstLine = lines[start];
        YamlNode root;
        std::string error;
        if (!formats::parseYaml(chunk, &root, &error, opts)) {
            c.syntax = yamlError(error);
            if (c.syntax->line == 0) c.syntax->line = c.line;
        } else if (!root.isMap() || root.map.size() != 1) {
            c.syntax = ReadError{c.line, "A top-level key (key: value) was expected."};
        } else {
            c.key = root.map.front().first;
            c.value = std::move(root.map.front().second);
            if (c.value.line == 0) c.value.line = c.line;
        }
        out.push_back(std::move(c));
    }
    return out;
}

std::string scalarText(const YamlNode& n) {
    std::string s, e;
    n.asString(&s, &e);
    return s;
}

std::size_t codePoints(std::string_view s) {
    std::size_t n = 0;
    for (char c : s)
        if ((static_cast<unsigned char>(c) & 0xC0) != 0x80) ++n;
    return n;
}

std::string modelList(const machine::Catalogue& cat) {
    std::string out;
    for (std::size_t i = 0; i < cat.models().size(); ++i)
        out += (i == 0 ? "" : i + 1 == cat.models().size() ? " and " : ", ") + cat.models()[i].model.value;
    return out;
}

template <class D> std::string variantList(const D& d) {
    std::string out;
    for (std::size_t i = 0; i < d.variants.size(); ++i) out += (i == 0 ? "" : ", ") + d.variants[i].id.value;
    return out;
}

template <class D>
std::optional<machine::VariantId> variantNamed(const D& d, std::string_view name) {
    for (const auto& v : d.variants)
        if (lowerAscii(v.id.value) == lowerAscii(name)) return v.id;
    return std::nullopt;
}

const machine::PeripheralDescriptor* peripheralNamed(const machine::Catalogue& cat, std::string_view name) {
    for (const auto& p : cat.peripherals())
        if (lowerAscii(p.id.value) == lowerAscii(name)) return &p;
    return nullptr;
}

std::string nameOf(const machine::PeripheralDescriptor& d) { return d.name.empty() ? d.id.value : d.name; }

// What a keyword that the rules do not apply refuses with (AC-62).
bool applies(ReadContext& ctx, std::string_view keyword, int line) {
    switch (ctx.rules().applies(keyword)) {
    case Applies::Yes: return true;
    case Applies::NotForThisModel:
        ctx.error(line, std::string(keyword) + " is not available on the " + ctx.model().model.value + ".");
        return false;
    case Applies::Undefined:
        ctx.error(line, std::string(keyword) + " is not defined yet for the " + ctx.model().model.value +
                            " by the preset format.");
        return false;
    }
    return false;
}

bool isPlaceOfAnotherModel(const machine::Catalogue& cat, const std::string& key) {
    for (const auto& m : cat.models())
        if (m.place(machine::PlaceName{key})) return true;
    return false;
}

// A place key (AC-2, AC-3; D-22).
std::optional<PlaceRequest> readPlace(const std::string& key, const YamlNode& v, int line, ReadContext& ctx) {
    const machine::PlaceName place{key};
    const PlaceKeyRule rule = ctx.rules().placeKey(place);
    std::string id, variant;
    if (v.isScalar()) {
        if (!rule.bare) {
            ctx.error(line, key + ": takes a reference, as in { name: <ID> }.");
            return std::nullopt;
        }
        const std::string s = scalarText(v);
        const std::size_t colon = s.find(':');
        id = s.substr(0, colon);
        if (colon != std::string::npos) variant = s.substr(colon + 1);
    } else if (v.isMap()) {
        const YamlNode* name = v.find("name");
        const YamlNode* file = v.find("file");
        if ((name ? 1 : 0) + (file ? 1 : 0) != 1 || v.map.size() != 1) {
            ctx.error(line, key + ": a reference has exactly one of name: and file:.");
            return std::nullopt;
        }
        if (file && !rule.byFile) {
            ctx.error(line, key + ": does not take a reference by file:.");
            return std::nullopt;
        }
        // LIB-7 (M-41): a module that is a medium, by its name tag or its file.
        if (const PlaceResolver& resolve = ctx.vocabulary().placeResolver()) {
            PlaceResolution res = resolve(place, v, line, ctx);
            if (res.how == PlaceResolution::How::Error) return std::nullopt;
            if (res.how == PlaceResolution::How::Done) {
                PlaceRequest r;
                r.line = line;
                r.request = std::move(res.request);
                r.request.place = machine::PlacePath{{place}};
                return r;
            }
        }
        if (file) {   // D-22: without LIB-7's resolver
            ctx.error(line, key + ": a reference by file: is not read by this release; use name:.");
            return std::nullopt;
        }
        if (!rule.byName || !name->isScalar()) {
            ctx.error(line, key + ": does not take a reference by name:.");
            return std::nullopt;
        }
        id = scalarText(*name);
    } else {
        ctx.error(line, key + ": names a peripheral.");
        return std::nullopt;
    }
    const machine::PeripheralDescriptor* d = peripheralNamed(ctx.catalogue(), id);
    if (!d) {
        ctx.error(line, "There is no peripheral " + id + ".");
        return std::nullopt;
    }
    const auto info = ctx.catalogue().place(ctx.model().model, place);
    if (!info || std::find(info->fitting.begin(), info->fitting.end(), d->id) == info->fitting.end()) {
        ctx.error(line, "The " + nameOf(*d) + " does not fit " + key + " of the " + ctx.model().model.value + ".");
        return std::nullopt;
    }
    PlaceRequest r;
    r.line = line;
    r.request.place = machine::PlacePath{{place}};
    r.request.peripheral = d->id;
    if (!variant.empty()) {
        r.request.variant = variantNamed(*d, variant);
        if (!r.request.variant) {
            ctx.error(line, "The " + nameOf(*d) + " has no variant " + variant +
                                (d->variants.empty() ? "." : "; its variants are " + variantList(*d) + "."));
            return std::nullopt;
        }
    }
    return r;
}

enum class EntryKind { Item, SkipWait, LetItFinish };
struct Entry {
    EntryKind kind;
    int line;
    std::unique_ptr<Item> item;
};

// One step of a step list, or of run-step.
std::unique_ptr<Item> readOneStep(const std::string& verb, const YamlNode& value, int line, const Vocabulary& vocab,
                                  ReadContext& ctx) {
    const StepDef* def = vocab.step(verb);
    if (!def) {
        ctx.error(line, "Unknown step " + verb + ".");   // AC-30
        return nullptr;
    }
    if (!applies(ctx, verb, line)) return nullptr;
    switch (def->value) {
    case StepDef::Value::None:
        if (!value.isNull()) {
            ctx.error(line, verb + ": takes no value.");
            return nullptr;
        }
        break;
    case StepDef::Value::Scalar:
        if (!value.isScalar()) {
            ctx.error(line, verb + ": needs a value.");
            return nullptr;
        }
        break;
    case StepDef::Value::Raw:
        if (!value.isScalar()) {
            ctx.error(line, verb + ": needs a text.");
            return nullptr;
        }
        break;
    case StepDef::Value::Any: break;
    }
    std::unique_ptr<Item> item = def->read(value, ctx);
    if (item && item->line == 0) item->line = line;
    return item;
}

void readSteps(const YamlNode& list, int line, const Vocabulary& vocab, ReadContext& ctx, std::vector<Entry>& out) {
    if (list.isNull() || (list.isSeq() && list.seq.empty())) {   // EC-8
        ctx.error(line, "keys: has no steps.");
        return;
    }
    if (!list.isSeq()) {
        ctx.error(line, "keys: holds a list of steps, each - verb: value.");
        return;
    }
    for (const YamlNode& s : list.seq) {
        const int at = s.line ? s.line : line;
        if (!s.isMap() || s.map.size() != 1) {
            ctx.error(at, "A step is written - verb: value.");
            continue;
        }
        const std::string& verb = s.map.front().first;
        const YamlNode& value = s.map.front().second;
        if (verb == kSkipWait || verb == kLetItFinish) {
            if (!value.isNull()) ctx.error(at, verb + ": takes no value.");
            else if (applies(ctx, verb, at))
                out.push_back(Entry{verb == kSkipWait ? EntryKind::SkipWait : EntryKind::LetItFinish, at, nullptr});
            continue;
        }
        if (auto item = readOneStep(verb, value, at, vocab, ctx)) out.push_back(Entry{EntryKind::Item, at, std::move(item)});
    }
}

const char* const kLibraryHeaderKeys[] = {"preset-name", "description", "timeout"};

} // namespace

std::variant<LoadPlan, ReadError> readPreset(const PresetSource& source, const ReadSetup& setup) {
    const std::string name = source.name();
    const std::optional<std::string> text = source.text();
    if (!text) return ReadError{0, source.readError()};   // EC-1
    std::vector<Chunk> chunks = splitChunks(*text, setup.vocabulary.rawVerbs());
    auto find = [&](const char* key) -> Chunk* {
        for (auto& c : chunks)
            if (!c.syntax && c.key == key) return &c;
        return nullptr;
    };
    const std::string product(setup.rules.productName());
    // A syntax error in format-version or model is reported before anything else.
    for (const auto& c : chunks)
        if (c.syntax && (c.firstLine.rfind("format-version:", 0) == 0 || c.firstLine.rfind("model:", 0) == 0))
            return ReadError{c.syntax->line, errorText(name, c.syntax->line, c.syntax->message)};

    // 1. format-version before every other key (README §2; AC-8).
    Chunk* fv = find("format-version");
    std::optional<std::int64_t> version;
    if (fv && fv->value.isScalar()) version = formats::parseNumber(scalarText(fv->value));
    if (!fv || (version && *version < 2))
        return ReadError{0, errorText(name, 0,
                                      "The file was written for an earlier release of " + product +
                                          " and must be upgraded by hand to preset format 2, as " +
                                          std::string(setup.rules.formatReference()) + " describes it.")};
    if (!version) return ReadError{fv->line, errorText(name, fv->line, "format-version: is a whole number.")};
    if (*version > 2)
        return ReadError{fv->line, errorText(name, fv->line,
                                             "The file was written by a newer release of " + product +
                                                 " (preset format " + std::to_string(*version) +
                                                 "); this release reads preset format 2.")};

    // 2. model (AC-9): nothing else can be checked without it.
    Chunk* mc = find("model");
    if (!mc) return ReadError{0, errorText(name, 0, "model: is required; the models are " + modelList(setup.catalogue) + ".")};
    const std::string written = mc->value.isScalar() ? scalarText(mc->value) : std::string();
    const std::size_t colon = written.find(':');
    const std::string modelName = written.substr(0, colon);
    const machine::MachineDescriptor* desc = nullptr;
    for (const auto& m : setup.catalogue.models())
        if (lowerAscii(m.model.value) == lowerAscii(modelName)) desc = &m;
    if (!desc)
        return ReadError{mc->line, errorText(name, mc->line,
                                             "There is no model " + modelName + " in " + product + "; its models are " +
                                                 modelList(setup.catalogue) + ".")};
    LoadPlan plan;
    plan.name = name;
    plan.model = desc->model;
    plan.modelLine = mc->line;
    if (colon != std::string::npos) {
        const std::string v = written.substr(colon + 1);
        plan.variant = variantNamed(*desc, v);
        if (!plan.variant)
            return ReadError{mc->line, errorText(name, mc->line,
                                                 desc->variants.empty()
                                                     ? "The " + desc->model.value + " has no variants."
                                                     : "The " + desc->model.value + " has no variant " + v +
                                                           "; its variants are " + variantList(*desc) + ".")};
    }
    plan.header.presetName = source.stem();

    ReadContext ctx(setup, source, *desc);
    plan.rules = &ctx.rules();
    ctx.setPlugged([&plan](const machine::PlaceName& p) -> std::optional<machine::PeripheralId> {
        for (const auto& r : plan.places)
            if (r.request.place.steps.size() == 1 && r.request.place.steps.front() == p) return r.request.peripheral;
        return std::nullopt;
    });

    // 3. The header keys, wherever they stand (AC-2, AC-14); each once.
    std::set<std::string> seen{"format-version", "model"};
    std::vector<std::pair<const HeaderKeyDef*, const Chunk*>> later;
    auto once = [&](const Chunk& c) {
        if (seen.insert(c.key).second) return true;
        ctx.error(c.line, c.key + ": is given twice; a header key appears once.");
        return false;
    };
    for (const Chunk& c : chunks) {
        if (c.syntax) {
            ctx.error(c.syntax->line, c.syntax->message);
            continue;
        }
        if (&c == fv || &c == mc) continue;
        const bool library = std::find(std::begin(kLibraryHeaderKeys), std::end(kLibraryHeaderKeys), c.key) !=
                             std::end(kLibraryHeaderKeys);
        if (c.key == "format-version" || c.key == "model" || library) {
            if (!once(c)) continue;
            if (!applies(ctx, c.key, c.line)) continue;
            if (c.key == "preset-name") {
                const std::string n = c.value.isScalar() ? scalarText(c.value) : std::string();
                if (n.empty()) ctx.error(c.line, "preset-name: needs a name.");
                else if (codePoints(n) > 18) ctx.error(c.line, "preset-name: has at most 18 characters.");   // D-12
                else {
                    plan.header.presetName = n;
                    plan.header.nameWritten = true;
                }
            } else if (c.key == "description") {   // D-26: one line
                const std::size_t col = c.firstLine.find(':');
                const std::string form = col == std::string::npos ? std::string() : c.firstLine.substr(col + 1);
                if (!c.value.isScalar() || form.find('|') == form.find_first_not_of(' '))
                    ctx.error(c.line, "description: is one line.");
                else plan.header.description = scalarText(c.value);
            } else if (c.key == "timeout") {   // D-11
                const auto t = c.value.isScalar() ? formats::parseNumber(scalarText(c.value)) : std::nullopt;
                if (!t || *t < 1) ctx.error(c.line, "timeout: is a whole number of seconds, at least 1.");
                else plan.header.timeoutSeconds = *t;
            }
            continue;
        }
        // D-31: the vocabulary first, so a product's header key may stand
        // under a place's name (Calc-U-1600's `host-drive:`).
        if (const HeaderKeyDef* h = setup.vocabulary.header(c.key)) {
            if (!once(c) || !applies(ctx, c.key, c.line)) continue;
            if (h->afterOtherHeaderKeys) later.emplace_back(h, &c);
            else h->read(c.value, c.line, ctx);
            continue;
        }
        if (desc->place(machine::PlaceName{c.key})) {
            if (!once(c) || !applies(ctx, c.key, c.line)) continue;
            if (auto r = readPlace(c.key, c.value, c.line, ctx)) plan.places.push_back(std::move(*r));
            continue;
        }
        if (c.key == kKeysSection || setup.vocabulary.section(c.key)) continue;   // below
        if (isPlaceOfAnotherModel(setup.catalogue, c.key)) {   // AC-62
            ctx.error(c.line, c.key + " is not available on the " + desc->model.value + ".");
            continue;
        }
        ctx.error(c.line, "Unknown key " + c.key + ".");   // AC-30
    }
    for (const auto& [h, c] : later) h->read(c->value, c->line, ctx);
    if (!ctx.firstError()) ctx.rules().checkHeader(plan, ctx);   // D-32

    // 4. The sections and steps in file order (AC-4, AC-14, AC-15).
    std::vector<Entry> entries;
    for (const Chunk& c : chunks) {
        if (c.syntax) continue;
        if (c.key == kKeysSection) {
            if (applies(ctx, c.key, c.line)) readSteps(c.value, c.line, setup.vocabulary, ctx, entries);
        } else if (const SectionDef* s = setup.vocabulary.section(c.key)) {
            if (!applies(ctx, c.key, c.line)) continue;
            if (auto item = s->read(c.value, ctx)) {
                if (item->line == 0) item->line = c.line;
                entries.push_back(Entry{EntryKind::Item, c.line, std::move(item)});
            }
        }
    }

    // 5. The markers (AC-58, AC-59).
    for (std::size_t i = 0; i < entries.size(); ++i) {
        Entry& e = entries[i];
        const bool first = i == 0, last = i + 1 == entries.size();
        if (e.kind == EntryKind::SkipWait) {
            if (first || last) ctx.error(e.line, "skip-wait: cannot be the first or the last item.");
            else if (!plan.items.empty()) plan.items.back().skipWaitAfter = true;
        } else if (e.kind == EntryKind::LetItFinish) {
            if (!last) ctx.error(e.line, "let-it-finish: can only be the last item.");
            else plan.letItFinish = true;
        } else {
            plan.items.push_back(PlannedItem{std::move(e.item), false});
        }
    }

    if (auto e = ctx.firstError()) return *e;
    plan.productHeader = ctx.takeKept();
    return plan;
}

std::variant<PresetHeader, ReadError> readHeader(const PresetSource& source, const ProductRules& rules,
                                                 const machine::Catalogue& catalogue) {
    const std::string name = source.name();
    const std::optional<std::string> text = source.text();
    if (!text) return ReadError{0, source.readError()};
    PresetHeader h;
    h.name = source.stem();
    bool version = false, model = false;
    for (const Chunk& c : splitChunks(*text, {})) {
        if (c.syntax) continue;   // only the header keys are read
        if (c.key == "format-version") {
            const auto v = c.value.isScalar() ? formats::parseNumber(scalarText(c.value)) : std::nullopt;
            if (!v || *v != 2)
                return ReadError{c.line, errorText(name, c.line, "This is not a preset of format 2 for " +
                                                                     std::string(rules.productName()) + ".")};
            version = true;
        } else if (c.key == "model") {
            const std::string w = c.value.isScalar() ? scalarText(c.value) : std::string();
            const std::string m = w.substr(0, w.find(':'));
            for (const auto& d : catalogue.models())
                if (lowerAscii(d.model.value) == lowerAscii(m)) {
                    h.model = d.model;
                    model = true;
                }
            if (!model) return ReadError{c.line, errorText(name, c.line, "There is no model " + m + ".")};
        } else if (c.key == "preset-name" && c.value.isScalar()) {
            h.name = scalarText(c.value);
        } else if (c.key == "description" && c.value.isScalar()) {
            h.description = scalarText(c.value);
        }
    }
    if (!version) return ReadError{0, errorText(name, 0, "format-version: 2 is missing.")};
    if (!model) return ReadError{0, errorText(name, 0, "model: is missing.")};
    return h;
}

std::unique_ptr<Item> readStep(std::string_view verb, const YamlNode& value, const ReadSetup& setup,
                               machine::Machine& m, std::string* error) {
    // The running machine in place of a header (§3.8).
    struct NoSource final : PresetSource {
        std::string name() const override { return "The step"; }
        std::string stem() const override { return {}; }
        std::optional<std::string> text() const override { return std::string(); }
        std::string readError() const override { return {}; }
        // LIB-7 (M-41; LIB-11 AC-22): a file a step's reference names
        // (`floppy { file: … }`), absolute or relative to the current folder.
        std::variant<std::vector<std::uint8_t>, std::string> readNamed(std::string_view p) const override {
            std::ifstream in(std::filesystem::path(std::u8string(p.begin(), p.end())), std::ios::binary);
            if (!in) return "The file " + std::string(p) + " cannot be read.";
            return std::vector<std::uint8_t>(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
        }
        std::string resolvedName(std::string_view p) const override { return std::string(p); }
    } source;
    ReadContext ctx(setup, source, m.descriptor());
    const machine::Plugged plugged = m.plugged();
    ctx.setPlugged([plugged](const machine::PlaceName& p) -> std::optional<machine::PeripheralId> {
        for (const auto& e : plugged.byPlace)
            if (e.place.steps.size() == 1 && e.place.steps.front() == p) return e.peripheral;
        return std::nullopt;
    });
    const std::string v(verb);
    std::unique_ptr<Item> item;
    if (v == kSkipWait || v == kLetItFinish) ctx.error(0, v + " has no command.");
    else item = readOneStep(v, value, 1, setup.vocabulary, ctx);
    if (auto e = ctx.firstError()) {
        // The message without the source's name.
        std::string msg = e->message;
        const std::string prefix = source.name();
        for (const std::string& cut : {prefix + ", line 1: ", prefix + ": "})
            if (msg.rfind(cut, 0) == 0) msg = msg.substr(cut.size());
        *error = msg;
        return nullptr;
    }
    return item;
}

} // namespace calcu::presets
