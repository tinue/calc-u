# C1600-2 Sharp program files: design
Status: approved by the user at the Part C2/D boundary (2026-11-16)

## 1. Scope

C1600-2 is a product row: one column, one step.

| Column | Step | Tests from (plan §3) | Done in (plan §5.1) |
|---|---|---|---|
| Calc-U-1600 | M-44 | AC-1–25, EC-1–6 | M-49 (AC-15 and the Qt6 half of AC-20, §6) |

Every criterion becomes a test in M-44. Two parts can only pass later:
- AC-15 names LIB-14 AC-34 (Qt6) and AC-35 (Web): the File menu of the
  family shell. Calc-U-1600's frontends move onto the shell in M-49
  (LIB-14 1600). Until then the two items stay in today's menus
  (`frontends/qt6/app/MainWindow.cpp:863-864`,
  `frontends/web/src/CalcU1600.jsx:269-270`) and send the new commands
  (D-9; Plan note P-1).
- AC-20 asks for two GUI scenarios in Qt6 and in the Web frontend. The
  Web ones exist today and are renamed in M-44. The Qt6 frontend has no
  GUI test today; its scenarios are written as attach clients in M-49,
  with the screenshot scenarios (LIB-14 AC-44).

The steps this design uses:
- M-26 (C1600-1): the Sharp machines on the machine interface; the
  product kind `c1600.loader-memory` (`core/Machine/SharpProductKinds.hpp:33-44`),
  "reshaped by C1600-2 AC-14 in M-44" (C1600-1 design §3.6 table); the
  loader adapters of `core/Machine/LegacyCalls.hpp:35-42`, "removed in
  M-44" (C1600-1 design §5).
- M-36 (LIB-5): the typing engine, `typeAndWait` / `typeTextAndWait`
  inside an operation (`library/typing/Awaitable.hpp`), the Sharp
  profiles (`app/SharpTyping.*`), typing support with prompt readiness
  (`library/typing/TypingSupport.hpp:23`, implemented in
  `core/PC1500/PC1500InterfaceMachine.cpp`, `core/PC1600/PC1600InterfaceMachine.cpp`).
  LIB-5 design §5 names the Sharp preset typers as adapters "removed in
  M-44 (C1600-2: typing a program in), M-46 (LIB-6 1600)".
- M-28 (LIB-11): the harness, `HarnessProduct::addCommands` and
  `sessionParts()` (`library/harness/Product.hpp:74-77`); the command
  word `c1600` (`app/HarnessProduct.cpp:99`).
- M-4 (FS 13): the reference runs of the 32 examples
  (`products/calc-u-1600/tests/reference/v0.8.1-examples/`, made by its
  `run.sh`). AC-13's auto-run is in the machine-code examples there.
- M-43 (LIB-14 L): the shell exists, but Calc-U-1600 is not on it before
  M-49 (above).

Not here: a preset's `program:` section and its keys (C1600-4, M-46; it
calls these loaders), the debugger's Build & Load (C1600-3, M-50; it
calls the one plan), dropping and Open With (LIB-13 1600, M-48; the
file kinds use these loaders), the tape bay (LIB-7 1600, M-47),
SharpDataExchange itself (used as it is, OQ-1). Hardware facts (header
layouts, BASIC pointers, the PC-1600's banks and program areas) are the
research's: `PC1500` skill (`Shared/Data-Formats/Binary-Exchange-Formats.md`,
`PC-1600/PC-1600-Memory-Architecture.md`, `PC-1600/PC-1600-Work-Area-Map.md`);
this design adds none.

Paths in the spec are at `b4ea8c8` (`Core/`, `Shared/`, `Qt6/`,
`Web/`). Here they are in today's tree: `core/`, `app/`,
`frontends/qt6/`, `frontends/web/`, `tests/`, `app/tests/` under
`products/calc-u-1600/`.

## 2. Modules and files

All under `products/calc-u-1600/` unless named.

### Core

