#include "presets/BundleFile.hpp"
#include "formats/Flow.hpp"
#include "formats/Number.hpp"
#include "formats/Yaml.hpp"
#include <algorithm>
#include <set>

namespace calcu::presets {

using formats::scalarText;
using formats::YamlNode;

namespace {

const std::string kName = std::string("examples:") + kBundleFile;

ReadError at(int line, const std::string& what) { return ReadError{line, errorText(kName, line, what)}; }

// A reference that accepts file: only (bundle.md §2).
std::variant<std::string, ReadError> fileRef(const YamlNode& n, const char* what) {
    if (!n.isMap() || n.map.size() != 1 || !n.find("file") || !n.find("file")->isScalar())
        return at(n.line, std::string(what) + " is a reference { file: <path> }.");
    const std::string path = scalarText(*n.find("file"));
    const auto inside = normalBundlePath(path);
    if (!inside) return at(n.line, "The path " + path + " leaves the bundle.");
    return *inside;
}

std::optional<ReadError> readEntry(const YamlNode& n, bool startup, const std::string& group,
                                   std::vector<BundleEntry>& out) {
    if (!n.isMap()) return at(n.line, "An entry is a mapping with preset: and files:.");
    for (const auto& [k, v] : n.map)
        if (k != "preset" && k != "files") return at(v.line ? v.line : n.line, "Unknown key " + k + ".");
    const YamlNode* p = n.find("preset");
    if (!p) return at(n.line, "An entry needs preset:.");
    auto preset = fileRef(*p, "preset:");
    if (auto* e = std::get_if<ReadError>(&preset)) return *e;
    BundleEntry e;
    e.preset = std::get<std::string>(preset);
    e.startup = startup;
    e.group = group;
    e.line = n.line;
    if (const YamlNode* f = n.find("files")) {
        if (!f->isSeq()) return at(f->line, "files: is a list of { file: <path> }.");
        for (const YamlNode& x : f->seq) {
            auto file = fileRef(x, "A file");
            if (auto* err = std::get_if<ReadError>(&file)) return *err;
            e.files.push_back(std::get<std::string>(file));
        }
    }
    out.push_back(std::move(e));
    return std::nullopt;
}

// A source that keeps every file the preset names (checkBundle).
class Recording final : public PresetSource {
public:
    explicit Recording(std::unique_ptr<PresetSource> s) : s_(std::move(s)) {}
    std::string name() const override { return s_->name(); }
    std::string stem() const override { return s_->stem(); }
    std::optional<std::string> text() const override { return s_->text(); }
    std::string readError() const override { return s_->readError(); }
    std::variant<std::vector<std::uint8_t>, std::string> readNamed(std::string_view p) const override {
        named.push_back(s_->resolvedName(p));
        return s_->readNamed(p);
    }
    std::string resolvedName(std::string_view p) const override { return s_->resolvedName(p); }
    mutable std::vector<std::string> named;

private:
    std::unique_ptr<PresetSource> s_;
};

} // namespace

std::variant<std::vector<BundleEntry>, ReadError> readBundleFile(const PresetBundle& bundle) {
    const auto bytes = bundle.read(kBundleFile);
    if (!bytes) return ReadError{0, "There is no bundle file " + kName + "."};
    const std::string text(bytes->begin(), bytes->end());
    YamlNode root;
    std::string error;
    if (!formats::parseYaml(text, &root, &error)) return ReadError{0, kName + ": " + error};
    if (!root.isMap()) return at(0, "The bundle file is a mapping.");
    const YamlNode* format = root.find("format");
    if (!format || scalarText(*format) != "example-bundle") return at(0, "format: example-bundle is missing.");
    const YamlNode* version = root.find("format-version");
    const auto v = version && version->isScalar() ? formats::parseNumber(scalarText(*version)) : std::nullopt;
    if (!v || *v != 1) return at(version ? version->line : 0, "This release reads format-version: 1 of the bundle file.");
    for (const auto& [k, n] : root.map)
        if (k != "format" && k != "format-version" && k != "startup" && k != "groups")
            return at(n.line, "Unknown key " + k + ".");
    std::vector<BundleEntry> out;
    const YamlNode* startup = root.find("startup");
    const YamlNode* groups = root.find("groups");
    if (!startup || !startup->isSeq()) return at(startup ? startup->line : 0, "startup: is a list of entries.");
    if (!groups || !groups->isSeq()) return at(groups ? groups->line : 0, "groups: is a list of groups.");
    for (const YamlNode& e : startup->seq)
        if (auto err = readEntry(e, true, {}, out)) return *err;
    for (const YamlNode& g : groups->seq) {
        if (!g.isMap() || !g.find("group") || !g.find("examples") || g.map.size() != 2)
            return at(g.line, "A group has group: and examples:.");
        const std::string name = scalarText(*g.find("group"));
        const YamlNode& examples = *g.find("examples");
        if (!examples.isSeq()) return at(examples.line, "examples: is a list of entries.");
        for (const YamlNode& e : examples.seq)
            if (auto err = readEntry(e, false, name, out)) return *err;
    }
    return out;
}

std::vector<PresetListEntry> presetList(const PresetBundle& bundle, const ProductRules& rules,
                                        const machine::Catalogue& catalogue, std::vector<std::string>* left) {
    std::vector<PresetListEntry> out;
    auto entries = readBundleFile(bundle);
    if (auto* e = std::get_if<ReadError>(&entries)) {
        if (left) left->push_back(e->message);
        return out;
    }
    for (const BundleEntry& b : std::get<std::vector<BundleEntry>>(entries)) {
        const auto source = bundleSource(bundle, b.preset);
        auto h = readHeader(*source, rules, catalogue);
        if (auto* e = std::get_if<ReadError>(&h)) {   // EC-9
            if (left) left->push_back("The preset list leaves out " + source->name() + ": " + e->message);
            continue;
        }
        const PresetHeader& header = std::get<PresetHeader>(h);
        PresetListEntry x;
        x.bundlePath = source->name();
        x.model = header.model;
        x.name = header.name;
        x.description = header.description;
        x.kind = b.startup ? PresetListEntry::Kind::Startup : PresetListEntry::Kind::Example;
        x.group = b.group;
        x.files = b.files;
        out.push_back(std::move(x));
    }
    return out;
}

std::vector<std::string> checkBundle(const PresetBundle& bundle, const ReadSetup& setup) {
    std::vector<std::string> problems;
    auto entries = readBundleFile(bundle);
    if (auto* e = std::get_if<ReadError>(&entries)) return {e->message};
    std::set<std::string> presets;
    for (const BundleEntry& b : std::get<std::vector<BundleEntry>>(entries)) {
        if (!presets.insert(b.preset).second)
            problems.push_back("examples:" + b.preset + " is listed more than once.");
        if (!bundle.read(b.preset)) problems.push_back("The listed preset examples:" + b.preset + " is missing.");
        for (const std::string& f : b.files)
            if (!bundle.read(f)) problems.push_back("The listed file examples:" + f + " is missing.");
        Recording source(bundleSource(bundle, b.preset));
        auto plan = readPreset(source, setup);
        if (auto* e = std::get_if<ReadError>(&plan)) {
            if (bundle.read(b.preset)) problems.push_back(e->message);
        }
        for (const std::string& named : source.named) {
            const std::string inBundle = named.rfind("examples:", 0) == 0 ? named.substr(9) : named;
            // bundle.md §3: or a bundled medium or module of the product (LIB-7).
            if (std::find(b.files.begin(), b.files.end(), inBundle) == b.files.end() &&
                !setup.vocabulary.isBundledMedium(named))
                problems.push_back("examples:" + b.preset + " names " + named + ", which is not listed with it.");
        }
    }
    return problems;
}

} // namespace calcu::presets
