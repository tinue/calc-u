# LIB-7 Media and save folder: design
Status: approved by the user at the Part C1/C2 boundary (2026-11-15)

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
    std::optional<Held> holds;  // unsaved: a template written to (AC-15) or a started name not yet written (AC-20)
    std::optional<std::string> armed;     // a pass-through place: the armed medium (AC-27)
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
| `media.choose` | `{place, entry}` | writes what the old medium has to save (AC-13); media place: eject, insert; module slot: (op) LIB-18's attach or replace with the medium's request, what follows as LIB-18 says; unreadable: the place empty and a notice | AC-8, AC-10 |
| `media.eject` | `{place}` | AC-13, then eject or (op) detach; an empty place: nothing (EC-6) | AC-13, EC-6 |
| `media.turn` | `{place}` | eject, insert the other side | AC-6 (Calc-U-1600 AC-39) |
| `media.arm` | `{place, entry}` | a pass-through place: the medium armed, not inserted | AC-26 |
| `media.feed` | `{place, side}` | the armed medium inserted with that side (the corner tap of C59-7 sends it) | AC-26 |
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
for a module slot) and binds it to the file where it lies, or to the
imported instance.

**Started under a name** (AC-20): `media.start-new` runs `planSave`
for Name & Save (asking before an instance is replaced), inserts the
template's content, and binds the place to the instance file to come;
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
| `<insertStep>` pass-through | insert with the side; waits within `timeout:` (LIB-6 AC-57) until the place gives the medium back; A3; bind | when given back (and fitted, C59-7) |
| `<armStep>` | the medium armed for the place; nothing inserted | at once |
| `eject` | A5; eject; an empty place: nothing (EC-6) | at once |
| `saveas` | `planSave` with `SaveasLive`/`SaveasTemplate`; `protected:` on a medium without a switch fails with that reason (EC-7); any refusal fails the preset (LIB-6 AC-17) | at once (AC-19's power cycle: when ready) |

- A medium named by `name:` or `file:` is a template when its file says
  so and an instance otherwise, also when named by `file:` (AC-25): an
  instance is written in place, so a preset run again starts from what
  the last run left.
- Each step logs one line with the kind's label and the name tag
  (AC-25; LIB-6 AC-18).
- AC-27: in `beforeMachineChange`, an armed medium of a pass-through
  place that never passed gives one log line "<label> “<name>” was armed
  and never fed."

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
| L3 | `MediaPlace`, `ModuleMedium`, test machine, conformance | AC-6 | `place_tests.cpp`: `ac6_place_states_kind_passage_sides`, `ac6_insert_eject_side_content_revision`, `ac6_waits_for_medium`, `ac6_write_protect_where_the_medium_has_one`; `library/machine/tests/conformance_tests.cpp`: `ac40_media_place_capability_per_media_place` (LIB-1 D-1); Frequent |
| L4 | topic `media` | AC-7 | `topic_tests.cpp`: `ac7_kinds_templates_instances_with_place`, `ac7_place_holds_and_waits`, `ac7_reread_after_own_write_and_on_refresh`, `ac7_outside_change_not_seen_until_refresh`; Frequent |
| L5 | pickers, choose | AC-8, AC-9, AC-10 | `picker_tests.cpp`: `ac8_order_empty_templates_instances_modules`, `ac8_tooltip_is_description`, `ac8_listing_label_and_sort_key` (a `ModuleListing` that labels by description in `menu-sort` order), `ac9_outdated_entry_disabled_with_hint`, `ac9_outdated_logged_once_per_start`, `ac10_old_medium_written_first`, `ac10_holding_place_new_medium_in_at_once`, `ac10_module_slot_is_a_replace_by_lib18`, `ac10_unreadable_leaves_place_empty_with_notice`; Frequent |
| L6 | autosave | AC-11–15, EC-1, EC-2, EC-8 | `autosave_tests.cpp`: `ac11_written_within_half_a_second_at_60_hz`, `ac11_not_written_while_nothing_changes`, `ac11_harness_written_at_end_of_command`, `ac12_pass_through_written_when_given_back_if_written`, `ac13_written_before_leaving_place`, `ac13_written_before_operation`, `ac13_written_before_machine_change`, `ac13_written_on_suspend`, `ac13_written_on_close`, `ac14_failed_write_one_notice_until_success`, `ac14_next_change_tries_again`, `ac15_picker_template_not_written_lives_in_place`, `ac15_preset_named_template_not_written`, `ac15_in_file_medium_not_written_unless_saveas`, `ec1_save_folder_made_at_first_write`, `ec2_outside_change_overwritten_not_merged`, `ec8_suspend_writes_every_pending_change`; Frequent |
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
