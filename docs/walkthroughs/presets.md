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

## 4. The traced files

One line per traced file: "holds", or the findings of section 5 it
showed. A finding named for one file can hold for others with the same
line; section 5 says how far it reaches. The tracers' full reports, with
each file written in version 2 and gone through line by line, are
working files outside the repository and are not kept.

**Group 59-A**

| File | Result |
|---|---|
| `examples/diag.ti59` | B-1, B-16, B-23 |
| `examples/diag.ti58` | A-2, B-10, B-16 |
| `examples/ram_test.ti59` | B-10, B-13, B-16 |
| `examples/sum.ti59` | B-10, B-16, B-23 |
| `examples/characters.ti59` | B-10, B-16 |
| the list of `Web/presets.json` | B-27 |

**Group 59-B**

| File | Result |
|---|---|
| `examples/printer-interrupt.ti59` | holds |
| `examples/picard.ti59` | A-1, B-1 |
| `examples/globe-print.ti59` | A-1, B-1, B-6, B-9, B-10, B-13 |
| `examples/switzerland.ti59` | A-1, B-1 |
| `examples/fastmode-sst.ti59` | holds |

**Group 59-C**

| File | Result |
|---|---|
| `screentest/test_58_glyphs.ti58` | B-10, B-15, B-21 |
| `screentest/test_58c_display.ti58c` | B-21 |
| `screentest/test_cdebug.ti59` | A-13, B-1, B-2, B-3, B-10, B-13, B-15, B-21 |
| `screentest/test_diag.ti59` | B-1, B-13, B-21 |
| `screentest/test_texas.ti59` | B-1, B-3, B-13, B-21 |
| `screentest/test_stopwatch.asm` | A-4, B-21 |

**Group 59-D**

| File | Result |
|---|---|
| `fixtures/statefiles/bad-partition.ti58` | A-2, B-14, B-15 |
| `fixtures/statefiles/edge-cases.ti59` | A-1, B-13, B-14, B-15 |
| `fixtures/statefiles/model-58c.ti58c` | A-2, B-15 |
| `fixtures/cards/program-bank1.u59` | B-14 |
| `fixtures/cards/data-bank3-nocue.u59` | B-14 |
| `fixtures/cards/no-data.u59, bad-bank.u59, bad-row.u59` | holds |
| `fixtures/cards/bank1-protected.hex, bank4-data.hex` | holds |
| `assembly/Decoder.asm, DPT.asm` | B-18, B-26 |
| `assembly/simple_count.asm` | B-18, B-26 |
| `assembly/stopwatch.asm` | A-4, B-18, B-26 |

**Group 59-E**

| File | Result |
|---|---|
| `debug/example-brief.ti59` | A-1, A-2, B-1, B-10, B-12 |
| `debug/example.ti59` | A-1, A-2, B-1, B-10 |
| `debug/nibbleerror.ti58` | A-2, B-10 |
| `debug/operror.ti59` | A-2, B-1, B-10, B-13, B-17 |
| `debug/printer-interrupt-bits.ti59` | A-1, B-1, B-10 |
| `debug/printer-interrupt-sweep.ti58c` | A-1, B-1, B-10 |
| `debug/printer-timing.ti59` | A-3, B-1, B-10 |
| `debug/ret_bug_label.ti59, ret_bug_repo_2levels.ti59` | A-2, B-10 |
| `debug/selftest-speed-check.ti59` | B-1, B-10 |
| `debug/test-cuecard.ti59, test-solidstate.ti59` | A-2, B-10, B-13 |
| `debug/test-magnetcard.ti59` | A-2, B-10 |

**Group 59-F**

| File | Result |
|---|---|
| `historic/characters-fastmode.ti59` | B-1, B-10 |
| `historic/pdcm-card-tricks.ti59` | B-1, B-6, B-7, B-8, B-10 |
| `historic/pdcm-make-cards.ti59` | A-12, B-1, B-6, B-8, B-10 |
| `historic/fast-grafik-3d-plot.ti59` | B-1, B-10, B-13 |

**Group 59-G**

| File | Result |
|---|---|
| `historic/trace-quirk.ti59` | B-10, B-13 |
| `historic/repartition.ti58c` | A-2, B-10, B-13, B-28 |
| `historic/printer-quirks.ti59` | A-2, B-1, B-13, B-14 |
| `historic/firmware.ti59` | A-2, B-13 |
| `historic/fastmode.ti58` | A-2, B-10 |
| `historic/fastmode.ti59, FastMode_Zero4.ti59, keycodes.ti59, ram_test_full_fast.ti59` | A-2 |
| `historic/stars-and-stripes.ti59` | A-1, B-1 |
| `historic/texas-print.ti59` | B-1, B-10, B-13 |
| `calendar/calendar-01, -05` | B-1, B-25 |
| `calendar/calendar-09-dispatch.ti59` | A-1, B-1, B-25 |
| `historic/erdkundetest.ti59, erdkundetest-2.ti59` (line by line) | A-1 |
| `historic/life.ti59` (line by line) | B-1, B-10 |
| `historic/Integer-Base-Conversion.ti59, Frequency-Transformation.ti59` (line by line) | A-2, B-14 |
| `calendar-02, -03, -06, -07` (line by line) | B-1, B-25 |
| `calendar-04-skillman.ti59` (line by line) | B-1, B-14, B-25 |
| `calendar-08-acosta.ti59` (line by line) | B-1, B-13, B-14, B-25 |

**Group 1600-A**

| File | Result |
|---|---|
| `examples/startup/default-pc1500.pc1500` | A-10, B-5 |
| `examples/startup/default-pc1500a.pc1500a` | B-5 |
| `examples/startup/default-pc1600.pc1600` | B-4, B-5, B-8, B-20 |
| `fixtures/presets/hanoi.pc1600` | B-4, B-20 |
| `fixtures/presets/maxed-out-mem.pc1600` | B-5, B-8, B-20 |
| `fixtures/presets/memtest_stock.pc1500` | B-4, B-20 |
| `fixtures/presets/startup-pc1500a.pc1500a` | B-5, B-20 |
| `fixtures/presets/startup-pc1600.pc1600` | B-4, B-5, B-8, B-20 |
| `examples/bundled.yaml` | holds |

**Group 1600-B**

| File | Result |
|---|---|
| `basic/hanoi.pc1600` | A-6, B-4, B-27 |
| `plotter/lissajou-1500.pc1500a` | B-4, B-27 |
| `plotter/lissajou-1600.pc1600` | A-5, B-4, B-27 |
| `plotter/globus.pc1600, biorhythmus_1600.pc1600, ascii.pc1600` | A-5, B-4, B-27 |
| `machine-code/memtest_1500a.pc1500a` | B-4, B-27 |

**Group 1600-C**

| File | Result |
|---|---|
| `plotter/ce150_demo.pc1500a` | B-4, B-27 |
| `plotter/ce150-text-and-frame.pc1600` | B-27 |
| `plotter/ce1600p-text-and-frame.pc1600` | A-5, B-25, B-27 |
| `plotter/lissajou-ce150.pc1600` | B-4, B-27 |
| `machine-code/Calculat.pc1600` | B-27, C-3 |
| `machine-code/memtest_bank.pc1500a, memtest_ce155.pc1500, memtest_CE1638.pc1500, memtest_stock.pc1500` | B-27 |
| `dev/presets/iterator.pc1500a` | A-10, B-4 |
| `dev/presets/loader_test_1600_memory_cards.pc1600` | A-5, A-10, B-4 |
| `dev/presets/supercard_debug.pc1600` | A-10, B-4 |
| `dev/presets/trace_demo.pc1500a` | A-10, B-4, B-17 |
| `dev/hardware-checks/adrtest, instrquirks, lcdallon` | A-10 |
| `dev/tape-matrix/run.py` | B-25 |

**Group 1600-D**

| File | Result |
|---|---|
| `screenshots/pc1500-ready.pc1500, pc1500a-ce158.pc1500a, pc1600-hanoi-list.pc1600, pc1600-hanoi-run.pc1600` | B-4, B-22 |
| `screenshots/pc1500a-ready.pc1500a` | B-4, B-22 |
| `screenshots/pc1600-lissajou.pc1600` | A-5, B-4, B-22 |
| `screenshots/pc1600-modules.pc1600` | A-5, A-7, B-4, B-5, B-8, B-22 |
| `screenshots/pc1600-ready.pc1600` | B-4, B-22 |
| `screenshots/pc1600-tape.pc1600` | A-5, B-4, B-22 |

