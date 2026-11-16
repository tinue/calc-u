# LIB-7 Media and save folder: design
Status: Library column approved at the Part C1/C2 boundary (2026-11-15); Calc-U-1600 column approved at the Part C2/D boundary (2026-11-16); Calc-U-59 column (§11): approved by the user at the Part D/E boundary (2026-11-16)

## 1. Scope

| Column | Step | Tests from (plan §3) | Done in (plan §5.1) |
|---|---|---|---|
| Library | M-41; the drawn pickers and dialogs with M-43 (D-7) | AC-1–31, AC-46; EC-1–9 | M-41 by the plan; proposed M-43 (D-7) |
| Calc-U-1600 | M-47 | AC-37–45 | M-47 |
| Calc-U-59 | M-55 (with C59-7) | AC-32–36, AC-47 | M-55 |

- M-41 changes only `library/`. No product code calls the new code
  before M-47 (Calc-U-1600) and M-55 (Calc-U-59). Its exit check is the
  library tests on the test machines of LIB-1 with kinds of medium built
  for the tests.
- M-41 waits for M-37 (LIB-4 L: the control state with its media place
  and place entries, §3.9) and M-38 (LIB-6 L: the step place and the
  place keys, §3.8). It also uses M-35 (LIB-18 L: `MachineControl::check`
  and `change`, the build, §3.10) and M-39 (LIB-8 L: the component sets
  `library/qt6/` and `library/web/components/`, D-7); the plan's "Waits
  for" should name both (report).
- The criteria that name a later feature pass in that feature's step
  (plan §1): AC-29 (the File menu items of LIB-14 AC-35) in M-43. AC-1's
  file opened from elsewhere is tested in M-41 through the command and
  the reads that LIB-13 (M-42) uses (§3.6, D-14).
