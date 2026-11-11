# Preset pass

Phase 5 (`docs/Process.md`; prompt `prompts/phase-5.md`). The presets,
examples and test files that both products ship are played on paper
against the feature specs. This file holds the coverage table, what was
traced, one entry per traced file, and the findings with what became of
each.

Pins: Calc-U-59 `v2.3.0` @ `231792c`; Calc-U-1600 `dev-0.8.1` @
`b4ea8c8`. Every `path:line` of a product is at its pin. Nothing was
built or run.

## 1. The material and who uses it

### 1.1 Calc-U-59

60 preset files (57 under `examples/`, 3 fixtures), 7 card fixtures and
5 ASM overlays.

Users that take many files at once:

| ID | User | Files | What it does |
|---|---|---|---|
| G1 | `tests/shared_state_applier_tests.cpp:97-119` | every preset under `examples/` | reads it, writes program and registers into a machine, runs to the key scan loop (at most 2,000,000 instructions), applies the reloads one after the other without the keys between them. It plays no key and no `Wait:`. |
| G2 | `tests/shared_formats_tests.cpp:132-160` with `tests/golden/formats.txt` | every preset and `.u59` card that the golden file lists; 10 newer presets are not in it | reads the file and compares a dump of what the reader made of it, byte for byte, with a record made by the Swift reader. It runs nothing. |
| G3 | `bin/simulators:42-53, 496-505` | all of `examples/screentest/`, and `assembly/stopwatch.asm`, `historic/characters-fastmode.ti59`, `characters.ti59`, `diag.ti58`, `diag.ti59` | copies them into the iOS simulators, where the UI tests pick them in the file picker. |
| G4 | `Web/build.js:75-79` with `Web/presets.json` | the ten presets directly under `examples/` | bundles them with the Web frontend, which lists them under "Preset". |

The UI tests (`Calc-U-59UITests/`) load a preset through the Preset
button and the file picker, wait up to 8 s of wall time for the
accessibility element `keystroke-playback-status` to read `playing`,
then for it to read `idle` within a time per test, and take a
screenshot; they assert nothing about the display
(`AppHelpers.swift:70-89`). No CI job runs them. `tools/agentic/` has a
reader of its own, which drops every directive but `PrinterTrace:`, and
knows no reloads and no cards (`tools/agentic/ti59file.py:43-91`);
`tools/headless/` loads no preset.

**`examples/`**

| File | Model | Module, printer | Sections | Directives | Users |
|---|---|---|---|---|---|
| `characters.ti59` | (TI-59) | –, on | PROGRAM, CUECARD | – | G1, G2, G3, G4 |
| `diag.ti58` | (TI-58) | –, on | PARTITION, PROGRAM, CUECARD | – | G1, G2, G3, G4 |
| `diag.ti59` | (TI-59) | –, on | PARTITION, PROGRAM, CUECARD, KEYSTROKES | – | G1, G2, G3, G4 |
| `fastmode-sst.ti59` | TI-59 | –, – | PROGRAM, KEYSTROKES | – | G1, G2, G4 |
| `globe-print.ti59` | TI-59 | –, on | PARTITION, PROGRAM, REGISTERS, CARD×4, KEYSTROKES | ZeroElapseTime, FullSpeed, InsertCard, ReportElapseTime | G1, G4 |
| `picard.ti59` | TI-59 | –, on | PARTITION, PROGRAM, REGISTERS, KEYSTROKES | ZeroElapseTime, FullSpeed, ReportElapseTime | G1, G4 |
| `printer-interrupt.ti59` | TI-59 | –, on | PROGRAM, KEYSTROKES | – | G1, G2, G4 |
| `ram_test.ti59` | (TI-59) | –, on | PROGRAM | – | G1, G2, G4 |
| `sum.ti59` | (TI-59) | –, – | PROGRAM, CUECARD | – | G1, G2, G4; `Web/tests/play-calculator.spec.js:210-215` (picks it, waits 5 s for the cue card title) |
| `switzerland.ti59` | TI-59 | –, on | PARTITION, PROGRAM, REGISTERS, KEYSTROKES | FullSpeed | G1, G2, G4 |

**`examples/calendar/`**

