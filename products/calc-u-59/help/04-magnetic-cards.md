# Magnetic cards

Calc-U-59's medium is the **magnetic card** of the TI-59; the TI-58 and
the TI-58C have no card reader. What every medium of the family does
(templates and instances, the save folder, Name & Save, opening and
dropping files) is in the [shared part of this
chapter](../../../library/help/04-media.md); this part shows the card.

A magnetic card has a name, a cue card and two sides, **0** and **1**, as
the real card has two. One card file holds both sides. A side is empty,
or holds one **bank**: a quarter of the calculator's memory, with the
partition it was written at, whether it is program or data, and whether
it is protected ([Magnetic cards and banks](01-introduction.md#magnetic-cards-and-banks)).
Either side can hold any bank, even both the same one.

## Fitting and feeding

Using a card takes two steps:

1. **Arm** it: it is fitted into the calculator above the keys, where it
   labels them with its cue card. Nothing is read or written yet, and
   the calculator sees nothing of it.
2. **Feed** a side by pressing a top corner of the card: the top left
   corner feeds side 0, the top right corner side 1. The corner lights
   up while the side passes the reader. The calculator reads the side, or
   writes it when it waits for a card to write. The card stays fitted
   afterwards, so the other side is one press away.

Each corner shows the number of the bank on its side, or nothing while
the side is empty; the number follows a write.

## Saving a program

1. Press the **Crd** button with the plus sign, **Arm Fresh Card**
   (**File ▸ Arm Fresh Card…**), and give the card a name. The fresh card
   is fitted above the keys, its cue card titled with its name. If a
   card of that name is saved, the dialog asks "Replace card `<name>`?"
   first: its first write replaces both sides and the cue card.
2. Press **2nd Write** for the bank you want to save: **1 2nd Write**
   for bank 1.
3. Press a corner of the card. Once the side has passed, it is saved in
   the card's file and the corner shows the bank it now holds.

A program too long for one side continues on the other: **2 2nd Write**,
then the other corner. Nothing is written to the save folder until the
fresh card's first write: arming a fresh card is
[Name & Save](../../../library/help/04-media.md#name--save) done up
front.

## Loading it back

1. Press the other **Crd** button, **Arm Card** (**File ▸ Arm Card…**),
   and choose the card. The dialog lists your saved cards by their names;
   with none saved it says "No cards found". On a computer and in the
   browser you can also drop a card file on the window.
2. Press the corner of the side you want. While the calculator waits
   for a key it reads the side by itself, as the real one does: the bank
   is stored when the card's partition is the calculator's and its
   checksum is right, and the display blinks otherwise.

## Good to know

- **Eject Card** takes the place of the Crd buttons while a side is in
  the reader. It takes the card out at once; the card stays fitted. A
  bank is kept only when the calculator had written all of it.
- **A library program's cue card** covers a fitted card, and the corners
  do nothing while it does. The card shows again when no library
  program is selected, for instance after **2nd Pgm 00**.
- **A fitted card stays** through a Reset, a power cycle and a Cold
  boot, and can be fed again afterwards. It goes when you fit another
  one, switch the model or load a preset. Its file stays as it is.
- **While a card is in the reader**, no other card can be fitted.
- **Only the side you write changes.** The card's name, its description,
  its cue card and the other side stay as the file holds them.
- **A card the calculator does not take at once**, while a program runs
  that asks for none, stays at the reader until the calculator takes it.
- **A program that asks for a card** (`n Write`, `n INV Write` in the
  program) waits until you feed a side or press R/S.
- **A card on a TI-58 or TI-58C** is refused with a notice.

## Where cards live

Cards are files `<name>.magnetcard.yaml` in the save folder (magnetic
card format 3, [Media files](10-media-files.md)), named after the card:

| System | Save folder |
|---|---|
| macOS, iPhone, iPad | the app's iCloud storage (Finder: iCloud Drive ▸ Calc-U-59), or its local Documents folder when iCloud is off |
| Windows, Linux | `Calc-U-59` in your home folder, unless **Settings ▸ Storage ▸ Save folder** names another |
| the browser | the browser's storage for the page, from one visit to the next; **Import Media…** brings a card file in, **Saved Media…** lists, downloads and deletes cards |

A card file opened from elsewhere, by a drop or with Open With, is used
and written back where it lies; in the browser it is imported into the
save folder first. A card file is plain text: its cue card can be edited
in a text editor ([Cue cards](09-cue-cards.md)), and the next time the
card is fitted it shows the change. Card files of 2.3.0 (`.U59`) are no
longer read; they are converted by hand
([From 2.3.0](10-media-files.md)).

Presets can feed cards and arm them, define cards of their own and save
them ([Presets of this product](06-presets.md#magnetic-cards-in-a-preset)).
