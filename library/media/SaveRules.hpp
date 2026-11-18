#pragma once
#include "media/Kind.hpp"
#include "media/Shelf.hpp"
#include <functional>
#include <optional>
#include <string>

// The save rules (docs/formats/media.md): one set for Name & Save, `saveas`
// and the harness; the import and the delete of the saved media.

namespace calcu::media {

enum class SaveBy { NameAndSave, SaveasLive, SaveasTemplate };

struct SaveRequest {
    const KindDecl* kind = nullptr;
    SaveBy by = SaveBy::NameAndSave;
    std::string target;                   // the name, or the file path (`file:`)
    bool byFile = false;
    bool replaceConfirmed = false;        // Name & Save: the user was asked
    std::optional<std::string> from;      // the place word whose medium is saved
};

struct Plan {
    enum class Verdict { Write, Refused, Ask } verdict = Verdict::Refused;
    std::string path, name, refusal;      // Ask: the question
    bool replaces = false;                // an instance or a user template is replaced
};

// The place word that holds the medium of the file at `path` now, if any.
using InPlace = std::function<std::optional<std::string>(const std::string& path)>;

Plan planSave(const SaveRequest&, const Shelf&, const InPlace&);

// A file imported by its file name and bytes; the plan's path is in
// the kind's folder.
struct ImportPlan {
    Plan plan;
    const KindDecl* kind = nullptr;
    MediumInfo info;
};
ImportPlan planImport(const std::string& fileName, const std::string& bytes, const MediumCodec::Companion&, const Shelf&);

// The shelf is read again first by the caller. Write deletes.
Plan planDelete(const KindDecl&, const std::string& name, bool confirmed, const Shelf&, const InPlace&);

// The sentence a refusal starts with when the frontend asks the user; the
// frontend asks and sends the same command again with `confirmed: true`.
inline constexpr const char* kAsks = "asks: ";

} // namespace calcu::media
