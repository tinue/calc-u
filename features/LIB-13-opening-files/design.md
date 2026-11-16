# LIB-13 Opening files: design
Status: Library column approved at the Part C1/C2 boundary (2026-11-15); Calc-U-1600 column approved at the Part C2/D boundary (2026-11-16)

## 1. Scope

| Column | Step | Tests from (plan §3) | Done in (plan §5.1) |
|---|---|---|---|
| Library | M-42 | AC-1 to AC-10; EC-1 to EC-7 | M-42 |
| Calc-U-1600 | M-48 | AC-17 to AC-22 | M-48 |
| Calc-U-59 | M-56 (with C59-4) | AC-11 to AC-16, AC-23 | M-56 |

Part C2 works on the Library column only. The product columns are
sketched in §4 and set out at the Part D and E boundaries.

M-42 waits for M-38 (LIB-6 L: the preset loader, a preset from a folder
or as its content, LIB-6 EC-5) and M-41 (LIB-7 L: media, import, Name &
Save, inserting). It uses M-35 (LIB-18 L: attaching a module, as the
slot's picker does) through LIB-7, and M-37 (LIB-4: the action `open`,
whose availability the control state gives). The Library column is shown with the test catalogue,
two file kinds built for the tests, and stand-ins for LIB-6 and LIB-7.
It is also shown once through the real parts of LIB-6 and LIB-7 (§6,
L12).

What the library criteria say about frontends (AC-5, AC-6, AC-8) is
tested at the session: each way in sends the one command, marked with
its way in, and the outcome does not depend on the way in, apart from
the size limit (AC-10). The wiring of each frontend's drop, Open With,
start argument and dialog is a product criterion (AC-14, AC-21) and is
done in M-48 and M-56 (§4).

M-42 changes `library/`. In the products it changes one function
(`looksLikePreset`, §5).

## 2. Modules and files