| File | Model | Module, printer | Sections | Directives | Users |
|---|---|---|---|---|---|
| `calendar-01-Weinberger.ti59` | TI-59 | ML, on | PROGRAM, REGISTERS, CUECARD, KEYSTROKES | ZeroElapseTime, FullSpeed, RegularSpeed, ReportElapseTime | G1, G2; `tools/agentic` by hand |
| `calendar-02-cargile.ti59` | TI-59 | ML, on | PROGRAM, REGISTERS, CUECARD, KEYSTROKES | ZeroElapseTime, FullSpeed, RegularSpeed, ReportElapseTime | G1, G2; `tools/agentic` by hand |
| `calendar-03-vanderburgh.ti59` | TI-59 | ML, on | PROGRAM, REGISTERS, CUECARD, KEYSTROKES | ZeroElapseTime, FullSpeed, RegularSpeed, ReportElapseTime | G1, G2; `tools/agentic` by hand |
| `calendar-04-skillman.ti59` | TI-59 | ML, on | PROGRAM, REGISTERS, CUECARD, KEYSTROKES | ZeroElapseTime, FullSpeed, RegularSpeed, ReportElapseTime | G1, G2; `tools/agentic` by hand |
| `calendar-05-vanderburgh.ti59` | TI-59 | ML, on | PARTITION, PROGRAM, REGISTERS, CUECARD, KEYSTROKES | ZeroElapseTime, FullSpeed, RegularSpeed, ReportElapseTime | G1, G2; `tools/agentic` by hand |
| `calendar-06-cargile.ti59` | TI-59 | ML, on | PARTITION, PROGRAM, REGISTERS, CUECARD, KEYSTROKES | ZeroElapseTime, FullSpeed, RegularSpeed, ReportElapseTime | G1, G2; `tools/agentic` by hand |
| `calendar-07-calidas.ti59` | TI-59 | ML, on | PROGRAM, REGISTERS, CUECARD, KEYSTROKES | ZeroElapseTime, FullSpeed, RegularSpeed, ReportElapseTime | G1, G2; `tools/agentic` by hand |
| `calendar-08-acosta.ti59` | TI-59 | ML, on | PROGRAM, REGISTERS, CUECARD, KEYSTROKES | FullSpeed, ZeroElapseTime, ReportElapseTime, RegularSpeed | G1, G2; `tools/agentic/calendar/years.py:18,32` (reference printout) |
| `calendar-09-dispatch.ti59` | TI-59 | –, on | PARTITION, PROGRAM, REGISTERS, CUECARD, KEYSTROKES | FullSpeed, ZeroElapseTime, ReportElapseTime | G1, G2; `tools/agentic` by hand |

**`examples/debug/`**

| File | Model | Module, printer | Sections | Directives | Users |
|---|---|---|---|---|---|
| `example-brief.ti59` | (TI-59) | ML, on | PARTITION, PROGRAM, REGISTERS, CUECARD, KEYSTROKES | Wait | G1, G2 |
| `example.ti59` | (TI-59) | –, – | PARTITION, PROGRAM, REGISTERS, CUECARD, KEYSTROKES | Wait | G1, G2 |
| `nibbleerror.ti58` | (TI-58) | ML, – | PARTITION, PROGRAM, CUECARD | – | G1, G2 |
| `operror.ti59` | (TI-59) | –, – | REGISTERS, KEYSTROKES | Trace | G1, G2 |
| `printer-interrupt-bits.ti59` | TI-59 | –, on | PROGRAM, REGISTERS, KEYSTROKES | – | G1, G2 |
| `printer-interrupt-sweep.ti58c` | TI-58C | –, on | PARTITION, PROGRAM, REGISTERS, KEYSTROKES | – | G1, G2 |
| `printer-timing.ti59` | TI-59 | –, on | PROGRAM, KEYSTROKES | RegularSpeed, ZeroElapseTime, ReportElapseTime, FullSpeed | G1, G2 |
| `ret_bug_label.ti59` | (TI-59) | –, – | PROGRAM, CUECARD | – | G1, G2 |
| `ret_bug_repo_2levels.ti59` | (TI-59) | –, on | PROGRAM, CUECARD | – | G1, G2 |
| `selftest-speed-check.ti59` | TI-59 | ML, – | KEYSTROKES | ZeroElapseTime, ReportElapseTime, FullSpeed | G1, G2 |
| `test-cuecard.ti59` | (TI-59) | –, – | REGISTERS, CUECARD | – | G1, G2 |
| `test-magnetcard.ti59` | (TI-59) | –, – | REGISTERS, CUECARD | – | G1, G2 |
| `test-solidstate.ti59` | (TI-59) | –, – | REGISTERS, CUECARD | – | G1, G2 |

**`examples/historic/`**

| File | Model | Module, printer | Sections | Directives | Users |
|---|---|---|---|---|---|
| `FastMode_Zero4.ti59` | (TI-59) | ML, – | PROGRAM | – | G1, G2 |
| `Frequency-Transformation.ti59` | (TI-59) | ML, on | PARTITION, PROGRAM, CUECARD | – | G1, G2 |
| `Integer-Base-Conversion.ti59` | (TI-59) | –, – | PROGRAM, CUECARD | – | G1, G2 |
| `characters-fastmode.ti59` | (TI-59) | ML, on | CUECARD, CARD, KEYSTROKES | InsertCard | G1, G2, G3; `tests/shared_cooperative_tests.cpp:478-484` (played to 8 printed lines, step 5 = 36) |
| `erdkundetest-2.ti59` | TI-59 | –, on | PARTITION, PROGRAM, REGISTERS, CUECARD, KEYSTROKES | – | G1 |
| `erdkundetest.ti59` | TI-59 | –, on | PARTITION, PROGRAM, REGISTERS, CUECARD, KEYSTROKES | – | G1 |
| `fast-grafik-3d-plot.ti59` | TI-59 | ML, on | PARTITION, PROGRAM×2, CUECARD×2, KEYSTROKES×2 (reload) | FullSpeed, RegularSpeed | G1 |
| `fastmode.ti58` | (TI-58) | –, – | PARTITION, PROGRAM | – | G1, G2 |
| `fastmode.ti59` | (TI-59) | –, – | PARTITION, PROGRAM | – | G1, G2 |
| `firmware.ti59` | (TI-59) | –, – | KEYSTROKES | Wait | G1, G2 |
| `keycodes.ti59` | (TI-59) | –, on | PROGRAM, CUECARD | – | G1, G2 |
| `life.ti59` | TI-59 | –, on | PARTITION, PROGRAM, CUECARD, KEYSTROKES | – | G1 |
| `pdcm-card-tricks.ti59` | TI-59 | –, on | PARTITION, CUECARD, CARD×6, KEYSTROKES | FullSpeed, InsertCard, ArmCard, RegularSpeed | G1 |
| `pdcm-make-cards.ti59` | TI-59 | –, on | PARTITION, PROGRAM×2, CUECARD×2, KEYSTROKES×3 (reload) | FullSpeed, ArmCard | G1 |
| `printer-quirks.ti59` | (TI-59) | ML, on | PROGRAM, KEYSTROKES | Wait, Trace, FullSpeed | G1, G2 |
| `ram_test_full_fast.ti59` | (TI-59) | –, on | PARTITION, PROGRAM | – | G1, G2 |
| `repartition.ti58c` | (TI-58C) | –, – | PARTITION, PROGRAM, KEYSTROKES | – | G1, G2 |
| `stars-and-stripes.ti59` | TI-59 | ML, on | PROGRAM, REGISTERS, CUECARD, KEYSTROKES | FullSpeed | G1, G2 |
| `texas-print.ti59` | TI-59 | ML, on | PARTITION, PROGRAM, CUECARD, KEYSTROKES | Wait, FullSpeed | G1, G2 |
| `trace-quirk.ti59` | TI-58 | –, on | PROGRAM, CUECARD, KEYSTROKES | – | G1, G2 |

