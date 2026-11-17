# What differs by model

Reset, Cold boot and the power cycle after a change of hardware are the
family's ([Models and machine
control](../../../library/help/03-models-and-machine-control.md)). What
each of them keeps depends on the model:

| | TI-59, TI-58 | TI-58C |
|---|---|---|
| **Reset** (Machine ▸ Reset) | the program, the data registers and the partition kept; everything else as after switching on | the same as a power cycle |
| **Power cycle** (a module chosen, the PC-100C attached or detached) | the memory lost: as the ROM's start-up leaves a calculator whose memory was lost | the constant memory kept (all of its memory, H00 to H03 included); everything else as after switching on |
| **Cold boot** (Machine ▸ Cold boot) | all memory zero, then the ROM's start-up | the same, the constant memory included |

"Everything else as after switching on" means, among other things: a
running program has stopped, and a library program that was selected no
longer is.

Through all three, on every model:

- the PC-100C and the module stay plugged in;
- the cue card that is fitted stays, and a fitted magnetic card can be fed
  again ([Magnetic cards](04-magnetic-cards.md)); a card in the reader at
  that moment is taken out as by Eject Card;
- the printer's TRACE switch keeps its position;
- an ASM overlay stops running; a Cold boot also removes it
  ([Debugger](07-debugger.md)).

A model switch, and every preset, builds a new calculator from a cold
boot. A TI-58C built at a start or a model switch without a startup
preset gets its constant memory back from its file
([Constant memory](09-constant-memory.md)).

## The models side by side

| | TI-59 | TI-58 | TI-58C |
|---|---|---|---|
| Memory | 960 steps or 100 registers | 480 steps or 60 registers | 480 steps or 60 registers, kept when switched off; H00 to H03 |
| Partition after switching on | 479.59 | 239.29 | 239.29 |
| Card reader | yes | no | no |
| Modules, the PC-100C | yes | yes | yes |
| Clock | regular | regular | about 15 % slower, as the real one |

A card fed to a TI-58 or TI-58C, by a drop or a preset, is refused: they
have no card reader. Registers 60 to 99 exist on the TI-59 only.
