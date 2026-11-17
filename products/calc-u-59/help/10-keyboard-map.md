# Keyboard map

Your computer's keyboard presses the calculator's keys by this map, the
same on every model and in every frontend that takes keys from a
keyboard: the Mac, Windows and Linux apps and the browser. In the
browser, the calculator takes the keys while it has the focus: click it
once; click elsewhere, or press Tab, and the page gets the keyboard back.

| Your key | Calculator key |
|---|---|
| `0` … `9` (also on the number pad) | `0` … `9` |
| `.` or `,` | `.` |
| `+` | `+` |
| `-` | `-` |
| `*` or `x` | `×` |
| `/` | `÷` |
| `=`, Return or Enter (also the number pad's) | `=` |
| Esc | `CLR` |
| `<` | `+/-` |
| `>` | `EE` |
| `(` | `(` |
| `)` | `)` |
| `a` … `e` | `A` … `E` |
| Shift + `a` … `e` | `A'` … `E'` (2nd, then the letter's key) |
| Shift + `x` | `Deg` (2nd, then `×`) |

- A letter is the letter your key carries on your keyboard layout, so the
  map follows the layout you use; every other character is the one your
  layout produces for the key.
- No other key reaches the calculator: 2nd, INV, STO, RCL, LRN, R/S and
  the rest are pressed on the faceplate (or in a preset, by their key IDs:
  [Preset sections](10-preset-sections.md#key-ids)).
- A key held with Command, Control, Option or Alt, or the Windows key, is
  never the calculator's: it is a shortcut of the menus or of the system.
- Your keyboard's auto-repeat is ignored. A key is held at least 100 ms of
  emulated time and until the ROM has read it, also when you let go
  sooner; two keys held at once reach the calculator together.
