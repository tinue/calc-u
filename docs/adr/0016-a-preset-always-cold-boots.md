# Loading a preset always builds its machine from a cold boot

Status: accepted (2026-11-09)

A preset fully describes its machine: loading it builds the machine anew
from a cold boot, with the model, peripherals and modules the preset
names, so the same preset gives the same machine every time. This is
Calc-U-1600's rule; Calc-U-59 today applies a preset to the running
calculator and keeps whatever the preset does not name (an inserted
module, the printer).

## Consequences

- To keep Calc-U-59 version 1 presets behaving as today (Briefing §1.1,
  §1.2), the v1 reader fills in what a v1 file leaves out from the
  current state at load time (for example the Solid State ROM module in
  the settings), and writes it explicitly when the preset is saved in the
  current version.
- Calc-U-59 presets lose the "add to the running calculator" use; where
  that is needed, it becomes a separate action, not a preset load.
