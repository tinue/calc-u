# Preset conversion check: version 1 presets against preset format 2

Status: research note, 2026-11-12.

This note checks every existing preset of both products against the format 2
specification in `docs/formats/`. The waiting rules are in `preset.md`
§5.3 and §6:

- the implicit wait between two items;
- `wait: <duration>`, a pause after it;
- `skip-wait:`, which drops it;
- `let-it-finish:`, allowed only as the last item.

It checks the specification only, not an implementation. Where version 1's wait
between two steps worked, the implicit wait of format 2 is taken to work too.

Baselines (read only):

- Calc-U-59 `v2.3.0`, parser and runner `Shared/StateFile.cpp`,
  `StateApplier.cpp`, `KeySequencer.cpp`, `SessionController.cpp`.
- Calc-U-1600 `dev-0.8.1` @ `b4ea8c8`, parser and runner
  `Core/Preset/PresetFile.cpp`, `PresetRunner.cpp`, and the BASIC typers.

File and line references are into the product repository at its pin.

## 1. Summary

| | Calc-U-59 | Calc-U-1600 | Total |
|---|---|---|---|
| Preset files found | 61 | 58 | 119 |
| Checked in detail | 30 (27 presets, 3 fixtures) | 22 | 52 |
| Skipped as covered (§6) | 31 | 36 | 67 |
| Convert without loss | 20 | 22 | 42 |
| Convert, but the cue card look is lost | 6 | 0 | 6 |
| Cannot be converted | 4 | 0 | 4 |
| Run the same, or differ only as intended | 20 | 21 | 41 |
| Fixable problem: the converted preset needs an edit (§3) | 9 | 1 | 10 |
| Real problem: needs a spec decision (§2) | 0 | 0 | 0 |
| To be deleted | 1 | 0 | 1 |

The 3 fixtures are malformed on purpose. Format 2 rejects them, which is
intended, so they count as "run the same, or differ only as intended". The
fourth preset that cannot be converted is `test-solidstate`, which will be
deleted (user, 2026-11-12).

## 2. Real problems

None. Every preset that stays can be converted, and runs the same or differs
only as intended, possibly after an edit (§3).

## 3. Fixable problems

The specification covers these presets. The converted file needs an edit that
a mechanical converter can't always make, and §9 of `preset.md` gives the rule
for each one.

| Edit | Presets | Why |
|---|---|---|
| `- let-it-finish:` as the last item | `fast-grafik-3d-plot` | Version 1 waited for the last key, running the plot to its end at full speed. Format 2 doesn't wait for the last item, so without `let-it-finish:` the plot runs on at regular speed. (`globus` and `lissajou-1500` get it from their bare `- wait:` by the §9.2 row) |
| `- regular-speed:` as the first step | `selftest-speed-check` (every converted C59 script, as a rule) | Version 1 defaulted to regular speed. The check compares a run at regular speed with one at full speed |
| `- skip-wait:` before `- wait: 2s` | `iterator` | BREAK has to reach the running program. Without `skip-wait:`, the implicit wait lets RUN finish its loop first |
| `- skip-wait:` before `- wait: 1.5s` | `test_cdebug`, `test_diag` | `SBR =` with `pause-on-start` leaves the machine paused. A paused machine is not idle, so the implicit wait would hold until the user resumes it |
| Set `timeout:` after measuring the run | `globe-print`, `calendar-09` (and `picard`, `switzerland`, `stars-and-stripes`) | A long print at full speed comes before `elapsed-time: report`, so the implicit wait has to cover it; version 1 had no limit |
| Blank cards as in-file cards with their own `cue-card:` (no data: the corners are empty until a side is written, then show its bank) | `pdcm-card-tricks` (`D-2` … `D2`) | Version 1 kept the card shown before; format 2 fits the blank card, so it gets its own cue card |
| `cue-card:` on the in-file card | `characters-fastmode` | Version 1 kept the preset's card shown; format 2 fits the in-file card, which has no cue card of its own |
| In-file cards with their cue cards, written, then `- saveas: { card: …, as: live, name: … }` | `pdcm-make-cards` (`PDCM-1`, `PDCM-2`, …) | Version 1 created the card files when written, in the card folder, with the latest `CUECARD:`. Format 2 creates a new card from the fresh template and saves it into the save folder by `saveas:` (`preset.md` §5.2.1, §9.1) |
| `slot-1: { name: ML }` | `firmware.ti59` | Version 1 used the user's module (normally ML); format 2 has none |

