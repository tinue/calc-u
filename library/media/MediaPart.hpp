#pragma once
#include "machine/Catalogue.hpp"
#include "machine/Machine.hpp"
#include "media/FileAccess.hpp"
#include "media/Kind.hpp"
#include "media/MediaPlace.hpp"
#include "media/MediaTopics.hpp"
#include "media/SaveRules.hpp"
#include "media/Shelf.hpp"
#include "sessions/MachineSession.hpp"
#include "sessions/SessionPart.hpp"
#include "settings/Paths.hpp"
#include "startup/MachineControl.hpp"
#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <type_traits>
#include <vector>

// The media part: the topic `media`, its commands (MediaTopics.hpp),
// autosave, the writes that do not wait, the notices and the log lines, and
// the probe whether the machine waits for a medium. One per app session; it
// goes after StartupPart and ControlPart among the library's parts.
//
// Autosave: an instance in a holding place or a module slot is written at
// most half a second after a change the machine made (in virtual time at the
// end of each command); a pass-through place's medium when it is given back;
// a tape when its motor stops (takeWriteNow). Every binding with a change not
// yet written is written at once before a command touches its place, before
// an operation, a machine change, suspending and closing. A template is never
// written: what the machine wrote stays in the place (`unsaved`) until Name &
// Save or until it leaves the place. A failed write gives one notice until a
// write of that medium succeeds; each write gives one log line.
//
// A place is named by its place word: a module slot by its place name
// (`slot-1`), a media place by its kind's eject word when the machine has
// one place of that kind, else by its MediaPlaceId.

namespace calcu::media {

struct MediaSetup {
    std::vector<KindDecl> kinds;
    const settings::Paths* paths = nullptr;          // the save folder and the tape folder
    ModuleListing listing;
    const machine::Catalogue* catalogue = nullptr;   // module slots: what fits, the modules that are not media
    // The machine control (module slots, the power cycle after the protect
    // switch is set, the media carried over a variant switch).
    startup::MachineControl* control = nullptr;
    // How the save folder is listed, read and written; null: plain.
    std::shared_ptr<const FileAccess> files;
};

// What the part keeps per place: where the medium came from, whether
// it is a template, the file it is saved to, the revision last written, the
// time a change was first seen and whether the last write failed. For a
// pass-through place the binding is the fitted medium (glossary: Fit, Arm):
// it stays after each pass and is fed again by a side.
struct Binding {
    const KindDecl* kind = nullptr;
    std::string name, description;
    Origin origin = Origin::Saved;
    bool isTemplate = true;
    std::string path;                       // an instance: its file; a template: the file it came from, if any
    std::optional<std::string> source;      // the bytes of that file (the codec's splice)
    std::string inFile;                     // an in-file medium: its name
    bool hasWriteProtect = false, writeProtected = false;
    std::optional<int> side;
    std::string peripheral;                 // a module slot: the module it is
    Content content;                        // a pass-through place: the fitted medium as it is now (what it gave back last)
    bool passed = false;                    // a pass-through place: it has passed since it was fitted
    std::uint64_t armed = 0;                // a pass-through place: changes with each fit
    std::uint64_t written = 0;              // the revision last written (or seen, for a template)
    bool pending = false;                   // a change seen, not yet written
    pacing::WallTime dated{0};
    bool failed = false;                    // one notice until a write succeeds
    bool unsaved = false;                   // a template written to, a started name not yet written
    bool started = false;                   // its file is replaced at the first write
    bool instance() const { return !isTemplate; }
};

// The in-file media of one preset: defined by a product's
// header key when the preset is read; put into the part's store at their
// first use in that load, kept until the next load or model switch (Reset,
// a power cycle and a Cold boot keep them).
struct InFileDef {
    std::string kind, name;
    Content content;
};
struct InFileDefs {
    InFileDefs();
    std::uint64_t serial;   // unique per read
    std::vector<InFileDef> items;
};

class MediaPart final : public sessions::SessionPart {
public:
    explicit MediaPart(MediaSetup);

    void attach(sessions::SessionSetup&) override;
    void afterCall(sessions::SessionContext&, pacing::WallTime) override;
    void beforeCall(sessions::SessionContext&, pacing::WallTime) override;
    void beforeFlush(sessions::SessionContext&) override;
    void beforeOperation(sessions::SessionContext&) override;
    void beforeMachineChange(sessions::SessionContext&) override;
    void machineChanged(sessions::SessionContext&) override;
    void operationEnded(sessions::SessionContext&) override;
    void suspending(sessions::SessionContext&) override;
    void closing(sessions::SessionContext&) override;

