#include "app/SharpMediaKinds.hpp"

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "Audio/WavFile.hpp"
#include "Connector/BatteryCardInstance.hpp"
#include "Connector/FloppyImageFile.hpp"
#include "Connector/MemoryCardCatalog.hpp"
#include "Connector/MemoryCardDefinition.hpp"
#include "Connector/SoftwareDefinedCard.hpp"
#include "FileIO.hpp"
#include "Machine/SharpCatalogue.hpp"
#include "Machine/SharpMediaPlaces.hpp"
#include "files/Files.hpp"
#include "formats/Yaml.hpp"

namespace cu1600 {

namespace media = calcu::media;
using media::FileOut;
using media::MediumCodec;
using media::ReadOutcome;

namespace {

ReadOutcome unreadable(std::string problem) {
    ReadOutcome r;
    r.status = ReadOutcome::Status::Unreadable;
    r.problem = std::move(problem);
    if (!r.problem.empty() && r.problem.back() != '.') r.problem += '.';
    return r;
}

// docs/formats/media.md: a name is written double-quoted as it is, so it
// has no '"' and no line end (as the magnetic card's, Calc-U-59).
bool writableName(const std::string& name, const char* what, std::string& problem) {
    if (name.find('"') == std::string::npos && name.find('\n') == std::string::npos) return true;
    problem = std::string("A ") + what + " name has no \" and no line end.";
    return false;
}

std::string yamlQuoted(const std::string& s) {
    const char q = s.find('"') == std::string::npos ? '"' : '\'';
    return std::string(1, q) + s + q;
}

// ── Battery-backed module (`.module.yaml`, version 1; AC-37, AC-41) ───────

bool switchOf(const MemoryCardDefinition& d, bool* on) {
    bool has = false;
    *on = false;
    for (const Region& r : d.regions)
        for (std::uint32_t b = 0; b < r.banking.bankCount; ++b) {
            const RegionContent& c = r.contentForBank(b);
            has = has || c.hasWriteProtect;
            *on = *on || (c.hasWriteProtect && c.writeProtected);
        }
    return has;
}

class CardCodec final : public MediumCodec {
public:
    explicit CardCodec(std::string bundledDir) : m_bundledDir(std::move(bundledDir)) {}

    ReadOutcome read(const std::string& bytes, const Companion&) const override {
        MemoryCardDefinition def;
        std::string error;
        if (!parseMemoryCardDefinition(bytes, &def, &error)) return unreadable(error);
        ReadOutcome r;
        r.info.formatVersion = static_cast<int>(kCardFormatVersion);
        r.info.name = def.moduleName;
        if (!def.battery) {   // a module that is not a medium (D-9): the catalogue lists it
            r.status = ReadOutcome::Status::NotMedium;
            r.problem = "It is not a battery-backed module.";
            return r;
        }
        r.status = ReadOutcome::Status::Read;
        r.info.description = def.description;
        r.info.isTemplate = def.isTemplate;
        r.info.hasWriteProtect = switchOf(def, &r.info.writeProtected);
        r.info.content = std::make_shared<sharp::ModuleContent>(std::move(def));
        return r;
    }

