# C59-2 TI preset content: design
Status: approved by the user at the Part D/E boundary (2026-11-16)

## 1. Scope

| Row | Step | Tests from (plan §3) | Done in (plan §5.1) |
|---|---|---|---|
| C59-2 | M-54 (with LIB-6 59, LIB-18 59, C59-3) | AC-1–18, EC-1–6 | M-65 (AC-10's four card examples in M-55, AC-12's site copies in M-65) |

M-54 is one step with four designs: this one (the product's sections,
the program memory kind, the `ti59` commands of AC-9, the preset files),
the Calc-U-59 column of LIB-6 (`features/LIB-6-presets/design.md` §11:
the product's rules, the reader of 2.3.0 gone, the frontends and tools on
the library's loader), that of LIB-18 (`features/LIB-18-machine-control-and-startup/design.md`
§11: the start, the switches and the resets) and C59-3
(`features/C59-3-ti-58c-constant-memory/design.md`). This design names
what only the TI presets have; the others name the wiring.

- **Waits for** (plan M-54): M-3 (FS 5: key readiness tells idle from
  running, `docs/walkthroughs/first-steps.md` §5; FS 11), M-8 (FS 7,
  FS 9), M-9 (FS 6: the reference runs of the 57 examples,
  `products/calc-u-59/tests/reference/v2.3.0-examples/`), M-25 (C59-6:
  the module slot and the PC-100C), M-34, M-35, M-53 (LIB-8 59: `paper
  text`, `paper marks` on the PC-100C; LIB-10 59: the debugger steps
  `pause:`, `pause-on-start:`, `step:`, `resume:`, `trace:`,
  `debug-panel:` that four examples use).
- Uses the library as it is on `main` after Part D (`7bdd51c`):
  `library/presets/` (LIB-6 L, Done), `library/parts/LibraryParts`
  (Part D simplification P-2), `library/startup/` (LIB-18 L, Done).
- Paths below without a prefix are under `products/calc-u-59/`. Line
  numbers are at `7bdd51c`; the spec's `Shared/`, `Web/`, `Qt6/` paths are
  at `v2.3.0` (`app/`, `frontends/web/`, `frontends/qt6/` here).
- Hardware facts are not restated here; they are in `rom/TI59-ROM-Guide.md`,
  `rom/TI58C-ROM-Guide.md` and the research skill `ti59-research`.

**What waits for M-55** (plan question 6, applied as C1600-4 did for
M-47): the presets that name a magnetic card cannot be read before
C59-7's keywords exist (`magnetic-cards:`, `card:`, `arm:`, `saveas:`).
They are the four of AC-10, which says so by name:

| Preset | Why it waits |
|---|---|
| `examples/globe-print.ti59` | `ArmCard:` with `CARD: mem:…` blocks |
| `examples/historic/characters-fastmode.ti59` | `CARD:`, `InsertCard:` |
| `examples/historic/pdcm-card-tricks.ti59` | `CARD:`, `ArmCard:`, `InsertCard:` |
| `examples/historic/pdcm-make-cards.ti59` | `CARD:`, `InsertCard:` |

They stay in the 2.3.0 format in M-54, are excluded by name from the
suite check (LIB-6 AC-43 says so), from `examples/bundle.yaml`
(`globe-print` is one of the ten examples of AC-13; D-13) and from the
reference comparison, where they are listed as waiting (§6 T8). Every
other example (53 files) is upgraded in M-54. A search of the examples
for `CARD:`, `ArmCard:` and `InsertCard:` finds these four only.

Clauses that pass in later steps (plan §1, "Completes" and Done):

