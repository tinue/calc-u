# The harness is one CLI with a session server

Status: accepted (2026-11-09)

The harness is one C++ program for the family, built on every platform by
the root CMake project (ADR 0003). It has one-shot subcommands (for
example `run`, `trace`, `dump`, `lcd`, `asm`, with `--product`) that print
plain lines an agent can read back, or JSON with `--json`, and a `serve`
mode that keeps a session open and takes one command per line, for work
that drives a machine step by step. It keeps every capability today's
tools offer, because a check of the skills, `CLAUDE.md` files, developer
docs and memory (2026-11-09) found all of them in agent use: Calc-U-59's
`tools/headless/` (ROM traces, the Python `Machine` driven step by step),
`tools/agentic/` (program runs, timing, profiling, keycode assembler) and
`read_trace.py`; Calc-U-1600's `pc1500_cli` and `pc1600_cli`, the plotter,
power and UART probes, `dap_smoke.py` and the `--shots` scenarios.

## Considered options

- CLI plus a Python module over the library: keeps today's Python
  driving, but two surfaces to document, and ctypes bindings are what makes
  `tools/headless/` macOS-only today.
- CLI only, step-by-step work as presets: loses the interactive driving
  agents use for ROM documentation.

## Consequences

- `tools/headless/`, `tools/agentic/`, the Calc-U-1600 CLIs and probes
  are replaced by the harness during migration; their skills and memory
  are updated in the same steps (Briefing §1.11).
- One-time verification setups (`dev/loader-matrix`, `dev/tape-matrix`)
  stay as records; `check_presets.sh` becomes a harness subcommand.
