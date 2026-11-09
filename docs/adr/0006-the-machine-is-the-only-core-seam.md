# The machine is the only seam into a core

Status: accepted (2026-11-09)

Everything above a core (app layer, library, harness, debugger) reaches
the emulated hardware only through the machine: the machine interface and
what a machine exposes through it (its connectors, capabilities and debug
targets). Nothing above a core uses core types or touches components
directly. Calc-U-59 already keeps its app layer away from `TMC0501`;
Calc-U-1600's app layer branches 89 times between a PC-1500 and a PC-1600
pointer (`Shared/MachineSession.hpp`), which this removes. One door per
core keeps the app layer and the library model-independent, so a new model
or product adds a machine, not branches above it.

## Considered options

- Several seams (debugger and peripherals reaching into the core
  directly): rejected; every extra seam would have to stay stable for
  every product.