**`examples/screentest/`**

| File | Model | Module, printer | Sections | Directives | Users |
|---|---|---|---|---|---|
| `test_58_glyphs.ti58` | TI-58 | ML, off | KEYSTROKES | Wait, Freeze | G1, G3; `ScreenshotTests.swift:208-215` (idle within 20 s, screenshot of the frozen display) |
| `test_58c_display.ti58c` | TI-58C | –, – | CUECARD, KEYSTROKES | – | G1, G2, G3; `ScreenshotTests.swift:194-202` (idle within 15 s, screenshot) |
| `test_cdebug.ti59` | TI-59 | LE, on | KEYSTROKES | FullSpeed, RegularSpeed, PrinterTrace, DebugPanel, FreezeOnStart, Wait | G1, G3; `ScreenshotTests.swift:286-310` (idle within 40 s, screenshot of the frozen debugger) |
| `test_diag.ti59` | TI-59 | –, on | PARTITION, PROGRAM, CUECARD, KEYSTROKES | FullSpeed, Wait | G1, G2, G3; `PresetLoadTests.swift:35-60` (playback starts within 8 s); `ScreenshotTests.swift:103-113` (idle within 10 s, screenshot) |
| `test_texas.ti59` | TI-59 | ML, on | PARTITION, PROGRAM, CUECARD, KEYSTROKES | Wait, FullSpeed | G1, G2, G3; `ScreenshotTests.swift:315-322` (idle within 45 s, screenshot) |

**`tests/fixtures/statefiles/`**

| File | Model | Module, printer | Sections | Directives | Users |
|---|---|---|---|---|---|
| `bad-partition.ti58` | (TI-58) | –, – | PARTITION, PROGRAM, CUECARD | – | G2 |
| `edge-cases.ti59` | ti-59 | ST, TRUE | PARTITION, PROGRAM, REGISTERS, CUECARD, KEYSTROKES | Trace, Wait, FullSpeed, RegularSpeed, ZeroElapseTime, ReportElapseTime, PrinterTrace | G2 |
| `model-58c.ti58c` | TI-58C | –, – | PARTITION, REGISTERS | – | G2 |

**`tests/fixtures/cards/`**

| File | What it is | Users |
|---|---|---|
| `program-bank1.u59`, `data-bank3-nocue.u59`, `no-data.u59`, `bad-bank.u59`, `bad-row.u59` | card files, text version 1.0 | G2 (`formats.txt:30937-30949`): read, data and cue card compared |
| `bank4-data.hex`, `bank1-protected.hex` | a bank as the ROM wrote it, as hexadecimal text | `tests/shared_preset_stages_tests.cpp:119-138`: a card built from a preset's block must equal it byte for byte |

**ASM overlays**

| File | Users |
|---|---|
| `examples/assembly/Decoder.asm`, `DPT.asm`, `simple_count.asm`, `stopwatch.asm` | `tests/shared_debug_tests.cpp:238-247`: read only, every instruction at most 1FFF; none is run. `stopwatch.asm` is also copied by G3. |
| `examples/screentest/test_stopwatch.asm` (the same bytes as `stopwatch.asm`) | `ScreenshotTests.swift:125-176`: chosen in the CPU tab, run, R/S, 3.5 s of wall time, R/S, screenshot |

`Web/presets.json` is read by `Web/build.js:76-79` and, as
`presets/manifest.json`, by `Web/src/Dialogs.jsx:113-117`.

### 1.2 Calc-U-1600

59 preset files: 32 under `examples/` (`prompts/phase-5.md` says 34;
the pin has 32, as C1600-4 AC-15 says), 9 for the screenshots of the
documentation, 4 and 3 under `dev/`, 5 test fixtures, 2 presets and 4
templates of the VS Code extension.

Users that take many files at once:

