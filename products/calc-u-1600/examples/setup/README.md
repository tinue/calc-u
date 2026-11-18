# examples/setup

See also `help/09-module-definitions.md` for
the workflow below.

These files are not demos to look at — they are **preparation scripts**. Each one
drives the emulator through the steps needed to get a memory module into a
known, useful state: format it, split it as a RAM disk, load a set of BASIC
programs, write a config file, and so on.

## Workflow

1. Run one of these preset files. It boots the right machine with the right
   module fitted and types/loads everything into the module.
2. Once the module is set up, do one of two things with it:

   - **Save the module.** End the preset with
     `- saveas: { place: slot-2, as: template|live, name|file: … }`: by
     name into your save folder, or as a file next to the preset
     (`file:`), which a "real" preset then names with
     `slot-N: { file: … }` or the step `- floppy: { file: … }`
     (`docs/formats/preset.md` §3, §5.2 at the repository root).
     `examples/dwx/make_diskworks_media.pc1600` does this for
     `examples/dwx/DiskWorks.pc1600`.

   - **Dump the module.** Use the Inspector's **Module YAML** button to emit
     its content, and paste the block it prints into a module definition's
     `initial-content:` (your own copy of a bundled one from
     `frontends/qt6/resources/cards/`, e.g. `ce1601m.module.yaml`). A fresh module from
     that definition then comes up already populated.

## Contents

| File | Prepares |
| --- | --- |
| `ce1638_bankswrm.pc1500` / `.pc1500a` | PC-1500 with a real CE-1638 128K module, bank-switch test loaded |
| `firmware_bootstrap_util*.pc1500a` | Firmware bootstrap / update utilities staged into a module |
| `update.bas`, `updaterm.bas`, `util_*.bas`, `utilrm_*.bas` | BASIC sources loaded by the bootstrap presets |
| `ce1638_bankswrm_fixed.bin` | Machine-code payload for the CE-1638 bank-switch test |