## 4. Intended differences

No action is needed for these.

| Difference | Presets |
|---|---|
| Full speed by default; a preset always ends at regular speed | All |
| The last item is not waited for: what it starts runs on at regular speed, under the user's control | `lcdallon`, `DiskWorks`, `Calculat`, `ce158_demo` ×2, `maxed-out-mem`, `hanoi`, `life`, `printer-interrupt-bits`, `erdkundetest`, `pdcm-card-tricks` |
| `Calculat` no longer runs at full speed for up to 1 h of emulated time before handing over control | `Calculat` |
| Commands are waited for to their end, not 1 s (PC-1600) or 12 s (PC-1500) | `memtest_stock`, `firmware_bootstrap_*`, `ce150-text-and-frame`, `flashtest_ce163f` |
| Absent hardware means none, and a cold boot follows; this clears the TI-58C constant memory | `nibbleerror`, `operror`, `printer-interrupt-sweep`, `repartition` |
| `model:` is required; the converter takes it from the suffix | `characters`, `nibbleerror`, `operror`, `printer-quirks` |
| Every card is fitted after it has passed (`mem:` cards included) | `globe-print`, `pdcm-card-tricks` |
| Magnetic card and Solid State cue card look (`Template`, `Banks`) dropped | calendar ×9, `globe-print`, `erdkundetest`, `life`, `test-magnetcard` |
| How long a key is held is the product's: long enough to register, not much longer (`preset.md` §5.1). `printer-timing` (Adv) relies on the product keeping today's hold | `printer-timing` |
| A bad line rejects the whole file | the 3 fixtures (they become rejection tests) |

## 5. Proposed spec changes (not applied)

The waiting rules and the key hold (§5.1) are already in `preset.md` and in
the GLOSSARY (uncommitted).
They include:

- `wait:` as a pause;
- `skip-wait:`;
- `let-it-finish:`;
- the three states: full speed, regular speed, paused;
- "idle" removed from the format specification;
- `saveas:` for TI-59 magnetic cards, and the fresh card as the magnetic
  card's implicit template;
- a reference to a missing card file is an error;
- a magnetic card's cue card is its own, not the preset's.

The proposals below are still open; their numbers are kept from the earlier
list.

**S3. §9.1, the converter's speed.** State that a converter writes
`- regular-speed:` as the first step of a version 1 script.

**S4. A fitted card without a cue card** (cue-card.md §2). No preset needs
this any more (`pdcm-card-tricks` defines its blank cards). It remains a
default to state: either the cue card shown before stays, or a plain card
titled with its card name (`media.md` §1.4 already says the latter for a
card saved without one).

**S6. Stopping a preset that waits without limit** (`let-it-finish:`, or an
implicit wait on a paused machine). The user can end a running
preset, through a frontend gesture or by loading another preset, without an
error.

**S9. §9.1, absent `MODEL:`.** The model comes from the suffix (`.ti59`,
`.ti58`, `.ti58c`).

**S10. §9.1, `PROGRAM:` and `REGISTERS:`.** Add the row "→ `program:` then
`data:`, whatever the order in the file". Version 1 always wrote the registers
after the program (`calendar-05`).

**S11. §9.3, cue card look.** State that a preset's own MagnetCard or
SolidStateCard becomes a plain stand-alone card, and that `ID` / `IdAlign`
cannot be converted. (`test-solidstate` is deleted.)

