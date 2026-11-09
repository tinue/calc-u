# Calc-U

Calc-U is a family of emulators for vintage calculators and pocket
computers. This glossary fixes one meaning per term for every document,
skill, comment and visible label in the project.

## Products and machines

**Product**:
One separately released emulator of the family, with its own version,
releases and help site: Calc-U-59, Calc-U-1600.
_Avoid_: app, emulator (for the released whole)

**Family**:
All Calc-U products together, recognisable as one family by their users.
_Avoid_: suite

**Model**:
A real device a product reproduces and the user picks: TI-59, TI-58,
TI-58C, PC-1500, PC-1500A, PC-1600.
_Avoid_: machine variant, variant (for this)

**Model family**:
The models one product emulates together: the TI-58/58C/59 for Calc-U-59,
the PC-1500/1500A/1600 for Calc-U-1600.

**Variant**:
A revision of a model or device that the user can choose and that changes
behaviour, mostly a ROM version: PC-1500 ROM A01/A03/A04, PC-1600 ROM
New/Old, CE-1600P ROM New/Old.
_Avoid_: revision, version (for this)

**Machine**:
One emulated model at run time, as a whole: its chips, its connectors and
whatever is plugged into them. Switching the model builds a new machine.
_Avoid_: emulator, calculator (for the running whole)

**Core**:
A product's emulation of the hardware of its model family (CPUs, chips,
memory), which builds machines. It stays per product.
_Avoid_: engine, emulator

## Hardware

**Connector**:
The physical part a peripheral plugs into, with its form and contacts: the
40-pin connector, the 60-pin connector, the PC-100C printer connector.
_Avoid_: port, bus, slot (for the physical part)

**Pin-out**:
Which signal is on which contact of a connector, and what it means. One
connector can carry different pin-outs: the 40-pin connector is the
PC-1500 memory slot, PC-1600 Slot 1 or PC-1600 Slot 2.
_Avoid_: interface, port (for this)

**I/O port**:
An input/output address of a CPU, such as port 3CH.

**Serial port**:
An RS-232 connection of a peripheral, such as the CE-158's.

**Component**:
A modelled part inside a machine: a chip or a functional block such as the
display, the keyboard matrix or the clock.
_Avoid_: device, part (for this)

**Peripheral**:
Emulated hardware outside the machine's main unit that plugs into one of
its connectors: a printer, the CE-158 with its serial port, a floppy
drive, a cassette recorder, a module.
_Avoid_: device, card, periphery

**Printer**:
A peripheral that prints or plots: PC-100C, CE-150, CE-1600P.

**Module**:
A peripheral that holds memory (ROM or RAM) and plugs into a module slot.
The two kinds are the solid state ROM module and the memory module.
_Avoid_: card, memory card, software-defined card

**Solid State ROM module**:
A TI-58/59 module holding one ROM of the TMC0540 chip family, such as the
Master Library.
_Avoid_: Solid State Software module, library module

**Memory module**:
A Sharp module with RAM or ROM, such as the CE-161, CE-155, CE-1600M or
CE-502B.
_Avoid_: memory card, card, RAM card

**Program module**:
A module that the PC-1500 ROM takes as holding a program: it cannot be
written and starts with a module header. The CE-502B is one; a CE-161
becomes one with its write-protect switch set.
_Avoid_: ROM card

**Write-protect switch**:
The switch on a memory module that blocks every write to its RAM; on the
CE-161 the manual calls it the "Exclusive Readout Area Set Switch".
_Avoid_: Set Switch, read-only flag

**Module slot**:
A connector of a machine with the pin-out for modules: the TI-59's 8-pin slot,
the PC-1500's memory slot, the PC-1600's Slot 1 and Slot 2.
_Avoid_: card slot, bay

**Module definition**:
The file that describes one memory module: its memory, how it is
addressed and the hosts it fits.
_Avoid_: card definition, card file

**Magnetic card**:
The TI-59's removable magnetic strip that stores a program or data, with
two sides.
_Avoid_: card (bare)

**Cue card**:
The printed card that labels the TI-59's user-defined keys for a program.

**Chip family**:
A group of chips of one design that differ in content, such as the
TMC0540 family of Solid State ROMs.

**Chip**:
One concrete part, a member of a chip family or a single design, such as
the TMC0541 (Master Library ROM) or the LH5801.

## Code structure

**Library**:
The code and material used by more than one product. It has an internal
structure of its own, which may again contain parts used across that
structure.
_Avoid_: kit, shared layer, common layer (for this)

**Feature ID**:
The stable ID of a feature folder: `LIB-n` for a library feature, `C59-n`
or `C1600-n` for a feature of one product.
_Avoid_: KIT-n

**App layer**:
The frontend-independent code of one product, used by all of that
product's frontends.
_Avoid_: shared layer, Shared (for this)

**Frontend**:
One GUI implementation of a product: Swift/SwiftUI, Qt6, Web.
_Avoid_: GUI, client (for this), platform

