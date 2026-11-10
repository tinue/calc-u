# Contract: media and save folder

Status: draft (2026-11-10). Forks MD-1 to MD-5 are decided (decision
log; MD-5 was added in the phase 4 review); open questions remain.

Inherits: ADR 0019 (media live in the save folder; a medium a preset
names by path is never written, its first write makes an instance; the
copy's name and collisions are this contract's), 0020 (cards written in
the current version), 0037 (only the current version is read; older
files get a hint; a one-time exception for the migration release, after
which a release reads one version back). Oddities: Calc-U-1600's "`saveas:` needs
`template`/`live`; name lives in the file" (`docs/oddities-triage.md`).
Builds on `presets.md` (PR-8: fit, insert, eject, arm; media places
pass-through or holding; `medium:` sections; `saveas:`).

Glossary: medium, template, instance, Name & Save (autosave), insert and
eject, fit, save folder.

## Today (facts)

| | Calc-U-59 | Calc-U-1600 |
|---|---|---|
| Media | magnetic cards (TI-59) | memory modules (battery-backed ones are media), floppy disks, cassette tapes |
| Files | card text `Calc-U-59 Card 2.0` (`.u59`), one file for both sides; 1.0 and two binary layouts read (`Shared/CardFile.hpp`) | `<name>.card.yaml`, `<name>.floppy.yaml`, `format-version: 1`; tapes are WAV (external) |
| Name | the file name; `.U59` added when missing (`StateFileFormat.md`, CARD) | inside the file (`module-name:`); the file name is derived, `/` and `:` become `-` (`User-Guide.md:516-518`) |
| Templates | — (any `.u59` file is opened and written back in place) | bundled modules and disks, never written; user templates via `saveas: template` |
| Instances | card files in the card folder (Swift: iCloud Documents or local; Qt: `~/Calc-U-59` or Settings; web: in memory for the page session) | Name & Save makes one in the save folder (default `~/Calc-U-1600`; web IDBFS), autosaves about 0.5 s after a change (`User-Guide.md:484-518`) |
| Collisions | `ArmCard: Results` writes `Results.U59`, creating it if missing | Name & Save: the GUI handles a name collision interactively; `saveas: live` overwrites a file of the same name but never takes a template's name; `saveas: template` may replace a user template, never a bundled name; `file:` form writes exactly that file (`Decisions.md`, `Core/Connector/MediaSave.hpp`) |
| Old or bad files | damaged card refused whole (ODD59-11) | a saved card without `format-version` drops out of the picker without a message, on purpose (`Decisions.md`, File formats) |
| Outside edits | — | a floppy edited outside while inserted is overwritten ("eject first", `TODO.md:470-474`) |
| Preset media | `mem:` cards, never saved | — |
| Other user files in the save folder | `ti58c.mem` (Qt: card folder; Swift: next to the cards; web: `localStorage`) | traces, screenshots, serial-link symlinks (default: the save folder) |

## Responsibilities

- The library keeps one media model for every product (ADR 0019): a
  medium is a template (bundled, or a user template) or an instance in
  the save folder; Name & Save makes an instance; an instance autosaves
  after a change.
- A picker lists the templates and the save folder's instances that fit
  the media place.
- A medium named by a preset (by name, by `file:`, or a `medium:`
  section) follows PR-8 and ADR 0019: a file named by path is never
  written; its first write makes an instance (MD-1); a `medium:` medium
  is never saved.
- Saving under a name follows one set of rules (MD-3) for the GUI's Name
  & Save, preset `saveas:` and the harness (as Calc-U-1600's
  `planMediaSave` already does for both of its paths).
- A file in an older version is refused with the hint (ADR 0037, MD-4).

## Non-responsibilities

- Where the save folder is on each platform, and settings that move it:
  contract 7.
- What a medium's content means (card banks, floppy sectors): the core's
  media capability.
- External formats (cassette WAV) follow their own rules; ADR 0037 does
  not touch them.

## Invariants

- I-1: A template is never written.
- I-2: The emulator never loses a write: a write to a template or to a
  preset-named file goes to an instance (ADR 0019).
- I-3: The emulator overwrites an instance only when the user or a
  preset asks for it: Name & Save after confirmation, a preset's
  explicit `saveas:`, or a preset's first write to a medium it names
  (MD-1).