    const Shelf& shelf() const { return shelf_; }
    void refreshShelf(sessions::SessionContext*);   // read again (log lines at the first scan)
    const std::vector<KindDecl>& kinds() const { return shelf_.kinds(); }
    const KindDecl* kind(std::string_view id) const { return shelf_.kind(id); }

    // ── For PresetMedia and the harness ──
    // The place word of the first place of `kind` on the machine, or nullopt.
    std::optional<std::string> placeOfKind(sessions::SessionContext&, const KindDecl&);
    bool hasPlace(sessions::SessionContext&, const std::string& word);
    const Binding* bindingOf(const std::string& word) const;
    // A medium in at once (holding; the place's old medium written first and
    // taken out) or fitted and fed by `side` (pass-through). The
    // refusal, or nullopt.
    std::optional<std::string> insert(sessions::SessionContext&, const std::string& word, Binding, Content, int side);
    // Written first, then out; an empty place: nothing. A pass-through place:
    // a medium passing is taken out and stays fitted; none passing: the
    // fitted medium goes.
    void eject(sessions::SessionContext&, const std::string& word);
    // A pass-through place: the medium fitted (glossary: Arm), nothing
    // inserted; it takes the place of the one fitted before. Refused while a
    // medium passes. The refusal, or nullopt.
    std::optional<std::string> arm(sessions::SessionContext&, const std::string& word, Binding, Content);
    // A pass-through place: the fitted medium inserted by `side`; it stays
    // fitted. Refused while one passes. The refusal, or nullopt.
    std::optional<std::string> feed(sessions::SessionContext&, const std::string& word, int side);
    // A pass-through place: the medium it gave back, if one came since the
    // last ask (and written). For a step that waits for its card.
    bool takePassed(sessions::SessionContext&, const std::string& word);
    bool passing(sessions::SessionContext&, const std::string& word);
    // The writes that do not wait, for every binding or one place.
    void flushAll(sessions::SessionContext&);
    void flush(sessions::SessionContext&, const std::string& word);

    struct SaveResult {
        bool ok = false;
        std::string reason;      // refused or failed; "asks: …" for Name & Save
        bool powerCycle = false; // the protect switch was set; the caller runs the power cycle
    };
    // The medium at `word` (or the in-file medium `inFile`
    // of `kind`) saved by the request; with `protect` the switch is set.
    SaveResult save(sessions::SessionContext&, const std::string& word, SaveRequest, bool protect);
    SaveResult saveInFile(sessions::SessionContext&, const KindDecl&, const std::string& inFile, SaveRequest, bool protect);
    // The power cycle after setting the protect switch, as an operation:
    // until key readiness.
    sessions::Task<bool> powerCycle(sessions::OperationContext&);

    // In-file media. The set of a read; its media are put into
    // the store at the first use in a load.
    void adoptInFile(sessions::SessionContext&, const InFileDefs&);
    Content inFileContent(const KindDecl&, const std::string& name);   // a name not defined: fresh
    std::optional<std::string> inFileInstance(const KindDecl&, const std::string& name) const;

    // The binding a place key's resolved request carries to after the build:
    // keyed by the request's content.
    void registerModule(const Content&, Binding);
    Binding bindingFor(const ShelfEntry&) const;   // a medium of the shelf as a binding

    // Import, export and delete, for the harness and the commands.
    struct Outcome {
        bool ok = false;
        std::string reason;
        std::string path;
    };
    Outcome importFile(sessions::SessionContext&, const std::string& path);
    Outcome importBytes(sessions::SessionContext&, const std::string& fileName, const std::string& bytes,
                     const MediumCodec::Companion&, bool replaceConfirmed = false);
    // The medium's files, its own first, by file name.
    std::optional<std::vector<FileOut>> exportFiles(const KindDecl&, const std::string& name, std::string* reason);
    Outcome remove(sessions::SessionContext&, const KindDecl&, const std::string& name, bool confirmed);

