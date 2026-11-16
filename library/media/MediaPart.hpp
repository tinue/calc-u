#pragma once
#include "machine/Catalogue.hpp"
#include "machine/Machine.hpp"
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
#include <vector>

// The media part (LIB-7 design §3.7; spec AC-1, AC-7 to AC-20, AC-27, AC-30,
// AC-31, EC-1 to EC-9): the topic `media`, the commands of §3.6, autosave
// (rules A1 to A9), the writes that do not wait, the notices and the log
// lines, and the wait probe of LIB-3 AC-22. One per app session; it goes
// after StartupPart and ControlPart among the library's parts.
//
// A place is named by its place word (§3.3): a module slot by its place name
// (`slot-1`), a media place by its kind's eject word when the machine has
// one place of that kind, else by its MediaPlaceId.

namespace calcu::media {

struct MediaSetup {
    std::vector<KindDecl> kinds;
    const settings::Paths* paths = nullptr;          // the save folder and the tape folder (LIB-9)
    ModuleListing listing;
    const machine::Catalogue* catalogue = nullptr;   // module slots: what fits, the modules that are not media
    // LIB-18's machine control (module slots, the power cycle of AC-19, the
    // variant carry of S-7a).
    startup::MachineControl* control = nullptr;
};

// What the part keeps per place (§3.7): where the medium came from, whether
// it is a template, the file it is saved to, the revision last written, the
// time a change was first seen and whether the last write failed. For a
// pass-through place the binding is the fitted medium (glossary: Fit, Arm):
// it stays after each pass and is fed again by a side (C59-7 AC-12).
struct Binding {
    const KindDecl* kind = nullptr;
    std::string name, description;
    Origin origin = Origin::Saved;
    bool isTemplate = true;
    std::string path;                       // an instance: its file; a template: the file it came from, if any
    std::optional<std::string> source;      // the bytes of that file (the codec's splice)
    std::string inFile;                     // an in-file medium: its name (AC-22)
    bool hasWriteProtect = false, writeProtected = false;
    std::optional<int> side;
    std::string peripheral;                 // a module slot: the module it is
    Content content;                        // a pass-through place: the fitted medium as it is now (what it gave back last)
    bool passed = false;                    // a pass-through place: it has passed since it was fitted (AC-27)
    std::uint64_t written = 0;              // the revision last written (or seen, for a template)
    bool pending = false;                   // A1: a change seen, not yet written
    pacing::WallTime dated{0};
    bool failed = false;                    // A8: one notice until a write succeeds
    bool unsaved = false;                   // a template written to (AC-15), a started name not yet written (AC-20)
    bool started = false;                   // AC-20: its file is replaced at the first write
    bool instance() const { return !isTemplate; }
};

// The in-file media of one preset (AC-22, D-10): defined by a product's
// header key when the preset is read; put into the part's store at their
// first use in that load, kept until the next load or model switch.
struct InFileDef {
    std::string kind, name;
    Content content;
};
struct InFileDefs {
    InFileDefs();
    std::uint64_t serial;   // unique per read
    std::vector<InFileDef> items;
};

class MediaPart final : public sessions::SessionPart, public sessions::WaitProbe {
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
    bool waitsForMedium(const machine::Machine&) const override;   // LIB-3 AC-22; LIB-17 AC-16

    const Shelf& shelf() const { return shelf_; }
    void refreshShelf(sessions::SessionContext*);   // AC-7: read again (log lines at the first scan)
    const std::vector<KindDecl>& kinds() const { return shelf_.kinds(); }
    const KindDecl* kind(std::string_view id) const { return shelf_.kind(id); }