| Clause | Passes in |
|---|---|
| AC-6's Qt6 GUI scenario (a preset's cue card title shows) | M-57 (LIB-14 59: the Qt6 scenarios with the IDs of the Web ones; no Calc-U-59 Qt6 test target exists before it, `docs/test-groups.md`) |
| AC-10, AC-11 for the four card examples; AC-13's `globe-print` | M-55 (C59-7) |
| AC-12 (the site's copies) | M-65 (plan §5.1) |
| AC-14's scenario in Qt6 and Swift (the bundled example loads in each) | M-57 (Qt6), M-58 (Swift): LIB-6 AC-42, LIB-6 §11.1 |

## 2. Modules and files

### New in `app/` and `core/`

| File | Holds |
|---|---|
| `app/TIPresetKeys.hpp/.cpp` | `cu59::addTIKeywords(presets::Vocabulary&)`: the sections `partition:` (AC-2), `program:` (AC-1, AC-3), `data:` (AC-4), `cue-card:` (AC-5, AC-6) and the step `printer-trace:` (LIB-6 AC-40); called from `TIPresetRules::addKeywords` (LIB-6 §11) |
| `app/Keycodes.hpp/.cpp` | `cu59::readKeycodes(std::string_view text, int lastStep) -> std::variant<std::vector<ProgramStep>, KeycodeError>`: the compact and the listing notation of `preset.md` §4.1 (AC-3, OQ-5); `readKeystrokeProgramFile` (format `ti-keystroke-program`, version 1; §4.1.1). Used by `program:` and by `ti59 program` (AC-9) |
| `app/ProgramMemory.hpp` | the product kind `c59.program-memory` (AC-17; §3.2): its interface, as the C59-6 kind `c59.printer-output` is declared in `core/PrinterOutput.hpp` |
| `core/TIProgramMemory.hpp/.cpp` | the kind's implementation on `TIMachine`, capability `RAM/c59.program-memory` on every TI model (§3.2) |
| `app/TIPresetCommands.cpp` | the product commands `c59.partition`, `c59.program`, `c59.registers` of `TIAppPart` (AC-9; §3.4) |
| `examples/bundle.yaml` | the bundle file of AC-13 (`docs/formats/bundle.md`; §4) |
| `examples/startup/default-ti59.ti59`, `default-ti58.ti58`, `default-ti58c.ti58c` | the three startup presets of AC-13 (§4) |
| `tests/preset_content_tests.cpp` | AC-1–9, AC-15–17, EC-1–6 (§6) |
| `tests/fixtures/presets/` | the small texts of the Technical Requirements, one error each (AC-2–AC-5), and the presets the part tests load |

### Changed

| File | Change |
|---|---|
| `app/CueCard.hpp/.cpp` | `readStandAloneCueCard(const formats::YamlNode&, Sink&)`: the stand-alone cue card of `cue-card.md` §3, §4 (AC-5, EC-5); `writeStandAloneCueCard` for C59-3's file. The 2.3.0 `CUECARD:` text reader and `CueCardTemplate` go (§3.3; D-7) |
| `app/Bcd.hpp/.cpp` | `parseDecimal(std::string_view) -> std::optional<…>`: a decimal literal with an optional exponent, or none; `encodeBcd(text)` (`app/Bcd.hpp:22-27`) is called only on a text it accepted (AC-4; today unparseable text gives zero) |
| `app/TIAppPart.hpp/.cpp`, `app/TIAppPartFlows.cpp` | the fitted own cue card set by `cue-card:` (`adoptPresetCue`, `TIAppPartFlows.cpp:108-114`, kept); the commands of AC-9 (`app/TIPresetCommands.cpp`); the preset flows go (LIB-6 §11.2) |
| `core/TIMachine.hpp/.cpp` | `capabilities()`/`capability()` list `RAM/c59.program-memory` (`core/TIMachine.hpp:46-52`) |
| `core/TIAdapters.hpp/.cpp` | `TIMemoryAdapter` (`core/TIAdapters.hpp:61-82`) loses `writeProgram`, `runInstructions` and the preset's other writes, now the kind's; it keeps the reads the debug log, the debug snapshots, the card files and the headless library use (§5) |
| `app/HarnessProduct.cpp` | `addCommands`: `ti59 partition`, `ti59 program`, `ti59 registers` (AC-9; `library/harness/Product.hpp:55`) |
| the 53 examples of §1, their comments; `examples/debug/readme.md`, `examples/calendar/CALENDAR_PROGRAMS.md`, `examples/assembly/README.md` where they name a 2.3.0 key or tool | version 2 by AC-10's table (§4) |
| `tools/agentic/calendar/fastcal.py:163-203`, `calendar-09-header.txt`, `years.py:21-25` | version 2 (AC-18; §4, LIB-6 §11.7) |
| `tools/goldens/formats_main.swift`, `gen.sh`, `tests/golden/formats.txt` | the 150 `STATE` records and their `END` lines go; `REG`, `BCDD`, `BCDT`, `MATH` stay (AC-16; the `CARD` records are M-55's) |
| `tests/reference_runs_tests.cpp` | the comparison of AC-11 on version 2 (§6 T8) |

### Deleted

`app/StateFile.hpp/.cpp`, `app/StateApplier.hpp/.cpp` (LIB-6 §11 names
the rest of the reader of 2.3.0), `tests/fixtures/statefiles/` (its three
files, Technical Requirements), `tests/shared_state_applier_tests.cpp` and
`tests/shared_preset_stages_tests.cpp` once their cases are ported (§6 T6),
the `STATE` cases of `tests/shared_formats_tests.cpp`
(`state_and_card_files_match_swift` loses its state half,
`keystroke_directives_unknown_or_without_unit_are_errors`,
`state_file_model`, `retired_skip_reset_and_key_99_are_errors`,
`control_directives`).

## 3. Seams and interfaces

### 3.1 What the product registers (TR: "through the place LIB-6 AC-29 gives")

```cpp
namespace cu59 {
void addTIKeywords(calcu::presets::Vocabulary&);   // called from TIPresetRules::addKeywords (LIB-6 §11.3)
}
```

| Keyword | Kind | Read checks (at read, with the line) | Carried out | After |
|---|---|---|---|---|
| `partition:` | section, a value (`SectionDef`, the value form LIB-6 §3.3 allows) | `NNN` or `NNN.digits` (the part after the point not read); not a number, or above the model's last step (959; 479 on the TI-58/58C): refused (AC-2) | the number of steps (value + 1) rounded up to a multiple of 80, at most 960, through the kind's `setPartition` | `Own` (finished when written; LIB-6 AC-15) |
| `program:` | section, a block | exactly one of `keycodes: \|` and `file:`; `keycodes` by `readKeycodes` with the model's last step; `file:` read by `ReadContext::readFile` (relative to the preset's folder; EC-5 without one) and by `readKeystrokeProgramFile` (`format`, `format-version: 1`, `program-name`, `keycodes` required; `description` optional; an unknown key refused) | the listed steps through `writeStep`, each at its place; every other step stays (AC-1, EC-1) | `Own` |
| `data:` | section, a mapping | each key `00`–`99`, or `H00`–`H03` on the TI-58C, matched without regard to case; 60–99 on the TI-58/58C, `H` on another model: refused; each value `parseDecimal`, else refused (AC-4) | each listed register through `writeRegister`, `H00`–`H03` through `writeExtra` (AC-4, EC-3); every other register stays | `Own` |
| `cue-card:` | section, a cue card block | `readStandAloneCueCard`: the keys of AC-5; `Template`, `Banks`, `id`, `model`, `printer`, `card` and every other key refused (AC-5, EC-5) | fitted as the machine's own card in place of the one before (AC-6, EC-4; `TIAppPart::adoptPresetCue`) | `TakenEffect` |
| `printer-trace:` | step, scalar `on`/`off` (`command = true`, LIB-6 §3.5) | the two words; without `printer: PC-100C` (`ReadContext::plugged`): refused (LIB-6 AC-40, AC-34) | the TRACE control of `printer/controls` (`peripherals::setControl`, LIB-2 AC-16) | `TakenEffect` |

Each section writes memory, so it waits for a running machine (LIB-6
D-27, AC-19). An item before it is waited for until the machine is idle
(LIB-6 AC-56), so a section after `keys:` is AC-7's reload: it is
carried out when the keys before it are done and the machine is idle,
and it changes only what it lists. The items keep their file order
(EC-2).

`partition:`, `program:`, `data:` and `cue-card:` are marked [59] by
`preset.md` §7; Calc-U-1600 does not register them, so a Sharp preset
that names them is refused as "unknown key" there (LIB-6 D-7). `program:`
is registered by both products under one name with each product's
reader (one name, two meanings by model: `preset.md` §4.1 gives the TI
and the Sharp form); a session has one product's vocabulary, so the two
never meet.

### 3.2 The program memory kind (AC-17)

```cpp
namespace cu59 {
// The product kind "program memory" (C59-2 AC-17; capabilities.md, product kinds).
// Capability `RAM/c59.program-memory` on the TI-59, TI-58 and TI-58C.
class ProgramMemory : public calcu::machine::Capability {
public:
    static constexpr calcu::machine::KindId kKind{"c59.program-memory"};
    virtual int lastStep() const = 0;                          // 959, 479 (the model's physical last step)
    virtual std::uint8_t readStep(int step) const = 0;
    virtual void writeStep(int step, std::uint8_t keycode) = 0;
    virtual int dataRegisterCount() const = 0;                 // 100 (TI-59), 60 (TI-58/58C)
    virtual Nibbles16 readRegister(int n) const = 0;           // data register NN, as STO/RCL number it
    virtual void writeRegister(int n, const Nibbles16&) = 0;
    virtual int extraCount() const = 0;                        // 4 on the TI-58C (H00–H03), else 0
    virtual Nibbles16 readExtra(int n) const = 0;
    virtual void writeExtra(int n, const Nibbles16&) = 0;
    virtual int partitionSteps() const = 0;                    // steps of program memory (a multiple of 80)
    virtual void setPartition(int steps) = 0;                  // the SCOM register the ROM keeps it in
    virtual Nibbles16 readRaw(int r) const = 0;                // raw RAM register R000…, for AC-11 and C59-3
    virtual int rawCount() const = 0;
};
}
```

- Where a program step, a data register and H00–H03 lie in the RAM and
  where the partition is kept are the core's today
  (`TIMemoryAdapter::writeProgramStep`, `writeDataRegister`,
  `setPartitionProgramRegs`, `writeRAMReg` with `kExtraRegisterBase`;
  `app/StateApplier.cpp:23-44`). The kind moves those calls behind
  `machine::Capability` (LIB-1 AC-11: the app reaches the machine only
  through capabilities); nothing about the layout changes.
- A write happens on the session's thread between two advances, as every
  capability call (LIB-1 §2a). What was written reads back (AC-17).
- The kind is listed by `TIMachine::capabilities()` with the owner
  component `RAM` (`core/TICatalogue.cpp:84`).
- The registers H00–H03 are not data registers 60–63 (AC-4): they are
  `writeExtra(0..3)`, raw registers 60–63 on the TI-58C
  (`rom/TI58C-ROM-Guide.md:37`).

### 3.3 Reading the blocks (AC-2 to AC-5)

- **Keycodes** (AC-3, OQ-5). A line is split at ` #` (README §1: a `#`
  after a space starts a comment). A first token of three or more digits
  is a step number (up to `lastStep`), followed by exactly one two-digit
  keycode; anything after it is the mnemonic and is not read (`473 00 0`).
  Otherwise every token is a two-digit keycode, written to the steps that
  follow the last step written (step 000 at the start of the section).
  Any other token (a one-digit number, a word) refuses the preset with
  the line of the token inside the file (the block scalar's first line
  plus its line in the block). No gap marker; a step number beyond the
  last step refuses it.
- **Values** (AC-4). `parseDecimal` takes `[-]digits[.digits][e|E[±]digits]`
  or `[-].digits…`; anything else is no number. A whole number is
  `encodeBcd(text)` digit for digit, cut after the 13th significant
  digit; any other value `encodeBcd(text)` rounded to 13 digits
  (`app/Bcd.hpp:22-31`; `preset.md` §4.2). The rule is today's
  (`Shared/Bcd.cpp:20-65`), so AC-16's `BCDD`, `BCDT` and `MATH`
  records still hold.
- **The cue card** (AC-5). `readStandAloneCueCard` reads `title`,
  `row-1`, `row-2`, `row-2-r`, `row-1-align`, `row-2-align`,
  `row-2-r-align`, `style`, `pencil-color`, `A`–`E` and `A'`–`E'` (`A′`
  accepted; the label keys compared in any case), with the defaults,
  `\blank` and the text tokens of `cue-card.md` §3, §4. The result is
  today's `CueCardContent` (`app/CueCard.hpp:36`) without a template: a
  stand-alone card always draws as the plain, hand-written card (AC-5;
  `cue-card.md` §1). `CueCardTemplate` (`app/CueCard.hpp:27`) stays only
  as long as the magnetic and module cards of today use it (M-55).

### 3.4 The commands of AC-9

Product session commands of `TIAppPart` (`app/TIPresetCommands.cpp`),
each through `ProgramMemory`, and the harness commands that send them
(`HarnessProduct::addCommands`, under the command word `ti59`,
`app/HarnessProduct.cpp:56`):

| Harness | Session command | Writes | Reads (no value) |
|---|---|---|---|
| `ti59 partition [<value>]` | `c59.partition {value?}` | the value as `partition:` reads it (refused where it would refuse the preset) | the partition as `partition:` writes it (`479`): the last program step |
| `ti59 program [<step>] <keycode>…` | `c59.program {step?, keycodes}` | the keycodes from `<step>` (three or more digits), else from 000, in the compact notation's rules | — |
| `ti59 program <step> count=<n>` | `c59.program {step, count}` | — | n keycodes from that step, one line each, `NNN KK` (Q-1) |
| `ti59 registers <NN>=<value>…` | `c59.registers {set}` | each as a `data:` entry (`H00`=… on the TI-58C) | — |
| `ti59 registers <NN>…` | `c59.registers {get}` | — | one line per register, `NN: <value>` as `data:` writes it (13 digits, exponent) |

A refusal gives the sentence the section would give, without a line. A
write is a change of memory, not an operation: it is done between two
host calls, at once (LIB-3 AC-12's rule for commands that start
nothing). While an operation runs the write commands are refused with
LIB-3's "Another operation is running." (as LIB-18 D-7 does for its
commands); the reads are not.

