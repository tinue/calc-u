# A small machine interface plus generic capabilities

Status: accepted (2026-11-09)

The library defines a small machine interface with what every machine
has: model and variant, reset and cold boot, advancing emulated time, key
down and up, display output, sound output, its connectors and its debug
targets. Everything else a machine offers is a **capability**: a generic
kind of function that a component or peripheral exposes, such as media
(magnetic card reader, cassette, floppy drive), real-time clock, serial
port or printer output. The library handles each capability the same way
for every product: the TI-59 card reader and the PC-1600 cassette are both
media, and a future SC61860 tape reuses the same capability. Extensions
named after a product exist only for functions that are really one of a
kind. This follows MAME's device interfaces (`device_image_interface`,
`device_rtc_interface`, …), which keep the machine interface small for
hundreds of systems.

## Considered options

- One complete interface with every function of every machine, optional
  ones returning "not supported": rejected; it grows with every product
  (the trap MAME avoided).
- No runtime interface, the library generic over each product's machine
  type (templates): rejected; it gives up a generic debugger, frontend and
  harness.

## Consequences

- The capabilities and their contracts are written in phase 4 (machine
  interface and descriptor, peripheral and connector model, media).
- Magnetic card handling moves from Calc-U-59-specific code to the media
  capability.