| ID | User | Files | What it does |
|---|---|---|---|
| CHK | `tools/check_presets.sh:10-11` | every preset in the repository except the VS Code templates | `pc1600_cli --check-preset`: reads and checks, builds nothing. A `program: file:` is read; the other files a preset names are not opened. Not run by CI. |
| REL | `release.yaml:25` | all of `examples/` | zipped as `Calc-U-1600-examples.zip` |
| BUN | `Qt6/CMakeLists.txt:214-253`, `Web/build.js:77-94` | `examples/bundled.yaml` and every file of the folders that hold a listed preset (`startup/`, `basic/`, `plotter/`, `machine-code/`) | bundled with both frontends |
| CAT | `Shared/PresetCatalog.cpp:58-98`, checked by `Shared/tests/shared_preset_catalog_tests.cpp:37-69` | the 3 startup presets and 7 examples of `examples/bundled.yaml` | reads the header keys only, for the Examples menu and the settings |

The screenshot scenarios (`docs/developer/screenshots/guide/*.shots.yaml`)
are run by the Qt6 app itself (`Calc-U-1600 --shots`,
`tools/make_screenshots.sh:33-40`): a preset is loaded to its end at
full speed, the machine stands still between the steps of a scenario,
and `key:`, `type:` and `run:` advance emulated time
(`Qt6/app/screenshots/ShotRunner.cpp:115, 233-271`). `dev/tape-matrix/run.py`
writes presets of its own and uses only the `.bas` files of the
examples. `tools/dap_smoke.py` waits for DAP events in wall time.

**`Core/tests/fixtures/presets/`**

| File | Model | Header keys | Sections | Steps | Users |
|---|---|---|---|---|---|
| `hanoi.pc1600` | PC-1600 | – | program (file:); keys×n | key, type | CHK; `job_tests.cpp:232`; `wasm_smoke.cpp:86` |
| `maxed-out-mem.pc1600` | PC-1600 | slot-1: CE-1600M, slot-2: CE-1601M | program (text: \|); keys×n | key, type | CHK; `job_tests.cpp:231` |
| `memtest_stock.pc1500` | PC-1500 | – | program (address:, file:); keys×n | key, type, wait integer | CHK; `job_tests.cpp:234` |
| `startup-pc1500a.pc1500a` | PC-1500A | plotter: CE-150, slot-1: CE-163F | keys | key, type | CHK; `job_tests.cpp:233`; `shared_app_session_tests.cpp:136`; `wasm_smoke.cpp:87` |
| `startup-pc1600.pc1600` | PC-1600 | floppy:, plotter: CE-1600P, slot-1: CE-1600M, slot-2: CE-1601M | keys | key, syncclock, type | CHK; `Core/tests/job_tests.cpp:230` (in parts = in one go); `shared_app_session_tests.cpp:91, 400` (run to its end, state checked); `tools/wasm_smoke.cpp:85` (`Memory:` on the display) |

**`dev/hardware-checks/`**

| File | Model | Header keys | Sections | Steps | Users |
|---|---|---|---|---|---|
| `adrtest.pc1500a` | PC-1500A | – | program (address:, file:); keys×n | key, type, wait integer | CHK |
| `instrquirks.pc1500a` | PC-1500A | – | program (address:, file:); keys×n | key, type, wait integer | CHK |
| `lcdallon.pc1500a` | PC-1500A | – | program (address:, file:); keys×n | key, type | CHK |

**`dev/presets/`**

| File | Model | Header keys | Sections | Steps | Users |
|---|---|---|---|---|---|
| `iterator.pc1500a` | PC-1500A | – | program (text: \|); keys×n | key, type, wait integer | CHK |
| `loader_test_1600_memory_cards.pc1600` | PC-1600 | plotter: CE-1600P, slot-1: CE-1600M, slot-2: CE-1601M | program (file:); keys×n | key, type, wait bare | CHK |
| `supercard_debug.pc1600` | PC-1600 | slot-2: superRAM | keys | key, type | CHK |
| `trace_demo.pc1500a` | PC-1500A | – | program (text: \|); keys×n | key, trace, type, wait integer | CHK; `pc1500_cli --preset` by hand |

**`docs/developer/screenshots/presets/`**

| File | Model | Header keys | Sections | Steps | Users |
|---|---|---|---|---|---|
| `pc1500-ready.pc1500` | PC-1500 | – | keys | key, type | CHK; `02-getting-started.shots.yaml:14` |
| `pc1500a-ce158.pc1500a` | PC-1500A | interface: CE-158 | program (text: \|); keys×n | key, type, wait bare | CHK; `07-com-ports.shots.yaml:14` |
| `pc1500a-ready.pc1500a` | PC-1500A | – | keys | key, type | CHK; `02-getting-started:7, 33`; `06-machine-code:8`; `09-appendix:7, 13` |
| `pc1600-hanoi-list.pc1600` | PC-1600 | – | program (file:); keys×n | key, type | CHK; `04-basic.shots.yaml:8` |
| `pc1600-hanoi-run.pc1600` | PC-1600 | – | program (file:); keys×n | key, type, wait integer | CHK; `04-basic.shots.yaml:12` |
| `pc1600-lissajou.pc1600` | PC-1600 | plotter: CE-1600P | program (file:); keys×n | key, type, wait bare | CHK; `01-introduction:8`; `03-plotter:7, 16` |
| `pc1600-modules.pc1600` | PC-1600 | floppy:, plotter: CE-1600P, slot-1: CE-1600M | keys | key, saveas, type | CHK; `05-modules.shots.yaml:8, 14` |
| `pc1600-ready.pc1600` | PC-1600 | – | keys | key, type | CHK; `02-getting-started:21, 27, 41`; `05-modules:21`; `06-machine-code:17`; `07-com-ports:7` |
| `pc1600-tape.pc1600` | PC-1600 | plotter: CE-1600P | program (file:); keys×n | key, type | CHK; `04-basic.shots.yaml:20` |

