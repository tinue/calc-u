# Contract: harness protocol

Status: draft (2026-11-10). Forks H-1 to H-3 are open.

Inherits: ADR 0026 (one C++ CLI for the family with one-shot
subcommands, plain lines or `--json`, and a `serve` mode taking one
command per line; keeps every capability of today's tools), 0033 (in
development and test builds every frontend can open its app session to
the serve protocol: local network for the iOS simulator, WebSocket for a
browser; release builds leave it out), Briefing §8 (today's capabilities
stay). Builds on `frontend.md` (FE-1 messages), `presets.md` (step
vocabulary), `debugger.md` (run control, DB-3: the harness may set
breakpoints).

## Capabilities to keep (facts)

| Tool today | Capabilities | Source |
|---|---|---|
| Calc-U-59 `tools/headless/` (C API + `ti59.py`) | headless run, scripted keys, step-by-step driving from Python, per-instruction records, state dump (registers, SCOM, RAM), printer lines, cards, disassembly, ROM coverage | `docs/inventory/calc-u-59.md`, Tests |
| Calc-U-59 `tools/agentic/` | run `.ti59` files, timing in emulated seconds, printout, hot steps, step cost, keycode assembler and disassembler | same |
| `read_trace.py` (both) | trace files to text or JSON | both inventories |
| Calc-U-1600 `pc1500_cli`, `pc1600_cli` | `--preset`, `--dump-basic`, `--dump-mem`, `--inspect <view>`, `--lcd-png`, `--lcd-text`, `--wav`, `--tape-in/-out`, `--ce158-pty/-rx/-tx`, `--run-after`, `--rom`, `--ce1600p-rom`, `--save-dir`, `--modules-dir`, `--check-preset` | `docs/inventory/calc-u-1600.md`, Tests |
| Calc-U-1600 probes | plotter, power and UART diagnostics | same |
| `dap_smoke.py`, `--shots` scenarios | DAP protocol smoke; screenshot scenarios (frozen emulation, isolated settings) | same |

## Responsibilities

- One program, `calc-u-harness` (name open, Q-1), with `--product`,
  running the same library and app-layer code as the frontends (ADR
  0009: the same code in virtual time).
- One-shot subcommands and `serve` mode (ADR 0026); `serve` is also what a
  frontend exposes for attach (ADR 0033).
- Every capability in the table above, as a common command or a
  product command (H-2).

## Non-responsibilities

- Not a frontend, not for end users (glossary).
- No GUI automation; screenshots come from the app session's own image
  of the display or panel, or from the frontend when attached.

## Invariants

- I-1: The harness drives a machine only through the app session and
  the machine interface, like a frontend (ADR 0006).
- I-2: Without attach, the harness runs in virtual time: the same input
  gives the same output (machine contract I-2).
- I-3: Attach exists only in development and test builds (ADR 0033).

## Open forks

### H-1: Command vocabulary

Options:
- (a) **Recommended:** the serve protocol's commands are the preset step
  vocabulary (`key`, `type`, `wait`, `expect`, `screenshot`, `insert`,
  `arm`, `fit`, `speed`, `pause`, `step`, `resume`, `trace`, …) plus
  inspection and control commands (`load` a preset, `lcd`, `dump`,
  `peek`/`poke`, `break`, `state` of a topic, `quit`). An agent learns one
  language for presets and the harness; a preset is a recorded harness
  session.
- (b) A separate command language for the harness.

### H-2: Product-specific commands

Options:
- (a) **Recommended:** common commands at the top level; commands that
  exist for one product only live under the product's name (`ti59 asm`,
  `ti59 keycodes`, `ti59 coverage`; `pc1600 dump-basic`, `pc1600 tape`,
  `pc1600 serial`), and `--product` selects the product for one-shot runs.
  A command present in both products (for example `dump`) is common, with
  product-specific output where the hardware differs.
- (b) Flat command list with product prefixes in the names.

### H-3: Attach transport and safety

Options:
- (a) **Recommended:** the serve protocol is line-based text (one command
  per line, a reply per line, `--json` replies on request) over stdin and
  stdout for `serve`, over TCP bound to localhost for an app (the iOS
  simulator shares the Mac's network), and over a WebSocket that the
  page opens to a localhost port given by a development flag. No
  authentication beyond localhost; never in release builds (I-3).
- (b) A binary or JSON-RPC protocol: harder for an agent to type and
  read.

## Fit table

| | TI-59 (58, 58C) | PC-1500 (1500A) | PC-1600 | PC-1403 (paper) | PC-1350 (paper) |
|---|---|---|---|---|---|
| Common commands | key, type (—), wait, expect, lcd, dump, peek, break, trace, screenshot | same, with `type` | same | same | same |
| Product commands (H-2) | `asm` (ASM overlay), keycode (dis)assembler, ROM coverage, printer lines, card dump | BASIC dump, tape in/out, CE-158 serial | BASIC dump, tape, serial, host drive, probes | — | — |
| Attach (ADR 0033) | Swift iOS simulator, macOS app, Qt, web | Qt, web | Qt, web | — | — |

## Open questions

- Q-1: The program's name.
- Q-2: The Python step-by-step driving of `tools/headless/` becomes a
  Python script talking to `serve` over a pipe; whether a small Python
  helper ships with the harness is decided in phase 6 with the skills.

## Decision log

| Date | ID | Decision |
|---|---|---|
| | | |