**Group 1600-E**

| File | Result |
|---|---|
| `vscode presets/debug-pc1500a.pc1500a, debug-pc1600.pc1600` | holds |
| `templates/pc1500a-program/debug.pc1500a` | B-19, B-24 |
| `templates/pc1600-program/debug.pc1600` | A-11, B-19, B-24 |
| `templates/pc1500-bus-rom/debug.pc1500a` | B-19, B-24, B-29 |
| `templates/pc1600-bus-rom/debug.pc1600` | A-9, B-19, B-24 |
| the presets `tools/dap_smoke.py` writes | A-10 |

**Group 1600-F**

| File | Result |
|---|---|
| `dwx/DiskWorks.pc1600` | B-4, B-5, B-8, B-11, B-26 |
| `dwx/make_diskworks_media.pc1600` | A-7, B-4, B-5, B-8, B-11, B-26 |
| `memory/flashtest_ce163f.pc1500a` | B-5 |
| `memory/maxed-out-mem.pc1600` | B-5 |
| `memory/memory-cards/pc1500-maxed-out.pc1500` | B-5, B-11, B-26 |
| `interfaces/ce158_demo.pc1500a` | B-25, B-26 |
| `interfaces/ce158_demo.pc1600` | B-25 |
| `interfaces/setcom1600.pc1600` | B-4, B-5 |
| `setup/ce1638_bankswrm.pc1500, .pc1500a` | B-5 |
| `setup/firmware_bootstrap_util_15, utilrm_15, utilrm_20` | B-5 |

## 5. Findings

The thirteen tracers reported 143 findings. Every one was checked
against the spec text and the pinned code by the main session; findings
of several tracers about one thing were merged, which leaves the 49
below. Classes (`prompts/phase-5.md`, step 6): **A** a recorded
decision was missing from a criterion or from the ledger, fixed; **B**
needs the user; **C** cannot be settled on paper and goes into
`first-steps.md`. Nothing a tracer reported was dropped as wrong; where
a finding falls away with the ruling on another, it says so.

### 5.1 Class A: fixed

| # | Finding | Record that was missing from the criterion | Fixed in |
|---|---|---|---|
| A-1 | `registers: \|` and `cue-card: \|` did not say that ` #` starts a comment; every register listing of the examples has such comments, so the upgraded files would have been refused | `presets.md` PR-6 ("inside the TI block scalars … ` #` starts a comment"); C59-2 decision log | C59-2 AC-4, AC-5 |
| A-2 | A 2.3.0 preset without `MODEL:` loads into the running model (`Shared/StateFile.cpp:545-552`, `Shared/SessionController.cpp:598`); its upgraded file names the model of its extension, so `diag.ti58` switches to a TI-58. No ledger row said so | `presets.md` PR-2 (`model:` is required); C59-2 AC-10, second row | ledger row 59-4 |
| A-3 | Key holds in emulated time change what `Adv` feeds at full speed and what an `Elapsed:` line counts (`reference/StateFileFormat.md:299-301`); no ledger row | `typing.md` T-1; `presets.md` PR-10 | ledger row 59-23 (new) |
| A-4 | `examples/screentest/test_stopwatch.asm`, the overlay the screenshot test loads (`ScreenshotTests.swift:125-176`), was not among the overlays that are upgraded | ADR 0037, ledger row 59-1 | C59-4 AC-20 |
| A-5 | The upgrade table of the Sharp presets had rows for `plotter: CE-150` and `- wait: 2` only; `plotter: CE-1600P` (13 files) and `- wait: 20` had none under "only this changes" | `presets.md` PR-5, PR-11; ledger row 1600-3 | C1600-4 AC-15 |
| A-6 | The reply to the preset list had no group and did not tell a startup preset from an example, which the Examples menu and the settings need (`Web/bindings.cpp:562-571`) | LIB-6 AC-31; LIB-14 AC-1, AC-12 | LIB-6 AC-32 |
| A-7 | LIB-7 AC-21 gave its notation (a path with a space in quotes) to "the steps of AC-26", `saveas` among them, while AC-26 keeps the rest of the line for `saveas`, as `- saveas: template slot-2:file:CE-1601M - Progs.card.yaml` needs | LIB-7 OQ-3 ("`saveas:` keeps today's form") | LIB-7 AC-21 |
| A-8 | Ledger row 1600-7 says only that the first write "replaces an instance of that name", not that it makes one, as row 59-7 does | `media.md` MD-1; LIB-7 AC-25 | void: rows 59-7 and 1600-7 withdrawn with F-1 (user, 2026-11-13) |
| A-9 | `listings` and `symbols` of a `debug:` block were "a list of files"; the VS Code template `pc1600-bus-rom` writes maps (`path`, `cpu`, `bank`), which today's reader takes (`Core/Debug/AttachKeys.hpp:69-88`) | C1600-4 AC-19, AC-20 (today's YAML, today's tests) | C1600-4 AC-11 |
| A-10 | The presets under `dev/` and the presets `tools/dap_smoke.py` writes as text were not named among those that are upgraded; LIB-6 AC-51 said the test suite checks all presets "as today", where today a tool does it by hand (`tools/check_presets.sh:2-11`) | C1600-4 decision log ("every shipped preset is upgraded and checked"); C1600-3 AC-20 | C1600-4 AC-15; LIB-6 AC-51 |
| A-11 | "The lowest address of the listing" would be an equate's address in a zasm listing; today only lines with bytes count (`Core/Debug/Listing/ZasmListing.cpp:90-91`) | C1600-3 AC-23 | C1600-3 AC-7 |
| A-12 | Two steps that name one card (`PDCM-1:1`, `PDCM-1:2`): one card or two fresh ones, the second replacing the first | C59-7 AC-27 ("both sides in one file"; `tests/shared_cooperative_tests.cpp:791-812`) | C59-7 AC-20 |
| A-13 | LIB-10 AC-13 listed what an operation lets through, and a pause-on-start that fires was not in the list; `test_cdebug.ti59` arms and fires inside its own load | LIB-10 AC-9 (no exception); LIB-6 AC-19 as decided on 2026-11-11 | LIB-10 AC-13 |

Each fix has its row in the decision log of its spec.

### 5.2 Class B: put to the user

Status of all: open, put to the user on 2026-11-11. Each has the facts,
the spec text it meets, and one recommendation.

**Re-evaluated on 2026-11-13 against `docs/formats/`** (the user's
format specification of 2026-11-12). "Settled" means the format states
it; "read into" says what was taken from it beyond its words. Applied so
far: LIB-6, C59-2, C1600-4 (commit `fe9fa0b`); the other specs wait for
the questions of §5.4.

