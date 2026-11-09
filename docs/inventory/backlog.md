# Backlog inventory

Every item of both products' `TODO.md` files and of `Calc-U-59/ideas/`,
each with a *suggested* target (Briefing §1.7, §7; `docs/Process.md`,
phase 1). Targets are suggestions; they are settled in phase 3. IDs are
stable: `B59-n` and `B1600-n`, in source order.

| Suggested target | Calc-U-59 | Calc-U-1600 | Total |
|---|---|---|---|
| design input | 14 | 15 | 29 |
| shared backlog | 10 | 3 | 13 |
| product backlog | 34 | 64 | 98 |
| done | 0 | 8 | 8 |
| dropped | 0 | 0 | 0 |
| **total** | 58 | 90 | 148 |

## Calc-U-59

Pinned: `v2.3.0` @ `231792c` (= `dev-2.3.0` @ `915a2b5`). Read on 2026-11-08.

Sources: `TODO.md` (35 lines) and the five documents in `ideas/` at the pin.
One row per bullet or sub-bullet that is its own idea; headings, status lines
and pure context (measurements, analysis, circuit detail) get no row. The
`Suggested target` column is a suggestion only; decisions are made in phase 3.
No item was found to be done at the pin; prerequisites that are done are named
in the Reason column.

| ID | Source | Item | Suggested target | Reason |
|---|---|---|---|---|
| B59-1 | `TODO.md:5` | Keep memory on model switch: RAM is lost on a switch (except TI-58C constant memory); keep memory per model | product backlog | Not done: a switch starts a new machine (`reference/SharedArchitecture.md:85-86`) |
| B59-2 | `TODO.md:6` | Emulation speed control: a multiplier to run faster or slower than real time | shared backlog | Pacing is a capability both products have |
| B59-3 | `TODO.md:7` | Rework display scanning: simple scanning on DIGIT 0 vs. scan per digit; open issues with the latter | design input | Modelling depth, Briefing §1.9 |
| B59-4 | `TODO.md:8` | Split TMC0501 and SCOM, to support out-of-phase DIGIT / KEY behaviour | design input | Needed for `Decoder.asm`, Briefing §1.9 |
| B59-5 | `TODO.md:11` | Enable/disable persistency for TI-58/59 (TI-58C always persistent) | product backlog | |
| B59-6 | `TODO.md:14` | Unified Mac input surface: whole window one keyboard surface, one meaning per key, no focus concept | design input | Shared keyboard layer and family coherence, Briefing §1.3, §1.5 |
| B59-7 | `TODO.md:15` | Keyboard support for the debug panels and the printer (resolved by the unified surface) | shared backlog | Calculator keyboard is done; panels are not |
| B59-8 | `TODO.md:16` | Keyboard support on iPadOS with a hardware keyboard | product backlog | Gated by `#if os(macOS)` in `App/Views/KeyboardView.swift` |
| B59-9 | `TODO.md:20` | CPU debugger breakpoints: a GUI (through `Shared/`), then conditional triggers (register content, IDLE-RUN change, …) | shared backlog | Core PC breakpoints exist (`Core/TI59Machine.hpp:103-105`); no GUI |
| B59-10 | `TODO.md:21` | CPU debugger: latch keyboard entries in single-step mode (`ideas/KeypressLatch.md`) | product backlog | ROM-specific key acceptance |
| B59-11 | `TODO.md:24` | Calculator debugger breakpoints (STO content, t content, PGM step) | product backlog | Nothing exists yet, not even PC-level (`TODO.md:24`) |
| B59-12 | `TODO.md:25` | Calculator debugger: latch keyboard entries in single-step mode | product backlog | Same mechanism as B59-10 |
| B59-13 | `TODO.md:28` | Event callbacks: push notifications for display and register changes instead of polling | design input | Seam between Shared and frontends, Briefing §1.4 |
| B59-14 | `TODO.md:29` | Card stacking in the GUI: let a user queue a stack of cards by hand | product backlog | Presets already queue cards (`ArmCard:`, `InsertCard:`) |
| B59-15 | `TODO.md:30` | Card sound: play a recorded card pass, emulator pass timed to match (`ideas/CardSound.md`) | product backlog | |
| B59-16 | `TODO.md:31` | REST API: scripted control of a running app | shared backlog | Prerequisites done: headless runs (`tools/headless/`, `tools/agentic/`) and GUI decoupling (`Shared/`) |
| B59-17 | `TODO.md:32` | Visually indicate when the calculator is frozen in a debugger (mainly iPhone) | shared backlog | Debugger GUI concept |
| B59-18 | `TODO.md:35` | Additional calculator models such as the TI-57 | design input | Briefing §2 names a possible TI-57 core; tests the core seam |
| B59-19 | `ideas/CardRecovery.md:3` | Card recovery rig: Pico capture tapped in parallel onto a working TI-59's card read, decoding the raw waveform to recover cards the TI-59 fails to read | product backlog | Hardware project outside the emulator |
| B59-20 | `ideas/CardRecovery.md:392` | Card-sense/position switches as a recording trigger | product backlog | Part of B59-19 |
| B59-21 | `ideas/CardRecovery.md:402` | Optional speed sensor (photo-interrupter or magnetic) for diagnostics and the flywheel clock | product backlog | Part of B59-19 |
| B59-22 | `ideas/CardRecovery.md:414` | Flywheel clock recovery that coasts through silent `00` gaps | product backlog | Part of B59-19 |
| B59-23 | `ideas/CardRecovery.md:429` | Adaptive per-track amplitude thresholding | product backlog | Part of B59-19 |
| B59-24 | `ideas/CardRecovery.md:434` | Azimuth correction by mechanical head adjustment with live waveform feedback | product backlog | Part of B59-19 |
| B59-25 | `ideas/CardRecovery.md:444` | Multi-pass, checksum-guided reconstruction of marginal nibbles | product backlog | Part of B59-19 |
| B59-26 | `ideas/CardRecovery.md:457` | Log raw waveforms; use `examples/diag.ti59`'s physical card as calibration reference | product backlog | Part of B59-19 |
| B59-27 | `ideas/CardRecovery.md:486` | Suggested order of attack (tap points, bench circuit, taps, captures, azimuth check, then decoder) | product backlog | Plan for B59-19 |
| B59-28 | `ideas/CardSound.md:3` | Play the sound of a real card pass while "C" shows; emulator pass lasts as long as the sound | product backlog | Same as B59-15 |
| B59-29 | `ideas/CardSound.md:10` | Record a cleaner take of a card pass and measure motor on, grab, motor off | product backlog | Step of B59-28 |
| B59-30 | `ideas/CardSound.md:16` | Card transport timing in the core: switch opens at once, reader busy for the free run, `CRD.RD`/`CRD.WR` hold about 67 instruction times per byte | design input | Core modelling depth (§1.9); a pass goes from ~0.47 s to ~1.4 s |
| B59-31 | `ideas/CardSound.md:25` | Expose the motor state through `Shared/`; the three GUIs only play the file | shared backlog | Machine sound playback is a frontend capability |
| B59-32 | `ideas/CardSound.md:33` | Tie the sound to the motor (first `CRD.RD`/`CRD.WR` to `CRD.OFF`), stop on R/S abort | product backlog | Detail of B59-28 |
| B59-33 | `ideas/CardSound.md:36` | Stop holding after a card's worth of bytes (refused card makes 261 `CRD.RD`) | product backlog | Detail of B59-30 |
| B59-34 | `ideas/CardSound.md:40` | Keep the realistic card timing even with the sound off | product backlog | Accuracy gain; slows card-using programs |
| B59-35 | `ideas/CardSound.md:43` | To check: full speed during a pass; whether tests or `ArmCard:`/`InsertCard:` depend on speed | product backlog | Open check for B59-30 |
| B59-36 | `ideas/CardSound.md:46` | To decide: include the sound of the card being pulled out by hand (leaning no) | product backlog | Open decision for B59-28 |
| B59-37 | `ideas/CardSound.md:131` | For exact timing: record a Write pass and a refused card; logic analyser on the printer connector (EXT, IRG, IDLE, clocks) | product backlog | Research task |
| B59-38 | `ideas/KeypressLatch.md:50` | Key latch in the core: hold a key pressed while frozen until the ROM's accept PC (0x0661 on TI-59/58), with a step-budget fallback | product backlog | Design proposal for B59-10/B59-12 |
| B59-39 | `ideas/KeypressLatch.md:94` | TI-58C: find the key-accept PC from a keypress trace | product backlog | Missing value for B59-38 |
| B59-40 | `ideas/KeypressLatch.md:124` | Refinement: replay a latched key as release, one empty sweep, press, when frozen on the idle-entry path | product backlog | Edge case of B59-38 |
| B59-41 | `ideas/KeypressLatch.md:142` | Optional UI badge "key latched" while armed | shared backlog | Debugger GUI indicator, like B59-17 |
| B59-42 | `ideas/Pico.md:15` | Port `Core/` to Raspberry Pi Pico (RP2040/RP2350) with a thin platform layer (pacing, GPIO keyboard scan, display driver) | product backlog | New platform; Briefing §2 adds no targets |
| B59-43 | `ideas/Pico.md:72` | Scan a donor TI-59 keyboard directly from the Pico at 3.3 V (pinout from the service manual) | product backlog | Part of B59-42 |
| B59-44 | `ideas/Pico.md:90` | Open question: reconcile the manual's 5 K-lines (`KD, KP, KQ, KS, KT`) with the core's 7-line `KN…KT` naming | product backlog | Research task |
| B59-45 | `ideas/Pico.md:95` | Drive the donor's LED display through a small transistor driver board; PIO for the multiplex | product backlog | Part of B59-42 |
| B59-46 | `ideas/Pico.md:144` | Card reader: keep the donor's TMC0594/LM324 daughtercard and speak the IRG/EXT/IDLE protocol to it | product backlog | Part of B59-42 |
| B59-47 | `ideas/Pico.md:153` | Fallback: raw analog decoding of the head signal on the Pico if the daughtercard is dead | product backlog | Part of B59-42 |
| B59-48 | `ideas/PresetConvergence.md:49` | Wait units: let Calc-U-1600 accept `2s` / `500ms` as Calc-U-59 does | design input | Preset engine and format contract; not done in Calc-U-1600 `v0.8.0` (`Core/Preset/PresetFile.cpp:229-235` takes seconds only) |
| B59-49 | `ideas/PresetConvergence.md:52` | Case: show the same lowercase directive spelling in both products' docs | design input | Glossary and help-site coherence, Briefing §1.5 |
| B59-50 | `ideas/PresetConvergence.md:55` | Format version: optional `FORMAT-VERSION: 2` in Calc-U-59 presets | design input | Briefing §1.2 versioning of own formats |
| B59-51 | `ideas/PresetConvergence.md:59` | One naming style (CamelCase vs kebab-case) for new directives; no aliases for existing ones | design input | Preset format contract |
| B59-52 | `ideas/PresetConvergence.md:81` | `Expect: <text>` directive in Calc-U-59 (display contains it), for ctest checks of `examples/` and agentic tools | design input | Exists in Calc-U-1600 (`- expect:`); preset engine and testing concept |
| B59-53 | `ideas/PresetConvergence.md:84` | `Screenshot:` plus a paper image (`SavePaper: plot.png`) in Calc-U-59 | design input | Exists in Calc-U-1600 (`- screenshot:`); preset engine and testing concept |
| B59-54 | `ideas/PresetConvergence.md:88` | A card file next to the preset (Calc-U-1600 `slot-1-file:` pattern) | design input | Media loading contract; sandbox limits noted in the source |
| B59-55 | `ideas/PresetConvergence.md:95` | Speed control and timing directives for Calc-U-1600 (`FullSpeed:`, `ZeroElapseTime:`, …) | shared backlog | Exists in Calc-U-59 only |
| B59-56 | `ideas/PresetConvergence.md:98` | Scriptable debugger GUI controls for Calc-U-1600 (`DebugPanel:`, `Freeze:`, `Step:`, `Resume:`) | shared backlog | Exists in Calc-U-59 only |
| B59-57 | `ideas/PresetConvergence.md:100` | Swapping media mid-script in Calc-U-1600, like `InsertCard:` | shared backlog | Exists in Calc-U-59 only |
| B59-58 | `ideas/PresetConvergence.md:104` | Recommendation: keep both preset syntaxes; apply points 1–4; add `Expect:` and a paper/screenshot directive to Calc-U-59 | design input | Input to the preset ADR (`docs/Process.md` phase 3) |