Library (M-42), new folder `library/opening/` of `calcu_library`,
namespace `calcu::opening` (the glossary's verb: to open a file; D-1).
It includes `machine`, `sessions`, `presets` (LIB-6) and `media`
(LIB-7). Its harness command lives in `library/harness/`, as every C2
feature's does, so `opening` does not include `harness`. The line
`opening: machine, sessions, presets, media` is added to LIB-1 §2a's
dependency rule in the same step.

| File | Holds |
|---|---|
| `library/opening/OpenRequest.hpp` | `Way`, `OpenedFile`, `OpenRequest`, the command and request names, `OpenReply`, `WouldTake` (§3.1) |
| `library/opening/FileKind.hpp` | `FileKind`: what a product supplies per file kind (§3.3) |
| `library/opening/Openers.hpp` | `PresetOpening`, `MediaOpening`: what the command needs of LIB-6 and LIB-7 (§3.4) |
| `library/opening/LibraryOpeners.hpp/.cpp` | `PresetsOpener`, `MediaOpener`: the two over LIB-6's command `load-preset` and LIB-7's media part (its reads of opened files) and command `media.open-file` (LIB-7 §3.6, §3.7) |
| `library/harness/OpenCommands.cpp`, `Commands.hpp` (changed) | `addOpenCommand(CommandRegistry&)`: the harness command `open <path>` (AC-9) |
| `library/opening/Classify.hpp/.cpp` | the tests of AC-1 in their order; `looksLikePreset` (AC-2); `wouldTake` (AC-6) |
| `library/opening/OpenPart.hpp/.cpp` | `OpenSetup`, `OpenPart : sessions::SessionPart`: the command, the request, the file kept for later (§3.5) |
| `library/opening/tests/TestOpeners.hpp` | stand-ins for `PresetOpening` and `MediaOpening` that record their calls; two file kinds for the tests |
| `library/opening/tests/classify_tests.cpp` | AC-1, AC-2, AC-10, EC-1, EC-5 |
| `library/opening/tests/open_tests.cpp` | AC-3, AC-4, AC-7, AC-8, EC-2 to EC-4, EC-6, EC-7 |
| `library/opening/tests/ways_tests.cpp` | AC-5, AC-6 |
| `library/sessions/Operation.hpp`, `SessionPart.hpp`, `AppSession.cpp` (changed) | `OperationResult::result`; `SessionContext::startOperation` (§3.6; in the list of C2's LIB-3 additions, LIB-18 §8) |
| `library/sessions/tests/operation_tests.cpp` (changed) | `operation_result_payload_in_reply`, `part_starts_operation_without_command` |
| `library/harness/Run.cpp`, `Product.hpp` (changed) | the harness session's `OpenPart`; `addOpenCommand` with the first commands; `HarnessProduct::fileKinds()`, `openLimits()` |
| `library/harness/tests/commands_tests.cpp` (changed) | AC-9 |
| `library/CMakeLists.txt` (changed) | the sources of `library/opening/` and its tests |
| `products/calc-u-1600/core/DropFile.cpp` (changed) | its `looksLikePreset` calls the library's (§5) |

## 3. Seams and interfaces

Sketches. Single-threaded on the session's thread; nothing thrown
across the interface (LIB-1 §2a).

### 3.1 The command, the request, the reply (AC-1, AC-3, AC-6, AC-8, AC-9)

```cpp
namespace calcu::opening {

enum class Way { Drop, OpenWith, Start, Dialog, Harness };   // AC-5, AC-9, AC-10

struct OpenedFile {
    std::string name;                                  // the file's name, without folders
    std::optional<std::string> path;                   // where the host has it
    std::optional<std::vector<std::uint8_t>> bytes;    // nullopt: the host could not read it (EC-5)
};
struct OpenRequest {
    std::vector<OpenedFile> files;   // one; several only from a Web file dialog (AC-8)
    Way way;
    bool upload = false;             // the host cannot write the file back (a Web frontend, AC-4)
    bool confirmed = false;          // the user said yes to replacing a medium of that name (AC-4; EC-6; LIB-7 D-6)
};

inline const sessions::CommandName kOpenCommand{"open"};             // {files: [{name, path?, bytes?}], way, upload?, confirmed?}
inline const sessions::RequestName kWouldTakeRequest{"open-would-take"};   // {name, size?} → WouldTake

enum class OpenedAs { Preset, Medium, FileKind, Kept };
struct OpenReply final : sessions::PayloadOf<OpenReply> {      // AC-3
    static constexpr sessions::PayloadKind kKind{"open"};
    OpenedAs as;
    std::string detail;     // Medium: the media place, or "imported"; FileKind: its name
    // same, toValue
};
struct WouldTake final : sessions::PayloadOf<WouldTake> { static constexpr sessions::PayloadKind kKind{"open-would-take"}; bool takes; };
}
```

- A refusal is the reply `done = false` with its sentence, and the same
  sentence as a notice (AC-1: "A refusal is a notice that names the file
  and says why"). The sentence always names the file (EC-5).
- An open that starts an operation (a preset, inserting a medium, a
  product kind's load) is replied to at the operation's end (LIB-3
  D-2). The reply then carries the `OpenReply` through
  `OperationResult::result` (§3.6).
- A name already taken (AC-4, Web): a medium of that kind already has
  the name, and the rule of Name & Save is "asks before replacing"
  (LIB-7 AC-17, first column). The reply is LIB-7's `asks:` (a refusal
  whose reason begins with `asks:` and holds the question; LIB-7 §3.6,
  D-6), with no notice. The frontend asks the user (LIB-14 N4). On yes it
  sends the same command again with `confirmed: true`. On no it sends
  nothing, so nothing is imported and nothing is put into a place (EC-6)
  (D-4).

### 3.2 The tests of AC-1, in order (AC-1, AC-2, AC-10, EC-1, EC-5)

`classify(const OpenedFile&, Way, const OpenSetup&)` gives one of
Refused (with its sentence), Preset, Medium, Kind (which one) or
Refused. Names are compared in lower case (ASCII).

| # | Test | Then | Sentence (D-5) |
|---|---|---|---|
| 1a | `bytes` is absent | refused | "`<name>` could not be read." |
| 1b | larger than `maxBytes[way]` (AC-10) | refused | "`<name>` is larger than `<limit>` and is not opened." |
| 1c | the name starts with `.` | refused | "`<name>` is a hidden file and is not opened." |
| 2 | ends in one of `PresetOpening::extensions()` | a preset, whatever it holds (EC-1) | (LIB-6's refusal, with its hint) |
| 3 | ends in one of `MediaOpening::suffixes()` | a medium (§3.5) | (§3.5) |
| 4 | the first of `OpenSetup::kinds` whose `takes` is true | that file kind | (the kind's own) |
| 5 | `looksLikePreset(bytes)` (AC-2) | a preset, as in 2 | |
| 6 | none | refused | "`<product>` cannot open `<name>`." |

`looksLikePreset` is Calc-U-1600's rule (`core/DropFile.cpp:20-37`):
UTF-8 without a zero byte, after an optional byte-order mark, with a
line that starts with `model:` in the first column. It moves into the
library (§5).

### 3.3 A product's file kinds (AC-1 test 4, AC-6; SP-3, I-4)

```cpp
class FileKind {
public:
    virtual std::string_view name() const = 0;                     // "basic-program": in the reply
    virtual bool takesName(std::string_view lowerName) const = 0;  // AC-6: by the name alone, for a drag
    virtual bool takes(const OpenedFile&) const = 0;               // AC-1 test 4: by its own rule
    virtual sessions::CommandOutcome open(const OpenedFile&, Way, sessions::SessionContext&) = 0;
protected:
    ~FileKind() = default;
};
```

- The kinds are asked in the order the product gives (AC-17: BASIC
  before machine code). What `open` does is the product's (C1600-2,
  C59-4, C59-7). It knows the way in, so that in the harness, which has
  no dialogs, a WAV loads its first file (AC-18).
- A kind may refuse what it takes, with its own sentence: Calc-U-59's
  `.rom.yaml` and `ti58c.memory.yaml` (AC-23). The open part adds the
  notice to a `Refused` that any kind returns.

### 3.4 What the command needs of LIB-6 and LIB-7 (A-1, A-2)

LIB-13 declares the two seams it uses. `LibraryOpeners.cpp` implements
them over LIB-6's and LIB-7's parts, so LIB-6 and LIB-7 do not know
this feature, and the tests can replace both. `MediaOpener` maps `read`,
`placesFor`, `placesInCatalogue` and `refusesNow` onto LIB-7's
`MediaPart::readOpened`, `placesFor`, `placesInCatalogue`, `refusesNow`
(LIB-7 §3.7), and `useWhereItLies`, `importFile` with `useImported` onto
LIB-7's command `media.open-file {name, place?, path?, bytes?, upload?,
confirmed?}` (LIB-7 §3.6), which imports first for an upload and only
imports when no place is given (D-8).

```cpp
class PresetOpening {                                     // LIB-6 (M-38)
public:
    virtual std::vector<std::string> extensions() const = 0;                  // ".ti59", ".pc1500a", lower case
    virtual sessions::CommandOutcome loadFile(const std::string& path, sessions::SessionContext&) = 0;   // from its folder
    virtual sessions::CommandOutcome loadContent(const OpenedFile&, sessions::SessionContext&) = 0;      // LIB-6 EC-5
protected: ~PresetOpening() = default;
};

struct MediaPlaceRef { machine::PlacePath ownerPlace; machine::MediaPlaceId id; };
struct MediumFile {                                        // what LIB-7 read and checked first (AC-4)
    std::optional<std::string> refusal;                    // outdated or unreadable, with LIB-7 AC-9's hint
    std::string kind, kindName;                            // "floppy-disk", "floppy disk"
    bool isTemplate = false;                               // LIB-7 AC-25
    bool isModule = false;                                 // goes into a module slot (plugged in as by the slot's picker)
};
class MediaOpening {                                      // LIB-7 (M-41)
public:
    virtual std::vector<std::string> suffixes() const = 0;                    // ".magnetcard.yaml", ".floppy.yaml", lower case
    virtual MediumFile read(const OpenedFile&) const = 0;
    // Where the medium can go on the machine now: the media places of its kind, for a module the slots it fits.
    virtual std::vector<MediaPlaceRef> placesFor(const MediumFile&, const OpenedFile&, const machine::Machine&) const = 0;
    // Where a medium of the kind would go on any model or peripheral, for the refusal's sentence.
    virtual std::vector<std::string> placesInCatalogue(const MediumFile&) const = 0;   // "the card-reader of the TI-59"
    virtual std::optional<std::string> refusesNow(const MediaPlaceRef&, const machine::Machine&) const = 0;   // EC-4
    virtual sessions::CommandOutcome useWhereItLies(const OpenedFile&, const MediaPlaceRef&, sessions::SessionContext&) = 0;
    struct Imported { bool done = false; bool nameTaken = false; std::string name; std::string refusal; };
    virtual Imported importFile(const OpenedFile&, bool confirmed, sessions::SessionContext&) = 0;   // LIB-7 AC-30, AC-17
    virtual sessions::CommandOutcome useImported(const std::string& name, const MediaPlaceRef&, sessions::SessionContext&) = 0;
protected: ~MediaOpening() = default;
};
```

`useWhereItLies` and `useImported` do what LIB-7 says for the place:
insert with the first side into a holding place after ejecting what was
there (LIB-7 AC-26); for a pass-through place, what the product says
follows: the medium is fitted, not fed (the TI-59 card: C59-7 AC-11;
LIB-7 design D-22, Part D P-3); for a module, the attach of the slot's
picker (LIB-7, LIB-18 AC-10). The file is the instance and is written
back where it lies, unless it is a template (LIB-7 AC-1, AC-11, AC-12,
AC-25).

### 3.5 The part (AC-1, AC-3, AC-4, AC-7, AC-8, EC-2 to EC-4, EC-6, EC-7)

```cpp
struct OpenSetup {
    std::string productName;                       // test 6's sentence
    PresetOpening* presets = nullptr;
    MediaOpening* media = nullptr;
    std::vector<FileKind*> kinds;                  // in the product's order
    std::map<Way, std::uint64_t> maxBytes;         // AC-10; a way not listed has no limit
    std::optional<std::string> uploadFolder;       // AC-8: where several files become one folder (Web: /uploads)
};

class OpenPart final : public sessions::SessionPart {
public:
    explicit OpenPart(OpenSetup);
    void attach(sessions::SessionSetup&) override;                  // the command `open`, the request `open-would-take`
    void afterCall(sessions::SessionContext&, pacing::WallTime) override;   // opens the file kept (AC-7)
};
```

**The command.**
1. **Busy or not started (AC-7).** While an operation runs, or while
   no machine is built yet (the start is an operation, LIB-18 AC-5),
   the request is kept and the reply is `Kept`. A second one takes the
   place of the first, and a log line names the one that was
   replaced (D-6).
2. **Several files (AC-8).** With an upload folder, they are written
   into a new folder `<uploadFolder>/open-<n>/`. The preset among them
   is opened by its path from there: the first in name order of those
   with a preset extension, else of those that pass test 5. If none is a
   preset, the first file as handed over is opened alone. Without an
   upload folder, only the first file is opened, alone (D-7).
3. **One file.** It goes through §3.2. A preset with a path that the
   host can write is loaded from its folder (`loadFile`). An upload, or
   a file without a path, is loaded as its content (`loadContent`;
   EC-3: LIB-6 refuses it then if it names other files).
4. **A medium (AC-4).** In this order:
   - `read`: a refusal is passed on with its hint (LIB-7 AC-9).
   - `placesFor` on the machine now. With exactly one place, that is
     the place. With none: refused, "The `<model>` has no place for
     `<name>` now; a `<kind name>` goes into `<placesInCatalogue>`."
     (EC-2 in place). With several (a module that fits two slots, two
     drives of a kind): refused, "`<name>` fits more than one place;
     choose it in the picker of the place it should go into."
   - `refusesNow` of that place: a place that is passing a medium
     through or writing one refuses, and nothing is imported (EC-4).
   - Written back where it lies (`path` set and not `upload`):
     `useWhereItLies`.
   - An upload: `importFile(file, confirmed)`. A name already taken
     replies `asks:` and changes nothing (EC-6). An imported medium
     is then put into its place with `useImported`. Only an upload is
     imported when it has no place now: then it is imported, the reply
     says `Medium` with "imported", and the notice of "no place" is
     sent (EC-2, Web; D-8).
5. **A file kind**: its `open`.
6. **Refused** otherwise.

The folder in which a file dialog starts is never touched by the
command, by any way in (EC-7). The frontend remembers it after its own
file dialog (`settings::rememberOpenFolder`, LIB-9 AC-29), as it does
today.

**The file kept (AC-7).** At the end of each host call, `afterCall`
checks for a kept file. If no operation runs and a machine is built,
it takes the file and opens it as the command would. An outcome that
starts an operation is started with `startOperation` (§3.6). A refusal
is a notice; it has no reply, because the command was already replied
to with `Kept`.

**The request `open-would-take` (AC-6).** True when tests 1c, 2 and 3
let the name through, or one kind's `takesName` is true. A `size`, if
the frontend knows it at the drag, is held against the drop's limit
(1b). A drag of several files is the frontend's to refuse; the request
takes one name.

### 3.6 Two additions to the sessions (LIB-3)

```cpp
// Operation.hpp: the payload of the reply when the operation ends well (AC-3 here).
struct OperationResult { bool ok = true; std::string message; PayloadPtr result; };

// SessionPart.hpp, SessionContext: an operation started by a part, not by a command
// (AC-7 here). Refused, false, while one runs. Its end sends no reply; a failure is a
// notice (LIB-3 AC-18).
virtual bool startOperation(Operation) = 0;
```

`AppSession::Impl::endOperation` passes `result` to the reply. The open
part wraps the operation an opener returns, so that its result is the
`OpenReply`. Nothing else changes for the other operations.

## 4. Product side (Parts D and E; sketch)

Today's code that the product columns replace. Their tasks are set out
at the Part D and E boundaries.

**Calc-U-1600 (M-48).**

| Today | After M-48 |
|---|---|
| `products/calc-u-1600/core/DropFile.hpp/.cpp` (`dropfile::classify`) | deleted. The BASIC and machine-code kinds are `FileKind`s in `app/SharpFileKinds.hpp/.cpp` (new) on C1600-2's one classifier (M-44), in that order. Headerless code only with `.bin` (`.rom` goes, AC-17). A WAV goes to the loader of its first readable file, else to BASIC |
| request `c1600.drop-target` (`app/SharpWebPart.cpp:274-282`) | gone; `open` and `open-would-take` |
| Qt6 `frontends/qt6/app/MainWindow.cpp:768-834` (drag, drop, the 1 MiB check, the pending drop, the folder remembered on a drop) | `open-would-take` at the drag, `open {way: drop}` at the drop. The limit and the pending file are the session's. No folder is remembered (EC-7) |
| Qt6 `frontends/qt6/app/main.cpp` (`QFileOpenEvent` → `openDroppedFile`; a file named at the start is passed over) | `open {way: open-with}`; the start argument `open {way: start}` (AC-21, OQ-2) |
| `frontends/qt6/resources/macos/Info.plist.in:49-99` | `.rom` goes from the document types (AC-21) |
| Web `frontends/web/src/CalcU1600.jsx:16, 136-186` (`MAX_DROP_BYTES`, `openDropped`, `pendingDrop`, `loadPresetFiles`), `src/engine.js:48` | `open {way: drop, upload: true}`, and the dialog's files `open {way: dialog, upload: true}`. an `asks` reply is asked with the shell's dialog |
| the size | `maxBytes`: 1 MiB for drop, Open With and the start argument (AC-20; D-9) |

**Calc-U-59 (M-56, with C59-4).**

| Today | After M-56 |
|---|---|
| `products/calc-u-59/app/TIAppPart.hpp:50-58` `DropAction`, `dropActionFor`; `TIAppPartFlows.cpp:604-623` `openDroppedFile`; commands `c59.open-dropped`, request `c59.drop-action` | gone. The magnetic card is LIB-7's medium (`.magnetcard.yaml`, C59-7, M-55). The kinds `KeystrokesKind` (`.keystrokes.yaml`, AC-23), `RomModuleFileKind` and `ConstantMemoryFileKind` (refused with a notice) and C59-4's ASM overlay are in `app/TIFileKinds.hpp/.cpp` (new) |
| Qt6 `frontends/qt6/app/MainWindow.cpp:145-160`, `EmulatorController.cpp:260`, `main.cpp` (`QFileOpenEvent`, the last start argument, always read as a preset) | `open-would-take`, `open` with the way; opened as what it is (AC-14) |
| Swift `frontends/swift/App/Views/CalculatorView.swift:212`, `App/EmulatorViewModel.swift:401-410`, `Bridge/TIAppSession.mm:582` | the drop on macOS asks `open-would-take` and sends `open` (AC-6, AC-14); a refusal is shown as a notice (AC-15) |
| Web `frontends/web/src/PlayCalculator.jsx:264-285` | `open {way: drop, upload: true}` |
| the size | no limit (AC-11) |

## 5. Adapters

| Adapter | File | Wraps | Removed in |
|---|---|---|---|
| `looksLikePreset` forward | `products/calc-u-1600/core/DropFile.cpp` | its own preset test now calls `calcu::opening::looksLikePreset`, so the rule exists once from M-42 (`program_file_tests.cpp:199-242` keep passing) | M-48 (`DropFile.*` goes) |
| Calc-U-1600's drop flow | `core/DropFile.*`, `app/SharpWebPart.cpp` (`c1600.drop-target`), Qt6 `MainWindow.cpp`, Web `CalcU1600.jsx` | today's classify-then-load, unchanged | M-48 |
| Calc-U-59's drop flow | `app/TIAppPart.*`, `TIAppPartFlows.cpp` (`c59.open-dropped`, `c59.drop-action`), the three frontends' drops | today's rule by name, unchanged | M-56 |

The harness has `open` from M-42 for every product. Until M-48 and
M-56 a product's harness session has no file kinds
(`HarnessProduct::fileKinds()` returns none), so `open` takes its
presets and media and refuses the rest.

## 6. Tasks

Library, M-42. Files in `library/opening/tests/` unless named; program
`calcu_library_tests`; every case Frequent. The tests use the test
catalogue (`test-seg`: media place `reader`, pass-through; one module
slot `slot-1`; `test-dot`: media place `drive`, holding; two module
places `card-1`, `card-2`), the stand-ins of `TestOpeners.hpp`
(presets `.tseg`, `.tdot`; media `.tcard.yaml`, `.tdisk.yaml`,
`.tmod.yaml`), the two test file kinds (`test.listing`: name `.lst`;
`test.blob`: content that starts with `BLOB`), and
`library/testing/`'s `TestFrontend.hpp` and `ScriptedClock.hpp`. Tests come before the code.

| Task | What | ACs/ECs | Test (file: cases) |
|---|---|---|---|
| L1 | CMake, the names, `OpenRequest` from and to `Value`, `OpenReply` | (support) | `open_tests.cpp`: `open_request_value_round_trip` |
| L2 | The two LIB-3 additions | (support) | `library/sessions/tests/operation_tests.cpp`: `operation_result_payload_in_reply`, `part_starts_operation_without_command`, `part_operation_refused_while_one_runs` |
| L3 | The order of the tests | AC-1, EC-1, EC-5 | `classify_tests.cpp`: `ac1_unreadable_large_hidden_refused_first`, `ac1_preset_extension_before_medium_and_kinds`, `ac1_medium_suffix_before_kinds`, `ac1_kinds_in_product_order`, `ac1_model_line_after_kinds`, `ac1_nothing_refused_with_notice_naming_file`, `ac1_names_compared_without_case`, `ec1_outdated_preset_extension_is_preset_refused_with_hint`, `ec5_unreadable_file_refused_notice_names_it` |
| L4 | `looksLikePreset` | AC-2 | `classify_tests.cpp`: `ac2_model_line_in_first_column`, `ac2_indented_or_commented_model_not_a_preset`, `ac2_zero_byte_or_bad_utf8_not_a_preset`, `ac2_bom_allowed` (the cases of Calc-U-1600 `program_file_tests.cpp:199-242`, without product code) |
| L5 | Limits per way in | AC-10 | `classify_tests.cpp`: `ac10_limit_per_way_in`, `ac10_way_without_limit_takes_any_size` |
| L6 | The reply | AC-3 | `open_tests.cpp`: `ac3_reply_says_preset_medium_kind`, `ac3_reply_says_kept`, `ac3_reply_says_why_refused` |
| L7 | A medium | AC-4, EC-2, EC-4, EC-6 | `open_tests.cpp`: `ac4_medium_checked_first_hint_passed_on`, `ac4_used_where_it_lies`, `ac4_template_not_written` (the stand-in records no write-back), `ac4_holding_place_inserted_after_eject`, `ac4_module_one_slot_plugged`, `ac4_module_two_slots_refused`, `ac4_no_place_now_refused_says_where`, `ac4_upload_imported_then_placed`, `ac4_upload_name_taken_asks_then_replace`, `ec2_no_drive_refused_in_place_imported_as_upload`, `ec4_place_passing_medium_refuses_nothing_imported`, `ec6_no_answer_nothing_imported_nothing_placed` |
| L8 | Ways in | AC-5 | `ways_tests.cpp`: `ac5_every_way_in_same_outcome_but_limit` (one file through `drop`, `open-with`, `start`, `dialog`, `harness`) |
| L9 | The drag | AC-6 | `ways_tests.cpp`: `ac6_would_take_by_tests_1_to_3_and_kind_names`, `ac6_would_take_ignores_content`, `ac6_size_at_drag_held_against_limit` |
| L10 | Kept for later | AC-7 | `open_tests.cpp`: `ac7_kept_while_operation_runs_opened_when_free`, `ac7_kept_before_machine_built`, `ac7_second_replaces_first`, `ac7_kept_file_refused_later_is_a_notice` |
| L11 | Several files | AC-8, EC-3 | `open_tests.cpp`: `ac8_several_files_one_folder_preset_found_by_path`, `ac8_first_preset_by_name`, `ac8_no_preset_first_file_alone`, `ec3_preset_alone_as_content_refused_when_it_names_files` |
| L12 | Through the real parts | AC-1, AC-4 | `real_parts_tests.cpp`: `open_preset_through_lib6_part`, `open_medium_through_lib7_part`, `ac4_pass_through_follows_product` (`LibraryOpeners`, with LIB-6's and LIB-7's test data; the last moved from L7's stand-ins in Part D P-3: a card is fitted, not fed) |
| L13 | The folder of the dialog | EC-7 | `open_tests.cpp`: `ec7_no_way_in_changes_open_folders` (a session with a settings file; `[folders]` unchanged after `open` by every way) |
| L14 | The harness | AC-9 | `library/harness/tests/commands_tests.cpp`: `ac9_open_path_replies_as_command` (the harness test product: a preset, a medium, a refused file) |

Coverage, Library: AC-1 to AC-10 (10 of 10), EC-1 to EC-7 (7 of 7).
Product tasks: §4, at the Part D and E boundaries.

## 7. Tools and agent knowledge

**Tools.** The harness gets `open <path>` in M-42
(`library/harness/OpenCommands.cpp`, registered by `Run.cpp` with the
common commands). It reads the file, sends `open {way: harness}`,
and prints the reply of AC-3. `Run.cpp` adds an `OpenPart` over the
harness session's LIB-6 and LIB-7 parts, with
`HarnessProduct::fileKinds()` and `openLimits()` (new; default none).
The harness has no upload folder. Today's tools (`tools/headless/`,
`pc1500_cli`, `pc1600_cli`) open no files by drop and are unaffected.

**Agent knowledge changed in the same step.**

| Step | Files |
|---|---|
| M-42 | `docs/test-groups.md` (the new cases by file); LIB-1 design §2a (the area `opening` and its dependency line) |
| M-48 | `products/calc-u-1600/docs/developer/Shared-Architecture.md` where it names `DropFile`; `Drag-And-Drop-Plan.md` stays history (plan M-48) |
| M-56 | `products/calc-u-59/reference/SharedArchitecture.md` § "Dropped Files"; the `calc-u-59-shared`, `-qt`, `-swift`, `-web` skills where they name `dropActionFor`, `c59.open-dropped` or `c59.drop-action` |

## 8. Golden runs and risks

- **Golden runs** are not affected: opening a file goes through the
  preset loader and the media code, whose own steps (M-38, M-41) have
  their runs. A difference is a finding.
- **The seams of LIB-6 and LIB-7 are assumed** (§3.4), and their
  designs are drafted beside this one. If LIB-6's load or LIB-7's
  import, insert or attach take another form, only `LibraryOpeners.cpp`
  changes. If LIB-7's Name & Save asks the user in another way than a
  reply and a second command, opening asks in that way (D-4).
- **A part that starts an operation** (§3.6) is new in LIB-3. The open
  part starts one only in `afterCall`, when none runs, so the busy rule
  of LIB-3 AC-16 still holds for every command.
- **Writing an upload folder** (AC-8): the library writes files into
  `<uploadFolder>/open-<n>/` in the Web's memory file system. The
  folders are not cleaned up; a page load empties `/uploads` (LIB-15).
- **`looksLikePreset` moves from a product to the library in M-42**
  (§5). Calc-U-1600's `program_file_tests.cpp:199-242` guard the move.

## 9. Decided (best guess)

- **D-1 (layout).** The area is `library/opening/`, `calcu::opening`,
  after the glossary's verb. LIB-1 §2a does not name one for LIB-13.
- **D-2 (AC-1).** The command takes the bytes from the host. The
  library reads no file of a drop itself, so a file the host cannot
  read is refused by test 1a (EC-5), and Swift's coordinated reading
  stays in the bridge.
- **D-3 (AC-4).** "Written back where it lies" is decided by the
  request: a path, and not marked `upload`. A Web frontend marks every
  file it hands over, its file dialog's included, because the files it
  writes into `/uploads` have a path but cannot be written back.
- **D-4 (AC-4, EC-6).** The question before replacing a medium is
  LIB-7's reply `asks:` and the same command again with `confirmed:
  true`, as Name & Save asks (LIB-7 D-6). No host service asks a
  question (LIB-3 §3.6).
- **D-5 (AC-1).** The sentences of §3.2 and §3.5. The limit is written
  as the product sets it ("1 MiB").
- **D-6 (AC-7).** The file that is replaced while kept gets no notice,
  only a log line, as today it is replaced without a word.
- **D-7 (AC-8).** Several files without an upload folder (no host sends
  them today) open the first one alone.
- **D-8 (AC-4, EC-2).** An upload with no place now is imported and
  then refused for the place with the notice. The medium is in the save
  folder for its picker. A file opened in place with no place now is
  refused and not copied.
- **D-9 (AC-20, AC-21).** A file named at the start of Calc-U-1600's Qt6
  program has the limit of Open With (1 MiB). The spec names drop and
  Open With and says the menu's dialogs have none; the start argument
  comes from the file manager like Open With. Product data, set in M-48.
- **D-10 (AC-9).** The harness command `open` is in
  `library/harness/OpenCommands.cpp`, as the other C2 features' harness
  commands are in `library/harness/`; `opening` does not include
  `harness`, so the two areas do not include each other.
- **O-1 (settled with LIB-7).** §3.4 is checked against LIB-6
  (`load-preset` with `{ref}` or `{content, name}`) and LIB-7: its media
  part answers what `read`, `placesFor`, `placesInCatalogue` and
  `refusesNow` ask, and `media.open-file` takes the place, the bytes,
  `upload` and `confirmed` (LIB-7 §3.6, §3.7, D-14). Only
  `LibraryOpeners.cpp` depends on them.

## Part D: the Calc-U-1600 column

### D1. Scope

| Column | Step | Tests from (plan §3) | Done in (plan §5.1) |
|---|---|---|---|
| Calc-U-1600 | M-48 | AC-17 to AC-22 | M-48 |

M-48 waits for M-42 (the Library column, Done), M-44 (C1600-2: the
loaders, the part `SharpLoaderPart` and its commands, the menu flows on
them; `features/C1600-2-sharp-program-files/design.md`) and M-47
(LIB-7 1600: Calc-U-1600's media kinds `.card.yaml`, `.floppy.yaml`,
`.tape.yaml` in a `MediaPart` of the product's session). It uses M-46
(LIB-6 1600: the product's `PresetsPart`, the preset extensions
`.pc1500`, `.pc1500a`, `.pc1600`). The line numbers below are those of
`main` at `60e5b09`; M-44, M-46 and M-47 change some of these files
first, so M-48 reads them again before it changes them.

The edge cases are the Library column's (EC-1 to EC-7, Done in M-42).
Where the product's ways in must keep them (EC-2 for the drive, EC-7 for
the folder), the tasks of D6 check them once more on the product.

### D2. What the code of M-42 changed against §1 to §9

The Library column was built as §3 says, with the deviations of the
qa-report (`features/LIB-13-opening-files/qa-report.md`, "Deviations"),
which are the record. Those that matter here:
- `MediaOpening` names places by LIB-7's place words, and import and use
  are one call, `importAndUse(file, place?, confirmed)`: the product
  column builds on that form, not on §3.4's `MediaPlaceRef`.
- `HarnessProduct::presetExtensions()` exists besides `fileKinds()` and
  `openLimits()` (`library/harness/Product.hpp:105-107`); the product
  sets all three here.
- `OpenReply` carries the file's name (`file`).
- A kept file whose opening asks gets a notice "`<name>` was not opened:
  `<question>`".

§4's table of what M-48 replaces holds; its line numbers moved: the
request `c1600.drop-target` is at `app/SharpWebPart.cpp:179-188` (not
`:274-282`), the Qt6 drag and drop at `frontends/qt6/app/MainWindow.cpp:749-819`,
the Web drop at `frontends/web/src/CalcU1600.jsx:17, 68, 138-186, 390-394`.

### D3. Modules and files

All under `products/calc-u-1600/` unless named.

| File | Change |
|---|---|
| `app/SharpFileKinds.hpp/.cpp` (new) | `BasicProgramKind`, `MachineCodeKind : calcu::opening::FileKind` on C1600-2's classifier (D4.1); `sharpOpenSetup(…)`: the product's `OpenSetup` (preset and media openers over its `PresetsPart` and `MediaPart`, the kinds in order, the limits, the upload folder) |
| `app/SharpAppPart.hpp/.cpp` | owns the `OpenPart` and the two kinds; `sessionParts()` lists the `OpenPart` after the presets, media and loader parts |
| `app/SharpTopics.hpp` | the stream topic `c1600.program-opened` (D4.2) |
| `app/StartFile.hpp/.cpp` (new) | `startFile(arguments, exists)`: the file named at the start of the Qt6 program (D4.4); Qt-free, so CoreTests test it |
| `app/HarnessProduct.cpp` | `presetExtensions()`, `fileKinds()` (the two kinds over the harness's own `SharpLoaderPart`), `openLimits()` (none: the harness way has no limit) |
| `app/SharpWebPart.cpp:165-188` | `c1600.upload` and `c1600.drop-target` go |
| `core/DropFile.hpp/.cpp` | deleted |
| `frontends/qt6/app/MainWindow.hpp/.cpp:749-819` | the drag asks `open-would-take`; the drop sends `open {way: drop}`; `dropTargetOf`, `kMaxDropBytes`, `droppableFile`, `openDroppedFile`, `drainPendingDrop`, `m_pendingDrop` and the `m_startupDone` gate go; the topic `c1600.program-opened` runs the program flow (D4.2); `loadPresetFile`'s `rememberOpenFile` only after the dialog (EC-7) |
| `frontends/qt6/app/main.cpp:28-54, 162-168` | the `QFileOpenEvent` handler sends `open {way: open-with}`; the start file `open {way: start}` |
| `frontends/qt6/resources/macos/Info.plist.in:97` | `rom` goes from the document types (AC-21) |
| `frontends/web/src/CalcU1600.jsx:17, 68, 138-186, 255, 390-394` | the drop sends `open {way: drop, upload: true}`; "Upload Preset…" sends its files as `open {way: dialog, upload: true}` (AC-8); `MAX_DROP_BYTES`, `pendingDrop`, `openDropped`, `loadPresetFiles` go; an `asks:` reply is asked and sent again with `confirmed`; the topic runs the program flow |
| `frontends/web/src/engine.js:62, 71` | `upload`, `dropTarget` go; `open`, the topic |
| `tests/program_file_tests.cpp:193-300` | the drop cases move to `app/tests/opening_tests.cpp` (AC-22) |
| `app/tests/opening_tests.cpp` (new) | AC-17 to AC-20, AC-22, EC-2, EC-7 on the product's session |
| `tools/check_open_with_types.sh` (new), `CMakeLists.txt` | the CTest `CalcU1600OpenWithTypes` (AC-21; Frequent) |
| `frontends/web/tests/play.spec.js` | the Web cases of D6 |

### D4. Seams and interfaces

#### D4.1 The two file kinds (AC-17)

```cpp
namespace shared {
class BasicProgramKind final : public calcu::opening::FileKind {    // name "basic-program"
public:
    explicit BasicProgramKind(SharpLoaderPart&);
    bool takesName(std::string_view) const override;          // D4.3
    bool takes(const calcu::opening::OpenedFile&) const override;
    calcu::sessions::CommandOutcome open(const calcu::opening::OpenedFile&, calcu::opening::Way,
                                         calcu::sessions::SessionContext&) override;
};
class MachineCodeKind final : public calcu::opening::FileKind { /* the same; name "machine-code-program" */ };
}
```

`takes` asks `sharp::loaders::fileKind` (C1600-2 design §3.2). The kinds
are asked in this order, BASIC first:

| What C1600-2 says the file is | Taken by |
|---|---|
| a BASIC listing, tokenized BASIC (PC-1500 or PC-1600) | `basic-program` |
| machine code behind a CE-158 or PC-1600 header | `machine-code-program` |
| headerless, `looksLike` set, and the name ends in `.bin` (any case) | `machine-code-program` |
| headerless otherwise (also `.rom`, AC-17) | neither |
| a cassette WAV: its first readable file | the kind of that file, by the rows above |
| a cassette WAV without a readable file | `basic-program`, so that the loader says why |
| empty, "something else" | neither: test 5 (a model line) or 6 (refused) of AC-1 follows |

These are today's rules of `dropfile::classify`
(`core/DropFile.cpp:27-45`) without `.rom`, and without its preset test
first: the preset extensions and the model line are the library's tests
2 and 5 (AC-1), the media suffixes its test 3.

#### D4.2 What follows when a program is opened (AC-18)

- **Harness** (`way: harness`, no dialogs): the kind loads at once
  through `SharpLoaderPart`: `basic-program` as `c1600.load-basic`,
  `machine-code-program` as `c1600.load-code` without an address, a
  WAV's first file. Headerless code is refused with the plan's sentence
  (it needs an address, C1600-2 AC-8). The reply is `OpenReply{FileKind,
  "<kind>"}`, or the refusal.
- **Every other way** (drop, Open With, start, the Load Preset…
  dialog): the kind sends the stream topic `c1600.program-opened {name,
  bytes, path?}` and replies `OpenReply{FileKind, "<kind>"}`. The
  frontend runs its program flow on it: the flows of File ▸ Load BASIC
  Program… and Load Machine Code… (C1600-2 design §3.9) after their
  file dialog, starting at `c1600.file-kind`. A WAV with several files
  asks which one, and the file chosen goes to the flow of its own kind:
  BASIC to the BASIC flow, machine code to the machine-code flow (with
  the address dialog for headerless code). The flow came from an open,
  so it remembers no folder (EC-7).
- The topic is used, not the reply, because a kept file (AC-7) is opened
  in `afterCall` and has no reply to carry it (D-11).

#### D4.3 The drag (AC-6)

`takesName` is true for every name: C1600-2 AC-1 has the content decide,
never the name, and a BASIC listing or a preset may have any name. A
drag is therefore taken unless tests 1b and 1c refuse it (larger than
1 MiB when Qt gives the size; a hidden file) or it carries more than one
file; the drop itself goes through all of AC-1 and refuses with a
notice. Today the drag reads the file and refuses what loads nowhere
(`MainWindow.cpp:757-778`). See Q-1.

#### D4.4 The ways in (AC-20, AC-21)

| Way | Qt6 | Web | Limit (AC-20, D-9) |
|---|---|---|---|
| drop | `dropEvent` reads the file (`bytes` absent if it cannot) and sends `open {files: [{name, path, bytes}], way: drop}` | `open {files: [{name, bytes}], way: drop, upload: true}` | 1 MiB |
| Open With | macOS `QFileOpenEvent` (`main.cpp:41-49`), also at a cold launch (the session keeps it, AC-7): `open {way: open-with}` | none | 1 MiB |
| start | `startFile(QCoreApplication::arguments(), exists)`: the last argument that names an existing file; `open {way: start}` once the window is shown (the session keeps it until the start ends, AC-7) | none | 1 MiB |
| Load Preset… dialog | `open {way: dialog}` with the chosen file | "Upload Preset…": its files as `open {way: dialog, upload: true}`, several as one folder (AC-8; upload folder `/uploads`, `frontends/web/bindings.cpp:43`) | none |
| harness | `open <path>` (M-42) | none | none |

The menu items Load BASIC Program… and Load Machine Code… do not send
`open` (AC-5). A refusal is the session's notice, shown as today's
notices are (row 1600-27). The macOS document types keep the three
preset extensions as the product's own and `.bas`, `.bbin`, `.bin` as
Alternate; `.rom` goes (AC-21). Windows and Linux get no Open With
(out of scope).

#### D4.5 Media (AC-19)

Through the library's `MediaOpener` over the product's `MediaPart`
(M-47): in Qt6 a disk file is inserted from where it lies when the
CE-1600F is there, and refused with a notice when it is not (EC-2); in
the Web it is imported, then inserted when the drive is there. A module
file is plugged in when its slot is clear from the context (the
PC-1500's one slot, or a module that fits only one of the PC-1600's),
else refused with the notice of AC-4; the Web imports it first. A
`.tape.yaml` is inserted into the cassette when the model has one
(LIB-7 AC-42). A WAV is never a tape here: it is a program file (D4.1).

### D5. Adapters

| Adapter | File | Removed in |
|---|---|---|
| `looksLikePreset` forward (M-42) | `core/DropFile.cpp:15` | M-48: `core/DropFile.*` deleted |
| Calc-U-1600's drop flow (§5; since M-44 on C1600-2's commands, C1600-2 design §5) | `core/DropFile.*`, `app/SharpWebPart.cpp:179-188` (`c1600.drop-target`), `frontends/qt6/app/MainWindow.cpp:749-819`, `frontends/web/src/CalcU1600.jsx:17, 68, 174-186` | M-48 |
| `c1600.upload` | `app/SharpWebPart.cpp:169-178`, `frontends/web/src/CalcU1600.jsx:138-148` | M-48: the open part's upload folder writes the files (AC-8) |

None is added. After M-48 nothing in Calc-U-1600 tells a file's kind
outside `sharp::loaders::fileKind` and the open part.

### D6. Tasks

Tests before code. `CoreTests` (Frequent) unless named; Web cases in
`CalcU1600WebPlaywright` (Longer), titled with the ID. Case names
`lib13_acN_…`. The session tests run on `app/tests/SessionFixture.hpp`
with the product's parts, a save folder of the test's own and the real
ROMs (`SKIP_UNLESS`).

| Task | What | ACs/ECs | Test (file: cases) |
|---|---|---|---|
| T16-1 | The product's data, the two kinds | AC-17 | `opening_tests.cpp`: `lib13_ac17_preset_extensions_media_suffixes`, `_kinds_asked_basic_then_code`, `_listing_and_tokenized_are_basic`, `_headered_code_is_code`, `_headerless_code_only_with_bin`, `_rom_is_not_code`, `_wav_goes_to_first_files_kind`, `_unreadable_wav_goes_to_basic_loader_which_says_why`, `_empty_and_other_taken_by_neither` |
| T16-2 | What follows | AC-18 | `opening_tests.cpp`: `lib13_ac18_gui_way_sends_program_opened_topic`, `_kept_file_also_sends_topic`; `tests/harness_tests.cpp`: `lib13_ac18_harness_open_loads_first_wav_file`, `_harness_open_refuses_headerless`, `_harness_open_preset_bas_and_headered_bin`; Web `play.spec.js`: `LIB-13 AC-18 a dropped WAV with several programs asks which one, then loads by its kind`, `LIB-13 AC-18 a dropped headerless .bin asks the address` |
| T16-3 | Media | AC-19, EC-2 | `opening_tests.cpp`: `lib13_ac19_disk_used_where_it_lies_written_back`, `lib13_ec2_disk_without_drive_refused_with_notice`, `lib13_ac19_upload_disk_imported_then_inserted`, `_module_one_place_plugged_pc1500`, `_module_two_places_refused_pc1600`, `_tape_definition_into_cassette`, `_wav_is_never_a_tape`; Web: `LIB-13 AC-19 a dropped disk file is imported and inserted` |
| T16-4 | Limits | AC-20 | `opening_tests.cpp`: `lib13_ac20_over_1mib_refused_drop_openwith_start`, `_dialog_and_harness_without_limit`; Web: `LIB-13 AC-20 a dropped file over 1 MiB is refused with a notice` |
| T16-5 | The ways in | AC-21, EC-7 | `opening_tests.cpp`: `lib13_ac21_start_file_is_last_file_argument` (`startFile`), `lib13_ec7_open_by_any_way_keeps_dialog_folders`; CTest `CalcU1600OpenWithTypes` (`tools/check_open_with_types.sh` over `Info.plist.in`: the three preset extensions as Owner, `bas`, `bbin`, `bin` as Alternate, no `rom`); Web: `LIB-13 AC-8 Upload Preset opens the preset among its files`; Qt6: a drop, an open from the Dock icon and Finder's Open With, and a start argument, checked by hand on macOS and recorded in the qa-report (the attach scenarios of M-49 take them over) |
| T16-6 | Today's tests | AC-22 | `opening_tests.cpp`: the cases of `program_file_tests.cpp:193-300` (`test_drop_targets` and the WAV cases) as `lib13_ac22_…`, through the product's open part instead of `dropfile::classify`; the `.rom` case changed by AC-17 |

Coverage: AC-17 to AC-22 have a task; the column is Done in M-48.

### D7. Tools and agent knowledge

**Tools.** The harness's `open` takes Calc-U-1600's presets by their
extensions, its media and its two kinds (D4.2). `pc1500_cli`,
`pc1600_cli` and the probes open no files by drop and are unaffected.

**Agent knowledge changed in M-48.**

| File | Change |
|---|---|
| `products/calc-u-1600/CLAUDE.md` | § Harness: `open <path>` opens presets, media and program files |
| `products/calc-u-1600/docs/developer/Shared-Architecture.md` | the open part and `SharpFileKinds` where the window's drop is described |
| `products/calc-u-1600/docs/developer/Web-Client.md:139` | `dropfile::classify` → the session's `open` |
| `docs/test-groups.md` | `opening_tests.cpp`, `CalcU1600OpenWithTypes` |
| `docs/glossary-mapping.md` | a row "Open, file kind": `dropfile::classify`, `c1600.drop-target`, `openDroppedFile` → `open`, `open-would-take`, `shared::BasicProgramKind`, `MachineCodeKind` |

`products/calc-u-1600/docs/background/plans/Drag-And-Drop-Plan.md`
stays history (plan M-48). No Calc-U-1600 memory file names `DropFile`,
the drop target or `openDroppedFile` (checked 2026-11-16 against the
`calc-u-1600` index); none changes. The product has no skills.

**Ledger.** Row 1600-15, its opening part, becomes visible: a disk file
can be dropped and is used where it lies (Web: imported); a module file
is plugged in when it is clear where it goes, else refused with a
notice; `.rom` is no longer a name of a machine-code file, on a drop or
with Open With; a file named at the start is opened; a drop no longer
changes the folder of a file dialog; a file over 1 MiB and a file the
product cannot open are refused with a notice.

### D8. Golden runs and risks

- **Golden and reference runs** are not affected: opening hands a file
  to the preset loader, the media part and C1600-2's loaders, whose
  steps (M-46, M-47, M-44) carry their runs.
- **Risk: a program flow started by a topic.** The frontend now starts a
  dialog flow on a topic, not on its own event. A topic that comes while
  a flow's dialog is open (the dialogs are not an operation, so a second
  drop is not kept) could start a second flow. Each frontend runs one
  program flow at a time and drops a topic that comes meanwhile, with a
  log line.
- **Risk: the Web drop as an upload.** A dropped preset alone is loaded
  as its content, as today's `loadPresetFiles([file])`; one that names
  other files is refused when it is read (EC-3), where today it fails at
  the missing file.
- **The start argument on macOS.** The system or an IDE may add
  arguments (`-NSDocumentRevisionsDebugMode YES`, `main.cpp:143-144`);
  `startFile` takes only an argument that names an existing file.

### D9. Decided (best guess)

- **D-11 (AC-18).** A program file opened by a way with a window is
  handed to the frontend's program flow by the topic
  `c1600.program-opened`; the dialogs stay the frontends', as for the
  menu items. In the harness the kind loads the file itself.
- **D-12 (AC-17).** The kinds' rules are those of `dropfile::classify`
  without `.rom` and without its preset test (D4.1).
- **D-13 (AC-6).** `takesName` is true for every name (D4.3; Q-1).
- **D-14 (AC-21).** The start file is the last argument that names an
  existing file; options and words that name no file are passed over,
  as Calc-U-59's "the last one counts" (AC-14).
- **D-15 (AC-20).** 1 MiB for drop, Open With and start (D-9); no limit
  for the dialog and the harness.

### D10. Questions for the user

- **Q-1 (the drag, AC-6).** Over the Qt6 window, a drag of any one file
  is shown as taken, and the drop refuses with a notice a file that is
  nothing the product opens (D4.3). Today the drag itself refuses such a
  file, because it reads it. Taking only the names `.bas`, `.bbin`,
  `.bin`, `.wav` and the preset and media names would keep the drag
  strict, but a listing or a preset under another name could then no
  longer be dropped. Is "any name at the drag, a notice at the drop"
  right? Recommendation: yes, and row 1600-15 gets the words "a drag
  over the window is taken for any one file".
  Answered (user, 2026-11-16, with the approval of the Part D designs): yes; row 1600-15 has the words.