    std::vector<FileOut> write(const Write& w, std::string& problem) const override {
        const auto* module = dynamic_cast<const sharp::ModuleContent*>(w.content.get());
        if (!module) {
            problem = "It is not a memory module.";
            return {};
        }
        // AC-41: spliced into the file it came from; without one (a module
        // the machine control plugged in by its name), into its bundled file.
        std::optional<std::string> source = w.source;
        if (!source) source = bundledText(module->definition.moduleName);
        if (!source) {
            problem = "There is no module file to save it from.";
            return {};
        }
        const SoftwareDefinedCard card(module->definition);
        std::string text, error;
        if (!spliceBatteryCardInstance(*source, w.name, module->definition.moduleName,
                                       formatBatteryCardInitialContentBlock(card.debugBankCount(), card.debugImage()),
                                       &text, &error, w.savedAt, w.asTemplate, w.writeProtected)) {
            problem = error;
            return {};
        }
        return {FileOut{"", text}};
    }

private:
    std::optional<std::string> bundledText(const std::string& moduleName) const {
        for (const MemoryCardCatalogEntry& e : scanMemoryCardDirectory(m_bundledDir, nullptr)) {
            if (e.moduleName != moduleName) continue;
            std::vector<std::uint8_t> bytes;
            if (readWholeFile(calcu::files::toPath(e.filePath), &bytes)) return std::string(bytes.begin(), bytes.end());
        }
        return std::nullopt;
    }
    std::string m_bundledDir;
};

// ── Floppy disk (`.floppy.yaml`, version 2; AC-37, AC-41) ───────────────

// The header (before `sides:`): its version, name and description.
struct FloppyHeader {
    long version = 0;
    std::string name, description;
};
std::optional<FloppyHeader> floppyHeader(const std::string& text) {
    calcu::formats::YamlNode root;
    std::string error;
    if (!calcu::formats::parseYaml(text.substr(0, text.find("\nsides:")), &root, &error) || !root.isMap()) return std::nullopt;
    FloppyHeader h;
    if (const auto* v = root.find("format-version")) v->asInt(&h.version, &error);
    if (const auto* n = root.find("disk-name")) n->asString(&h.name, &error);
    if (const auto* d = root.find("description")) d->asString(&h.description, &error);
    return h;
}

class FloppyCodec final : public MediumCodec {
public:
    ReadOutcome read(const std::string& bytes, const Companion&) const override {
        const auto header = floppyHeader(bytes);
        if (header && header->version == 1) {   // 0.8.x: sides a and b (row 1600-19)
            ReadOutcome r;
            r.status = ReadOutcome::Status::Outdated;
            r.info.formatVersion = 1;
            r.info.name = header->name;
            return r;
        }
        FloppyFile disk;
        std::string error;
        if (!parseFloppyFile(bytes, &disk, &error)) return unreadable(error);
        ReadOutcome r;
        r.status = ReadOutcome::Status::Read;
        r.info.formatVersion = static_cast<int>(kFloppyFormatVersion);
        r.info.name = disk.diskName;
        r.info.description = disk.description;
        r.info.isTemplate = disk.isTemplate;
        r.info.content = std::make_shared<sharp::FloppyContent>(std::move(disk.image));
        return r;
    }