| # | Status | What the format says, and what is left |
|---|---|---|
| B-1 | Settled | A `key:` has finished when its last press is done and the machine is idle; every item waits for that, bounded by `timeout:` (default 3600 s emulated), after which the item **fails** (not "goes on", as recommended); the last item is not waited for; user keys are dropped while a preset runs; anything that resets ends it (`preset.md` §6). Applied: LIB-6 AC-56, AC-57, AC-60, AC-61. LIB-5 AC-17 is still to be aligned |
| B-2 | **Ruled 2026-11-13 (user): the release completes on the paused machine, which does not see it; written as "the rest of the hold in host time, in the harness at once" (LIB-5 AC-20, LIB-6 AC-19)** | Debugger and frontend steps go on while paused; a paused machine is not idle; `test_cdebug` gets `skip-wait:` (conversion check §3). Left: whether a press whose hold the pause interrupts comes up while paused; if not, it is never "carried out" and `skip-wait:` never lets the next step start. Recommendation unchanged |
| B-3 | **Ruled 2026-11-13 (user): (1), the scan loop only (C59-1 AC-18). after `skip-wait:` the first press goes down at once, ready or not (corrected by the user, see B-4); holding a key into a running program needs a `key-down` step, not in this release** | Unchanged. Applied meanwhile: a TI-59 that waits for a card is idle (LIB-6 AC-36; `preset.md` §6.2) |
| B-4 | **Ruled 2026-11-13 (user): (a) idle = key readiness for 20 frames, `INPUT` and RESERVE included (LIB-6 AC-45); (b) presses as today (LIB-5 AC-14, AC-28). And with B-3: after `skip-wait:` the first press goes down at once (`preset.md` §5.1, LIB-6 AC-58)** | The waits after a `type:` or `key:` are the implicit wait now, not 1 s or 12 s (`preset.md` §9.2); the hold and the start of a press are "the product's" (§5.1). So "tick for tick" falls away. Left: (a) the holds and the start of a line: recommendation, today's (4 + 4 frames; PC-1600 key readiness for 15 frames, at most 15 s); (b) Sharp idle must include a program waiting at `INPUT` and the RESERVE mode, or `globus`, `biorhythmus_1600`, `ascii` and the PC-1600 startup preset fail at `timeout:` (C-2) |
| B-5 | Ruled as recommended (user, 2026-11-13, with F-1; applied in LIB-7 AC-5, AC-15, AC-25) | A template is never written; Name & Save copies it with its live contents; a saved copy never takes a bundled template's name (`media.md`, Templates and instances; `modules.md` §1.8; `preset.md` §5.2.1). Read into: a template a preset names by name keeps what is written in its place, as one chosen in a picker. A-8 follows. To apply in LIB-7 with §5.4 F-1 |
| B-6 | Settled, part 3 reversed | Every card is fitted after it has passed, `mem:` cards included (parts 1, 2 as recommended); but a magnetic card carries its own cue card, never the preset's: without one it shows a plain card titled with its name (`cue-card.md` §2, `media.md` §1.4). To apply in C59-7 |
| B-7 | Settled otherwise | A `mem:` name the preset does not define is a fresh in-file card (`preset.md` §3.1); no section is needed |
| B-8 | Mostly settled; (d) **ruled 2026-11-13 (user): kind and name tag, no path (LIB-7 AC-25)**; (a) **ruled 2026-11-13 (user): sides 0 and 1 for cards and disks, the floppy format goes to version 2 (`media.md`, `preset.md` §9.4, LIB-7 AC-37, ledger 59-24, 1600-19)** | (a) side 1 and side A by default (§3, §5.2); the case of side names is open; (b) `{ name: Formatted, side: B }` (§9.2); (c) void: `arm:` is only for the user to feed, and a new card fitted replaces the one before; (d) open; (e) card files are `.magnetcard.yaml` version 3, a missing or outdated one refuses; whether old `.u59` files get the hint is §5.4 F-2 |
| B-9 | Void | The 30 minutes give way to `timeout:` (`preset.md` §6.3); as the last item a card is not waited for |
| B-10 | **Ruled 2026-11-13 (user): as recommended, worded for format 2 (C59-2 AC-11)** | The upgraded files run differently by design (full speed, last item not waited for, `let-it-finish:`, `skip-wait:`), so the comparison is at the idle machine after the load (harness `wait`, LIB-6 AC-34; C59-2 AC-11 so worded). Cards a preset writes are saved by `saveas:` now; compare their content. Recommendation otherwise unchanged |
| B-11 | **Ruled 2026-11-13 (user): (a) to (d) as recommended (C1600-4 AC-16; LIB-6 AC-43, AC-51)** | (a) to (c) unchanged; (d) new: C1600-4 AC-16 "at the end of the load" no longer compares like with like; recommendation: as B-10, at the idle machine, the intended differences of the conversion check §4 excepted by name |
| B-12 | **Ruled 2026-11-13 (user): corrected to `11` (C59-2 AC-10)** | Unchanged; under §4.1 `11.` is no keycode either |
| B-13 | **(a), (d), (f) ruled 2026-11-13 (user; C59-2 AC-10)**; the rest settled by the format | (a) a tab is an error, the converter's rule is open (conversion guide set aside); (b) settled: no `IdAlign` in the format; (c) settled: an empty label is an empty cell (`cue-card.md` §3); (d) open; (e) reversed: a duration may have a fraction, `1.5s`; (f) open; (g) void: `magnetic-cards:` is a header key and may stand anywhere |
| B-14 | **(a), (e) ruled 2026-11-13 (user): last line without a line end; fixed values read in any case (`README.md` §1)**; the rest settled by the format | (a) open; (b) settled: tokens are split by spaces; (c) settled as recommended (§4.2); (d) settled as recommended (§5.4, S16); (e) settled: an unknown key refuses, labels in any case, `A′` accepted; the case of values open; (f) settled: card files are version 3, without `Template` and `Banks` |
| B-15 | **Ruled 2026-11-13 (user): as recommended (C59-2 AC-10, Technical Requirements)** | The three fixtures become rejection tests (conversion check §1). The rest open |
| B-16 | **Ruled 2026-11-13 (user; the Sharp printers' equivalent switches are an open point of `features/INDEX.md`): TRACE belongs to the printer like its paper; a preset's absent `printer-trace` is off; every absent keyword has a default (C59-6 AC-6, LIB-6 AC-65, `preset.md` §2, §5.5, ledger 59-25)** | Unchanged; the step is `printer-trace:` now |
| B-17 | **Ruled 2026-11-13 (user): (a) key and card records go, a trace is the CPU log only (LIB-10 AC-33, ledger 59-19); (b) the run's own save folder by default, an option overrides (LIB-11 AC-8)** | Unchanged; and `screenshot:` writes to the trace folder (`preset.md` §5.4) against LIB-9 AC-17 (LIB-6 OQ-6) |
| B-18 | **Ruled 2026-11-13 (user): (a), (b), (d), (e) as recommended; (c) a cold boot or a new machine removes the overlay (LIB-10 AC-29; C59-4 AC-15, AC-17, AC-20, AC-23, AC-26; ledger 59-1)** | Unchanged |
| B-19 | (a), (b) **ruled 2026-11-13 (user): two uses only, normal breakpoints or a stop at the reset vector; no more specified (C1600-3 AC-6, AC-24)**; (c), (e) **ruled 2026-11-13 (user) as recommended: at session start the program waits for `configurationDone`; Build & Load without a program does the set-up again, then as at `configurationDone`. Applied in C1600-3 AC-7, AC-25**; C1600-4 OQ-5 **ruled 2026-11-13 (user) as recommended: `reset: none` and `after: none` defaults, §3.4 reset worded as AC-24, no `preset` key in the block. Applied in C1600-4 AC-11, `preset.md` §3.4**; (d), (f) settled | (d) settled as recommended: a load for use ignores the block, which is still read by the format (§3.4); (f) settled against the recommendation: `bus-rom:` once (C1600-4 AC-9) |
| B-20 | **Ruled 2026-11-13 (user) as recommended: a fixed instant by default; an option names another date and time or the host's. Applied in LIB-11 AC-21, LIB-2 AC-23** | Unchanged; `syncclock:` is Sharp-only |
| B-21 | **Ruled 2026-11-13 (user) as recommended, "for now": (a) busy topic as a stream, mechanism from spike 4; (b) a settings file as a launch argument in development builds; (c) `key-down`/`key-up`; (d) `test_diag`'s wait dropped. Simulator UI testing needs work later (C59-8 OQ-5). Applied in C59-8 AC-21, OQ-5; conversion check** | (d) the conversion check keeps `test_cdebug`'s `wait: 1.5s` behind a `skip-wait:`; for `test_diag` see §5.4 (conversion check) |
| B-22 | **Ruled 2026-11-13 (user, left to the specification) as recommended: Qt6 gets harness attach; `--shots` is its client; LIB-14 owns scenarios and object names. Applied in LIB-14 AC-44, LIB-11, LIB-16 AC-12, `harness.md` fit table** | Unchanged; the command names follow LIB-6 AC-34 |
| B-23 | **Ruled 2026-11-13 (user) as recommended: what the example puts on screen, `sum.ti59`'s cue card in Calc-U-59, the Sharp example the design's; offline load of a bundled example kept. Applied in LIB-6 AC-42, LIB-15 AC-23** | Unchanged |
| B-24 | **Ruled 2026-11-13 (user) as recommended: each template filled and checked; RENUM with a ROM fixture. Applied in C1600-3 AC-19, C1600-4 AC-15, LIB-6 AC-51** | Unchanged |
| B-25 | (a), (b), (d) **ruled 2026-11-13 (user): calendar scripts upgraded; old scripts and probes not taken over (tape-matrix, the three probes). Applied in C59-2 AC-18, LIB-11 OQ-5, C1600-5 AC-10 withdrawn, `harness.md`**; (c) **ruled 2026-11-13 (user), revised recommendation: no hold; refused as EC-3 says, the script builds with commands, connects, then starts. Applied in C1600-5 EC-3** | Unchanged |
| B-26 | **Ruled 2026-11-13 (user) as recommended: comments corrected, READMEs under `examples/` upgraded with the files. Applied in C59-2 AC-10, C59-4 AC-20, C1600-4 AC-15** | Unchanged |
| B-27 | (a), (d) **ruled 2026-11-13 (user): a bundle file (`docs/formats/bundle.md`) lists the presets and their files, order and groups; every distribution bundles exactly that; a selection; no examples zip; the site takes the files from the repository. Applied in LIB-6 AC-31, C1600-4 AC-17, C59-2 AC-13, LIB-19 AC-1, AC-9, OQ-5, glossary**; (c) **ruled 2026-11-13 (user) as recommended: a path inside the bundle; unlisted refused with a notice. Applied in LIB-15 AC-16** | (b) settled otherwise: a list shows the name tag (`preset-name`, else the file stem), the description is the tooltip or longer entry (`README.md` §2). (a), (c), (d) open |
| B-28 | **Ruled 2026-11-13 (user): zeroed defaults, then the memory file where one is handed, then the processor powered; nothing clears memory after the start. Applied in C59-1 AC-6, C59-3 AC-8, AC-9, C59-2 AC-1, EC-3, `media.md` §4** | A cold boot zeroes all memory before the processor starts, the constant memory included (`preset.md` §2); what the ROM's start-up then writes is confirmed by C-6 |
| B-29 | **Ruled 2026-11-13 (user) as recommended: "unless the model's start leaves it there"; C-7 confirms. Applied in C1600-4 AC-4** | Unchanged |

#### Steps and time

**B-1 — When a `key` step ends on a TI.**
Today: after each key, the last one too, playback waits until the
processor is back in the key scan loop, so until the program the key
started has stopped; only then does the next directive act and the
script end (`Shared/KeySequencer.cpp:157-167`). The format reference
gives this as the way to run a program at full speed
(`reference/StateFileFormat.md:291-296`). The user can press keys
meanwhile. Specs: LIB-5 AC-17 "Given `key`, then the reply comes when
the last key is up"; LIB-6 EC-4 "the load is over after that step, and
the program runs on at regular speed"; `speed`, `elapsed`, `switch`,
`trace` need no emulated time (LIB-6 AC-19); C59-2 AC-10 writes one
step per line and adds nothing. Under the specs: the `Elapsed:` lines
of thirteen examples (the calendar examples, `picard`, `globe-print`,
`selftest-speed-check`, `printer-timing`) report 100 ms; `FullSpeed:`
before the key that starts a run lasts 100 ms (`test_texas`,
`fast-grafik-3d-plot`, `switzerland`); `trace: off` and `switch:
PC-100C TRACE on` act while the program before them still runs
(`operror`, `test_cdebug`); `insert:` closes the card switch into a
running program (`globe-print`, `characters-fastmode`). The comparison
of C59-2 AC-11 does not see it, because it looks at the idle machine.
Found by seven tracers.
*Recommendation:* on a TI model a `key` step ends when the machine is
idle again after its last key (LIB-6 AC-36), as 2.3.0 does, with the
bound of LIB-6 AC-15: after one hour of emulated time the preset goes on
and a line of the log says so. LIB-5 AC-17, AC-20 and LIB-6 EC-4 are
worded so. What follows from it: while the program a preset started
runs, the load is still an operation, so R/S pressed by the user is
dropped (ledger row 59-5); Reset ends it.

**B-2 — A pause that lands inside the hold of a key.**
`test_cdebug.ti59` arms `pause-on-start: program` and presses `SBR`,
`=`. The ROM appears to take a key while it is held
(`rom/TI59-commented.asm:2100-2116`), so the program starts and the
pause lands inside the 100 ms hold; a run has to confirm it (C-1). Today the hold is wall time: the key comes up on the
frozen machine and the wait after it ends at once
(`Shared/KeySequencer.cpp:119-126, 152-167`). Specs: the hold is
emulated time (LIB-5 AC-1, AC-20); "a step that needs emulated time to
pass waits until the machine runs again", `key` among them (LIB-6
AC-19). The `key` step would never end, and the `wait: 1.5s` that was
fixed on 2026-11-11 would never be reached: the same preset would hang
one step earlier.
*Recommendation:* as for `wait:` with a duration: while the machine is
paused, what is left of a hold passes in the host's time, the key comes
up, and the step ends with the machine still paused; in the harness at
once. A key that is not yet down waits for the machine to run, as now.

