# Contract: debugger (debug target, DAP, debug panel)

Status: draft (2026-11-10). DB-1, DB-3, DB-4, DB-6 decided; DB-2 and
DB-5 replaced by a minimal agreement (below), decided. DB-6 and the
note on DB-1 were added in the phase 4 review.

Inherits: ADR 0006 (debug targets are part of what a machine exposes),
0023 (the debug panel is in the shared component sets), Briefing §2
(DAP for Calc-U-59 is a non-goal). Oddities: ODD59-27 (the session
ignores core breakpoints), Calc-U-1600's "INSPECTOR views in Core" and
debugger entries (nothing copied into projects, `debug:` block, ROM
extensions, bus ROM shadows, `boot: debug`, pre-execution snapshots)
(`docs/oddities-triage.md`). Builds on `machine.md` (debug targets in
the descriptor), `frontend.md` (FE-1 topics), `presets.md` (PR-7:
`debug-panel:`, `pause:`, `pause-on-start:`, `step:`, `resume:` in both
products).

## Today (facts)

| | Calc-U-59 | Calc-U-1600 |
|---|---|---|
| Debugger UI | debug panel in every frontend: CALCULATOR tab (registers, flags, HIR, T, return stack, partition, listing, PRG SOURCE), CPU tab (history, registers, control registers, display state, ROM heatmap), LOG tab; ASM overlay (`reference/DebugAPI.md`, `FrontendParity.md`) | DAP server in the Qt frontend (`Qt6/app/debug/`, written against QJson), VS Code extension; INSPECTOR panel, views built in Core (`Decisions.md:636-642`); Qt only, not on the web |
| Levels | FREEZE / STEP per panel: CALCULATOR = keystroke program steps, CPU = instructions | instructions only; no BASIC-level debugger (`TODO.md:457-461`) |
| "On start" | FREEZE ON START: CALCULATOR freezes at the next step boundary (`pcChangeArmed`); CPU freezes when the CPU leaves the keyboard scan loop (`scanLoopExitArmed`) | `stopOnEntry`: stop right after attaching (after a reset: before the first instruction) (`docs/Debugger.md`) |
| Breakpoints | core PC breakpoints exist (`TI59Machine::addBreakpoint`), used only by the headless tools; the session ignores them (ODD59-27) | PC breakpoints with conditions, hit counts, log messages; data breakpoints (watches); bank qualifiers; "breakpoints stop only while the debugger runs the machine; a preset, a load or a normal boot never stops on one" |
| Target interface | `TI59Machine` calls (`snapshotCPU`, `stepN`, `stepUntilNextKeycode`, RAM/SCOM access, trace ring) | `debug::DebugTarget`: CPUs as threads, registers (≤ 16 bits), side-effect-free `peek`/`poke` per address space (bytes), decode, history ring, breakpoints, watches, bank state (`Core/Debug/DebugTarget.hpp`) |
| Snapshots | pre-execution (`beginNextStep`: capture before the next step) | pre-execution everywhere: live frame, history, trace (`Decisions.md:866-871`) |
| Traces | binary trace v2 (`TI59`), `tools/read_trace.py` | `TRACE.bin` v3 (both CPUs), `tools/read_trace.py` |
| Product-only | ASM overlay (`0x1800-0x1FFF`), ROM heatmap | listings and symbols, `debug:` block, `boot: debug`, bus ROM, `slot-N-rom:` |

## Minimal agreement (decided 2026-11-10)

The user's framing: the two debuggers serve different goals. The TI-59's
CPU level was for understanding the ROM, a goal reached with the fully
commented disassembly; it is now educational. Calc-U-1600's debugger is
for user programs, ROM extensions and drivers, close to a modern
debugger through VS Code. The project finds common ground in the
architecture, not much more, and keeps each product's material intact.

Common ground in the library:
1. One debug-target interface (DB-1).
2. One run control on the session's thread: pause, resume, step(n),
   breakpoints and watches (DB-3), pre-execution snapshots (I-1).
3. Named levels: every target has `cpu`; a product may declare a
   `program` level (the calculator level) and defines what one step
   there is. The library only routes pause, step(n) and an optional
   pause-on-start event to it. TI-58/59: one program step as today,
   including stepping through the ROM's and the modules' keycode
   programs (no step out, as today). Calc-U-1600: a BASIC level stepping
   statement by statement is prepared, not built.
4. The preset steps of PR-7 drive this run control (`pause: program |
   cpu`, `step: cpu 5`, `resume:`, `pause-on-start: …`); `debug-panel:`
   selects a view where the product has a debug panel.
5. The debug panel is per product (DB-6): each product keeps its own
   panel in each of its frontends, fed by product topics
   (`frontend.md`, FE-6). The shared component sets hold only the run
   control's buttons (pause, step, resume).

Stays product-specific, unchanged:
- Calc-U-59: the CALCULATOR / CPU / LOG panel, FREEZE and FREEZE ON
  START, ASM overlay, ROM heatmap; no DAP.
