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
| Preset files found | 62 | 59 | 121 |
| Checked in detail | 30 (27 presets, 3 fixtures) | 22 | 52 |
| Skipped as covered (§6) | 31 | 36 | 67 |
| Convert without loss | 20 | 22 | 42 |
| Convert, but the cue card look is lost | 6 | 0 | 6 |
| Cannot be converted | 4 | 0 | 4 |
| Run the same, or differ only as intended | 20 | 21 | 41 |
| Fixable problem: the converted preset needs an edit (§3) | 9 | 1 | 10 |
| Real problem: needs a spec decision (§2) | 0 | 0 | 0 |
| To be deleted | 1 | 0 | 1 |

Corrected on 2026-11-13: the pins hold 62 and 59 preset files. The rows
below "found" were counted before that and leave out three files, which
§3.1 now covers: `historic/trace-quirk.ti59`,
`screentest/test_58_glyphs.ti58` and `interfaces/setcom1600.pc1600`;
§6 names 32 skipped Calc-U-59 files, not 31.

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
| `- let-it-finish:` as the last item | `fast-grafik-3d-plot`, `switzerland`, `stars-and-stripes`, `printer-quirks`, `texas-print`, `screentest/test_texas` | Version 1 waited for the last key, running the plot or the print to its end at full speed (each ends with `FullSpeed:` and the key that starts it). Format 2 doesn't wait for the last item, so without `let-it-finish:` the plot runs on at regular speed. (`globus` and `lissajou-1500` get it from their bare `- wait:` by the §9.2 row) |
| `- regular-speed:` as the first step | `selftest-speed-check` (every converted C59 script, as a rule) | Version 1 defaulted to regular speed. The check compares a run at regular speed with one at full speed |
| `- skip-wait:` before `- wait: 2s` | `iterator` | BREAK has to reach the running program. Without `skip-wait:`, the implicit wait lets RUN finish its loop first |
| `- skip-wait:` before `- wait: 1.5s` | `test_cdebug` | `SBR =` with `pause-on-start` leaves the machine paused. A paused machine is not idle, so the implicit wait would hold until the user resumes it |
| `- skip-wait:` before `- wait: 3s` | `pc1600-hanoi-run` (developer screenshots) | Version 1 waited after RUN only until BUSY cleared, so the 3 s show the title screen. Format 2 would wait for RUN until the machine is idle, which comes at the `INPUT` of `hanoi.bas` at the earliest, and its screenshot would show that prompt |
| Set `timeout:` after measuring the run | `globe-print`, `picard`, every calendar example (`calendar-01` … `calendar-09`) | A long print at full speed comes before `elapsed-time: report`, so the implicit wait has to cover it; version 1 had no limit |
| Blank cards as in-file cards with their own `cue-card:` (no data: the corners are empty until a side is written, then show its bank) | `pdcm-card-tricks` (`D-2` … `D2`) | Version 1 kept the card shown before; format 2 fits the blank card, so it gets its own cue card |
| `cue-card:` on the in-file card | `characters-fastmode` | Version 1 kept the preset's card shown; format 2 fits the in-file card, which has no cue card of its own |
| In-file cards with their cue cards, written, then `- saveas: { card: …, as: live, name: … }` | `pdcm-make-cards` (`PDCM-1`, `PDCM-2`, …) | Version 1 created the card files when written, in the card folder, with the latest `CUECARD:`. Format 2 creates a new card from the fresh template and saves it into the save folder by `saveas:` (`preset.md` §5.2.1, §9.1) |
| `slot-1: { name: ML }` | `firmware.ti59` | Version 1 used the user's module (normally ML); format 2 has none |

### 3.1 Added on 2026-11-13

| Preset | Result |
|---|---|
| `historic/trace-quirk.ti59` | Converts; the cue card look (`Template: MagnetCard`, `Banks:`) is lost. `MODEL: TI-58` in a `.ti59` file: the converted file keeps both (C59-2 EC-6). It ends with keys that need no wait; runs the same |
| `screentest/test_58_glyphs.ti58` | Converts without loss (`PRINTER: off` → no printer). The `Del` keys are waited for, then `- wait: 400ms`, then `- pause: cpu` as the last item, which leaves the machine paused as today; runs the same |
| `interfaces/setcom1600.pc1600` | Converts without loss (`slot-2: { name: CE-1601M }`). Every `type:` is waited for until idle instead of BUSY; differs only as intended (§4, "Commands are waited for to their end") |

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

## 5. Spec changes

**In the specification** (`docs/formats/`, GLOSSARY):

- waiting: the implicit wait, `wait:` as a pause, `skip-wait:`,
  `let-it-finish:`; three states (full speed, regular speed, paused);
- "idle" is the product's matter; a key is held long enough to register;
- `saveas:` for TI-59 magnetic cards; the fresh card is the implicit
  template; a missing card file is an error; a magnetic card has its own
  cue card;
- decided 2026-11-12:
  - S4: a magnetic card without a cue card shows a plain card titled with
    its card name; the corners always show the written banks
    (cue-card.md §2);
  - S6: a reset, a cold boot and a model change end a running preset
    without an error; loading another preset is refused while one runs
    (preset.md §6.1; corrected 2026-11-13, user);
  - S13: a preset doesn't write the TI-58C constant memory file; it takes
    the machine's state at power-off as usual (media.md §4);
  - S14: the 18-character and uniqueness rules apply only to a written
    `preset-name`, not to a file stem (README §2);
  - S15: the `debug:` block keeps camelCase, as an exception
    (README §1);
  - S16: `trace: off` with no trace open has no effect (preset.md §5.4);
  - S17: the user's key presses are dropped while a preset runs
    (preset.md §6.1).

**Open, conversion guide** (`preset.md` §9; set aside for now):

- S3: a converted C59 script starts with `- regular-speed:`.
- S9: an absent `MODEL:` is taken from the suffix.
- S10: `PROGRAM:` before `REGISTERS:`, whatever the file order.
- S11: a preset's own MagnetCard or SolidStateCard becomes a plain card;
  `ID` / `IdAlign` can't be converted.
- §9.2 rows:
  - `plotter: CE-150` → `printer: CE-150`;
  - `floppy-file: …,B` → `{ file: …, side: B }`;
  - `\` in `expect:` text.

**Later, as its own task** (S18): align the documents that still describe
the old waiting rules:

- LIB-6 AC-16, AC-19 and AC-21;
- `docs/contracts/presets.md` PR-4 and PR-5;
- the `wait` upgrade table in C1600-4.

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
| test_diag | needs no edit: it has no pause-on-start; `15 E` is waited for, then `- wait: 1.5s` is the last item. The upgrade drops that last `- wait: 1.5s`, which existed only for the UI test's poll (user, 2026-11-13, B-21 (d) of `docs/walkthroughs/presets.md`) |
| Core fixture `hanoi.pc1600`, `pc1600-hanoi-list` | hanoi.pc1600 (`pc1600-hanoi-run` has an edit of its own, §3) |
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