**`examples/basic/`**

| File | Model | Header keys | Sections | Steps | Users |
|---|---|---|---|---|---|
| `hanoi.pc1600` | PC-1600 | – | program (file:); keys×n | key, type | CHK, REL, BUN, CAT; `shared_preset_catalog_tests.cpp:73-75`; `Web/tests/play.spec.js:84`; `Web/tests/offline.spec.js:59-63` (loaded from the cache; waits for the model, no notice) |

**`examples/dwx/`**

| File | Model | Header keys | Sections | Steps | Users |
|---|---|---|---|---|---|
| `DiskWorks.pc1600` | PC-1600 | floppy-file:, host-drive:, plotter: CE-1600P, slot-1: CE-1600M, slot-2-file: | keys | key, type | CHK, REL; documentation only |
| `make_diskworks_media.pc1600` | PC-1600 | floppy:, plotter: CE-1600P, slot-1: CE-1600M, slot-2: CE-1601M | program (file:)×n; keys×n | key, saveas, type | CHK, REL; documentation only |

**`examples/interfaces/`**

| File | Model | Header keys | Sections | Steps | Users |
|---|---|---|---|---|---|
| `ce158_demo.pc1500a` | PC-1500A | interface: CE-158 | program (text: \|); keys×n | key, type | CHK, REL |
| `ce158_demo.pc1600` | PC-1600 | interface: CE-158 | program (text: \|); keys×n | key, type | CHK, REL |
| `setcom1600.pc1600` | PC-1600 | slot-2: CE-1601M | program (file:); keys×n | key, type | CHK, REL |

**`examples/machine-code/`**

| File | Model | Header keys | Sections | Steps | Users |
|---|---|---|---|---|---|
| `Calculat.pc1600` | PC-1600 | slot-1: CE-1600M, slot-2: CE-1601M | program (file:); keys×n | key, type | CHK, REL, BUN; documentation only |
| `memtest_1500a.pc1500a` | PC-1500A | – | program (address:, file:); keys×n | key, type, wait integer | CHK, REL, BUN, CAT |
| `memtest_CE1638.pc1500` | PC-1500 | slot-1: CE-1638 | program (address:, file:); keys×n | key, type, wait integer | CHK, REL, BUN |
| `memtest_bank.pc1500a` | PC-1500A | slot-1: CE-1638 | program (address:, file:); keys×n | key, type, wait integer | CHK, REL, BUN; documentation only |
| `memtest_ce155.pc1500` | PC-1500 | slot-1: CE-155 | program (address:, file:); keys×n | key, type, wait integer | CHK, REL, BUN |
| `memtest_stock.pc1500` | PC-1500 | – | program (address:, file:); keys×n | key, type, wait integer | CHK, REL, BUN |

**`examples/memory/`**

| File | Model | Header keys | Sections | Steps | Users |
|---|---|---|---|---|---|
| `flashtest_ce163f.pc1500a` | PC-1500A | slot-1: CE-163F | keys | expect, key, type | CHK, REL |
| `maxed-out-mem.pc1600` | PC-1600 | slot-1: CE-1600M, slot-2: CE-1601M | program (text: \|); keys×n | key, type | CHK, REL |
| `memory-cards/pc1500-maxed-out.pc1500` | PC-1500 | slot-1-file: | keys | key, type, wait integer | CHK, REL |

**`examples/plotter/`**

| File | Model | Header keys | Sections | Steps | Users |
|---|---|---|---|---|---|
| `ascii.pc1600` | PC-1600 | plotter: CE-1600P | program (file:); keys×n | key, type, wait bare | CHK, REL, BUN, CAT |
| `biorhythmus_1600.pc1600` | PC-1600 | plotter: CE-1600P | program (file:); keys×n | key, type, wait bare, wait integer | CHK, REL, BUN, CAT |
| `ce150-text-and-frame.pc1600` | PC-1600 | plotter: CE-150 | keys | type, wait bare | CHK, REL, BUN |
| `ce150_demo.pc1500a` | PC-1500A | plotter: CE-150 | program (text: \|); keys×n | key, type | CHK, REL, BUN |
| `ce1600p-text-and-frame.pc1600` | PC-1600 | plotter: CE-1600P | keys | type, wait bare | CHK, REL, BUN; `tools/pc1600_plotter_probe` by hand |
| `globus.pc1600` | PC-1600 | plotter: CE-1600P | program (file:); keys×n | key, type, wait bare | CHK, REL, BUN, CAT |
| `lissajou-1500.pc1500a` | PC-1500A | plotter: CE-150 | program (file:); keys×n | key, type, wait bare | CHK, REL, BUN, CAT |
| `lissajou-1600.pc1600` | PC-1600 | plotter: CE-1600P | program (file:); keys×n | key, type, wait bare | CHK, REL, BUN, CAT; `Web/tests/play.spec.js:63, 211-213` (waits 45 s for a paper that is not empty) |
| `lissajou-ce150.pc1600` | PC-1600 | plotter: CE-150 | program (file:); keys×n | key, type, wait bare | CHK, REL, BUN |

**`examples/setup/`**

