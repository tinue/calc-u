# C59-3 TI-58C constant memory: design
Status: Draft for the Part D/E boundary (2026-11-16)

## 1. Scope

| Row | Step | Tests from (plan §3) | Done in (plan §5.1) |
|---|---|---|---|
| C59-3 | M-54 (with LIB-6 59, C59-2, LIB-18 59) | AC-1–14, EC-1–8 | M-54 |

C59-3 rests on the start and the presets of this step (plan §2.1): a
TI-58C is built by LIB-18 (`features/LIB-18-machine-control-and-startup/design.md`
§11), from a preset by LIB-6 (`features/LIB-6-presets/design.md` §11),
and its cue card is fitted by C59-2's `cue-card:` section
(`features/C59-2-ti-preset-content/design.md`). This design names the
constant memory: the product kind, the file, when it is read and when it
is written.

- **Waits for** (plan M-54): M-35 (LIB-18 L: `MachinePlan::content`,
  `BuildOrigin`, `MachineControl::lastBuild()`), M-41 (LIB-7 L:
  `SessionPart::beforeMachineChange`, `closing`; LIB-18 §3.4 "added by
  later C2 steps"), M-34 (LIB-15 59: the Web frontend's save folder on the
  persistent file system, LIB-15 AC-20).
- What the TI-58C keeps through a Reset, a power cycle and a Cold boot
  inside the machine is C59-1 AC-6, built in M-19: `TIMachine` takes the
  constant memory as build content and puts it into the RAM before the
  processor's reset (`core/TIMachine.cpp:14-21`; `core/TICatalogue.hpp:45-55`,
  `struct TIConstantMemory`; `core/TICatalogue.cpp:179-180`), keeps it
  through `powerCycle()` and zeroes it at `coldBoot()`
  (`core/TIMachine.cpp:150-164`). Nothing of the core changes here but
  the kind of AC-1.
- Paths below without a prefix are under `products/calc-u-59/`; line
  numbers at `7bdd51c`. The spec's `Shared/`, `Web/`, `App/`, `Qt6/` paths
  are at `v2.3.0`.

**Today** (the code this design replaces): `TIAppPart::startMachine`
builds the machine without content and, on a TI-58C, runs the ROM's
start-up for 300,000 instructions and then writes the saved RAM over it
(`app/TIAppPartFlows.cpp:66-72`, `app/StateApplier.cpp:87-91`), the order
OQ-2 doubted (row 59-15). The file is `ti58c.mem` in 2.3.0's text layout
(`app/ConstantMemory.hpp:9-28`, `app/TIFileAccess.hpp:35`), written
whenever the running TI-58C advanced and the text changed, half a second
later (`app/TIAppPartFlows.cpp:232-241, 270-271`; `app/EmulatorSession.cpp:150`
marks the RAM dirty after every advance), on a model switch, a clean
reset, a preset load, a reload, suspend and the part's end
(`app/TIAppPartFlows.cpp:32, 139, 647, 709`; `app/TIAppPart.cpp:82, 280`),
and silently when it fails or cannot be read
(`app/TIAppPartFlows.cpp:243-252`).

## 2. Modules and files

### New

| File | Holds |
|---|---|
| `app/ConstantMemoryKind.hpp` | the product kind `c59.constant-memory` (AC-1; §3.1) |
| `core/TIConstantMemoryCapability.hpp/.cpp` | the kind on a TI-58C, capability `RAM/c59.constant-memory`; its change counter (§3.1) |
| `app/ConstantMemoryFile.hpp/.cpp` | `readConstantMemoryFile(std::string_view) -> std::variant<ConstantMemoryContents, FileProblem>` and `writeConstantMemoryFile(const ConstantMemoryContents&) -> std::string`: constant memory format 1 (`docs/formats/media.md` §4.1; AC-4, EC-2, EC-3, EC-5) |
| `app/ConstantMemoryKeeper.hpp/.cpp` | `cu59::ConstantMemoryKeeper`: the file's side of `TIAppPart` (§3.3): what it hands a build, whether the running machine was built from the file, the half-second write, the writes at once, the notices |
| `tests/constant_memory_tests.cpp` | AC-1–12, AC-14, EC-1–7 (§6) |

