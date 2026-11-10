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
The card in the window above the top row of keys that labels the
TI-58/58C/59's user-defined keys for a program. Three types exist: the
magnetic card (TI-59 only), the blank non-magnetic card (all three models)
and the pre-printed card delivered with a Solid State ROM module, such as
the Master Library's (all three models).

**Chip family**:
A group of chips of one design that differ in content, such as the
TMC0540 family of Solid State ROMs.

**Chip**:
One concrete part, a member of a chip family or a single design, such as
the TMC0541 (Master Library ROM) or the LH5801.

**Floppy disk**:
A disk for the PC-1600's CE-1600F drive, with two **sides**, A and B, used
one at a time.
_Avoid_: disk image, floppy (bare)

**Cassette tape**:
A tape for the cassette interface of a CE-150 or CE-1600P, stored as sound.
_Avoid_: tape image, WAV (for this)

**Host drive**:
A PC-1600 drive (S3:) that the emulator backs with a folder on the
computer. It has no real counterpart.
_Avoid_: mounted directory, virtual drive

**Constant memory**:
The TI-58C's RAM that keeps its contents while the calculator is off.
_Avoid_: continuous memory, permanent memory

**Partition**:
The TI-58/59's split of its RAM between program steps and data registers.

## Code structure

**Capability**:
A generic kind of function that a component or peripheral offers through
the machine, handled the same way for every product: media, real-time
clock, serial port, printer output, …
_Avoid_: extension, feature (for this), device interface

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

**Example**:
A preset bundled with a product to show what it can do.
_Avoid_: sample, demo, bundled preset

**Startup preset**:
The preset a machine is built from when the product starts or the model is
switched.
_Avoid_: default preset

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
Placing a cue card in the window above the top row of keys, where it
labels them; a TI-59 magnetic card is fitted after it was read.
_Avoid_: insert (for this)

**Save folder**:
The folder where a product keeps its instances and other user files.
_Avoid_: battery-card saves, save directory, storage

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

**Program**:
User instructions stored in a machine's memory. The kinds are the
**keystroke program** (TI-58/59), the **BASIC program** (Sharp) and the
**machine-code program**.
_Avoid_: code, listing (for this)

**Program step**:
One memory cell of a keystroke program.
_Avoid_: step (bare), instruction (for this)

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
The repository that holds the source of every product and publishes their
releases: tinue/calc-u.
_Avoid_: monorepo

**Release tag**:
The tag of one release, with the product as prefix, such as
`calc-u-59-v2.4.0`.
_Avoid_: version tag (for this)

**Archived repository**:
A product's former repository, read-only, which keeps its history and its
earlier releases: tinue/Calc-U-59, tinue/Calc-U-1600.
_Avoid_: old repository, release repository

**Release**:
One published version of one product, with its assets.

**Research repository**:
A repository of hardware and firmware research, such as `ti59-research` or
`Sharp1500-1600-Ref`.

## Added with the contracts (phase 4)

Terms the contracts in `docs/contracts/` introduced. Added on 2026-11-10
in the phase 4 review; they wait for the review gate like the contracts.

**Emulated time**:
The time a machine has run, counted by the machine itself and independent
of the host's clock.

**Tick**:
The unit of emulated time of a model: one period of a real clock of that
model, at the rate its machine descriptor gives.
_Avoid_: cycle, step (for the unit across models)

**Machine descriptor**:
The static description of a model and its variants: ROMs needed, tick
rate, display, keys, connectors, places, components and their modelling
depth. A **peripheral descriptor** is the same for a peripheral; a
product's **catalogue** lists all of its descriptors.
_Avoid_: model info, machine info

**Place**:
A named position on a machine or peripheral where the user puts a
peripheral, as the control bar and the presets name it: `slot-1`,
`printer`, `interface`. Several places can share one connector.
_Avoid_: bay, slot (for this), port

**Media place**:
A place for a medium: the card reader, a floppy drive, a cassette. It is
**pass-through** (takes the medium through and gives it back, as the
TI-59 card reader) or **holding** (keeps it until it is ejected).
_Avoid_: bay, drive (for the general term)