**Frontend interface**:
The interface through which the app layer calls back into a frontend.
_Avoid_: platform (for this)

**Platform**:
An operating system and CPU architecture, or the browser, that a frontend
is built for: macOS arm64, iOS, Windows x86_64, Linux aarch64, …
_Avoid_: target, OS (for this)

**Harness**:
The one documented interface of the family for running a product without
a GUI, made for agents: development, test scenarios, ROM tracing. It has
product-specific commands where the hardware differs. It is not a
frontend and not meant for end users.
_Avoid_: CLI, API, headless emulator, agentic runner

## Sessions

**Machine session**:
A machine together with its run loop, pacing and input queue.
_Avoid_: emulator session, session (bare)

**App session**:
One window's state around a machine session: settings, files and media,
the debugger, and the frontend interface.
_Avoid_: session controller, session (bare)

## Presets

**Preset**:
A file that sets up a machine: the model, its peripherals and modules,
then program, data and scripted input.
_Avoid_: state file, snapshot, setup file

**Preset format**:
The file format of presets.
_Avoid_: state file format

## Running

**Full speed**:
The emulator running a machine as fast as the host allows.
_Avoid_: turbo, fast mode, maximum speed

**Regular speed**:
The emulator running a machine paced to the real device's speed.
_Avoid_: normal speed, real time

**Fast mode**:
A state of the real TI-59, found by users, in which a program runs faster
than usual; a behaviour of the machine, not of the emulator.
_Avoid_: full speed, turbo (for this)

**Reset**:
The emulator restarting a machine with its memory kept. On the TI-58/59
the real calculator cannot do this; on the TI-58C it equals a power cycle;
on the PC-1500/1600 it is the reset button.
_Avoid_: soft reset, restart, power cycle (for this)

**Cold boot**:
The emulator starting a machine as after a long battery removal, with all
memory empty, on every model. A preset builds its machine from a cold
boot.
_Avoid_: Reset All, clean reset, Clean, power cycle (for this)

**Power cycle**:
Switching a real device off and on. What survives depends on the model:
the TI-58/59 lose their RAM, the TI-58C keeps its constant memory, the
PC-1500/1600 keep all memory.
_Avoid_: reset (for this)

**Reset button**:
The physical button of the PC-1500/1600 that starts the ROM's **reset
routine**; the routine also runs after some power cycles, for example
after a module change. The PC-1600's **ALL RESET** is its deeper form.

**Battery removal**:
Power gone long enough for a real device's RAM to lose its contents.

## Media

**Medium**:
A removable item that holds user data and can be named and saved: a
magnetic card, a floppy disk, a cassette tape, a battery-backed module.
Plural: media.
_Avoid_: removable, item, card

**Template**:
A medium bundled with a product, which is never written.
_Avoid_: bundled card, default

**Instance**:
A user's saved medium made from a template.
_Avoid_: copy, saved card

**Name & Save**:
Giving a template a name and saving it as an instance, which from then on
saves itself after each change (**autosave**).
_Avoid_: save as, duplicate

**Insert**:
Putting a medium into its place on a machine; **Eject** takes it out.
_Avoid_: mount, load, attach (for this)

**Fit**:
Placing a TI-59 magnetic card in the window above the top row of keys
after it was read, where it labels the keys.
_Avoid_: insert (for this)

## Input

**Key**:
A key of the emulated machine.
_Avoid_: button (for this)

**Host key**:
A key of the computer's keyboard.
_Avoid_: physical key, PC key

**Keyboard map**:
The mapping from host keys to keys, per model.
_Avoid_: key map, host key map

**Typing**:
Feeding text into a machine as a sequence of key presses, as when pasting
text or from a preset.
_Avoid_: keystroke file, input injection

## Debugging

**Debugger**:
Stopping, stepping and breakpoints on a running machine, wherever its user
interface sits: in the frontend or in an editor through DAP.

**Debug panel**:
The view of a machine's state inside a frontend.
_Avoid_: inspector, debug view

**Trace**:
A recorded per-instruction log of a CPU, saved as a **trace file**.
_Avoid_: log (for this), trace dump

**Pause**:
The debugger stopping a running machine; **Resume** lets it run on.
_Avoid_: freeze, halt, break

**ASM overlay**:
Hand-assembled TI-59 CPU code that the debugger loads into an extra ROM
area beyond the real ROM and runs there.

## Settings

**Settings**:
A user's preferences for a product, kept in its **settings file**.
_Avoid_: preferences, defaults, config

**Startup model**:
The model a product starts with: a fixed model or the last used one.
_Avoid_: default model

## Repositories and releases

**Source repository**:
The repository that holds the source of every product.
_Avoid_: monorepo

**Release repository**:
The repository where a product's releases are published.
_Avoid_: old repository

**Release**:
One published version of one product, with its assets.

**Research repository**:
A repository of hardware and firmware research, such as `ti59-research` or
`Sharp1500-1600-Ref`.
