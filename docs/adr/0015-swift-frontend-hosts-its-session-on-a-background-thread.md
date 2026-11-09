# The Swift frontend hosts its session on a background thread

Status: accepted (2026-11-09)

The Swift frontend runs its app session on a thread of its own (ADR 0009),
and the bridge is a message boundary: commands go in, display, sound and
state updates come out. SwiftUI stays smooth even when a heavier machine
such as a PC-1600 runs on iOS later, and the message shape matches the web
frontends, whose sessions already run in a Web Worker.

## Considered options

- The main thread: simplest and enough for the TI-59, but a slow
  emulation step would stall the UI.
