# Peripherals are keyed by connector

Status: accepted (2026-11-09)

A peripheral declares the pin-outs it fits and sees only the signals of
the connector it is plugged into; the machine wires its connectors to its
components. A peripheral therefore works in every machine that offers its
pin-out: the CE-161 fits the PC-1500 memory slot and both PC-1600 slots
without knowing which machine it is in. This is Calc-U-1600's rule ("cards
know only the bus", `Decisions.md`), now for every product.

## Considered options

- Keyed by machine (a peripheral written for one machine and allowed to
  reach into it): rejected; no reuse across models, and it would open a
  second seam into the core (ADR 0006).