    std::vector<FileOut> write(const Write& w, std::string& problem) const override {
        const auto* disk = dynamic_cast<const sharp::FloppyContent*>(w.content.get());
        if (!disk) {
            problem = "It is not a floppy disk.";
            return {};
        }
        if (!writableName(w.name, "disk", problem)) return {};
        std::string description;   // AC-41: kept
        if (w.source)
            if (const auto h = floppyHeader(*w.source)) description = h->description;
        return {FileOut{"", formatFloppyFile(w.name, disk->image, w.asTemplate, description, w.savedAt)}};
    }
};

// ── Cassette (`.cassette.yaml`, version 1, with its WAV; AC-42) ──────────────

struct CassetteDefinition {
    std::string name, description, audio;
    bool isTemplate = false;
};
std::optional<CassetteDefinition> cassetteDefinition(const std::string& text, std::string* error) {
    calcu::formats::YamlNode root;
    if (!calcu::formats::parseYaml(text, &root, error)) return std::nullopt;
    std::string format;
    const auto* f = root.find("format");
    if (!root.isMap() || !f || !f->asString(&format, error) || format != "cassette") {
        *error = "not a cassette definition (expected 'format: cassette')";
        return std::nullopt;
    }
    if (!calcu::formats::requireFormatVersion(root, 1, "cassette definition", error)) return std::nullopt;
    if (!root.requireOnlyKeys({"format", "format-version", "cassette-name", "description", "template", "saved", "audio", "position"}, error))
        return std::nullopt;
    CassetteDefinition t;
    const auto* n = root.find("cassette-name");
    if (!n || !n->asString(&t.name, error) || t.name.empty()) {
        *error = "missing 'cassette-name'";
        return std::nullopt;
    }
    if (const auto* d = root.find("description"); d && !d->asString(&t.description, error)) return std::nullopt;
    if (const auto* tp = root.find("template"); tp && !tp->asBool(&t.isTemplate, error)) return std::nullopt;
    const auto* a = root.find("audio");
    const auto* file = a && a->isMap() ? a->find("file") : nullptr;
    if (!file || !a->requireOnlyKeys({"file"}, error) || !file->asString(&t.audio, error) || t.audio.empty()) {
        *error = "'audio' must be { file: <sound file> }";
        return std::nullopt;
    }
    if (const auto* p = root.find("position")) {   // reserved: 0 (OQ-9)
        long pos = -1;
        if (!p->asInt(&pos, error) || pos != 0) {
            *error = "'position' is reserved and must be 0";
            return std::nullopt;
        }
    }
    return t;
}

class CassetteCodec final : public MediumCodec {
public:
    ReadOutcome read(const std::string& bytes, const Companion& companion) const override {
        std::string error;
        const auto t = cassetteDefinition(bytes, &error);
        if (!t) return unreadable(error);
        const auto wav = companion ? companion(t->audio) : std::nullopt;
        if (!wav) return unreadable("its sound file " + t->audio + " is missing");
        std::vector<std::uint8_t> sound(wav->begin(), wav->end());
        WavData decoded;
        if (!readWavBytes(sound, decoded, error)) return unreadable("its sound file " + t->audio + ": " + error);
        ReadOutcome r;
        r.status = ReadOutcome::Status::Read;
        r.info.formatVersion = 1;
        r.info.name = t->name;
        r.info.description = t->description;
        r.info.isTemplate = t->isTemplate;
        r.info.companions = {t->audio};
        r.info.content = std::make_shared<sharp::TapeContent>(std::move(sound));
        return r;
    }

