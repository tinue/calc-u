# Examples

Sample presets, BASIC programs and machine-code programs to try in
Calc-U-1600. A preset (`.pc1500`, `.pc1500a`, `.pc1600`) builds a machine
from a cold boot — model, modules, peripherals — and then types or loads a
program; open one with **File ▸ Load Preset…**. A `.bas` file can also be
loaded on its own with **File ▸ Load BASIC Program…**. See
`help/06-presets.md` and `help/10-preset-sections.md` for the full preset format.

Each preset names itself with `preset-name:` (short) and `description:`
(one line). The presets are in preset format 2 (`format-version: 2`,
`docs/formats/preset.md` at the repository root); a file of version 1 is
refused with a hint. [bundle.yaml](bundle.yaml) lists the presets the app
and the web client come with, each with the files it needs: `startup` for
**Settings ▸ Startup presets**, `groups` for **File ▸ Examples**. Only
those files are bundled. A preset in either list must have both keys (the
test suite checks it).

## plotter/ — CE-150 and CE-1600P

| File | Machine | What it does |
|---|---|---|
| `lissajou-1500.pc1500a` | PC-1500A + CE-150 | Lissajous figure |
| `lissajou-1600.pc1600` | PC-1600 + CE-1600P | Lissajous figure |
| `lissajou-ce150.pc1600` | PC-1600 + CE-150 | Lissajous figure, PC-1600 driving the CE-150 in MODE 1 |
| `biorhythmus_1600.pc1600` | PC-1600 + CE-1600P | Biorhythm chart for a name and birth date |
| `globus.pc1600` | PC-1600 + CE-1600P | Globe of the earth |
| `ascii.pc1600` | PC-1600 + CE-1600P | ASCII/hex character chart |
| `ce150_demo.pc1500a` | PC-1500A + CE-150 | Four-colour box spiral plus a text caption |
| `ce150-text-and-frame.pc1600` | PC-1600 + CE-150 | Character sizes and the largest rectangle the plotter can draw |
| `ce1600p-text-and-frame.pc1600` | PC-1600 + CE-1600P | The same for the CE-1600P |

## basic/ — BASIC programs

| File | Machine | What it does |
|---|---|---|
| `hanoi.pc1600` | PC-1600 | Towers of Hanoi game, up to seven bricks |
| `dampflok.bas` | PC-1600 | Steam train animation on the LCD, with sound (from Baum's *PC-1600 Systemhandbuch*) |
| `old-vs-new-rom.bas` | PC-1600 + CE-1600P | Plots the characters that differ between the old and the new ROM |

## interfaces/ — CE-158 and the PC-1600 serial port

| File | Machine | What it does |
|---|---|---|
| `ce158_demo.pc1500a` | PC-1500A + CE-158 | Prints on the parallel port, sends a line out of the serial port and echoes one back |
| `ce158_demo.pc1600` | PC-1600 + CE-158 | The same on the PC-1600 |
| `setcom1600.pc1600` | PC-1600 | Serial setup for exchanging files with Sharp-Data-Exchange over `COM1:` |

## machine-code/ — machine-code programs

| File | Machine | What it does |
|---|---|---|
| `memtest_stock.pc1500` | PC-1500 | Memory test of the built-in RAM |
| `memtest_1500a.pc1500a` | PC-1500A | Memory test, loaded into the machine-language area |
| `memtest_ce155.pc1500` | PC-1500 + CE-155 | Memory test with the CE-155 module |
| `memtest_CE1638.pc1500` | PC-1500 + CE-1638 | Memory test with the CE-1638 module |
| `memtest_bank.pc1500a` | PC-1500A + CE-1638 | Memory test across all banks of a banked module |
| `memtest.asm`, `memtest_bank.asm` | | Sources of the memory tests (LH5801, sdas syntax) |
| `Calculat.pc1600` | PC-1600 | CalCula, an HP-style RPN calculator (KiKiSoft); user guide in the preset header |

## dwx/ — DiskWorks, self-contained

| File | Machine | What it does |
|---|---|---|
| `DiskWorks.pc1600` | PC-1600 + CE-1600P/F + CE-1600M + CE-1601M + host drive | DiskWorks v3 (KiKiSoft), loaded from the folder `S3/` mounted as `S3:`; S2: and X: hold the sample programs |
| `make_diskworks_media.pc1600` | same | Makes the card `CE-1601M - Progs.module.yaml` and the disk `Progs.floppy.yaml` next to it, as templates |

## memory/ — memory modules and cards

| File | Machine | What it does |
|---|---|---|
| `maxed-out-mem.pc1600` | PC-1600 + CE-1600M + CE-1601M | Formats the CE-1601M and splits it into RAM disk and expansion memory |
| `flashtest_ce163f.pc1500a` | PC-1500A + CE-163F | How flash writes work, from BASIC POKE/PEEK |
| `memory-cards/` | PC-1500 | A self-made module definition and a preset that uses it |

## startup/ — startup presets

One startup preset per model, offered under **Settings ▸ Startup presets**
(bundle.yaml's `startup`), applied whenever that model is selected.

## setup/ — preparation scripts

Presets that bring a memory card or CE-163F firmware into a known state
for other presets to use. See `setup/README.md`.
