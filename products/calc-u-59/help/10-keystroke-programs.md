# Keystroke program files

A keystroke program file holds one program of the TI-58, TI-58C or
TI-59, as keycodes, in a file of its own: suffix `.keystrokes.yaml`,
format version 1. A preset uses it with `program: { file: … }`
([Preset sections](10-preset-sections.md#program)), so one program can
serve several presets.

```yaml
format: ti-keystroke-program
format-version: 1
program-name: Loan
description: Monthly payment of a loan
keycodes: |
  000 76 LBL
  001 11 A
  002 42 STO
  003 01 01
```

| Key | Required | Value |
|---|---|---|
| `format` | yes | `ti-keystroke-program` |
| `format-version` | yes | `1` |
| `program-name` | yes | The program's name |
| `description` | no | One line about the program |
| `keycodes` | yes | The program, in the keycode notation of a preset's `program:` section: compact lines of two-digit keycodes, or listing lines with the step number, the keycode and the mnemonic ([Preset sections](10-preset-sections.md#program)) |

- The file follows the family's YAML rules
  ([Reference](../../../library/help/10-reference.md#syntax)): an unknown
  key, a tab or a bad keycode refuses the file as a whole.
- Keycodes are not key IDs: the digit 1 is keycode `01`.

## Opening a keystroke program file

A keystroke program file can also be opened on its own: dropped on the
window or opened with your system's Open With. Its program goes into the
running calculator, each listed step where it belongs (a compact line
from step 000), without a cold boot: the module, the printer, the
registers and the steps the program does not list stay as they are. On a TI-58 or TI-58C, a step above 479 is an error.
