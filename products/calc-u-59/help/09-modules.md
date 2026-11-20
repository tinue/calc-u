# Solid State ROM modules

A **Solid State ROM module** holds a library of programs in its ROM, up
to 5,000 steps, which run straight from the module and leave your memory
free. Calc-U-59 has all fourteen modules Texas Instruments made; one goes
into the **module slot** of every model.

| ID | Module |
|---|---|
| ML | Master Library |
| ST | Applied Statistics |
| RE | Investment |
| SY | Surveying |
| NG | Marine Navigation |
| AV | Aviation |
| LE | Leisure Library |
| SA | Securities Analysis |
| BD | Business Decisions |
| MU | Math/Utilities |
| EE | EE Library |
| SE | Structural Engineering |
| AG | Agriculture |
| RP | RPN Simulator |

## Choosing a module

- **The module picker** on the control bar lists the fourteen modules by
  name, and the empty slot.
- **A preset** names its module with `slot-1: { name: ML }`; a preset
  without `slot-1:` leaves the slot empty
  ([Presets of this product](06-presets.md)).
- Without a startup preset, Calc-U-59 starts with the Master Library.
  The module you choose is not remembered for the next start; set a
  startup preset to start with another one.

Choosing a module goes through a power cycle, as plugging in the real
module does: the TI-59 and TI-58 lose their program and data registers,
the TI-58C keeps its constant memory ([What differs by
model](09-models.md)).

## Using a module

**2nd Pgm** and a two-digit program number selects a program of the
module, as on the real calculator, and shows its cue card above the
keys: the pre-printed card of the module's program (the Master Library
has 25, the Leisure Library 21), or a plain card titled with the
module's name for a program without one ([Cue cards](09-cue-cards.md)).
While a library program is selected its card covers a fitted magnetic
card; **2nd Pgm 00** goes back to your own program and your card.

If the cue card or the programs look unexpected, check which module is
in the slot first.

The modules' ROMs come with the release; each is a Solid State ROM
module file (`.rom.yaml`, [Media files](10-media-files.md)), which you do
not open yourself: a module file opened or dropped is refused with a
notice.