## Calc-U-1600

Source: `TODO.md` at `v0.8.0` @ `d976d3f` (= `dev-0.8.0` @ `70c3258`),
732 lines. Read on 2026-11-08. Unchanged in line numbers at the new pin
`b4ea8c8`; only the CE-161 note at `TODO.md:212-213` was reworded. One row per bullet or sub-bullet that is its
own idea; headings, context paragraphs and measurement/evidence bullets
("Ruled out", plan steps of one item) get no row. `Suggested target` is a
suggestion only; the decision is made when the merged backlog is written.

| ID | Source | Item | Suggested target | Reason |
|---|---|---|---|---|
| B1600-1 | `TODO.md:8` | PC-1600 BASIC runs ~0.4-0.65 % fast vs a real unit (MODE 0); cause open | product backlog | |
| B1600-2 | `TODO.md:37` | One wait per M1 cycle (BEEP pitch) | done | Listed under "Found and fixed" (f05e42e); `kM1WaitStates`, Decisions.md:67-70 |
| B1600-3 | `TODO.md:38` | Sub-CPU IOCS 25H fast-path probe | done | "Found and fixed" (6d6318a); Decisions.md:156-159 |
| B1600-4 | `TODO.md:39` | Sub-CPU response time 1.66 ms per command byte | done | "Found and fixed" (64538ae); `PC1600SubCpu::kResponseMicros` (TODO.md:619) |
| B1600-5 | `TODO.md:40` | LCD busy until the 4th LCD-clock edge | done | "Found and fixed" (c18411b); `kBusyClocks = 4`, Decisions.md:230 |
| B1600-6 | `TODO.md:41` | The ON key's live PB7 level | done | "Found and fixed" (5011e2b); Decisions.md:145-149 |
| B1600-7 | `TODO.md:42` | SRIRQ reports the 0.5 s event on every tick | done | "Found and fixed" in dev-0.6.0; Decisions.md:174-179 |
| B1600-8 | `TODO.md:78` | Next step for the speed residual: copy the FOR/NEXT arithmetic routine into RAM and time it | product backlog | |
| B1600-9 | `TODO.md:81` | Re-measure MODE 1 (LH-5803) speed since the host-pacing fix | product backlog | |
| B1600-10 | `TODO.md:83` | Empty ME0 slot reads FFH; real hardware returns the last bus byte. Decide whether ME0 follows ME1, record in Decisions.md | product backlog | |
| B1600-11 | `TODO.md:103` | 9400H trap decode (LHNMIO): one address vs. step into 9400H, port 30H b0 gate; hardware test | product backlog | |
| B1600-12 | `TODO.md:111` | LH5803 NMI edge vs. level: confirm where the level is gated | product backlog | |
| B1600-13 | `TODO.md:116` | Trap path timing: real cost of the display mirror, what port 30H b0 switches | product backlog | |
| B1600-14 | `TODO.md:121` | LH-5803 maskable interrupt (MI, FFF8H) not driven | product backlog | |
| B1600-15 | `TODO.md:127` | Port 34H (P_LHMSK2) not modelled; model with the MI path | product backlog | |
| B1600-16 | `TODO.md:130` | ME1 wait states (LHWAIT) not modelled; ME1 `**00H-**0FH` below 8000H never reaches cards | product backlog | |
| B1600-17 | `TODO.md:137` | IN0-IN7 bit order of the LH-5803 vs KIN: verify | product backlog | |
| B1600-18 | `TODO.md:145` | RS-232C / SIO connector mux: separate `calcu1600-sio.serial` | product backlog | |
| B1600-19 | `TODO.md:151` | Capture on-wire `SAVE"COM1:"` framing from a ROM trace; one live round-trip with SharpDataExchange over the PTY | product backlog | |
| B1600-20 | `TODO.md:156` | Localhost-socket serial transport (`SocketSerialLink`) for `OUTSTAT`/RTS end-to-end, `socat` bridging | design input | A second `SerialLink` transport; relevant to the device/port contract and to platforms without PTYs |
| B1600-21 | `TODO.md:164` | Open questions of the load/save matrix (`INPUT#-1` MODE 1 via CE-1600P; `SAVE`/`LOAD "CAS:"` MODE 1; CE-150/CE-158 `PRINT#`/`INPUT#` MODE 0) | product backlog | |
| B1600-22 | `TODO.md:172` | CE-158 on PC-1600 MODE 1: `SETDEV KI` + `INPUT` reads keyboard, not UART; trace and check on real unit | product backlog | |
| B1600-23 | `TODO.md:177` | Fast loader limit for CE-158 BASIC in MODE 1 (MEM + 1 vs MEM − 2): decide | product backlog | |
| B1600-24 | `TODO.md:194` | PC-1600 slot pin 2 (PVIN) reads low; drive it high | product backlog | |
| B1600-25 | `TODO.md:199` | 40-pin connector: model DME0 and ME1 cycles instead of the shortcut (6-step plan, card files, tests) | design input | Connector/pin model is shared by both machines; relevant to the device/port contract (Briefing §1.9) |
| B1600-26 | `TODO.md:261` | INHIBIT / INH on the PC-1600 (polarity, ROM banks, test card) | product backlog | |
| B1600-27 | `TODO.md:273` | Interrupt request lines: an interrupt-source interface like `InhibitSource`, per card, OR'd on the chain | design input | New connector-level interface; device/port contract |
| B1600-28 | `TODO.md:283` | WAIT lines (WEX, W1, WAIT); S6·WEX interrupt forwarding | product backlog | |
| B1600-29 | `TODO.md:289` | PC-1600 RSTE external reset line not modelled | product backlog | |
| B1600-30 | `TODO.md:291` | BFO / φOS (VCC-on, CPU-phase clock); BF0 gating of chip selects | product backlog | |
| B1600-31 | `TODO.md:300` | Does the CE-1600P I/O decode see LH5803 ME1 cycles? Probe IO7N on a real unit (decides CE-158 + CE-1600P) | product backlog | |
| B1600-32 | `TODO.md:309` | DME0 on Z-80 cycles, M1 on memory cycles, PT/PU/PVOUT on I/O cycles undocumented; logic-probe the 60-pin plug | product backlog | |
| B1600-33 | `TODO.md:313` | PU drive handover between SC7852 and LH5803 not measured | product backlog | |
| B1600-34 | `TODO.md:321` | PC-1600 MODE 1: `CLOAD` of PC-1500 tapes through the CE-1600P (find what drives 1AH b7) | product backlog | |
| B1600-35 | `TODO.md:334` | CE-150 on a PC-1600 (MODE 1): no tape path | product backlog | |
| B1600-36 | `TODO.md:341` | Real-hardware cross-check, remaining half: `CLOAD` emulator WAVs on real units | product backlog | |
| B1600-37 | `TODO.md:356` | GitHub Pages landing page `tinue.github.io/Calc-U-1600` with "PC-1500 / PC-1500A / PC-1600 emulator" title, screenshots, downloads; repo homepage field | shared backlog | Overlaps with calc-u.org and the Calc-U-1600 help site (Briefing §4.3); placement is a site decision |
| B1600-38 | `TODO.md:360` | Register the site with Google Search Console and Bing Webmaster Tools | shared backlog | Site-level task for calc-u.org |
| B1600-39 | `TODO.md:362` | Social preview image (repo settings) | product backlog | |
| B1600-40 | `TODO.md:366` | Announcement post with screenshots (HP Museum, VCFed, Reddit, Show HN) at 1.0 | product backlog | |
| B1600-41 | `TODO.md:369` | Emulator lists (Emulation General Wiki, awesome lists) | product backlog | |
| B1600-42 | `TODO.md:371` | Wikipedia PC-1500 / PC-1600 articles: propose on talk page | product backlog | |
| B1600-43 | `TODO.md:373` | Ask Sharp fan/collector sites to link it | product backlog | |
| B1600-44 | `TODO.md:374` | Package managers: Homebrew cask, Flathub or AUR, winget | shared backlog | Packaging channel applies to every desktop product |
| B1600-45 | `TODO.md:375` | Short YouTube demo (e.g. CE-1600P plotting) | product backlog | |
| B1600-46 | `TODO.md:384` | Web play page on calc-u.org/1600 wiring `Calc-U-1600-web.zip` into calc-u-site like `59/`; replaces tinue.github.io/1600 | design input | In scope per Briefing §4.3 (Calc-U-1600 help site with web client) |
| B1600-47 | `TODO.md:387` | User-guide section for the web, and a parity table (Calc-U-59 style) | design input | Help-site coherence (Briefing §1.5, §4.3) |
| B1600-48 | `TODO.md:389` | Check the web client on a real iPad (touch, two-finger SHIFT, install, persistence, audio) | product backlog | |
| B1600-49 | `TODO.md:391` | Move the DAP debugger to Shared (needs a vendored JSON value type); `DapServer` stays per frontend | design input | Debugger contract (Process phase 4); Briefing §1.3 mentions the DAP debugger |
| B1600-50 | `TODO.md:395` | CLIs on Shared: run presets through `AppSession` with a headless Platform; drop duplicated `drainWav` hook | design input | Headless harnesses must keep working (Briefing §8); frontend platform interface |
| B1600-51 | `TODO.md:398` | `tools/build_*.sh` and the probes still list Core sources by hand | design input | Repository layout / CMake structure ADR (Process phase 3) |
| B1600-52 | `TODO.md:403` | LCD text: the kana set (PC-1500 second charset, PC-1600 MODE 1); check MODE 1 text parsing | product backlog | |
| B1600-53 | `TODO.md:413` | Sub-CPU F-pin tones (key click, `ALARM$`, wake-up beep, hour signal): measure, then drive `PiezoSampler` | product backlog | |
| B1600-54 | `TODO.md:423` | Sub-CPU commands still unnamed (IOCS 0CH-0FH, 1BH, 1FH, 26H, 1CH/1DH, LH-5803 DCH): real-unit tests | product backlog | |
| B1600-55 | `TODO.md:430` | CE-158 together with the CE-1600P (blocked on IO7N) | product backlog | |
| B1600-56 | `TODO.md:436` | View a memory module's or diskette's directory and files in the INSPECTOR (`sde_disk_list`/`sde_disk_get`; own RAM-disk decoder) | product backlog | |
| B1600-57 | `TODO.md:446` | Debugger manual checks in VS Code (disassembly view, Banks scope, dynamic configs, Create Debug Project, Build & Load, RENUM template, checklist) | product backlog | |
| B1600-58 | `TODO.md:456` | CLion recipe in docs/Debugger.md is untried | product backlog | |
| B1600-59 | `TODO.md:457` | BASIC-level debugger (line breakpoints, statement step, variables) | product backlog | |
| B1600-60 | `TODO.md:462` | Create Debug Project: templates for slot-module ROMs (PC-1500 module, PC-1600 S1/S2) | product backlog | |
| B1600-61 | `TODO.md:467` | PC-1600 auto power-off control (Settings toggle for `KEYWK3`, OFF/ON control) | product backlog | |
| B1600-62 | `TODO.md:470` | Floppy `dirty` key: pick up outside changes to an inserted `.floppy.yaml` (format, periodic check in `FloppyBay`) | design input | Format change planned pre-1.0 at `format-version: 1`; touches format versioning and the media bays |
| B1600-63 | `TODO.md:498` | SharpDataExchange: `sde put`/`del` set `dirty: true`, refuse templates; libsharpdx reader accepts the key | product backlog | Work in the separate SDE repository (Briefing §2) |
| B1600-64 | `TODO.md:509` | Picker and serial-status plumbing written twice (ROM-version enum clones, enum↔string, old-ROM fallback, four Machine-menu builders, serial status pair); one picker with one guard | design input | GUI duplication across menus/pickers; GUI sharing decision (Briefing §1.3) |
| B1600-65 | `TODO.md:535` | Peripheral buttons one setter per peripheral; one attach/detach path per peripheral kind, "enabled" derived from the bus | design input | Device/port model and `ControlState` |
| B1600-66 | `TODO.md:550` | Card/floppy lists and classification written twice and re-parse files; catalogue parse mode without `initial-content` | design input | Media library contract (Process phase 4) |
| B1600-67 | `TODO.md:581` | `PresetLoadResult` / attach path return `{path, isTemplate, battery}` | design input | Same area as B1600-66 |
| B1600-68 | `TODO.md:583` | A shared `NamedFileCatalog`-level helper for lists and classify, used by both bays | design input | Same area as B1600-66 |
| B1600-69 | `TODO.md:589` | PC-1600 live typing rides on the paste feeder; give `PC1600Machine` a key queue like the PC-1500 | design input | Typing strategy (keyboard) contract |
| B1600-70 | `TODO.md:600` | `MachineCodeLoad::setAddress()` repeats `planLoad()`'s PC-1600 steps; unify range-check wording (five places) | product backlog | |
| B1600-71 | `TODO.md:608` | Host drive leftovers in `HostDirectoryDrive.hpp` (`doRename`, `doCreate`, `doWrite`) | product backlog | |
| B1600-72 | `TODO.md:614` | `PC1600BasicTyper.cpp` `codePointCount`: use `decodeUtf8` | product backlog | |
| B1600-73 | `TODO.md:624` | LCD timing re-fit with datasheet behaviour (busy after reads, phase-locked to φ, ignore writes while busy) against the scrolling-PRINT benchmark | product backlog | |
| B1600-74 | `TODO.md:648` | Sub-CPU response time re-fit (per-command time), against A−C = 45 ms | product backlog | |
| B1600-75 | `TODO.md:660` | TC8576F datasheet check (PSR BUSY/XBUSY, DSTB delay) | done | Struck through, "done (dev-0.6.0)" |
| B1600-76 | `TODO.md:662` | Sub-CPU protocol spec in the research corpus | done | Struck through, "done in the corpus" (Ref/PC-1600/PC-1600-SubCPU-LU57813P.md) |
| B1600-77 | `TODO.md:668` | Debugger parses the project preset twice per session action; pass the `PresetFile` down | product backlog | |
| B1600-78 | `TODO.md:678` | `--lcd-png` / `--lcd-text` blocks duplicated in both CLIs: helpers in `tools/CliCommon.hpp` | product backlog | |
| B1600-79 | `TODO.md:681` | `isBasic(Kind)` / `isCode(Kind)` in `ProgramFile.hpp` for `DropFile` and `PresetFile` | product backlog | |
| B1600-80 | `TODO.md:685` | `parsePresetFile` spells top-level keys three times: one `{name, isBlock}` table | product backlog | |
| B1600-81 | `TODO.md:687` | `parsePresetFile` `plotter`/`interfaceName` locals: assign directly | product backlog | |
| B1600-82 | `TODO.md:689` | `SettingsDialog.cpp` `addOpenFolderRow` special case: pass tooltip in | product backlog | |
| B1600-83 | `TODO.md:692` | `extension.js` port fallback redundant | product backlog | |
| B1600-84 | `TODO.md:695` | `tools/dap_smoke.py`: a `session(port, **attach)` context manager | product backlog | |
| B1600-85 | `TODO.md:697` | PC-1600 module window base: follow the ROM's probe, not the image size | product backlog | |
| B1600-86 | `TODO.md:713` | PC-1600 loaders route page C / page D themselves: one `PC1600Memory::busWrite(bank, addr, data, n)` | product backlog | |
| B1600-87 | `TODO.md:725` | loader-matrix `readFile`: use `readWholeFile` | product backlog | |
| B1600-88 | `TODO.md:726` | loader-matrix `lcd()` / `find("ERROR")`: use `LcdText::logField()` / `contains()` | product backlog | |
| B1600-89 | `TODO.md:728` | loader-matrix PC-1600 MEM: use `readPC1600ProgramAreas(peek).memS0` | product backlog | |
| B1600-90 | `TODO.md:731` | loader-matrix `snapshot()`: use `PC1600Machine::debugCopyInternalRam()` | product backlog | |

Notes:
- `TODO.md:185-192` ("Expansion connectors: one model on both machines",
  done 2026-10-07) is a context paragraph, not a bullet; no row. It records
  the finished connector work.
- `TODO.md:226-252` (the DME0 plan steps and open assumption) belong to
  B1600-25; `TODO.md:46-79` ("Ruled out") are evidence for B1600-1.
- No item was found that is a candidate for *dropped*.