| File | Model | Header keys | Sections | Steps | Users |
|---|---|---|---|---|---|
| `ce1638_bankswrm.pc1500` | PC-1500 | slot-1: CE-1638 | program (address:, file:); keys×n | key, type | CHK, REL |
| `ce1638_bankswrm.pc1500a` | PC-1500A | slot-1: CE-1638 | program (address:, file:); keys×n | key, type | CHK, REL |
| `firmware_bootstrap_util_15.pc1500a` | PC-1500A | slot-1: CE-163F | program (file:)×n; keys×n | key, type, wait integer | CHK, REL |
| `firmware_bootstrap_utilrm_15.pc1500a` | PC-1500A | slot-1: CE-163F | program (file:)×n; keys×n | key, type, wait integer | CHK, REL |
| `firmware_bootstrap_utilrm_20.pc1500a` | PC-1500A | slot-1: CE-163F | program (file:)×n; keys×n | key, type, wait integer | CHK, REL |

**`examples/startup/`**

| File | Model | Header keys | Sections | Steps | Users |
|---|---|---|---|---|---|
| `default-pc1500.pc1500` | PC-1500 | plotter: CE-150, slot-1: CE-163F | keys | key, type | CHK, REL, BUN, CAT; `tools/dap_smoke.py:119` (clean start of a debug session) |
| `default-pc1500a.pc1500a` | PC-1500A | plotter: CE-150, slot-1: CE-163F | keys | key, type | CHK, REL, BUN, CAT; `Shared/tests/shared_app_session_tests.cpp:409-419` (run to its end; CE-150 attached) |
| `default-pc1600.pc1600` | PC-1600 | floppy:, plotter: CE-1600P, slot-1: CE-1600M, slot-2: CE-1601M | keys | key, syncclock, type | CHK, REL, BUN, CAT; `Web/tests/play.spec.js:71-80` (chosen in Settings, not run); `shared_app_session_tests.cpp:421-423` (header); `08-presets.shots.yaml:8` (loaded, LCD captured) |

**`vscode/calcu1600-debug/presets/`**

| File | Model | Header keys | Sections | Steps | Users |
|---|---|---|---|---|---|
| `debug-pc1500a.pc1500a` | PC-1500A | slot-1: CE-163F | keys | key, type | CHK; `extension.js:17, 142` (fallback preset) |
| `debug-pc1600.pc1600` | PC-1600 | – | keys | key, type | CHK; `extension.js:16, 142` (fallback preset); `tools/dap_smoke.py:202, 438, 514` |

**`vscode/calcu1600-debug/templates/`**

| File | Model | Header keys | Sections | Steps | Users |
|---|---|---|---|---|---|
| `pc1500-bus-rom/debug.pc1500a` | PC-1500A | bus-rom: (block), debug: (block) | program (text: \|); keys×n | key, type | `scaffold.js:17, 49-58`; `tools/dap_smoke.py:374-429` |
| `pc1500a-program/debug.pc1500a` | PC-1500A | debug: (block) | keys | key, type | `scaffold.js:14-16, 49-58` |
| `pc1600-bus-rom/debug.pc1600` | PC-1600 | bus-rom: (block), debug: (block) | – | – | `scaffold.js:14-16, 49-58` |
| `pc1600-program/debug.pc1600` | PC-1600 | debug: (block) | keys | key, type | `scaffold.js:14-16, 49-58` |

## 2. Constructs of today's formats and the files that use them

### 2.1 Calc-U-59, the 2.3.0 preset format (`reference/StateFileFormat.md`)

"fixtures" stands for `tests/fixtures/statefiles/`. A count without
names means "many"; section 1 has them per file.

