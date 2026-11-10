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
| 59-5 | A preset's steps run at full speed unless a speed step asks for regular speed; user keys are dropped while it runs. | `contracts/presets.md`, PR-4; `contracts/typing.md`, T-4 |
| 59-6 | Magnetic cards live in the save folder. A card file elsewhere is no longer opened and written back in place; a card gets a name inside its file. | ADR 0019; `contracts/media.md`, MD-2, MD-5 |
| 59-7 | The first write to a card a preset names makes an instance in the save folder and replaces an instance of that name. | `contracts/media.md`, MD-1 |
| 59-8 | On the web, magnetic cards persist across page loads. When the product is open twice in one browser, only the first copy keeps what is saved; the second says so. | `contracts/settings.md`, ST-3; `features/LIB-15-web-client-shell/spec.md` |
| 59-9 | The Solid State ROM module is chosen in the control bar, not in the settings, and the choice is not remembered across starts; a startup preset sets it. The picker also offers an empty slot. | ADR 0021, 0025; `contracts/presets.md`, PR-9 |
| 59-10 | Without a startup preset the product starts with a TI-59, the Master Library module and no printer (today: printer connected). | ADR 0021 |
| 59-11 | Changing the module, or connecting or disconnecting the PC-100C, power-cycles the machine: the TI-58 and TI-59 lose program and data registers; the TI-58C keeps its constant memory. | `contracts/peripherals.md`, P-1 |
| 59-12 | The TI-58C keeps its constant memory across starts only while no startup preset is set for it; a startup preset cold boots it. | `contracts/capabilities.md`, C-3 |
| 59-13 | A second key pressed while another is held no longer releases the first; both reach the key matrix. | `contracts/typing.md`, T-3 |
| 59-14 | Menus and the control bar follow the family structure. | ADR 0024 |

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
| 1600-8 | A held host key stays down until it is released, for every key, so the ROM's own auto-repeat works; the PC-1500's fixed key cadence goes once its key readiness is found. | `contracts/typing.md`, T-2, Q-3 |
