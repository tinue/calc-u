#include "media/Shelf.hpp"
#include "media/Names.hpp"
#include "settings/AtomicFile.hpp"
#include <algorithm>
#include <filesystem>

namespace calcu::media {

namespace {

// The file names of a folder of the kind's suffix, sorted by their fold.
std::vector<std::string> filesOf(const FileAccess& files, const std::string& folder, const KindDecl& k) {
    std::vector<std::string> out;
    if (folder.empty()) return out;
    for (std::string& name : files.list(folder))
        if (hasSuffix(name, k)) out.push_back(std::move(name));
    std::sort(out.begin(), out.end(), [](const std::string& a, const std::string& b) {
        const std::string fa = foldName(a), fb = foldName(b);
        return fa != fb ? fa < fb : a < b;
    });
    return out;
}

} // namespace

const char* originText(Origin o) {
    switch (o) {
    case Origin::Bundled: return "bundled";
    case Origin::Implicit: return "implicit";
    case Origin::Saved: return "saved";
    case Origin::Opened: return "opened";
    case Origin::InFile: return "in-file";
    }
    return "";
}

std::string folderOfPath(const std::string& path) {
    const std::size_t at = path.find_last_of("/\\");
    return at == std::string::npos ? std::string() : path.substr(0, at);
}

std::string joinFile(const std::string& folder, const std::string& name) {
    if (folder.empty()) return name;
    const char last = folder.back();
    return last == '/' || last == '\\' ? folder + name : folder + "/" + name;
}

MediumCodec::Companion companionsBeside(const std::string& path) {
    const std::string dir = folderOfPath(path);
    return [dir](const std::string& rel) { return settings::readWholeFile(joinFile(dir, rel)); };
}

Shelf::Shelf(const std::vector<KindDecl>& kinds, const settings::Paths* paths, std::shared_ptr<const FileAccess> files)
    : kinds_(kinds), paths_(paths), files_access_(files ? std::move(files) : plainFileAccess()), files_(files_access_.get()) {}

std::string Shelf::folderOf(const KindDecl& k) const {
    if (!paths_) return {};
    return paths_->folder(k.folder).value_or(std::string());
}

std::string Shelf::outdatedHint(const KindDecl& k, int version) {
    return k.formatName + " version " + std::to_string(version) + " is outdated: the current version is " +
           std::to_string(k.currentVersion) + ", described in " + k.describedAt + ".";
}

std::optional<ReadOutcome> Shelf::readFile(const KindDecl& k, const std::string& path, std::string* bytesOut) const {
    const auto bytes = files_->read(path).bytes;
    if (!bytes || !k.codec) return std::nullopt;
    if (bytesOut) *bytesOut = *bytes;
    return k.codec->read(*bytes, companionsBeside(path));
}

std::vector<std::string> Shelf::reread() {
    std::vector<std::string> lines;
    entries_.clear();
    fileNames_.clear();
    for (const KindDecl& k : kinds_) {
        auto entry = [&](const std::string& folder, const std::string& file, Origin origin) {
            const std::string path = joinFile(folder, file);
            std::string bytes;
            const auto r = readFile(k, path, &bytes);
            if (r && r->status == ReadOutcome::Status::NotMedium) return;   // no medium of the kind
            if (!r || r->status == ReadOutcome::Status::Unreadable) {
                lines.push_back("The " + k.label + " file " + file + " cannot be read" +
                                (r && !r->problem.empty() ? ": " + r->problem : std::string(".")));
                return;
            }
            ShelfEntry e;
            e.kind = &k;
            e.fileName = file;
            e.path = path;
            e.origin = origin;
            e.info = r->info;
            e.bytes = std::move(bytes);
            e.name = nameOf(r->info, file, k);
            e.description = r->info.description;
            e.isTemplate = origin == Origin::Bundled || r->info.isTemplate;   // bundled files are templates
            e.hasWriteProtect = r->info.hasWriteProtect;
            e.writeProtected = r->info.writeProtected;
            if (r->status == ReadOutcome::Status::Outdated) {
                e.outdatedHint = outdatedHint(k, r->info.formatVersion);
                lines.push_back("The " + k.label + " file " + file + " is outdated: " + *e.outdatedHint);
            }
            entries_.push_back(std::move(e));
        };
        for (const std::string& f : filesOf(*files_, k.bundledFolder, k)) entry(k.bundledFolder, f, Origin::Bundled);
        for (const ImplicitTemplate& t : k.implicitTemplates) {
            ShelfEntry e;
            e.kind = &k;
            e.name = t.name;
            e.description = t.description;
            e.origin = Origin::Implicit;
            e.isTemplate = true;
            e.info.name = t.name;
            e.info.description = t.description;
            e.info.isTemplate = true;
            e.info.content = t.make ? t.make() : nullptr;
            entries_.push_back(std::move(e));
        }
        const std::string folder = folderOf(k);
        for (const std::string& f : filesOf(*files_, folder, k)) {
            fileNames_.emplace_back(k.id, foldName(f));
            const std::size_t before = entries_.size();
            entry(folder, f, Origin::Saved);
            if (entries_.size() == before) continue;
            const ShelfEntry& e = entries_.back();
            // A saved file with a bundled or implicit template's name is not listed.
            const bool clash = std::any_of(entries_.begin(), entries_.end() - 1, [&](const ShelfEntry& o) {
                return o.kind == &k && (o.origin == Origin::Bundled || o.origin == Origin::Implicit) &&
                       sameName(o.name, e.name);
            });
            if (clash) {
                lines.push_back("The " + k.label + " file " + f + " carries the name of a bundled " + k.label + " “" +
                                e.name + "” and is left out.");
                entries_.pop_back();
            }
        }
    }
    const bool first = !scanned_;
    scanned_ = true;
    return first ? lines : std::vector<std::string>{};   // logged at the first scan only
}

std::vector<const ShelfEntry*> Shelf::entries(const KindDecl& k) const {
    std::vector<const ShelfEntry*> out;
    for (const ShelfEntry& e : entries_)
        if (e.kind->id == k.id) out.push_back(&e);
    return out;
}

const ShelfEntry* Shelf::find(const KindDecl& k, std::string_view name) const {
    const ShelfEntry* firstSaved = nullptr;
    const std::string own = foldName(fileNameFor(std::string(name), k));
    for (const ShelfEntry& e : entries_) {
        if (e.kind->id != k.id || !sameName(e.name, name)) continue;
        if (e.origin == Origin::Bundled || e.origin == Origin::Implicit) return &e;
    }
    for (const ShelfEntry& e : entries_) {
        if (e.kind->id != k.id || !sameName(e.name, name) || e.origin != Origin::Saved) continue;
        if (foldName(e.fileName) == own) return &e;   // the file the name gives
        if (!firstSaved) firstSaved = &e;             // else the first by file name
    }
    return firstSaved;
}

const ShelfEntry* Shelf::byFileName(const KindDecl& k, std::string_view fileName) const {
    for (const ShelfEntry& e : entries_)
        if (e.kind->id == k.id && e.origin == Origin::Saved && foldName(e.fileName) == foldName(fileName)) return &e;
    return nullptr;
}

const ShelfEntry* Shelf::byPath(std::string_view path) const {
    for (const ShelfEntry& e : entries_)
        if (!e.path.empty() && e.path == path) return &e;
    return nullptr;
}

bool Shelf::fileTaken(const KindDecl& k, std::string_view fileName) const {
    const std::string f = foldName(fileName);
    return std::any_of(fileNames_.begin(), fileNames_.end(), [&](const auto& x) { return x.first == k.id && x.second == f; });
}

const KindDecl* Shelf::kindOfFile(std::string_view fileName) const {
    for (const KindDecl& k : kinds_)
        if (hasSuffix(fileName, k)) return &k;
    return nullptr;
}

const KindDecl* Shelf::kind(std::string_view id) const {
    for (const KindDecl& k : kinds_)
        if (k.id == id) return &k;
    return nullptr;
}

} // namespace calcu::media
