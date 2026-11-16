---
name: calc-u-59-presets
description: Use this skill when writing or editing Calc-U-59 presets of format 2 (.ti59 / .ti58 / .ti58c), keystroke program files (.keystrokes.yaml), the bundle file examples/bundle.yaml, or keys: steps with the TI key IDs. Covers where the format is specified, the keycode vs. key ID distinction, the TI sections (partition:, program:, data:, cue-card:) and how to check a preset with calcu.
user-invocable: true
---

# Calc-U-59 presets

The format is specified once for both products, in
**`docs/formats/preset.md`** (repository root): the header keys, the TI
sections `partition:` (§4.3), `program:` (§4.1, the keycode notation;
§4.1.1 keystroke program files), `data:` (§4.2), `cue-card:` (§4.4), the
steps of a `keys:` section (§5), speed and waiting (§6), which keyword
applies to which model (§7) and the TI key IDs (§8.1). The cue card block
is **`docs/formats/cue-card.md`**, the bundle file **`docs/formats/bundle.md`**.
Read them before writing or editing a preset. Presets of 2.3.0 (no
`format-version: 2`) are refused with the hint; there is no converter
(ADR 0037).

The code: the library's loader (`library/presets/`, LIB-6) with
Calc-U-59's rules `app/TIPresetRules` (idle, `expect:`, `screenshot:`,
`slot-1` by `name:` only) and keywords `app/TIPresetKeys` (the four
sections, `printer-trace:`); the keycode reader `app/Keycodes`; the cue card
reader `app/CueCardYaml`. The code wins where it and these notes disagree;
the format documents win over the code.

## Mistakes that come up most

- **`program:` takes keycodes, `key:` takes key IDs.** Digit 1 is keycode
  `01` in `program:` and key ID `1` in a `- key:` step; `Pgm` is the one step
  `36` and the key ID `Pgm` (2nd, then LRN). A two-digit number such as `12`
  in a `key:` step is an error: `1 2` is two keys.
- **A second function is one key ID** (`- key: Op`, `- key: 2nd Op`), never
  `- key: 2nd` and `- key: Op` as two steps (that presses 2nd twice).
- **Compact or listing.** A compact keycode line holds two-digit keycodes
  only, one after the other; a listing line starts with a step number of
  three or more digits, then one keycode, then the mnemonic, which is not
  read (`473 00 0`). A word or a one-digit token on a compact line refuses
  the preset. There is no `...` gap marker: write `# ...` as a comment.
- **Only what is listed is written.** A `program:` or `data:` section after
  a `keys:` section changes the machine in the middle of the preset and
  keeps everything it does not list (C59-2 AC-7).
- **Full speed by default.** A preset runs at full speed unless a
  `- regular-speed:` comes first; an upgraded example starts its first
  `keys:` with `- regular-speed:` where 2.3.0 ran it at regular speed. Adv
  needs regular speed, or the paper runs away.
- **The last item is not waited for.** Add `- let-it-finish:` last when the
  run the last key starts is part of the result. A paused machine is never
  idle: nothing may follow a key that makes it pause (pause-on-start).
- **`data:` values** are decimal numbers (`-1.5e-3`); a whole number keeps
  its first 13 digits, cut, not rounded; registers 60–99 exist on the
  TI-59 only; `H00`–`H03` on the TI-58C only, and they are not 60–63.
- **The machine is what the header names:** no `slot-1` means no module, no
  `printer:` means no PC-100C (the upgraded examples name `slot-1: { name:
  ML }` and `printer: PC-100C`, what a fresh 2.3.0 had).
- **`expect:` on a TI compares a number** with X in 13 digits (after `π`,
  `3.14159265359` passes, `3.141592654` fails); no regular expressions.

## Print-code register values

Some programs store text for the PC-100C: a register holds up to five
2-digit character codes (`Op 01`–`Op 04`), each a `row col` pair of the
printer's 8 × 8 table (`PRN_CODE[64]` in `core/TMC0501.cpp`);
`3013351523` → 30 13 35 15 23 → `MARCH`. A value with fewer than five
characters has fewer digits; keep it as printed.

## Checking a preset

- `calcu --product 59 check-preset <file>…` reads it as a load would;
  `calcu --product 59 preset <file> then wait then lcd then paper text`
  carries it out in emulated time (products/calc-u-59/CLAUDE.md,
  "Harness"); `ti59 program 000 count=<n>` and `ti59 registers <NN>…` read
  what it wrote.
- The examples are checked by `calcu_preset_check.CalcU59Presets`
  (Frequent) and against their 2.3.0 reference runs by
  `CalcU59ReferenceRuns` (Longer; `tests/reference_runs_tests.cpp`).
- The four examples with magnetic cards (`globe-print`,
  `historic/characters-fastmode`, `historic/pdcm-card-tricks`,
  `historic/pdcm-make-cards`) stay in the 2.3.0 format until M-55.

## Transcribing magazine listings (52-Notes)

Lessons from the 52-Notes calendar programs
(`examples/calendar/calendar-01-Weinberger.ti59` and the rest of the series, see
`examples/calendar/CALENDAR_PROGRAMS.md`). The listings use their own shorthand
(`S29`/`R13` for STO/RCL, `rtn` for the merged RTN, `CP`, `x=t`, `xGEt`,
`Op`, `Adv`, `Pgm` for 2nd functions); don't assume it carries over to
other sources.

- **Keep packed print-code registers as printed.** A register with four
  characters has eight digits; don't pad it to ten with `00`.
- **An OCR'd "xXt" can be `x=t` (67) or `x⇄t` (32), and the step count
  doesn't decide it.** A branch destination can be a single step (any
  keycode but 00–09 and 40 is a label, `app/KeyNames.hpp`
  `argumentSteps`), so `x=t` plus its label takes the same two steps as
  `x⇄t` plus the next instruction. Decide each occurrence by what makes
  sense at that point: a branch, or an exchange.

## Related

- XCUITest scenario files and verifying a load: `calc-u-59-uitesting`.
- The bundled examples (`examples/bundle.yaml`) and how each frontend loads
  them: `calc-u-59-shared`, `calc-u-59-web`.