### Changed

| File | Change |
|---|---|
| `core/TIMachine.hpp/.cpp` | `capabilities()` lists `RAM/c59.constant-memory` on the TI-58C (`core/TIMachine.hpp:46-52`); the core counts the writes into RAM registers 0–63 that change a byte (§3.1) |
| `app/TIFileAccess.hpp/.cpp` | `kConstantMemoryFile` (`app/TIFileAccess.hpp:35`) is `ti58c.memory.yaml`, at the root of the save folder; `writeConstantMemory` returns whether it was written (EC-4); `ti58c.mem` is neither read nor written (EC-2) |
| `app/TIAppPart.hpp/.cpp`, `app/TIAppPartFlows.cpp` | `persistConstantMemory`, `loadConstantMemory` and the debounce in `frame` (`app/TIAppPartFlows.cpp:232-252, 270-271`) go to `ConstantMemoryKeeper`; the calls at a model switch, a clean reset, a load, a reload, suspend and destruction go (§3.3 says when it writes now); `c59.persist-constant-memory` (`app/TIAppPart.hpp:211`, `app/TIAppPart.cpp:135`) goes (no frontend sends it: AC-6's moments are the session's) |
| `app/TIProductMachines.cpp` (LIB-18 §11.2) | `builtInDefault(TI-58C)` puts `ConstantMemoryKeeper::contentForBuild()` into `MachinePlan::content` (AC-7, EC-1, EC-2, EC-3, EC-6) |
| `frontends/swift/Bridge/TIAppSession.mm:221` | the name it leaves out of `cardWritten` is `ti58c.memory.yaml` |
| `frontends/swift/App/CardStorage.swift:3-5`, `frontends/swift/App/MachineModel.swift:24` | comments: the constant memory file's new name |
| `frontends/qt6/app/AppPaths.cpp:86`, `frontends/qt6/app/SettingsDialog.cpp:75` | `constantMemoryFile()` goes (the part names the file); the folder row's text names `ti58c.memory.yaml` |
| `frontends/web/tests/storage.spec.js:4-12, 50` | the file is `ti58c.memory.yaml` (LIB-15 AC-20's constant memory clause) |
| `tests/shared_formats_tests.cpp:374-392` | `constant_memory_round_trip` ported to format 1 as `c59_3_ac14_round_trip_zero_registers_left_out` |
| `tests/part_flows_tests.cpp:93` | `part_constant_memory_keeps_the_loaded_card` goes: a preset no longer writes the file (AC-10); its other half is `c59_3_ac7_cue_card_of_the_file_fitted_again` |

### Deleted

`app/ConstantMemory.hpp/.cpp` (2.3.0's layout; `ConstantMemoryContents`
moves to `app/ConstantMemoryFile.hpp`), `StateApplier::powerOn` (with
`app/StateApplier.*`, C59-2).

## 3. Seams and interfaces

### 3.1 The constant memory kind (AC-1)

```cpp
namespace cu59 {
// The product kind "constant memory" (C59-3 AC-1; capabilities.md C-3).
// Capability `RAM/c59.constant-memory` on the TI-58C only.
class ConstantMemory : public calcu::machine::Capability {
public:
    static constexpr calcu::machine::KindId kKind{"c59.constant-memory"};
    // The 64 raw registers R000–R063 (H00–H03 are R060–R063), 16 nibbles each
    // (rom/TI58C-ROM-Guide.md:37; docs/formats/media.md §4.1).
    virtual std::array<Nibbles16, 64> content() const = 0;
    // Changes whenever a write changed a nibble of R000–R063.
    virtual std::uint64_t changeCount() const = 0;
};
}
```

- The counter is the core's: the RAM write path of `TI59Machine` raises
  it when a write into registers 0–63 changes a nibble. It does not
  change when a write stores what was there, so a running program that
  only reads, or rewrites the same values, leaves the file alone (AC-5,
  "not written when nothing changed"). A build, a `powerCycle()` and a
  `coldBoot()` that change the RAM raise it.
- `content()` is read on the session's thread between advances (LIB-1
  §2a). The capability is listed on the TI-58C only; a TI-59 or TI-58
  lists none (AC-1).
