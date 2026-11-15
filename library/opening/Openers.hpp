#pragma once
#include "machine/Machine.hpp"
#include "opening/OpenRequest.hpp"
#include "sessions/SessionPart.hpp"
#include <optional>
#include <string>
#include <vector>

// What the command needs of LIB-6 and LIB-7 (LIB-13 design §3.4): two seams
// that LibraryOpeners.cpp implements over their parts, so that LIB-6 and
// LIB-7 do not know this feature and the tests can replace both. A refusal
// an opener gives is not yet a notice: the open part sends it (AC-1).

namespace calcu::opening {

class PresetOpening {   // LIB-6
public:
    virtual std::vector<std::string> extensions() const = 0;   // ".ti59", ".pc1500a": lower case
    virtual sessions::CommandOutcome loadFile(const std::string& path, sessions::SessionContext&) = 0;   // from its folder
    virtual sessions::CommandOutcome loadContent(const OpenedFile&, sessions::SessionContext&) = 0;      // LIB-6 EC-5

protected:
    ~PresetOpening() = default;
};

// What LIB-7 read and checked first (AC-4).
struct MediumFile {
    std::optional<std::string> refusal;   // outdated or unreadable, with LIB-7 AC-9's hint
    std::string kind, kindName;           // "test-disk", "test disk"
    bool isTemplate = false;              // LIB-7 AC-25
    bool isModule = false;                // goes into a module slot (plugged in as by the slot's picker)
};

// A media place is named by its place word, as LIB-7 names it (a module
// slot by its place name, a media place by its kind's eject word or ID).
class MediaOpening {   // LIB-7
public:
    virtual std::vector<std::string> suffixes() const = 0;   // ".magnetcard.yaml", ".floppy.yaml": lower case
    virtual MediumFile read(const OpenedFile&) const = 0;
    // Where the medium can go on the machine now: the media places of its
    // kind, for a module the slots it fits.
    virtual std::vector<std::string> placesFor(const MediumFile&, const OpenedFile&, const machine::Machine&) const = 0;
    // Where a medium of the kind would go on any model or peripheral, for
    // the refusal's sentence ("the reader of the test-seg").
    virtual std::vector<std::string> placesInCatalogue(const MediumFile&) const = 0;
    virtual std::optional<std::string> refusesNow(const std::string& place, const machine::Machine&) const = 0;   // EC-4
    // The file is the medium's instance from now on and is written back
    // where it lies, unless it is a template (LIB-7 AC-1, AC-11, AC-12, AC-25).
    virtual sessions::CommandOutcome useWhereItLies(const OpenedFile&, const std::string& place, sessions::SessionContext&) = 0;
    // An upload (LIB-7 AC-30, AC-17): imported into the save folder first;
    // a name already taken and not `confirmed` is LIB-7's `asks:` refusal,
    // and nothing changes. With a place, the imported medium is then put
    // there; without one it is only imported (D-8).
    virtual sessions::CommandOutcome importAndUse(const OpenedFile&, const std::optional<std::string>& place,
                                                  bool confirmed, sessions::SessionContext&) = 0;

protected:
    ~MediaOpening() = default;
};

} // namespace calcu::opening
