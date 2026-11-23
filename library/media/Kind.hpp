#pragma once
#include "machine/Attach.hpp"
#include "machine/Descriptor.hpp"
#include "machine/Ids.hpp"
#include "machine/ProductData.hpp"
#include "settings/Paths.hpp"
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

// Kinds of medium: what a product supplies per kind, behind one seam. The
// library reads no format: all it knows of a file is what the kind's codec
// gives (MediumInfo), and the content it hands to a media place and takes
// back from it unread.

namespace calcu::media {

// The kind's own content: parsed by the product's codec, handed to a media
// place or carried in a PeripheralRequest, never looked into here.
using Content = std::shared_ptr<const machine::ProductData>;

struct MediumInfo {                 // all the library knows of a file
    std::string name;               // the name tag; empty when the file has none
    std::string description;
    bool isTemplate = false;
    int formatVersion = 0;
    bool writeProtected = false;    // the switch is on in the file
    bool hasWriteProtect = false;   // the medium has a switch at all
    std::vector<std::string> companions;   // files it names by a relative path (a tape's sound)
    Content content;
};

struct ReadOutcome {
    // NotMedium: a file of the kind's suffix that is no medium of it (a
    // Calc-U-1600 module definition without a battery: a module that is not
    // a medium, a ROM module for one). The shelf neither lists nor logs it.
    // For a kind held by a module slot, info.content is the module: a
    // preset's `file:` and `open` plug it without a binding, so it is never
    // saved. Every other reader takes it as Unreadable, with the problem.
    enum class Status { Read, Unreadable, NotMedium } status = Status::Unreadable;
    bool unreadable() const { return status == Status::Unreadable || status == Status::NotMedium; }
    MediumInfo info;                // Read; NotMedium: the module, if any
    std::string problem;            // Unreadable: why, one sentence
};

struct FileOut {                    // the medium's file first, then its companions
    std::string relativePath;       // the first: unused (the target); a companion: beside the target
    std::string bytes;
};

class MediumCodec {
public:
    // A companion file the medium names, by its path relative to the
    // medium's file: its bytes, or nullopt.
    using Companion = std::function<std::optional<std::string>(const std::string& relativePath)>;
    virtual ReadOutcome read(const std::string& bytes, const Companion&) const = 0;

    struct Write {
        std::string name;                        // the name tag to write
        std::string fileStem;                    // the file name without the suffix; companions named after it
        bool asTemplate = false;
        std::optional<bool> writeProtected;
        Content content;                         // from the place, or the file's own (import, export)
        std::optional<std::string> source;       // the bytes of the file it came from, for a codec that keeps the rest of it (a module)
        std::string savedAt;                     // ISO-8601 UTC from SessionContext::hostTime()
    };
    // Empty with `problem` set: the content cannot be written in this format.
    virtual std::vector<FileOut> write(const Write&, std::string& problem) const = 0;

protected:
    ~MediumCodec() = default;
};

// A template without a file (the fresh card).
struct ImplicitTemplate {
    std::string name, description;
    std::function<Content()> make;
};

enum class HeldBy { MediaPlace, ModuleSlot };

struct PresetWords {                 // empty: the kind has no such step
    std::string insertStep;          // "card", "floppy", "cassette"
    std::string armStep;             // "arm" (pass-through places only)
    std::string ejectWord;           // "floppy", "cassette": `eject: <word>`, also the `saveas: { place: }` word
    std::string inFileKey;           // "card": `saveas: { card: }`, and `mem:` references
    bool insertNeedsTime = false;    // `card:` waits until the card has passed
};

struct KindDecl {
    std::string id;                  // the descriptor's MediaPlaceDescriptor::mediumKind ("floppy-disk")
    std::string label;               // "floppy disk": notices and the log
    std::string suffix;              // ".floppy.yaml"
    // The suffixes of the files a medium of this kind names beside it (a
    // tape's ".wav"): Import Media… offers them in its file picker and takes
    // them as companions of the media picked with them.
    std::vector<std::string> companionSuffixes;
    settings::Folder folder = settings::Folder::Save;   // Cassettes for a cassette
    std::string bundledFolder;       // as the frontend or harness finds it; empty: none
    const MediumCodec* codec = nullptr;
    std::vector<ImplicitTemplate> implicitTemplates;
    HeldBy heldBy = HeldBy::MediaPlace;
    // HeldBy::ModuleSlot: which catalogue peripherals are templates of this
    // kind (not "modules that are not media"), and the request a
    // medium of this kind becomes for a place.
    std::function<bool(const machine::PeripheralDescriptor&)> isMedium;
    std::function<std::optional<machine::PeripheralRequest>(const MediumInfo&, const machine::PlacePath&)> moduleRequest;
    // HeldBy::ModuleSlot: whether the medium fits a slot of that pin-out
    // (the picker lists the modules that fit the slot). Empty: it fits every
    // slot of its kind.
    std::function<bool(const MediumInfo&, const machine::PinOutId&)> fitsPinOut;
    PresetWords words;
    // Calc-U-59's magnetic card: the part reads the instance's file again
    // just before each write and hands its bytes to the codec as
    // Write::source, so a write keeps what the file holds of the rest (the
    // other side of the card). Default: the bytes last read or written.
    bool rereadBeforeWrite = false;
    // False: no Name & Save is offered for a medium of the kind; the topic
    // never sets `nameAndSave` and the command refuses (the magnetic card).
    bool offersNameAndSave = true;
    // The question of a replace, `{name}` standing for the name (the
    // magnetic card words its own); empty: the library's sentence.
    std::string replaceQuestion;
};

// The `asks:` question of a replace of the medium named `name`.
std::string replaceQuestionFor(const KindDecl&, const std::string& name);

// A file of a kind a module slot holds that is no medium but defines a
// module (ReadOutcome::Status::NotMedium): plugged as that module, without
// a binding, so it is never saved.
inline bool isPlainModule(const KindDecl& k, const ReadOutcome& r) {
    return k.heldBy == HeldBy::ModuleSlot && r.status == ReadOutcome::Status::NotMedium && r.info.content;
}

// For the non-media modules of a module slot: label, tooltip and
// order. Default: the descriptor's name, no tooltip, by name. Calc-U-59: the
// module's `description`, no tooltip, `menu-sort`.
struct ModuleListing {
    std::function<std::string(const machine::PeripheralDescriptor&)> label;
    std::function<std::optional<std::string>(const machine::PeripheralDescriptor&)> tooltip;
    std::function<int(const machine::PeripheralDescriptor&)> sortKey;   // equal keys keep the catalogue's order
    // The places of a model that are module slots although no kind of
    // medium fits them: their picker lists the empty slot and the modules
    // (Calc-U-59's `slot-1`, whose modules are not media).
    std::vector<machine::PlaceName> moduleSlots;   // not `slots`: a Qt macro
};

} // namespace calcu::media
