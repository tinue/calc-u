# Calc-U-1600 reference

This chapter describes the files Calc-U-1600 reads and writes, each in
its current version, and the keyboard map. What every product of the
family shares (the grammar of the preset format, the rules of the
settings file, the glossary) is in the [shared reference](../../../library/help/10-reference.md);
the pages below add what is Calc-U-1600's own.

| File | Extension | Version | Where |
|---|---|---|---|
| Preset | `.pc1500`, `.pc1500a`, `.pc1600` | `format-version: 2` | [Preset sections](10-preset-sections.md) |
| Module definition (a memory module, and a saved battery-backed module) | `.module.yaml` | `format: memory-module`, `format-version: 1` | [Module definition format](10-module-definition-format.md) |
| Floppy disk | `.floppy.yaml` | `format-version: 2` | [Media files](10-media-files.md#floppy-disk) |
| Cassette definition (with its sound file) | `.cassette.yaml` and `.wav` | `format: cassette`, `format-version: 1` | [Media files](10-media-files.md#cassette-definition) |
| Settings file | `Calc-U-1600.ini` | `format-version = 1` | [below](#the-settings-file) |
| Keyboard map | (built in) | | [Keyboard map](10-keyboard-map.md) |

What a module definition expresses, and why, is described in
[Module definitions](09-module-definitions.md); the format page here is
how it is written.

## The settings file

Calc-U-1600 keeps its settings in one file, `Calc-U-1600.ini`, in the folder that
[Settings](../../../library/help/08-settings.md#the-settings-file)
names for each platform, with `<Product>` as `Calc-U-1600`.

The rules of the file (its sections, how a
missing or unknown key is treated, which keys the product writes itself)
are those of the [shared reference](../../../library/help/10-reference.md)
and the [shared Settings chapter](../../../library/help/08-settings.md); Calc-U-1600's
own keys and the names it gives the family's keys are in
[Settings](08-settings.md).
