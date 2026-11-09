# Settings files are INI

Status: accepted (2026-11-09)

Each product's settings file is an INI file (`[section]` and
`key = value`), for example `Calc-U-59.ini` and `Calc-U-1600.ini`, as MAME
does with `mame.ini`. Settings are flat by nature, and INI is the easiest
to edit by hand and to document. Where the file lives on each platform and
the lookup hierarchy are part of the settings and paths contract
(phase 4).

## Considered options

- YAML-shaped like presets (ADR 0018): one syntax family, but heavier to
  edit by hand for flat key-value data.
