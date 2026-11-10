# LIB-9: Settings and paths

<!-- spec.md is the stable CONTRACT: WHAT, not HOW. It changes only
     deliberately, when the requirements change, and stays unchanged
     during implementation. Design lives in design.md, QA results in
     qa-report.md. No status or dates here: status lives only in
     features/INDEX.md. -->

## Dependencies
- Features: none.
- Contracts and decisions: `docs/contracts/settings.md` (ST-1 to ST-4,
  I-1 to I-3); ADR 0021, 0022, 0037; `docs/behaviour-changes.md` rows
  59-2, 59-9, 1600-2.

What the three status columns mean for this feature:
- **Library:** the settings file is read and written, and every path is
  resolved, by one implementation, shown with keys defined for the tests.
- **Calc-U-59, Calc-U-1600:** every frontend of the product keeps its
  settings in the product's settings file and takes its folders from the
  library; the platform stores are no longer read or written for
  settings (one exception: AC-34).

## User Stories
- As a user, I want my preferences in one documented file per product,
  so that I can read, edit and back them up.
- As a user who edits the file by hand, I want my edits to survive when
  the product writes the file.
- As a user, I want my media in a save folder I can find, and on the
  desktop I want to move it.
- As a user of the Swift frontend, I want my media in iCloud as today,
  so that they are on all my devices.
- As the developer of a product, I want to declare my keys and defaults
  once and have every frontend use them.
- As an agent, I want a test run to use settings and folders of its own,
  so that it does not touch mine.

## Out of Scope
- The settings dialog, and which settings a frontend offers in it
  (owner: LIB-14; the Swift one: C59-8).
- What a startup preset does, and how a value of `[preset]` names a
  bundled example (owner: LIB-6); building the startup model's machine
  (owner: LIB-18).
- What is stored in the save folder and how: media (LIB-7),
  `ti58c.mem` (C59-3), traces (`features/INDEX.md`, open point
  "Traces"), screenshots (LIB-6), serial links (LIB-2).
- The ROM folder (owner: LIB-12).
- The Web frontend's persistent file system: that it survives a page
  load (behaviour change 59-8) and its one-writer rule (owner: LIB-15).
  This feature only places the settings file and the save folder inside
  the file system the Web frontend passes.
- Which settings file and save folder a harness run uses when it names
  none, and the command-line form for naming them (owner: LIB-11).
- The help's settings chapter (owner: LIB-16).
- An exporter for settings and a lookup hierarchy of several settings
  files (`Briefing.md` §1.2: later).
- Converting the platform stores (decided: none, ST-1).

## Acceptance Criteria

### Library

The settings file
- [ ] **AC-1** — Given a product named `<Product>`, then its settings
  are kept in one INI file named `<Product>.ini`, with `[section]`
  headers and `key = value` lines; section and key names are kebab-case,
  except the keys of `[preset]`, which are model names as the glossary
  writes them (`settings.md` ST-4).
- [ ] **AC-33** — Given a settings file, then it is read by this
  grammar: UTF-8 without a byte order mark; both line endings; a line
  starting with `#` or `;` is a comment; a value is the rest of the line
  after `=`, trimmed, with no quoting and no escapes; section and key
  names are matched without regard to case; of a key given twice the
  last one counts; booleans are `true` and `false`; a path is absolute,
  and a leading `~` stands for the user's home folder.