- The kind holds 64 registers, not today's 120 (`app/ConstantMemory.hpp:17`,
  `kConstantMemoryBytes`): the TI-58C's CMOS memory is 64 raw registers
  (C59-3 decision log, 2026-11-13).

### 3.2 The file (AC-3, AC-4, EC-2, EC-3, EC-5)

`ti58c.memory.yaml` at the root of the save folder (`TIFileAccess`,
which takes the save folder from the host: LIB-9 `Paths`; Swift's
iCloud folder through `TIAppSetup::saveFolder` and `fileIO`; the Web's
persistent file system, LIB-15 AC-20). It is not a medium: no media place,
no picker, no `saveas` (`settings.md` ST-3; `media.md` Q-2).

Read with the library's YAML reader (`formats::parseYaml`, the default
options):

| Case | Result |
|---|---|
| no file | `NoFile` (EC-1) |
| `format` not `ti58c-constant-memory`, or `format-version` missing or below 1 | `Outdated` (EC-2): the file is not read; a notice "The constant memory file ti58c.memory.yaml is outdated; it is replaced at the first change. Its format is described in the User Guide, chapter Formats." (D-6) |
| `format-version` above 1 | `Outdated` with "written by a newer release" (README §2) |
| version 1, but a key unknown, `ram` without `encoding: register-hex`, a line that is not `Rnnn: <8 hex byte pairs>`, a register above `R063`, or a `cue-card` that `readStandAloneCueCard` refuses (C59-2 §3.3) | `Damaged` (EC-3): nothing of it used; a notice "The constant memory file ti58c.memory.yaml cannot be read (line <n>: <what>); the TI-58C starts with zeroed memory." |
| version 1 with no register line | a valid empty constant memory (EC-5) |

The `register-hex` lines are today's `Rnnn:` lines (`docs/formats/README.md`
§1.1; `preset.md` §9.3: "rows copied unchanged"), so `app/Bcd.hpp`'s
`encodeRegisterLine` and `parseRegisterLineHex` (`app/Bcd.hpp:36-45`) read
and write them. The writer leaves out registers that are zero, writes
`cue-card` only when an own card is fitted (`writeStandAloneCueCard`), and
no `memory-name` or `description`. `ti58c.mem` of 2.3.0 is ignored: not
read, not changed, no notice (EC-2).

### 3.3 When the memory comes back and when it is written (AC-5 to AC-12)

```cpp
namespace cu59 {
class ConstantMemoryKeeper {
public:
    ConstantMemoryKeeper(TIFileAccess&, std::function<void(std::string)> notice);
    // AC-7, EC-1–EC-3, EC-6: what a TI-58C built without a preset gets: the file's
    // registers, or zeroed memory (no file, outdated, damaged). Reads the file
    // (once per build); keeps the file's cue card for fitting (AC-7).
    std::shared_ptr<const TIConstantMemory> contentForBuild();
    std::optional<CueCardContent> fileCueCard() const;
    // A new machine (TIAppPart::machineChanged): whether it was built from the
    // file (LIB-18's lastBuild(): origin Start or ModelSwitch, the model TI-58C,
    // the content the keeper handed); anything else is not (AC-8, AC-10, AC-12, EC-7).
    void machineBuilt(calcu::machine::Machine&, const calcu::startup::BuiltWith&);
    // AC-5: in afterCall, with the session's wall time: a change seen
    // (changeCount or the fitted own card) is written at the latest 500 ms later.
    void afterCall(calcu::machine::Machine*, const std::optional<CueCardContent>& ownCard, calcu::pacing::WallTime now);
    // AC-6: at once, if the machine is from the file and something changed.
    void writeNow(calcu::machine::Machine*, const std::optional<CueCardContent>& ownCard);
};
}
```