| Construct | Files |
|---|---|
| no version line | all 60 |
| `MODEL:` | 34; none in 26, where the extension counts (21 `.ti59`, 4 `.ti58`, 1 `.ti58c`) |
| `MODEL:` other than the extension | `historic/trace-quirk.ti59`, fixtures `model-58c.ti58c` |
| `SOLID-STATE-MODULE:` | 22; none in 38 |
| `PRINTER: on` | 39; `PRINTER: off`: `screentest/test_58_glyphs.ti58`; `PRINTER: true`: fixtures `edge-cases.ti59`; none in 19 |
| `MODEL:`, `SOLID-STATE-MODULE:` or `PRINTER:` inside a section | 14 (for example `characters.ti59`, `calendar-05`, `debug/example-brief.ti59`) |
| `PARTITION: nnn.xx` | 27; without `.xx`: fixtures only |
| `PROGRAM:` lines of keycodes | 18 |
| `PROGRAM:` lines with a step number, with a label after the keycode | 35, 34 |
| `PROGRAM:` gap `...` | 8 (for example `diag.ti59`, `historic/fastmode.ti59`) |
| `PROGRAM:` a keycode of one digit, a step number of four digits, words between keycodes | fixtures only |
| `PROGRAM:` a keycode followed by a point (`002  11. A`) | `debug/example-brief.ti59:16` |
| `REGISTERS:` | 25; registers 60 to 99: 9; exponent, negative value: `debug/example.ti59`; more than 13 digits: `debug/example.ti59`, `example-brief.ti59` |
| `REGISTERS:` `HNN` | fixtures only (`edge-cases.ti59`, `model-58c.ti58c`) |
| `CUECARD:` | 39: `Template: MagnetCard` 32, `CueCard` 5, `SolidStateCard` 1 (`debug/test-solidstate.ti59`) |
| `CUECARD:` keys `Title`, `Banks`, `A` to `E`, `Row1`, `Row1Align` | many |
| `CUECARD:` keys `A'` to `E'` | `debug/test-cuecard.ti59`, `test-magnetcard.ti59` |
| `CUECARD:` keys `ID`, `Row2`, `Row2R`, `Row2Align`, `Row2RAlign` | `debug/test-solidstate.ti59`; `Row2`, `Row2Align` also `historic/texas-print.ti59`, `screentest/test_texas.ti59` |
| `CUECARD:` key `IdAlign` (taken and not used today) | `debug/test-solidstate.ti59:7` |
| `CUECARD:` keys `PencilColor`, `Style` | `screentest/test_58c_display.ti58c` (`PencilColor`); fixtures |
| `CUECARD:` a label written with `′` (U+2032) | fixtures `edge-cases.ti59` only |
| `CUECARD:` `\blank` and math notation in a value | `calendar-08-acosta.ti59`, `screentest/test_58c_display.ti58c`, fixtures |
| `KEYSTROKES:` | 41; none in 19 |
| several keys on one line; mnemonics after the codes | 26; 11 |
| `21` (2nd) followed by a key | 22 |
| a number below 11 or above 95 on a key line (passed over today, `Shared/StateFile.cpp:95`) | 10 as a mnemonic (`92   0`); fixtures (`10`, `96`) |
| the first `KEYSTROKES:` starts with a key | 31; with `FullSpeed:` 6; with `RegularSpeed:` `debug/printer-timing.ti59`; with `Trace:` `debug/operror.ti59`; with `InsertCard:` `historic/characters-fastmode.ti59` |
| `FullSpeed:`, `RegularSpeed:` | 24, 13 |
| `Wait:` in seconds | 6; with a fraction (`1.5s`): `screentest/test_cdebug.ti59`, `test_diag.ti59`; in `ms`: `screentest/test_58_glyphs.ti58` |
| `ZeroElapseTime:`, `ReportElapseTime:` | 14 each (the calendar examples, `globe-print.ti59`, `picard.ti59`, …) |
| `Trace: <name>`, `Trace: Off` | `debug/operror.ti59`; `Trace: Off` alone: `historic/printer-quirks.ti59` |
| `PrinterTrace: on`, `off` | `screentest/test_cdebug.ti59` |
| `DebugPanel: Calculator`, `FreezeOnStart: Calculator` | `screentest/test_cdebug.ti59` |
| `Freeze: CPU` | `screentest/test_58_glyphs.ti58` |
| reload: `PROGRAM:`, `CUECARD:` after `KEYSTROKES:`; several `KEYSTROKES:` | `historic/fast-grafik-3d-plot.ti59`, `historic/pdcm-make-cards.ti59` |
| reload: `PARTITION:` | `historic/fast-grafik-3d-plot.ti59` |
| `CARD: mem:` … `END-CARD` with `Bank:` | `globe-print.ti59`, `historic/characters-fastmode.ti59`, `historic/pdcm-card-tricks.ti59` |
| `CARD:` with `PROGRAM:`; with `REGISTERS:`; with `CUECARD:` | `characters-fastmode`, `pdcm-card-tricks`; `globe-print`; `globe-print`, `pdcm-card-tricks` |
| `InsertCard: mem:` | `globe-print.ti59`, `characters-fastmode`, `pdcm-card-tricks` |
| `ArmCard: mem:` | `historic/pdcm-card-tricks.ti59` |
| `ArmCard:` with a card file and a side (`PDCM-1:2`) | `historic/pdcm-make-cards.ti59` |
| a tab in the file | `ram_test.ti59` |
| a `#` without a space before it | none |

Used by no file: `SKIP-RESET:` and key code 99 (errors today);
`CutPaper:`; `DebugPanel: CPU` and `Log`; `Freeze: Calculator`;
`FreezeOnStart: CPU` and `Off`; `Step:`; `Resume:`; `InsertCard:` with a
card file; `Protected:` and `Partition:` in a `CARD:` block; a key code
between 11 and 95 that has no key. The card files of version 2.0 and
the binary layouts have no fixture; the five `.u59` fixtures are
version 1.0.

### 2.2 Calc-U-1600, preset format version 1 (`Core/Preset/PresetFile.cpp`)