### 3.5 Seams assumed of the parallel designs

| # | Of | What this design takes | If it is not so |
|---|---|---|---|
| S-1 | LIB-8 59, LIB-10 59 (M-53) | `paper text`, `paper marks` for the PC-100C (LIB-8 AC-39); the TI debugger steps (`pause:`, `pause-on-start:`, `step:`, `resume:`, `debug-panel:`, `trace:`) registered in the vocabulary of `LibraryParts` | the four examples with debugger directives (`historic/printer-quirks`, `debug/operror`, `screentest/test_58_glyphs`, `screentest/test_cdebug`) wait for the step that gives them, by name, as the card examples do |
| S-2 | C59-3 (this step) | `writeStandAloneCueCard` and `readStandAloneCueCard` are shared with the constant memory file | — |
| S-3 | C59-7 (M-55) | the magnetic card's `cue-card` reads with `readStandAloneCueCard`'s rules plus the magnetic card's keys | M-55 adds its own reader |

## 4. The shipped presets (AC-10, AC-13 to AC-15, AC-18)

**The upgrade.** By hand, file by file, by AC-10's table and `preset.md`
§9.1, §9.3, in one commit per folder (`examples/`, `calendar/`,
`debug/`, `historic/`, `screentest/`), each folder checked by `calcu
--product 59 check-preset` and by its reference runs (§6 T7, T8) before
the next. No converter is written (as C1600-4 D-4): 53 files, and the
rows that need judgement (`let-it-finish:`, `skip-wait:`, the comments)
are the ones a converter cannot make (`docs/research/preset-conversion-check.md`
§3). Rules settled here, beside AC-10's table:

