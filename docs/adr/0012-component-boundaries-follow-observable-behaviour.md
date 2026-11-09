# Component boundaries follow observable behaviour

Status: accepted (2026-11-09)

A component's boundary follows a real chip or block boundary, and a
machine is split exactly where some observable behaviour depends on that
boundary; modelling every chip is not a goal in itself. Chips that act as
one towards the rest of the machine may stay one component: the TI-58C's
two ROM chips are stacked with every leg connected and look like a single
chip to the CPU, so they need not be modelled separately. Where parts can
disagree, they must be separate: the CPU and the ROM chips each drive the
D lines and can have different opinions of where they are in a WAIT
cycle, which today's single `TMC0501` component cannot express, so
Calc-U-59 will eventually split the CPU from its ROM chips. Coarse
components are allowed; refining one never changes the machine interface
or the capabilities the machine exposes. The depth of each machine is
recorded in its machine descriptor (phase 4).
