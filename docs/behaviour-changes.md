# User-visible behaviour changes

Briefing §1.1 says no user-visible feature breaks. The decisions of
phases 3 and 4 made exceptions and changes; this file lists them in one
place, per product, each with its source. It is the seed of each
product's release notes for its first release from this repository, and
the list of places where the acceptance criteria of phase 4b do **not**
describe today's behaviour (`docs/Process.md`, phase 4b).

A new decision that changes what a user sees adds a row here in the same
step. Label and menu renames are not repeated here; they are in
`docs/glossary-mapping.md` ("Visible renames").

## Calc-U-59 (against 2.3.0)

| # | Change | Source |
|---|---|---|
| 59-1 | Files written for 2.3.0 are refused with a hint to upgrade them: presets (`.ti59`, `.ti58`, `.ti58c`), `ti58c.mem`, ASM overlays and magnetic cards (text 1.0, text 2.0 and the binary layouts; the card format gets a new version for the name field). A one-time exception for this release. | ADR 0037; `contracts/media.md`, MD-2, Q-4 |
| 59-2 | Settings start at their defaults; the platform stores are not converted. | `contracts/settings.md`, ST-1 |
| 59-3 | The preset format is new (YAML-shaped, key IDs instead of matrix codes, `format-version: 2`); a preset with any error is refused whole. | ADR 0017, 0018; `contracts/presets.md` |
| 59-4 | A preset always builds its machine from a cold boot. It no longer adds to the running calculator, and it no longer keeps the module or printer it does not name. | ADR 0016 |
| 59-5 | A preset's steps run at full speed unless a speed step asks for regular speed; user keys are dropped while it runs. While a preset or another operation runs at full speed for more than half a second, "Loading…" is shown. | `contracts/presets.md`, PR-4; `contracts/typing.md`, T-4; LIB-14 AC-15 |
| 59-6 | Magnetic cards live in the save folder. A card file elsewhere is no longer opened and written back in place; a card gets a name inside its file. | ADR 0019; `contracts/media.md`, MD-2, MD-5 |
| 59-7 | The first write to a card a preset names makes an instance in the save folder and replaces an instance of that name. | `contracts/media.md`, MD-1 |
| 59-8 | On the web, magnetic cards persist across page loads. When the product is open twice in one browser, only the first copy keeps what is saved; the second says so. | `contracts/settings.md`, ST-3; `features/LIB-15-web-client-shell/spec.md` |
| 59-9 | The Solid State ROM module is chosen in the control bar, not in the settings, and the choice is not remembered across starts; a startup preset sets it. The picker also offers an empty slot. | ADR 0021, 0025; `contracts/presets.md`, PR-9 |
| 59-10 | Without a startup preset the product starts with a TI-59, the Master Library module and no printer (today: printer connected). | ADR 0021 |
| 59-11 | Changing the module, or connecting or disconnecting the PC-100C, power-cycles the machine: the TI-58 and TI-59 lose program and data registers; the TI-58C keeps its constant memory. | `contracts/peripherals.md`, P-1 |
| 59-12 | The TI-58C keeps its constant memory across starts only while no startup preset is set for it; a startup preset cold boots it. | `contracts/capabilities.md`, C-3 |
| 59-13 | A second key pressed while another is held no longer releases the first; both reach the key matrix. | `contracts/typing.md`, T-3 |
| 59-14 | Menus and the control bar follow the family structure. Where there is a menu bar (Qt6, the Swift frontend on macOS, the Web frontend's wide layout), Reset, Cold boot (today "Clean"), Load Preset and Settings are menu items and leave the bar; "Clean" with Ctrl or Command held goes. Where there is none (iOS, the Web frontend's portrait layout), the three buttons stay, a More button holds Examples, Cold boot and Help, and the long press for "Clean" goes. The PC-100C is connected with a button on the bar, not in the printer panel. On macOS the Swift frontend's model picker moves from the window toolbar to the bar and Help is new. The Machine menu has Resume. The Web frontend shows a notice as a dialog instead of a line below the calculator. | ADR 0024; LIB-14, C59-8 (user, batch 5 and batch 6 gates) |
| 59-15 | The TI-58C's constant memory is in place before the ROM starts, so the partition and the display format come back at a start as on the real calculator. To be confirmed in phase 6 whether 2.3.0, which restores the registers after the ROM's start-up, showed otherwise. | C59-3 AC-2 (user, batch 5 gate) |
| 59-16 | A damaged `ti58c.mem` and a failed write of it are reported with a notice (today: silent). | C59-3 EC-3, EC-4 (user, batch 5 gate) |
| 59-17 | Reset keeps the cue card that is fitted, on every model (today: it is taken away); a fitted magnetic card can still be fed afterwards. `ti58c.mem` remembers the fitted cue card as it is. | user, batch 5 and batch 6 gates; C59-3 AC-4, AC-11; C59-7 AC-25 |
| 59-19 | Traces: one at a time (today the button's and a preset's can run side by side); a trace that cannot be started fails its preset (today: an alert, and the preset goes on); a notice when the size limit ends a preset's trace (today: only the button's). | LIB-10 AC-22 to AC-24 (user, batch 6 gate) |
| 59-18 | While the PC-100C is detached the paper panel shows an empty paper without a name (today: the paper stays in view); the paper is back when the printer is attached again. | LIB-8 AC-8, AC-12, AC-29 (user, batch 6 gate) |