- I-4: One media model and one set of save rules for every product,
  frontend and the harness.

## Open forks

### MD-1: The copy made on the first write of a preset-named medium (ADR 0019)

**Decided (2026-11-10): option (b), overwrite** (the user's choice; (a)
was recommended). The instance takes the medium's name and replaces an
instance of that name, so a preset's re-run replaces the last run's
results. Consequence: a user's own instance that happens to share the
name is replaced too; the preset log says which instance was written.
I-3 is worded accordingly.

Situation: a preset names `file: results.u59` (or a template by name);
the program writes to it; the write goes to a new instance in the save
folder. Running the preset again starts from the unchanged file and
makes the copy again.

Options:
- (a) **Recommended:** the instance takes the medium's name; if an
  instance of that name exists, it gets the first free numbered name
  (`results 2`, `results 3`, …). Nothing is overwritten (I-3) and nothing
  asks in the middle of a preset run (a preset is one operation, PR-4);
  the preset log and a notice say which name was used. Cost: repeated
  runs leave numbered copies.
- (b) Overwrite the instance of that name: a re-run replaces the last
  run's results, but also a user's own instance that happens to share the
  name (breaks I-3).
- (c) Ask the user when the write happens: interrupts the run; the
  harness and headless runs have nobody to ask.

### MD-2: Where a medium's name lives