| Moment | What happens | AC |
|---|---|---|
| a TI-58C built at the start or at a model switch, no startup preset set (`builtInDefault`) | `contentForBuild()` into `MachinePlan::content`: the RAM holds it before the processor starts (`core/TIMachine.cpp:14-21`); the ROM's start-up restores the partition and the display format from it; the file's cue card is fitted again at `machineChanged` | AC-2, AC-7; row 59-15 |
| the same with no file, an outdated or a damaged one | zeroed content, no card; the machine is from the file and writes it at the first change | EC-1, EC-2, EC-3 |
| a startup preset set but not loadable (LIB-18 EC-1) | LIB-18 builds `builtInDefault`, so the same as above | EC-6 |
| a TI-58C built from a preset or a startup preset (`BuildOrigin::Preset`, `StartupPreset`) | no content: zeroed before the processor starts; the card is the preset's or none; the machine is not from the file and never writes it (also when the preset fails while running) | AC-8, AC-10, EC-7 |
| a change while a machine from the file runs (`changeCount`, or the own card fitted, replaced or taken off) | written at the latest 500 ms of the session's wall time later (LIB-3 AC-2), not when the text equals what was written last | AC-5 |
| the machine from the file is about to be replaced (`SessionPart::beforeMachineChange`) | written at once if it changed | AC-6 |
| suspend (`SessionPart::suspending`; the Web's page hidden, LIB-15 AC-2) | written at once if it changed | AC-6 |
| the product quits (`SessionPart::closing`, `AppSession::close()`) | written at once if it changed | AC-6 |
| Cold boot on a machine from the file | the machine zeroes its RAM before the processor (C59-1 AC-6); the counter changes, so the file follows within 500 ms of key readiness, with the own card still fitted | AC-9, AC-10 |
| Reset or power cycle (a module change, the PC-100C attached or detached) | the memory stays in the machine; the file is not read; the own card stays fitted (LIB-18 AC-23) | AC-11 |
| a TI-59 or TI-58 runs | the keeper does nothing; the file keeps what the last TI-58C left | AC-12 |
| a write fails | the machine runs on; a notice "The constant memory could not be saved to ti58c.memory.yaml: <reason>." once per start of the product; the next change tries again | EC-4 |

- **Which machine is from the file.** `TIAppPart::machineChanged` reads
  `MachineControl::lastBuild()` (LIB-18 §3.4) and hands it to the keeper:
  from the file means origin `Start` or `ModelSwitch` and the model
  TI-58C (LIB-18 §3.5: the start and the model switch build
  `builtInDefault` without a preset, also after `CannotLoad`). `Preset`
  and `StartupPreset` are not. A Reset, a power cycle and a Cold boot
  make no new build, so the machine stays what it was.
- **The own card.** The cue card stored is the machine's own card
  (`TIAppPart::m_userCueCard`, set by `cue-card:` and kept through
  Reset, LIB-18 AC-23); a module program's cue card shows over it and is
  not stored (AC-4, `cue-card.md` §2). The keeper is told the own card
  after each call, so fitting a card on a machine from the file is a
  change (AC-5).
- **Not read again.** A running product reads the file only in
  `contentForBuild()`, i.e. at a build without a preset (TR).

### 3.4 Seams assumed of the parallel designs

| # | Of | What this design takes | If it is not so |
|---|---|---|---|
| S-1 | LIB-18 59 (this step, §11) | `TIProductMachines::builtInDefault(TI-58C)` asks the keeper; `lastBuild()` gives the origin | — |
| S-2 | C59-7 (M-55) | the TI-58C has no card reader, so no magnetic card is fitted there; M-55 changes nothing of this file | — |
| S-3 | LIB-14 59, C59-8 (M-57, M-58) | the notices of EC-2–EC-4 are LIB-3 notices the frontends show as today | — |

## 4. Every frontend (AC-13)

The file is the app session's, written through `TIFileAccess`; each
frontend only hands its save folder:

- **Qt6:** the save folder of LIB-9 (`frontends/qt6/app/EmulatorController.cpp:66`).
- **Swift:** the iCloud Drive folder or the local Documents
  (`frontends/swift/App/CardStorage.swift`), written through the bridge's
  coordinated `FileIO` (`frontends/swift/Bridge/TIAppSession.mm:210-222`),
  as today; the file last written wins between devices (TR).
- **Web:** the save folder on the persistent file system
  (`frontends/web/bindings.cpp:51-63`; LIB-15 AC-20); 2.3.0's
  `localStorage` key is not read and nothing is said (EC-8; LIB-9 AC-23).

## 5. Adapters

None added. Removed with this row: the reading and writing of
`ti58c.mem` in `TIAppPart` (`persistConstantMemory`,
`loadConstantMemory`, `c59.persist-constant-memory`) and `powerOn`
(`app/StateApplier.cpp:87-91`), which wrote the memory after the ROM's
start-up instead of before it.

## 6. Tasks

Tests come before the code. `CalcU59Tests` (Frequent) unless the row
says otherwise; ROM-gated cases `SKIP_UNLESS` the ROMs. The cases run the
app session with `tests/SessionRig.hpp`'s `PartRig` and a temporary save
folder (`library/testing/TempFolder`), paced on a `ScriptedClock` where
the half second matters (LIB-3 AC-2), else in virtual time; or the file
reader on a text. Names `c59_3_acN_…`.

| Task | What | ACs/ECs | Test (file: cases) |
|---|---|---|---|
| T1 | The kind | AC-1 | `constant_memory_tests.cpp`: `c59_3_ac1_listed_on_ti58c_only`, `c59_3_ac1_counter_changes_with_content_only` |
| T2 | Back as a switched-on TI-58C | AC-2 | `c59_3_ac2_partition_and_display_format_come_back` (a TI-58C with `Fix 2` and a partition other than 239, quit and started again: the same partition and display; the exit check of M-54), `c59_3_ac2_memory_in_place_before_the_processor` (`machine_tests.cpp:297`'s content case at the app's level) |
| T3 | The file | AC-3, AC-4, EC-2, EC-3, EC-5 | `c59_3_ac3_file_at_root_of_save_folder_not_a_medium`, `c59_3_ac4_format_1_r000_to_r063_h_as_r060_r063`, `c59_3_ac4_own_card_stored_module_card_not`, `c59_3_ac4_register_above_r063_refused`, `c59_3_ec2_outdated_or_no_version_not_read_notice`, `c59_3_ec2_ti58c_mem_ignored_without_notice`, `c59_3_ec3_damaged_line_nothing_used_notice`, `c59_3_ec5_no_register_lines_valid` |
| T4 | Writing | AC-5, AC-6, EC-4 | `c59_3_ac5_written_within_half_a_second_wall_time`, `c59_3_ac5_not_written_without_change`, `c59_3_ac6_written_at_once_before_replace_on_suspend_on_quit`, `c59_3_ec4_failed_write_notice_once_next_change_retries` (a `FileIO` that fails) |
| T5 | When it comes back | AC-7, AC-8, EC-1, EC-6, EC-7 | `c59_3_ac7_start_and_model_switch_get_the_file`, `c59_3_ac7_cue_card_of_the_file_fitted_again`, `c59_3_ac8_preset_or_startup_preset_zeroed_preset_card`, `c59_3_ec1_no_file_zeroed_written_at_first_change`, `c59_3_ec6_startup_preset_not_loadable_gets_the_file`, `c59_3_ec7_failed_preset_writes_nothing` |
| T6 | Reset, Cold boot, other models | AC-9, AC-10, AC-11, AC-12 | `c59_3_ac9_cold_boot_zeroes_then_startup`, `c59_3_ac10_preset_machine_never_writes`, `c59_3_ac10_cold_boot_of_file_machine_file_follows_card_kept`, `c59_3_ac11_reset_power_cycle_memory_and_card_stay_file_not_read`, `c59_3_ac12_ti59_ti58_neither_read_nor_write` |
| T7 | Frontends, today's tests | AC-13, AC-14, EC-8 | `c59_3_ac14_round_trip_zero_registers_left_out` (ports `tests/shared_formats_tests.cpp:374`), `c59_3_ac14_model_switch_away_and_back_keeps_memory`; Web `storage.spec.js` `c59_3_ac13 the constant memory is ti58c.memory.yaml in the save folder` and `c59_3_ac14 kept across a page load` (ports `play-calculator.spec.js:232`, the spec's `Web/tests/play-calculator.spec.js:181-196`; Longer), `c59_3_ec8 2.3.0's localStorage memory is not read` (Longer); Qt6 and Swift: `c59_3_ac13_the_host_folder_is_the_one_written` (the `TIAppSetup::saveFolder` and `fileIO` each host passes, as `lib3_ac30_card_and_constant_memory_files_without_frontend` does today, `tests/session_tests.cpp:126`) |