- [ ] **AC-2** — Given a frontend on a platform, then the settings file
  is in exactly one place: the Application Support folder of that app on
  macOS and iOS (inside the sandbox container for a sandboxed app,
  `~/Library/Application Support/<Product>/` otherwise);
  `%APPDATA%\<Product>\` on Windows; `$XDG_CONFIG_HOME/<Product>/`, or
  `~/.config/<Product>/` when that variable is not set, on Linux; the
  file system the Web frontend passes. `<Product>` is the product's name
  as it is written (`Calc-U-59`, `Calc-U-1600`) on every platform. No
  other place is searched.
- [ ] **AC-3** — Given a settings file that lacks a key, when the
  setting is read, then it has its documented default.
- [ ] **AC-4** — Given a settings file with a key or a section the
  product does not know, when the product writes the file, then that key
  or section is still in the file with its value, and so are the
  comments, the blank lines and the order of the file.
- [ ] **AC-5** — Given no settings file, when the product starts, then
  every setting has its default and the product works; no file is
  written until a setting is changed, and the file then holds only the
  keys that have been set. A key that was set stays in the file, also
  when it is set back to its default.
- [ ] **AC-6** — Given a setting is changed in a frontend, when that
  frontend is quit and started again, then the setting has the new
  value. The file is read when the product starts and not again while
  it runs.
- [ ] **AC-7** — Given a settings file, then it states its format
  version, and a file in another version is treated as ADR 0037 says
  (where the version sits and what "refused" means for a settings file:
  OQ-2).
- [ ] **AC-8** — Given any product, then no setting chooses or remembers
  which modules, peripherals or media a machine has (`settings.md` I-1;
  ADR 0021).

Startup keys
- [ ] **AC-9** — Given `[startup] model`, then it accepts `last` or the
  name of one of the product's models, and its default is `last`; a
  value that is neither counts as `last` (both products today:
  Calc-U-59 `Shared/SettingsKeys.hpp:29-33`, Calc-U-1600
  `Shared/Settings.cpp:80-88`).
- [ ] **AC-10** — Given `[preset]`, then it has one key per model of the
  product, named after the model, whose value names that model's startup
  preset; an empty or missing value means that no startup preset is set.
- [ ] **AC-11** — Given `[trace] max-file-size-mb` is missing, zero or
  negative, then the limit is 50 (Calc-U-59
  `Shared/SettingsKeys.hpp:36-38`; both products' default).

Paths
- [ ] **AC-12** — Given the host directories its caller passes when the
  library starts, then the library gives the place of the settings file,
  the save folder, the tape folder, the trace folder, the folder for
  screenshots and the folder for serial links, and of `ti58c.mem`; no
  frontend and no other feature works out one of these places itself.
  A folder the user chooses through the platform (AC-24) is passed in by
  the frontend like a host directory.
- [ ] **AC-13** — Given a Qt6 frontend, then the save folder is
  `~/<Product>` in the user's home folder unless `[storage] save-folder`
  names another folder.
- [ ] **AC-14** — Given a Swift frontend (iOS or macOS), then the save
  folder is the app's iCloud Documents container, and the app's local
  Documents folder when iCloud is not available.
- [ ] **AC-15** — Given a Web frontend, then the save folder and the
  settings file are inside the file system that frontend passes.
- [ ] **AC-16** — Given the save folder, then media instances and
  `ti58c.mem` are at its root and cassette tapes are in `Tapes/` inside
  it, unless `[storage] tape-folder` names another folder.
- [ ] **AC-17** — Given a Qt6 frontend, then traces are written to the
  save folder unless `[trace] folder` names another folder, serial links
  to the save folder unless `[serial] link-folder` names another folder,
  and screenshots to the save folder (`settings.md` ST-3, ST-4; the
  other frontends: OQ-7).
- [ ] **AC-18** — Given a caller (the harness, a test) passes host
  directories of its own, then nothing is read from or written to the
  user's settings file and save folder.

### Calc-U-59
- [ ] **AC-19** — Given Calc-U-59, then its settings file is
  `Calc-U-59.ini` and holds these keys with these defaults:
  `[startup] model = last` (accepts `last`, `TI-59`, `TI-58`, `TI-58C`);
  `[preset] TI-59`, `TI-58`, `TI-58C` (empty);
  `[storage] save-folder` (empty; used by Qt6 only);
  `[trace] folder` (empty) and `max-file-size-mb = 50`;
  `[display] led-style = modernized` (accepts `modernized`, `classic`);
  `[keyboard] feedback = off` (accepts `off`, `haptic`, `click`);
  and the app-written sections `[window]` and `[folders]`
  (`settings.md` ST-4; today's defaults: Calc-U-59
  `Shared/SettingsKeys.hpp:13-24`; the app-written keys: OQ-5).
- [ ] **AC-20** — Given `[startup] model = last` and no model was used
  before, when Calc-U-59 starts, then the startup model is the TI-59
  (`Shared/SettingsKeys.hpp:29-33`).
- [ ] **AC-21** — Given the first start of the first release from this
  repository, then every setting has its default; nothing is taken from
  UserDefaults, QSettings or `localStorage` (behaviour change 59-2).
- [ ] **AC-22** — Given Calc-U-59, then no setting holds the Solid State
  ROM module (behaviour change 59-9; today: `solidStateModuleID`).
- [ ] **AC-23** — Given the Web frontend, then its settings are in
  `Calc-U-59.ini` and no `localStorage` key with the prefix `calcu59/`
  is read or written.
- [ ] **AC-24** — Given the Swift frontend, then the trace location can
  be Application Support (the default), iCloud, or on macOS a folder the
  user chooses, as today (`settings.md`, facts and ST-3; the keys:
  OQ-7).
- [ ] **AC-25** — Given Swift iOS, Swift macOS, Qt6 and Web, then the
  keys of AC-19 have the same names, values and defaults in each; a key
  a frontend has no use for (for example `[keyboard] feedback` in Qt6,
  `[storage] save-folder` outside Qt6) is kept in the file and ignored.
- [ ] **AC-34** — Given the Swift frontend on macOS and a trace folder
  the user chose, then the grant macOS gives for that folder (a
  security-scoped bookmark) stays in the platform store; the frontend
  resolves it and passes the folder in (AC-12). It is the only thing a
  frontend keeps outside the settings file.
- [ ] **AC-35** — Given the Qt6 frontend, then the size and place of its
  window and the split between its panels are in `[window]` as plain
  numbers.

### Calc-U-1600
- [ ] **AC-26** — Given Calc-U-1600, then its settings file is
  `Calc-U-1600.ini` and holds these keys with these defaults:
  `[startup] model = last` (accepts `last`, `PC-1500`, `PC-1500A`,
  `PC-1600`);
  `[preset] PC-1500`, `PC-1500A`, `PC-1600` (empty);
  `[storage] save-folder` and `tape-folder` (empty);
  `[trace] folder` (empty) and `max-file-size-mb = 50`;
  `[debug] dap-enabled = false` and `dap-port = 32168`;
  `[serial] link-folder` (empty);
  and the app-written sections `[window]` and `[folders]`
  (`settings.md` ST-4; today's defaults: Calc-U-1600
  `Shared/Settings.hpp:34-126`; the app-written keys: OQ-5).
- [ ] **AC-27** — Given `[startup] model = last` and no model was used
  before, when Calc-U-1600 starts, then the startup model is the
  PC-1500A (`Shared/Settings.hpp:114-126`).
- [ ] **AC-28** — Given `[debug] dap-port` is outside 1024 to 65535,
  then the port is 32168 (`Shared/Settings.cpp:75-78`).
- [ ] **AC-29** — Given `[folders]`, then for each kind of file the
  product opens (presets, BASIC programs, machine-code programs, the
  host drive folder) it holds either a fixed folder the dialog starts
  in, or, when none is set, the folder last used for that kind
  (`Shared/Settings.hpp:22-62`).
- [ ] **AC-30** — Given `[window]`, then it holds the last size the user
  gave the main window; without it the window is 900 × 500
  (`Shared/Settings.hpp:100-104`).
- [ ] **AC-31** — Given the first start of the first release from this
  repository, then every setting has its default; nothing is taken from
  QSettings or `localStorage` (behaviour change 1600-2).
- [ ] **AC-32** — Given Qt6 and Web, then the keys of AC-26 have the
  same names, values and defaults in each; a key the Web frontend has no
  use for (DAP, serial links, window size, folders;
  `docs/inventory/divergence-matrix.md`, M64) is kept in the file and
  ignored.

## Edge Cases
- **EC-1** — Given `[startup] model = last` and the last used model is
  not a model of the product (a hand edit, a file from another
  product), then the startup model is the product's fallback (AC-20,
  AC-27; both products today, sources as in AC-9).
- **EC-2** — Given the Swift frontend and iCloud becomes unavailable or
  available between two starts, then the save folder is the one that
  AC-14 gives at each start; media are not moved between the two
  folders.
- **EC-3** — Given `[storage] save-folder` names another folder and
  `[storage] tape-folder` is empty, then tapes go to `Tapes/` inside
  that folder.
- **EC-4** — Given the Calc-U-59 Swift frontend on macOS and a Qt6
  build of Calc-U-59 on the same Mac, then each has its own settings
  file (AC-2); a change in one does not show in the other.

- **EC-5** — Given a value its key does not accept (text where a number
  belongs, an unknown `led-style`), then the setting has its default,
  and the value stays in the file until that setting is changed.
- **EC-6** — Given a settings file that cannot be read as INI, then it
  is left untouched, the product starts with the defaults and says so in
  its log.
- **EC-7** — Given the product is ended by force while it writes the
  settings file, then the file holds either the old or the new content,
  never part of one.
- **EC-8** — Given the save folder, or `Tapes/` inside it, does not
  exist, when something is first written there, then it is created.
- **EC-9** — Given `[storage] save-folder` names a folder that cannot be
  created or written, then the user is told, and nothing is written to
  another folder instead.
- **EC-10** — Given two running instances of one product, when both
  write the settings file, then the last write wins and no key the
  other one does not know is lost. A hand edit made while the product
  runs can be lost at its next write.

## Technical Requirements (optional)
- The library decides nothing about the platform: every base folder
  (home, Application Support, Documents, the iCloud container, the Web
  frontend's file system) is passed in by its caller (`settings.md`,
  responsibilities; Calc-U-1600's rule today).
- Each product's keys, with section, default and accepted values, are
  declared in one place, from which the help's settings chapter is
  written (`settings.md`, responsibilities).
- The settings file is an own format (ADR 0037).

## Open Questions
- [ ] **OQ-2** — The version of the settings file. ADR 0037 names
  settings files among the own formats: a file in an older version, or
  without a version, is refused with a hint. Two things are open. Where
  the version sits in an INI file. And what "refused" means here, since
  the product must still start: I-3 and AC-3 to AC-5 make the file
  tolerant, and a user may write one by hand from the help.
  *Recommendation:* a `format-version` key at the top of the file. A
  file with an older or no version is not read: the product starts with
  the defaults, says so once with the hint, and keeps the old file
  beside the new one under another name when it first writes. No such
  file exists before the first release from this repository, so this
  matters only later and for hand-written files.
- [ ] **OQ-5** — The app-written keys. ST-4 names the sections
  `[window]` and `[folders]` but no keys. Needed: the last used model
  (both products keep `lastUsedModel` today; *recommendation:*
  `[startup] last-model`); the names in `[folders]` for Calc-U-1600's
  four kinds (AC-29) and for Calc-U-59's Qt6 keys of today
  (`preset/openDir`, `asm/openDir`, `lastOpenDir`); the names in
  `[window]`; and whether Calc-U-59's Swift key `portraitDebugPage` is
  kept. *Recommendation:* list them in this spec once named; dropping
  `portraitDebugPage` would be a visible change and need a row.
- [ ] **OQ-7** — Traces and screenshots outside Qt6. For the Swift
  frontend: the key names and values of the trace location (AC-24) next
  to `[trace] folder`, and where screenshots go. For the Web frontends:
  where traces and screenshots go (today Calc-U-59's Web frontend offers
  a trace as a download, `settings.md`, facts).

## Decision Log

| Decision | Rationale | Date |
|----------|-----------|------|
| Settings hold preferences, the startup model and each model's startup preset; nothing about hardware | ADR 0021; `settings.md` I-1 | 2026-11-09 |
| The settings file is INI, `<Product>.ini` | ADR 0022 | 2026-11-09 |
| No conversion of the platform stores | `settings.md` ST-1; ADR 0037 | 2026-11-10 |
| One fixed place per platform by its convention; no lookup hierarchy yet | `settings.md` ST-2 | 2026-11-10 |
| Save folder: iCloud Documents with local fallback (Swift), `~/<Product>` with an override (Qt6), persistent IndexedDB (Web); the layout inside it | `settings.md` ST-3 | 2026-11-10 |
| Sections from Calc-U-1600's groups, kebab-case names, glossary values; app-written state in `[window]` and `[folders]`; on the web the INI is a file | `settings.md` ST-4 | 2026-11-10 |
| A missing key has its default; an unknown key is kept on write | `settings.md` I-3 | 2026-11-10 |
| Proposed in this spec, for the gate: the trace size rule of AC-11 holds for both products (Calc-U-1600 reads a non-number as 0 today) | One reader, one rule (`settings.md` I-2) | 2026-11-10 |
| A key is written only once it has been set; the file is read at the start only; comments, blank lines and order survive a write | Recommendation applied; the user raised no objection at the batch 1 gate | 2026-11-10 |
| A bad value counts as missing; an unreadable file is left untouched; a write never leaves half a file | Recommendation applied; the user raised no objection at the batch 1 gate | 2026-11-10 |
| The INI grammar of AC-33 | ADR 0022 fixes only sections and `key = value`; recommendation applied; the user raised no objection at the batch 1 gate | 2026-11-10 |
| The settings folders carry the product's name as written, on every platform | Recommendation applied; the user raised no objection at the batch 1 gate | 2026-11-10 |
| The macOS bookmark of a custom trace folder stays in the platform store; Qt6 window state is plain numbers in `[window]` | A bookmark is a grant by macOS, not a preference; recommendation applied; the user raised no objection at the batch 1 gate | 2026-11-10 |
| A missing save folder is created at the first write; an override that cannot be used is reported and never replaced silently | Recommendation applied; the user raised no objection at the batch 1 gate | 2026-11-10 |