- Calc-U-1600: DAP and the VS Code extension, listings, `debug:` block,
  `boot: debug`, bus ROM, INSPECTOR; no in-app stepping debugger.
- `stopOnEntry` (DAP: pause at once on attach) and FREEZE ON START
  (pause at the next event) stay separate concepts.

Backlog, not part of the migration:
- Calc-U-1600 BASIC-level debugger (already `B1600-59`).
- Calc-U-59 CPU tab showing the commented ROM disassembly instead of raw
  mnemonics (new, from the user on 2026-11-10; `B59-59` in
  `docs/inventory/backlog.md`).

## Responsibilities

- A machine exposes one **debug target** per CPU-like thing it models
  (machine contract), through one library interface (DB-1).
- The library's **run control** pauses, steps and resumes a machine at
  the levels its target offers (DB-2), and checks breakpoints and watches
  (DB-3).
- The **debug panel** content comes to the frontend as product topics
  (`frontend.md`, FE-5, FE-6); each product's frontends render their own
  panel (DB-6).
- Preset steps `pause:`, `pause-on-start:`, `step:`, `resume:`,
  `debug-panel:` (PR-7) drive the same run control in both products.

## Non-responsibilities

- Trace file formats stay per product (own formats, ADR 0037 applies).
- Listings, symbol tables and the `debug:` block: Calc-U-1600 product
  material (DAP use cases).
- The ASM overlay: a Calc-U-59 capability of the TI machine.
- DAP for Calc-U-59 (Briefing §2).

## Invariants

- I-1: Snapshots are pre-execution everywhere (both products today).
- I-2: Reading through a debug target has no side effects; a read that
  would disturb a device is reported as unreadable (Calc-U-1600 `peek`).
- I-3: Breakpoints stop the machine only while a debugger runs it; never
  inside an operation (preset, load, boot) (Calc-U-1600 rule).
- I-4: The run control runs on the session's thread; no locks (ADR 0009).

## Open forks

### DB-1: The library's debug-target interface

**Decided (2026-11-10): option (a).**

Facts: Calc-U-1600's `DebugTarget` assumes 8-bit memory cells and
registers of at most 16 bits. The TI-59's CPU has 16-nibble registers
(A-E), 13-bit ROM words, RAM registers of 16 nibbles, SCOM, and program
steps.

Options:
- (a) **Recommended:** Calc-U-1600's `DebugTarget` becomes the library
  interface, generalised: each target declares its address spaces (name,
  cell width, size: TI ROM words, RAM registers, SCOM; Sharp ME0/ME1,
  Z-80 memory) and its registers with any width (the TI's 64-bit nibble
  registers); history, decode, breakpoints and watches as today. The TI
  machine adapts `TMC0501` to it.
  Added in the review (2026-11-10, approved by the user): a target
  states which parts it offers. Every target has registers, reading and
  writing its address spaces, PC breakpoints and stepping; watches,
  breakpoint conditions and hit counts, decode and the history ring are
  optional per target. The TI target starts with the required parts and
  what its harness commands need, since nothing else uses it (no DAP).
- (b) Calc-U-1600's interface as it is, the TI keeps its own: two
  debugger stacks, against ADR 0006's one seam.

### DB-2: Debug levels, and "on start" vs `stopOnEntry`

**Replaced by the minimal agreement** (the user found the proposal below
too far-reaching; kept for the record).

Facts: Calc-U-59 debugs at two levels (keystroke program steps,
instructions); Calc-U-1600 at one. FREEZE ON START waits for an event
(the next program step; the CPU leaving the key-scan loop); `stopOnEntry`
stops at once when the debugger attaches. They are different things.

Options:
- (a) **Recommended:** a debug target lists its **levels**: `cpu` on
  every machine; `program` where a program level exists (TI-58/59
  keystroke programs today; a later BASIC level would be one too). The
  run control pauses and steps per level. `pause-on-start:` arms an
  event per level: `program` = the next program step, `cpu` = the CPU
  leaving the key-scan loop (needs key readiness, `typing.md` T-2).
  `stopOnEntry` stays a DAP launch option meaning "pause at once". Preset
  steps name the level (`pause: program`, `step: cpu 5`); today's panel
  names (Calculator, CPU) are labels in Calc-U-59's panel.
- (b) Only the CPU level in the library; the TI's program level stays
  Calc-U-59 code beside it.

### DB-3: Breakpoints in GUI sessions (ODD59-27)

**Decided (2026-11-10): option (a); ODD59-27 superseded.**

Options:
- (a) **Recommended:** one rule (I-3): breakpoints and watches are
  checked whenever a debugger client has set some and the machine is
  not in an operation; a hit pauses the machine like a pause step. In
  Calc-U-59 no GUI sets breakpoints (no new feature), but the harness
  can, so ODD59-27 is superseded: the session no longer ignores them.
- (b) Keep ODD59-27: breakpoints only in the headless tools.

### DB-4: Where the DAP server lives

**Decided (2026-11-10): option (a), the Calc-U-1600 app layer.**