    std::vector<FileOut> write(const Write& w, std::string& problem) const override {
        const auto* tape = dynamic_cast<const sharp::TapeContent*>(w.content.get());
        if (!tape) {
            problem = "It is not a cassette.";
            return {};
        }
        if (!writableName(w.name, "cassette", problem)) return {};
        // D-26: a definition written again keeps its sound file's path; a
        // copy (Name & Save) gets its WAV beside it, named after its file.
        std::string description, audio = w.fileStem + ".wav";
        if (w.source) {
            std::string error;
            if (const auto t = cassetteDefinition(*w.source, &error)) {
                description = t->description;
                if (t->name == w.name) audio = t->audio;
            }
        }
        std::string text = "# Calc-U-1600 cassette\nformat: cassette\nformat-version: 1\n";
        text += "cassette-name: \"" + w.name + "\"\n";
        if (!description.empty()) text += "description: " + yamlQuoted(description) + "\n";
        if (w.asTemplate) text += "template: true\n";
        if (!w.savedAt.empty()) text += "saved: " + w.savedAt + "\n";
        text += "audio: { file: " + yamlQuoted(audio) + " }\nposition: 0\n";
        return {FileOut{"", text}, FileOut{audio, std::string(tape->wav.begin(), tape->wav.end())}};
    }
};


const MemoryCardDefinition* definitionOf(const calcu::machine::PeripheralDescriptor& d) {
    const auto* data = dynamic_cast<const sharp::ModuleDefinitionData*>(d.productData.get());
    return data ? &data->definition : nullptr;
}

} // namespace

std::vector<media::KindDecl> mediaKinds(const std::string& bundledDir, calcu::machine::Catalogue& catalogue) {
    static const FloppyCodec floppyCodec;
    static const CassetteCodec cassetteCodec;
    // One card codec per bundled folder (a test's own, the product's).
    static std::vector<std::unique_ptr<CardCodec>> cardCodecs;
    cardCodecs.push_back(std::make_unique<CardCodec>(bundledDir));

    media::KindDecl module;
    module.id = kModuleKind;
    module.label = "memory module";
    module.suffix = kModuleFileSuffix;
    module.bundledFolder = bundledDir;
    module.codec = cardCodecs.back().get();
    module.formatName = "The module definition format";
    module.describedAt = "the User Guide, \"Module definition format\"";
    module.currentVersion = static_cast<int>(kCardFormatVersion);
    module.heldBy = media::HeldBy::ModuleSlot;
    module.isMedium = [](const calcu::machine::PeripheralDescriptor& d) {
        const MemoryCardDefinition* def = definitionOf(d);
        return def && def->battery;
    };
    calcu::machine::Catalogue* cat = &catalogue;
    module.moduleRequest = [cat](const media::MediumInfo& info,
                                 const calcu::machine::PlacePath& place) -> std::optional<calcu::machine::PeripheralRequest> {
        const auto* content = dynamic_cast<const sharp::ModuleContent*>(info.content.get());
        if (!content) return std::nullopt;
        const calcu::machine::PeripheralId id{content->definition.moduleName};
        if (!cat->peripheral(id)) cat->addPeripheral(sharp::moduleDescriptor(content->definition));   // a saved instance
        calcu::machine::PeripheralRequest r;
        r.place = place;
        r.peripheral = id;
        // A new object per request: the machine takes the card out of it,
        // and the media part finds the binding by it (S-4).
        r.content = std::make_shared<sharp::ModuleContent>(content->definition);
        return r;
    };

    module.fitsPinOut = [](const media::MediumInfo& info, const calcu::machine::PinOutId& pinOut) {
        const auto* content = dynamic_cast<const sharp::ModuleContent*>(info.content.get());
        if (!content) return false;
        for (CardHost h : content->definition.compatibleHosts) {   // AC-41's table, as moduleDescriptor()
            const char* p = h == CardHost::PC1600Slot1   ? sharp::kPC1600Slot1PinOut
                            : h == CardHost::PC1600Slot2 ? sharp::kPC1600Slot2PinOut
                                                         : sharp::kPC1500SlotPinOut;
            if (pinOut.value == p) return true;
        }
        return false;
    };

    media::KindDecl floppy;
    floppy.id = kFloppyKind;
    floppy.label = "floppy disk";
    floppy.suffix = kFloppyFileSuffix;
    floppy.bundledFolder = bundledDir;
    floppy.codec = &floppyCodec;
    floppy.formatName = "The floppy disk format";
    floppy.describedAt = "the User Guide, \"Media files\", \"Floppy disk\" (sides 0: and 1: in place of a: and b:)";
    floppy.currentVersion = static_cast<int>(kFloppyFormatVersion);
    floppy.words = media::PresetWords{"floppy", "", "floppy", "", false};

    media::KindDecl cassette;
    cassette.id = kCassetteKind;
    cassette.label = "cassette";
    cassette.suffix = kCassetteFileSuffix;
    cassette.companionSuffixes = {".wav"};   // its sound, beside it (LIB-7 AC-43)
    cassette.folder = calcu::settings::Folder::Cassettes;
    cassette.bundledFolder = bundledDir;
    cassette.codec = &cassetteCodec;
    cassette.formatName = "The cassette definition format";
    cassette.describedAt = "the User Guide, \"Media files\", \"Cassette definition\"";
    cassette.currentVersion = 1;
    cassette.words = media::PresetWords{"cassette", "", "cassette", "", false};

    return {module, floppy, cassette};
}

media::ModuleListing moduleListing() {
    media::ModuleListing l;
    l.label = [](const calcu::machine::PeripheralDescriptor& d) { return d.name.empty() ? d.id.value : d.name; };
    l.tooltip = [](const calcu::machine::PeripheralDescriptor& d) -> std::optional<std::string> {
        const MemoryCardDefinition* def = definitionOf(d);
        if (!def || def->description.empty()) return std::nullopt;
        return def->description;
    };
    l.sortKey = [](const calcu::machine::PeripheralDescriptor&) { return 0; };   // the catalogue's order
    return l;
}

} // namespace cu1600
