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
addressed and the hosts it fits (`.module.yaml`, `format: memory-module`,
`docs/formats/modules.md` §1).
_Avoid_: card definition, card file

**Magnetic card**:
The TI-59's removable magnetic strip that stores a program or data, with
two sides.
_Avoid_: card (bare)

**Cue card**:
The card in the window above the top row of keys that labels the
TI-58/58C/59's user-defined keys for a program. Three kinds exist
(`docs/formats/cue-card.md` §1): the **stand-alone cue card**, the blank
non-magnetic card (all three models); the **magnetic card** (TI-59
only); and the **module cue card**, pre-printed for a library program of
a Solid State ROM module, such as the Master Library's (all three
models). The machine's **own card** is the stand-alone or magnetic card
fitted; a module cue card is shown above it while its library program
is selected.

**Chip family**:
A group of chips of one design that differ in content, such as the
TMC0540 family of Solid State ROMs.

**Chip**:
One concrete part, a member of a chip family or a single design, such as
the TMC0541 (Master Library ROM) or the LH5801.

**Floppy disk**:
A disk for the PC-1600's CE-1600F drive, with two **sides**, 0 and 1, used
one at a time.
_Avoid_: disk image, floppy (bare)

**Cassette**:
The medium of the cassette recorder of a CE-150 or CE-1600P, stored as a
**cassette definition** (`.cassette.yaml`) that names its sound file
(`.wav`) (`docs/formats/media.md` §3). What it holds is a tape, whose
sound the recorder plays or records; the tape counter counts it.
_Avoid_: tape image, WAV (for this), cassette tape (as the name of the file)

**Host drive**:
A PC-1600 drive (S3:) that the emulator backs with a folder on the
computer. It has no real counterpart.
_Avoid_: mounted directory, virtual drive

**Constant memory**:
The TI-58C's RAM that keeps its contents while the calculator is off.
The **constant memory file** (`ti58c.memory.yaml`) holds it with the
fitted cue card between starts (`docs/formats/media.md` §4).
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

**Product word**:
The short word that names a product wherever a developer types or reads
it: `c59`, `c1600` (a future Calc-U-1403: `c1403`). It is the harness's
command word of the product commands (`c59 asm`, `c1600 dump-basic`), the
value of `--product`, the prefix of the DAP server's own requests
(`c1600/load`) and the VS Code debug type (`c1600`); with a dot it is the
prefix of the product's topics, commands and requests (`c59.cue-card`,
`c1600.display-text`). Machine and model names (`ti59`, `pc1600`) stay
for files and machines (`.ti59`, `PC1600KeyLayout`).
_Avoid_: short form, command word (for the product)

## Sessions

**Machine session**:
A machine together with its run loop, pacing and input queue.
_Avoid_: emulator session, session (bare)

**App session**:
One window's state around a machine session: settings, files and media,
the debugger, and the frontend interface.
_Avoid_: session controller, session (bare)

**Listener**:
What talks to an app session from outside: a frontend, the harness, a
test. It is sent the topics, and the replies to its own commands and
requests.
_Avoid_: client, host (for this)

## Presets

**Preset**:
A file that sets up a machine: the model, its peripherals and modules,
then program, data and scripted input.
_Avoid_: state file, snapshot, setup file

**Preset format**:
The file format of presets.
_Avoid_: state file format

**Example**:
A preset in a product's `examples/` folder that shows what it can do;
those its bundle file lists are bundled with the product.
_Avoid_: sample, demo

**Bundled preset**:
A preset the bundle file lists: a startup preset or an example.

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
magnetic card, a floppy disk, a cassette, a battery-backed module.
Plural: media. A **removable medium** (a magnetic card, a floppy disk,
a cassette) is changed while the machine runs and is inserted by a
preset step; a module is installed before the machine starts, by a
header key, like the other **installed peripherals**
(`docs/formats/preset.md` §3, §5.2).
_Avoid_: removable (alone), item, card

**Name tag**:
The short name a file of a format carries in its `<kind>-name` key
(`card-name`, `disk-name`, …), shown in pickers and lists and matched by
a `name:` reference (`docs/formats/README.md` §2).

**Card file**, **In-file card**:
A card file is a magnetic card saved as a file (`.magnetcard.yaml`). An
in-file card is a magnetic card a preset defines in its header key
`magnetic-cards:` and addresses with `mem:`; it is written to a file
only by `saveas:` (`docs/formats/media.md` §1.2, `preset.md` §3.1).

**Keystroke program file**:
A TI keystroke program kept in a file of its own (`.keystrokes.yaml`,
`docs/formats/preset.md` §4.1.1), which a preset's `program:` names by
`file:`.

