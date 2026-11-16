#pragma once
#include "media/FileAccess.hpp"
#include "media/Kind.hpp"
#include <memory>
#include "settings/Paths.hpp"
#include <optional>
#include <string>
#include <string_view>
#include <vector>

// The shelf (LIB-7 design §3.4; spec AC-1, AC-5, AC-7, AC-9, EC-3; D-8,
// D-12): the media of every kind from the bundled folders, the save folder,
// the tape folder and the implicit templates, read through each kind's
// codec. Read again when the part wrote to a folder and when a picker or the
// saved media list is opened; folders are not watched (AC-7).

namespace calcu::media {

enum class Origin { Bundled, Implicit, Saved, Opened, InFile };
const char* originText(Origin);

struct ShelfEntry {
    const KindDecl* kind = nullptr;
    std::string name, description, fileName;   // fileName: in its folder, never a path in a topic (D-17)
    std::string path;                          // for the part only
    Origin origin = Origin::Saved;
    bool isTemplate = false, hasWriteProtect = false, writeProtected = false;
    std::optional<std::string> outdatedHint;   // AC-9: the entry cannot be chosen
    MediumInfo info;                           // Read: what the codec gave
    std::string bytes;                         // the file as read (the codec's splice source)
};

class Shelf {
public:
    // `files`: how the folders are listed and read (D-30); null: the plain access.
    Shelf(const std::vector<KindDecl>&, const settings::Paths*, std::shared_ptr<const FileAccess> files = nullptr);
    const FileAccess& files() const { return *files_; }

    // AC-7: the folders read again. The log lines (EC-3, AC-9, D-8: one per
    // file) come only from the first scan after the session started (D-12).
    std::vector<std::string> reread();
    bool scanned() const { return scanned_; }
    // The kind's entries: bundled, implicit, then saved (outdated included).
    std::vector<const ShelfEntry*> entries(const KindDecl&) const;
    // AC-5: bundled and implicit first, then the save folder. Of two saved
    // files with one name, the one whose file name the name gives, else the
    // first by file name (OQ-1). Outdated entries are found too.
    const ShelfEntry* find(const KindDecl&, std::string_view name) const;
    // A saved entry by its file name in the kind's folder.
    const ShelfEntry* byFileName(const KindDecl&, std::string_view fileName) const;
    const ShelfEntry* byPath(std::string_view path) const;
    // Every file name of the kind's suffix in its folder, also those that
    // cannot be read (AC-4's "a file of that name exists").
    bool fileTaken(const KindDecl&, std::string_view fileName) const;
    // A file read through the kind's codec (a `file:` reference, an opened
    // file); nullopt when it cannot be read at all.
    std::optional<ReadOutcome> readFile(const KindDecl&, const std::string& path, std::string* bytes = nullptr) const;
    // The kind of a file by its suffix, or nullptr.
    const KindDecl* kindOfFile(std::string_view fileName) const;
    const KindDecl* kind(std::string_view id) const;
    const std::vector<KindDecl>& kinds() const { return kinds_; }
    // The kind's folder (save folder or tape folder), "" when there is none.
    std::string folderOf(const KindDecl&) const;
    const settings::Paths* paths() const { return paths_; }
    // AC-9: "<format> version <n> is outdated: …".
    static std::string outdatedHint(const KindDecl&, int version);

private:
    std::vector<KindDecl> kinds_;
    const settings::Paths* paths_;
    std::shared_ptr<const FileAccess> files_access_;
    const FileAccess* files_;
    std::vector<ShelfEntry> entries_;
    std::vector<std::pair<std::string, std::string>> fileNames_;   // kind id, folded file name
    bool scanned_ = false;
};

// The companion reader of a file at `path`: the files beside it.
MediumCodec::Companion companionsBeside(const std::string& path);
// "<dir>/<name>" with "/" (the folder of a path).
std::string folderOfPath(const std::string& path);
std::string joinFile(const std::string& folder, const std::string& name);

} // namespace calcu::media
