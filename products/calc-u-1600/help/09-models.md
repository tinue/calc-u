# What differs by model

[Models and machine control](../../../library/help/03-models-and-machine-control.md)
describes the model, the variant, Reset, Cold boot and the power cycle for
every product. This page says what they mean on the PC-1500, the PC-1500A
and the PC-1600.

## ROM variants

| Model | Variants | In the Machine menu |
|---|---|---|
| PC-1500 | ROM A01, A03 or A04 (the default) | **ROM Revision** |
| PC-1500A | one ROM, no choice | — |
| PC-1600 | ROM New (the default) or Old | **ROM Version** |

The CE-1600P has its own ROM, New (the default) or Old: **Machine ▸
CE-1600P ROM** while the CE-1600P is plugged in
([The plotters](05-peripherals.md#the-plotters)).

## Reset, power cycle and Cold boot

Each of them ends when the machine is ready for keys again.

| | PC-1500, PC-1500A | PC-1600 |
|---|---|---|
| **Reset** | the reset button: the memory is kept | the reset button: the internal memory is kept |
| **Power cycle** | switched off and on again: all memory is kept | the same |
| **Cold boot** | the machine's own memory is emptied, as taking the batteries out would (the PC-1500 has no ALL RESET of its own) | the internal memory is emptied: the PC-1600's ALL RESET |

**Memory modules.** A Reset and a power cycle keep what is in a module. A
Cold boot also empties a RAM module that has no battery of its own (CE-151,
CE-155), as the real device would; a battery-backed module (CE-161,
CE-1638, CE-163F, CE-1600M, CE-1601M, 256K custom, superRAM r4, and your own with
`battery: true`) keeps its content.

**The clock.** After a Reset or a Cold boot the calculator's clock is set
from your computer. A Reset of the PC-1500 or PC-1500A first puts it back
to 1 January 2000; a Reset of the PC-1600 leaves it.

## What causes a power cycle

A power cycle is what the real machine needs to find a changed module or
peripheral, because its ROM looks for them at power-on. It happens when:

- you choose a module in a slot picker (also the empty slot);
- you attach or detach a peripheral (CE-150, CE-158, CE-1600P);
- you mount or unmount a folder as a drive (PC-1600,
  [A folder as a drive](09-host-drive.md));
- you Name & Save a CE-161 with its write-protect switch set: the module
  then reads as a program module.

The memory is kept. When the layout of the memory changed, the ROM finds
it and asks you to set BASIC up again: the calculator shows
`NEW0? :CHECK`, and you type `NEW 0`, as on the real device. On a PC-1500,
plugging a program module in or out moves BASIC's program area
([Media](04-media.md)).

## A model switch

A model switch gives you a fresh machine with no modules and no
peripherals, cold booted. A startup preset for that model
([Settings](08-settings.md#startup-presets)) is applied straight after the
switch. Reset, Cold boot and a model switch stop whatever runs (a preset, a
load).
