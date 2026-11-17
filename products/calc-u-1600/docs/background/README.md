# Background

Why Calc-U-1600 is built the way it is: settled decisions, the plans
features were built from, and handoff notes from investigations. None of
this is needed to use or build the emulator — for that, see the
[main README](../../README.md) and the
[developer documentation](../developer/README.md).

## Decisions

- [Decisions.md](Decisions.md) — things that look wrong, redundant or
  over-complicated at first sight but are deliberate. **Read it before a
  cleanup, simplification or review**, and add an entry when a recurring
  question gets settled.

## Hardware notes

- [CE-161-Program-Module.md](CE-161-Program-Module.md) — how the CE-161
  RAM module becomes a program module: the write-protect switch, the
  PC-1500 ROM's power-on scan and module header, and what the manual's
  `POKE` does.

## Ideas

Features thought through but not planned: what the hardware and ROMs
offer and which routes look workable.

- [PC1500-Card-Disk-Idea.md](PC1500-Card-Disk-Idea.md) — a software-only
  RAM disk for the PC-1500(A) on a CE-1638 / CE-163F card, with ideas from
  the PC-1600 ROM's RAM disk and the PC-1500's load paths and hooks.

## Plans

> **Historic.** These plans were written for Calc-U-1600 up to 0.8.1, in
> its own repository, before the move into the Calc-U repository (phase 6
> of `docs/Process.md` there). They use the names and paths of 0.8.x:
> `Core/` is now `core/`, `Shared/` (the Shared layer) the app layer
> `app/` (namespace `cu1600`), `Qt6/` and `Web/` are `frontends/qt6/` and
> `frontends/web/`; the CLIs `pc1500_cli`, `pc1600_cli` and the probes are
> the harness `calcu --product 1600`; `PresetFile`, `PresetRunner` and
> `bundled.yaml` are the library's presets (LIB-6, `examples/bundle.yaml`);
> the bays (`ModuleBay`, `FloppyBay`, `TapeBay`) are the library's media
> part (LIB-7); "card" for a memory module or a peripheral, "turbo" for
> full speed. They are not updated. How the product works today:
> [developer documentation](../developer/README.md) and, at the
> repository root, the features in `features/` (`C1600-n` and the
> library's `LIB-n`) with `docs/GLOSSARY.md`.

Each plan was written before the work and committed with it. Most start
with a status note on how the implementation differed. A new feature is
specified and designed in `features/` at the repository root, not here.

| Plan | Written | Status |
|---|---|---|
| [DAP debug server](plans/DAP-Debugger-Plan.md) | 2026-09-25 | Implemented |
| [Debugger restructuring](plans/Debugger-Restructuring-Plan.md) | 2026-09-25 | Implemented (R1–R12) |
| [Debugger IDE integration by use case](plans/Debugger-Use-Cases-Plan.md) | 2026-09-30 | Implemented (P0–P6); CLion untested, no slot-module template |
| [BASIC debugger: how to plan it](plans/BASIC-Debugger-Planning.md) | 2026-09-26 | Recommendation only, no plan or code yet (TODO.md, feature ideas) |
| [Code cleanup backlog (low-risk part)](plans/Code-Cleanup-Plan.md) | 2026-09-26 | Implemented; picker, card/floppy and timing re-fit left in TODO.md |
| [Preset review: one naming rule, presets up to date](plans/Preset-Review-Plan.md) | 2026-09-30 | Implemented |
| [Expansion connectors: mechanical groundwork](plans/Expansion-Connectors-Plan.md) | 2026-09-26 | Implemented; signal work done in the 60-pin plan |
| [60-pin / 40-pin connectors: real contacts](plans/Sixty-Pin-Connector-Plan.md) | 2026-10-07 | Implemented; hardware questions open in TODO.md |
| [PC-1600 LCD: HD61102/HD61203 datasheet facts](plans/PC1600-LCD-Datasheet-Plan.md) | 2026-09-26 | Phase 1 implemented; Phase 2 (timing) in TODO.md |
| [PC-1600 sub-CPU, TC8576F CPC and RTC/wake-up](plans/PC1600-SubCpu-CPC-Plan.md) | 2026-09-26 | Implemented |
| [PC-1600: type KBII accented characters](plans/KBII-Typing-Plan.md) | 2026-09-27 | Implemented |
| [Loaders follow MODE and program area](plans/Loader-Mode-Plan.md) | 2026-09-27 | Implemented |
| [Loaders: one file classifier from libsharpdx](plans/Loader-File-Kind-Plan.md) | 2026-09-27 | Implemented |
| [Drag-and-drop loading](plans/Drag-And-Drop-Plan.md) | 2026-09-30 | Implemented |
| [PC-1500 ROM extension demo: RENUM](plans/PC1500-RENUM-ROM-Plan.md) | 2026-10-02 | Implemented |
| [Reading the LCD as text](plans/LCD-Text-Plan.md) | 2026-10-03 | Implemented |
| [Pre-execution snapshots: history and trace](plans/Pre-Execution-Snapshots-Plan.md) | 2026-10-04 | Implemented |
| [Cassette CLOAD / CSAVE via WAV files](plans/Cassette-Tape-Plan.md) | 2026-10-04 | Implemented for PC-1500 + CE-150 and PC-1600 MODE 0; MODE 1 `CLOAD`, CE-150 on PC-1600 and the real-unit cross-check open in TODO.md |
| [Pre-1.0 format freeze: versions, `slot-N-rom:`, one `saveas:`](plans/Format-Freeze-Plan.md) | 2026-10-10 | Implemented |
| [Debug panel → INSPECTOR: pointers, memory views, dumps](plans/Inspector-Panel-Plan.md) | 2026-10-10 | Implemented; RAM-disk directory listing open in TODO.md |
| [Shared layer for a WebAssembly GUI](plans/Shared-Layer-Plan.md) | 2026-11-06 | Implemented (phases 0–5); the DAP moved into the app layer in M-50 (C1600-3) |
| [Shared layer follow-up: frontend-neutral seams](plans/Shared-Layer-Followup-Plan.md) | 2026-11-06 | Implemented |
| [Web GUI: WebAssembly frontend, embed + PWA](plans/Web-GUI-Plan.md) | 2026-11-06 | Implemented; calc-u.org page, user docs and iPad check in TODO.md |
| [Names and descriptions: presets, cards, disks](plans/Media-Names-Plan.md) | 2026-11-07 | Implemented |
| [RAM write-protect switch; CE-161 battery-backed](plans/RAM-Write-Protect-Plan.md) | 2026-11-09 | Implemented |

## Handoffs

Notes left at the end of a working session on work that is still open:
where things stand and what to pick up next. A handoff is removed once
its work is resolved.

None open. (The debugger handoff became
[developer/Debugger-Internals.md](../developer/Debugger-Internals.md); its
open checks are in TODO.md.)
