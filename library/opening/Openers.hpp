#pragma once
#include "machine/Machine.hpp"
#include "opening/OpenRequest.hpp"
#include "sessions/SessionPart.hpp"
#include <optional>
#include <string>
#include <vector>

// What the command needs of presets and media: two seams that
// LibraryOpeners.cpp implements over their parts, so that presets and media
// do not know opening and the tests can replace both. A refusal an opener
// gives is not yet a notice: the open part sends it.

namespace calcu::opening {

class PresetOpening {   // presets/
public:
    virtual std::vector<std::string> extensions() const = 0;   // ".ti59", ".pc1500a": lower case
    virtual sessions::CommandOutcome loadFile(const std::string& path, sessions::SessionContext&) = 0;   // from its folder
    virtual sessions::CommandOutcome loadContent(const OpenedFile&, sessions::SessionContext&) = 0;      // an upload: from its bytes

protected:
    ~PresetOpening() = default;
};

// What media read and checked first.
struct MediumFile {
    std::optional<std::string> refusal;   // outdated or unreadable, with media's hint
    std::string kind, kindName;           // "test-disk", "test disk"
    bool isTemplate = false;              // opened, never written back
    bool isModule = false;                // goes into a module slot (plugged in as by the slot's picker)
};

// A media place is named by its place word, as media names it (a module
// slot by its place name, a media place by its kind's eject word or ID).
class MediaOpening {   // media/
public:
    virtual std::vector<std::string> suffixes() const = 0;   // ".magnetcard.yaml", ".floppy.yaml": lower case
    virtual MediumFile read(const OpenedFile&) const = 0;
    // Where the medium can go on the machine now: the media places of its
    // kind, for a module the slots it fits.
    virtual std::vector<std::string> placesFor(const MediumFile&, const OpenedFile&, const machine::Machine&) const = 0;
    // Where a medium of the kind would go on any model or peripheral, for
    // the refusal's sentence ("the reader of the test-seg").
    virtual std::vector<std::string> placesInCatalogue(const MediumFile&) const = 0;
    virtual std::optional<std::string> refusesNow(const std::string& place, const machine::Machine&) const = 0;   // a place that refuses now
    // The file is the medium's instance from now on and is written back
    // where it lies, unless it is a template.
    virtual sessions::CommandOutcome useWhereItLies(const OpenedFile&, const std::string& place, sessions::SessionContext&) = 0;
    // An upload: imported into the save folder first; a name already taken
    // and not `confirmed` is media's `asks:` refusal, and nothing changes.
    // With a place, the imported medium is then put there; without one it
    // is only imported (it waits in the save folder for its picker).
    virtual sessions::CommandOutcome importAndUse(const OpenedFile&, const std::optional<std::string>& place,
                                                  bool confirmed, sessions::SessionContext&) = 0;

protected:
    ~MediaOpening() = default;
};

} // namespace calcu::opening