    // Opened files, for the opening area (library/opening/).
    struct OpenedMedium {
        std::optional<std::string> refusal;
        const KindDecl* kind = nullptr;
        MediumInfo info;            // as read, with the companions beside the file
        bool isModule = false;
        bool plainModule = false;   // a module file that is no medium: plugged, never saved
    };
    // `path`: where the file lies, for its companions (a tape's sound file
    // beside it); empty for an upload.
    OpenedMedium readOpened(std::string_view name, const std::string& bytes, const std::string& path = {}) const;
    std::vector<std::string> placesFor(const OpenedMedium&, const machine::Machine&) const;
    std::vector<std::string> placesInCatalogue(const OpenedMedium&) const;
    std::optional<std::string> refusesNow(const std::string& place, const machine::Machine&) const;
    // The command `media.open-file`, for the open part.
    sessions::CommandOutcome openFile(const sessions::Value&, sessions::SessionContext&);

    // The topic as it would be sent now (tests, the harness's `media`).
    std::shared_ptr<const MediaState> state(sessions::SessionContext&);

private:
    struct Place {
        std::string word;
        const KindDecl* kind = nullptr;
        bool slot = false;
        machine::PlacePath path;        // a slot: the slot; a media place: its owner's place
        machine::MediaPlaceId id;       // a media place
        bool passThrough = false;
        std::vector<int> sides;
    };
    struct InFileItem {
        std::string kind, name;
        Content content;
        std::optional<std::string> instance;   // saved `as: live`: its file
        std::string instanceName;
    };

    void sync(sessions::SessionContext&);
    void newMachine(sessions::SessionContext&);
    void computePlaces(const machine::Machine&);
    const Place* place(const std::string& word) const;
    // M is Machine or const Machine: MediaPlace* or const MediaPlace*.
    template <class M> using PlaceCap = std::conditional_t<std::is_const_v<M>, const MediaPlace, MediaPlace>;
    template <class M> static PlaceCap<M>* placeCap(M&, const Place&);
    ModuleMedium* moduleCap(machine::Machine&, const Place&) const;
    std::optional<machine::PeripheralId> pluggedAt(const machine::Machine&, const Place&) const;
    std::uint64_t revisionOf(machine::Machine&, const Place&) const;
    Content contentOf(machine::Machine&, const Place&, const Binding&) const;
    void bindSlots(sessions::SessionContext&);
    void autosave(sessions::SessionContext&, bool atOnce, pacing::WallTime now);
    void givenBack(sessions::SessionContext&, const Place&, Binding&);
    bool write(sessions::SessionContext&, const Place*, Binding&, Content, std::uint64_t revision);
    void changed(sessions::SessionContext&, const Place&, Binding&, std::uint64_t revision, Content);
    void updateTopic(sessions::SessionContext&);
    std::vector<PickerEntry> picker(sessions::SessionContext&, const Place&) const;
    std::optional<std::string> inPlace(const std::string& path) const;
    std::optional<std::string> passingNow(const machine::Machine&, const Place&) const;   // the refusal while a medium passes
    // Command helpers.
    sessions::CommandOutcome choose(sessions::SessionContext&, const std::string& word, const std::string& entry);
    sessions::CommandOutcome changeSlot(sessions::SessionContext&, const Place&, std::optional<machine::PeripheralRequest>,
                                        std::optional<Binding>);
    struct Resolved {
        std::optional<std::string> refusal;
        Binding binding;
        Content content;
        std::optional<machine::PeripheralRequest> module;   // a slot: the request
    };
    Resolved resolveEntry(const Place&, const std::string& entry) const;

    Shelf shelf_;
    ModuleListing listing_;
    const machine::Catalogue* catalogue_;
    startup::MachineControl* control_ = nullptr;
    sessions::StateTopic* topic_ = nullptr;
    std::vector<Place> places_;
    std::map<std::string, Binding> bindings_;
    std::map<std::string, Binding> pendingSlot_;   // media.choose: bound when the change has run
    std::vector<std::pair<std::weak_ptr<const machine::ProductData>, Binding>> registry_;
    std::vector<InFileItem> inFile_;
    std::uint64_t inFileSerial_ = 0;
    std::uint64_t generation_ = ~std::uint64_t{0};
    std::map<std::string, bool> passed_;   // a pass-through place gave back since the last takePassed
    std::vector<std::pair<std::string, Binding>> carried_;   // a variant switch: the media of media places
    bool fromPlan_ = false;
    std::uint64_t armSerial_ = 0;
    pacing::WallTime lastCall_{0}, gap_{0}, callStart_{0};
};

} // namespace calcu::media
