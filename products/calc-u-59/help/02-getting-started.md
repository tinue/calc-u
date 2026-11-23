# Getting started with Calc-U-59

The window, the control bar, the menus and the settings dialog are the
family's; the [shared part of this chapter](../../../library/help/02-getting-started.md)
describes them ([the window](../../../library/help/02-getting-started.md#the-window),
[the menus](../../../library/help/02-getting-started.md#the-menus),
[desktop and mobile](../../../library/help/02-getting-started.md#desktop-and-mobile),
[the control bar](../../../library/help/02-getting-started.md#the-control-bar)).
This part shows what they hold on the TI-59, the TI-58 and the TI-58C,
and how the window looks on each system. How to install Calc-U-59 is in
[Installing Calc-U-59](02-installing.md).

Calc-U-59 starts with the model you chose as the startup model, the
TI-59 the first time, with the Master Library module and no printer,
or with the startup preset you set for that model
([Presets](06-presets.md)). The calculator is on at once.

## The calculator

The window's main part is the calculator: the red LED display at the
top, the cue card below it, and the keys. The cue card names the keys
**A** to **E** and **A′** to **E′** for the program that is loaded: a
library program's own card, a magnetic card, or a cue card a preset
fits ([Cue cards](09-cue-cards.md)).

- Click or touch a key to press it. A key stays the one you pressed
  first; moving the pointer onto another key does not move the press.
- **Press and hold the display** to run the calculator at full speed,
  for a long program or a long listing on the printer; it runs at
  regular speed again when you let go.
- The display can draw its segments **Modernized** or **Classic**
  (Settings ▸ Display ▸ Segment Style).

## The control bar

The control bar below the calculator holds, from left to right:

1. the **model picker**: 59, 58, 58C, short for TI-59, TI-58, TI-58C;
2. the **module picker** of the module slot: the fourteen Solid State
   ROM modules and the empty slot ([Solid State ROM modules](09-modules.md));
3. on the TI-59 only, the two **Crd** buttons, Arm Card and Arm Fresh
   Card, and **Eject Card** in their place while a card passes the reader
   ([Magnetic cards](04-magnetic-cards.md));
4. the **More** button (⋯), at the right edge.

Where the bar is wide enough, the buttons show their labels beside
their icons (**Crd**, **More**); else the icons alone.

The bar is the same on a desktop and on mobile.

The button **PC-100C**, which attaches or detaches the printer, is at
the left of the paper panel's header, above the paper it prints on
([The PC-100C printer](05-printer.md)).

Changing the module, or attaching or detaching the PC-100C, goes through
a power cycle, as on the real calculator ([What differs by
model](09-models.md)). While a preset loads, the model, module, printer
and card controls are disabled; the card buttons' tooltip says why.

## The menus

On a desktop (macOS, Windows, Linux, the browser in a tab on a
computer) the window has a menu bar:

| Menu | Items |
|---|---|
| File | Load Preset…; Examples ▸; on the TI-59 Arm Card… and Arm Fresh Card…; in the browser Import Media… and Saved Media…; Settings…; Quit |
| Machine | Model ▸; Peripherals ▸; Reset; Cold boot; Resume |
| Help | Calc-U-59 Help; About Calc-U-59 |

Calc-U-59 has no Edit menu: it has neither Copy Screen nor Paste Text.
On macOS, Settings…, About and Quit are in the application menu. The
shortcuts are those of the family: Load Preset… Ctrl+O (⌘O), Reset
Ctrl+R (⌘R), Cold boot Ctrl+Shift+R (⇧⌘R).

On every bar the **More** button (⋯) holds every menu item, each menu
under its name. On mobile (the
iPhone, the iPad, the browser version installed on a phone or tablet, a
browser asking for the mobile site) there is no menu bar, so More is the
way to them.

**Help ▸ Calc-U-59 Help** opens [calc-u.org/59](https://calc-u.org/59/) in
your browser.

## The window on each system

- **Mac, Windows and Linux** (the desktop app): the calculator, the paper panel
  and the debug panel side by side, with dividers you can move; the
  window comes back at its last size and place. In a window too low for
  the debugger it is left out; in one too narrow for the paper beside the
  calculator, the calculator and the paper are pages you turn with the
  arrow in the top corner.
- **iPhone**: portrait only. The calculator fills the screen; the
  printer and the debugger are pages you turn with the arrows in the top
  corners (the debugger only with Settings ▸ Layout ▸ Debug Page in
  Portrait).
- **iPad**: in landscape, at least 820 by 560 points, the calculator, the
  paper and the debugger side by side as on the Mac; in a smaller
  landscape window the calculator and the paper; in portrait the pages
  of the iPhone. An iPhone that opens flat shows them side by side as on
  the iPad.
- **The browser** ([calc-u.org/59/play/](https://calc-u.org/59/play/),
  or installed as an app): the **wide** layout with the paper and the
  debugger; the **portrait** layout with the paper as a second page. In a
  tab on a computer the page has the menu bar; installed on a phone or
  tablet it has none. Installed, it chooses by the window as the desktop
  app does: the paper beside the calculator where there is room for it
  (the debugger too on a larger screen), else the pages. On Android the
  installed app stays upright, as the calculator is.

![The calculator, the paper and the debugger on an iPad in landscape](images/ipad-13-2752x2064.png)

## Your keyboard

On a computer you can type on the calculator with your keyboard: the
digits, the operation keys, **A**–**E**, **EE** and the parentheses,
with **Shift** and a letter for **A′**–**E′**. In the browser, click the
calculator first; a golden outline shows that it has the keyboard. The
whole table is the [keyboard map](10-keyboard-map.md).

## Opening files

A file you drop on the window, open with the product or name when you
start it is opened as what it is
([Media](../../../library/help/04-media.md#opening-and-dropping-files)):

| File | What happens |
|---|---|
| a preset (`.ti59`, `.ti58`, `.ti58c`) | it is loaded ([Presets](06-presets.md)) |
| a magnetic card (`.magnetcard.yaml`) | it is fitted, on the TI-59 ([Magnetic cards](04-magnetic-cards.md)) |
| a keystroke program (`.keystrokes.yaml`) | its program goes into the running calculator, without a cold boot ([Keystroke program files](10-keystroke-programs.md)) |
| an ASM overlay (`.asm`) | it is put over the ROM ([Debugger](07-debugger.md)) |
| a Solid State ROM module file, the constant memory file | refused with a notice |

On Windows the installer makes Calc-U-59 open presets with a double
click; on Linux, name the file on the command line.