    // ── For PresetMedia and the harness (§3.8, §3.11) ──
    // The place word of the first place of `kind` on the machine, or nullopt.
    std::optional<std::string> placeOfKind(sessions::SessionContext&, const KindDecl&);
    bool hasPlace(sessions::SessionContext&, const std::string& word);
    const Binding* bindingOf(const std::string& word) const;
    // A medium in at once (holding; the place's old medium written first and
    // taken out, AC-13, EC-5) or fitted and fed by `side` (pass-through). The
    // refusal, or nullopt.
    std::optional<std::string> insert(sessions::SessionContext&, const std::string& word, Binding, Content, int side);
    // AC-13 then out; an empty place: nothing (EC-6). A pass-through place:
    // a medium passing is taken out and stays fitted (C59-7 AC-10, AC-25);
    // none passing: the fitted medium goes.
    void eject(sessions::SessionContext&, const std::string& word);
    // A pass-through place: the medium fitted (glossary: Arm), nothing
    // inserted; it takes the place of the one fitted before. Refused while a
    // medium passes (C59-7 AC-16). The refusal, or nullopt.
    std::optional<std::string> arm(sessions::SessionContext&, const std::string& word, Binding, Content);
    // A pass-through place: the fitted medium inserted by `side`; it stays
    // fitted (C59-7 AC-12). Refused while one passes. The refusal, or nullopt.
    std::optional<std::string> feed(sessions::SessionContext&, const std::string& word, int side);
    // A pass-through place: the medium it gave back, if one came since the
    // last ask (A3 done). For a step that waits for its card.
    bool takePassed(sessions::SessionContext&, const std::string& word);
    bool passing(sessions::SessionContext&, const std::string& word);
    // A5 for every binding, or one place.
    void flushAll(sessions::SessionContext&);
    void flush(sessions::SessionContext&, const std::string& word);

    struct SaveResult {
        bool ok = false;
        std::string reason;      // refused or failed; "asks: …" for Name & Save (D-6)
        bool powerCycle = false; // AC-19: the protect switch was set; the caller runs the power cycle
    };
    // AC-17 to AC-19: the medium at `word` (or the in-file medium `inFile`
    // of `kind`) saved by the request; with `protect` the switch is set.
    SaveResult save(sessions::SessionContext&, const std::string& word, SaveRequest, bool protect);
    SaveResult saveInFile(sessions::SessionContext&, const KindDecl&, const std::string& inFile, SaveRequest, bool protect);
    // AC-19's power cycle as an operation: until key readiness.
    sessions::Task<bool> powerCycle(sessions::OperationContext&);

    // In-file media (AC-22, D-10). The set of a read; its media are put into
    // the store at the first use in a load.
    void adoptInFile(sessions::SessionContext&, const InFileDefs&);
    Content inFileContent(const KindDecl&, const std::string& name);   // a name not defined: fresh (AC-23)
    std::optional<std::string> inFileInstance(const KindDecl&, const std::string& name) const;

    // The binding a place key's resolved request carries to after the build
    // (S-4): keyed by the request's content.
    void registerModule(const Content&, Binding);
    Binding bindingFor(const ShelfEntry&) const;   // a medium of the shelf as a binding

    // AC-30, AC-31 for the harness and the commands.
    struct Outcome {
        bool ok = false;
        std::string reason;
        std::string path;
    };
    Outcome importFile(sessions::SessionContext&, const std::string& path);
    Outcome importBytes(sessions::SessionContext&, const std::string& fileName, const std::string& bytes,
                     const MediumCodec::Companion&, bool replaceConfirmed = false);
    // The medium's files, its own first, by file name (D-13).
    std::optional<std::vector<FileOut>> exportFiles(const KindDecl&, const std::string& name, std::string* reason);
    Outcome remove(sessions::SessionContext&, const KindDecl&, const std::string& name, bool confirmed);

    // Opened files, for LIB-13 (§3.7; AC-1, AC-30).
    struct OpenedMedium {
        std::optional<std::string> refusal;
        const KindDecl* kind = nullptr;
        bool isTemplate = false, isModule = false;
    };
    // `path`: where the file lies, for its companions (a tape's sound file
    // beside it); empty for an upload.
    OpenedMedium readOpened(std::string_view name, const std::string& bytes, const std::string& path = {}) const;
    std::vector<std::string> placesFor(const OpenedMedium&, const machine::Machine&) const;
    std::vector<std::string> placesInCatalogue(const OpenedMedium&) const;
    std::optional<std::string> refusesNow(const std::string& place, const machine::Machine&) const;
    // The command `media.open-file` (§3.6), for LIB-13's opener.
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
    MediaPlace* placeCap(machine::Machine&, const Place&) const;
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
    std::optional<std::string> passingNow(machine::Machine&, const Place&) const;   // the refusal while a medium passes
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
    std::vector<std::pair<std::string, Binding>> carried_;   // a variant switch: the media of media places (S-7a)
    bool fromPlan_ = false;
    pacing::WallTime lastCall_{0}, gap_{0}, callStart_{0};
};

} // namespace calcu::media