| Construct | Files |
|---|---|
| `format-version: 1` | all 59 |
| `model:` `PC-1500`, `PC-1500A`, `PC-1600` | 8, 22, 29 |
| `preset-name:`, `description:` | the 32 examples |
| `plotter: CE-150`, `plotter: CE-1600P` | 7, 13 |
| `interface: CE-158` | `interfaces/ce158_demo.pc1500a`, `ce158_demo.pc1600`, screenshots `pc1500a-ce158.pc1500a` |
| `slot-1:` with `CE-155`, `CE-1638`, `CE-163F`, `CE-1600M` | 1, 4, 8, 9 |
| `slot-2: CE-1601M`; `slot-2: superRAM` | 8; `dev/presets/supercard_debug.pc1600` |
| `slot-1-file:`; `slot-2-file:` | `memory/memory-cards/pc1500-maxed-out.pc1500`; `dwx/DiskWorks.pc1600` |
| `floppy:` | `startup/default-pc1600.pc1600`, its fixture copy, screenshots `pc1600-modules.pc1600`, `dwx/make_diskworks_media.pc1600` |
| `floppy-file:`, `host-drive:` | `dwx/DiskWorks.pc1600` |
| `bus-rom:` (`file`, `address`, `pv`; `file`, `bank`) | VS Code templates `pc1500-bus-rom`, `pc1600-bus-rom` |
| `debug:` block | the four VS Code templates (`program` with `bin`, `listing`, `after`; `listings`; `command`; `boot`, `cpu`, `bank`) |
| a header key after a section | three VS Code templates |
| header keys only | VS Code template `pc1600-bus-rom` |
| a quoted value | `interfaces/setcom1600.pc1600`, `memory/maxed-out-mem.pc1600` |
| `program:` with `file:` (a `.bas`) | 18 |
| `program:` with `file:` (a `.bin`), with `address:` | 12, 11 |
| `program:` with `text: \|` | 9 |
| `program:` twice or more | `dwx/make_diskworks_media.pc1600`, the three `setup/firmware_bootstrap_*` |
| `keys:` more than once, around a `program:` | 39 |
| `- key:` with `mode`, `cl` | 54, 33; `enter`, `f1`, `f2`, `rsv`, `shift`: the PC-1600 startup preset and a few; `break`, `up`: `dev/presets/iterator.pc1500a` |
| `- type:` | 58; with lower-case letters 14; with a `#` in the text: `dwx/make_diskworks_media.pc1600`, `machine-code/Calculat.pc1600`; starting with a quote: the PC-1600 startup preset and its fixture copy |
| `- wait:` with whole seconds | 16 |
| `- wait:` without a value | 11 |
| `- expect:` | `memory/flashtest_ce163f.pc1500a` |
| `- syncclock:` | `startup/default-pc1600.pc1600`, its fixture copy |
| `- saveas:` | `dwx/make_diskworks_media.pc1600` (`template …:file:`), screenshots `pc1600-modules.pc1600` (`live slot-1:My programs`) |
| `- trace:` | `dev/presets/trace_demo.pc1500a` |
| `{{name}}` placeholders | the four VS Code templates |

Used by no file: `- screenshot:`; `slot-1-rom:`, `slot-2-rom:`;
`typed:` and `length:` in `program:`; a variant after `model:` or
`plotter:` (`PC-1600:old`, `CE-1600P:old`); a side after `floppy:`
(`,A`, `,B`); `- wait:` with a fraction; `- key: ent`; `- key:` with
several keys (new in version 2); a header key given twice.

## 3. What was traced

Traced in full, each by a fresh Opus 5.5 agent that got the files of
one group, the rules, and the specs to read by itself:

| Group | Files | Why |
|---|---|---|
| 59-A | `examples/diag.ti59`, `diag.ti58`, `ram_test.ti59`, `sum.ti59`, `characters.ti59` | bundled with the Web frontend (G4); `sum.ti59` in a Web test |
| 59-B | `examples/printer-interrupt.ti59`, `picard.ti59`, `globe-print.ti59`, `switzerland.ti59`, `fastmode-sst.ti59` | bundled with the Web frontend (G4) |
| 59-C | the five presets of `examples/screentest/` and `test_stopwatch.asm` | the UI tests and the App Store screenshots depend on them |
| 59-D | `tests/fixtures/statefiles/` (3), `tests/fixtures/cards/` (7), `examples/assembly/*.asm` (4) | fixtures of tests; the constructs only fixtures use |
| 59-E | the 13 presets of `examples/debug/` | constructs: traces, the three cue card templates, `IdAlign`, a TI-58C |
| 59-F | `historic/characters-fastmode.ti59`, `pdcm-card-tricks.ti59`, `pdcm-make-cards.ti59`, `fast-grafik-3d-plot.ti59` | a test plays the first; constructs: cards, arm and insert, reloads |
| 59-G | 11 more of `examples/historic/`, and `calendar-01`, `-05`, `-09` in full; the other 5 historic and 6 calendar files line by line for anything new | constructs; the first file of each folder |
| 1600-A | `examples/startup/` (3), `Core/tests/fixtures/presets/` (5), `examples/bundled.yaml` | startup presets; fixtures of tests |
| 1600-B | the seven listed examples (`basic/hanoi`, `plotter/lissajou-1500`, `lissajou-1600`, `globus`, `biorhythmus_1600`, `ascii`, `machine-code/memtest_1500a`) | bundled and listed (CAT); two in Web tests |
| 1600-C | the other four of `examples/plotter/`, the other five of `examples/machine-code/`, `dev/presets/` (4), `dev/hardware-checks/` (3) | developer tools; constructs: `trace`, `break`, `superRAM` |
| 1600-D | `docs/developer/screenshots/presets/` (9) with the scenarios | the screenshot scenarios depend on them |
| 1600-E | `vscode/calcu1600-debug/presets/` (2) and `templates/` (4) | the VS Code extension and `tools/dap_smoke.py` depend on them |
| 1600-F | `examples/dwx/` (2), `memory/` (3), `interfaces/` (3), `setup/` (5) | constructs: media by file, `saveas`, `host-drive`, `expect`, two `program:` sections |

Every file of the material is thus in a group. Left to a line-by-line
look instead of a full trace: five historic and six calendar examples
of Calc-U-59 (group 59-G), which have the shape of the files traced
before them and no user of their own. Not traced: the `.bas`, `.bin`,
`.card.yaml` and `.floppy.yaml` files that presets name, beyond what a
preset does with them; the copies on calc-u.org, which are the same
files (`docs/inventory/calc-u-site.md`).