**Decided (2026-11-10): option (b), the name lives in the file**, for
every medium (the user's choice; (a) was recommended). Together with
MD-3: the file is named after the name (plus its suffix), and a name
with a character a file name cannot hold is refused, so name and file
name agree when the emulator writes them. A file renamed outside the
emulator keeps the name inside it. Consequences:
- The magnetic card format gets a name field and a new version; under
  ADR 0037 every Calc-U-59 user's saved cards (2.0 and older) are refused
  after the update with the hint. The release notes say so.
- Calc-U-1600's `module-name:` rule stays (triage: "name lives in the
  file", stays).

Facts: Calc-U-59 cards are named by their file name; Calc-U-1600 modules
and disks carry the name inside the file. Under ADR 0037 the side whose
format changes gets a new version, and every earlier file of that kind
is refused.

Options:
- (a) **Recommended:** the file name is the name, for every medium.
  Calc-U-59's card format 2.0 stays as it is, so users' saved cards keep
  working after the update; Calc-U-1600's module and disk formats drop
  `module-name:` / the disk name and move to a new version (Calc-U-1600
  needs no backwards compatibility; its few saved instances are refused
  with the hint). Renaming a file renames the medium; names are limited
  to what a file name can hold.
- (b) The name lives in the file, for every medium (Calc-U-1600's rule):
  any character in a name; but the card format changes, so every
  Calc-U-59 user's saved cards are refused after the update.

### MD-5: Name, file name and two files with one name

**Decided (2026-11-10, user, in the phase 4 review).** Refines MD-2.

- The name inside the file is the medium's name. The file name counts
  only as a fallback, for a file that carries no name inside.
- A save dialog asks for the name only; the file name is derived from
  it. If a file of that derived name already exists and holds a medium
  with a different name, the new file gets a numbered name: with a file
  `foo` whose medium is called "bar", saving a medium called "foo"
  writes `foo-1`.
- Two files that carry the same name inside both show in the picker
  under that name. Preventing this is the user's.

### MD-3: Saving under a name, and collisions

**Decided (2026-11-10): option (a).** The character rule now follows
MD-2 (b): names are written into the file, and the file is named after
them.

Options:
- (a) **Recommended:** Calc-U-1600's rules for every product. Name &
  Save in the GUI asks before replacing an instance of that name and
  never takes a template's name. `saveas: live` replaces an instance of
  that name (the preset asks for it explicitly). `saveas: template` may
  replace a user template, never a bundled one. The `file:` form writes
  exactly that file. A name with a character a file name cannot hold is
  refused with a message; today's silent `/` → `-` goes.
- (b) Keep Calc-U-1600's silent replacement of `/` and `:`: two
  different names can then map to one file.

### MD-4: How refused older files show (ADR 0037)

**Decided (2026-11-10): option (a).**

Facts: Calc-U-1600 today drops a card without `format-version` from the
picker silently, on purpose; ADR 0037 asks for a clear hint.

Options:
- (a) **Recommended:** an outdated file in the save folder appears in its
  picker as a disabled entry marked outdated; choosing or hovering it
  shows the hint (which format, which version is current, where the
  format is documented). A preset that names one is refused with the same
  hint (ADR 0017). The scan reports each outdated file once per start in
  the log, not as a pop-up.
- (b) Left out of the picker, with one notice per start listing the
  outdated files: no clutter in the picker, but a notice at every start
  until the files are fixed.

## Fit table

| | TI-59 (58, 58C) | PC-1500 (1500A) | PC-1600 | PC-1403 (paper) | PC-1350 (paper) |
|---|---|---|---|---|---|
| Media places (PR-8) | card reader, pass-through, sides 1/2 (TI-59 only) | module slot (battery-backed modules), cassette (CE-150) | 2 module slots, floppy (CE-1600F; holding, sides A/B), cassette (CE-1600P) | cassette (CE-126P, CE-124) | RAM card slot (CE-201M, CE-202M: battery-backed), cassette |
| Templates | bundled cards: none today | bundled modules | bundled modules and disks (Blank, Formatted) | — | — |
| Instance files | `.u59` (card 2.0) | `.card.yaml` | `.card.yaml`, `.floppy.yaml` | — | — |
| Name (MD-2) | in the file (new card version) | in the file | in the file | — | — |
| Preset-defined media | `medium:` (today `CARD: mem:`) | `medium:` possible | `medium:` possible | — | — |
| External media formats | — | cassette WAV | cassette WAV | cassette WAV | cassette WAV |

## Open questions

- Q-4 (answered 2026-11-10, user): the magnetic card format gets a new
  version, as MD-2 says. The user's condition for keeping 2.0 was that
  the current reader ignores an unknown keyword; it does not:
  Calc-U-59 2.3.0 refuses a card text 2.0 file with an unknown line at
  the top level or in `HEADER:` and skips unknown keys only inside
  `CUECARD:` (`Shared/CardFile.cpp`, `parseCardTextV2`; `Shared/CueCard.cpp`,
  `parseLine`). A name field is therefore an incompatible change. Saved
  2.0 cards are refused once at the update (ADR 0037).
- Q-5 (phase 6): When two instances carry the same name (MD-5), which
  one a save by name replaces (`saveas: live`, Name & Save, a preset's
  first write).
- Q-1 (answered 2026-11-10): cassette tapes keep their own rules (fixed role,
  written at each motor stop into `Tapes/`); WAV is an external format.
  Background: today fixed-role (play or record) and written at
  each motor stop into the tapes folder; whether a recorded tape is an
  instance with Name & Save like the other media, or keeps its own rules,
  is open (the WAV format itself is external).
- Q-2 (answered 2026-11-10, `settings.md` ST-3): `ti58c.mem` is not a
  medium; it lives at the root of the save folder.
- Q-3 (answered 2026-11-10, `settings.md` ST-3): the web save folder is
  persistent IndexedDB in both products.

## Decision log

| Date | ID | Decision |
|---|---|---|
| 2026-11-10 | MD-1 | The first write to a preset-named medium makes an instance with the medium's name and replaces an instance of that name. Settles ADR 0019's open point. |
| 2026-11-10 | MD-2 | Every medium carries its name inside its file; the file is named after it. The card format gets a name field and a new version; saved Calc-U-59 cards are refused after the update (ADR 0037). |
| 2026-11-10 | MD-3 | Calc-U-1600's save rules for every product, frontend and the harness; a name a file name cannot hold is refused. |
| 2026-11-10 | MD-5 | The name inside the file is the name; the file name is only the fallback for a file without one. The file name is derived from the name, numbered (`foo-1`) when that file exists with another name inside. Two files with the same name both show; preventing it is the user's. |
| 2026-11-10 | MD-2 (confirmed) | The magnetic card format gets a new version for the name field; 2.3.0's reader refuses unknown keywords, so the change is incompatible (Q-4). |
| 2026-11-10 | MD-4 | Outdated files show as disabled "outdated" entries in their picker with the hint; a preset naming one is refused with the same hint; logged once per start. |