**B-3 — Key readiness on a TI while a program runs.**
Today a press waits for the narrow scan loop (`0x063C-0x0660` on the
TI-59, `Shared/MachineModelInfo.hpp:59-68`), never for the moments in
which a running program samples R/S. C59-1 AC-18 does not name the
loops ("there are several"); LIB-6 AC-36 and OQ-1 assume that readiness
can show while a program runs. If it did, LIB-5 AC-20 would press `2nd`
into the running diagnostic of `test_cdebug`.
*Recommendation:* C59-1 AC-18 says that key readiness is the steady
state of today's scan loop, with a printer its confirmation scan, and
not the sampling of a running program. The first step on LIB-6 AC-36
then only confirms the 100 ms.

**B-4 — `key` and `type` on the Sharp machines.**
Today (`Core/PC1600/PC1600BasicTyper.cpp:150-204, 243-289`,
`Core/PC1600/PC1600PresetLoader.cpp:143-157`,
`Core/PC1500/PC1500BasicTyper.cpp:16-19, 79-193`): a `type` line starts
at once on the PC-1500; on the PC-1600 with a plotter it starts when the
ROM is in the command loop or reads the key matrix, held 15 frames, and
after 15 s all the same. A `key` step is 4 frames down and 4 up, on the
PC-1600 after the same wait. After ENTER the PC-1500 waits for its
settle test, at most 12 s, the PC-1600 until BUSY is off, at most 1 s,
and 12 frames more. After the build the PC-1600 also waits until it is
steady in the command loop, at most 20 s. Idle is the command loop for
20 frames in a row. Specs: "a `type` starts its line only when the
machine reports [prompt readiness]" (LIB-5 AC-14), with no bound (EC-3);
prompt readiness is the command loop (AC-35), which an `INPUT`, the
RESERVE mode and possibly MODE 1 are outside of; a `key` step is the
2-frame minimum (AC-28); the wait after ENTER is "prompt readiness"
(AC-17); nothing is done after the build of a PC-1600 (LIB-6 AC-46).
Under the specs the three bundled examples that answer an `INPUT`
(`globus`, `biorhythmus_1600`, `ascii`) hang at their first answer, and
so may the PC-1600 startup preset, which types in RESERVE mode, and
with it the start of the product; every key step and every load ends
at another tick than today, so the comparison of C1600-4 AC-16 "at the
end of the load" differs without a defect (`hanoi`, `ce150_demo`).
Found by all six Sharp tracers.
*Recommendation:* in a preset and in the harness, `key` and `type` on
the Sharp models do what they do today, tick for tick, and the criteria
say what that is: LIB-5 AC-14 holds "where the typing profile says so";
LIB-5 AC-28 gets the start of a line and of a key step (PC-1500: at
once; PC-1600: key readiness of C1600-1 AC-21 for 15 frames, at most
15 s, with a plotter), the 4 + 4 frames of a key step, and the two
waits after ENTER as they are; LIB-6 AC-46 gets the PC-1600's wait
after the build; LIB-6 AC-45 gets the 20 frames. The reference runs of
C1600-4 AC-16 then compare like with like.

#### Media and cards