- **Speed** (AC-10, row "the first `KEYSTROKES:`"): an example whose
  first `KEYSTROKES:` does not start with `FullSpeed:` starts its first
  `keys:` with `- regular-speed:`; its later `FullSpeed:`/`RegularSpeed:`
  lines become the steps of the same names.
- **The last key** (AC-10, `preset.md` §9.1): `- let-it-finish:` after
  the last step where the reference run's result (display, paper,
  registers) comes from what that key starts; it is decided per file
  from the reference run (D-12). `historic/life.ti59` gets none: it never
  stops (AC-11).
- **`skip-wait:`**: before a step that has to reach a running program
  (an `R/S` that stops one), as LIB-6 AC-58 says; the conversion check
  names no TI example that needs it (§3 there), and the reference runs
  show any.
- **`Wait:`** becomes `- wait:` with the same duration; it now counts
  emulated time (LIB-6 AC-20), as the reference runs record emulated
  results only (AC-11).
- **The module and the printer** (AC-10, behaviour change 59-4): none
  named → `slot-1: { name: ML }` and `printer: PC-100C`; `PRINTER: off` →
  no `printer:`.
- **`debug/example-brief.ti59`** line 16: `002  11. A` → `002 11 A`
  (AC-10).
- **`historic/trace-quirk.ti59`** keeps its extension and its `MODEL:`
  as `model:` (EC-6; LIB-6 AC-5).