**Key ID**:
The stable name of a key, listed in the machine descriptor.
_Avoid_: key name, matrix code, key code (for this)

**Display kind**:
The form of a machine's display output: a **segment display** (positions
of named segments) or a **dot-matrix display** (dots, optionally in
character cells). An **indicator** is a named symbol of either kind
outside its positions or dots, such as a status-line symbol.

**Generic kind**:
A kind of capability that the library defines and handles the same way
for every product.

**Product kind**:
A kind of capability, topic, command or control entry that one product
defines for a function only it has; the library carries it without
naming it.
_Avoid_: extension, hook

**Typing engine**:
The library part that turns host keys, on-screen key hits and text into
key presses on a machine.
_Avoid_: key sequencer, typer

**Typing profile**:
What a product supplies per model for the typing engine: key sequences
for characters, the minimum hold, the cadence.

**Pacing profile**:
What a product supplies per model for the pacer: the slice length and
the model's pacing particulars.

**Key readiness**:
The state in which a machine's ROM can take the next key.
_Avoid_: scan loop (for the general term)

**Operation**:
Long work an app session runs as one unit, such as loading a preset or a
power cycle. While one runs, the session is **busy**.
_Avoid_: job, task (for the user-visible unit)

**Topic**:
A named part of what an app session sends to its frontend: a **state
topic** carries its whole state, a **stream topic** what was appended.
_Avoid_: event, callback, change flag

**Control state**:
What the controls around the machine offer right now: models, variants,
places and what fits them, media places, panels.

**Control bar**:
The bar below the calculator that holds the machine's hardware: the
model, then one picker per module slot and media place.
_Avoid_: toolbar

**Paper panel**:
The view of a printer's paper inside a frontend.
_Avoid_: printer view, plotter view

**Mark**:
One thing a printer put on its paper: a dot, or a pen stroke with its
colour. The **text layer** is the characters of each printed line with
their place on the paper, where the printer knows them.

**Host service**:
Something only a frontend can give an app session: the audio sink, the
clipboard, a file or folder picker, opening a URL.

**Debug target**:
What a machine exposes of one CPU for debugging: registers, address
spaces, breakpoints, stepping.

**Run control**:
The library part that pauses, steps and resumes a machine and checks
breakpoints.

**Level**:
What one debugger step means: `cpu` (one instruction) on every machine,
`program` (one program step) where a product declares it.

**ROM lookup**:
The library part that finds each ROM image a machine needs: bundled
first, then the user's **ROM folder**.

**ROM repository**:
A repository that holds ROM dumps, from which a product fetches them at
build time.

**Arm**:
Laying a medium ready at a media place, so that it is fed when the
machine waits for one.

**Outdated**:
Said of a file in an older version of an own format than the release
reads.

## Added with the feature specs (phase 4b)

**Slice**:
The stretch of emulated time that the pacer runs in one go. Its length
is part of the pacing profile.
_Avoid_: batch, frame (for this)

**Lag**:
How far a machine's emulated time is behind the wall clock at regular
speed.

**Header key**:
A top-level `key: value` line of a preset that describes the preset or
its machine.

**Section**:
A top-level key of a preset with a block below it, or, where its owner
says so, with only a value. Sections are carried out in file order.

**Step**:
One item of a step list in a preset (`- verb: value`). Not a program
step.

**Idle**:
Said of a machine that has finished what it was doing and waits for the
user. Each product says how this is told for its models.

**Reference**:
The name of a bundled preset without a path:
`examples:<path inside the product's bundled presets>`.

**Preset list**:
The presets a product bundles: its startup presets and its examples.
_Avoid_: catalogue (which lists descriptors)

**Shell**:
The part of a component set that every product's window is put together
from: the menu bar, the control bar, the settings dialog, the About
dialog, the notice, "Loading…", and the area for panels.

**Faceplate**:
The picture of the calculator with its display and its on-screen keys.

**Hit map**:
What says which area of a faceplate is which key.
