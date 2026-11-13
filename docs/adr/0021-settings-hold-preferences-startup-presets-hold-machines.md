# Settings hold preferences; startup presets hold machines

Status: accepted (2026-11-09); the open point on module changes was
settled 2026-11-10: a power cycle (`docs/contracts/peripherals.md`, P-1).
Amended 2026-11-10 (user, `docs/contracts/capabilities.md`, C-3): no
bundled startup preset restores `ti58c.mem`. The TI-58C keeps its
constant memory across starts only while no startup preset is set for
it; a startup preset cold boots it, and `ti58c.mem` follows. The
sentence on the TI-58C under "Consequences" no longer applies.
Amended 2026-11-13 (user, F-8 of `docs/walkthroughs/presets.md`): a
TI-58C a preset or a startup preset built never writes the constant
memory file, which is now `ti58c.memory.yaml` (`docs/formats/media.md`
§4).

A product's settings hold the user's preferences, the startup model and
each model's startup preset, nothing about the machine's hardware. The
machine a model starts with comes from its startup preset, which builds it
from a cold boot (ADR 0016); a product does not remember the last
modules or peripherals by itself. As long as no startup preset is set, a
product starts with its built-in default machine. This is how Calc-U-1600
works today; Calc-U-59 today keeps the Solid State ROM module in its
settings.

## Consequences

- Calc-U-59's built-in default is a TI-59 with the Master Library module
  and no printer, as the TI-59 came out of the box.
- Calc-U-59 bundles three startup presets, one per model, with sensible
  defaults, as Calc-U-1600 bundles its own: Master Library module, printer
  attached, and for the TI-58C its constant memory restored from
  `ti58c.mem` if that file exists. The user picks one in the settings.
- A user's stored module choice is not converted; it falls away with the
  module setting.
- A module chosen while running lasts until the product quits; to keep it
  across restarts, the user sets a startup preset. Whether a module change
  keeps the RAM (Calc-U-59 today: a reset that keeps memory) or cold boots
  (Calc-U-1600 today: the machine restarts) is open for the peripheral and
  connector contract (phase 4).