- **Comments** (AC-10): a comment that names a 2.3.0 key (`KEYSTROKES`,
  matrix codes, `FullSpeed:`), a tool (`ti59file.py`, `tools/headless`)
  or a format (`state file`) is reworded to the version 2 name or the
  harness command; the rest stays word for word.

**The ten examples and their names** (AC-13, AC-15, OQ-3): `description`
is today's title (`frontends/web/presets.json:1-12`); `preset-name` is a
short name of at most 18 characters, chosen here (D-14):

| Group | File | `preset-name` | `description` |
|---|---|---|---|
| Diagnostics | `diag.ti59` | TI-59 diagnostic | TI-59 calculator diagnostic |
| | `diag.ti58` | TI-58 diagnostic | TI-58 calculator diagnostic |
| | `ram_test.ti59` | RAM test | RAM test |
| Programs | `sum.ti59` | Sum of N numbers | Sum of N numbers |
| | `characters.ti59` | Character table | Print character table |
| | `fastmode-sst.ti59` | Fast Mode by SST | The shortest way into Fast Mode |
| Printer pictures | `printer-interrupt.ti59` | Printer interrupt | The printer interrupt, smallest form |
| | `picard.ti59` | Picard | Picard (printer-interrupt picture) |
| | `globe-print.ti59` | Globe | Globe (printer-interrupt picture); from M-55 (§1) |
| | `switzerland.ti59` | Switzerland | Switzerland (printer-interrupt outline map) |

`fastmode-sst` is the last of "Programs" (row 59-26). Every other
upgraded example also gets a `preset-name` and `description` where its
comment gives them; AC-15's check covers only the listed presets.

**The startup presets** (AC-13, ADR 0021 as amended): `examples/startup/default-ti59.ti59`
(and `-ti58`, `-ti58c`): `format-version: 2`, `model:`, `preset-name:
TI-59` (`TI-58`, `TI-58C`), `description: The TI-59 with the Master
Library module and the PC-100C`, `slot-1: { name: ML }`, `printer:
PC-100C`, nothing else. None restores the constant memory (C59-3 AC-8).

**The bundle file** `examples/bundle.yaml` (AC-13; `docs/formats/bundle.md`):
`startup:` the three startup presets; `groups:` the three groups of the
table in that order, each preset with the files it names (none of the
ten names a file). `globe-print` is left out until M-55 (§1; plan
question 6: "they stay out of the bundle file until their step"); between
the two steps the Web frontend offers nine examples. Not a release (plan
§4); not a ledger row.

**Bundling** (AC-14): the Qt6 frontend (`frontends/qt6/CMakeLists.txt`,
in the way Calc-U-1600's `frontends/qt6/CMakeLists.txt:219-252` reads
`{ file: … }` lines), the Swift app (a build phase that copies the same
files into the app's `examples/`, checked by
`frontends/swift/check_xcode_project.sh`) and the Web build
(`frontends/web/build.js:66-71` reads `bundle.yaml` instead of
`frontends/web/presets.json`, which goes) copy exactly the files
`bundle.yaml` lists, not their folders. The library's `checkBundle` with
`presetNameProblems` is AC-15's check (LIB-6 §3.7, as C1600-4 AC-18).

**The calendar scripts** (AC-18): `fastcal.py` writes `calendar-09` as
its version 2 file: the header (`calendar-09-header.txt`) becomes the
version 2 header and cue card, the program a `program:` section in the
listing notation, the keys a `keys:` section of `- key:` steps;
`years.py` finds the year in the version 2 file as today: the four
`- key:` digit steps before the first step that names `A` (matrix code
11, key ID `A`), and writes a copy with other digits for each year
(LIB-6 §11.7 for how the scripts run).

## 5. Adapters

This row removes, with LIB-6 59 and LIB-18 59 (their §11 list the rest):
`TIPresetOperation` (LIB-3 §5; LIB-6 §5), `TIKeystrokePlayback` (LIB-5 §5)
and the `TIMemoryAdapter` calls that wrote a preset (`writeProgram`,
`writeProgramStep`, `writeDataRegister`, `writeRAMReg`,
`setPartitionProgramRegs`, `runInstructions`). `TIMemoryAdapter` stays
for its readers: the debug log and snapshots until M-56 (C59-4), the
card files until M-55 (C59-7), the headless library until M-59 (C59-9).
It adds none.

## 6. Tasks

Tests come before the code. `CalcU59Tests` (Frequent) unless the row
says otherwise; ROM-gated cases `SKIP_UNLESS` the ROMs. The cases run the
product's harness entry in virtual time (`calcu --product 59` in-process,
`tests/harness_tests.cpp`'s helper), the app session with
`tests/SessionRig.hpp`'s `PartRig`, or `presets::readPreset` with
`TIPresetRules` on a text. Names `c59_2_acN_…`.

