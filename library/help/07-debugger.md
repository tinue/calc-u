# Debugger

The debugger stops a running machine, lets you step through it, and
records what its processor does. Use it to find out why a program does
what it does, or to follow the ROM through a routine. What the
product's debug panel shows, its debugger in an editor, breakpoints and
watches are in the product's part: [Debugger of this product](chapter:7).

## Running and paused

A machine is either **running** or **paused**. While it is paused:

- no emulated time passes: the display keeps its last picture, nothing
  is printed, and what waits for time waits;
- every processor of the machine stands between two instructions, so
  its registers and memory can be read as they are;
- the machine is paused as a whole: on a model with several processors,
  all of them stop.

A pause has a **level**. Every product has the level of single
instructions, `cpu`; a product may have a level of its own, such as one
program step of a user program. The product's part says which levels it
has.

## Pause, step and resume

The window has three **run-control buttons** for a level: pause, step
and resume. Their labels and their place are the product's.

| Button | Can be used | Does |
|---|---|---|
| Pause | while the machine runs | pauses it where that level lands |
| Step | while it is paused at that level | carries out one step and pauses again; held down, it repeats four times a second |
| Resume | while it is paused at that level | lets it run on |

**Machine ▸ Resume** resumes the machine whatever paused it. It can be
used while the machine is paused.

- After a resume, the machine's clocks are given the computer's time,
  and pacing starts afresh: the time the machine was paused is not made
  up.
- Reset and Cold boot while paused let the machine run again. A model
  switch, a variant switch or a preset builds a new machine, which is
  not paused.
- While an operation runs (a preset loads, a file is loaded),
  breakpoints and watches do not pause the machine; a pause that the
  preset itself asks for does.
- A machine that is paused stays paused while the product is in the
  background or the page is hidden.

## Traces

A **trace** records every instruction the processor carries out into a
trace file, in the product's trace format. Start one with the product's
trace button or with a preset's `trace:` step.

- Only one trace runs at a time: starting a trace ends the one that
  runs.
- The trace file goes into the trace folder: the save folder, unless
  `[trace] folder` names another one ([Settings](08-settings.md)). A
  file of the same name is replaced. In a Web frontend that has traces,
  a trace that has ended is offered as a download.
- The trace folder and the size limit are read when a trace starts, so a
  change in the settings is used by the next trace. The folder is made
  when it is missing; when it cannot be made or written, the trace does
  not start and a notice says "The trace folder cannot be written:" and
  why.
- A trace ends with `trace: off`, at the end of the preset that started
  it, when a new machine is built (a line of the log says so), or when
  you end it with the button.
- **Size limit.** When the trace file reaches `[trace]
  max-file-size-mb` (50 MB unless the settings say otherwise; zero or
  less also means 50), the trace
  ends, the file is complete up to there, and a notice says so.
- While a trace runs, the window shows it, with the file name, whoever
  started it.

## Debugger steps in a preset

A preset can drive the debugger with these steps
([Presets](06-presets.md)):

| Step | What it does |
|---|---|
| `trace: <file name>` | starts a trace into that file in the trace folder; no `/` or `\` in the name |
| `trace: off` | ends the trace |
| `pause: <level>` | pauses the machine at that level; already paused: nothing happens |
| `pause-on-start: <level>` / `off` | pauses at the next start the level names (the next program step; the processor leaving key readiness); `off` disarms it |
| `step: { level: <level>, count: n }` | while paused, carries out n steps (1 to 100000; 1 without a count); not paused, a line in the log and nothing else |
| `resume:` | lets the paused machine run on |

Not every product defines every one of these steps on every model; the
product's part says which it takes. A step it does not take refuses the
preset when it is read.

While the machine is paused, a step that needs it to run (a key press, a
program to write into memory) waits until it runs again; the debugger
steps go on, so `step:` and `resume:` work. A `wait:` while paused counts
the computer's time and ends with the machine still paused. A preset
that ends paused leaves the machine paused.