Coverage: T1–T7 cover AC-1 to AC-14 and EC-1 to EC-8.

## 7. Tools and agent knowledge

**Tools.** None reads the file. `tools/headless/` builds its TI-58C
without one, as today.

**Agent knowledge in M-54:**

| File | Change |
|---|---|
| `reference/SharedArchitecture.md`, `reference/CoreArchitecture.md` ("TI-58C Extra (Constant Memory) Registers", cited at `app/StateApplier.cpp:65-68`) | the file is `ti58c.memory.yaml`, constant memory format 1, read at a build before the processor starts; the kind `c59.constant-memory` |
| `reference/FrontendParity.md` | the row of the constant memory: the same file in all three frontends |
| memory `calc-u-59/state_vs_preset_terminology.md` | rewritten: "state file" is no term any more (`docs/glossary-mapping.md`, row "Preset, preset format"); a `.ti59` file is a preset; `ti58c.memory.yaml` is the constant memory file, never a preset |
| `calc-u-59-memories.md` (the index) | that entry's description |

**Ledger rows that become visible in M-54:** 59-1 (`ti58c.mem` ignored),
59-12, 59-15, 59-16, 59-17 (the constant memory file keeps the fitted
own card). No new row.

## 8. Golden runs and risks

- **Golden runs.** `tests/golden-runs/ti58c-constant-memory.run` builds
  through `TIMachine` with content (C59-1); this design adds a
  capability and a counter on the RAM write path. The golden replays
  compare display, RAM and ticks; a counter changes none of them. A
  difference is a finding.
