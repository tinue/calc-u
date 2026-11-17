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

// Kinds of medium (LIB-7 design §3.1; spec AC-46; capabilities.md Q-2): what
// a product supplies per kind, behind one seam. The library reads no format:
// all it knows of a file is what the kind's codec gives (MediumInfo), and the
// content it hands to a media place and takes back from it unread.

namespace calcu::media {

// The kind's own content: parsed by the product's codec, handed to a media
// place or carried in a PeripheralRequest, never looked into here (D-2).
using Content = std::shared_ptr<const machine::ProductData>;

struct MediumInfo {                 // AC-46: all the library knows of a file
    std::string name;               // the name tag; empty when the file has none (AC-2)
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
    // a medium, AC-5, D-9). The shelf neither lists nor logs it; every other
    // reader takes it as Unreadable, with the problem (M-47).
    enum class Status { Read, Outdated, Unreadable, NotMedium } status = Status::Unreadable;
    bool unreadable() const { return status == Status::Unreadable || status == Status::NotMedium; }
    MediumInfo info;                // Read; Outdated: name and version only
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
        std::string fileStem;                    // the file name without the suffix (AC-4); companions named after it
        bool asTemplate = false;
        std::optional<bool> writeProtected;      // AC-19
        Content content;                         // from the place, or the file's own (import, export)
        std::optional<std::string> source;       // the bytes of the file it came from (Calc-U-1600 AC-41's splice)
        std::string savedAt;                     // ISO-8601 UTC from SessionContext::hostTime() (A-7)
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

struct PresetWords {                 // AC-21, AC-26; empty: the kind has no such step
    std::string insertStep;          // "card", "floppy", "tape"
    std::string armStep;             // "arm" (pass-through places only)
    std::string ejectWord;           // "floppy", "tape": `eject: <word>`, also the `saveas: { place: }` word
    std::string inFileKey;           // "card": `saveas: { card: }`, and `mem:` references (AC-22)
    bool insertNeedsTime = false;    // `card:` waits until the card has passed (LIB-6 AC-19, AC-56)
};

struct KindDecl {
    std::string id;                  // the descriptor's MediaPlaceDescriptor::mediumKind ("floppy-disk")
    std::string label;               // "floppy disk": notices and the log (AC-25)
    std::string suffix;              // ".floppy.yaml" (AC-4)
    // The suffixes of the files a medium of this kind names beside it (a
    // tape's ".wav"): Import Media… offers them in its file picker and takes
    // them as companions of the media picked with them (AC-30, AC-43).
    std::vector<std::string> companionSuffixes;
    settings::Folder folder = settings::Folder::Save;   // Tapes for a tape (LIB-9 AC-16)
    std::string bundledFolder;       // as the frontend or harness finds it; empty: none
    const MediumCodec* codec = nullptr;
    std::string formatName, describedAt;   // the hint of AC-9
    int currentVersion = 0;
    std::vector<ImplicitTemplate> implicitTemplates;
    HeldBy heldBy = HeldBy::MediaPlace;
    // HeldBy::ModuleSlot: which catalogue peripherals are templates of this
    // kind (not "modules that are not media", AC-5, D-9), and the request a
    // medium of this kind becomes for a place.
    std::function<bool(const machine::PeripheralDescriptor&)> isMedium;
    std::function<std::optional<machine::PeripheralRequest>(const MediumInfo&, const machine::PlacePath&)> moduleRequest;
    // HeldBy::ModuleSlot: whether the medium fits a slot of that pin-out
    // (AC-8 "the modules that fit the slot", Calc-U-1600 AC-38; LIB-2 AC-2).
    // Empty: it fits every slot of its kind (M-47).
    std::function<bool(const MediumInfo&, const machine::PinOutId&)> fitsPinOut;
    PresetWords words;
    // LIB-7 design §11 (M-55; Calc-U-59's magnetic card). The part reads the
    // instance's file again just before each write and hands its bytes to
    // the codec as Write::source, so a write keeps what the file holds of
    // the rest (C59-7 AC-5, EC-3; D-27). Default: the bytes last read or
    // written.
    bool rereadBeforeWrite = false;
    // False: no Name & Save is offered for a medium of the kind; the topic
    // never sets `nameAndSave` and the command refuses (D-28).
    bool offersNameAndSave = true;
    // The question of a replace (AC-17 row 5, AC-20), `{name}` standing for
    // the name; empty: the library's sentence (D-31).
    std::string replaceQuestion;
};

// The `asks:` question of a replace of the medium named `name` (D-31).
std::string replaceQuestionFor(const KindDecl&, const std::string& name);

// For the non-media modules of a module slot (AC-8): label, tooltip and
// order. Default: the descriptor's name, no tooltip, by name. Calc-U-59: the
// module's `description`, no tooltip, `menu-sort` (C59-6 AC-8).
struct ModuleListing {
    std::function<std::string(const machine::PeripheralDescriptor&)> label;
    std::function<std::optional<std::string>(const machine::PeripheralDescriptor&)> tooltip;
    std::function<int(const machine::PeripheralDescriptor&)> sortKey;   // equal keys keep the catalogue's order
    // The places of a model that are module slots although no kind of
    // medium fits them: their picker lists the empty slot and the modules
    // (AC-8, AC-47; Calc-U-59's `slot-1`, whose modules are not media).
    std::vector<machine::PlaceName> slots;
};

} // namespace calcu::media
