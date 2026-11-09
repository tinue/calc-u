# Settings hold preferences; startup presets hold machines

Status: accepted (2026-11-09)

A product's settings hold the user's preferences, the startup model and
each model's startup preset, nothing about the machine's hardware. The
machine a model starts with comes from its startup preset, which builds it
from a cold boot (ADR 0016); a product does not remember the last
modules or peripherals by itself. Calc-U-1600 works this way today;
Calc-U-59 today keeps the Solid State ROM module in its settings.

## Consequences

- Without a startup preset, Calc-U-59 starts as a TI-59 with the Master
  Library module inserted, as the TI-59 came out of the box. On the first
  start of the new release, a Calc-U-59 user's stored module choice is
  converted into a startup preset, so their setup survives (Briefing
  §1.2, settings conversion).
- A module chosen while running lasts until the product quits; to keep it
  across restarts, the user saves a startup preset. Whether a module change
  keeps the RAM (Calc-U-59 today: a reset that keeps memory) or cold boots
  (Calc-U-1600 today: the machine restarts) is open for the peripheral and
  connector contract (phase 4).
