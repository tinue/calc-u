# The TI-59's printer and module slot become functional connectors

Status: accepted (2026-11-09)

In the migration, the PC-100C printer and the Solid State ROM module
become peripherals on connectors of the TI-58/58C/59 machines, with a
functional pin-out: named signals and what they carry, not contact-level
timing. Today both live inside `TMC0501` (the printer as a flag,
`setPrinterConnected`). As peripherals they follow the same model as in
Calc-U-1600 (ADR 0008, 0010), which also serves family coherence: a
printer is a peripheral in every product. Contact-level detail can follow
later without changing the interface; the sources are the datamath
TMC0540 page for the slot and the hand count of the PC-100C contacts
(Briefing §1.9). The magnetic card reader stays inside the calculator but
is exposed as the media capability (ADR 0007).

## Considered options

- Contact level now: most faithful, but more work than the migration
  needs, and which PC-100C spring is unconnected is still unknown.
- Keep both implicit: rejected; the TI-59 would be the only machine
  without peripheral objects.