**Template**:
A medium bundled with a product, which is never written, or one the
user made (a **user template**: a file in the save folder that says it
is one). The magnetic card's one template is the fresh card, which is
always there and has no file.
_Avoid_: bundled card, default

**Instance**:
A medium whose file is not a template (no `template: true`), wherever
it came from; it saves itself in place after each change.
_Avoid_: copy, saved card

**Name & Save**:
Giving a template a name and saving it as an instance, which from then on
saves itself after each change (**autosave**).
_Avoid_: save as, duplicate

**Insert**:
Putting a medium into its place on a machine; **Eject** takes it out.
_Avoid_: mount, load, attach (for this)

**Fit**:
Making a cue card the one shown in the window above the top row of keys,
where it labels them. A card is fitted by a preset's cue card, by a
magnetic card after it has passed through the reader (read or written)
or when it is armed, by a TI-58C starting from its constant memory
file, and by selecting a library program of a module, whose cue card is
shown over the own card until the program is deselected
(`docs/formats/cue-card.md` §2). Fitting changes nothing in the
machine.
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
peripheral or a medium, as the control bar and the presets name it:
`slot-1`, `printer`, `interface`, `card-reader`, `floppy`,
`cassette-recorder`. Several places can share one connector.
_Avoid_: bay, slot (for this), port

**Media place**:
A place for a medium: the card reader (`card-reader`), a floppy drive
(`floppy`), the cassette recorder (`cassette-recorder`) of a CE-150 or
CE-1600P. It is
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
places and what fits them, media places, panels, and the controls of the
plugged-in peripherals with their positions.

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
Making a TI-59 magnetic card ready for the user to feed by hand: the
card is fitted, and the user feeds it by tapping a corner. Arming does
nothing electrically; the machine doesn't detect the card until it is
inserted.

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
A top-level key of a preset that describes the preset or its machine,
wherever it stands; it has a value on its line or, where its owner says
so, a block.

**Section**:
A top-level key of a preset with a block below it, or, where its owner
says so, with only a value. Sections are carried out in file order.

**Step**:
One item of a step list in a preset (`- verb: value`). Not a program
step.

**Idle**:
Said of a machine that has finished what it was doing and waits for the
user; on the TI-59 also a machine waiting for a magnetic card. Each
product says how this is told for its models; the formats don't.

**Implicit wait**:
The wait of a preset, between two items, until the item before has
finished. Not after the last item.

**Skip wait**:
The preset step `skip-wait:`, which drops the implicit wait after the
item before it, so that the next step acts on a running machine.

**Let it finish**:
The preset step `let-it-finish:`, allowed only as the last item: the
machine runs on in the state in force until it is idle, then the preset
ends.
_Avoid_: wait (for this)

**Reference**:
A mapping that addresses a resource by exactly one of `file:`, `name:`,
`mem:` or `folder:`, with the options its owner allows
(`docs/formats/README.md` §3).

**Bundle path**:
A bundled preset's path inside the bundle, written
`examples:<path inside the product's bundled presets>` where a place
takes it.

**Bundle file**:
The file `examples/bundle.yaml` of a product: the presets it bundles,
every file they need, and their order and groups
(`docs/formats/bundle.md`).

**Preset list**:
The presets a product bundles: its startup presets and its examples,
as its bundle file lists them.
_Avoid_: catalogue (which lists descriptors)

**Shell**:
The part of a component set that every product's window is put together
from: the menu bar, the control bar, the settings dialog, the About
dialog, the notice, "Loading…", and the area for panels.

**Faceplate**:
The picture of the calculator with its display and its on-screen keys.

**Hit map**:
What says which area of a faceplate is which key.

**Kind** (of medium):
What one sort of media place takes: a magnetic card, a floppy disk, a
battery-backed module, a cassette. Each kind has its file suffix
and its format.

**Saved media**:
The media in the save folder and the cassette folder, and the list of them
that a Web frontend shows.

**Breakpoint**:
An address of a debug target at which the run control pauses the
machine before the instruction there is carried out.

**Watch**:
A range of cells of an address space; the run control pauses the
machine after an instruction that read or wrote one of them.

**History** (of a debug target):
Its last instructions, each with the registers as they were before
it.

**Side**, **Bank**, **Pass**, **Fresh** (of a magnetic card):
A magnetic card has two sides, 0 and 1; a side holds one bank, a
quarter of the TI-59's memory, numbered 1 to 4. A pass is one run of a
side through the card reader. A card is fresh while neither side has
been written.

**Library program**:
One of the numbered programs of a Solid State ROM module, selected with
`Pgm`.

