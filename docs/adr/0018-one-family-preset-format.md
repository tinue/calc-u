# One preset format for the family

Status: accepted (2026-11-09); partly superseded 2026-11-10 by ADR 0037 (no v1 reader; Calc-U-59 version 1 presets are refused with a hint to upgrade)

The family has one preset format, versioned and YAML-shaped like
Calc-U-1600's, with product-specific sections where a model family needs
them (for example the TI-58/59 partition or cue cards). It is parsed by
the one preset loader in the library (Briefing §1.3). Calc-U-59 writes it
as its next preset version and reads its sectioned version 1 through a
v1 reader forever (Briefing §1.2); Calc-U-1600 moves to it without
compatibility. Users of every product see one preset language. The syntax
itself is the preset contract in phase 4, starting from Calc-U-59's
`ideas/PresetConvergence.md` and Calc-U-1600's format.

## Considered options

- One engine over two syntaxes: less change for users today, but two
  languages to document and keep in step, against Briefing §1.5 ("a
  preset means the same thing everywhere").