- Clauses of earlier designs that pass here: LIB-1 D-1 (the conformance
  check of one media place capability per media place, §2); LIB-11
  AC-8 (bundled media in a harness run) and AC-22 (the real mapping
  commands `floppy`, `card`, `saveas`: in M-41 with the test kinds'
  steps and `saveas`; the products' words with their columns).
- Not here: the formats of the kinds (C59-7 for the magnetic card; the
  Calc-U-1600 column for `.card.yaml`, `.floppy.yaml`, `.tape.yaml`);
  where the folders are (LIB-9); the persistent file system (LIB-15);
  fitting and the cue cards (C59-7); what a module change does to the
  machine (LIB-18); where the pickers and menu items stand (LIB-14).

## 2. Modules and files

### Library (M-41): `library/media/`, namespace `calcu::media`

The folder LIB-1 §2a expects (`library/media/`). It includes `machine`,
`formats`, `settings` (Paths, `writeFileAtomically`), `sessions`,
`peripherals` (the controls kind, for the write-protect switch),
`control` (LIB-4), `presets` (LIB-6) and `startup` (LIB-18). `harness`
includes `media` (it may include any area but `web`); `media` does not
include `harness`: its harness commands are in `library/harness/`, as
every C2 feature's are. The line for
`media` in LIB-1 §2a's dependency rule is added in M-41 (report). Its
tests are compiled into `calcu_library_tests` (Frequent).

| File | Holds |
|---|---|
| `Kind.hpp` | `Content`, `MediumInfo`, `ReadOutcome`, `FileOut`, `MediumCodec`, `ImplicitTemplate`, `PresetWords`, `HeldBy`, `KindDecl` (§3.1; AC-46) |
| `Names.hpp/.cpp` | `checkName`, `sameName`, `fileNameFor`, `numberedFileName` (§3.2; AC-2–4) |
| `MediaPlace.hpp` | the generic capability kinds `MediaPlace` and `ModuleMedium` (§3.3; AC-6) |
| `Shelf.hpp/.cpp` | `Shelf`: the media of every kind from the bundled folders, the save folder, the tape folder and the implicit templates; lookup by name and kind (§3.4; AC-1, AC-5, AC-9, EC-3) |
| `SaveRules.hpp/.cpp` | `planSave` (the table of AC-17), `planImport` (AC-30), `planDelete` (AC-31, EC-4) (§3.5) |
| `Writer.hpp/.cpp` | `writeMedium`: the codec's files written each by `settings::writeFileAtomically`, the folder made first (EC-1, TR) |
| `MediaTopics.hpp/.cpp` | the payload `MediaState` of the topic `media`, the picker entries, the command and request names (§3.6; AC-7, AC-8) |
| `MediaPart.hpp/.cpp` | `MediaPart`: the session part (topic, commands, autosave, flushes, notices, log) and the `WaitProbe` (§3.7) |
| `PresetMedia.hpp/.cpp` | `addMediaToPresets`: the slot keys' resolver, the in-file media store, the steps `<insert>`, `<arm>`, `eject`, `saveas` on LIB-6's place (§3.8; AC-21–27) |
| `library/harness/MediaCommands.cpp`, `Commands.hpp` (changed) | `addMediaCommands(CommandRegistry&)`: `media`, `media import`, `media export`, `media delete` (§3.11; AC-26, AC-28); the steps reach the harness through LIB-6's `run-step` |
| `tests/TestKinds.hpp/.cpp` | the kinds built for the tests: `test-card` (pass-through, sides 0 and 1, the implicit template `Fresh`, in-file media), `test-disk` (holding, with an optional companion file), `test-module` (held by a module slot, with the write-protect switch); a YAML codec for each, version 2 current, version 1 outdated (§6) |
| `tests/MediaFixture.hpp` | an app session with `MediaPart`, LIB-4's and LIB-6's parts, LIB-3's test frontend and scripted clock, a save folder and a bundled folder of the test's own |
| `tests/*_tests.cpp` | one file per area (§6) |
| `library/CMakeLists.txt` (changed) | the sources of `library/media/` and its tests |
| `docs/test-groups.md` (changed) | the media tests in Frequent; `library/web/tests/media.spec.js` in Longer (M-43) |

Changed in M-41 outside the folder:

| File | Change |
|---|---|
| `library/machine/tests/TestMachines.hpp/.cpp` | `TestMachine` offers a `MediaPlace` for each media place of its descriptor (`reader`: pass-through, sides 0 and 1, "waits" settable by the test; `drive`: holding, side 0) with a byte buffer the test's "ROM" reads and writes through test hooks; a new peripheral `test-memory-module` that fits `test.slot` and `test.card`, with the switch `write-protect` (controls kind) and a `ModuleMedium` |
| `library/machine/Conformance.cpp` | LIB-1 D-1: one capability of kind `MediaPlace` per media place of the descriptor and of each plugged peripheral, its `id()` that place |
| `library/sessions/SessionPart.hpp`, `AppSession.cpp`, `MachineSession.hpp/.cpp` | the hooks of §3.12 (AC-13; in the list of C2's LIB-3 additions, LIB-18 §8) |
| `library/startup/MachineControl.*` (LIB-18) | `build` and `change` call `beforeMachineChange` (S-7); `addCarry`, through which the part adds a module medium's content to a plug carried over at a variant switch (S-7a; LIB-18 §3.4) |
| `library/harness/Run.cpp` | `addMediaCommands` called where the run builds its parts (§3.11) |
| `library/web/glue/` | none: the commands travel as every command does (LIB-15) |

### Component sets (M-41; D-7)

In the sets that M-39 (LIB-8) creates; names prefixed as LIB-14 D-17
asks.

| File | Holds |
|---|---|
| `library/web/components/MediaPicker.jsx`, `NameAndSaveDialog.jsx`, `SavedMediaDialog.jsx` (in `COMPONENT_SOURCES`) | `CalcuMediaPicker` (one control-bar picker from a place's entries), `CalcuNameAndSaveDialog`, `CalcuSavedMediaDialog` |
| `library/qt6/MediaPicker.hpp/.cpp`, `NameAndSaveDialog.hpp/.cpp` (in `calcu_qt6`) | `MediaPicker`, `NameAndSaveDialog` (Qt6 has no saved media list, spec OQ-4) |
| `library/qt6/tests/media_components_tests.cpp` (in `calcu_qt6_tests`) | AC-8, AC-9, AC-16 drawn, without a window |
| `library/web/tests/media.spec.js` | AC-31 and the drawn AC-8, AC-9, AC-16 on the test product (Longer) |

With M-43: the File menu items "Import Media…" and "Saved Media…" are
LIB-14's menu model (its M2, group FileMedia), and the case `ac29` of
`media.spec.js` is added then.

### Calc-U-1600 (M-47) and Calc-U-59 (M-55)

§4. Named here so that the column steps have a frame; their tests are
those of the plan.

## 3. Seams and interfaces

Sketches. Names of other C2 features (`control`, `presets`, `startup`)
stand for what their designs give; each assumption is a seam in §3.13.

### 3.1 Kinds (AC-46; TR "behind one seam"; `capabilities.md` Q-2)

```cpp
namespace calcu::media {

// The kind's own content (Q-2): parsed by the product's codec, handed to a
// media place or carried in a PeripheralRequest, never looked into here.
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
    enum class Status { Read, Outdated, Unreadable } status = Status::Unreadable;
    MediumInfo info;                // Read; Outdated: name and version only
    std::string problem;            // Unreadable: why, one sentence
};

struct FileOut { std::string relativePath; std::string bytes; };   // the medium's file first, then its companions

class MediumCodec {
public:
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

struct ImplicitTemplate { std::string name, description; std::function<Content()> make; };   // the fresh card: no file

enum class HeldBy { MediaPlace, ModuleSlot };

struct PresetWords {                 // AC-21, AC-26; empty: the kind has no such step
    std::string insertStep;          // "card", "floppy", "tape"
    std::string armStep;             // "arm" (pass-through places only)
    std::string ejectWord;           // "floppy", "tape": `eject: <word>`, also the `saveas: { place: }` word
    std::string inFileKey;           // "card": `saveas: { card: }`, and `mem:` references (AC-22)
    bool insertNeedsTime = false;    // `card:` waits until the card has passed (LIB-6 AC-19, AC-56)
    std::string marks;               // LIB-6 AC-62: "[59: TI-59]", "[1600: PC-1600 with CE-1600P]"
};

struct KindDecl {
    std::string id;                  // the descriptor's MediaPlaceDescriptor::mediumKind ("floppy-disk")
    std::string label;               // "floppy disk": notices and the log (AC-25)
    std::string suffix;              // ".floppy.yaml" (AC-4)
    settings::Folder folder = settings::Folder::Save;   // Tapes for a tape (LIB-9 AC-16)
    std::string bundledFolder;       // as the frontend or harness finds it; empty: none
    const MediumCodec* codec = nullptr;
    std::string formatName, describedAt;   // the hint of AC-9
    int currentVersion = 0;
    std::vector<ImplicitTemplate> implicitTemplates;
    HeldBy heldBy = HeldBy::MediaPlace;
    // HeldBy::ModuleSlot: which catalogue peripherals are templates of this kind
    // (not "modules that are not media", AC-5, D-9), and the request a medium
    // of this kind becomes for a place.
    std::function<bool(const machine::PeripheralDescriptor&)> isMedium;
    std::function<std::optional<machine::PeripheralRequest>(const MediumInfo&, const machine::PlacePath&)> moduleRequest;
    PresetWords words;
};

// For the non-media modules of a module slot (AC-8): label, tooltip and
// order. Default: the descriptor's name, no tooltip, by name. Calc-U-59:
// the module's `description`, no tooltip, `menu-sort` (C59-6 AC-8).
struct ModuleListing {
    std::function<std::string(const machine::PeripheralDescriptor&)> label;
    std::function<std::optional<std::string>(const machine::PeripheralDescriptor&)> tooltip;
    std::function<int(const machine::PeripheralDescriptor&)> sortKey;   // equal keys keep the catalogue's order
};

}  // namespace calcu::media
```

The library reads no format (TR). A product supplies one `KindDecl` per
kind at the start of its app session (§3.7). Content written back from a
place is handed to `write` with the `source` bytes of the file it came
from, so that the codec can splice (Calc-U-1600 AC-41) or write whole
(the floppy disk, the magnetic card).

### 3.2 Names (AC-2, AC-3, AC-4)

```cpp
struct NameCheck { std::string name; std::string refusal; };   // refusal empty: accepted
NameCheck checkName(std::string_view raw);       // AC-3: trim, runs of spaces to one; refuse empty,
                                                 // / \ : * ? " < > | and U+0000–U+001F, U+007F
bool sameName(std::string_view, std::string_view);   // AC-3: case folded (D-3)
std::string fileNameFor(const std::string& name, const KindDecl&);   // name + suffix (AC-4)
std::string numberedFileName(const std::string& name, const KindDecl&, const std::function<bool(const std::string&)>& taken);   // foo-1, foo-2, …
std::string nameOf(const MediumInfo&, const std::string& fileName, const KindDecl&);   // AC-2: inner name, else the file name without the suffix
```

"A file of that name exists" (AC-4) compares file names with the same
fold as names (D-4), so a save folder behaves alike on a file system
that tells case apart and on one that does not.

### 3.3 The media kind: two capability interfaces (AC-6; D-1)

```cpp
class MediaPlace : public machine::Capability {     // a media place of a descriptor
public:
    static constexpr machine::KindId kKind{"media-place"};
    machine::KindId kind() const override { return kKind; }
    virtual machine::MediaPlaceId id() const = 0;   // its descriptor entry: kind, passage, sides
    struct Inserted { bool ok = true; std::string refusal; };   // the content is the kind's; a refusal names why
    virtual Inserted insert(Content, int side) = 0; // AC-10: in at once, no power cycle
    virtual Content eject() = 0;                    // what it holds now; nullptr when empty
    virtual std::optional<int> side() const = 0;    // std::nullopt: empty
    virtual Content content() const = 0;            // for saving
    virtual std::uint64_t revision() const = 0;     // changes with the content (AC-11)
    virtual bool waitsForMedium() const = 0;        // LIB-3 AC-22
    virtual std::optional<bool> writeProtected() const { return std::nullopt; }   // nullopt: no switch
    virtual void setWriteProtected() {}             // AC-19: on, never off
    // Pass-through: the medium has passed and is given back (AC-12).
    struct GivenBack { Content content; bool written = false; };
    virtual std::optional<GivenBack> takeGivenBack() { return std::nullopt; }
    // Holding: the machine set the moment of a write (a tape: the motor stopped, D-11).
    virtual bool takeWriteNow() { return false; }
};

class ModuleMedium : public machine::Capability {   // a plugged memory module that is a medium
public:
    static constexpr machine::KindId kKind{"module-medium"};
    machine::KindId kind() const override { return kKind; }
    virtual Content content() const = 0;
    virtual std::uint64_t revision() const = 0;
};
```

- A media place of a descriptor (`MachineDescriptor::mediaPlaces`,
  `PeripheralDescriptor::mediaPlaces`) has one `MediaPlace`; its owner
  is in the `CapabilityEntry` (LIB-1 §3.7). The conformance check of
  LIB-1 D-1 checks it.
- A module slot is not a media place of a descriptor. A memory module
  that is a medium is a peripheral in that slot (glossary: Kind of
  medium) and offers a `ModuleMedium`. Inserting is LIB-18's attach
  (an empty slot) or replace (AC-10, LIB-2 AC-35); ejecting is its
  detach. Its write-protect switch is the controls kind's `write-protect`
  (LIB-2 D-9); the library sets it on through `Controls::set` (AC-19).
- Sides: turning a medium over is an eject and an insert of the other
  side (Calc-U-1600 AC-39), done by the media part.

The library addresses both by a **place word**: a module slot by its
place name (`slot-1`), a media place by its kind's `ejectWord`
(`floppy`), else its `MediaPlaceId`. `PlaceRef` in §3.6 carries it.

### 3.4 The shelf (AC-1, AC-5, AC-7, AC-9, EC-3)

```cpp
enum class Origin { Bundled, Implicit, Saved, Opened, InFile };
struct ShelfEntry {
    const KindDecl* kind;
    std::string name, description, fileName;   // fileName: in its folder, never a path in a topic (D-17)
    std::string path;                          // for the part only
    Origin origin;
    bool isTemplate = false, hasWriteProtect = false, writeProtected = false;
    std::optional<std::string> outdatedHint;   // AC-9: "<format> version <n> … described in <place>"
};
class Shelf {
public:
    Shelf(const std::vector<KindDecl>&, const settings::Paths&);
    struct Scan { std::vector<std::string> logLines; };   // EC-3, AC-9, D-8: one per file
    Scan reread();                                         // AC-7: the folders, read again
    std::vector<ShelfEntry> entries(const KindDecl&) const;   // templates, instances, outdated
    // AC-5: bundled and implicit first, then the save folder. Of two saved
    // files with one name, the one whose file name the name gives, else the
    // first by file name (OQ-1).
    const ShelfEntry* find(const KindDecl&, std::string_view name) const;
    std::optional<ReadOutcome> readFile(const KindDecl&, const std::string& path) const;   // a `file:` reference, an opened file
};
```

- Files of a kind's suffix in the bundled folder, the kind's folder of
  the save folder (save folder or tape folder), each read through the
  codec; companions through the `Companion` callback relative to the
  file. A file that is `Unreadable` is not listed and gives one log line
  (EC-3); an `Outdated` one is listed as an entry that cannot be chosen,
  with the hint, and gives one log line (AC-9). The lines are logged at
  the first scan after the app session started (D-12), not at each
  re-read.
- A saved file whose name is that of a bundled or implicit template of
  its kind is not listed and gives one log line (D-8; AC-5 "an instance
  never carries the name of a bundled template").
- Bundled files are templates whatever their file says (AC-1): a
  bundled file is never written.
- The shelf is read again when the part wrote to a folder, and on the
  command `media.refresh`, which a frontend sends when it opens a picker
  or the saved media list (AC-7). Folders are not watched.

### 3.5 Save rules (AC-17, AC-30, AC-31)

```cpp
enum class SaveBy { NameAndSave, SaveasLive, SaveasTemplate };
struct SaveRequest {
    const KindDecl* kind; SaveBy by;
    std::variant<std::string /*name*/, std::string /*file path*/> target;   // name: or file:
    bool replaceConfirmed = false;              // Name & Save: the user was asked (AC-17 row 5)
    std::optional<PlaceRef> from;               // the place whose medium is saved, for row 4
};
struct SavePlan {
    enum class Verdict { Write, Refused, Ask } verdict;
    std::string path, name, refusal;            // Ask: the sentence of the question
};
SavePlan planSave(const SaveRequest&, const Shelf&, const PlaceIndex& inPlaces);
```

`planSave` applies the rows of AC-17 in their order, among media of the
kind: AC-3; a bundled or implicit name; a user template's name (refused
unless `SaveasTemplate`, which replaces it); an instance in another
place now; an instance (Name & Save: `Ask` unless confirmed, D-6;
`SaveasLive`: replaces; `SaveasTemplate`: refused); and, when the file
the name gives exists and is unreadable or outdated, a numbered file
(AC-4). An instance that is replaced keeps its file; the name tag takes
the new spelling (D-4). The `file:` form writes that file with its stem
as the name, refused for a bundled file, and for a user template unless
`SaveasTemplate`.

`planImport` (AC-30): the kind by suffix; readable and current; its inner
name (AC-2) not that of any medium of the kind, bundled, implicit or
saved; the file name by AC-4 in the kind's folder; written byte for byte
with its companions, as the template or instance it says it is.
`planDelete` (AC-31, EC-4): the shelf read again first; a medium that is
no longer there gives "… is no longer there."; one in a place is
refused; otherwise `Ask` until confirmed.

### 3.6 The topic `media` and the commands (AC-7, AC-8, AC-9, AC-16, AC-20, AC-29–31)

State topic `media`, payload `MediaState`:

```cpp
struct PickerEntry {
    enum class Group { Empty, BundledTemplate, UserTemplate, Instance, Module } group;
    std::string id;             // what `media.choose` takes back
    std::string label;          // the name; a Solid State ROM module: its description (AC-8)
    std::optional<std::string> tooltip;   // the description
    bool canChoose = true;      // false: outdated (AC-9)
    std::optional<std::string> hint;      // the outdated hint
};
struct PlaceState {
    std::string place;          // the place word (§3.3)
    std::string kind;
    bool passThrough = false;
    std::vector<int> sides;
    struct Held { std::string name; std::optional<int> side; bool isTemplate, unsaved, writeProtected; };
    std::optional<Held> holds;  // unsaved: a template written to (AC-15) or a started name not yet written (AC-20);
                                // a pass-through place: the fitted medium, `side` the one passing now (D-22)
    std::optional<int> passing;           // a pass-through place: the side passing through now (C59-7 AC-12, AC-13)
    bool waits = false;         // the machine waits for a medium now
    bool nameAndSave = false;   // AC-16, EC-9
    bool askWriteProtect = false;         // AC-16: the medium has a switch still off
    std::vector<PickerEntry> picker;      // AC-8 order; joins LIB-4's place entry (§3.9)
};
struct KindState {
    std::string kind, label;
    struct Item { std::string name, description, fileName; Origin origin; bool isTemplate, outdated; std::optional<std::string> inPlace; };
    std::vector<Item> templates, instances;   // AC-7; the saved media list reads `Saved`/`Opened` items (AC-31)
};
struct MediaState final : sessions::PayloadOf<MediaState> {
    static constexpr sessions::PayloadKind kKind{"media"};
    std::vector<KindState> kinds;
    std::vector<PlaceState> places;      // in the control state's order
};
```

Commands (library names, no prefix, as LIB-3's `speed`; arguments in
braces; (op) starts an operation):

| Command | Arguments | Does | ACs |
|---|---|---|---|
| `media.refresh` | — | the shelf read again (a picker or the list opened) | AC-7 |
| `media.choose` | `{place, entry}` | writes what the old medium has to save (AC-13); holding place: eject, insert; pass-through place: fitted, not fed, refused while a medium passes (D-22); module slot: (op) LIB-18's attach or replace with the medium's request, what follows as LIB-18 says; unreadable: the place empty and a notice | AC-8, AC-10 |
| `media.eject` | `{place}` | AC-13, then eject or (op) detach; an empty place: nothing (EC-6); a pass-through place: a medium passing is taken out and stays fitted, else the fitted medium goes (D-22) | AC-13, EC-6 |
| `media.turn` | `{place}` | eject, insert the other side | AC-6 (Calc-U-1600 AC-39) |
| `media.arm` | `{place, entry}` | a pass-through place: the medium fitted (armed), not inserted; it takes the place of the one fitted before; refused while a medium passes (D-22) | AC-26 |
| `media.feed` | `{place, side}` | the fitted medium inserted with that side (the corner tap of C59-7 sends it); it stays fitted; refused while a medium passes (D-22) | AC-26 |
| `media.name-and-save` | `{place, name, protect?, confirmed?}` | AC-16–19; reply `asks:` with the question when an instance has the name and `confirmed` is not set (D-6); with `protect`: (op), the power cycle of AC-19 | AC-16–19, EC-9 |
| `media.start-new` | `{place, template, name, confirmed?}` | AC-20: Name & Save done up front, `asks:` as above; the file at the first write | AC-20 |
| `media.open-file` | `{name, place?, path?, bytes?, upload?, confirmed?}` | AC-1: a medium opened from elsewhere, put into `place` (a media place word or a module slot). With `upload` (the host cannot write it back, a Web frontend) it is imported first by AC-30, `asks:` when its name is taken and `confirmed` is not set; else used and written where it lies (`path`). Without `place` (an upload that has no place now, LIB-13 D-8) it is only imported. A template is not written. For LIB-13 (§3.7, "Opened files") | AC-1, AC-30 |
| `media.import` | `{paths?}` | without paths: asks the host's `file-picker` (LIB-15 AC-4) with the suffixes; then AC-30 for each | AC-30 |
| `media.export` | `{kind, name}` | hands each file of the medium to the host's `keep-file` (LIB-15 AC-9), the medium's file first | AC-31 |
| `media.delete` | `{kind, name, confirmed?}` | AC-31: reply `asks:` until confirmed; EC-4 | AC-31, EC-4 |

A reply `asks:` is a `Refused` whose reason begins with the agreed word
`asks:` and holds the question (D-6); nothing changed and no notice is
sent. The frontend draws the question (LIB-14 N4) and, on yes, sends
the same command again with `confirmed: true`; on no it sends nothing.
LIB-13's `open` asks the same way. The harness never gets `asks:` (its
commands are the `saveas` columns and a confirmed delete).

Every command and every write ends with the topic computed and sent if
it differs (LIB-3 AC-5).

### 3.7 The media part (AC-10–15, AC-18–20, EC-1, EC-2, EC-8)

```cpp
struct MediaSetup {
    std::vector<KindDecl> kinds;
    const settings::Paths* paths;
    ModuleListing listing;
};
class MediaPart final : public sessions::SessionPart, public sessions::WaitProbe {
public:
    explicit MediaPart(MediaSetup);
    void attach(sessions::SessionSetup&) override;              // the topic, the commands
    void afterCall(sessions::SessionContext&, pacing::WallTime) override;   // autosave (AC-11)
    void beforeFlush(sessions::SessionContext&) override;       // LIB-4's hook; VirtualTime: autosave at once (AC-11); the topic
    void beforeOperation(sessions::SessionContext&) override;   // AC-13
    void beforeMachineChange(sessions::SessionContext&) override;   // AC-13, AC-27
    void machineChanged(sessions::SessionContext&) override;    // rebind places; in-file media on a model switch (D-10)
    void suspending(sessions::SessionContext&) override;        // AC-13, EC-8
    void closing(sessions::SessionContext&) override;           // AC-13: quit
    bool waitsForMedium(const machine::Machine&) const override;   // LIB-3 AC-22; LIB-17 AC-16
    const Shelf& shelf() const;
    // For PresetMedia and HarnessMedia (§3.8, §3.11):
    Binding bindingOf(const PlaceRef&) const;
    void bind(const PlaceRef&, Binding);
    WriteResult flush(sessions::SessionContext&, const PlaceRef&);   // AC-13
    SaveResult save(sessions::SessionContext&, const PlaceRef&, const SaveRequest&);   // AC-17–19
    // Opened files, for LIB-13's MediaOpening (its §3.4; AC-1, AC-30):
    struct OpenedMedium { std::optional<std::string> refusal; const KindDecl* kind; bool isTemplate, isModule; };
    OpenedMedium readOpened(std::string_view name, const std::string& bytes) const;   // by suffix; refusal: outdated (AC-9 hint) or unreadable
    std::vector<std::string> placesFor(const OpenedMedium&, const machine::Machine&) const;   // place words now: its kind's media places, for a module the slots it fits
    std::vector<std::string> placesInCatalogue(const OpenedMedium&) const;   // "the card-reader of the TI-59", for a refusal's sentence
    std::optional<std::string> refusesNow(const std::string& place, const machine::Machine&) const;   // passing or writing a medium (LIB-13 EC-4)
};
```

A **binding** is what the part keeps per place: the shelf entry the
medium came from (or an in-file medium), whether it is a template, the
file it is saved to (an instance), the revision last written, the time
a change was first seen, and whether the last write failed.

**Autosave rules**

| # | Rule | Serves |
|---|---|---|
| A1 | An instance in a holding place or a module slot: after each host call the revision is compared with the last written. A change is dated at the start of the call that showed it. It is written at the end of the first call for which `now + gap ≥ dated + 500 ms`, where `gap` is the time between the last two calls, so that the write is never later than half a second after the change (D-5). Same revision: nothing is written | AC-11 |
| A2 | VirtualTime (the harness): at the end of each command (LIB-4's `beforeFlush`, which ends every command) | AC-11 |
| A3 | A pass-through place: `takeGivenBack()` after each call (and after each command); `written` → the instance written at once | AC-12 |
| A4 | `takeWriteNow()` → written at once (a tape when its motor stops, D-11) | AC-11, Calc-U-1600 AC-42 |
| A5 | Written at once, for every binding with a change not written: before `media.choose`, `media.eject`, `media.turn`, `media.name-and-save` touch the place; `beforeOperation`; `beforeMachineChange`; `suspending`; `closing` | AC-13, EC-8 |
| A6 | A template (bundled, user, implicit, in-file) is never written; what the machine wrote stays in the place (`unsaved`) until Name & Save or until it leaves the place | AC-15, AC-25 |
| A7 | A write: the codec's files, each through `settings::writeFileAtomically` after `Paths::ensure` of the kind's folder (EC-1); the content written is the place's, whatever the file holds now (EC-2) | EC-1, EC-2, TR |
| A8 | A failed write: one notice "The <label> “<name>” could not be saved: <problem>." until a write of that medium succeeds; the next change tries again (A1) | AC-14 |
| A9 | Each write: one log line "Saved <label> “<name>”." (kind and name tag, never the path); the shelf is read again | AC-25 |

**Name & Save** (AC-16, AC-18, AC-19): offered (`nameAndSave`) for a
place that holds a medium that is not an instance and whose content can
change (a medium with a write-protect switch that is on cannot change).
`planSave`; `Write` → the codec writes with `asTemplate = false`; the
binding becomes that instance (autosaved from then on, AC-18), without
a power cycle and without touching the machine. With `protect`: the
file says so, `MediaPlace::setWriteProtected()` or `Controls::set(
write-protect, true)`, then the power cycle that follows an attach
(LIB-18, §3.10), as one operation (AC-19). Saved as a template by
`saveas`, the binding stays a template (AC-18).

**Opened files** (AC-1; LIB-13 AC-4). LIB-13 decides which place an
opened medium goes into, from `readOpened`, `placesFor`,
`placesInCatalogue` and `refusesNow`, and then sends `media.open-file`
with that place. The command does what LIB-7 says for the place (§3.10
for a module slot; a pass-through place: fitted, not fed, D-22) and
binds it to the file where it lies, or to the imported instance.

**A pass-through place** (D-22): its binding is the fitted medium, one
state. `media.arm`, `media.start-new`, `media.choose`, `media.open-file`
and the arm step fit it (the binding, with its content, nothing
inserted); `media.feed` inserts a side of it, and it stays fitted; A3
keeps its content as the place gave it back, so the next feed inserts
what the last pass wrote. Nothing is fitted or fed while a medium
passes (C59-7 AC-16). The topic's `holds` is the fitted medium and
`passing` the side in the place now.

**Started under a name** (AC-20): `media.start-new` runs `planSave`
for Name & Save (asking before an instance is replaced), inserts the
template's content (a pass-through place: fits it, D-22), and binds the place to the instance file to come;
the template is not written, and the file is replaced at the first
write (A1, A3).

### 3.8 Media in presets (AC-21–27, EC-5–7; LIB-6 AC-3, AC-29)

`addMediaToPresets(presets place, MediaPart&)` registers, for each kind:

- **Slot keys** (LIB-6 AC-3): a resolver for a place whose pin-out takes
  a module. `{ name: }` is looked up in one list of names: the non-media
  modules that fit the place and the media of every `ModuleSlot` kind
  (AC-5); `{ file: }` is read through the kind of its suffix. A bare
  value, a missing, unreadable or outdated medium is an error with the
  line, at read time (AC-21, AC-23). The result is the
  `PeripheralRequest` for LIB-18's build (AC-24) and a binding the part
  takes when the machine is built.
- **Steps**, with the kind's marks (LIB-6 AC-62): `<insertStep>`
  (reference with `side:` where the place has sides, default 0;
  `name:`, `file:`, and `mem:` where the kind has `inFileKey`);
  `<armStep>` (no side); `eject: <ejectWord>`; and once `saveas:` (a
  mapping: `place:` or `<inFileKey>:`, `as: live|template`, optional
  `protected: true`, and `name:` or `file:`). Unknown keys, a bare value
  and `slot-1-file:`, `floppy-file:` are refused by LIB-6 (AC-21, LIB-6
  AC-30). Each reference is resolved when the preset is read (AC-23).
- **In-file media** (AC-22): a store, per load, of media defined in the
  preset. A product's header key (`magnetic-cards:`, C59-7) puts its
  items into the store through `InFileMedia::define(kind, name, content)`
  when the preset is read; the library knows only the name and the
  content. A `mem:` name that the preset does not define is a fresh
  in-file medium from the kind's implicit template (AC-23). In-file
  media live until the next load or model switch (D-10); writes stay in
  the store (A6); `saveas: { <inFileKey>: … }` writes one, and `as: live`
  binds it to that instance from then on (AC-15).

What the steps do:

| Step | Carried out (an operation step of LIB-6) | Finished (LIB-6 AC-56) |
|---|---|---|
| `<insertStep>` holding | A5 for the place; eject (EC-5); insert; bind | at once |
| `<insertStep>` pass-through | fit (bind), feed the side; waits within `timeout:` (LIB-6 AC-57) until the place gives the medium back; A3; it stays fitted (D-22) | when given back (and fitted, C59-7) |
| `<armStep>` | the medium fitted for the place (bind); nothing inserted; fails while a medium passes | at once |
| `eject` | A5; eject; an empty place: nothing (EC-6) | at once |
| `saveas` | `planSave` with `SaveasLive`/`SaveasTemplate`; `protected:` on a medium without a switch fails with that reason (EC-7); any refusal fails the preset (LIB-6 AC-17) | at once (AC-19's power cycle: when ready) |

- A medium named by `name:` or `file:` is a template when its file says
  so and an instance otherwise, also when named by `file:` (AC-25): an
  instance is written in place, so a preset run again starts from what
  the last run left.
- Each step logs one line with the kind's label and the name tag
  (AC-25; LIB-6 AC-18).
- AC-27: when the next machine is built, a fitted medium of a
  pass-through place that has not passed since it was fitted gives one
  log line "<label> “<name>” was armed and never fed."; a variant switch,
  which carries the fitted medium over (S-7a), gives none (D-22).

### 3.9 The pickers and the control state (AC-8, AC-9; LIB-4 AC-1, AC-7)

The picker of a place is computed in the part, from LIB-4's control
state (the place, its peripherals that fit, their availability) and the
shelf, and sent in `PlaceState::picker`: the empty place (where the
place can be empty); bundled and implicit templates, then the user's,
each by name; the instances by name; then, in a module slot, the
modules that are not media in the `ModuleListing` order. Outdated files
are entries that cannot be chosen, with the hint (AC-9). An entry LIB-4
marks not available now (an operation runs, the machine refuses) is
`canChoose = false` with LIB-4's reason. The frontend draws the list as
it is; no rule lives in it (LIB-4 TR).

### 3.10 Module slots and LIB-18 (AC-10, AC-19)

- `media.choose` in a module slot makes a `startup::PlugChange` (LIB-18
  §3.4) with the request the kind's `moduleRequest` gives, content
  included (or `MachineControl::requestFor` for a non-media module): an
  attach to the empty slot, a replace of a held one, a detach for the
  empty entry. `MachineControl::check` refuses at once (LIB-18 AC-12);
  otherwise an operation runs `MachineControl::change`. What follows is
  LIB-18's (AC-21 for the TI models, AC-29 for the Sharp ones). The part
  binds the slot when the operation ends.
- AC-19's power cycle is the one LIB-18 runs after an attach that the
  pin-out says is followed by one (LIB-2 AC-28), started by the part as
  an operation, over at key readiness.

### 3.11 Harness (AC-26, AC-28; LIB-11 AC-8, AC-22)

`addMediaCommands(CommandRegistry&)` (`library/harness/MediaCommands.cpp`),
common commands:

| Command | Reply |
|---|---|
| `media` | the topic `media` as `state` gives it (AC-7) |
| `media import <file>` | done, or refused with the reason (AC-30) |
| `media export <kind> <name> <path>` | done (the medium's files written beside `<path>`, its file at `<path>`), or refused |
| `media delete <kind> <name>` | done (no question: the command is the confirmation), "… is no longer there.", or refused while in a place |
| `<insertStep> <reference>`, `<armStep> <reference>`, `eject <word>`, `saveas <mapping>` | as the steps (LIB-11 AC-22: `floppy { name: Disk2 }`): the media steps have `StepDef::command = true`, so LIB-6's `addPresetCommands` registers them and sends `run-step` (LIB-6 §3.5, §3.9, D-15); `addMediaCommands` adds none of them |

The run's save folder is empty unless `--save-folder` gives one (LIB-11
AC-8, `RunSetup`); the bundled folders are the product's, so bundled
media are always there. Replies name files as `<save folder>/…`.

### 3.12 Hooks added to LIB-3's session part (AC-13)

LIB-3's `SessionPart` (approved, M-22) has `beforeCall`, `afterCall`,
`machineChanged` (after the change), `operationEnded`, `suspending`;
LIB-4 adds `beforeFlush` (M-37), which ends every host call and
command and serves A2. AC-13 needs three more moments. M-41 adds them,
each with an empty default (the consolidated list of C2's LIB-3
additions is LIB-18 §8):

| Hook | Called by | When |
|---|---|---|
| `beforeOperation(SessionContext&)` | `AppSession::Impl::startOperation` | before the operation's first step |
| `beforeMachineChange(SessionContext&)` | `MachineSession` (before `setMachine`, `takeMachine`) and LIB-18's `MachineControl::build` and `change` (attach, detach, replace) through `MachineSession::aboutToChange()` | before the machine or a peripheral of it changes |
| `closing(SessionContext&)` | `AppSession::~AppSession` | the product quits or the window closes; no message goes out after it |

### 3.13 Seams assumed of the parallel C2 designs

| # | Of | What this design takes | If it is not so |
|---|---|---|---|
| S-1 | LIB-4 (M-37) | the control state, read below the frontend (`control::` current state): per place its peripherals that fit and their availability with the reason; per media place its owner, kind, passage, sides; the action "inserting a medium" | the part computes the fit from the catalogue (LIB-2 AC-2) and availability from `busy()` |
| S-2 | LIB-4 (M-37) | `SessionPart::beforeFlush` (LIB-4 §3.5), called at the start of every flush | — |
| S-3 | LIB-6 (M-38) | the place for steps and header keys (AC-29): a check at read time with the line, an operation step that may wait within `timeout:`, the marks of AC-62 | — (the spec requires it) |
| S-4 | LIB-6 (M-38) | the resolver of a place key's reference (LIB-6 AC-3: "resolving it is that owner's") returning a `PeripheralRequest` and something carried to after the build | the part reads the built machine's slots and finds the binding by the resolved path |
| S-5 | LIB-6 (M-38) | a reference reader for `docs/formats/README.md` §3 (exactly one of `name:`, `file:`, `mem:`, plus the owner's options) | `media/Reference.hpp` in M-41 |
| S-6 | LIB-6 (M-38) | a per-load store for the in-file media, reset at each load, filled by a product header key | the part keeps it and is told of a load by `beforeOperation` |
| S-7 | LIB-18 (M-35) | `startup::MachineControl` (LIB-18 §3.4): `PlugChange` with a full `PeripheralRequest` (content included), `check` before the operation and `change` inside it, which runs what follows; `untilKeyReady`. M-41 makes `build` and `change` call `beforeMachineChange` | — |
| S-7a | LIB-18 (M-35, M-41) | on a variant switch the part adds a module medium's content to the plug `variantPlan` carries over, through `MachineControl::addCarry` (added in M-41, LIB-18 §3.4), and puts the media of media places back at `machineChanged`, so the new machine has the same media (LIB-18 AC-7) | — |
| S-8 | LIB-13 (M-42) | it reads an opened file with `readOpened`, `placesFor`, `placesInCatalogue`, `refusesNow` and sends `media.open-file` with the place it chose (§3.6, §3.7) | — |
| S-9 | LIB-8 (M-39), LIB-14 (M-43) | the React and Qt6 component sets that M-39 creates, in which `MediaPicker`, `NameAndSaveDialog`, `SavedMediaDialog` are written in M-41; the File menu items of LIB-14 AC-35 in M-43 | D-7 |

## 4. Product side

M-41 changes no product code. The product columns:

### Calc-U-1600 (M-47)

| File | Change |
|---|---|
| `products/calc-u-1600/app/SharpMediaKinds.hpp/.cpp` (new) | the three `KindDecl`s and codecs: `memory-module` (`.card.yaml` v1, `HeldBy::ModuleSlot`; codec over `core/Connector/MemoryCardDefinition.hpp` and the splice of `BatteryCardInstance.hpp`, AC-41; `isMedium` = a battery-backed definition; `moduleRequest` over C1600-1's slot requests), `floppy-disk` (`.floppy.yaml` v2, sides `0:`/`1:`; codec over `FloppyImageFile.hpp`), `cassette-tape` (`.tape.yaml` v1 with its WAV, `Folder::Tapes`, new; the bundled empty tape) |
| `core/` media places | the CE-1600F drive's `c1600.floppy-drive` product kind (C1600-1 §5) becomes a `MediaPlace` (holding); the tape deck's `c1600.tape-deck` (`tapePlay`, `tapeRecord`, `tapeEject`, `tapeStatus`) becomes a `MediaPlace` (holding, `takeWriteNow` at motor stop, D-11) plus a product kind for the counter and the motor lamp; a battery-backed `SoftwareDefinedCard` offers `ModuleMedium` over `contentRevision()` and `debugImage()` |
| `app/ModuleBay.*`, `FloppyBay.*`, `TapeBay.*`, `MediaLibrary.*`, `DirectoryScanCache.hpp` | deleted; the library's part and shelf |
| `core/Connector/MediaSave.hpp` | `planMediaSave`, `mediaSaveRefusalText`, `writeFileAtomically` deleted (library rules, `settings::writeFileAtomically`); `formatCardSave` moves into the codec |
| `core/Connector/NamedFileCatalog.hpp` | `scanNamedFiles`, `resolveNamedFile`, `namedFileName` deleted where media use them (the shelf, `fileNameFor`) |
| `core/Preset/PresetMedia.hpp`, `core/PC1600/PC1600PresetMedia.hpp` | `savePresetMedia` goes; `pc1500_cli`, `pc1600_cli` call the library's `planSave` and writer (tools rule, plan §1) until M-51 |
| `app/SharpAppPart.*`, `SharpTopics.hpp` | `MediaPart` in the parts; `c1600.select-module` → `media.choose`; `c1600.tapes` and `MachinePayload::moduleChanges` go (LIB-3's interim topics, re-pointed); `AppPaths` folder methods (LIB-9 A-1) go |
| `frontends/qt6/app/MemoryModuleManager.*`, `FloppyDiskManager.*`, `TapeManager.*`, `ControlBar.*` | onto the topic `media` and its commands; drawn by the library's components with LIB-14 1600 (M-49) |
| `frontends/web/src/Dialogs.jsx`, `ControlBar.jsx` | the saved media and import through the library's components and commands (AC-43) |
| tests | `app/tests/shared_bay_tests.cpp`, `shared_app_session_tests.cpp`, `tests/battery_card_instance_tests.cpp`, `tape_deck_tests.cpp`, `frontends/web/tests/play.spec.js` ported to the topic and commands (AC-44, AC-45) |

### Calc-U-59 (M-55, with C59-7)

| File | Change |
|---|---|
| `products/calc-u-59/app/TIMediaKinds.hpp/.cpp` (new) | `magnetic-card`: `.magnetcard.yaml` v3 (C59-7's codec), sides 0 and 1, the implicit template "fresh card", `words = {card, arm, —, card, insertNeedsTime}` with the marks `[59: TI-59]`; `ModuleListing` from the module files (`description`, `menu-sort`; C59-6 AC-8) |
| `core/TIAdapters.*`, `TI59Machine.*` | the card reader offers `MediaPlace` (pass-through, `waitsForMedium`, `takeGivenBack`); C59-1's `TICardReaderAdapter` goes |
| `app/TICardWaitAdapter.hpp` | deleted: `MediaPart` is the `WaitProbe` (LIB-17 §5) |
| `app/TIAppPart.*`, `TIAppPartFlows.cpp` | the topic `c59.card-reader` and the commands `c59.fit-card`, `c59.fit-blank-card`, `c59.eject-card` → `media` and its commands; `c59.tap-card` sends `media.feed`; the cue card and fitting stay C59-7's |
| `app/TIFileAccess.*`, `CardFile.*` | `readCard`, `writeCard`, `cardFilePath` go; `CardFile` replaced by C59-7's format 3 |
| frontends | Qt6 `CardPickerDialog`, Web `Dialogs.jsx` (download per card, upload in the picker) → the picker and the saved media (AC-36); Swift `CardStorage.swift`, `CardPickerView.swift` onto the topic (drawn natively in C59-8, M-58) |

## 5. Adapters

M-41 needs none: no product calls the new code. Until the column steps
the old calls stay behind these adapters, which earlier designs named:

| Adapter | File | Wraps | Removed in |
|---|---|---|---|
| A-1 interim Sharp media | `products/calc-u-1600/app/SharpAppPart.*` | `ModuleBay`, `FloppyBay`, `TapeBay`, `MediaLibrary` behind `c1600.select-module`, `c1600.tapes`, `MachinePayload::moduleChanges` (LIB-3 §5) | M-47 |
| A-2 `AppPaths` folders | `products/calc-u-1600/app/AppPaths.*` | LIB-9 A-1 (`instanceDir`, `tapeDir`) | M-47 |
| A-3 `planMediaSave`, `savePresetMedia` | `core/Connector/MediaSave.hpp`, `core/Preset/PresetMedia.hpp` | today's save rules for the bays and `pc1500_cli`, `pc1600_cli` | M-47 (the tools: onto the library; gone M-51) |
| A-4 interim TI card flow | `products/calc-u-59/app/TIAppPart.*` | `c59.card-reader`, `c59.fit-card`, `c59.fit-blank-card`, `c59.eject-card` (LIB-3 §5) | M-55 |
| A-5 `TIFileAccess` cards | `products/calc-u-59/app/TIFileAccess.*` | `readCard`, `writeCard`, `cardFilePath` (LIB-3 §5) | M-55 |
| A-6 `TICardWaitAdapter`, `TICardReaderAdapter` | `products/calc-u-59/app/`, `core/TIAdapters.hpp` | the card wait (LIB-17 §5, C59-1) | M-55 |
| A-7 Qt6 `AppPaths::cardDir()` | `products/calc-u-59/frontends/qt6/app/AppPaths.hpp` | LIB-9 A-2 | M-55 |

## 6. Tasks

Tests come before the code. Groups as in `docs/test-groups.md`. The
library tests run on LIB-1's test machines (`test-seg` with the
pass-through `reader`, `test-dot` with the holding `drive`, the new
`test-memory-module` in `slot-1`) through `MediaFixture`, with the kinds
of `TestKinds` and folders of the test's own. Case names follow LIB-1
§2a (`acN_`, `ecN_`).

| Task | What | ACs/ECs | Test (file, cases, group) |
|---|---|---|---|
| L1 | `Names` | AC-2, AC-3, AC-4 | `names_tests.cpp`: `ac2_inner_name_wins`, `ac2_file_name_counts_only_without_inner_name`, `ac3_spaces_trimmed_and_runs_collapsed`, `ac3_refused_characters_and_control_characters`, `ac3_empty_refused`, `ac3_names_differing_in_case_are_one`, `ac4_file_is_name_and_suffix_in_save_folder`, `ac4_numbered_when_file_holds_another_name`; Frequent |
| L2 | `Kind`, `Shelf` | AC-1, AC-5, AC-46, EC-3 | `shelf_tests.cpp`: `ac1_bundled_is_template_and_never_written`, `ac1_user_template_by_its_file`, `ac1_instance_in_save_folder`, `ac5_bundled_first_then_save_folder`, `ac5_two_files_one_name_both_listed_and_found_by_rule`, `ac5_saved_file_with_bundled_name_not_listed_and_logged`, `ac5_module_slot_one_list_of_names`, `ac46_codec_gives_all_the_library_knows`, `ac46_content_handed_through_unread`, `ec3_unreadable_not_listed_one_log_line_per_start`; Frequent |
| L3 | `MediaPlace`, `ModuleMedium`, test machine, conformance; the fitted medium of a pass-through place | AC-6, AC-26 | `place_tests.cpp`: `ac6_place_states_kind_passage_sides`, `ac6_insert_eject_side_content_revision`, `ac6_waits_for_medium`, `ac6_write_protect_where_the_medium_has_one`, `ac26_fitted_card_fed_by_each_side_stays_fitted`, `ac26_nothing_fitted_or_fed_while_a_card_passes`, `ac26_eject_takes_passing_card_out_still_fitted`, `ac26_choose_fits_does_not_feed` (D-22); `library/machine/tests/conformance_tests.cpp`: `ac40_media_place_capability_per_media_place` (LIB-1 D-1); Frequent |
| L4 | topic `media` | AC-7 | `topic_tests.cpp`: `ac7_kinds_templates_instances_with_place`, `ac7_place_holds_and_waits`, `ac7_reread_after_own_write_and_on_refresh`, `ac7_outside_change_not_seen_until_refresh`; Frequent |
| L5 | pickers, choose | AC-8, AC-9, AC-10 | `picker_tests.cpp`: `ac8_order_empty_templates_instances_modules`, `ac8_tooltip_is_description`, `ac8_listing_label_and_sort_key` (a `ModuleListing` that labels by description in `menu-sort` order), `ac9_outdated_entry_disabled_with_hint`, `ac9_outdated_logged_once_per_start`, `ac10_old_medium_written_first`, `ac10_holding_place_new_medium_in_at_once`, `ac10_module_slot_is_a_replace_by_lib18`, `ac10_unreadable_leaves_place_empty_with_notice`; Frequent |
| L6 | autosave | AC-11–15, EC-1, EC-2, EC-8 | `autosave_tests.cpp`: `ac11_written_within_half_a_second_at_60_hz`, `ac11_not_written_while_nothing_changes`, `ac11_harness_written_at_end_of_command`, `ac12_pass_through_written_when_given_back_if_written`, `ac12_fed_again_inserts_what_the_last_pass_wrote`, `ac13_written_before_leaving_place`, `ac13_written_before_operation`, `ac13_written_before_machine_change`, `ac13_written_on_suspend`, `ac13_written_on_close`, `ac14_failed_write_one_notice_until_success`, `ac14_next_change_tries_again`, `ac15_picker_template_not_written_lives_in_place`, `ac15_preset_named_template_not_written`, `ac15_in_file_medium_not_written_unless_saveas`, `ec1_save_folder_made_at_first_write`, `ec2_outside_change_overwritten_not_merged`, `ec8_suspend_writes_every_pending_change`; Frequent |
| L7 | Name & Save, rules | AC-16–20, EC-7, EC-9 | `save_tests.cpp`: `ac16_offered_for_template_that_can_change`, `ac16_asks_write_protect_only_when_switch_off`, `ac17_rules_table` (each row × column), `ac17_file_form_writes_that_file`, `ac17_file_form_refused_for_bundled_and_user_template`, `ac17_name_and_save_asks_before_replacing`, `ac18_place_holds_saved_medium_no_power_cycle`, `ac18_live_autosaved_template_not`, `ac19_protect_in_file_at_once_and_power_cycle`, `ac19_no_command_switches_it_off`, `ac20_started_name_asked_first_file_at_first_write`, `ec7_saveas_protected_without_switch_fails`, `ec9_name_and_save_not_for_instance`; Frequent |
| L8 | presets | AC-21–27, EC-5, EC-6 | `preset_media_tests.cpp` (a test product's sections and the test kinds' steps `test-card:`, `test-arm:`, `test-disk:`): `ac21_reference_mapping_name_file_mem_side`, `ac21_bare_value_refused`, `ac21_old_file_keys_refused`, `ac22_in_file_media_from_header_key_saved_only_by_saveas`, `ac23_missing_outdated_unreadable_refused_at_read`, `ac23_undefined_mem_is_fresh`, `ac24_slot_instance_fitted_before_start_and_autosaved`, `ac24_step_inserts_instance`, `ac25_template_file_never_written_wherever`, `ac25_instance_by_file_written_in_place_rerun_starts_from_it`, `ac25_log_names_kind_and_name_tag`, `ac26_steps_table`, `ac26_pass_through_step_finishes_when_passed_within_timeout`, `ac27_armed_never_fed_logged_at_next_build`, `ec5_insert_ejects_held_first`, `ec6_eject_empty_place_nothing`; Frequent |
| L9 | harness | AC-26, AC-28 | `library/harness/tests/media_commands_tests.cpp`: `ac26_steps_as_commands`, `ac28_media_reads_topic`, `ac28_import_export_delete`, `ac28_run_has_empty_save_folder_and_bundled_media` (LIB-11 AC-8), `ac26_mapping_line_as_step` (LIB-11 AC-22); Frequent |
| L10 | saved media, import | AC-30, AC-31, EC-4 | `saved_media_tests.cpp`: `ac30_import_current_medium_under_inner_name`, `ac30_import_refused_name_taken_outdated_unreadable`, `ac30_import_with_companion_file`, `ac31_list_by_kind_and_name_marks_template_and_in_place`, `ac31_export_hands_files_to_keep_file`, `ac31_delete_asks_then_deletes_not_in_place`, `ec4_second_delete_no_longer_there`; `ac1_opened_file_written_where_it_lies` (the command of LIB-13); Frequent |
| L11 | components (M-41; AC-29 in M-43, D-7) | AC-8, AC-9, AC-16, AC-29, AC-31 | `library/qt6/tests/media_components_tests.cpp` (`calcu_qt6_tests`, offscreen): `ac8_picker_draws_entries`, `ac9_outdated_entry_disabled_with_hint_tooltip`, `ac16_name_and_save_dialog_asks_name_and_switch`; Frequent. `library/web/tests/media.spec.js`: `ac31 saved media lists downloads deletes after asking`, `ac8 picker lists the saved instance`, `ac29 media in persistent file system after reload`; with M-43 `ac29 file menu has import and saved media`; Longer |

Coverage: L1–L11 cover AC-1 to AC-31 and AC-46 (32 of 32) and EC-1 to
EC-9 (9 of 9). AC-29 passes in M-43 (§1).

## 7. Tools and agent knowledge

- Today's tools: unaffected by M-41. `pc1500_cli` and `pc1600_cli`
  (`savePresetMedia`) move onto the library's rules in M-47 (A-3) and go
  in M-51; Calc-U-59's `tools/headless/` and `tools/agentic/` reach cards
  through presets and move with M-54 / M-55.
- Agent knowledge in M-41: none to rename (new folder; the glossary has
  medium, template, instance, Name & Save, kind, saved media, media
  place). `docs/test-groups.md` gets the new tests. LIB-1 §2a gets the
  `media` line of the dependency rule (report).
- M-47 and M-55: as the plan lists (`docs/developer/Floppy-Image-Format.md`,
  Calc-U-1600 memory files on media, `Shared-Architecture.md` rows for
  the bays; `calc-u-59-statefiles` and `calc-u-59-shared` skills,
  `reference/StateFileFormat.md`, Calc-U-59 memory files on cards). The
  `docs/glossary-mapping.md` rows "bay", "Card Folder", "Battery-card
  saves" are applied where the code takes the terms.

## 8. Golden runs and risks

- **Golden runs** are not touched in M-41 (no product code). In M-47 the
  reference runs of M-4 with media (DiskWorks, the presets that waited,
  plan question 6) and in M-55 the four card examples are the checks;
  a difference is a finding.
- **Hooks in an approved design** (§3.12): three empty-default hooks on
  `SessionPart`. No product part overrides them before M-47, so nothing
  changes for the products; LIB-3's tests stay green.
- **Parallel seams** (§3.13): if LIB-4, LIB-6 or LIB-18 give these
  seams other shapes, M-41 adapts to them; the fallbacks keep the step
  inside `library/media/`.
- **Test catalogue.** A new peripheral and new capabilities on
  `TestMachine` can change counts in LIB-1, LIB-2 and LIB-4 tests (the
  catalogue's peripherals, `capabilities()`, the control state). M-41
  runs them all and adjusts expectations that list the catalogue, never
  a rule.
- **Hidden tab** (EC-8): the part writes on `suspend`; IDBFS
  `autoPersist` then queues a sync (LIB-15 §3.3). Whether a browser
  completes the IndexedDB transaction of a hidden tab is not verified
  (LIB-15 §8); M-55's web test checks a reload after hiding.
- **Names a host refuses** beyond AC-3 (Windows device names such as
  `CON`, very long names) are not checked; a save then fails with the
  host's problem (AC-14 for autosave, a refusal for a save). Open point
  (D-21).
- **Case folding** (D-3) may surprise a user with two Calc-U-1600 media
  "Work" and "work" today: both are listed, a save by either name finds
  the one whose file name the name gives (OQ-1). Ledger row proposed.

## 9. Decided (best guess)

- **D-1 (AC-6, AC-46; `capabilities.md` C-1, Q-2).** The generic media
  kind has two interfaces: `MediaPlace` for the media places of a
  descriptor and `ModuleMedium` for a plugged memory module that is a
  medium. A module slot is a place of a kind (glossary), not a media
  place of the descriptor, so LIB-4 lists no extra place for it.
- **D-2 (Q-2).** Content crosses as `machine::ProductData`, the type a
  `PeripheralRequest` already carries; the library holds it by
  `shared_ptr` and never looks into it.
- **D-3 (AC-3).** "Differ only in upper and lower case" folds the ASCII
  letters. Other letters are compared as written (no Unicode table in
  the library). Open point: a name with "Ä" and one with "ä" are two
  names.
- **D-4 (AC-4, AC-5).** File names are compared with the same fold, so
  "the file the name gives" is found alike on every file system. A save
  that replaces an instance keeps its file and writes the new spelling
  into the name tag.
- **D-5 (AC-11).** The half second is kept by dating a change at the
  start of the call that showed it and writing at the end of the first
  call that would otherwise pass the deadline (rule A1). Today's
  Calc-U-1600 waits 0.5 s after the frame it saw the change in.
- **D-6 (AC-17, AC-31).** "Asks before replacing" and "after the user
  confirmed" are a reply `asks:` with the question and no notice; the
  frontend asks and sends the same command again with `confirmed: true`
  (one argument name for every command that asks, LIB-13's `open`
  included). No new host service.
- **D-7 (AC-8, AC-9, AC-16, AC-29, AC-31; plan).** The picker entries,
  the Name & Save rules and the saved media list are computed in M-41
  and sent in the topic `media`; the components that draw them (picker,
  Name & Save dialog, saved media dialog) are written in M-41 in the
  React and Qt6 component sets that M-39 (LIB-8) creates. The File menu
  items of AC-29 are LIB-14's menu model (M-43), so the Library column
  is Done in M-43 (report: plan §5.1).
- **D-8 (AC-5).** A saved file that carries a bundled or implicit
  template's name is not listed and gives a log line at each start, as
  Calc-U-1600 leaves it out today (`ModuleBay.hpp`, `ModuleLists`).
- **D-9 (AC-5, AC-8).** "Modules that are not media" are the fitting
  peripherals for which no `ModuleSlot` kind's `isMedium` holds; label,
  tooltip and order come from the product's `ModuleListing`.
- **D-10 (AC-22; `preset.md` §3.1).** In-file media are replaced at each
  load and cleared at a model switch; Reset, a power cycle and a Cold
  boot keep them.
- **D-11 (AC-11; Calc-U-1600 AC-42).** A holding place can ask for a
  write now (`takeWriteNow`); the tape uses it when its motor stops, so
  today's moment of the tape's write stays.
- **D-12 (AC-9, EC-3).** "At each start" is the first scan after the app
  session started; re-reads do not log again.
- **D-13 (AC-31).** A medium with companion files (a tape) is handed to
  the user as its files one after the other, the definition first; the
  harness writes them side by side.
- **D-14 (AC-1; LIB-13 AC-4).** A medium opened from elsewhere comes
  through `media.open-file` with the place LIB-13 chose from the part's
  reads (§3.7); it is listed with origin "opened" while it is in a place
  and written where it lies; an upload (a Web frontend) is imported
  first (AC-30). LIB-13's O-1 is settled by this.
- **D-15 (AC-13).** The hooks of §3.12 are added to LIB-3's session part
  in M-41 (no earlier C2 step needs them; LIB-18 §8); the end of a
  command is LIB-4's `beforeFlush`; quitting is `closing()`, called by
  the app session's destructor.
- **D-16 (AC-19).** A save with the write-protect switch is an operation
  (the power cycle runs until key readiness); a save without it is a
  plain command.
- **D-17 (AC-25).** The topic and every notice name a file by its file
  name or its medium by label and name tag, never by a path; harness
  replies write the folder as `<save folder>` (LIB-11 AC-8).
- **D-18 (AC-16).** "Content can change": a medium whose write-protect
  switch is on in its file cannot change, so Name & Save is not offered
  for it; everything else can.
- **D-19 (AC-7, AC-31).** The saved media are the save folder's and the
  tape folder's; a kind's `folder` says which. An import of a tape goes
  to the tape folder.
- **D-20 (AC-26).** `eject:` and `saveas:` are the library's steps; the
  insert and arm steps are each kind's, named by the product (`card`,
  `arm`, `floppy`, `tape`); `saveas: { place: }` takes a module slot's
  place name or a media place's `ejectWord`.
- **D-21 (open point).** No length limit for names and no check of names
  a host reserves: the spec names none (§8).
- **D-22 (AC-12, AC-26, AC-27; C59-7 AC-10 to AC-17, AC-25; LIB-13
  AC-4, AC-12; Part D simplification P-3, 2026-11-16).** A pass-through
  place's binding is the fitted medium; there is no separate armed
  medium. Departures from the design before it: (1) `media.feed` keeps
  the medium fitted, so it can be fed again by either side (before: the
  armed medium was used up by one feed); (2) the fitted medium's content
  is what the place gave back last (before: the armed copy went stale
  after a write); (3) `media.choose` and `media.open-file` fit a medium
  for a pass-through place (before: they inserted side 0); (4) arming,
  choosing, opening, starting new and feeding are refused while a medium
  passes (before: an insert pulled the passing medium out); (5)
  `media.eject` while a medium passes takes it out of the place and
  leaves it fitted (C59-7 AC-10 with AC-25: "taken out as by Eject
  Card" while "what is fitted stays fitted"); with none passing it
  removes the fitted medium; (6) the topic's `armed` is gone (no client
  read it): `holds` is the fitted medium, `holds.side` and the new
  `passing` the side passing now; (7) AC-27's line is written when the
  next machine is built for a fitted medium that has not passed since it
  was fitted, not on a variant switch, which carries it over. Open for
  M-55: the per-side bank numbers of C59-7 AC-11 are the product's, read
  from the fitted content; a product whose `eject()` of a passing card
  keeps a whole written bank (C59-7 AC-10) reports it through
  `takeGivenBack()`.

## 10. Part D: the Calc-U-1600 column

Draft for the Part C2/D boundary review (2026-11-16). Paths without a
prefix are under `products/calc-u-1600/`; line numbers at `60e5b09`.
It replaces §4's Calc-U-1600 table as the plan of M-47; where it
differs from that table it says so.

### 10.1 Scope

| Column | Step | Tests from (plan §3) | Done in |
|---|---|---|---|
| Calc-U-1600 | M-47 | AC-37–45 | M-47 (plan §5.1) |

- **Waits for** M-41 (this feature's Library column, Done in M-42) and
  M-46 (LIB-6 1600, LIB-18 1600, C1600-4: the library's loader, the
  `MachineControl` the module slots use, the bays' adapter that this
  step removes).
- **Also passes here:** C1600-4 AC-15 and AC-16 for the presets that
  waited (C1600-4 design §1: `DiskWorks`, `make_diskworks_media`,
  `pc1500-maxed-out`, `startup/default-pc1600`, the `startup-pc1600`
  fixture), so C1600-4 is Done here; LIB-6 1600 AC-47's `floppy:`
  clause; LIB-1 1600 and LIB-2 1600 (AC-33, AC-40 of LIB-1: the media
  places; plan §5.1 "Done in M-47"); LIB-18 AC-28's disk and modules
  again, now through `addCarry`.
- **Where the library built in C2 differs from §3** (qa-report,
  "Deviations"): S-1 by its fallback (the part computes what fits a
  slot from the catalogue); the slot resolver is
  `Vocabulary::setPlaceResolver`; `PresetWords` has no `marks` (the
  markers come from `ModelRules::applies`); `closing` comes from
  `AppSession::close()`, which the host calls; `addMediaCommands` takes
  the run's part and `HarnessProduct` has `mediaKinds()` and
  `moduleListing()`; `media.start-new` on a holding place inserts.
  This column uses those shapes.
- **Kind IDs.** The descriptors already name the kinds
  (`core/Machine/SharpCatalogue.cpp:92, 130-131, 142, 165-166`):
  `battery-backed-module`, `floppy-disk`, `cassette`. The `KindDecl`s
  take these IDs, not `memory-module` and `cassette-tape` of §4 (D-22).
  The two slots leave `mediaPlaces` (lines 92, 130-131): a module slot
  is not a media place (D-1); the module is a medium of a `ModuleSlot`
  kind.
- Hardware facts (the CE-1600F, the cassette interfaces) are not
  restated; they are in the research skill `PC1500` and the product's
  memory files on the floppy and the tape.

### 10.2 Modules and files

**New**

| File | Holds |
|---|---|
| `app/SharpMediaKinds.hpp/.cpp` | `shared::mediaKinds(const BundleDirs&)`: the three `KindDecl`s and their codecs (§10.3); `shared::moduleListing()` (the module's `module-name`, its description as the tooltip, the catalogue's order) |
| `core/Machine/SharpMediaPlaces.hpp/.cpp` | the `MediaPlace` of the CE-1600F drive and of the cassette (holding), and the `ModuleMedium` of a battery-backed module (§10.4) |
| `frontends/qt6/resources/cards/empty.tape.yaml`, `empty.wav` | the bundled tape template "Empty" (AC-42; D-23) |
| `tests/media_kinds_tests.cpp` | AC-37, AC-41, AC-42 (codecs), AC-44's ports (§10.6) |

**Changed**

| File | Change |
|---|---|
| `core/Connector/FloppyImageFile.hpp:12-56, 61, 77-78, 109-179` | `kFloppyFormatVersion` 2; `kSideKeys` `{"0", "1"}`; a file of version 1 is `Outdated` (its hint: `preset.md` §9.4), not "unsupported" (row 1600-19) |
| `frontends/qt6/resources/cards/blank.floppy.yaml`, `formatted.floppy.yaml`, `examples/dwx/Progs.floppy.yaml` | version 2, sides `0:` and `1:`, the same bytes |
| `core/Machine/SharpCatalogue.cpp:92, 130-131` | the slots leave `mediaPlaces` (§10.1) |
| `core/Machine/SharpProductKinds.hpp:100-125`, `core/PC1500/PC1500InterfaceMachine.cpp:60, 119`, `core/PC1600/PC1600InterfaceMachine.cpp:69-77, 115-126, 180` | `FloppyDriveAccess` keeps only `motorOn()` (the lamp of AC-39); its disk, side, eject and load move to the drive's `MediaPlace`. `TapeDeckAccess` keeps the counter, the motor and the record state (AC-42's counter); load, record and eject move to the cassette's `MediaPlace` |
| `core/Machine/SharpMachine.cpp:100, 232` | the write-protect switch is the controls kind's `write-protect` on a battery-backed module (LIB-2 D-9) |
| `core/Machine/LegacyCalls.hpp:67-73, 95` | `moduleIn`, `atOffPoint` and `presetMedia` go: the media part reads `ModuleMedium`; the off point is the machine's own (C1600-1) behind `MediaPlace::insert` |
| `app/SharpAppPart.hpp/.cpp` | `MediaPart` (with `addMediaToPresets` on `PresetsPart`'s vocabulary) in `addLibraryParts`, after `StartupPart` (LIB-6 §10.2); the bays and their members (105-110, 144-172, 298-305, 320-342, 787-817, 849-912), `c1600.select-module` (249-253), the topic `c1600.tapes` (`TapesPayload` 1104-1111) and `MachinePayload::moduleChanges` go; `frameDuties` no longer ticks bays |
| `app/AppPaths.hpp/.cpp` | `instanceDir`, `instancePathFor`, `floppyInstancePathFor`, `tapeDir`, `tapePathFor`, `planNamedSave`, `atomicWriteFile`, `mediaSaveRefusalMessage` (A-2) go: LIB-9's `Paths` and `settings::writeFileAtomically`; `bundledResourcesDir` stays |
| `app/ControlState.cpp:34-36` | the floppy and tape places shown and usable from the media topic's places (a place is listed when its peripheral is plugged) |
| `app/SharpWebPart.cpp:169-178, 234-310, 425-507` | `c1600.name-save-card`, `c1600.select-disk`, `c1600.toggle-disk-side`, `c1600.name-save-disk`, `c1600.select-tape`, `c1600.tape-exists`, `c1600.record-tape`, `c1600.media-items`, `c1600.export-media`, `c1600.import-media`, `c1600.remove-media` go: the library's `media.*` commands; `ui()`'s pickers are the topic's; `c1600.upload` stays for program files (LIB-13 1600, M-48); `c1600.card-yaml` (the inspector's dump) stays |
| `app/HarnessProduct.cpp:95` | `mediaKinds()` and `moduleListing()` from `app/SharpMediaKinds` |
| `frontends/qt6/app/ControlBar.cpp:36-62, 97-256, 279-411` | the pickers are filled from `PlaceState::picker` (groups, disabled outdated entries with their hint; AC-8, AC-9); choosing sends `media.choose`, the side button `media.turn`, Name & Save opens the library's `calcu::qt6::NameAndSaveDialog`; the tape counter and lamps from `c1600.machine` as today. The library's `MediaPicker` replaces the combo boxes with LIB-14 1600 (M-49) |
| `frontends/qt6/app/MainWindow.cpp:126-211, 354-379, 545-560` | the module, disk and tape slots of the window send `media.*`; the two dialogs are `NameAndSaveDialog` with `setPrompt("Instance name:")` / `("Disk name:")` (AC-40) |
| `frontends/qt6/app/MemoryModuleManager.*`, `FloppyDiskManager.*`, `TapeManager.*` | deleted |
| `frontends/qt6/app/SettingsDialog.cpp:262-279` | "Battery-card saves:" reads "Save folder:" (`docs/glossary-mapping.md` row "Save folder") |
| `frontends/web/src/engine.js:72-103`, `ControlBar.jsx:27-137`, `CalcU1600.jsx:190-227, 272-273`, `Dialogs.jsx:248-298` | `CalcuMediaPicker` per place of the topic, `CalcuNameAndSaveDialog`, `CalcuSavedMediaDialog` (wrapped as the Web test product does, LIB-7 M-42 deviations); File ▸ "Import Media…" and "Saved Media…" from LIB-14's shell model; `media.import`, `media.export`, `media.delete` (AC-43) |
| `CMakeLists.txt`, `frontends/qt6/CMakeLists.txt:177-211`, `frontends/web/build.sh:44-46` | bundle `*.tape.yaml` and `empty.wav` with the cards and disks |
| the five waiting presets (C1600-4 §1), `examples/bundle.yaml` | version 2: `- floppy: { name: Formatted }` / `{ file: Progs.floppy.yaml }` in a first `keys:` section, `slot-N: { file: … }`, `host-drive: { folder: S3 }`, `- saveas: { place: slot-2, as: template, file: CE-1601M - Progs.card.yaml }` and `{ place: floppy, … }`; `startup/default-pc1600.pc1600` back into `startup:` |
| `CMakeLists.txt` (`calcu_add_preset_check(CalcU1600Presets …)`) | `EXCLUDE` empty |

**Deleted** (A-1 to A-3): `app/ModuleBay.*`, `FloppyBay.*`, `TapeBay.*`,
`MediaLibrary.*`, `DirectoryScanCache.hpp`; `core/Connector/MediaSave.hpp`
(`planMediaSave`, `mediaSaveRefusalText`, `writeFileAtomically`;
`formatCardSave` moves into the card codec); in
`core/Connector/NamedFileCatalog.hpp` `namedFileName` (39-56, the `/`
and `:` replacement, row 1600-5) and the scans that the media use;
`MemoryCardCatalog.hpp`'s `scanMemoryCardDirectory` and
`resolveModuleSpecByName`, `FloppyImageFile.hpp`'s `scanFloppyDirectory`
and `resolveFloppyByName` (the shelf); `core/Preset/PresetMedia.hpp`,
`core/PC1600/PC1600PresetMedia.hpp` (the CLIs save through the session
of `CliPresetRun`, LIB-6 §10.5).

### 10.3 The kinds (AC-37, AC-41, AC-42)

| | `battery-backed-module` | `floppy-disk` | `cassette` |
|---|---|---|---|
| suffix, version | `.card.yaml`, 1 (`MemoryCardDefinition.hpp:25`) | `.floppy.yaml`, 2 | `.tape.yaml`, 1 (`media.md` §3) |
| name tag | `module-name` | `disk-name` | `tape-name` |
| folder | Save | Save | Tapes (LIB-9 AC-16) |
| bundled | `frontends/qt6/resources/cards/*.card.yaml` and `roms/*.card.yaml` (as `ModuleBay.cpp:36-50`) | `blank`, `formatted` | `empty` |
| held by | `ModuleSlot`; `isMedium` = a definition with `battery: true`; `moduleRequest` = the slot's `PeripheralRequest` with a `ModuleCard` built from the definition and its content | `MediaPlace` `floppy` (sides 0, 1) | `MediaPlace` `cassette` |
| write-protect | where the definition has a `write-protect:` region (CE-161) | — | — |
| companions | — | — | the WAV named by `audio: { file: }` |
| words | — (slot keys) | `floppy`, eject word `floppy` | `tape`, eject word `tape` |

- **Card codec.** `read`: `parseMemoryCardDefinition` (1345-1423); the
  info's content is the definition with its content. `write`: with
  `source`, `spliceBatteryCardInstance` (`BatteryCardInstance.hpp:250-341`):
  the file kept but for content, name and template mark (AC-41); without
  one (an import is byte for byte; a first save of a bundled template),
  the template's file spliced. The header lines `# created:`,
  `# last-saved:` take `Write::savedAt`.
- **Floppy codec.** `read`: `parseFloppyFile` with sides `0`, `1`;
  version 1 → `Outdated` with the hint "Floppy disk format version 1 …
  described in `preset.md` §9.4 / the User Guide". `write`: whole, its
  description kept (`FloppyBay.cpp:154`).
- **Tape codec** (new): `read` the definition (`format: cassette-tape`,
  `format-version: 1`, `tape-name`, `description`, `template`, `audio`,
  `position` = 0, else an error) and the WAV through `Companion`
  (`core/Audio/WavFile.hpp:53`); `write` the definition and the WAV
  (mono 16-bit, 48000 Hz, `writeWavMono16`, `:14`) side by side, the WAV
  named after the file stem (`<stem>.wav`). A definition written by
  hand may name any relative WAV; it is read, and written back to that
  same path.
- **AC-37** is otherwise today's format: the name keys, `template:
  true`, the module definition at version 1.

### 10.4 The places (AC-38, AC-39, AC-42)

- **Module slots (AC-38).** `media.choose` in `slot-1`/`slot-2` makes
  LIB-18's `Replace` (LIB-18 AC-29: a power cycle, the memory kept,
  `NEW0? :CHECK`); the other slot and the disk are not touched (row
  1600-13). The picker is AC-8's: the empty slot, the bundled templates,
  the user's templates, the instances, then the modules that are not
  media (the ROM modules: CE-163F, CE-1638 …), each only where it fits
  the slot (LIB-2 AC-2).
- **The drive (AC-39).** Holding, sides 0 and 1. The side button sends
  `media.turn` and is labelled with the side up, "0" or "1"; the lamp
  is `FloppyDriveAccess::motorOn()`; Name & Save for a template. A disk
  inserted into a drive that is not plugged in is not possible: the
  place exists while the CE-1600P is plugged.
- **The cassette (AC-42).** Holding; `takeWriteNow()` true when the
  motor stops after recording and at eject, so the WAV is written then
  (D-11); a failed write is the notice of A8. The empty tape is a
  template: recording onto it keeps the sound in the place, `unsaved`,
  until Name & Save (A6). The record button of today ("save = a new
  tape to record", `TapeBay::recordNew` 64-72) becomes `media.start-new
  {place: cassette, template: Empty, name}` (AC-20): the name is asked
  first, the files are written at the first motor stop. The picker
  lists the tape definitions only; a WAV of the tape folder without a
  definition is not a tape (row 1600-18).
- **ModuleMedium.** A battery-backed module's card offers `content()`
  (its RAM as the definition's content) and `revision()`
  (`contentRevision()`); autosave by A1 (half a second; AC-44's "the
  half second").

### 10.5 Presets, the harness, the Web (AC-43)

- `addMediaToPresets(presets.vocabulary(), media)` gives `floppy:`,
  `tape:`, `eject:`, `saveas:` and the slot resolver; `SharpModelRules::applies`
  marks `floppy:` "[1600: PC-1600 with CE-1600P]" and `tape:` "[1600:
  with CE-150 or CE-1600P]" by `ReadContext::plugged` (LIB-6 AC-47's
  last clause).
- A slot's ROM module built by `slot-N-rom:` (C1600-4 AC-8) is never an
  instance: its request's card is no shelf entry, so the part binds
  nothing to it and `saveas:` of that slot fails ("A module built from a
  ROM file is never saved.").
- The harness: `HarnessProduct::mediaKinds()`; the run's save folder as
  LIB-11 AC-8 gives it; `media`, `media import|export|delete` and the
  steps as commands (§3.11).
- **AC-43.** The Web's import takes `.card.yaml`, `.floppy.yaml` and a
  `.tape.yaml` with its WAV (both chosen in one picker, the WAV as the
  companion; a definition without its WAV is refused naming it); the
  saved media list is `CalcuSavedMediaDialog`; delete asks first (row
  1600-13).

### 10.6 Tasks

Tests come before the code. `CoreTests` (Frequent) unless the row says
otherwise; the cases run the product's app session (`app/tests/SessionFixture.hpp`)
with a save folder and a tape folder of their own, or the harness entry
with `--save-folder`. Names `lib7_acN_…`.

| Task | What | ACs | Test (file: cases, group) |
|---|---|---|---|
| P1 | Codecs | AC-37, AC-41 | `media_kinds_tests.cpp`: `lib7_ac37_three_kinds_suffixes_versions_name_keys`, `lib7_ac37_floppy_v2_sides_0_1_v1_outdated_with_hint`, `lib7_ac37_card_v1_read_and_written`, `lib7_ac41_card_file_kept_but_content_name_template_mark`, `lib7_ac41_disk_written_whole_description_kept` |
| P2 | Module slots | AC-38 | `lib7_ac38_picker_fits_the_slot_in_ac8_order`, `lib7_ac38_choose_is_replace_power_cycle_memory_kept`, `lib7_ac38_other_slot_and_disk_untouched` |
| P3 | Drive | AC-39 | `lib7_ac39_turn_is_eject_and_insert_other_side`, `lib7_ac39_lamp_while_motor_runs`, `lib7_ac39_name_and_save_for_a_template_disk` |
| P4 | Name & Save | AC-40 | `lib7_ac40_instance_name_replaced_after_asking`, `lib7_ac40_name_with_slash_or_colon_refused`, `lib7_ac40_write_protect_power_cycle` (CE-161, row 1600-17); the dialogs' texts: `calcu_qt6_tests` `lib7_ac40_qt6_prompts_instance_and_disk_name` |
| P5 | Tapes | AC-42 | `lib7_ac42_definition_with_wav_is_a_medium`, `lib7_ac42_empty_tape_template_offered`, `lib7_ac42_start_new_records_written_at_motor_stop_and_eject`, `lib7_ac42_name_and_save_copies_definition_and_wav_side_by_side`, `lib7_ac42_wav_alone_not_listed`, `lib7_ac42_hand_written_definition_any_relative_wav`, `lib7_ac42_preset_tape_and_eject_tape`, `lib7_ac42_counter_position_and_length_green_while_motor` |
| P6 | Web | AC-43 | `play.spec.js`: `lib7_ac43 import card disk and tape with wav`, `lib7_ac43 saved media list`, `lib7_ac43 delete asks first` (Longer) |
| P7 | Today's tests | AC-44 | the cases of AC-44's list ported onto the topic and commands as `lib7_ac44_<old name>`: `app/tests/shared_bay_tests.cpp:52-168` (module, floppy, tape bays; the scan cache goes), `shared_app_session_tests.cpp:109-340` (tape status, control state, media library, card write-protect), `tests/battery_card_instance_tests.cpp:225-513` (splice, templates, saves), `tests/tape_deck_tests.cpp:91-342`, `tests/ce1600f_tests.cpp:408-485` (the floppy file now version 2; `:446` expects version 1 refused as outdated), `play.spec.js:203-348`, with AC-38's and AC-40's changes |
| P8 | GUI scenarios | AC-45 | Qt6: `CalcU1600Shots.lib7-ac16-module-template-saved` and `CalcU1600Shots.lib7-ac8-picker-lists-instance` (screenshot scenarios run as checks; Longer); Web: `play.spec.js:236-266` as `lib7_ac16 module template saved under a name is an instance` and `lib7_ac8 picker lists it among the instances` (Longer) |
| P9 | The presets that waited | C1600-4 AC-15, AC-16; LIB-6 AC-47 | `calcu_preset_check.CalcU1600Presets` without exclusions; `CalcU1600ExampleReferences` with the four examples (Longer; DiskWorks: the exit check of M-47, its display and the disk's content; `make_diskworks_media`: the files its `saveas` wrote, without time stamps); `c1600_4_ac15_no_version_1_left` without exceptions; `lib6_ac47_floppy_without_ce1600p_refused` |
| P10 | Places of LIB-1 | LIB-1 AC-33, AC-40 | `descriptor_tests.cpp`: `lib1_ac40_one_media_place_capability_per_media_place` (`media::checkMediaPlaces` on each model with each peripheral) |

Coverage: P1–P8 cover AC-37 to AC-45.

### 10.7 Tools and agent knowledge (M-47)

| File | Change |
|---|---|
| `products/calc-u-1600/CLAUDE.md` | the Harness section: media are read (`media`, `floppy`, `tape`, `eject`, `saveas` as commands); "Not yet: … media" goes |
| `products/calc-u-1600/docs/developer/Floppy-Image-Format.md` | version 2, sides `0:`/`1:` (§4, the example 94-120); the upgrade by hand |
| `products/calc-u-1600/docs/developer/Shared-Architecture.md`, `Web-Client.md` | the bays' rows → the library's media part and `app/SharpMediaKinds` |
| `products/calc-u-1600/docs/User-Guide.md` | the media chapters: tape definitions, the empty tape, Name & Save asks before replacing, sides 0 and 1, "Save folder" |
| `products/calc-u-1600/docs/Memory-Card-Definition-Format.md` | unchanged (version 1); the save folder's name |
| `docs/glossary-mapping.md` | rows "bay", "Card Folder", "Battery-card saves" applied (`ModuleBay`, `FloppyBay`, `TapeBay` → `MediaPart`; "Battery-card saves:" → "Save folder:") |
| `docs/test-groups.md` | the new and ported files; `CalcU1600ExampleReferences` with all 32 |
| memory `calc-u-1600/project_floppy_yaml_format.md` | version 2, sides 0/1; found through the shelf (bundled first, then the save folder); "–empty–" is the empty entry of the picker |
| memory `calc-u-1600/project_cassette_tape.md` | tapes are tape definitions with their WAV in the tape folder; the empty tape template; `TapeManager` gone |
| memory `calc-u-1600/project_pre_1_0_format_decisions.md` | "one saveas pipeline (MediaSave.hpp)" → the library's `planSave` and the codecs |
| memory `calc-u-1600/feedback_machine_config_on_control_bar.md` | the module pickers are the media pickers of the topic `media` |
| `calc-u-1600-memories.md` (index) | the descriptions of these four entries |

**Ledger rows that become visible in M-47:** 1600-1 (the disks of
version 2), 1600-3 (the media steps `floppy:`, `tape:`, `eject:`), 1600-5,
1600-6, 1600-13, 1600-17, 1600-18, 1600-19, 1600-31 (plan §5.2). No new
row: the Qt6 Name & Save dialogs keep their prompts, and the record
button asks the name first as today.

### 10.8 Golden and reference runs, risks

- **Golden runs** `pc1500a-ce151` and `pc1600-ce1600m` plug RAM modules
  through the builder; the card is now built by the codec's request
  (`moduleRequest`), the same definition. A difference is a finding.
- **Reference runs** of the presets that waited (P9): DiskWorks and
  `make_diskworks_media` include the content of the disk and the module
  and the files `saveas` wrote; the `.meta` files of M-4 list them.
- **Floppy version 2.** A user's 0.8.x disk is shown as outdated with the
  hint (rows 1600-6, 1600-19); it is not converted (`preset.md` §9.4 by
  hand).
- **Tape recording moment** (D-11): the WAV is written at each motor
  stop as today; recording onto the empty template writes nothing until
  Name & Save or `start-new`, which is today's flow (a name first) in
  another order of clicks.
- **Hidden tab** (§8): the Web's `suspending` write is checked by P6's
  reload case.
- **The ROM full-set files** in `roms/` that `ModuleBay.cpp:36-50` lists
  (the CE-502B's definition) stay modules that are not media
  (`battery: false`).

### 10.9 Decided (best guess)

- **D-22 (kind IDs).** The descriptors' medium kinds
  (`battery-backed-module`, `floppy-disk`, `cassette`) are the
  `KindDecl` IDs; §4's names were placeholders. The slots leave the
  descriptors' `mediaPlaces` (D-1).
- **D-23 (the empty tape).** A bundled template `empty.tape.yaml`
  ("Empty", "A blank cassette") with an `empty.wav` of no samples (a
  header only, mono 16-bit, 48000 Hz), so that the empty tape is a file
  like the other templates and the codec has one path.
- **D-24 (recording on a template).** The record button is
  `media.start-new` on the empty tape (AC-20), so a recording always has
  a name before its first write, as today.
- **D-25 (the dialogs).** The library's Name & Save dialogs in Qt6 and on
  the Web from M-47, with today's prompts (AC-40); the control-bar
  pickers keep today's widgets, filled from the topic, until LIB-14
  1600 (M-49) puts the library's pickers in.
- **D-26 (a hand-written tape definition).** Read and written back with
  the WAV path it names; Name & Save writes the copy's WAV next to the
  copy (`media.md` §3).

### 10.10 Questions for the user

None that splits this column.

## 11. Part E: the Calc-U-59 column

Approved by the user at the Part D/E boundary (2026-11-16). Paths without a prefix are
under `products/calc-u-59/`; line numbers at `7bdd51c`. It replaces §4's
Calc-U-59 table as the plan of M-55; where it differs from that table it
says so. The card itself (its content, the card reader, the format, the
header key `magnetic-cards:`, fitting and the cue cards) is C59-7's
design; this section holds the media model's side and the library
additions the product needs.

### 11.1 Scope

| Column | Step | Tests from (plan §3) | Done in |
|---|---|---|---|
| Calc-U-59 | M-55 (with C59-7) | AC-32–36, AC-47 | proposed M-57 (D-29; plan §5.1 says M-55) |

- **Waits for** M-41 (Library column, Done in M-42), M-54 (LIB-6 59,
  LIB-18 59, C59-2, C59-3: the library's loader and `MachineControl`
  for every Calc-U-59 preset and start; `TIAppPart` filling a
  `parts::LibraryPartsSetup`), M-3 (FS 10) and M-9 (the four card
  examples' recordings), as the plan says. It uses M-53 (LIB-10 59).
- **Also passes here** (plan §5.1): LIB-1 AC-33 for the TI-59's card
  reader (one media place capability per media place, so LIB-1 59 is
  Done here); LIB-15 AC-20's cards (LIB-15 59 Done here); C59-2 AC-10's
  four card examples; LIB-3 AC-29's `CardChanged` row (the media
  topic); LIB-11 AC-22's `card { mem: X, side: 0 }` on a real product.
- **Where the library built in C2 and Part D differs from §3**, this
  column uses the built shapes (qa-report, "Deviations"; Part D
  simplification P-3): the binding of a pass-through place is the fitted
  medium (D-22); `media.start-new` on a pass-through place fits the
  medium; the slot resolver is `Vocabulary::setPlaceResolver`; the
  markers come from `ModelRules::applies`; `closing` comes from
  `AppSession::close()`; `addMediaCommands` takes the run's part;
  `HarnessProduct` gives the parts through `prepare()` and
  `libraryParts()` (LIB-11 §D.3, superseded paragraph).
- **Kind ID.** The descriptor already names the kind
  (`core/TICatalogue.cpp:89-90`): `magnetic-card`, the place
  `card-reader`, pass-through, sides 0 and 1. §4's file
  `app/TIMediaKinds.*` is C59-7's `app/TIMagneticCardKind.*`.

### 11.2 Modules and files

**Library additions** (each with a test in `library/media/tests/`, in
`calcu_library_tests`; they change nothing for Calc-U-1600, whose kinds
keep the defaults):

| File | Change | For |
|---|---|---|
| `library/media/Kind.hpp` (`KindDecl`) | `bool rereadBeforeWrite = false`: the part reads the instance's file just before each write and hands its bytes to the codec as `Write::source` | C59-7 AC-5, EC-3 (D-27) |
| same | `bool offersNameAndSave = true`: false, and the place's `nameAndSave` is never set for a medium of the kind | C59-7 AC-13, AC-20 (D-28) |
| same | `std::string replaceQuestion`: the `asks:` question of AC-17 row 5 and AC-20 with `{name}` for the name; empty: the library's sentence ("A <label> named “<name>” exists. Replace it?", `library/media/SaveRules.cpp:85-86`, `MediaPart.cpp:1075`) | C59-7 AC-15 (D-31) |
| `library/media/FileAccess.hpp` (new), `MediaPart.hpp:32-40` (`MediaSetup::files`), `Shelf.cpp`, `Writer.cpp:41-47` | `media::FileAccess { list(folder), read(path), write(path, bytes) }`, default the plain one (`std::filesystem` and `settings::writeFileAtomically`, as now); the shelf and the writer go through it | Swift's iCloud coordination (C59-5 D-6; D-30) |

If M-54 made a host file seam for the constant memory file in
`library/settings/`, `FileAccess` is that seam and `MediaSetup::files`
takes it; no second one is made.

**Product** (C59-7 §2 has the full list):

| File | Change |
|---|---|
| `app/TIMagneticCardKind.hpp/.cpp` (new) | the `KindDecl` `magnetic-card` and the format 3 codec (C59-7 §3.3) |
| `core/TICardReaderPlace.hpp/.cpp`, `core/TIMagneticCard.hpp/.cpp` (new) | the `MediaPlace` of the card reader; the card's content (C59-7 §3.1, §3.2) |
| `app/TIAppPart.*` | the app layer's `LibraryPartsSetup` gets the media part's setup: the kind, the module listing (C59-6 AC-8: `description`, no tooltip, `menu-sort`), the save folder of LIB-9's `Paths`, the host's `FileAccess`; the interim card flow goes (A-4) |
| `app/TIFileAccess.*` | the card calls go (A-5); the file goes whole when M-54 has taken the constant memory out |
| `app/TIAppPartInternal.hpp:34-38` | `waitsForMedium` of `Hooks` goes: `MediaPart` is the `WaitProbe` (A-6; LIB-17 §5) |
| `core/TIAdapters.hpp:43-59, 154, 158`, `.cpp` | `TICardReaderAdapter`, `cardReaderOf` go (A-6) |
| `app/HarnessProduct.cpp` | the run's parts are the app layer's (LIB-11 §11), so `card`, `arm`, `saveas { card: … }`, `media`, `media import|export|delete` work with the kind |
| `frontends/qt6/app/AppPaths.hpp:29-30`, `.cpp` | `cardDir()` goes (A-7) |
| `tests/descriptor_tests.cpp:147, 195` | the `lib1_ac33_…` cases check the card reader's capability with `media::checkMediaPlaces` (LIB-1 AC-33) |

### 11.3 The product's side (AC-32 to AC-35, AC-47)

- **AC-32.** One kind, `magnetic-card` (`.magnetcard.yaml`, format 3);
  one media place, the TI-59's `card-reader` (pass-through, sides 0 and
  1); no bundled card files; one template, the implicit "Fresh card"
  (C59-7 D-10), which has no file.
- **AC-47.** The `slot-1` picker of every TI model is AC-8's with one
  group: the empty slot and the 14 Solid State ROM modules (not media:
  no kind is `HeldBy::ModuleSlot`), labelled by `description`, ordered
  by `menu-sort` (the `ModuleListing`). Choosing sends `media.choose
  {place: slot-1, entry}`: a replace (an attach, or a detach for the
  empty entry) through LIB-18's `MachineControl`, followed by the power
  cycle of LIB-18 AC-21 (rows 59-9, 59-11). The frontends' module menus
  (the Qt6, Web and Swift settings) are filled from it in M-55 (C59-6
  §5, the adapter "module list from module files for today's menus");
  the control-bar picker comes with LIB-14 59 (M-57) and C59-8 (M-58).
- **AC-34.** A card read and written through the reader is written when
  it is given back (A3 with the place's `takeGivenBack`); what the write
  keeps of the other side is C59-7's (the re-read of D-27).
- **AC-35.** `card:` (with `side: 0|1`) and `arm:` (no side) take `{
  name: }`, `{ file: }` or `{ mem: }`; a missing `name:` or `file:`
  refuses the preset (AC-23; today a blank card goes in,
  `app/TIAppPartFlows.cpp:500`; D-32); an undefined `mem:` is a fresh
  in-file card; a card file is written in place unless it is a template
  (AC-25).

### 11.4 Frontends (AC-33, AC-36)

- **Qt6** (C59-7 §4): the card dialogs and the control bar on the media
  topic; the save folder is LIB-9's (`AppPaths::cardDir()` goes). No
  saved media list (OQ-4).
- **Web**, AC-36: the import (`media.import`, AC-30) and the saved media
  (`CalcuSavedMediaDialog`, AC-31, wrapped as the Web test product does)
  replace the per-card download and the upload in the card picker
  (`frontends/web/src/Dialogs.jsx:62, 94, 100`; row 59-43). Calc-U-59's
  page has no File menu before LIB-14 59 (M-57), so in M-55 they are
  reached from the card dialog ("Import…", "Saved Media…"); M-57 puts
  them into the File menu as AC-29 says (D-29). The cards are files of
  the persistent save folder since M-34 (LIB-15 §9); the card list of
  `TIWebPart` (`app/TIWebPart.cpp:396`) and its commands go.
- **Swift** (named for M-58): on the media topic through the bridge in
  M-55, so that AC-33 holds for its iCloud save folder; drawn natively
  with the family structure in C59-8 (M-58). The coordinated file access
  is the bridge's `TICoordinatedFiles` as the media part's `FileAccess`
  (D-30).

### 11.5 Adapters removed (M-55)

| Adapter (§5) | Result |
|---|---|
| A-4 interim TI card flow (`c59.card-reader`, `c59.fit-card`, `c59.fit-blank-card`, `c59.eject-card`) | gone; also `c59.feed-side` and the Web's `c59.web.fit-card`, `c59.web.fit-blank-card`, `c59.web.card-bytes`, `c59.web.fresh-card-name`, `c59.web.card` |
| A-5 `TIFileAccess` cards | gone |
| A-6 `TICardWaitAdapter`, `TICardReaderAdapter` | gone |
| A-7 Qt6 `AppPaths::cardDir()` | gone |

No new adapter. A card file dropped before M-56 is not an adapter: the
interim drop flow sends the library's `media.open-file` (C59-7 §3.7).

### 11.6 Tasks

Tests before the code. `CalcU59Tests` (Frequent) unless the row says
otherwise, on `PartRig` with a save folder of the test's own, or the
harness entry with `--save-folder`; `SKIP_UNLESS` the ROMs where the ROM
runs. Names `lib7_acN_…`; the library additions `acN_…` in
`calcu_library_tests`.

| Task | What | ACs | Test (file: cases) |
|---|---|---|---|
| E1 | Library additions | AC-1, AC-12, AC-16, AC-17 | `library/media/tests/save_tests.cpp`: `ac17_kind_replace_question_words`, `ac16_kind_without_name_and_save_never_offers_it`; `autosave_tests.cpp`: `ac12_reread_before_write_hands_current_file_as_source`; `shelf_tests.cpp`: `ac1_shelf_and_writer_through_the_hosts_file_access` (a recording `FileAccess`) |
| E2 | The kind on Calc-U-59 | AC-32 | `tests/media_tests.cpp` (new): `lib7_ac32_one_kind_one_place_pass_through_sides_0_1`, `lib7_ac32_no_bundled_cards_fresh_card_template_without_file` |
| E3 | Module slot picker | AC-47 | `lib7_ac47_slot_picker_empty_and_14_modules_by_menu_sort_description_label` (each model), `lib7_ac47_choose_replaces_and_power_cycles` |
| E4 | Save folder per frontend | AC-33 | `lib7_ac33_names_and_collisions_for_cards` (AC-1 to AC-5 on cards: template or instance by the file, the name inside, AC-3's rules, `foo-1`); Qt6: `lib7_ac33_qt6_cards_in_lib9_save_folder` (the app's paths, no window); Web: `storage.spec.js` `lib7_ac33 cards in the persistent save folder` (Longer); Swift: `Calc-U-59Tests` `lib7_ac33_swift_cards_in_icloud_save_folder` through the coordinated access (run from the main checkout) |
| E5 | Written when given back | AC-34 | `lib7_ac34_instance_written_when_given_back`, `lib7_ac34_read_pass_writes_nothing` |
| E6 | Presets | AC-35 | `lib7_ac35_card_and_arm_by_name_file_mem`, `lib7_ac35_missing_name_or_file_refuses`, `lib7_ac35_undefined_mem_fresh`, `lib7_ac35_card_file_written_in_place_unless_template`; the harness: `lib7_ac35_harness_card_mem_side_and_saveas_card` (LIB-11 AC-22) |
| E7 | Web | AC-36 | `play-calculator.spec.js` (Longer): `lib7_ac36 import a card file`, `lib7_ac36 saved media lists downloads deletes after asking`, `lib7_ac36 no per card download or upload in the card picker`; `lib7_ac36 file menu has import and saved media` in M-57 |
| E8 | LIB-1 59 | LIB-1 AC-33 | `tests/descriptor_tests.cpp:147, 195`: the two `lib1_ac33_…` cases with the card reader's media place capability (`media::checkMediaPlaces`) |
| E9 | LIB-15 59 | LIB-15 AC-20 | `storage.spec.js`: `ac20 cards survive a page load` (a card written by a program, the page reloaded, the card dialog lists it) (Longer) |

Coverage: AC-32 to AC-36 and AC-47 (6 of 6); AC-36's File menu clause
in M-57 (D-29).

### 11.7 Tools and agent knowledge (M-55)

- **Today's tools.** `tools/headless/capi.cpp:82, 213, 231-242` uses
  `TICardReaderAdapter` (`h_card_insert`, `h_card_eject`,
  `h_card_present`, `h_card_mode`, `h_card_waiting`, `h_lib_exec_pc`).
  By plan §1's tools rule they move in M-55: the `h_card_*` calls onto
  the card reader's `MediaPlace` (`insert` with a content of one side,
  `eject`, `side`, `waitsForMedium`), `h_lib_exec_pc` onto the debug
  target, so `ti59.py`'s `card_insert`, `card_eject`, `card_present`,
  `card_waiting` and `coverage.py`'s card scenarios
  (`tools/headless/coverage.py:233-239`) keep working until M-59 removes
  the tool (LIB-11 §11).
- **Agent knowledge** (plan M-55): the `calc-u-59-statefiles` skill
  (cards), `reference/StateFileFormat.md` (§ "Card files"), the memory
  files on cards (`calc-u-59/card_timing_audio.md`,
  `cuecard_template_semantics.md`, `solid_state_pc_display.md`), the
  skills `calc-u-59-shared`, `-qt`, `-web`, `-swift`, `-core`; the
  `docs/glossary-mapping.md` rows "Magnetic card" (`CardDocument`,
  `CardFile`, `.u59`) and "Medium, kind of medium, media place"
  (`CardFile`, `TIFileAccess`) marked done. C59-7 §7 lists the lines.
- `docs/test-groups.md`: the new tests and Web cases.

### 11.8 Golden and reference runs, risks

- **Golden runs** do not use the media part; they pass unchanged.
- **Reference runs** of the four card examples (C59-7 T12): the in-file
  banks come from `MediaPart::inFileContent` instead of
  `TIAppPart::memoryCardBanks()` (`app/TIAppPart.hpp:316-322`); a
  difference is a finding.
- **The re-read** (D-27) changes when a file is read, not what is
  written for the side that passed; LIB-7's own tests keep the default.
- **`FileAccess`** (D-30): iCloud placeholders (`.<name>.icloud`) are
  hidden files and do not end in the kind's suffix, so the shelf passes
  them over, as `isCardFileName` does today (`app/CardFile.hpp:95-97`).
- **The Web without a File menu until M-57** (D-29): every function is
  there from M-55; only AC-29's place of the two items waits.

### 11.9 Decided (best guess)

- **D-27 (C59-7 AC-5, EC-3; AC-12).** A kind may ask for its instance's
  file to be read again just before each write and handed to the codec
  as the source (`rereadBeforeWrite`); the card uses it so that a write
  keeps the other side as the file holds it. The other kinds keep
  today's rule (the source is the bytes last read or written).
- **D-28 (AC-16; C59-7 AC-13, AC-20).** A kind may say that it offers no
  Name & Save (`offersNameAndSave`); the magnetic card does not, so no
  frontend draws it for the card reader.
- **D-29 (AC-29, AC-36; plan §1).** Calc-U-59's Web page has no File
  menu before M-57; in M-55 the import and the saved media are reached
  from the card dialog, and M-57 moves them into the File menu. The
  column is Done in M-57 (plan note); every function passes in M-55.
- **D-30 (AC-33; C59-5 D-6).** The media part reads and writes the save
  folder through a file access the host may hand it
  (`MediaSetup::files`), so that Swift keeps its coordinated iCloud
  access; the default is today's plain access.
- **D-31 (AC-17, AC-20; C59-7 AC-15).** A kind may word the replace
  question itself (`replaceQuestion`), because C59-7 AC-15 gives the
  card's words.
- **D-32 (AC-23, AC-35).** A `card:` or `arm:` step that names a card
  file or name that does not exist refuses the preset, as AC-23 and
  AC-35 say, where 2.3.0 put a blank card in and saved it under that
  name; a new card in a preset is an in-file card saved by `saveas:`
  (C59-7 AC-20, AC-30's upgrade row). Ledger row proposed (§11.10).

### 11.10 Proposed ledger rows and plan notes

Ledger rows that become visible in M-55 (plan §5.2): 59-1 (cards),
59-6, 59-8 (cards kept), 59-17 (C59-7 AC-25), 59-20 (cards), 59-24,
59-27, 59-43 (the Web's import and saved media). New rows (numbered and written
into `docs/behaviour-changes.md` at the D/E boundary, 2026-11-16):
- **59-49** — In a preset, `card:` or `arm:` with a card file or name
  that does not exist refuses the preset (today: a blank card goes in and
  is saved under that name at its first write); a new card in a preset
  is an in-file card of `magnetic-cards:`, saved by `saveas:`. (LIB-7
  AC-23, AC-35; C59-7 AC-20; D-32.)
- **59-50** — (with the answer to C59-7 Q-1: "Arm Card") The card buttons, menu
  items and dialogs read "Arm Card" and "Arm Fresh Card" (today "Fit
  Card", "Fit Fresh Card"). (`docs/glossary-mapping.md` row "Arm"; C59-7
  Q-1.)
- **59-51** — The card dialogs list the saved cards by the names
  inside them (today: by file name, `card.U59` by default); a card's
  file is named after its name (`<name>.magnetcard.yaml`). (AC-2, AC-4;
  C59-7 AC-14.) It adds to 59-6.

Plan notes (for the orchestrator; the plan is not edited here):
- §5.1: LIB-7 Calc-U-59 "Done in" M-57 (D-29); C59-7 "Done in" M-58
  (C59-7 D-11).
- M-55 "Waits for": also M-53 (the debug target that takes over the card
  reader adapter's `libExecPC`, C59-7 §3.6).
- M-55 "Knowledge, ledger": add 59-43 and the proposed rows above.
