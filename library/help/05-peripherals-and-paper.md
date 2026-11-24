# Peripherals and paper

A **peripheral** is hardware you plug into the machine: a printer, a
module, an interface. It goes into a **place**, a named position on the
machine or on another peripheral, such as a module slot or the printer's
place. Which peripherals your product has, and what each does, is in
[the product's part of this chapter](chapter:5).

## Places and what fits them

Each place takes the peripherals whose plug fits it; the pickers and
menus list only those. Some places take one peripheral, others share a
connector, so that several peripherals are plugged in at once, each in
its own place. A peripheral can have places of its own, so that another
peripheral goes into it.

A peripheral that does not fit, or a place that is full, refuses the
attach; nothing changes, and a notice says why.

## Attaching and detaching

You attach and detach a peripheral in three ways:

- **On the control bar.** A place that only one peripheral fits has a
  button with its name, which shows whether it is plugged in: press it
  to attach or detach. A place that several fit has a picker with an
  empty choice. A module slot has its own picker, with the modules and
  memory modules that fit it ([Media](04-media.md#choosing-a-medium)).
- **In the menu.** **Machine ▸ Peripherals ▸** lists each place other
  than a module slot with the peripherals that fit it, the plugged-in one marked. Choose another to
  attach it in place of what is there; choose the marked one to detach
  it.
- **In a preset**, which names the peripherals of its machine
  ([Presets](06-presets.md)).

Choosing another peripheral for a place that holds one replaces it in
one step. After an attach or a detach the machine goes through a power
cycle where the real one would, and the change is over when it is ready
for keys again
([What follows a change of hardware](03-models-and-machine-control.md#what-follows-a-change-of-hardware)).

A peripheral whose attach needs more than a choice (a folder to name,
for instance) has no control on the bar and no entry under Peripherals
▸; the product gives it its own menu items.

Peripherals stay plugged in through a Reset, a power cycle and a Cold
boot.

## Controls of a peripheral

Some peripherals have keys or switches of their own, which are not keys
of the machine. Their panel shows them: a key as a button that is down
while you hold it, a switch as a button that shows its position.

## Serial ports

A peripheral or a model with a serial port can be linked to your
computer. On macOS and Linux the product then gives the serial port a
**serial port file** in the folder for serial links (set in the
settings dialog; by default the save folder). A program on your
computer opens that file as a serial port, with any line settings, and
the bytes pass as they are.

The file keeps its name as long as the product runs. When the machine
or the peripheral with the port goes away (a model switch, a detach),
the file stays, so that a program that has it open is not cut off; it
is connected again when a port of the same kind is back.

## Clocks

A machine with a real-time clock is given your computer's date and time
when it is built, and again after every operation, such as a Reset or a
loaded preset. In between it runs on the machine's own time: at full
speed it runs ahead, while the machine is paused it stands still.

## The paper panel

Everything a printer puts on its paper is shown in the **paper panel**.
A product with printers always shows the panel. Its header shows the
printer's name in the middle and, on the right, the control for the
view. The row below the paper has the printer's controls that the
product shows on the left and **Copy** (two sheets) and **Cut**
(scissors) as icons on the right; their tooltips name them. In a narrow
panel the controls show short labels. A switch shows its position by
its dot.

- While a printer is plugged in, the panel shows its paper under the
  printer's name, the whole width of the paper scaled to the panel, each
  dot in its shade or each pen stroke in its colour.
- While none is plugged in, the panel shows an empty paper without a
  name, and Copy and Cut cannot be used.

The view follows the newest marks while it is at the end of the paper.
When you scroll further back, it stays where you are. Scroll with the
wheel or by touch; there is no scroll bar.

A printer that also knows the text it printed has a control that
switches the panel between the marks and the text; the text can be
selected.

### One paper per printer

The product keeps one paper for each type of printer, from its start
until it quits, whatever machine runs:

| What happens | That printer's paper |
|---|---|
| Reset, Cold boot, a power cycle | stays |
| the printer is detached, or the model switched to one without it | kept, but not shown |
| the printer is attached again, on any model, in any variant | shown again as it was left |
| you load a preset that names the printer | empty at the start |
| a machine is built from a startup preset at a model switch | stays |
| Cut | empty |
| the product quits | gone; paper is not saved |

So a plot made on one model is still on the paper when you attach the
same printer to another model. A paper feed without printing makes the
paper longer, and the panel shows the blank paper.

## The paper in its own window

On a computer the paper panel can have a window of its own, which you
move and size like any other window. The button at the right edge of
the panel's header (a window with an arrow out of it, **Open in Own
Window**) moves the panel there; the paper, the printer's controls, Copy
and Cut go with it, and the paper stays as it was. The room the panel
leaves in the main window goes to the product's other panels.

In its own window the same button (**Back to Main Window**) moves the
panel back; closing the window does the same. The paper still fills the
width of its window and scrolls up and down.

The product remembers where the panel is and the window's place and
size: after a quit it starts with the paper in its own window again
where you left it. In a browser the window is a small window of its own
without the browser's address bar and menus; a browser that blocks such
windows on a start keeps the paper in the page and says so, and the
button opens it again. A window that you closed with the main window
comes back on the next start; the machine pauses while the browser tab
with the faceplate is hidden, as it does without a paper window.

On a phone or a tablet, and in a page that embeds the product, the
paper always stays in the main window.

## Copy and Cut

- **Copy** puts a picture of the whole paper on the clipboard, at the
  paper's real size, marks on white; with a printer that knows its text,
  also the text, one line per printed line. A very long paper is copied
  at a lower resolution, never cropped.
- **Cut** copies the paper and then empties it; what the printer prints
  next starts a new paper.

The panel shows "Copied to clipboard" for a moment, or "Copy failed"
when the clipboard took nothing. Copy and Cut cannot be used on an
empty paper.

In a browser that does not take a picture on the clipboard, the text
alone is copied where there is text; otherwise the picture is offered as
a download.

A preset can copy the paper with the step `- copy-paper:` and cut it
with `- cut-paper:`, which copies first, as Cut does ([Presets](06-presets.md)).
In the browser a preset never copies: `- copy-paper:` does nothing and
`- cut-paper:` only cuts.
