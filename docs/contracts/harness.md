# Contract: harness protocol

Status: draft (2026-11-10). Forks H-1 to H-4 are decided (decision log;
H-4 was added in the phase 4 review).

Inherits: ADR 0026 (one C++ CLI for the family with one-shot
subcommands, plain lines or `--json`, and a `serve` mode taking one
command per line; keeps every capability of today's tools), 0033 (in
development and test builds every frontend can open its app session to
the serve protocol: local network for the iOS simulator, WebSocket for a
browser; release builds leave it out), Briefing §8 (today's capabilities
stay). Builds on `frontend.md` (FE-1 messages), `presets.md` (step
vocabulary), `debugger.md` (run control, DB-3: the harness may set
breakpoints).

## Functions to keep (facts)

("Capabilities" in Briefing §8 and ADR 0026 means what the tools can do;
the glossary keeps that word for what a machine exposes.)

| Tool today | Functions | Source |
|---|---|---|
| Calc-U-59 `tools/headless/` (C API + `ti59.py`) | headless run, scripted keys, step-by-step driving from Python, per-instruction records, state dump (registers, SCOM, RAM), printer lines, cards, disassembly, ROM coverage | `docs/inventory/calc-u-59.md`, Tests |
| Calc-U-59 `tools/agentic/` | run `.ti59` files, timing in emulated seconds, printout, hot steps, step cost, keycode assembler and disassembler | same |
| `read_trace.py` (both) | trace files to text or JSON | both inventories |
| Calc-U-1600 `pc1500_cli`, `pc1600_cli` | `--preset`, `--dump-basic`, `--dump-mem`, `--inspect <view>`, `--lcd-png`, `--lcd-text`, `--wav`, `--tape-in/-out`, `--ce158-pty/-rx/-tx`, `--run-after`, `--rom`, `--ce1600p-rom`, `--save-dir`, `--modules-dir`, `--check-preset` | `docs/inventory/calc-u-1600.md`, Tests |
| Calc-U-1600 probes | plotter, power and UART diagnostics | same |
| `dap_smoke.py`, `--shots` scenarios | DAP protocol smoke; screenshot scenarios (frozen emulation, isolated settings) | same |

## Responsibilities

- One program, `calcu` (Q-1), with `--product`,
  running the same library and app-layer code as the frontends (ADR
  0009: the same code in virtual time).
- One-shot subcommands and `serve` mode (ADR 0026); `serve` is also what a
  frontend exposes for attach (ADR 0033).
- Every function in the table above, as a common command or a
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

**Decided (2026-11-10): option (a).**

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

**Decided (2026-11-10): option (a).**

Options:
- (a) **Recommended:** common commands at the top level; commands that
  exist for one product only live under the product's name (`ti59 asm`,
  `ti59 keycodes`, `ti59 coverage`; `pc1600 dump-basic`, `pc1600 tape`,
  `pc1600 serial`), and `--product` selects the product for one-shot runs.
  A command present in both products (for example `dump`) is common, with
  product-specific output where the hardware differs.
- (b) Flat command list with product prefixes in the names.

### H-3: Attach transport and safety

**Decided (2026-11-10): option (a).**

Options:
- (a) **Recommended:** the serve protocol is line-based text (one command
  per line, a reply per line, `--json` replies on request) over stdin and
  stdout for `serve`, over TCP bound to localhost for an app (the iOS
  simulator shares the Mac's network), and over a WebSocket that the
  page opens to a localhost port given by a development flag. No
  authentication beyond localhost; never in release builds (I-3).
- (b) A binary or JSON-RPC protocol: harder for an agent to type and
  read.

### H-4: Which frontends get attach, and when

**Decided (2026-11-10, in the review; approved by the user).** ADR 0033
carries a note.

Facts: ADR 0033 says every frontend can open its app session to the
serve protocol. Its reason is the iOS simulator, where UI automation
often fails. The Web frontends have working Playwright suites (28 and 21
tests, in CI); the Qt6 frontends have screenshot scenarios.

Options:
- (a) **Recommended:** the protocol and the transports of H-3 stay as
  designed, so any frontend **may** offer attach. The migration builds
  it for the Swift frontend (iOS simulator and macOS); Qt6 and the web
  get it when a need shows.
- (b) All three in the migration: two transports built before anyone
  needs them.

## Fit table

| | TI-59 (58, 58C) | PC-1500 (1500A) | PC-1600 | PC-1403 (paper) | PC-1350 (paper) |
|---|---|---|---|---|---|
| Common commands | key, type (—), wait, expect, lcd, dump, peek, break, trace, screenshot | same, with `type` | same | same | same |
| Product commands (H-2) | `asm` (ASM overlay), keycode (dis)assembler, ROM coverage, printer lines, card dump | BASIC dump, tape in/out, CE-158 serial | BASIC dump, tape, serial, host drive, probes | — | — |
| Attach (ADR 0033, H-4) | Swift (iOS simulator, macOS) in the migration; Qt, web later | Qt, web later | Qt, web later | — | — |

## Open questions

- Q-1 (answered 2026-11-10): the program is called `calcu`.
- Q-2 (phase 6, with the skills): The Python step-by-step driving of `tools/headless/` becomes a
  Python script talking to `serve` over a pipe; whether a small Python
  helper ships with the harness is decided in phase 6 with the skills.

## Decision log

| Date | ID | Decision |
|---|---|---|
| 2026-11-10 | H-1 | The serve protocol's commands are the preset step vocabulary plus inspection and control commands; a preset is a recorded harness session. |
| 2026-11-10 | H-2 | Common commands at the top level; product-only commands under the product's name; `--product` for one-shot runs. |
| 2026-11-10 | H-4 | Any frontend may offer attach; the migration builds it for the Swift frontend only, Qt6 and the web follow when needed. |
| 2026-11-10 | H-3 | Line-based text (`--json` on request) over stdin/stdout, localhost TCP for apps, a WebSocket from the page in development builds; localhost only; never in release builds. |
| 2026-11-10 | Functions to keep (amended, user, phase 4b, batch 3 gate) | The Calc-U-1600 probes and the Calc-U-59 calendar scripts stay scripts that use `calcu serve`; they do not become commands. Product commands are the index rows C59-9 and C1600-5. |
| 2026-11-10 | H-1 (completed, user, 2026-11-10, batch 7 gate) | `serve` takes `--dap <port>` with Calc-U-1600: the debug server is hosted without a wall clock, the machine running unpaced whenever a debugger has it running. |
| 2026-11-10 | H-2 (completed, user, 2026-11-10, batch 8 gate) | Product commands of Calc-U-59 (C59-9): `ti59 coverage`, read from the debugger core's `profile`, `line-times`, `run-time`, `hot-steps`, `step-trace`, `step-cost`, `keycodes list` and `keycodes assemble`, which reads what `list` writes; a listing uses the key names of typing. The coverage scenarios and the fast-mode matrix stay scripts. Product commands of Calc-U-1600 (C1600-5): `pc1600 dump-basic`, `inspect <view>`, `tape record <path>` and `tape status`, `serial <port> feed|capture|link|report` for the CE-158's port and the PC-1600's own RS-232C port, `host-drive <folder>|off`. No serial port has a host link in a harness run unless `link` asks. |
