#include "media/SaveRules.hpp"
#include "media/Names.hpp"

namespace calcu::media {

std::string replaceQuestionFor(const KindDecl& k, const std::string& name) {
    if (k.replaceQuestion.empty()) return "A " + k.label + " named “" + name + "” exists. Replace it?";
    std::string q = k.replaceQuestion;
    for (std::size_t at = q.find("{name}"); at != std::string::npos; at = q.find("{name}", at + name.size()))
        q.replace(at, 6, name);
    return q;
}

namespace {

Plan refused(std::string why) {
    Plan p;
    p.verdict = Plan::Verdict::Refused;
    p.refusal = std::move(why);
    return p;
}

std::string inQuotes(const std::string& s) { return "“" + s + "”"; }

bool isBundledPath(const Shelf& shelf, const KindDecl& k, const std::string& path) {
    for (const ShelfEntry* e : shelf.entries(k))
        if (e->origin == Origin::Bundled && e->path == path) return true;
    return !k.bundledFolder.empty() && folderOfPath(path) == k.bundledFolder;
}

// The file the name gives, or the first free numbered one
// when that file holds another medium, cannot be read or is outdated.
std::string targetFor(const Shelf& shelf, const KindDecl& k, const std::string& name, const ShelfEntry* replacing) {
    const std::string folder = shelf.folderOf(k);
    const std::string own = fileNameFor(name, k);
    if (replacing) return replacing->path;   // a replaced instance keeps its file
    if (!shelf.fileTaken(k, own)) return joinFile(folder, own);
    return joinFile(folder, numberedFileName(name, k, [&](const std::string& f) { return shelf.fileTaken(k, f); }));
}

} // namespace

Plan planSave(const SaveRequest& r, const Shelf& shelf, const InPlace& inPlace) {
    const KindDecl& k = *r.kind;
    const bool live = r.by != SaveBy::SaveasTemplate;
    if (r.byFile) {   // the file: form writes exactly the file it names
        const std::string file = baseName(r.target);
        if (!hasSuffix(file, k)) return refused("The file " + file + " does not end in " + k.suffix + ".");
        if (isBundledPath(shelf, k, r.target)) return refused("The bundled " + k.label + " file " + file + " is never written.");
        std::string bytes;
        if (const auto read = shelf.readFile(k, r.target, &bytes);
            read && read->status == ReadOutcome::Status::Read && read->info.isTemplate && live)
            return refused("The file " + file + " is a template; only a save as a template replaces it.");
        Plan p;
        p.verdict = Plan::Verdict::Write;
        p.path = r.target;
        p.name = stemOf(file, k);
        p.replaces = bytes.size() > 0;
        return p;
    }
    const NameCheck n = checkName(r.target);
    if (!n.refusal.empty()) return refused(n.refusal);   // a name that is refused
    const std::string& name = n.name;
    const ShelfEntry* same = nullptr;
    for (const ShelfEntry* e : shelf.entries(k)) {
        if (!sameName(e->name, name)) continue;
        if (e->origin == Origin::Bundled || e->origin == Origin::Implicit)   // a bundled or implicit template's name
            return refused(inQuotes(name) + " is the name of a bundled " + k.label + ".");
    }
    same = shelf.find(k, name);
    if (same && same->outdatedHint) same = nullptr;   // an outdated file: a numbered file below
    if (same && same->isTemplate) {   // a user template's name
        if (r.by != SaveBy::SaveasTemplate) return refused(inQuotes(name) + " is the name of a template.");
        Plan p;
        p.verdict = Plan::Verdict::Write;
        p.path = same->path;
        p.name = name;
        p.replaces = true;
        return p;
    }
    if (same) {   // an instance
        const std::optional<std::string> at = inPlace ? inPlace(same->path) : std::nullopt;
        if (at && (!r.from || *at != *r.from))   // an instance in another place
            return refused("The " + k.label + " " + inQuotes(same->name) + " is in " + *at + " now.");
        switch (r.by) {   // an instance: asked, replaced or refused
        case SaveBy::SaveasTemplate: return refused(inQuotes(name) + " is the name of an instance.");
        case SaveBy::NameAndSave:
            if (!r.replaceConfirmed) {
                Plan p;
                p.verdict = Plan::Verdict::Ask;
                p.name = name;
                p.path = same->path;
                p.refusal = std::string(kAsks) + replaceQuestionFor(k, same->name);
                return p;
            }
            break;
        case SaveBy::SaveasLive: break;
        }
        Plan p;
        p.verdict = Plan::Verdict::Write;
        p.path = same->path;
        p.name = name;
        p.replaces = true;
        return p;
    }
    Plan p;   // a new file
    p.verdict = Plan::Verdict::Write;
    p.name = name;
    p.path = targetFor(shelf, k, name, nullptr);
    if (shelf.folderOf(k).empty()) return refused("There is no folder to save the " + k.label + " in.");
    return p;
}

ImportPlan planImport(const std::string& fileName, const std::string& bytes, const MediumCodec::Companion& companion,
                      const Shelf& shelf) {
    ImportPlan out;
    const std::string file = baseName(fileName);
    out.kind = shelf.kindOfFile(file);
    if (!out.kind || !out.kind->codec) {
        out.plan = refused("The file " + file + " is not a medium of this product.");
        return out;
    }
    const KindDecl& k = *out.kind;
    const ReadOutcome r = k.codec->read(bytes, companion);
    if (r.unreadable()) {
        out.plan = refused("The " + k.label + " file " + file + " cannot be read" + (r.problem.empty() ? "." : ": " + r.problem));
        return out;
    }
    if (r.status == ReadOutcome::Status::Outdated) {
        out.plan = refused("The " + k.label + " file " + file + " is outdated: " +
                           Shelf::outdatedHint(k, r.info.formatVersion));
        return out;
    }
    out.info = r.info;
    const std::string name = nameOf(r.info, file, k);
    for (const ShelfEntry* e : shelf.entries(k))
        if (sameName(e->name, name)) {
            out.plan = refused("A " + k.label + " named " + inQuotes(e->name) + " exists already.");
            return out;
        }
    if (shelf.folderOf(k).empty()) {
        out.plan = refused("There is no folder to import the " + k.label + " into.");
        return out;
    }
    out.plan.verdict = Plan::Verdict::Write;
    out.plan.name = name;
    out.plan.path = targetFor(shelf, k, name, nullptr);
    return out;
}

Plan planDelete(const KindDecl& k, const std::string& name, bool confirmed, const Shelf& shelf, const InPlace& inPlace) {
    const ShelfEntry* e = nullptr;
    for (const ShelfEntry* x : shelf.entries(k))
        if (x->origin == Origin::Saved && sameName(x->name, name)) {
            e = x;
            if (foldName(x->fileName) == foldName(fileNameFor(name, k))) break;
        }
    if (!e) return refused("The " + k.label + " " + inQuotes(name) + " is no longer there.");
    if (const auto at = inPlace ? inPlace(e->path) : std::nullopt)
        return refused("The " + k.label + " " + inQuotes(e->name) + " is in " + *at + " now.");
    Plan p;
    p.name = e->name;
    p.path = e->path;
    if (!confirmed) {
        p.verdict = Plan::Verdict::Ask;
        p.refusal = std::string(kAsks) + "Delete the " + k.label + " " + inQuotes(e->name) + "?";
        return p;
    }
    p.verdict = Plan::Verdict::Write;
    return p;
}

} // namespace calcu::media
