---
name: calc-u-59-core
description: Use this skill when working on the C++ emulation core of Calc-U-59 (files in core/). Covers the TMC0501 CPU emulator, BCD register model, SCOM memory, machine variants (TI-59/58/58C), ROM/RAM, instruction set changes, mnemonic workflow, and trace infrastructure. Trigger on any task that touches core/*.cpp, core/*.hpp, tools/mnemonics.tsv, ROM disassembly, trace debugging, or machine variant behaviour.
user-invocable: true
---

# Calc-U-59 Emulator Core

This skill covers all work inside `core/` and the associated Python dev tools.

---

## Domain Primer (TI-59 background)

The TI-59, TI-58, and TI-58C are Texas Instruments handheld programmable calculators (1977–79). This project emulates them at the hardware level: **the original ROM firmware runs unmodified** on an emulated TMC0501 CPU, so calculator behaviour emerges from the firmware — debugging usually means tracing ROM execution, not reading emulator logic.

- **Keystroke programming**: users program the calculator by recording key presses. Each program step stores one two-digit **keycode**; the TI-59 holds up to 960 steps / 100 data registers, with RAM partitioned between the two.
- **Hardware chips**: TMC0501 (CPU), TMC0571 (SCOM — scratchpad RAM + display/keyboard scan), TMC0540 (CROM — plug-in "Solid State Software" library module).
- **Solid State Software modules**: plug-in ROM cartridges holding up to 5000 pre-recorded program steps (as keycodes). The user runs library program *n* with `2nd Pgm nn`. The Master Library module ships with the calculator and is loaded by default in the emulator.
- The display is a 12-digit LED; the firmware multiplexes it digit-by-digit during IDLE.

---

## Where the facts live

Do not restate these here — read them:

| Topic | Document |
|---|---|
| Components, registers, SCOM[9] and the return stack, RAM layout, TI-58C extra registers, instruction set, `step()` pipeline and its return bits, display, keyboard/`WAIT`, card reader, CROM and `libExecPC`, PRG SOURCE flag, printer, threading/locks, trace API, variant table | `reference/CoreArchitecture.md` |
| How the firmware uses the hardware: SCOM map, fA/fB flags, AOS, run loop, labels, subroutines, cards, printing, error sites | `rom/TI59-ROM-Guide.md` |
| What the TI-58C ROM does differently, known core issues it exposed | `rom/TI58C-ROM-Guide.md` |
| Fast mode; the 1F printer interrupt | `rom/fastmode.md`, `rom/printerinterrupt.md` |
| `CpuFrame` fields, trace file format, ASM overlay, debugger semantics | `reference/DebugAPI.md` |
| Pacing, run loop, debugger model — everything above the hardware | `reference/SharedArchitecture.md` |

Annotated ROM listings: `rom/TI59-commented.asm`, `rom/TI58C-commented.asm`
(labels: `rom/*-labels.asm`; keycode sub-programs: `rom/*-keycode-programs.asm`).

---

## Rules for Core changes

- **ROM files are read by the library's lookup only** (LIB-12; ADR 0035).
  The core supplies the reader: `core/TIRomReader` decodes each file for its
  amount (`TIRomText` for the Datamath dumps, `core/ModuleFile` for a
  module's `<ID>.rom.yaml`, `docs/formats/modules.md` §2) and adds the set's
  checks (6144 words, 64 rows, the TI-59 sentinels). The core opens no file;
  the machine gets the files as bundled in `RomImage`s. The 24 files are
  flat in `roms/` (`roms/README.md`, `roms/fetch_roms.sh`).

- **`calcu::machine::Machine` is the only public interface** (C59-1,
  ADR 0006): `TIMachine` (`core/TIMachine.hpp`), built with
  `calcu::machine::build(tiCatalogue(), TIMachineBuilder, …)`
  (`core/TICatalogue.hpp`; `cu59::buildMachine` in `app/ProductRoms.hpp`,
  from the `RomSet` of the ROM lookup, LIB-12).
  Model, advances and tick count, Reset/power cycle/cold boot, key readiness,
  keys (key IDs) and the display output go through it only. What it does not
  have yet goes through the adapters of `core/TIAdapters.hpp`, each until its
  removal step (`TICardReaderAdapter` went in M-55: the card reader is the media place `core/TICardReaderPlace` with `core/TIMagneticCard`'s content and `buildCardBank`; `libExecPC` is `c59.calculator-state`'s),
  `TIMemoryAdapter` (M-54; its readers M-59); `idleAndKeyDetectRange()` is
  the scan/detect range. The debug panel's reads are product kinds since
  M-56 (`core/TIDebugKinds.hpp`: `c59.asm-overlay`, `c59.rom-heatmap`,
  `c59.calculator-state`, `c59.machine-log`; find one with
  `cu59::kindOf<K>(machine)`); `idleAndKeyDetectRange()` is
  the scan/detect range for `tools/headless/` and the tests. The debugger
  reaches the core through `Machine::debugging()`: the debug target
  `TMC0501` (`core/TIDebugging.hpp`; LIB-10, M-53), whose
  `InstructionEvent::marks` say step boundary, scan loop and weight; traces
  through the capability `TMC0501/c59.trace-records`
  (`core/TraceRecords.hpp`). `TIMachine` alone sets the trace flags. `TI59Machine`,
  `TMC0501`, `MachineVariant`, `TIKeys`, `TIRomRanges` are core-internal; the
  CTest `CalcU59CoreSeam` (`tests/check_core_seam.sh`) fails on an include of
  them outside `core/` and `tests/`. Don't add a call to an adapter that the
  interface has (C59-1 AC-17); a new need goes to the feature that owns it.
- **Peripherals (C59-6):** the PC-100C (`core/PC100C.*`) and a Solid State
  ROM module (`core/SolidStateRomModule.*`) are peripherals in the places
  `printer` and `slot-1`, plugged by `TIMachine::plug/unplug` through
  `Machine::attach/detach/replace` (LIB-2); a change is followed by a power
  cycle the caller runs. `TMC0501` sees only their functional pin-outs
  (`core/PrinterPinOut.hpp`, `core/ModuleSlotPinOut.hpp`, wired with
  `TiConnectors`) and holds no printer state and no module image; the
  `libExecPC` latch stays in the CPU. The PC-100C's controls (PRINT, ADV,
  TRACE) and its paper (`printer/paper-output`, LIB-8's kind; the
  description, font and dot rules in `core/PC100CPaper.hpp`) are
  capabilities; its code lines are kept only for the golden runs and the
  headless tool (`TIMachineTesting::printedCodeLines`, until M-59); `CoreArchitecture.md` § "Component Overview", "Printer
  (PC-100C)", "Solid-State Library Module".
- **Ticks:** one instruction is 32 ticks, 128 in IDLE on the TI-59/58; the
  tick count is monotonic across every operation (`TIMachine::cpuReset`).
  Every path that runs an instruction goes through `TIMachine::stepOne()`
  (the Reset restore, the polled-hardware bit).
- **Core stays hardware-only.** File formats, run-loop pacing, key timing and
  debugger snapshots belong in `app/` (calc-u-59-shared skill).
- **The core holds no lock, no atomic and starts no thread** (ADR 0009).
  Every frontend runs the machine on the one thread of its app session
  (since M-34 also the Web); nothing serialises calls on it.
- **Trace format:** a new `CpuFrame` field changes the binary trace format
  (`reference/DebugAPI.md`); avoid it where an accessor will do (as
  `libExecPC()` does).

---

## Mnemonics Workflow

`tools/mnemonics.tsv` is the **single source of truth** for all TMC0501 instruction mnemonics.

- Python tools (`disasm.py`, `read_trace.py`) read the TSV at runtime — no regeneration needed.
- `uv run python tools/disasm.py` regenerates `rom/TI59.asm` and `rom/TI58C.asm`.
- C++ arrays in `TMC0501.cpp` are hard-coded and must be regenerated after any TSV change:

```bash
uv run python tools/disasm.py --emit-cpp > /tmp/generated.cpp
# Paste over the section marked "── Generated tables ──" in core/TMC0501.cpp
```

---

## Dev Tools Quick Reference

| Tool | Purpose |
|------|---------|
| `tools/headless/ti59.py` | Drive the core from Python, annotated ROM trace per key (`--variant`, `--program`, `--module`); see `tools/headless/README.md` |
| `tools/headless/coverage.py` | Which keystroke scenario runs which ROM word |
| `tools/agentic/` | Run, time and profile `.ti59` programs headless; read `tools/agentic/README.md` first |
| `tools/disasm.py` | ROM disassembler; `--emit-cpp` regenerates C++ mnemonic arrays |
| `tools/annotate_rom.py` | Apply comment batches to `rom/*-commented.asm` |
| `tools/read_trace.py` | Parse a binary trace file → text/JSON |
| `tools/compare_trace.py` | Find the first divergence between two trace files |
| `tools/compare_logs.py` | Align two log files on matching ROM address sequence |

---

## Global Rules

1. **Build all, don't run.** A `core/` change must pass all builds — CMake
   with its tests, Xcode, and the web client (commands: CLAUDE.md § "Frontend
   Parity").
   Do not launch the iOS Simulator or the built app — that is the user's job.
   When runtime verification is needed, ask the user to launch the app and
   say precisely what to look for. Headless runs (`tools/headless/`,
   `tools/agentic/`) are fine.

2. **Build on macOS.** Verify Swift-facing changes with a macOS build; build for iOS only when explicitly required (commands: `calc-u-59-swift` skill, § "Build Verification").

3. **Commit often, never push.** Commit after each logical unit of work. Never run `git push`.

4. **Cite your sources.** When referencing a hardware spec, standard, or external document, include the https URL (or a pointer to the local reference file) in the relevant code comment or documentation.