Facts: it is Calc-U-1600-only (Briefing §2), in the Qt frontend, written
against QJson; Calc-U-1600's own docs list it as "not in Shared yet"
(`Shared-Architecture.md:133-140`). The web has no DAP.

Options:
- (a) **Recommended:** DAP stays Calc-U-1600 product code (used by one
  product, so not library), moved out of the Qt frontend into the app
  layer on the library's debug target and run control, without Qt types.
  It then works in any frontend that can listen on a socket, and the
  harness can host it.
- (b) Stays in the Qt frontend as today.
- (c) In the library: shared code used by one product.

### DB-5: The debug panel's content

**Replaced by the minimal agreement, point 5.**

Facts: Calc-U-1600 builds its INSPECTOR views in Core; Calc-U-59 builds
its panel data and text in Shared (`DebugModel`, `DebugText`).

Options:
- (a) **Recommended:** product code below the frontend builds the panel
  as generic views (tables, text, hex dumps, a heatmap), sent as a debug
  topic (FE-1); the shared component sets (ADR 0023) render generic
  views, and each product names its tabs. Both products already build
  their views below the frontend.
- (b) Per-product panel widgets in each component set.

### DB-6: The debug panel is per product

**Decided (2026-11-10, user, in the phase 4 review).**

Facts: ADR 0023 lists the debug panel among the shared component sets.
The minimal agreement keeps "the CALCULATOR / CPU / LOG panel" and the
INSPECTOR product-specific and unchanged; DB-5's generic views were
found too far-reaching.

Options:
- (a) **Chosen:** per-product panels. Each product keeps its own panel
  widgets per frontend, fed by product topics. Shared in the library:
  the debug target, the run control, and the pause, step and resume
  controls. ADR 0023 carries a note.
- (b) One shared panel rendering generic views (DB-5 (a)).
- (c) A shared frame with product content inside.

## Fit table

| | TI-59 (58, 58C) | PC-1500 (1500A) | PC-1600 | PC-1403 (paper) | PC-1350 (paper) |
|---|---|---|---|---|---|
| Targets (threads) | TMC0501 | LH5801 | SC7852 (Z-80), LH5803 | SC61860 | SC61860 |
| Address spaces (DB-1) | ROM words (13 bit), RAM registers (16 nibbles), SCOM, program steps | ME0, ME1 | Z-80 memory (page banks), LH5803 ME0/ME1 | internal RAM 00-5F, memory | internal RAM, memory |
| Levels (DB-2) | `cpu`, `program` | `cpu` | `cpu` | `cpu` | `cpu` |
| `pause-on-start: cpu` | key-scan loop exit (scan-loop ranges) | needs key readiness (typing Q-3) | key-matrix reads | — | — |
| UI | debug panel (all frontends) | INSPECTOR panel; DAP (Qt) | INSPECTOR panel; DAP (Qt) | — | — |
| Product-only | ASM overlay, ROM heatmap | listings, `boot: debug`, bus ROM | same | — | — |

## Open questions

- Q-1 (answered 2026-11-10, user): when this is built (phase 6), research the
  PC-1500's key-scan loop and implement key readiness, so
  `pause-on-start: cpu` works there too (also answers `typing.md` Q-3).
  Background: Whether `pause-on-start: cpu` on the PC-1500 waits for its key
  readiness signal (typing Q-3) or is not offered until one exists.

## Decision log

| Date | ID | Decision |
|---|---|---|
| 2026-11-10 | DB-1 | Calc-U-1600's `DebugTarget` becomes the library interface, generalised: address spaces with their cell width, registers of any width. |
| 2026-11-10 | DB-3 | Breakpoints and watches are checked whenever a debugger client set some and no operation runs; ODD59-27 superseded. Calc-U-59 gets no breakpoint UI. |
| 2026-11-10 | DB-4 | DAP stays Calc-U-1600 product code, moved out of the Qt frontend into the app layer, on the library's debug target and run control. |
| 2026-11-10 | DB-1 (note) | A debug target states which parts it offers: registers, address spaces, PC breakpoints and stepping are required; watches, conditions, decode and history are optional per target. |
| 2026-11-10 | DB-6 | The debug panel is per product, fed by product topics; the library shares the debug target, the run control and the pause, step and resume controls. ADR 0023 carries a note. |
| 2026-11-10 | DB-2, DB-5 | Replaced by the minimal agreement: common debug target, run control and named levels in the library; panels, DAP and freezes stay product-specific; two backlog items. |
| 2026-11-10 | Traces (settled in phase 4b, user, 2026-11-10, batch 6 gate) | Starting, limiting and ending a trace is library code, for the step, the command and the buttons; the content of a trace file stays per product. One trace at a time; the size limit holds for every trace. |
| 2026-11-10 | Minimal agreement (completed, user, 2026-11-10, batch 6 gate) | A step at another level takes the pause over; breakpoints belong to the one that set them; the run control can pause before the first instruction after a start; the family's Machine menu has Resume. |
