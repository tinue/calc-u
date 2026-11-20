# Calc-U-59 reference

This chapter describes the files Calc-U-59 reads and writes, each in its
current version, and the keyboard map. What every product of the family
shares (the grammar of the preset format, the rules of the settings
file, the glossary) is in the [shared reference](../../../library/help/10-reference.md);
the pages below add what is Calc-U-59's own.

| File | Extension | Version | Where |
|---|---|---|---|
| Preset | `.ti59`, `.ti58`, `.ti58c` | `format-version: 2` | [Preset sections](10-preset-sections.md) |
| Keystroke program file | `.keystrokes.yaml` | `format-version: 1` | [Keystroke program files](10-keystroke-programs.md) |
| Magnetic card (TI-59) | `.magnetcard.yaml` | `format-version: 3` | [Media files](10-media-files.md#magnetic-card) |
| Constant memory file (TI-58C) | `ti58c.memory.yaml` | `format-version: 1` | [Media files](10-media-files.md#constant-memory-file) |
| Solid State ROM module | `.rom.yaml` | `format-version: 1` | [Media files](10-media-files.md#solid-state-rom-module) |
| Cue card block (in presets, cards, modules and the constant memory file) | — | as the file it stands in | [Media files](10-media-files.md#the-cue-card-block) |
| ASM overlay | `.asm` | `Calc-U-59 ASM 2.0` | [ASM overlay](10-asm-overlay.md) |
| Settings file | `Calc-U-59.ini` | `format-version = 1` | [below](#the-settings-file) |
| Keyboard map | (built in) | | [Keyboard map](10-keyboard-map.md) |

The full specification of every format, for those who write tools, is
in the source repository:
[docs/formats/](https://github.com/tinue/calc-u/blob/main/docs/formats/README.md).

## The settings file

Calc-U-59 keeps its settings in one file, `Calc-U-59.ini`, in the folder that
[Settings](../../../library/help/08-settings.md#the-settings-file)
names for each platform, with `<Product>` as `Calc-U-59`.

The rules of the file (its sections, how a
missing or unknown key is treated, which keys the product writes itself)
are those of the [shared reference](../../../library/help/10-reference.md#the-settings-file)
and the [shared Settings chapter](../../../library/help/08-settings.md); Calc-U-59's
own keys and the names it gives the family's keys are in
[Settings](08-settings.md).