**S13. TI-58C constant memory.** State whether the constant memory file holds
the machine's state when a preset ends.

**S14. Preset names** (README §2). The 18-character limit and the uniqueness
rule apply to the displayed name. Uniqueness covers one product and one model.
This affects:

- `loader_test_1600_memory_cards`, which is 29 characters;
- four templates, all named `debug`.

**S15. `debug:` keys** (`stopOnEntry`, `cleanStart`, `allReset`). Either:

- rename them to kebab-case;
- or exempt them from README §1.

**S16. Small additions.**

- `trace: off` with no trace open has no effect.
- §9.2 rows:
  - `plotter: CE-150` → `printer: CE-150`;
  - `floppy-file: …,B` → `{ file: …, side: B }`;
  - a `\` in `expect:` text now starts an escape.

**S17. The user during a preset.** State:

- whether the user's key presses are refused or held;
- that loading a preset ends a running one.

**S18. Documents to align with the waiting rules.**

- LIB-6:
  - AC-16 (speed, the three states);
  - AC-19 (`wait:` while paused: wall-clock time, which matches);
  - AC-21 (`wait:` without a value becomes `let-it-finish:`, without the
    half second and the 1 h limit).
- `docs/contracts/presets.md` PR-4 and PR-5.
- The `wait` upgrade table in `features/C1600-4-sharp-preset-content/spec.md`.

## 6. Skipped presets

Each skipped preset uses the same keywords, in the same way, as the preset that
covers it.

| Skipped | Covered by |
|---|---|
| calendar-02, -03, -04, -07 | calendar-01 |
| calendar-06 | calendar-05 |
| keycodes, sum, Integer-Base-Conversion, ret_bug_label, ret_bug_repo_2levels, ram_test, ram_test_full_fast, FastMode_Zero4, fastmode.ti59, Frequency-Transformation, diag.ti58 | characters |
| fastmode.ti58 | nibbleerror |
| test-magnetcard | test-cuecard |
| example-brief | example |
| printer-interrupt, fastmode-sst, work/calendar/fastcal, work/calendar/newcal, diag.ti59 | printer-interrupt-bits |
| picard, switzerland, stars-and-stripes | globe-print / calendar-09 |
| erdkundetest-2 | erdkundetest |
| texas-print, test_texas | printer-quirks |
| test_58c_display | repartition |
| test_diag | test_cdebug |
| Core fixture `hanoi.pc1600`, `pc1600-hanoi-list`, `pc1600-hanoi-run` | hanoi.pc1600 |
| Core fixture `maxed-out-mem.pc1600`, `supercard_debug` | maxed-out-mem.pc1600 |
| Core fixture `memtest_stock.pc1500`, `adrtest`, `instrquirks`, `memtest_1500a`, `memtest_CE1638`, `memtest_ce155`, `memtest_bank`, `pc1500-maxed-out.pc1500` | memtest_stock.pc1500 |
| `startup-pc1500a` fixture, `default-pc1500`, `pc1500-ready`, `pc1500a-ready`, `pc1600-ready`, `debug-pc1500a`, `debug-pc1600`, `pc1600-tape` | default-pc1500a |
| `startup-pc1600` fixture | default-pc1600 |
| `pc1500a-ce158`, `pc1600-lissajou`, `lissajou-1600`, `lissajou-ce150`, `ce150_demo.pc1500a` | lissajou-1500 |
| `ce1600p-text-and-frame` | ce150-text-and-frame |
| `ascii`, `loader_test_1600_memory_cards`, `biorhythmus_1600` | globus |
| `ce1638_bankswrm.pc1500`, `ce1638_bankswrm.pc1500a` | trace_demo |
| `firmware_bootstrap_utilrm_15`, `firmware_bootstrap_utilrm_20` | firmware_bootstrap_util_15 |
| `pc1600-modules` | make_diskworks_media |
| `pc1600-program` template | pc1500a-program template |