| Task | What | ACs/ECs | Test (file: cases) |
|---|---|---|---|
| T1 | The kind | AC-17 | `preset_content_tests.cpp`: `c59_2_ac17_listed_on_every_model`, `c59_2_ac17_step_register_extra_partition_read_back` (each model; H00–H03 on the TI-58C only) |
| T2 | `partition:` | AC-2 | `c59_2_ac2_rounds_up_to_80_at_most_960`, `c59_2_ac2_point_part_not_read`, `c59_2_ac2_none_keeps_rom_partition` (479, 239, 239), `c59_2_ac2_not_a_number_or_above_last_step_refused` (fixtures `bad-partition-*.ti5x`) |
| T3 | `program:` | AC-1, AC-3, EC-1 | `c59_2_ac3_compact_and_listing_notations`, `c59_2_ac3_mnemonic_not_read_even_a_number`, `c59_2_ac3_one_digit_or_word_refused_with_line`, `c59_2_ac3_step_beyond_last_refused`, `c59_2_ac3_keystroke_program_file_read_by_same_rules`, `c59_2_ac3_file_breaking_its_format_refused`, `c59_2_ac1_only_listed_steps_written`, `c59_2_ec1_steps_beyond_partition_written` |
| T4 | `data:` | AC-4, EC-2, EC-3 | `c59_2_ac4_whole_number_digit_for_digit_cut_after_13`, `c59_2_ac4_other_values_rounded_to_13`, `c59_2_ac4_no_number_refused`, `c59_2_ac4_60_to_99_on_ti58_refused_h_off_ti58c_refused`, `c59_2_ac4_h00_to_h03_are_not_60_to_63`, `c59_2_ec2_data_before_partition_in_file_order`, `c59_2_ec3_h_registers_written_after_cold_boot_else_as_startup_left` |
| T5 | `cue-card:` | AC-5, AC-6, EC-4, EC-5 | `c59_2_ac5_keys_defaults_tokens_prime_any_case`, `c59_2_ac5_template_banks_id_refused`, `c59_2_ac6_topic_says_fitted_card`, `c59_2_ac6_no_section_no_card`, `c59_2_ec4_second_replaces_first`, `c59_2_ec5_model_printer_card_unknown_keys`; Web `play-calculator.spec.js:282` renamed `c59_2_ac6 a preset's cue card title shows` (Longer) |
| T6 | Sections in file order; today's tests | AC-1, AC-7, AC-16 | `c59_2_ac1_first_stage_as_today` (a 2.3.0 example's first stage against its reference RAM), `c59_2_ac7_sections_after_keys_are_reloads`; AC-16: the cases of `tests/shared_state_applier_tests.cpp` (`apply_state_ti58_default_partition_and_limits`, `reload_touches_only_given_sections`, `data_registers_match_the_rom`, `reload_changes_only_the_listed_steps_and_registers`) and of `tests/shared_preset_stages_tests.cpp:44-69` ported onto version 2 texts as `c59_2_ac16_<old name>`; `bcd_regline_math_match_swift` (`REG`, `BCDD`, `BCDT`, `MATH`) unchanged; the old files go |
| T7 | The product's part of a load | AC-8 | LIB-6 §11.6 P2, P5 (`lib6_ac36_…`, `lib6_ac40_screenshot_…`): the idle rule, no after-build step, the picture |
| T8 | The commands | AC-9 | `harness_tests.cpp`: `c59_2_ac9_partition_program_registers_write_and_read`, `c59_2_ac9_refused_where_the_section_refuses`, `c59_2_ac9_count_reads_keycodes` (Q-1) |
| T9 | The upgrade | AC-10, AC-15, EC-6 | `calcu_preset_check.CalcU59Presets` (LIB-6 §11.6 P8, Frequent) over the 53 examples and the three startup presets, `EXCLUDE` the four of §1; `c59_2_ac10_no_2_3_0_preset_left` (a scan of the tracked `.ti59`/`.ti58`/`.ti58c` files for one without `format-version: 2`, the four excepted until M-55); `c59_2_ac15_names_short_described_once_unique_per_model` (`checkBundle`); `c59_2_ec6_trace_quirk_keeps_extension_and_model` |
| T10 | References | AC-11 | `CalcU59ReferenceRuns` (Longer; `tests/reference_runs_tests.cpp`), as §8 says: per example, `preset <file>` then `wait` through the harness entry in virtual time; then the display, the partition, the program steps and registers (the raw RAM through `readRaw`), `paper text`, `paper marks`, no card written; compared with `tests/reference/v2.3.0-examples/` (`.txt`'s `display:`, `partition:`, `--- ram`; `.paper-text`, `.paper-marks`); tick counts, `Elapsed:`, the log and the `playback:` line not compared; `historic/life.ti59` loaded without a failure only; the four of §1 listed as waiting (M-55); the intended differences of the conversion check §4 named per example with their reason |
| T11 | Bundle and frontends | AC-13, AC-14 | `c59_2_ac13_bundle_lists_three_startup_and_todays_examples_in_groups` (`readBundleFile`; `globe-print` from M-55), `c59_2_ac14_bundled_files_are_the_bundle_files` (CTest `CalcU59BundledFiles`, `cmake -P` over the Qt build tree, the Web `dist/presets` and the app's `examples/` of `xcodebuild`; Longer) |
| T12 | Calendar scripts | AC-18 | `CalcU59CalendarScripts` (Frequent, with Python 3): `fastcal.py` writes `calendar-09` equal to the tracked file; `years.py examples/calendar/calendar-09-dispatch.ti59 2026` finds the year and its rows check (it runs `calcu serve`; LIB-6 §11.7) |

Coverage: T1–T12 cover AC-1 to AC-18 (AC-12 in M-65) and EC-1 to EC-6.
AC-10, AC-11 and AC-13 for the four card examples pass in M-55 (C59-7's
design).

## 7. Tools and agent knowledge

**Tools.** `tools/agentic/` and the calendar scripts: LIB-6 §11.7.
`tools/goldens/` loses its `STATE` records.

**Agent knowledge in M-54** (with LIB-6 §11.7, LIB-18 §11.7, C59-3 §7):

| File | Change |
|---|---|
| skill `calc-u-59-statefiles` (`products/calc-u-59/.claude/skills/calc-u-59-statefiles/`) | rewritten for preset format 2: the TI sections of this design, key IDs instead of matrix codes (`preset.md` §8.1), the steps of LIB-6; it points to `docs/formats/preset.md` instead of restating it; renamed `calc-u-59-presets` (`docs/glossary-mapping.md`, row "Preset, preset format": `StateFile`, "state file" → preset), and every reference to the old name changes in the same commit (`products/calc-u-59/CLAUDE.md` skill table) |
| `reference/StateFileFormat.md` | deleted; `products/calc-u-59/CLAUDE.md`'s architecture list points to `docs/formats/preset.md` and `docs/formats/cue-card.md` |
| `prompt/ti59-agent-prompt.md` | presets of version 2; key IDs; the harness commands (`preset`, `wait`, `ti59 program`) instead of `ti59file.py` |
| `docs/glossary-mapping.md` | row "Preset, preset format", Calc-U-59 column: done (M-54): `calcu::presets` with `cu59::TIPresetRules` and `app/TIPresetKeys`; the preset list from `examples/bundle.yaml`; was `StateFile`, `StateApplier`, `TIKeystrokePlayback`, `presets.json`. Rows "Full speed", "Regular speed": `FullSpeed:`, `RegularSpeed:` → the steps `full-speed:`, `regular-speed:` (plan M-54) |
| memory `calc-u-59/cuecard_template_semantics.md` | a preset's cue card has no `Template`: it is the plain stand-alone card (`cue-card.md` §1); the magnetic card's look is M-55's |
| memory `calc-u-59/rom_comment_policy.md` | "the example/state file" → "the example preset" |
| memory `calc-u-59/pdcm_example_branch.md` | `frontends/web/presets.json` → `examples/bundle.yaml` |
| memory `calc-u-59/calendar_speed_project.md` | `calendar-09` is written by `fastcal.py` in version 2 |
| `calc-u-59-memories.md` (the index) | the descriptions of the entries above |

**Ledger rows that become visible in M-54:** 59-1 (presets of 2.3.0
refused with the hint), 59-3, 59-4, 59-5, 59-26; 59-42 is LIB-6's. No
new row (D-14's names are AC-13's "short names chosen when the files
are upgraded").

## 8. Golden and reference runs, risks

- **Golden runs** (`tests/golden_runs_tests.cpp`, `tests/golden-runs/`)
  drive the core directly; this row adds a capability to `TIMachine` and
  changes no path they use. A difference is a finding.
- **Reference runs (AC-11).** The M-9 records come from 2.3.0's
  session; the new runs go through the harness at the idle machine. The
  write path changes: 2.3.0 ran the ROM's start-up for 300,000
  instructions, cleared the RAM (keeping H00–H03) and then wrote
  (`app/StateApplier.cpp:57-75`); now the machine is built from a cold
  boot, the start-up runs to key readiness (LIB-18), and the sections
  write only what they list into what the start-up left (AC-1, C59-1
  AC-6). AC-1 says the first stage gives what today's gives; T6 checks
  it, T10 for every example. A difference is a finding, named per
  example with its reason, never a new reference (CLAUDE.md; the user's
  ruling: `printer-timing.ti59` is never re-recorded).
- **`debug/example-brief.ti59`.** AC-10 says its reference is recorded
  from a 2.3.0 file with the correction of line 16. The reference README
  does not say whether the M-9 record was; T10's first run shows it at
  step 002 of its `--- ram` section. If it was not, that one difference
  is named in the comparison's table as AC-10's correction (not a new
  record).
- **The 13-digit rule** of `data:` is `encodeBcd`'s today; the `BCDD`
  and `BCDT` records guard it.
- **Hand upgrade of 53 files.** Each folder is checked by `check-preset`
  and by its reference runs in the same step.
- **`CalcU59ReferenceRuns` in Longer.** It was 9 s on the app session
  (`docs/test-groups.md`); the harness entry in virtual time is not
  slower. It runs in the step (memory longer-tests-allowed) and at the
  end of Part E.

## 9. Decided (best guess)

- **D-1 (where the keywords live).** `app/TIPresetKeys.*`, one function
  the product's rules call (as Calc-U-1600's `app/SharpPresetKeys`).
- **D-2 (the kind's surface).** One product kind for program steps, data
  registers, H00–H03, the partition and the raw registers, so that the
  sections, the `ti59` commands, the reference comparison and C59-3 use
  one interface; the RAM layout stays the core's.
- **D-3 (no start-up run of our own).** A section writes into the
  machine as the build and LIB-18's `untilKeyReady` left it; 2.3.0's
  300,000-instruction start-up and RAM clear go, since a preset's
  machine is a cold boot (ADR 0016, C59-1 AC-6). The reference runs are
  the check.
- **D-4 (`partition:` and the ROM).** The section sets the partition
  where the ROM keeps it, as `StateApplier` did
  (`app/StateApplier.cpp:43-44`), not by keying `n Op 17`.
- **D-5 (one keycode reader).** `program:`, `.keystrokes.yaml`, `ti59
  program` and LIB-13's keystroke program file kind (M-56) read keycodes
  by `readKeycodes`.
- **D-6 (`data:` keys).** `NN` two digits; `HNN` in any case; a key
  given twice in one section is refused (README §1: a key once per
  mapping).
- **D-7 (cue card reader).** One reader for a stand-alone cue card,
  shared with C59-3's file and, with its own keys added, M-55's magnetic
  card. 2.3.0's `CUECARD:` text reader goes with the presets that used it;
  the Swift parity check `state_and_card_files_match_swift` keeps only
  its card half until M-55.
- **D-8 (sections finish when written).** `partition:`, `program:`,
  `data:` are `After::Own` and end when written (LIB-6 AC-15);
  `cue-card:` has taken effect when the topic says the card.
- **D-9 (commands write at once).** `c59.partition`, `c59.program`,
  `c59.registers` change memory between host calls; no operation, no
  busy; refused while one runs.
- **D-10 (no converter).** As C1600-4 D-4.
- **D-11 (speed rule of the upgrade).** AC-10's table: the first `keys:`
  starts with `- regular-speed:` unless 2.3.0's started with `FullSpeed:`.
- **D-12 (`let-it-finish:` per file).** Decided from each file's
  reference run: added where the result compared (display, paper,
  memory) comes from the last key's run; AC-11's harness `wait` reaches
  the same idle machine either way, so this choice changes what a user
  sees after a load in the GUI, not the comparison.
- **D-13 (the four card examples).** They wait for M-55 in the 2.3.0
  format, out of the suite check, the bundle file and the comparison,
  each by name (AC-10, plan question 6).
- **D-14 (short names).** The `preset-name`s of §4's table; each at most
  18 characters, none twice per model.
- **D-15 (startup presets).** In `examples/startup/`, named
  `default-<model>`, as Calc-U-1600's (`products/calc-u-1600/examples/bundle.yaml`).
- **D-16 (the comparison reads RAM raw).** AC-11's "program steps and
  registers" are compared as the raw RAM registers the M-9 records hold
  (`--- ram`, 120 registers), read through `ProgramMemory::readRaw`;
  this compares steps, data and H00–H03 at once and needs no reading of
  the partition's boundary.
- **D-17 (the old comparison table).** `kAccepted`
  (`tests/reference_runs_tests.cpp:127-155`) goes: AC-11 leaves out the
  log, ticks and `Elapsed:`, so its `Elapsed:` and `playback:` rows have
  nothing left to accept; `life.ti59` is excepted by AC-11 itself. The
  screentest freezes (`playback:` rows) are LIB-10 59's (M-53); AC-11's
  harness `wait` ends at a paused machine, so those two examples are
  compared paused.
- **D-18 (the skill's name).** `calc-u-59-statefiles` becomes
  `calc-u-59-presets`: "state file" is no term of the glossary
  (`docs/glossary-mapping.md`, row "Preset, preset format"), and a rename
  changes the skills that use the old name in the same step (root
  `CLAUDE.md`). Its description says presets of format 2 (`.ti59`,
  `.ti58`, `.ti58c`), keystroke program files and the TI key IDs (LIB-6
  Plan note P-3).

## 10. Questions for the user

1. **`ti59 program` reads or writes (AC-9).** AC-9 gives `ti59 program
   [<step>] <keycode>…` to write and `ti59 program <step> [<count>]` to
   read. A keycode is two digits, so `ti59 program 010 05` can be
   either. *Recommendation:* the count is written `count=<n>` (as
   `limit=` in C59-9 AC-3), and every bare token after the step is a
   keycode; this design is written so (§3.4, T8). A decision-log row in
   C59-2 records it if agreed.
   Answered (user, 2026-11-16, with the approval of the Part E designs): `count=<n>`;
   the decision-log row is in the spec.

## 11. Proposed ledger rows and plan notes

**Ledger rows:** none new (§7).

**Plan note P-1.** M-54's "Does" names "the 57 examples upgraded (the
four card examples in M-55)": add that `globe-print` is out of
`examples/bundle.yaml` until M-55, so the Web frontend offers nine
examples between the two steps.
