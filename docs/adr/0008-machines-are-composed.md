# Machines are composed of components and peripherals

Status: accepted (2026-11-09)

Every core builds its machines by composition: a machine is assembled from
components and from peripherals plugged into connectors, and components
and peripherals expose capabilities (ADR 0007). The models of a family
differ by their parts list, not by separate cores. This is Calc-U-1600's
structure (separate CPUs, bus arbiter, memory, peripherals on pin-numbered
connectors); Calc-U-59 folds almost the whole calculator into `TMC0501`.
How finely each machine is split is a separate decision (modelling
depth); a coarse component such as today's `TMC0501` is allowed.

## Considered options

- One core per model family with its own internal structure, only the
  machine interface common: rejected; two structural patterns, and the
  capabilities would have no common place to come from.