**B-5 — A bundled template that a preset names.**
Today a template a preset names stays a template in its place: what the
machine writes lives there, no file is made, and Name & Save keeps it
under a name of the user's (`Shared/ModuleBay.cpp:98-112`,
`Shared/ModuleBay.hpp:81-86`); the user guide tells the user to do so
(`docs/User-Guide.md:1013-1019`). Specs: LIB-7 AC-25, "a medium by
file, or a template by name": the first write makes an instance with
the template's name, replacing one of that name; AC-5: a lookup by name
still finds the template; AC-16, EC-9: no Name & Save for an instance.
Under the specs: every preset that names a RAM module (`slot-1:
CE-1600M`) leaves an instance "CE-1600M" in the save folder, which the
next such preset replaces; the PC-1600 startup preset does so at every
start; the setup presets (`examples/setup/`), whose purpose is a
prepared module to keep, leave it under the bundled name, where no
preset finds it and Name & Save cannot rename it; the pickers show two
entries of one name. ADR 0019 itself speaks only of "a medium a preset
names by path".
*Recommendation:* a template that a preset names by name behaves like
one chosen in a picker (LIB-7 AC-15): what is written lives in the
place, and Name & Save is offered, as today. LIB-7 AC-25 holds for a
medium named by file and for a card name on the TI-59. The exception in
AC-5 goes, row 1600-7 says "by file" (A-8), and row 1600-13 is not
touched.

**B-6 — What shows above the keys when a preset passes a card.**
Today (`Shared/SessionController.cpp:494-536`,
`tests/shared_cooperative_tests.cpp:450-461, 791-812`): a card file a
preset feeds is fitted and stays fitted; a `mem:` card is not fitted,
but its cue card shows from then on (`globe-print` shows "part 2",
"part 3"); the preset's own cue card goes onto every card file the
preset writes. Specs: `- insert:` with a card "inserts it without
fitting it" (C59-7 AC-17, `presets.md` PR-8); the topic shows "the
fitted cue card" (AC-22); a fresh card gets its name as title (AC-15);
nothing says which cue card a card written by a preset carries. No
ledger row for any of the three.
*Recommendation:* keep today's three: a card that `insert` or `arm`
names by name or by file is fitted when it goes in; a `mem:` card with
a `cue-card:` fits that cue card, as a `cue-card:` section would; a
fresh card a preset writes carries the cue card fitted at its first
write, else its name as title. C59-7 AC-17, AC-20 and PR-8 are worded
so. If not: three ledger rows.

**B-7 — A blank `mem:` card.**
`pdcm-card-tricks.ti59` arms `mem:D-1`, `mem:D0`, `mem:D1`, `mem:D2`
and `mem:D-2`, which have no `CARD:` block: blank cards that the
program writes and reads back (`reference/StateFileFormat.md:418`).
Specs: LIB-7 AC-23 refuses "a `mem:` name without its section"; a card
section needs `bank:` (C59-7 AC-18).
*Recommendation:* a `medium:` section with `kind: card` and only its
`name:` is a blank card; `bank:` is required only with `steps:` or
`registers:`. The upgrade writes such a section for each blank card.

**B-8 — Small rules for media in presets.**
(a) A side that is left out: today side A of a disk, side 1 of a card
(`Core/Preset/PresetFile.cpp:689-690`, `Shared/StateFile.cpp:166-178`);
LIB-7 AC-21 and C59-7 AC-17 say it for `insert:` on a card only.
(b) Today's `floppy: name,B` has no upgrade row. (c) Several armed
cards: today a queue, each used once, in order
(`Shared/EmulatorSession.cpp:296-322`); LIB-7 AC-26 does not say.
(d) "The log says which file was written" (LIB-7 AC-25): a path would
break LIB-11 AC-8 and the equal logs of LIB-6 AC-13. (e) A 2.3.0 card
`PDCM-1.U59` in the save folder when a preset names `PDCM-1`: refused
when read (LIB-7 AC-23) or a numbered file (AC-17, last row); and
whether `.U59` is `.u59`.
*Recommendation:* (a) without a side, the first (A, 1), for every key
and step, side names in any case; (b) `,B` becomes the word `B`;
(c) armed media queue and pass in the order armed, each once; (d) the
log names the file by its name in the save folder; (e) refused when
read, with the hint, as AC-23 says, and the suffix is read in any case.

**B-9 — A preset the card reader ends after 30 minutes.**
C59-7 AC-17: "the preset ends there with a line in the log". LIB-6
AC-17 gives a failing step a reply and a notice; LIB-6 AC-43 loads
every example "to its end without a failure".
*Recommendation:* it fails there as LIB-6 AC-17 says, with the card
reader's reason.

#### The proof that the upgrade changed nothing

**B-10 — The reference runs of the TI examples (C59-2 AC-11).**
AC-11 records them "with the tools of 2.3.0 (`tools/agentic/ti59file.py`,
`tools/headless/`)". That tool has a reader of its own: always a TI-59
with the Master Library and the printer (`ti59file.py:163`); every
directive but `PrinterTrace:` dropped (`:76-91`); no reloads, no cards
(`tools/agentic/README.md:250-255`); the partition set by keys; `002
11.` read as 11 and a register cut to 13 digits where 2.3.0 rounds; it
fails on a file without keys. `tools/headless/` loads no preset. So
the named tools cannot record the 17 examples without keys, the 8 for a
TI-58 or TI-58C, the 4 with cards and 2 with reloads. Also: a preset
that ends paused (`test_58_glyphs`, `test_cdebug`) is never "idle" and
never passes "an hour"; a program that does not stop (`life.ti59`) has
no moment that is the same in both runs; the cards a preset writes
(`pdcm-make-cards`, whose only product they are) are not compared.
*Recommendation:* a reference run is recorded by a driver on 2.3.0's
own session code (`SessionController` in its cooperative mode with a
clock the driver supplies, as `tests/shared_cooperative_tests.cpp:478-484`
does), on the model of AC-10. It ends when the script has ended and the
machine is idle, or is paused, or one hour of emulated time after the
last key went down. Compared are also the banks and cue cards of every
card the preset wrote.

**B-11 — The comparison of the Sharp examples (C1600-4 AC-16).**
(a) It compares display, BASIC area, machine code and paper, not the
media; for `make_diskworks_media` and the setup presets the module or
the disk is the whole result. (b) A reference run of
`make_diskworks_media` rewrites two tracked files
(`- saveas: template …:file:`), and `DiskWorks` mounts the tracked
folder `examples/dwx/S3`. (c) Three examples (`DiskWorks`,
`make_diskworks_media`, `pc1500-maxed-out`), and four on the TI side,
are upgraded only "when LIB-7 and LIB-10 are built", while LIB-6 AC-43
and AC-51 check every preset as soon as the loader is there.
*Recommendation:* (a) the content of every medium in a place at the end
of the load, and of every file a `saveas` wrote, without its time
stamps, is compared too; (b) both runs are made in a copy of
`examples/`; (c) the presets that wait for LIB-7, LIB-10 or C59-7 are
left out of the checks of LIB-6 AC-43 and AC-51 until then, by name.

**B-12 — `debug/example-brief.ti59:16`: `002  11. A`.**
2.3.0 passes `11.` over, so step 002 stays 00 and the program has no
label A (`Shared/StateFile.cpp:35-43`; `tests/golden/formats.txt:16574-16578`).
C59-2 AC-3 refuses the line. The author meant 11.
*Recommendation:* the example is corrected to `002 11`, and its
reference run is recorded from the corrected 2.3.0 file.