## Calc-U-1600 (against 0.8.1)

| # | Change | Source |
|---|---|---|
| 1600-1 | Presets with `format-version: 1` are refused with a hint; the format is version 2. Module definitions and floppy disks stay readable as long as their formats do not change in the migration. | ADR 0037; `contracts/presets.md`, PR-2 |
| 1600-2 | Settings start at their defaults (INI file). | `contracts/settings.md`, ST-1 |
| 1600-3 | Preset syntax: `- key:` takes several key IDs, `- wait:` needs a unit, the hardware key `plotter:` becomes `printer:`; debugger, paper and switch steps are new. | `contracts/presets.md`, PR-5, PR-7, PR-11 |
| 1600-4 | Choosing a module power-cycles the machine (today: a restart like a model switch). The memory is kept through the power cycle; the ROM then finds the changed memory and asks the user to set BASIC up again (`NEW0? :CHECK`), as on the real device and as after today's restart. | `contracts/peripherals.md`, P-1 |
| 1600-5 | A media name with a character a file name cannot hold is refused (today: `/` and `:` silently become `-`). | `contracts/media.md`, MD-3 |
| 1600-6 | An outdated file shows in its picker as a disabled entry with a hint (today: left out silently). | `contracts/media.md`, MD-4 |
| 1600-7 | The first write to a medium a preset names replaces an instance of that name. | `contracts/media.md`, MD-1 |
| 1600-9 | Keys pressed while a preset, a load, a reset or a power cycle runs are dropped (today: held back in Qt6, passed on in the Web frontend). Reset, Cold boot and a model switch stop what runs. | `contracts/typing.md`, T-4; `contracts/frontend.md`, Q-1 as amended |
| 1600-10 | Menus and the control bar follow the family structure: Settings… is in File; Help has a help item; Examples is a menu of its own in the Web frontend; on the PC-1600's bar a printer picker (none, CE-150, CE-1600P) replaces the CE-1600P button and the CE-158 has a button; a notice has no title; a control that cannot be used says why. | ADR 0024; LIB-14 (user, batch 5 gate) |
| 1600-11 | A preset is checked as far as possible before the running machine is given up: a module or a disk that is missing, a host drive folder that does not exist, a ROM image that does not fit, a key that is none and a character without a key refuse the preset and leave the machine alone (today: found after the old machine is gone). A preset opened without its folder is refused when it names a file. | `contracts/presets.md`, I-2; LIB-6 AC-7, AC-52, EC-5 (user, batch 5 gate) |
| 1600-13 | Media: a failed autosave is reported with a notice (today: silent); every change is written when the Web frontend's tab is hidden; a module change leaves the other slot and the disk alone; Name & Save asks before it replaces an instance of that name (today: the name is refused); deleting a saved medium in the Web frontend asks first. | LIB-7 AC-13, AC-14, AC-31, AC-38, AC-40 (user, batch 6 gate) |
| 1600-14 | Traces: the size limit holds for a preset's trace too, and a notice says when it ends a trace (today: a line in the INSPECTOR, for the button's trace only). A preset can pause the machine in both frontends; the Machine menu has Resume. "Copied to clipboard" is shown after Copy on the paper. | LIB-10 AC-24, AC-40, AC-43; LIB-14 AC-1; LIB-8 AC-20 (user, batch 6 gate) |
| 1600-12 | The paper panel is always shown: empty and without a name while no plotter is plugged in. The CE-150 and the CE-1600P each keep their paper until Cut, a preset that names them, or the end of the program: through detaching, a change of ROM, a module change and a model switch, also from a PC-1500 to a PC-1600 (today: the panel is there only with a plotter, and the paper is lost at each of these). | LIB-8 AC-8, AC-12, AC-36 (user, batch 6 gate) |
| 1600-8 | A held host key stays down until it is released, for every key, so the ROM's own auto-repeat works; the PC-1500's fixed key cadence goes once its key readiness is found. | `contracts/typing.md`, T-2, Q-3 |
