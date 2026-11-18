# Contract: settings and paths

Status: draft (2026-11-10). Forks ST-1 to ST-4 are decided (decision
log).

Inherits: ADR 0021 (settings hold preferences, the startup model and each
model's startup preset; nothing about hardware), 0022 (settings files are
INI, `Calc-U-59.ini`, `Calc-U-1600.ini`), 0037 (only the current version
of own formats is read; a one-time exception for the migration release),
Briefing §1.2 (settings become a file format;
"the first start of the new release converts the existing platform store
into the file"; a lookup hierarchy may follow later). Open questions
handed over: `media.md` Q-2 (`ti58c.mem`), Q-3 (the Calc-U-59 web save
folder).

## Today (facts)

| | Calc-U-59 | Calc-U-1600 |
|---|---|---|
| Store | UserDefaults (Swift), QSettings `Calc-U-59/Calc-U-59` (Qt), `localStorage` prefix `calcu59/` (web) | `shared::SettingsStore` behind QSettings (Qt) and the web's store; keys in `Shared/Settings.hpp` |
| Shared keys | `startupModel` (int, −1 = last used), `lastUsedModel`, `solidStateModuleID`, `traceMaxFileSizeMB`, `ledFontStyle`, `keyboardFeedback` (`Shared/SettingsKeys.hpp`) | `startup/modelPreference` (`last` or a model), `startup/lastUsedModel`, `preset/default/<model>`, `trace/maxFileSizeMB`, `trace/dirOverride`, `storage/instanceDirOverride`, `storage/tapeDirOverride`, `serial/linkDirectory`, `debug/dapEnabled`, `debug/dapPort`, `<group>openDir`, `<group>lastOpenDir`, `window/width`, `window/height` |
| App-only keys | Swift: `traceLocation`, `traceCustomPath`, `traceCustomPathBookmark`, `portraitDebugPage`; Qt: `storage/cardDirOverride`, `trace/dirOverride`, `windowGeometry`, `splitterState`, `preset/openDir`, `asm/openDir`, `lastOpenDir` | — |
| Save folder | Swift: iCloud Documents container, local Documents as fallback (`App/CardStorage.swift`); Qt: `~/Calc-U-59` or `storage/cardDirOverride`; web: in memory for the page session | `~/Calc-U-1600` or `storage/instanceDirOverride`, "no AppData/Application Support fallback in this design at all" (`Shared/Settings.hpp`); tapes in `Tapes` inside it; web: IDBFS |
| Traces | Swift: Application Support (default), iCloud or a custom folder (macOS, security-scoped bookmark); Qt: the card folder or `trace/dirOverride`; web: MEMFS, offered as download | the save folder or `trace/dirOverride` |
| `ti58c.mem` | Swift: next to the cards; Qt: the card folder; web: `localStorage` `calcu59/ti58c.mem` | — |
| Rule | — | the frontend passes `HostDirs` and a `SettingsStore`; Shared never decides locations (`docs/developer/Shared-Architecture.md:44-47`) |

## Responsibilities

- One settings implementation in the library reads and writes each
  product's INI file (Briefing §1.2, ADR 0022), with the product's keys
  documented in one place.
- Settings hold preferences, the startup model and each model's startup
  preset (ADR 0021), plus window and dialog state the frontends need
  (ST-4).
- The library resolves every path a product uses (settings file, save
  folder, cassettes, traces, serial links, `ti58c.memory.yaml`) from what the frontend
  passes (Calc-U-1600's rule: the frontend passes the host's directories,
  the library decides nothing about the platform).

## Non-responsibilities

- What a medium is and how it is saved (`media.md`).
- The settings dialog's look (frontend components, ADR 0023).
- ROM folders (ROM lookup, contract 9).

## Invariants

- I-1: Settings never hold hardware (ADR 0021).
- I-2: One file per product, one reader and writer, the same keys in
  every frontend of that product.
- I-3: A setting missing from the file has its documented default; an
  unknown key is kept when the file is written (hand edits survive).

## Open forks

### ST-1: Converting the platform stores on first start

**Decided (2026-11-10): option (a), no conversion.**

Facts: Briefing §1.2 asks the first start to convert the platform store
into the INI file. The inventory lists the stores as an own, unversioned
format (`calc-u-59.md`, Compatibility surface), and ADR 0037 now says an
own format is read only in its current version. The module choice is
already not converted (ADR 0021).

Options:
- (a) **Recommended:** no conversion; the new release starts with the
  defaults and writes the INI file. This follows ADR 0037; the user sets
  the few preferences again (startup model, LED style, keyboard
  feedback, trace size, folders). The Briefing's sentence is marked
  replaced. The release notes say so.
- (b) Convert once, as an exception to ADR 0037: the user keeps every
  preference, at the cost of reading code for three stores that is used
  once.

### ST-2: Where the settings file lives

**Decided (2026-11-10): option (a), platform convention.**

Options:
- (a) **Recommended:** one fixed, platform-conventional place per
  platform, no lookup hierarchy yet (Briefing §1.2 allows it later):
  macOS and iOS: the app's Application Support folder (inside the
  sandbox for the Swift app); Windows: `%APPDATA%\<Product>\`; Linux:
  `$XDG_CONFIG_HOME/<Product>/` (default `~/.config/<Product>/`; the
  product's name as written, `Calc-U-1600`, as on every platform); web:
  the persistent file system of the page (ST-4). File name
  `<Product>.ini`.
- (b) In the save folder (`~/Calc-U-1600/Calc-U-1600.ini`): easy to find,
  but on iOS and the sandboxed macOS app the save folder is iCloud, and
  the settings would sync across devices with different screen sizes.

### ST-3: The save folder per platform, and what goes in it

**Decided (2026-11-10): option (a).** Answers `media.md` Q-2 and Q-3.

Options:
- (a) **Recommended:** the defaults stay as today where they work:
  Swift (iOS, macOS): the iCloud Documents container with local Documents
  as fallback (Calc-U-59 today; cards sync across the user's devices);
  Qt (Windows, Linux, macOS): `~/<Product>` (both products today); web:
  persistent IndexedDB for both products (answers `media.md` Q-3:
  Calc-U-59's web save folder stops being per page session). An override
  in the settings on desktop Qt (`storage` section). Layout: instances
  and `ti58c.memory.yaml` at the root (answers `media.md` Q-2), cassettes
  in `Cassettes/` (`[storage] cassette-folder` names another); traces and screenshots, and the serial links (Calc-U-1600),
  default to the save folder on Qt and keep the Swift trace setting
  (Application Support, iCloud, custom).
- (b) One rule for every platform (`~/<Product>`): impossible in the
  sandboxed Swift apps and on the web.

### ST-4: Keys, values and the web

**Decided (2026-11-10): option (a).**

Options:
- (a) **Recommended:** INI sections follow Calc-U-1600's groups, names in
  kebab-case like the preset format, values in glossary terms rather
  than numbers: `[startup] model = last | TI-59 | …`, `[preset]
  TI-59 = <path>` (the startup preset per model), `[storage] save-folder =`,
  `cassette-folder =`, `[trace] folder =`, `max-file-size-mb = 50` (zero or less: 50), `[debug]
  dap-enabled =`, `dap-port =` (Calc-U-1600), `[serial] link-folder =`,
  `[display] led-style = modernized | classic` and `[keyboard] feedback =
  off | haptic | click` (Calc-U-59), `[window]` and `[folders]` for
  window and dialog state, written by the app. On the web the INI file
  is a file in the page's persistent file system (IDBFS), read by the same
  implementation, instead of `localStorage` keys.
- (b) Keep each product's present key names: two styles in one family.

## Fit table

| | Calc-U-59 | Calc-U-1600 |
|---|---|---|
| Frontends and platforms | Swift iOS, Swift macOS, Qt Windows/Linux (macOS dev), Web | Qt macOS/Windows/Linux, Web |
| Settings file (ST-2) | Application Support (Swift); `%APPDATA%`, `~/.config` (Qt); IDBFS (web) | Application Support (macOS Qt); `%APPDATA%`, `~/.config`; IDBFS |
| Save folder (ST-3) | iCloud Documents / local (Swift); `~/Calc-U-59` (Qt); IndexedDB (web) | `~/Calc-U-1600` (Qt); IndexedDB (web) |
| Product keys | `[display] led-style`, `[keyboard] feedback`, Swift trace location | `[debug]`, `[serial]`, `[storage] cassette-folder` |

(The machine columns of the other contracts do not apply: settings are
per product, not per model.)

## Open questions

- Q-1 (answered 2026-11-10, ST-3): the Swift macOS app keeps iCloud as
  its default save folder.

## Decision log

| Date | ID | Decision |
|---|---|---|
| 2026-11-10 | ST-1 | No conversion of the platform stores (ADR 0037); the new release starts with the defaults. Replaces the Briefing §1.2 sentence. |
| 2026-11-10 | ST-2 | `<Product>.ini` in the platform-conventional place: Application Support (macOS, iOS), `%APPDATA%` (Windows), `$XDG_CONFIG_HOME` (Linux), the page's persistent file system (web). No lookup hierarchy yet. |
| 2026-11-10 | ST-3 | Save folder: iCloud Documents with local fallback (Swift), `~/<Product>` with an override (Qt), persistent IndexedDB (web, both products). Instances and `ti58c.mem` at the root, tapes in `Tapes/`; traces, screenshots and serial links default to the save folder on Qt; Swift keeps its trace location setting. |
| 2026-11-10 | ST-4 | INI sections from Calc-U-1600's groups, kebab-case names, glossary values; app-written state in `[window]` and `[folders]`; on the web the INI is a file in IDBFS. |
| 2026-11-11 | ST-4 (completed, user, 2026-11-11) | The app-written keys are `[startup] last-model`, the keys of `[folders]` (one for each kind of file a product opens) and those of `[window]` (plain numbers; `portrait-debug-page` for the Swift frontend). The Swift trace location is `[trace] location = app-support | icloud | folder`; a Web frontend offers a finished trace as a download. |
| 2026-11-18 | ST-2, ST-3, ST-4 (corrected, release simplification R-6, D-3) | The code as it stands: the settings folder is `<Product>` as the product's name is written on every platform (`~/.config/Calc-U-1600/` on Linux); cassettes lie in `Cassettes/` in the save folder unless `[storage] cassette-folder` names another (was `Tapes/`); `[trace] max-file-size-mb` takes zero or less as 50. The ST-3 row's `Tapes/` stands for its date. |