**B-13 — Upgrade rules that C59-2 AC-10 lacks.**
(a) Tabs (`ram_test.ti59`, 152 lines): LIB-6 refuses a tab.
(b) `IdAlign:` (`debug/test-solidstate.ti59:7`), read and not used
today (`Shared/CueCard.cpp:204-205`), no key of AC-5. (c) A label key
without a value (`D:` in `debug/test-cuecard.ti59`). (d) `21` alone on a
line and its key on the next (`test_texas`, `fast-grafik-3d-plot`,
`calendar-08`, `operror`): "a second function by its name" would give
`- key: 2nd`, `- key: Op`, and `Op` presses `2nd` again (LIB-5 AC-26).
(e) `Wait: 1.5s`: whether a duration takes a fraction (LIB-6 AC-1:
"decimal digits"). (f) A directive that is commented out for the user
to switch on (`#Trace: printhack.bin`, `historic/printer-quirks.ti59:46`).
(g) Where the `medium:` sections of a former `CARD:` block go, and that
LIB-6 AC-15 and C59-2 AC-7 ("sections after a step list … are carried
out when … idle") do not mean them (LIB-7 AC-22).
*Recommendation:* (a) spaces; (b) the line is dropped; (c) an empty
value is an empty label; (d) a second function gets its name only where
`21` and its key stand on one 2.3.0 line; a line with `21` alone stays
`- key: 2nd`, the next key keeps its key ID, and LIB-5 AC-26 says
"within one step"; (e) no fraction: `1500ms`; (f) it is written in
version 2 (`# - trace: printhack.bin`); (g) at the end of the file, and
AC-15 and AC-7 except them by name.

**B-14 — Small points of the grammar and of the TI blocks.**
(a) Five examples end without a line end; LIB-6 AC-1 says "lines end
with LF or CRLF". (b) Leading spaces of a line in `steps: |`.
(c) `registers:` for a register beyond the partition in force
(`calendar-04`, `calendar-08`): written into the program area today
(`reference/StateFileFormat.md:109-112`); C59-2 EC-1 covers steps only.
(d) `trace: off` while no trace runs (`historic/printer-quirks.ti59:59`).
(e) Cue-card values: an unlisted value keeps the default today,
`Template` is matched in exact case and the others in any, and `A′`
(U+2032) is a label (`Shared/CueCard.cpp:111-134, 187-224`). (f) Card
file: values in any case today (`Shared/CardFile.cpp:104, 236`); a
card's cue card is always a MagnetCard, and `Banks` is dropped
(`:198-201, 296-300`), while C59-7 AC-2 allows every key of C59-2 AC-5.
*Recommendation:* (a) the last line may end without one; (b) not read;
(c) written all the same, as today (EC-1); (d) nothing happens; (e) an
unlisted value refuses, values are read in any case, `A′` to `E′` stay;
(f) values in any case; in a card file `Template` can only be
`MagnetCard`, and `Banks` refuses the file.

**B-15 — The preset fixtures and the screentest presets.**
The three files of `tests/fixtures/statefiles/` exist for the `STATE`
golden records, which "are not carried over" (LIB-6, C59-2, Technical
Requirements); their lines are today's lenient cases and have no
mapping. C59-2 AC-10 speaks of "those … that the new tests still load".
The five presets of `examples/screentest/` are among the 57, but only
the UI tests use them.
*Recommendation:* the three fixtures go with the `STATE` records, and
each refusal of C59-2 AC-2 to AC-5 is tested by a text with that one
error; the 57 of AC-10, the screentest presets among them, are all
checked and loaded by LIB-6 AC-43.

#### Debugger, traces, overlays

**B-16 — The TRACE switch of a PC-100C that a preset builds.**
Today TRACE survives a load; `PRINTER: on` in the file sets it off
(`Core/TI59Machine.cpp:39-45, 215-221`, `Shared/SessionController.cpp:657`).
C59-6 AC-6 speaks of reset, power cycle, cold boot, detach; LIB-18 AC-2
of a new machine does not name switches.
*Recommendation:* a machine that is built has every switch of its
peripherals in its first position (TRACE off).

**B-17 — Traces.**
(a) Today a preset's trace holds instructions and gaps; key and card
records go to the button's trace only (`Shared/EmulatorSession.cpp:68-77,
243-252`). LIB-10 makes them one trace (AC-22, AC-23, AC-33). (b) The
harness has no trace folder: LIB-9 AC-17 names it per frontend, and a
one-shot run removes its save folder (LIB-11 AC-8), so `operror.ti59`
fails its load in the test suite or its trace is gone before
`tools/read_trace.py` can read it; `dev/presets/trace_demo.pc1500a`
tells its reader to do just that.
*Recommendation:* (a) every trace holds key and card records, added to
row 59-19; (b) the harness takes a trace folder as an option; without
it, traces go to the run's save folder.

**B-18 — ASM overlays.**
(a) LIB-10 AC-29 lets a pause land "at the next program step boundary,
or, while none comes because the calculator waits for a key, after
50000 instructions". In an overlay neither holds, so FREEZE, `step:
program` and a second Run would wait without end; today the cap holds
whatever the processor does (`Core/TI59Machine.cpp:287-297`). (b) Run
enters at 1800 with COND set; without it `DPT.asm` runs into dead
address space (`Core/TMC0501.cpp:446-450`); C59-4 AC-17 is silent.
(c) Whether an overlay still "runs" after a model switch or a preset:
today yes (`Shared/SessionController.cpp:93-135`). (d) C59-4 AC-23
keeps "what the tests check about the overlay's reading", and that test
checks that a run of two digits is passed over and a value is cut to 13
bits (`tests/shared_debug_tests.cpp:249-251`), both of which AC-15
refuses; row 59-1 names only the second. (e) What a word or another
character after `HEX:` does.
*Recommendation:* (a) the cap holds whatever the processor does;
(b) AC-17 says "with COND set", and AC-20 runs `DPT.asm`; (c) a new
machine ends the run, and the instructions are put over the new ROM;
(d) AC-23 excepts both, and row 59-1 names the short run; (e) it
refuses the file.

**B-19 — The DAP server and the `debug:` block.**
(a) "A Reset or a Cold boot follows that is paused before its first
instruction" (C1600-3 AC-6, AC-17) meets "Reset … is over when the
machine reports key readiness" (LIB-18 AC-13), "an operation runs, then
breakpoints … do not pause" (LIB-10 AC-13) and "the server holds
[messages] back" during an operation (C1600-3 AC-8): the reset never
ends, or breakpoints in the boot never fire. Today the debugger's reset
is the reset alone, and the boot runs in normal frames
(`Qt6/app/debug/DebugController.cpp:262-277`). (b) `boot: debug` forces
a Cold boot today and overrides `reset:` (`DapSession.cpp:430-444`);
LIB-10 AC-15 ("a new machine … is not paused") and AC-46 disagree.
(c) Between Build & Load and `configurationDone`: paused (C1600-3
AC-4) or running (AC-7). (d) Whether the files a `debug:` block names
must exist when the preset is read (LIB-6 AC-7); a project from a
template has none before its first build. (e) Build & Load without a
program, which the two ROM templates use, has no criterion.
(f) `bus-rom:` given twice: "each header key may appear once" (LIB-6
AC-1), "a key that repeats adds to the list" (C1600-4 AC-9), "the last
one only" (AC-15).
*Recommendation:* (a) a Reset or Cold boot with a pause at its start is
over at that pause, and the boot after it is no operation; (b) `boot:
debug` takes no `reset:`, and AC-15 holds "unless AC-46 is armed";
(c) paused until `configurationDone`; (d) not read with the preset;
(e) the set-up again, then resume, or paused with reason entry under
`stopOnEntry`; (f) AC-9 holds, as today, and LIB-6 AC-1 and the table
of AC-15 except `bus-rom:`.

#### Harness, tests, tools

**B-20 — The host's date and time in the harness.**
A build and `syncclock` hand a real-time clock the host's time (LIB-2
AC-23, LIB-6 AC-27), and the log gets it; LIB-11 AC-6 asks for equal
replies from equal runs. Today's test pins the clock
(`Core/tests/job_tests.cpp:228-235`).
*Recommendation:* the harness takes the host date and time as an option
and uses a fixed instant without it.

**B-21 — The UI tests of the Swift frontend.**
Today: `--UITesting` shows an element `keystroke-playback-status`; a
test taps Preset, picks a file and waits for `playing`, then `idle`
(`AppHelpers.swift:70-89`); settings come as launch arguments into
UserDefaults (`ScreenshotTests.swift:27-41`); a hidden button presses
R/S into a running overlay. Specs: C59-8 AC-21 removes the three aids,
"the tests reach the session through the attached harness instead";
LIB-9 AC-21 reads nothing from UserDefaults. Open: (a) who speaks to
the attached harness from an XCUITest, and what a test waits on after a
load it started with a tap; a load at full speed can begin and end
between two polls. (b) How a test launch gets its settings (classic
segments, the debug page in portrait, the model). (c) `key R/S` waits
for key readiness, which an overlay never reports (LIB-5 AC-3, EC-3).
(d) The `Wait: 1.5s` lines of `test_diag` and `test_cdebug` exist only
so that the poll sees `playing`.
*Recommendation:* (a) settled by the spike on attach in the simulator
(first steps) and written into C59-8 then; the signal is the end of the
operation on the busy topic, received as a stream; (b) a development
build takes a launch argument that names a settings file for that
launch; (c) C59-8 AC-21 names `key-down` and `key-up`; (d) the two
lines are dropped in the upgrade.

**B-22 — The screenshot scenarios of Calc-U-1600.**
The runner is the Qt6 app (`--shots`): it stops the pacer for the whole
run, loads a preset to its end in one call, and advances emulated time
itself for `run:`, `key:` and `type:` (`ShotRunner.cpp:115, 233-271`).
Specs: operations run in parts by host calls, none blocks the GUI
thread (LIB-3 AC-3, AC-14, AC-35); a frontend has no command that lets
emulated time pass; `--shots` is "not a harness function" (LIB-11
AC-18) and the pictures are "made as today" (LIB-16 AC-12). The runner
as it is would wait without end for the start. Two scenarios click
`Edit > Settings…`, which moves to File (LIB-14 AC-34); the scenarios
find widgets by object names no criterion keeps.
*Recommendation:* the Qt6 frontend gets harness attach (`harness.md`
H-4, "when a need shows"), and the runner becomes its client: `preset`,
`wait`, `key`, `type` in place of its own stepping. LIB-14 owns the
scenarios and the object names they use, and they are upgraded with the
shell.

**B-23 — Two GUI scenarios the specs cite.**
LIB-6 AC-42: "a bundled example is loaded and the display shows its
result"; the cited Web test checks the cue card of `sum.ti59`, and the
Swift test only that playback began; four of the five bundled presets
of that group have no keys. `Web/tests/play-calculator.spec.js:471-491`
loads a bundled preset offline; LIB-15 AC-23 keeps "runs every model".
*Recommendation:* AC-42 names `sum.ti59` and its cue card; LIB-15 AC-23
adds "and loads a bundled example".

**B-24 — The VS Code templates.**
C1600-4 AC-15: the templates "are upgraded by hand and are not
checked"; C1600-3 AC-19 says the scenarios check them, and one of ten
uses a template, skipping without `sdaslh5801` (`tools/dap_smoke.py:370-373`).
*Recommendation:* a test fills each template as `scaffold.js:49-58`
does and runs `check-preset` on it; the RENUM scenario uses a built ROM
kept as a fixture.

**B-25 — Tools around the examples.**
(a) `tools/agentic/calendar/fastcal.py` generates `calendar-09` in the
2.3.0 format, and `years.py` finds the year by matrix code 11
(`fastcal.py:163-203`, `years.py:21-25`). (b) `dev/tape-matrix/run.py`
writes version 1 presets (`:44, 91-96`); `dev/README.md:20-22` calls it
a one-time check. (c) The CE-158's peer is set before the preset builds
the machine today (`tools/pc1500_cli.cpp:185-187`); `pc1600 serial` is
refused without a CE-158 (C1600-5 EC-3), and `preset` replies after the
program has sent its first line. (d) `tools/pc1600_plotter_probe`
prints pen and motor events; no command gives them.
*Recommendation:* (a) the calendar scripts read and write version 2
with the upgrade; (b) kept as the record of a finished check, not
upgraded; (c) a `pc1600 serial` command given before a machine has the
CE-158 holds for the next one built; (d) the probe keeps what `paper
marks` and the log give.

**B-26 — Texts next to the examples.**
C1600-4 AC-15: "in each, only this changes". Comments in the presets
and the READMEs under `examples/` name keys that are gone
(`slot-N-file:`, `floppy-file:`) and tools that are gone
(`examples/setup/README.md:18-21`, `ce158_demo.pc1500a:7-8`);
`examples/assembly/README.md:8-22` describes the 2.3.0 overlay format.
No spec owns them.
*Recommendation:* the upgrade of a file corrects its comments, and the
READMEs under `examples/` are upgraded with them (C59-2 AC-10, C59-4
AC-20, C1600-4 AC-15).

#### Lists and bundles

**B-27 — The preset lists.**
(a) Calc-U-1600 bundles the whole folder of each listed preset
(`examples/bundled.yaml:9-10`, `Web/build.js:77-94`), so nine unlisted
examples are bundled and start with `?preset=`; C1600-4 AC-17: "each
preset with the files it names". (b) An entry of Examples is labelled
by its description today (`Shared/PresetCatalog.hpp:29`), which two Web
tests click; LIB-14 AC-1 gives no label. (c) What the Web `preset` prop
takes (LIB-15 AC-16). (d) C59-2 AC-13: "today's order" and three
groups; `fastmode-sst` is tenth today and in the second group.
*Recommendation:* (a) the whole folders, as today; (b) the description,
else the name; (c) a reference, or a path inside the bundled presets;
(d) today's order within each group.

**B-28 — "Empty" constant memory of a TI-58C a preset built.**
C59-2 EC-3, AC-1 and C59-3 AC-8 say empty; C59-1 AC-6 says a cold boot
is "as the ROM's start-up leaves a machine whose memory was all zero",
and that start-up writes its signature into H00
(`rom/TI58C-ROM-Guide.md:165-176`). Today a load keeps these registers
for that reason (`Shared/StateApplier.cpp:66-73`).
*Recommendation:* the three criteria say "as the ROM's start-up leaves
it"; first step C-6 confirms it.

**B-29 — C1600-4 AC-4: "the preset has to put the machine into program
mode before (`- key: mode`)".** The RENUM template types its program
without a `mode` and presses `mode` afterwards to go to RUN, so the
PC-1500A is in PRO after its cold start; following AC-4 would switch it
out.
*Recommendation:* AC-4 says "unless the model's start leaves it there";
first step C-7 confirms which models.

### 5.4 Questions the formats raise (2026-11-13)

Where `docs/formats/` reverses a decision recorded with "user" before
2026-11-12, or is silent where a spec needs an answer. The reversals are
taken as the user's newer decision in LIB-6, C59-2 and C1600-4; the
other specs are aligned once these are answered.

| # | Question | The format | Earlier decision |
|---|---|---|---|
| F-1 | Who writes a medium's file. **Ruled 2026-11-13 (user): the file decides; recommendation followed; applied in LIB-7, ADR 0019, `contracts/media.md`, ledger** | Decided by the file: no `template` key, an instance saved in place; a template is never written, "the product makes an instance first or refuses the write" (`media.md`) | ADR 0019 as amended, `media.md` MD-1, LIB-7 AC-25: a medium a preset names by path is never written; its first write makes an instance |
| F-2 | The version of a file that is saved again. **Ruled 2026-11-13 (user): the README sentence was wrong. A version changes only when the requirements change; after the migration release a file of the previous version is silently upgraded in place; the migration release reads only the current version. Applied in `docs/formats/README.md` §2 and ADR 0037** | "A saved instance keeps the `format-version` of the file it was saved from" (`README.md` §2) | ADR 0020, 0037: written in the current version |
| F-3 | Loading a preset while one runs. **Ruled 2026-11-13 (user): the old rule; only Reset, Cold boot and a model switch end a running preset, hung or not; everything else is refused. Corrected in `preset.md` §6.1 and LIB-6; nothing else changes** | It ends the running preset, as anything that resets "directly or indirectly" (`preset.md` §6.1); applied in LIB-6 AC-12 | `frontend.md` Q-1 (batch 4): only Reset, Cold boot, a model switch; LIB-3 AC-16, LIB-4, LIB-14 AC-16, LIB-18 AC-16. Also open: which of variant switch, attach, detach is "indirectly" |
| F-4 | Debugger steps on Calc-U-1600. **Ruled 2026-11-13 (user): follow the format, Calc-U-59 only for now. Applied in LIB-10 AC-19, AC-40, `contracts/debugger.md`** | [undefined] on the Sharp models (§5.6) | PR-7, the user's addition: in both products (LIB-10 AC-19, AC-40) |
| F-5 | The TI key names. **Ruled 2026-11-13 (user): the format's names only. Applied in C59-1 AC-9, LIB-5 AC-26, C59-9 AC-11** | §8.1: printer names and their spellings (`x²`, `x^2`, `STF`, `2nd CLR`); `x2`, `2ndCLR`, `StFlg` are not in it | LIB-5 AC-26 and OQ-7, C59-1 AC-9 (2026-11-11), C59-9 OQ-1 |
| F-6 | `cut-paper:`. **Ruled 2026-11-13 (user): no value; cuts the paper of the printer plugged in, as the GUI; each printer keeps its paper; a session step, so the harness does it too. Applied in `preset.md` §5.5, LIB-8 AC-22, AC-23, EC-2, LIB-6 AC-63** | No value; without a printer no effect; a frontend step, which the harness passes over (§5.7) | LIB-8 AC-22 (batch 6): an owner's name, every paper |
| F-7 | Tapes. **Ruled 2026-11-13 (user): keep with the format; one bundled template, the empty tape; a dropped WAV is loaded. Applied in LIB-7 AC-37, AC-42, AC-43, OQ-9, LIB-13 AC-19, `contracts/media.md` Q-1, ledger 1600-18** | A `.tape.yaml` definition with its `.wav`, templates, Name & Save, `position`, the key `tape:` (`media.md` §3) | `media.md` Q-1, LIB-7 AC-42: a tape is a WAV, no templates, no header key; today's WAVs in `Tapes/` (Briefing §1.2); where definitions live; `tape:` with a CE-150 on a PC-1600 (LIB-6 OQ-10) |
| F-8 | The TI-58C constant memory file. **Ruled 2026-11-13 (user): (B), a machine a preset built never writes the file. Applied in C59-3 AC-5, AC-6, AC-10, EC-7, `contracts/capabilities.md` C-3, ledger 59-12** | Written when the calculator is switched off or the product quits; a preset never writes it (`media.md` §4, S13) | `capabilities.md` C-3, C59-3 AC-5, AC-10: it always mirrors the machine, half a second after a change, and follows a cold boot |
| F-9 | Solid State ROM module files. **Ruled 2026-11-13 (user): as recommended, extended to Calc-U-1600's CE-502B. Applied in LIB-12 AC-12, AC-14, `contracts/roms.md` I-3, C59-6 AC-8, LIB-6 AC-35, OQ-8** | One `.rom.yaml` per module with ROM and cue cards; `slot-1: { file: }` allowed (`modules.md` §2) | `roms.md` I-1 (no ROM data in the source repository), LIB-12 AC-8, AC-9, C59-6 (ID only, 2026-11-11); LIB-6 OQ-8 |
| F-10 | The cue card at a cold boot. **Ruled 2026-11-13 (user): a preset's cold boot only. `cue-card.md` §2 worded so; LIB-18 AC-23 and row 59-17 stay** | "A cold boot (every preset starts with one): no card" (`cue-card.md` §2) | LIB-18 AC-23, row 59-17 (batch 7): the Cold boot command keeps the card. Read: the row means a preset's cold boot |
| F-11 | Where `screenshot:` writes. **Ruled 2026-11-13 (user): the earlier decision, the save folder; `preset.md` §5.4 corrected, LIB-6 OQ-6 closed** | The trace folder (§5.4) | LIB-9 AC-17, LIB-6 OQ-4: the save folder (LIB-6 OQ-6) |
| F-12 | `wait:` at full speed on an idle TI. **Ruled 2026-11-13 (user): ODD59-6 stays, a preset's `wait:` at full speed is not paced (LIB-17 AC-16); and the higher requirement that a full-speed preset's elapsed time is about that of a quick real user, on every model (LIB-6 AC-64)** | "As fast as the host allows" (§6.1) | ODD59-6 in LIB-17 AC-16 (batch 2): an idle TI at full speed runs at the pace of regular speed |
| F-13 | Case of names. **Ruled 2026-11-13 (user): case preserving and case-insensitive, as LIB-7 AC-3 and MD-3 decided; `docs/formats/README.md` §3 corrected** | "Compared as written", TI and Sharp product names excepted (`README.md` §3) | LIB-7 AC-3, MD-3 (batch 6): names that differ only in case are one name |
| F-14 | The module picker. **Ruled 2026-11-13 (user): `description` as the label, `menu-sort` order, as today. Applied in LIB-7 AC-8, C59-6 AC-8, OQ-4, C59-8 AC-6** | The picker shows `description`; `menu-sort` orders (`modules.md` §2.1) | C59-6 OQ-4 (the title for the picker), LIB-7 AC-8 (sorted by name), C59-8 AC-6 (no description on iOS) |
| F-15 | The harness and the waiting rules. **Decided 2026-11-13, left to the specification by the user: (b), commands reply when carried out, `wait` asks for idle. Applied in `contracts/harness.md` H-1, LIB-5 AC-17, LIB-11 AC-12, LIB-6 AC-34, OQ-7** | Silent; frontend steps are passed over | `harness.md` H-1, one vocabulary; LIB-6 OQ-7; whether `key`/`type` commands wait until idle |

Smaller points, each in the spec named: LIB-6 OQ-9 (`expect:` against
X); C59-2 OQ-5 (one-digit tokens, a line after `...`); C1600-4 OQ-5
(the `debug:` block); the constant memory file numbers R000 to R119
without H cells (C59-3 AC-4); a module program without a printed card
(C59-7 AC-22, a fallback card or the own card); an in-file card fitted
after a Reset feeds its defined content; no step inserts or ejects a
floppy disk after the build (LIB-7 EC-5, EC-6; harness commands only?);
what a write keeps in a card file (`description`, `saved`).

**Status of the smaller points:** C1600-4 OQ-5 **ruled 2026-11-13 (user), see B-19**; LIB-6 OQ-9 **ruled 2026-11-13 (user): full precision of X, 13 digits, not rounded. Applied in LIB-6 AC-40, OQ-9, `preset.md` §5.4**. LIB-7 OQ-9 and LIB-6 OQ-10 **ruled 2026-11-13 (user): a saved tape's WAV beside its definition, a hand-written one names it by any relative path; `position` reserved, 0; old WAVs as 1600-18; CE-150 tape on a PC-1600 not restricted. Applied in LIB-7 AC-42, `media.md` §3, LIB-6 OQ-10, C1600-1 AC-16, LIB-1 AC-39**.

**The conversion check** (`docs/research/preset-conversion-check.md`)
had four facts that the pins do not bear out (corrected in `602988b`): `test_diag.ti59` has no
pause-on-start and needs no `skip-wait:`; `switzerland` and
`stars-and-stripes` have no `ReportElapseTime:` and need
`let-it-finish:`, not a `timeout:`; `printer-quirks`, `texas-print` and
`screentest/test_texas` end with `FullSpeed:` and a key that starts a
print, and so need `let-it-finish:` too; the counts are 62 Calc-U-59 and
59 Calc-U-1600 files at the pins (this walkthrough says 60 and 59, the
check 61 and 58). And `pc1600-hanoi-run` has `- wait: 3` after
`type: RUN`, so under format 2 its screenshot shows the `INPUT` prompt
of `hanoi.bas`, not the title.

### 5.3 Class C: into the first steps

| # | What cannot be settled on paper | Showed on | First step |
|---|---|---|---|
| C-1 | Whether a pause-on-start lands inside the 100 ms hold of the key that starts the program, and whether a TI reports key readiness while a program runs | `screentest/test_cdebug.ti59` (B-2, B-3) | with the check of LIB-6 AC-36 |
| C-2 | Whether a PC-1600 in MODE 1, and in RESERVE mode, is ever in the command loop that is prompt readiness | `plotter/lissajou-ce150.pc1600`, `startup/default-pc1600.pc1600` (B-4) | with the reference runs |
| C-3 | How long the load of an auto-run program lasts that never returns to BASIC: up to the hour, with the keyboard dead | `machine-code/Calculat.pc1600` | with the reference runs |
| C-4 | Whether `DiskWorks` writes into the folder `examples/dwx/S3` | `dwx/DiskWorks.pc1600` (B-11) | with the reference runs |
| C-5 | Whether the start-up of a Sharp ROM writes into a battery-backed module | every preset that names one (only if B-5 keeps LIB-7 AC-25 for templates) | with the reference runs |
| C-6 | What a TI-58C holds in H00 to H03 after a cold boot, and whether a program a preset wrote outlives a Reset | `historic/repartition.ti58c` (B-28) | with the TI-58C check |
| C-7 | Which mode a PC-1500 and a PC-1500A are in after a cold boot | VS Code template `pc1500-bus-rom` (B-29) | with the golden runs |

Falling away with a ruling: the card switch closed into a running
program or into the reset routine (`globe-print`,
`characters-fastmode`) with B-1; whether 2 frames are enough for a
Sharp key, and the first step of a PC-1600 preset in the tail of the
start-up, with B-4.
