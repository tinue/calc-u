# Headless harness

Drives the TI machine from Python (ctypes) through the machine interface
(`calcu::machine::Machine`, built with `tiCatalogue()`; C59-1) and the
adapters of `core/TIAdapters.hpp`, to capture ROM traces of keystroke
sequences — the tool behind the trace annotations in
`rom/*-commented.asm`. `build.sh` compiles a `.dylib` with clang++, so the
harness runs on macOS only.

```bash
tools/headless/build.sh                                  # once (auto-run on first import)
uv run python tools/headless/ti59.py 5 + 7 =             # trace each key, idle loop collapsed
uv run python tools/headless/ti59.py --full 5            # don't collapse the idle loop
uv run python tools/headless/ti59.py --boot              # also print the power-on trace
uv run python tools/headless/ti59.py --summary 5 + 7 =   # distinct PCs per key with hit counts
uv run python tools/headless/ti59.py --program 2,99,91 RST R/S   # keycodes written to step 000…
uv run python tools/headless/ti59.py --printer --module ML 2nd Pgm 0 1 …
uv run python tools/headless/ti59.py --variant TI58C 5 + 7 =     # TI-58C ROM (labels: rom/TI58C-labels.asm)
```

Each trace line: `PC OPCODE dDIGIT [c0 = COND active] mnemonic  changes…`,
where *changes* lists the registers / SCOM rows (`S<n>`, MSD first) the
instruction modified. Labels from `rom/TI59-labels.asm` are shown inline.

Key names: the 45 key IDs of C59-1 AC-9 (`2nd`, `INV`, `lnx`, `x⇄t`, `x²`,
`√x`, `1/x`, `yˣ`, `÷`, `×`, `+/-`, …) and their other spellings of
`docs/formats/preset.md` §8.1 (`x<>t`, `x^2`, `sqrt`, `y^x`, `/`, `*`, …;
`x2` is gone), a matrix code (`83`), the second functions of `SECOND`, plus
`PRINT` / `PAPER` for the PC-100's own buttons; 2nd functions are two
presses, as on the hardware. The library resolves a name through the
descriptor (`h_key_down`, -1 for an unknown name). Library use:
`from ti59 import Machine` — `Machine().key("5")` returns per-instruction
`HRec` records (post-execution CPU state).

`coverage.py` maps keystroke scenarios to the ROM words they execute
(`build`, `show`, `unique`, `gaps`; see its docstring).

C API (`capi.cpp`): `h_new(model)` and `h_load_model_roms` build the machine
(the state after a cold boot); `h_key_down(name)` / `h_key_up(name)`;
`h_stepn(n)` runs n instructions (n × `advance(1)`); `h_elapsed` is the tick
count / 32 (weighted instructions; it goes on across a reset); `h_reset` is
a power cycle (2.3.0's core reset); `h_display(segments12, dp12,
brightness12)` is the display output since the last call (index 0
rightmost); `h_instruction_hz(model)` is the tick rate / 32; records,
snapshot, RAM, program, card and printer calls go through the adapters.
`TI59HEADLESS_LIB` points `ti59.py` at a library built elsewhere (the CTest
`CalcU59HeadlessSmoke`).

For running, timing and profiling whole `.ti59` programs (rather than
tracing the ROM), see `tools/agentic/`.