- **The counter on the write path** costs one compare per RAM write;
  the TI-58C's pacing checkpoint (`CalcU59PacingRealClock`, Checkpoint)
  shows it at the release step.
- **Row 59-15** says "to be confirmed in phase 6 whether 2.3.0 showed
  otherwise". T2 shows today's build order is the real calculator's; the
  2.3.0 order (start-up, then the RAM) is not run again: the code that
  did it goes. The row stays as written.
- **Two devices on iCloud** write the same file; the last write wins,
  nothing is merged (TR), as today.

## 9. Decided (best guess)

- **D-1 (the counter).** In the core, on writes that change a nibble of
  R000–R063, so that a running program that does not change memory does
  not write the file every half second, as today's dirty mark after every
  advance does (`app/EmulatorSession.cpp:150`).
- **D-2 (one keeper).** The file's rules live in one class beside
  `TIAppPart`, not spread over its flows as today; it is told the build,
  the calls and the moments, and reads the machine only through the kind.
- **D-3 (from the file).** Decided by LIB-18's `lastBuild()` origin:
  `Start` and `ModelSwitch` build without a preset; `Preset` and
  `StartupPreset` with one. No flag of its own.
- **D-4 (the own card is a change).** Fitting, replacing or taking off
  the own card on a machine from the file writes the file (AC-5: "its
  constant memory or the fitted cue card changes").
- **D-5 (64 registers).** The kind and the file hold R000–R063; the
  TI-58C's content passed to the build is the same 64 registers in the
  core's 120-register image (`TIConstantMemory::ram`,
  `core/TICatalogue.hpp:53-55`), the rest zero.
- **D-6 (notice texts).** The sentences of §3.2 and §3.3; "where the
  format is described" (EC-2) is the help's chapter on formats, until
  M-60 gives its address (as LIB-6 D-30 for the preset format).
- **D-7 (the command `c59.persist-constant-memory` goes).** AC-6's
  moments are session moments (`beforeMachineChange`, `suspending`,
  `closing`); no frontend needs to ask.
- **D-8 (notice once per start).** EC-4's "once per start of the
  product" is once per app session: the keeper remembers that it has
  said it.

## 10. Questions for the user

None that splits this design.

## 11. Proposed ledger rows and plan notes

**Ledger rows:** none new. Row 59-15's "to be confirmed" is answered by
T2; the merge of M-54 may drop that sentence (the row's text is the
ledger owner's).

**Plan notes:** none.
