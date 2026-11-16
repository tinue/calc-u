#include "opening/LibraryOpeners.hpp"
#include "media/MediaPart.hpp"
#include "opening/Classify.hpp"
#include "presets/PresetsPart.hpp"

namespace calcu::opening {

using sessions::Value;

namespace {
std::string text(const OpenedFile& f) { return f.bytes ? std::string(f.bytes->begin(), f.bytes->end()) : std::string(); }

media::MediaPart::OpenedMedium opened(const media::MediaPart& m, const OpenedFile& f) {
    return m.readOpened(fileNameOf(f), text(f), f.path.value_or(std::string()));
}

Value fileArgs(const OpenedFile& f) {
    Value::Object o{{"name", Value(fileNameOf(f))}};
    if (f.path) o.push_back({"path", Value(*f.path)});
    if (f.bytes) o.push_back({"bytes", Value(Value::Bytes(*f.bytes))});
    return Value(std::move(o));
}
} // namespace

// ── LIB-6 ──

PresetsOpener::PresetsOpener(presets::PresetsPart& p, std::vector<std::string> extensions) : part_(&p) {
    for (auto& e : extensions) extensions_.push_back(lowerAscii(e));
}

sessions::CommandOutcome PresetsOpener::loadFile(const std::string& path, sessions::SessionContext& c) {
    return part_->loadPreset(Value::object({{"ref", path}}), c);
}

sessions::CommandOutcome PresetsOpener::loadContent(const OpenedFile& f, sessions::SessionContext& c) {
    return part_->loadPreset(Value::object({{"content", text(f)}, {"name", fileNameOf(f)}}), c);
}

// ── LIB-7 ──

MediaOpener::MediaOpener(media::MediaPart& m) : part_(&m) {}

std::vector<std::string> MediaOpener::suffixes() const {
    std::vector<std::string> out;
    for (const auto& k : part_->kinds())
        if (!k.suffix.empty()) out.push_back(lowerAscii(k.suffix));
    return out;
}

MediumFile MediaOpener::read(const OpenedFile& f) const {
    const auto o = opened(*part_, f);
    MediumFile m;
    m.refusal = o.refusal;
    if (o.kind) {
        m.kind = o.kind->id;
        m.kindName = o.kind->label;
    }
    m.isTemplate = o.isTemplate;
    m.isModule = o.isModule;
    return m;
}

std::vector<std::string> MediaOpener::placesFor(const MediumFile&, const OpenedFile& f, const machine::Machine& mc) const {
    return part_->placesFor(opened(*part_, f), mc);
}

std::vector<std::string> MediaOpener::placesInCatalogue(const MediumFile& m) const {
    media::MediaPart::OpenedMedium o;
    o.kind = part_->kind(m.kind);
    o.isTemplate = m.isTemplate;
    o.isModule = m.isModule;
    return part_->placesInCatalogue(o);
}

std::optional<std::string> MediaOpener::refusesNow(const std::string& place, const machine::Machine& mc) const {
    return part_->refusesNow(place, mc);
}

sessions::CommandOutcome MediaOpener::useWhereItLies(const OpenedFile& f, const std::string& place,
                                                     sessions::SessionContext& c) {
    Value args = fileArgs(f);
    std::vector<Value::Member> o = *args.members();
    o.push_back({"place", Value(place)});
    return part_->openFile(Value(std::move(o)), c);
}

sessions::CommandOutcome MediaOpener::importAndUse(const OpenedFile& f, const std::optional<std::string>& place,
                                                   bool confirmed, sessions::SessionContext& c) {
    Value args = fileArgs(f);
    std::vector<Value::Member> o = *args.members();
    o.push_back({"upload", Value(true)});
    if (confirmed) o.push_back({"confirmed", Value(true)});
    if (place) o.push_back({"place", Value(*place)});
    return part_->openFile(Value(std::move(o)), c);
}

} // namespace calcu::opening