| File | Change |
|---|---|
| `core/Machine/SharpProductKinds.hpp` | `LoaderMemory` gets the shape of AC-14 (§3.1); comment block updated |
| `core/PC1500/PC1500InterfaceMachine.cpp:24`, `core/PC1600/PC1600InterfaceMachine.cpp:29` | the kind's implementations over `memory().peek/poke` (PC-1500) and `debugPeek`, `mode1`, `programAreaTitle`, `debugWriteInternalRam`, `debugWriteSlotBus`, `debugSlotBusWritable`, `debugSlotImage`, `debugBankState` (`core/PC1600/PC1600Machine.hpp:383-474`) |
| `core/Loaders.hpp/.cpp` (new) | `sharp::loaders`: the entry points of every caller on `calcu::machine::Machine&`, through the kind only (§3.2-§3.6) |
| `core/BasicTyping.hpp/.cpp` (new) | `sharp::loaders::typeProgram`: typing a listing in through the typing engine inside an operation (§3.4) |
| `core/ProgramFile.hpp/.cpp` | stays the one classifier (AC-1, AC-3); `classify` gives `Kind::Empty` for zero bytes and a damaged header as `damaged` (EC-1, EC-2, today already); the sentence of each kind for a refusal (§3.8) |
| `core/MachineCodeFile.hpp/.cpp` | `planLoad` (`:240-297`) is the one plan; `plan()` (`:299-334`, the menu's second plan) and `Plan` go; `LoadError` gets its one sentence (`refusal`, §3.8); `advice` (`:350-505`) stays; `PC1600State` is read from the kind (`loadState(LoaderMemory&)`) |
| `core/PC1500/PC1500BasicLoader.*`, `core/PC1600/PC1600BasicLoader.*` | take `sharp::LoaderMemory&` and the model instead of `PC1500Machine&` / `PC1600Machine&`; `loadBasicProgramFile(…, path)` goes (callers read the file) |
| `core/PC1500/PC1500MachineCodeLoader.*`, `core/PC1600/PC1600MachineCodeLoader.*` | the same; the PC-1500 writer checks the whole range before it writes (AC-10, OQ-2); `pc1600LoadState`, `pc1600SlotGeometry`, `pc1600BasicAreas` read the kind |
| `core/PC1600/PC1600ProgramPlacement.*` | unchanged (it already reads through a `peek` function and `SlotGeometry`) |
| `core/PC1500/PC1500BasicTyper.*`, `core/PC1600/PC1600BasicTyper.*` | `typeBasicProgramTextJob`, `typeBasicProgramText`, `settleUntilProgramPtrStable` go (§5). `tapKeyJob`, `typeLineJob`, the waits and the boot run stay for the preset steps until M-46 |
| `core/Basic/BasicLoadResults.hpp` | `BasicTypeResult` stays (the typer's result) |
| `core/Machine/LegacyCalls.hpp/.cpp` | `loadBasicProgram`, `loadMachineCode`, `machineCodeLoadState`, `userRam` (`:35-42`) go; `applyPresetJob` gets the typing hand-off (§3.10) |
| `core/Preset/PresetRunner.hpp/.cpp`, `core/PC1500/PC1500PresetLoader.cpp:67-80`, `core/PC1600/PC1600PresetLoader.cpp:169-186` | `PresetMachine`'s `typeBasicProgram`, `loadBasicFile`, `codeTarget`, `codeState`, `loadMachineCode` (`PresetRunner.hpp:125-139`) go; `runProgram` (`PresetRunner.cpp:269-310`) and `loadBinaryProgram` (`:180-267`) call `sharp::loaders` (§3.10) |
| `core/Debug/ProgramLoader.cpp:17-80` | plans and writes through `sharp::loaders` (`:26`, `:55`, `:67-68`); the machine pointers go from its signature (§3.10) |
| `core/DropFile.cpp` | unchanged in M-44 (it calls `programfile::classify`); deleted in M-48 (LIB-13) |

### App layer

| File | Change |
|---|---|
| `app/SharpLoaderPart.hpp/.cpp` (new) | `shared::SharpLoaderPart : calcu::sessions::SessionPart`: the commands and requests of §3.7, the typing-in operation, the start with `run`. Needs the session's machine and the `TypingPart` only, so the harness can carry it before `SharpAppPart` joins the harness (M-51) |
| `app/SharpTopics.hpp:48-49` | `c1600.load-basic`, `c1600.load-machine-code` replaced by the names of §3.7; the payloads `FileKindReply`, `CodePlanReply`, `CodeLoadedReply` |
| `app/SharpAppPart.hpp/.cpp` | `MachineCodeLoadRequest` (`.hpp:56-61`), `loadBasicProgram`, `loadMachineCode` (`.hpp:146-147`, `.cpp:543-563`) and their two commands (`.cpp:283-299`) go; `sessionParts()` lists a `SharpLoaderPart` it owns |
| `app/MachineCodeLoad.hpp/.cpp` | deleted. `programChoice` is the WAV list of `c1600.file-kind`; the plan, the address check and the reply are the part's (§3.7) |
| `app/SharpWebPart.hpp:68`, `app/SharpWebPart.cpp:190-233` | `c1600.open-program`, `c1600.load-basic-picked`, `c1600.plan-machine-code`, `c1600.check-address`, `c1600.load-planned-machine-code` and `m_program`, `m_code` go: the page sends the part's commands. `c1600.upload` and `c1600.drop-target` stay (M-48) |
| `app/HarnessProduct.hpp/.cpp` | a `SharpLoaderPart` in `sessionParts()` (`:119`), after `typing_`; `addCommands` registers the four commands of AC-21 under `c1600` (§3.7) |
| `app/tests/SessionFixture.hpp` | unchanged; the fixture's session has the part through `sessionParts()` |

### Frontends

| File | Change |
|---|---|
| `frontends/qt6/app/MainWindow.cpp:175-182, 390-488` | the two menu flows send `c1600.file-kind`, `c1600.plan-code`, `c1600.load-basic`, `c1600.load-code` through the controller and show the replies; no `shared::MachineCodeLoad`, no call on the part (§3.9). `loadBasicProgramFile`, `loadMachineCodeFile` keep their names (the drop calls them until M-48) |
| `frontends/qt6/app/MachineCodeLoadDialog.hpp/.cpp` | takes the plan reply and asks `c1600.plan-code` with the typed address on each change (AC-18) |
| `frontends/web/src/CalcU1600.jsx:150-172` | `openProgram` on the same commands |
| `frontends/web/src/Dialogs.jsx:99-150` | `AddressDialog` asks `planCode` per keystroke; `LoadedDialog` shows the reply's parts |
| `frontends/web/src/engine.js:64-71` | the engine's messages `fileKind`, `loadBasic`, `planCode`, `loadCode` instead of `openProgram`, `loadBasic`, `planMachineCode`, `checkAddress`, `loadMachineCode` |
| `frontends/web/tests/play.spec.js:297-337` | the two scenarios under their IDs (AC-20) and the cases of §6 |

### Tests

| File | Holds |
|---|---|
| `tests/program_files_tests.cpp` (new) | AC-1, AC-2, AC-3, EC-1, EC-2, EC-4 |
| `tests/loader_memory_tests.cpp` (new) | AC-14 |
| `tests/basic_fastloader_tests.cpp`, `tests/pc1600_basicloader_tests.cpp` (ported) | AC-4, AC-5, AC-6, EC-3, EC-5, on the kind |
| `tests/basic_typing_tests.cpp` (new; `basictyper_tests.cpp`, `pc1600_basictyper_tests.cpp` lose the cases of the typer that goes) | AC-7 |
| `tests/machine_code_file_tests.cpp` (ported) | AC-8, AC-9, AC-10, AC-11, AC-12, AC-23 |
| `app/tests/loader_part_tests.cpp` (new) | AC-13, AC-24, AC-25, EC-6; the replies of §3.7 |
| `tests/harness_tests.cpp` | AC-21 |
| `app/tests/shared_app_session_tests.cpp:193-252` | the `programChoice` and `MachineCodeLoad` cases move to `loader_part_tests.cpp` (AC-22) |

## 3. Seams and interfaces

Sketches. Single-threaded on the session's thread; nothing thrown
across an interface; no wall clock (ADR 0009).

### 3.1 The product kind (AC-14)

`c1600.loader-memory` keeps its kind ID and its owner, the main CPU
(C1600-1 D-1). Its new shape:

```cpp
namespace sharp {
class LoaderMemory : public calcu::machine::Capability {
public:
    static constexpr calcu::machine::KindId kKind{"c1600.loader-memory"};
    enum class Processor { Main, Lh5803 };       // Main: the LH5801 (PC-1500) or the Z-80 (PC-1600)
    enum class Mode { None, Mode0, Mode1 };      // None on the PC-1500
    struct Write { bool stored = false; std::uint32_t firstMiss = 0; };   // all or nothing
    struct Module { bool present = false; std::uint32_t size = 0; int banks = 0; };

    // Reads as the processor sees memory now; no effect on the machine.
    virtual std::uint8_t peek(std::uint16_t address, Processor = Processor::Main) const = 0;
    virtual Mode mode() const = 0;
    virtual int programArea() const = 0;         // PC-1600: 0 S0, 1 S1, 2 S2 (TITLE); PC-1500: 0
    // The built-in RAM: PC-1500 its RAM in ME0; PC-1600 the internal RAM at &C000-&FFFF.
    virtual Write writeBuiltIn(std::uint16_t address, const std::uint8_t*, std::size_t) = 0;
    // PC-1600: a module's RAM in global bank 0-3 behind &8000-&BFFF, whichever bank is switched in.
    virtual Write writeSlot(int bank, std::uint16_t address, const std::uint8_t*, std::size_t) = 0;
    virtual bool slotRam(int bank, std::uint16_t address, std::size_t) const = 0;
    virtual Module module(int slot) const = 0;   // slot 1 or 2
    virtual std::vector<std::uint8_t> internalRam() const = 0;   // as today (the probes, the tests)
};
}
```

- Every write is all or nothing: the range is checked first, and a
  write that is not stored says so with the first address that is not
  RAM (AC-10, AC-14). This is where the PC-1500's change of OQ-2 lives.
- `poke` (today `:41`) becomes `writeBuiltIn`. Its users outside the
  loaders are the tests' `memoryOf` (`tests/SharpTestSupport.hpp:216`)
  and the probes (`tools/CliCommon.hpp:127` only peeks); they follow in
  the same step.
- The PC-1500 answers `Mode::None`, area 0, no modules, `writeSlot`
  false. Its memory modules lie in ME0 and are reached by `writeBuiltIn`,
  as the loaders do today.

### 3.2 What a file is (AC-1, AC-2, AC-3, EC-1, EC-2, EC-4)

```cpp
namespace sharp::loaders {
struct FileKind {
    programfile::Kind kind;                 // Empty, BasicListing, BasicPC1500, BasicPC1600, CodeLH5801, CodeZ80, Headerless, Other
    std::string words;                      // Other: "plain text", "a Reserve Area as text", ... (programfile::describe)
    std::optional<machinecode::Cpu> looksLike;   // Headerless: LH5801 or Z-80 code by SDE's guess; never picks the CPU
    std::optional<Header> header;           // AC-2: name, load address (PC-1600: with bank), length, start address (0 / FFFF: none)
    bool damaged = false;                   // EC-2: a header's start without a complete header of a known kind
    std::vector<std::string> tapeFiles;     // AC-3: a WAV's readable files in tape order; empty: not a WAV, or none readable
};
FileKind fileKind(const std::vector<std::uint8_t>& bytes);
// AC-3, EC-4: the image of the file `n` (from 1) of a WAV, or the bytes themselves; n = 0: the first.
std::optional<std::vector<std::uint8_t>> image(const std::vector<std::uint8_t>& bytes, int n, std::string* why);
}
```

- `fileKind` is `programfile::classify` (`core/ProgramFile.cpp:59-96`)
  and `programfile::tapeFiles` (`:48-57`), unchanged rules (AC-1, OQ-1).
  A WAV whose files cannot be read is `Other` with the words "a cassette
  recording without a file that can be read" (AC-3).
- Every loader asks `fileKind` first, so an empty file is refused as
  empty by each (EC-1). Today the machine-code loader took it as
  headerless (`core/MachineCodeFile.cpp:62-69`).
- A loader given a WAV and no number takes the first readable file
  (EC-4; the preset, AC-13 of C1600-4).

### 3.3 Loading BASIC (AC-4, AC-5, AC-6, EC-3, EC-5)

```cpp
namespace sharp::loaders {
struct BasicLoaded { bool ok = false; std::uint16_t start = 0, end = 0; std::string refusal; };
BasicLoaded loadBasic(calcu::machine::Machine&, const std::vector<std::uint8_t>& image);
}
```

- The model comes from the machine's descriptor; the memory from
  `LoaderMemory` alone. The rules are those of today's loaders
  (`core/PC1500/PC1500BasicLoader.cpp:62-115`,
  `core/PC1600/PC1600BasicLoader.cpp:56-73, 158-194`,
  `core/PC1600/PC1600ProgramPlacement.cpp:270-286`): the listing
  tokenized for the model and mode (AC-5), the program area that is
  selected (AC-6), the pointers checked before anything is written; no
  reset, no NEW, mode and area as before (AC-4).
- Machine code given to it is refused with the hint to load it as
  machine code (EC-5). Pointers not of a set-up machine are refused with
  the hint to run NEW first (EC-3).
- Not an operation (AC-24): it writes at once, between two host calls.

### 3.4 Typing a program in (AC-7)

```cpp
namespace sharp::loaders {
// Inside an operation; the presses through the typing engine (LIB-5 AC-16, AC-28).
calcu::sessions::Task<BasicTypeResult> typeProgram(calcu::sessions::OperationContext&,
                                                   calcu::typing::TypingEngine&,
                                                   calcu::machine::Machine&, std::string listing);
}
```

Per line of the listing, an empty line left out:
1. The whole listing is checked first with the profile's `charKeys`
   (LIB-5 D-14): a character without a key refuses the command before
   anything is typed.
2. On the PC-1600 a line of more than 79 characters is not typed and
   counts as not stored (`core/PC1600/PC1600BasicTyper.cpp:36`, today's
   count of code points).
3. Before: the end of the program and the line's own record
   (`basic::LineStoreCheck`, `core/Basic/BasicLineStoreCheck.hpp`, read
   through `LoaderMemory::peek`).
4. `typeTextAndWait(ctx, engine, line, enter = true)`.
5. Wait until `TypingSupport::promptReady()` is true, at most 12 s of
   emulated time; then until the end of the program has not moved for
   30 frames, at most 6 s (`ctx.run` in pieces of one frame).
6. Stored when the end or the line's record changed.

Afterwards: no line stored, or some not: failed, naming them ("no
program line was stored" with today's PRO-mode hint, or "`n` program
line(s) not stored", `PC1600BasicTyper.cpp:332-339`). The loader never
presses MODE. The typer reads no machine class.

### 3.5 Machine code (AC-8, AC-9, AC-10, AC-11, AC-12, AC-23, EC-1)

```cpp
namespace sharp::loaders {
struct CodeRequest {                       // what a caller gives by hand
    std::optional<std::uint32_t> address;  // in the processor's space (AC-9)
    std::optional<std::size_t> length;
    std::optional<machinecode::Cpu> cpu;   // headerless only: the caller names it (the debugger, `cpu=`)
};
// The one plan (AC-8, AC-9, AC-23): machinecode::planLoad with the state read from the kind.
machinecode::LoadPlan planCode(calcu::machine::Machine&, const std::vector<std::uint8_t>& image, const CodeRequest&);
struct CodeLoaded {                        // AC-11, AC-12
    std::string range;                     // "&C0C5-&C0EE"
    std::string area;                      // PC-1600: "S0", "S1", "S2"; empty on the PC-1500
    int bank = 0;
    machinecode::Advice advice;            // NEW and its note, the starting command and its note
    std::vector<std::string> warnings;     // pc1600WorkAreaWarning, "BASIC can overwrite it"
};
// Writes what the plan says; refuses with the plan's sentence, writes nothing then (AC-10).
std::variant<CodeLoaded, std::string> loadCode(calcu::machine::Machine&, const std::vector<std::uint8_t>& image,
                                               const CodeRequest&);
}
```

- **One plan.** `machinecode::plan()` (`core/MachineCodeFile.cpp:299-334`),
  which the menu used, goes; the menu, the preset, the debugger and the
  harness call `planCode`, which is `planLoad` (`:240-297`) with the
  options of the caller. The order of the refusals of AC-8's table is
  `planLoad`'s order; where today's order differs from the table, the
  table wins (task T5 checks each row in order). `checkRange` is always
  on: the plan checks the range before the writer (AC-8 "before
  anything is written").
- **Where it goes (AC-23):** `planLoad`'s rules as today, the bank from
  the header when no address is given by hand.
- **The processor (AC-9):** the header's, else the caller's, else the
  mode's. Every sentence names addresses in that processor's space
  (`inCpu`, `:36`).
- **The write (AC-10):** `writeBuiltIn` or `writeSlot` of the kind,
  all or nothing; a block that runs past &BFFF into the internal RAM is
  checked as two ranges first and written only when both hold.
- **The reply (AC-11, AC-12):** `advice` (`:350-505`) with RAM_ST /
  RAM_END read through `peek` on the PC-1500 (today `pc1500UserRam`,
  `core/PC1500/PC1500MachineCodeLoader.cpp:32-35`).

### 3.6 Loading and starting (AC-13)

`load-code … run` loads by §3.5 and, when the file's header has an
address to start at, types the starting command with ENTER through the
engine (`typeTextAndWait`, origin Preset) and waits until the machine
is idle: prompt readiness true for 20 frames in a row (D-4), at most one
hour of emulated time. After the hour it is done all the same, and a
line of the log says so. Without such an address nothing is started and
the reply says so. It is an operation (AC-24). Within a preset the wait
is the preset's own (C1600-4 EC-3, M-46); until M-46 the format-1
runner keeps its `waitUntilBasicIdle` with its hour
(`core/Preset/PresetRunner.cpp:263`).

### 3.7 The session part, its commands and the harness (AC-21, AC-24, EC-6)

`SharpLoaderPart` declares (in `app/SharpTopics.hpp`):

| Name | Kind | Arguments | Reply |
|---|---|---|---|
| `c1600.file-kind` | request | `{bytes}` | `FileKindReply` (§3.2): kind, words, header, `looksLike`, `tapeFiles` |
| `c1600.load-basic` | command, at once | `{bytes, name, file?}` | done, or refused with the sentence |
| `c1600.type-basic` | command, operation | `{bytes, name}` | done, or failed with the lines not stored |
| `c1600.plan-code` | request | `{bytes, file?, address?, length?, cpu?}` | `CodePlanReply`: `refusal?`, `needsAddress`, `proposed` (AC-8's row "headerless"), `length`, `cpu` with its note, `where` (the place the code would go, or the warning) |
| `c1600.load-code` | command; an operation with `run` | `{bytes, name, file?, address?, length?, cpu?, run?}` | `CodeLoadedReply` (§3.5), or refused with the sentence |

- `file` chooses a file of a WAV (1-based); without it the first
  (AC-21). An address arrives as the user typed it (`C000`, `&C000`,
  `$C000`, `0xC000`; `machinecode::parseHexAddress`,
  `core/MachineCodeFile.hpp:216-237`); a text it cannot read is the
  plan's refusal "Not a hex address" (today's hint).
- **Busy (AC-24, EC-6).** The at-once commands and requests are
  refused while an operation runs, with LIB-3's sentence; the two
  operations are refused by LIB-3 AC-16. A preset's `program:` does not
  go through these commands: it calls `sharp::loaders` inside its own
  operation (§3.10), which is LIB-3 AC-17's "unless that operation
  itself calls it".
- **Stateless.** Each call carries the bytes (at most a few KB for a
  Sharp program; a WAV up to 1 MiB). The part keeps nothing between the
  dialog's calls (D-3).

The harness (AC-21): `HarnessProduct::addCommands` registers under
`c1600` (LIB-11 AC-13):

| Command | Sends | Reply lines |
|---|---|---|
| `c1600 file-kind <file>` | `c1600.file-kind` | `kind: …`, the header's fields, `words: …`, for a WAV `file n: <name>` per file |
| `c1600 load-basic <file> [<n>]` | `c1600.load-basic` | `done`, or `refused: <sentence>` |
| `c1600 type-basic <file>` | `c1600.type-basic` | `done`, or not done with `not stored: <line>` per line |
| `c1600 load-code <file> [<n>] [address=<a>] [length=<l>] [cpu=<lh5803\|z80>] [run]` | `c1600.load-code` | `range:`, `area:`, `bank:`, `new:`, `new-note:`, `start:`, `start-note:`, `warning:` per warning |

The harness session gets the part from `sessionParts()` (it has the
machine of the harness's own start, A-1 of LIB-11 1600, and the typing
part; it needs nothing of `SharpAppPart`). The CLI's `--help` lists the
four (LIB-11 AC-13).

### 3.8 One sentence per case (AC-25, EC-2, OQ-4)

`core/Loaders.cpp` holds one function that words every refusal:
`std::string refusal(const Refusal&)`, where `Refusal` is the case and
its numbers (AC-8's rows, the BASIC loader's cases, the classifier's
"empty", "damaged header", "not a program file"). The words are the
menu's of today: `machinecode::plan()` and `MachineCodeLoad::setAddress`
(`app/MachineCodeLoad.cpp:67-110`) for machine code, the BASIC loaders'
`BasicLoadResult::error` for BASIC. The sentence starts with the file's
name: "`<name>`: `<reason>`". The menu shows it as the notice's text
(AC-17, LIB-14 AC-17); the harness as `refused:`; the preset puts its
line number before it (format 1 until M-46: its `tag`, "section N: ",
which is today's prefix; from M-46 the line number of LIB-6's
refusals). The preset's own words
of today (`core/Preset/PresetRunner.cpp:208-232`, "has no machine-code
header: 'address' is required" and the like) go.

### 3.9 The frontends (AC-15 to AC-20)

Both frontends keep their two menu items and dialogs where they are
until M-49 (§1), and talk to the part by messages only.

Load BASIC Program… (AC-15, AC-16, AC-17): the dialog offers `.bas`,
`.bbin`, `.wav` and all files (Qt6 `MainWindow.cpp:179`; Web `pickFiles`
`CalcU1600.jsx:269`), starting in the Basic folder or the last used
(Qt6, LIB-9 AC-29, `AppSettings::openStartDir`). Then `c1600.file-kind`;
a WAV with several files asks "`<file>` holds `<n>` files. Load which
one?" with the list (Qt6 `QInputDialog`, `MainWindow.cpp:413-421`; Web
`choice`), Cancel loads nothing; then `c1600.load-basic`; a refusal is
the notice.

Load Machine Code… (AC-15, AC-16, AC-18, AC-19): `.bin`, `.wav` and all
files, starting in the Assembly folder. File kind, WAV choice as above,
then `c1600.plan-code`. A refusal is the notice. Headerless code opens
the address dialog (Qt6 `MachineCodeLoadDialog`, Web `AddressDialog`):
the proposed address, the length, the processor note; each change of the
text sends `c1600.plan-code` with the address, shows `where` or the
refusal, and enables Load only while the plan holds. Load sends
`c1600.load-code` without `run`; the reply is shown in "Machine Code
Loaded" under "Keep BASIC from overwriting it:" and "Start it:"
(`MainWindow.cpp:458-480`, `Dialogs.jsx:134-150`); then the starting
command goes to the input line with LIB-5's `paste` (no ENTER). The menu
never sends `run`.

### 3.10 The other callers

- **Format-1 presets (until M-46).** `runProgram` and
  `loadBinaryProgram` call `sharp::loaders::loadBasic`, `planCode`,
  `loadCode`, `refusal` on the machine; `PresetMachine`'s loader
  functions go. A typed `program:` calls `typeProgram`: the preset
  operation's `OperationContext` and the session's typing engine reach
  the format-1 runner through `legacy::applyPresetJob`
  (`core/Machine/LegacyCalls.hpp:52-55`, two more parameters;
  `SharpAppPart::runOperation`, `app/SharpAppPart.cpp:346-357`, passes
  the context it now drops). That adapter goes in M-46 as planned.
  The auto-run keeps today's `typeLine` + `waitUntilBasicIdle` until
  M-46 (§3.6).
- **The debugger (until M-45 and M-50).** `debug::loadProgram`
  (`core/Debug/ProgramLoader.cpp`) plans with `planCode` (its `cpu`
  from the toolchain, AC-9) and writes with `loadCode`; its signature
  takes `Machine&` instead of the two machine pointers, so
  `legacy::loadProgram` (`LegacyCalls.hpp:46-47`) passes the machine.
  That adapter stays for M-45.
- **Today's tools.** `pc1500_cli`, `pc1600_cli` and the probes load
  programs only through presets; they follow the preset loaders without
  change of their own. `dev/loader-matrix/` (its own build) drives
  presets too and is run once in this step (§8).

## 4. Product side: what is replaced

| Today | After M-44 |
|---|---|
| `sharp::legacy::loadBasicProgram`, `loadMachineCode`, `machineCodeLoadState`, `userRam` (`core/Machine/LegacyCalls.hpp:35-42`, `.cpp:20-45`) | `sharp::loaders` on the kind (§3) |
| two plans: `machinecode::plan` (menu) and `planLoad` (preset, debugger) | `planCode` = `planLoad` (TR "one plan") |
| `app/MachineCodeLoad.*` on `const SharpAppPart&` (the Qt6 frontend calls it on the part from its own thread, `MainWindow.cpp:440, 459`) | `c1600.plan-code`, `c1600.load-code` replies |
| `c1600.load-basic {bytes, path}`, `c1600.load-machine-code {bytes, addr, slot, bank}` (`app/SharpAppPart.cpp:283-299`): the frontend gives slot and bank | the frontend gives the file and what the user typed; the plan decides slot and bank |
| the Web page's stateful commands (`app/SharpWebPart.cpp:190-233`) | the part's stateless ones |
| three wordings of a refusal | one (§3.8, row 1600-15) |
| `typeBasicProgramTextJob` of both typers (`PC1500BasicTyper.cpp:195-255`, `PC1600BasicTyper.cpp:291-363`), keys pressed by the typers themselves | `typeProgram` over the typing engine |
| the PC-1500 writer that writes the rest and then fails (`PC1500MachineCodeLoader.cpp:15-29`) | all or nothing (AC-10) |
| no loader commands in the harness | `c1600 file-kind`, `load-basic`, `type-basic`, `load-code` |

## 5. Adapters

Removed in M-44:

| Adapter | File | Since |
|---|---|---|
| `legacy::loadBasicProgram`, `loadMachineCode`, `machineCodeLoadState`, `userRam` | `core/Machine/LegacyCalls.*` | M-26 (C1600-1 design §5, "M-44") |
| the Sharp preset typers' `typeBasicProgramTextJob` | `core/PC1500/PC1500BasicTyper.*`, `core/PC1600/PC1600BasicTyper.*` | LIB-5 design §5, "M-44 (C1600-2)" |

New or changed, each with the step that removes it:

| Adapter | File | Wraps | Removed in |
|---|---|---|---|
| `applyPresetJob`'s typing hand-off | `core/Machine/LegacyCalls.hpp:52-55` | the operation context and the typing engine, passed into the format-1 runner for a typed `program:` (§3.10) | M-46 (LIB-6 1600: the runner goes) |
| `loadMachineCode`/`loadBasicProgram` of the frontends' drop | `frontends/qt6/app/MainWindow.cpp:799-819`, `frontends/web/src/CalcU1600.jsx:174-186` | today's drop flow, now on the new commands, still classified by `dropfile::classify` | M-48 (LIB-13 1600) |

`legacy::loadProgram` (debugger, M-45) and `legacy::typeLine` (the UART
probe, M-51) stay as planned; their signatures follow §3.10.

## 6. Tasks

Tests before code in each task. Program `CoreTests` (Frequent) unless
named; Web cases in `CalcU1600WebPlaywright` (Longer). Case names
`c1600_2_acN_…` / `c1600_2_ecN_…`; the Web cases' titles start with the
ID (`C1600-2 AC-18 …`, ADR 0032). Machines from `tests/SharpTestSupport.hpp`
with the real ROMs (`SKIP_UNLESS` without them).

| Task | What | ACs/ECs | Test (file: cases) |
|---|---|---|---|
| T1 | The kind | AC-14 | `loader_memory_tests.cpp`: `c1600_2_ac14_listed_by_every_sharp_model`, `_peek_without_effect`, `_peek_lh5803_view_pc1600`, `_mode_and_area_pc1600`, `_write_built_in_and_slot_any_bank_switched_in`, `_slot_ram_and_module_size_banks`, `_write_not_stored_says_so_writes_nothing` |
| T2 | The classifier | AC-1, AC-2, AC-3, EC-1, EC-2, EC-4 | `program_files_tests.cpp`: `c1600_2_ac1_every_kind_by_content_not_name`, `_headerless_looks_like_lh5801_or_z80`, `_other_with_words`; `c1600_2_ac2_ce158_header_fields`, `_pc1600_header_fields_with_bank`, `_start_zero_or_ffff_none`; `c1600_2_ac3_wav_files_in_tape_order_with_names`, `_wav_none_readable_is_other_with_words`; `c1600_2_ec1_empty_refused_as_empty_by_every_loader`; `c1600_2_ec2_partial_header_refused_saying_so`; `c1600_2_ec4_wav_first_file_by_default` (the cases of `program_file_tests.cpp:21-190, 272-300` move here) |
| T3 | BASIC load | AC-4, AC-5, AC-6, EC-3, EC-5 | `basic_fastloader_tests.cpp`, `pc1600_basicloader_tests.cpp` (ported to the kind): `c1600_2_ac4_same_memory_as_typed_pc1500`, `_pc1600` (today `:131`, `:421`), `_nothing_else_changes_mode_area_kept`; `c1600_2_ac5_listing_tokenized_per_model_and_mode`, `_mode1_non_ascii_refused`, `_tokenized_file_per_machine_and_mode`; `c1600_2_ac6_pc1500_program_start`, `_s0_parts_in_table_order_line_not_split`, `_s1_s2_in_module`, `_refused_names_pointer_or_room_nothing_written`, `_slot_without_program_module_refused`; `c1600_2_ec3_new0_check_hint_to_run_new`; `c1600_2_ec5_code_to_basic_loader_hint` |
| T4 | Typing in | AC-7 | `basic_typing_tests.cpp` (a session with TypingPart and the loader part, `app/tests/SessionFixture.hpp`, virtual time): `c1600_2_ac7_lines_typed_with_enter_by_engine_pc1500`, `_pc1600`, `_empty_line_left_out`, `_char_without_key_refuses_before_typing`, `_waits_prompt_then_end_stable`, `_line_over_79_not_typed_pc1600`, `_names_lines_not_stored`, `_no_line_stored_fails_with_pro_hint`, `_mode_not_pressed`; `c1600_2_ac7_memory_equals_old_typer` (the program of `basictyper_tests.cpp` and `pc1600_basictyper_tests.cpp`, compared with the old typer's memory recorded once before the code changes, as fixture `tests/fixtures/typed-*.bin`) |
| T5 | The plan | AC-8, AC-9, AC-23 | `machine_code_file_tests.cpp`: `c1600_2_ac8_row1_not_code_basic_hint`, … one case per row of the table, `_rows_checked_in_order`, `_same_plan_menu_preset_debugger_harness`; `c1600_2_ac23_address_by_hand_else_header`, `_length_by_hand_else_payload`, `_bank_from_header_or_area`, `_headerless_no_bank`; `c1600_2_ac9_cpu_by_header_caller_mode`, `_addresses_in_cpu_space` (the cases of today's file, `:65-461`, kept and renamed where they are these) |
| T6 | The write | AC-10 | `machine_code_file_tests.cpp`: `c1600_2_ac10_written_any_bank_switched_in`, `_not_ram_nothing_written_pc1500`, `_not_ram_nothing_written_pc1600`, `_crossing_bfff_both_checked` |
| T7 | The reply | AC-11, AC-12 | `machine_code_file_tests.cpp`: `c1600_2_ac11_parts_of_reply`, one case per row of the NEW table (today `:349`, `:367`), `_call_forms_call_callbank_xcall`, `_start_address_header_else_first_byte`; `c1600_2_ac12_work_area_warning_loaded` |
| T8 | Start | AC-13 | `app/tests/loader_part_tests.cpp`: `c1600_2_ac13_run_types_start_and_waits_idle`, `_without_start_address_nothing_started`, `_bound_reached_done_with_log_line` (the hour is a member of the part, set to 2 s by the test, D-5) |
| T9 | The part | AC-21, AC-24, EC-6 | `loader_part_tests.cpp`: `c1600_2_ac24_load_at_once_not_operations`, `_type_and_run_are_operations`, `_refused_while_another_runs`; `c1600_2_ec6_menu_and_harness_refused_while_busy`; `tests/harness_tests.cpp`: `c1600_2_ac21_file_kind`, `_load_basic_with_n`, `_type_basic_lines_not_stored`, `_load_code_options_and_run`, `_listed_under_c1600` |
| T10 | One sentence | AC-25 | `loader_part_tests.cpp`: `c1600_2_ac25_same_sentence_menu_harness_preset` (each case of §3.8 through `c1600.load-code`, `c1600 load-code` and a format-1 preset; the preset's has its prefix) |
| T11 | Today's tests | AC-22 | the files of the spec, ported: `program_file_tests.cpp` (its drop cases `:193-300` stay for `dropfile::classify` until M-48), `basic_fastloader_tests.cpp`, `pc1600_basicloader_tests.cpp`, `machine_code_file_tests.cpp`, `presetloader_trace_tests.cpp:326-360`, `app/tests/shared_app_session_tests.cpp:193-252` → `loader_part_tests.cpp` (`c1600_2_ac22_…`: one wav one image, two files listed, address checked as typed) |
| T12 | Frontends | AC-15–19 | Web `play.spec.js`: `C1600-2 AC-15 the two File items and what they offer`, `C1600-2 AC-17 a BASIC program is loaded, a refusal is a notice`, `C1600-2 AC-19 the loaded dialog and the CALL in the input line, not started`. Qt6: built, checked by hand in M-44 (both items, a WAV, a headerless file); the scenarios in M-49 |
| T13 | GUI scenarios | AC-20 | Web: `play.spec.js:297` renamed `C1600-2 AC-18 headerless code asks the address, then NEW and CALL`, `:317` `C1600-2 AC-16 a WAV with several programs asks which one`. Qt6: the same two as attach scenarios in M-49 (LIB-14 AC-44) |
| R1 | Reference and matrix runs | §8 | `tests/reference/v0.8.1-examples/run.sh` against the M-44 tree; `dev/loader-matrix/run_pc1500.sh`, `run_pc1600.sh`; by hand, results in the qa-report |

Coverage: AC-1 to AC-25 and EC-1 to EC-6 have a task; AC-15 and the Qt6
half of AC-20 pass in M-49.

## 7. Tools and agent knowledge

**Tools.** The harness gets the four `c1600` commands (§3.7): the exit
check of M-44 ("every loader checked through the harness") is T9's
harness cases plus one run by hand of each command on a PC-1500A and a
PC-1600. `pc1500_cli`, `pc1600_cli` and the probes are unchanged for
their users (§3.10); `tools/check_presets.sh` is not affected.

**Agent knowledge changed in M-44.**

| File | Change |
|---|---|
| `products/calc-u-1600/CLAUDE.md` | § Harness: the four `c1600` commands; "Not yet" loses "the loaders" |
| `products/calc-u-1600/docs/developer/README.md` | where the loaders live (`core/Loaders`, `app/SharpLoaderPart`) (plan M-44) |
| `products/calc-u-1600/docs/developer/Shared-Architecture.md:75` | the row `MachineCodeLoad` becomes `SharpLoaderPart` |
| `products/calc-u-1600/docs/developer/Web-Client.md` | the page's loader messages (§2, engine) |
| `docs/glossary-mapping.md` | a row "Plan": `machinecode::plan`, `MachineCodeLoad` → `sharp::loaders::planCode`, `c1600.plan-code`; a row "Program file kind": `programfile::classify` → `sharp::loaders::fileKind` |
| `docs/test-groups.md` | the new and ported files of §6 in the Calc-U-1600 table |
| memory `calc-u-1600/project_fast_basic_binary_loader.md` | its "Code:" lines (`loadBasicProgram(machine, bytes)`, `loadBasicProgramFile(machine, path)`) name `sharp::loaders::loadBasic` on the kind |
| memory `calc-u-1600/project_pc1600_native_basic_and_preset_program.md` | `typeBasicProgramText` → `sharp::loaders::typeProgram` over the typing engine |

`docs/background/plans/Loader-*` stay history (plan M-44). No
Calc-U-1600 skill names the loaders (the product has no skills).

**Ledger.** Row 1600-15, its loader part: a PC-1500 machine-code load
that meets a byte that is not RAM writes nothing; an empty file is
refused as empty by every loader; one wording per refusal. The row's
other parts become visible in M-48.

## 8. Golden runs, reference runs and risks

- **Golden runs** (`tests/golden_runs_tests.cpp`) drive the machine and
  use no loader; the kind's new shape does not touch the core's memory
  model. A difference is a finding.
- **Reference runs (M-4).** The examples that load programs (the
  `machine-code_*`, `setup_*`, `memory_*`, `basic_hanoi` and the
  plotter examples with a `program:`) load through the format-1 runner,
  which now calls `sharp::loaders`. R1 runs `run.sh` with the M-44 tree
  and compares the `.out` files (display, BASIC pointers, program area,
  machine-code area): they must be equal. The `.log` times of a typed
  `program:` may move (§3.4 waits by prompt readiness, not by the PC
  window of `settleUntilIdle`); each difference is a finding in the
  qa-report, not a new reference.
- **Loader matrix.** `dev/loader-matrix` (ROM against loader, by preset
  `program: file:`): PC-1500 72 of 80 with the 8 known deliberate
  differences, PC-1600 250 of 250 (memory `project_loader_matrix_pc1500.md`)
  must hold after M-44.
- **Risk: typing in through the engine.** The typer's keys now follow
  the profile's cadence (4 + 4 frames, LIB-5 AC-28) and wait for prompt
  readiness, where the old typer pressed keys itself and waited for the
  PC to settle. A line could be judged before the ROM has stored it.
  T4's comparison with the old typer's memory guards it. If a typed
  preset fails in R1, the preset path keeps the old typer as an adapter
  until M-46 (one line in `PC1500PresetLoader.cpp` / `PC1600PresetLoader.cpp`)
  and the menu's and harness's `type-basic` use the new one; the
  qa-report then records the deviation.
- **Risk: the order of AC-8.** `planLoad` checks in its own order; if a
  file meets two rows, the reason may change from today's. T5's
  `_rows_checked_in_order` fixes the spec's order.
- **Risk: the plan request per keystroke** sends the file's bytes each
  time (D-3). A Sharp program is a few KB; a WAV is decoded each time
  (tens of ms for a long tape). If the Web dialog lags, the part keeps
  the last decoded WAV by its bytes' hash (an implementation detail; no
  change of the messages).
- **Frontends in one step.** Qt6 is built and checked by hand; the Web
  by Playwright (Longer, at the end of the step). CI is not dispatched
  (plan §1): no workflow or packaging changes.

## 9. Decided (best guess)

- **D-1 (layout).** The loaders' entry points are `core/Loaders.*` and
  `core/BasicTyping.*` (namespace `sharp::loaders`), next to today's
  `core/ProgramFile.*` and `core/MachineCodeFile.*`, which stay. The
  per-model writers keep their files and lose their machine classes.
  Fewer moved files than a new folder.
- **D-2 (the part).** A part of its own, `SharpLoaderPart`, not more
  commands in `SharpAppPart`: the harness needs the loaders in M-44 and
  carries no `SharpAppPart` before M-51 (LIB-11 D-6).
- **D-3 (stateless).** Each request and command carries the file; the
  part keeps no "picked program". One message set for Qt6, Web and the
  harness.
- **D-4 (idle, AC-13).** "Idle" is prompt readiness held for 20 frames
  (a third of a second), the notion of today's `waitUntilBasicIdle`
  (memory `project_preset_parameterless_wait.md`: "held continuously
  for ~a third of a second") over typing support instead of PC windows.
- **D-5 (the hour, AC-13).** The bound is a member of the part, one
  hour by default; tests set it shorter. The harness's own one hour for
  `key` and `type` (LIB-5 D-5) is not used here: the command is an
  operation and replies at its end.
- **D-6 (typing in, AC-7).** The whole listing is checked for keys
  before the first line (the spec: "refuses the command before anything
  is typed"); today the typer stops at the line with the character.
- **D-7 (sentences, AC-25).** The sentence starts with the file's name;
  the preset's prefix is its `tag` until M-46. Today's menu words are
  kept word for word where the menu had them.
- **D-8 (CPU names in the harness).** `cpu=lh5803` or `cpu=z80`, any
  case, the names of the spec's processors.
- **D-9 (menus until M-49).** The two items stay in today's menus and
  dialogs and go into the LIB-14 shell with the rest of the window in
  M-49 (P-1).

## 10. Plan notes

- **P-1 (M-44 "Does").** The step text says "the two menu items and
  their dialogs in the LIB-14 shell". Calc-U-1600 moves onto the shell
  in M-49, and §5.1 already has C1600-2 Done in M-49 "AC-15 with M-49".
  Proposed wording: "the two menu items and their dialogs on the new
  commands, in today's menus; in the LIB-14 shell with M-49".
- **P-2 (§5.1, C1600-2 row).** "Tested in: M-44 (AC-15 with M-49)"
  becomes "M-44 (AC-15 and the Qt6 scenarios of AC-20 with M-49)".

## 11. Questions for the user

1. **Typed `program:` of a preset in M-44 or M-46?** The design moves a
   preset's typed program onto the new typer already in M-44 (one typer;
   LIB-5 design §5 says so), with the old typer as fallback if the
   reference runs differ (§8). The other way is to leave presets on the
   old typer until M-46 and use the new one only for `type-basic`.
   Recommendation: M-44, as written.
   Answered (user, 2026-11-16, with the approval of the Part D designs): M-44, as written.
