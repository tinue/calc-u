#include "TestKinds.hpp"
#include <cstdlib>
#include <map>
#include <memory>

namespace calcu::media::test {

namespace {

std::map<std::string, std::string> fields(const std::string& bytes, bool* ok) {
    std::map<std::string, std::string> out;
    *ok = true;
    std::size_t at = 0;
    while (at < bytes.size()) {
        std::size_t end = bytes.find('\n', at);
        if (end == std::string::npos) end = bytes.size();
        const std::string line = bytes.substr(at, end - at);
        at = end + 1;
        if (line.empty()) continue;
        const std::size_t colon = line.find(": ");
        if (colon == std::string::npos) {
            *ok = false;
            return out;
        }
        out[line.substr(0, colon)] = line.substr(colon + 2);
    }
    return out;
}

class TestCodec final : public MediumCodec {
public:
    TestCodec(std::string format, bool module) : format_(std::move(format)), module_(module) {}

    ReadOutcome read(const std::string& bytes, const Companion& companion) const override {
        ReadOutcome r;
        bool ok = true;
        auto f = fields(bytes, &ok);
        if (!ok || f["format"] != format_) {
            r.problem = "It is not a " + format_ + " file.";
            return r;
        }
        r.info.formatVersion = std::atoi(f["format-version"].c_str());
        r.info.name = f["name"];
        if (f["medium"] == "no") {   // a file of the suffix that is no medium of the kind
            r.status = ReadOutcome::Status::NotMedium;
            return r;
        }
        if (r.info.formatVersion != 2) {
            r.problem = "Its version is not known.";
            return r;
        }
        if (!f.count("content")) {
            r.problem = "It has no content.";
            return r;
        }
        std::string content = f["content"];
        if (f.count("companion")) {
            const auto c = companion ? companion(f["companion"]) : std::nullopt;
            if (!c) {
                r.problem = "Its companion file " + f["companion"] + " is missing.";
                return r;
            }
            content += "+" + *c;
            r.info.companions = {f["companion"]};
        }
        r.status = ReadOutcome::Status::Read;
        r.info.description = f["description"];
        r.info.isTemplate = f["template"] == "true";
        r.info.hasWriteProtect = module_;
        r.info.writeProtected = f["write-protect"] == "true";
        r.info.content = contentFrom(content);
        return r;
    }

    std::vector<FileOut> write(const Write& w, std::string& problem) const override {
        const std::string content = contentOf(w.content);
        if (content.find('\n') != std::string::npos) {
            problem = "A test medium holds no line break.";
            return {};
        }
        TestFile t;
        t.format = format_;
        t.name = w.name;
        t.isTemplate = w.asTemplate;
        if (module_) t.writeProtect = w.writeProtected.value_or(false);
        if (w.source) {   // the description is kept (as Calc-U-1600's codecs do)
            bool ok = true;
            auto f = fields(*w.source, &ok);
            if (ok) t.description = f["description"];
        }
        const std::size_t plus = content.find('+');
        t.content = content.substr(0, plus);
        std::vector<FileOut> out{FileOut{"", ""}};
        if (plus != std::string::npos) {
            t.companion = w.fileStem + ".dat";
            out.push_back(FileOut{t.companion, content.substr(plus + 1)});
        }
        out.front().bytes = t.text() + "saved: " + w.savedAt + "\n";
        return out;
    }

private:
    std::string format_;
    bool module_;
};

const TestCodec& codec(const std::string& f) {
    static const TestCodec card("test-card", false), disk("test-disk", false), module("test-module", true);
    return f == "test-card" ? card : f == "test-disk" ? disk : module;
}

} // namespace

std::string TestFile::text() const {
    std::string out = "format: " + format + "\nformat-version: " + std::to_string(version) + "\n";
    if (!name.empty()) out += "name: " + name + "\n";
    if (!description.empty()) out += "description: " + description + "\n";
    if (isTemplate) out += "template: true\n";
    if (writeProtect) out += std::string("write-protect: ") + (*writeProtect ? "true" : "false") + "\n";
    if (!companion.empty()) out += "companion: " + companion + "\n";
    out += "content: " + content + "\n";
    return out;
}

std::string contentOf(const Content& c) {
    const auto* t = dynamic_cast<const mt::TestMediumContent*>(c.get());
    return t ? t->bytes : std::string();
}

Content contentFrom(std::string bytes) { return std::make_shared<mt::TestMediumContent>(std::move(bytes)); }

std::vector<KindDecl> testKinds(const TestKindFolders& folders) {
    KindDecl card;
    card.id = "test-card";
    card.label = "test card";
    card.suffix = ".testcard.yaml";
    card.bundledFolder = folders.cards;
    card.codec = &codec("test-card");
    card.implicitTemplates = {ImplicitTemplate{"Fresh", "A fresh test card", [] { return contentFrom(""); }}};
    card.words = PresetWords{"test-card", "test-arm", "", "test-card", true};

    KindDecl disk;
    disk.id = "test-disk";
    disk.label = "test disk";
    disk.suffix = ".testdisk.yaml";
    disk.companionSuffixes = {".dat"};   // its companion files (Import Media… takes them along)
    disk.bundledFolder = folders.disks;
    disk.codec = &codec("test-disk");
    disk.words = PresetWords{"test-disk", "", "disk", "", false};

    KindDecl module;
    module.id = "test-module";
    module.label = "test module";
    module.suffix = ".testmodule.yaml";
    module.bundledFolder = folders.modules;
    module.codec = &codec("test-module");
    module.heldBy = HeldBy::ModuleSlot;
    module.isMedium = [](const machine::PeripheralDescriptor& d) { return d.id == mt::kMemoryModule; };
    module.moduleRequest = [](const MediumInfo& info, const machine::PlacePath& at) -> std::optional<machine::PeripheralRequest> {
        machine::PeripheralRequest r;
        r.place = at;
        r.peripheral = mt::kMemoryModule;
        r.content = info.content;
        r.switches = {machine::SwitchPosition{kWriteProtect, info.writeProtected}};
        return r;
    };
    return {card, disk, module};
}

} // namespace calcu::media::test