**Open** (a file), **File kind**:
To open a file is to hand it to the app session, which finds out what
it is and does what belongs to it. A file kind is one sort of file a
product opens besides presets and media.

**BASIC listing**, **Tokenized BASIC**:
A BASIC program as text; the same as the machine stores it, in a file
with a header.

**Header**, **Headerless**:
The first bytes of a program file that say what it is, how long it is
and where it goes; code in a file without them.

**Program area**, **Program mode**:
The part of memory BASIC keeps the program in (on the PC-1600: S0, S1
or S2); the mode in which typed lines are stored (PRO).

**Plan**, **Starting command**:
The loader's answer to where a machine-code file would go, made before
anything is written; the BASIC command that starts loaded code.

**DAP**, **Server**, **Client**, **DAP session**, **Configuration**, **Thread**:
The Debug Adapter Protocol; the part of Calc-U-1600 that answers it;
the debugger that connects; the time from a client's `initialize` to
its disconnect; what the client sends with `attach`; DAP's word for a
debug target.

**Build & Load**, **Clean start**:
Putting a freshly built program into the machine from the debugger;
building the machine anew before that.

**Listing** (of an assembler), **Binding**:
The file an assembler writes with addresses, bytes and source lines;
what ties a listing or a table of symbols to a debug target.

**Heatmap**, **Dump** (in Calc-U-59's debug panel):
The heatmap shows how often each part of the ROM was carried out. A
dump is a text that lists a part of the machine's memory into the log.

**Help**, **Shared part**, **Product part**, **Help asset**, **Help tool**:
The help of a product is its user guide. A chapter has a shared part,
written once for the family and holding nothing of one product, a
product part, which belongs to one product, or both. The help asset is
the file `<Product>-help.zip` that a release carries; the help tool is
the command of the source repository that builds and checks it.

**Site folder**, **Pin** (of the site), **Preview**:
A product's site folder is its part of calc-u.org, `/59/` or `/1600/`.
A pin is the record, kept in the site, of one release file by its
address, its version and its sha256, or, for the examples, of the
release's tag in the source repository (LIB-19 AC-1). A preview is a build of a coming
release shown next to the released one.

**Mnemonic**, **Listing**, **Assembly** (of a keystroke program):
A mnemonic is the name of a keycode, which is the name of its key. A
listing is a keystroke program written as one line per step, as the
PC-100C's `List` prints it: step number, keycode, mnemonic; a preset's
`keycodes:` takes it (`docs/formats/preset.md` §4.1). An assembly is a
keystroke program written as one line per instruction, mnemonics with
their operands and no keycodes (`c59 keycodes disassemble`, C59-9).
Both are written forms of a program; the program itself is not called
a listing.

**View** (of the INSPECTOR), **Character time**:
A view is one of the texts the INSPECTOR, Calc-U-1600's debug panel,
shows. A character time is the time one character takes on a serial
port at the speed the port is set to: ten bits.

**Host link**, **Serial port file**:
A host link is what connects a machine's serial port to the user's
computer. A serial port file is the name under which a host link
appears there, a pseudo-terminal that any serial program can open.

**Suspend**, **Continue** (a session):
To suspend a session is the frontend stopping its machine while the
product is in the background; to continue it is the opposite. Pause and
resume are the debugger's words.

**Live input**, **Typed text**, **Press**, **Frame** (of typing):
Live input is what a user does now, with a host key or an on-screen
key. Typed text is a string the typing engine types by itself (a paste,
a `type` step). A press is one key going down and later up. A frame is
1/60 second of emulated time, the unit Calc-U-1600 counts its key holds
in; a slice is not called a frame.

## Decision log

| Date | Decision | Source |
|---|---|---|
| 2026-11-17 | "Cassette" names the medium everywhere (the kind, the preset step and eject word, `format: cassette`, `.cassette.yaml`, `cassette-name`, the cassette folder); its place is the `cassette-recorder` (CE-125 manual: "microcassette recorder", PC-1403 manual: "cassette recorder"); the entry "Cassette tape" became **Cassette**, "Place" and "Media place" name the cassette recorder; the module definition is `.module.yaml` | user, 2026-11-17 (Part E review Q-5, Q-6; release prep step 2) |
| 2026-11-17 | **Control state** also holds the controls of the plugged-in peripherals with their positions (LIB-4 AC-21) | user, 2026-11-17 (Part E review Q-6; release prep step 3) |
| 2026-11-17 | New entry **Product word**: `c59`, `c1600` (future `c1403`) for the harness's product commands, `--product`, the DAP requests and the VS Code debug type, and as the dotted prefix of product topics, commands and requests | user, 2026-11-17 (Part E review Q-6; release prep step 1) |
