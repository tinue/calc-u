# LIB-9 Settings and paths: design
Status: approved by the user at the Part B/C1 boundary (2026-11-14)

## 1. Scope

| Column | Step | Tests from (plan §3) | Done in (plan §5.1) |
|---|---|---|---|
| L | M-20 | AC-1–18, AC-33; EC-1–10 | M-20 |
| 1600 | M-30 | AC-26–32 | M-30 |
| 59 | M-32 | AC-19–25, AC-34–35 (AC-23 with M-34) | M-34 |

- M-20 waits for nothing; M-30 for M-20 and M-26; M-32 for M-20 and M-19.
- AC-23 (no `calcu59/` key for settings) passes in M-34 (LIB-15 59), when
  the Calc-U-59 Web frontend gets its persistent file system. M-32 keeps
  the Web settings behind an adapter (§5, A-5).
- Nothing of this spec lands outside C1. AC-17's Web part (a trace and a
  screenshot are offered as a download) is shown in M-20 with the
  library alone; the products' Web frontends get it with LIB-10 and LIB-6
  (§5, A-3, A-4).
- No first step of `docs/walkthroughs/first-steps.md` concerns this
  feature.

## 2. Modules and files

### Library (M-20): `library/settings/`, namespace `calcu::settings`, in `calcu_library` (LIB-1 §2a)

| File | Holds |
|---|---|
| `library/settings/IniDocument.hpp/.cpp` | the lossless INI document: parse (AC-33), look-up, change in place, write back (AC-4) |
| `library/settings/Declaration.hpp/.cpp` | `KeyDecl`, `ProductDecl`; the family keys every product shares (`family::`); value checks (AC-3, AC-9–11, EC-5) |
| `library/settings/SettingsFile.hpp/.cpp` | load at start, create with defaults, `format-version`, typed reads, write-through on change (AC-5–7, EC-6, EC-7, EC-10) |
| `library/settings/HostDirs.hpp` | `FrontendKind`, `HostDirs`: what the caller passes (AC-12, AC-18) |
| `library/settings/PlatformBase.hpp/.cpp` | pure helper: the settings base of AC-2 from inputs the caller passes (OS, environment values, home) |
| `library/settings/Paths.hpp/.cpp` | the settings file, save folder, tape, trace, screenshot and serial-link folders, files at the save folder's root (AC-2, AC-12–17, EC-3, EC-8, EC-9) |
| `library/settings/Startup.hpp/.cpp` | the startup model, the startup preset per model (AC-9, AC-10, EC-1) |
| `library/settings/OpenFolders.hpp/.cpp` | the folder a file dialog starts in, per kind of file (AC-29; Calc-U-59's `[folders]`) |
| `library/settings/AtomicFile.hpp/.cpp` | write beside, rename into place (EC-7) |
| `library/settings/tests/TestProduct.hpp` | a product declared for the tests (`Test-Product`, models `Model-A`, `Model-B`, fallback `Model-B`, one key of each kind) |
| `library/settings/tests/ini_document_tests.cpp` | grammar and round trip |
| `library/settings/tests/settings_file_tests.cpp` | defaults, creation, version, bad values, atomic write, two writers |
| `library/settings/tests/paths_tests.cpp` | every platform's settings place, save folder per frontend, folder layout and overrides |
| `library/settings/tests/startup_tests.cpp` | startup keys, trace size, open folders |
| `library/CMakeLists.txt` (changed) | the sources of `library/settings/` join `calcu_library` (also under Emscripten), its tests join `calcu_library_tests` |
| `library/settings/check_platform_stores.sh` | source check used by both products: no QSettings, UserDefaults/`@AppStorage` or `localStorage` for settings in a product's frontends, with a named allow-list (AC-21, AC-31, AC-34) |

### Calc-U-1600 (M-30)

| File | Change |
|---|---|
| `products/calc-u-1600/app/Settings.hpp/.cpp` | `shared::SettingsStore` goes; `shared::Settings` becomes the product's declaration (`settingsDecl()`, AC-26) plus typed accessors over a `calcu::settings::SettingsFile&` |
| `products/calc-u-1600/app/AppPaths.hpp/.cpp` | folder resolution moves to `calcu::settings::Paths`; Name & Save planning, `displayPath`, `joinPath`, `isUnderDir` stay; `atomicWriteFile` stays for media (LIB-7's) |
| `products/calc-u-1600/app/tests/MemorySettingsStore.hpp` | deleted; tests use `SettingsFile` on a temporary folder |
| `products/calc-u-1600/app/tests/shared_settings_tests.cpp` | rewritten for AC-26–30, AC-32 |
| `products/calc-u-1600/app/tests/shared_{app_session,bay,machine_session,preset_catalog}_tests.cpp`, `tools/wasm_smoke.cpp` | build `HostDirs` and a `SettingsFile` instead of `MemorySettingsStore` |
| `products/calc-u-1600/frontends/qt6/app/AppSettings.hpp` | `QtSettingsStore`, `backingStore()`, `isolatedStorePath()` go; the QString wrappers stay, over the new `shared::Settings` |
| `products/calc-u-1600/frontends/qt6/app/AppPaths.hpp/.cpp` | builds `HostDirs` (Qt6, home, settings base); `setIsolatedInstanceDir` goes |
| `products/calc-u-1600/frontends/qt6/app/main.cpp` | `--shots` passes temporary `HostDirs` (AC-18) instead of `isolatedStorePath()`/`setIsolatedInstanceDir()` (lines 116-122) |
| `products/calc-u-1600/frontends/qt6/app/MacAppSupport.mm/.h` | adds the Application Support lookup for the settings base on macOS |
| `products/calc-u-1600/frontends/web/bindings.cpp` | `WebSettingsStore` (lines 193-236) goes; `WebController` opens the INI file at `/persist/settings/Calc-U-1600/Calc-U-1600.ini` (`settingsBase = /persist/settings`; LIB-15 §3.3, D-6) |
| `products/calc-u-1600/frontends/web/src/engine.js` | `STORAGE_PREFIX`, `storageSet`, `storedSettings` and the `store` message go (lines 20-39, 175) |
| `products/calc-u-1600/frontends/web/calc-engine-worker.js` | the `init` message no longer carries `settings` |

### Calc-U-59 (M-32)

| File | Change |
|---|---|
| `products/calc-u-59/app/SettingsKeys.hpp` | deleted |
| `products/calc-u-59/app/Settings.hpp/.cpp` (new) | `cu59::settings::settingsDecl()` (AC-19) and typed accessors over `calcu::settings::SettingsFile` |
| `products/calc-u-59/tests/shared_settings_tests.cpp` (new) | AC-19, AC-20, AC-22, AC-24, AC-25, AC-35; replaces `TEST(settings_resolve)` in `tests/shared_formats_tests.cpp:470` |
| `products/calc-u-59/frontends/qt6/app/AppSettings.hpp` | QSettings goes; wrappers over `cu59::settings` |
| `products/calc-u-59/frontends/qt6/app/AppPaths.hpp/.cpp` | `defaultDir`, `cardDir`, `traceDir`, `constantMemoryFile` forward to `calcu::settings::Paths` |
| `products/calc-u-59/frontends/qt6/app/MainWindow.cpp` | `saveGeometry`/`restoreGeometry` and the splitter's `saveState` (lines 48-49, 112-113) become `[window]` numbers (AC-35) |
| `products/calc-u-59/frontends/qt6/app/SettingsDialog.cpp`, `KeyboardWidget.cpp`, `EmulatorController.cpp`, `main.cpp` | read and write through the wrappers |
| `products/calc-u-59/frontends/swift/Bridge/TISettings.h/.mm` | the Swift face of `cu59::settings` and the `SettingsFile` (get/set by key, typed) |
| `products/calc-u-59/frontends/swift/App/AppSettings.swift` | an observable settings model over `TISettings` replaces `UserDefaults` and `SettingsKey`; the bookmark alone stays in `UserDefaults` (AC-34) |
| `products/calc-u-59/frontends/swift/App/Views/SettingsView.swift`, `CalculatorView.swift`, `KeyboardView.swift`, `EmulatorViewModel.swift` | `@AppStorage` and `UserDefaults` calls go (lines listed in §4) |
| `products/calc-u-59/frontends/swift/App/CardStorage.swift` | stays: resolves iCloud or local Documents and passes it as the save folder |
| `products/calc-u-59/frontends/swift/Calc-U-59UITests/ScreenshotTests.swift` | `settingsArguments` (lines 31-36) become settings given at launch for that run only (§4) |
| `products/calc-u-59/frontends/web/bindings.cpp`, `src/engine.js` | settings through `SettingsFile`; the INI text kept by adapter A-5 until M-34 |

## 3. Seams and interfaces

Sketches; names in the code may differ in detail. C++20 (ADR 0005): no
`std::expected`, results are small structs.

```cpp
namespace calcu::settings {

// ── Declaration: each product's keys in one place (Technical Requirements) ──
enum class ValueKind { Text, Path, Boolean, Integer, Choice, ModelOrLast, ModelName };
struct KeyDecl {
    std::string section, key;          // kebab-case; [preset] keys are model names (AC-1)
    ValueKind kind;
    std::string defaultValue;          // as written in the file (AC-3, AC-5)
    std::vector<std::string> choices;  // Choice (led-style, feedback, location)
    std::optional<long> min, max;      // Integer; outside -> default (AC-11, AC-28)
    bool writtenByApp = false;         // [window], [folders], last-model
    std::string help;                  // one line, for LIB-16's settings chapter
};
struct ProductDecl {
    std::string productName;           // "Calc-U-59", "Calc-U-1600" (AC-1, AC-2)
    std::vector<std::string> models;   // glossary model names
    std::string fallbackModel;         // AC-20, AC-27, EC-1
    std::vector<KeyDecl> keys;         // family:: keys the product has + its own
};
namespace family {  // the keys ST-4 gives both products; a product lists those it has
    KeyDecl startupModel(); KeyDecl startupLastModel(); KeyDecl presetFor(std::string model);
    KeyDecl saveFolder(); KeyDecl tapeFolder(); KeyDecl traceFolder();
    KeyDecl traceMaxFileSizeMb(); KeyDecl traceLocation(); KeyDecl serialLinkFolder();
    KeyDecl openFolder(std::string kind); KeyDecl windowNumber(std::string key, long def);
}
inline constexpr int kFormatVersion = 1;  // the top-level `format-version` (AC-7)

// ── INI document (AC-33, AC-4) ──
struct ParseResult;  // {std::optional<IniDocument> doc; std::string problem; int line;}
class IniDocument {
public:
    static ParseResult parse(std::string_view bytes);
    std::optional<std::string> get(std::string_view section, std::string_view key) const;
    void set(std::string_view section, std::string_view key, std::string_view value);
    std::string text() const;          // unchanged document -> the same bytes
};

// ── Settings file (AC-3, AC-5–7, EC-5–7, EC-10) ──
enum class Outcome { Read, Created, Unreadable, NoVersion, OtherVersion, DefaultsOnly };
class SettingsFile {
public:
    static SettingsFile open(const ProductDecl&, std::string path);   // read once, at start
    static SettingsFile defaultsOnly(const ProductDecl&);             // nothing read or written (AC-18; LIB-11 AC-8)
    Outcome outcome() const;  const std::string& problem() const;     // for the log / a notice
    std::string text(std::string_view section, std::string_view key) const;  // declared default if missing or not accepted
    bool boolean(...) const;  long integer(...) const;
    struct WriteResult { bool ok; std::string problem; };
    WriteResult set(std::string_view section, std::string_view key, std::string value);  // write-through, atomic
};

// ── Host directories and places (AC-2, AC-12–18) ──
enum class FrontendKind { Qt6, Swift, Web };   // the harness passes Qt6 with its own HostDirs (D-11)
struct HostDirs {
    FrontendKind frontend;
    std::string home;                // `~` in values (AC-33); Qt6: base of the save folder (AC-13)
    std::string settingsBase;        // AC-2: Application Support / %APPDATA% / XDG / the Web file system
    std::string saveFolder;          // Swift: iCloud or local Documents (AC-14); Web: in its file system (AC-15); Qt6: empty
    std::string applicationSupport;  // Swift: [trace] location = app-support (AC-24)
    std::string chosenTraceFolder;   // Swift macOS: the folder the bookmark grants (AC-34); else empty
};
enum class HostOs { MacOS, IOS, Windows, Linux, Web };
std::string settingsBaseFor(HostOs, const std::function<std::optional<std::string>(const char*)>& env,
                            const std::string& home, const std::string& applicationSupport);
std::string settingsFilePath(const ProductDecl&, const HostDirs&);  // <settingsBase>/<Product>/<Product>.ini

enum class Folder { Save, Tapes, Traces, Screenshots, SerialLinks };
class Paths {
public:
    Paths(const ProductDecl&, const SettingsFile&, HostDirs);
    std::string settingsFile() const;
    std::string saveFolder() const;                         // AC-13–15
    std::string tapeFolder() const;                         // AC-16, EC-3
    std::optional<std::string> traceFolder() const;         // AC-17, AC-24; none on Web
    std::optional<std::string> screenshotFolder() const;    // AC-17; none on Web
    std::optional<std::string> serialLinkFolder() const;    // AC-17; Qt6 only
    std::string saveFolderFile(std::string_view name) const;  // AC-16: `ti58c.memory.yaml`
    struct Ensured { bool ok; std::string path, problem; };
    Ensured ensure(Folder) const;  // created at first write (EC-8); an override that fails is reported, no other folder used (EC-9)
};

// ── Startup and dialogs ──
std::string startupModel(const ProductDecl&, const SettingsFile&);           // AC-9, EC-1
std::string startupPreset(const SettingsFile&, std::string_view model);      // AC-10: empty = none
std::string openStartFolder(const SettingsFile&, std::string_view kind, const std::string& home);  // AC-29
void rememberOpenFolder(SettingsFile&, std::string_view kind, const std::string& folder);
void setFixedOpenFolder(SettingsFile&, std::string_view kind, const std::string& folder);
}
```

| Seam | ACs/ECs |
|---|---|
| `IniDocument` | AC-1, AC-33, AC-4, EC-6 |
| `ProductDecl`, `family::`, value checks | AC-1, AC-3, AC-8 (no hardware key can be declared: the declaration has no kind for it, and the product tests list every key), AC-9–11, EC-5 |
| `SettingsFile` | AC-3, AC-5, AC-6, AC-7, EC-5, EC-6, EC-7, EC-10 |
| `settingsBaseFor`, `settingsFilePath` | AC-2, EC-4 |
| `HostDirs`, `Paths` | AC-12–18, EC-2, EC-3, EC-8, EC-9 |
| `startupModel`, `startupPreset` | AC-9, AC-10, EC-1 |
| `openStartFolder` & co. | AC-29 (and Calc-U-59's `[folders]`, AC-19) |

Rules the seams carry:
- **Read once** (AC-6): `open` reads the file; nothing re-reads it while
  the product runs. **Write-through:** `set` changes the document and
  writes the whole file at once (beside, then rename). It writes nothing
  when the value is unchanged.
- **First start** (AC-5): no file → `Outcome::Created`; the file is
  written with `format-version = 1` at the top, then every declared key
  at its default, in declaration order, one `# help` line above each.
  App-written keys without a default are written empty.
- **Version** (AC-7): `format-version` is a top-level key (before the
  first section). Missing → `NoVersion`; another number →
  `OtherVersion`; both start with the defaults and say so through
  `problem()`. **Unreadable** (EC-6) → `Unreadable`: a BOM, bad UTF-8, or
  a non-blank, non-comment line that is neither `[section]` nor holds
  `=` (keys before the first section are the top level). In these three
  states nothing is written to that path in this run; a change the user
  makes then lasts until quit.
- **Bad value** (EC-5): a read returns the default; the raw text stays in
  the document and is written back unchanged until `set` replaces it.
- **Two instances** (EC-10): `set` merges on write: it reads the file on
  disk, changes only the keys this instance set, and writes it back, so
  no key of the other instance is lost; the last write wins for a key
  both set. Values in memory are not refreshed from disk (AC-6 read as
  "not reloaded").
- **`~`** (AC-33): a path value starting with `~` is expanded against
  `HostDirs::home` by `Paths`; the stored text keeps the `~`.
- **Platform**: the library reads no environment variable and calls no
  platform API. `settingsBaseFor` takes the OS, a lookup the caller
  supplies and the folders the caller found; it holds AC-2's rule once
  and is testable on every host (M-20's exit check). It is a pure
  function fed by the caller, so the library still decides nothing about
  the platform (D-5).
- **Threading**: `SettingsFile` is not thread-safe; one thread owns it.
  In Qt6 and the Web that is the thread of the app session. In the Swift
  frontend it stays with the frontend on the main thread (`TISettings`),
  also after M-33 moves the session to its own thread (C59-5); the
  session gets the values it needs when it is built. The settings file
  needs no iCloud file coordination: it lies in Application Support.
  Today's `NSFileCoordinator` calls are for media files
  (`EmulatorViewModel.swift:493-538`) and are not touched.
- **Model names**: plain strings from `ProductDecl::models`. LIB-1's
  descriptor of a product could supply the list and the fallback later;
  this design does not depend on it.

## 4. Product side

### Calc-U-1600 (M-30)

| Today | After M-30 |
|---|---|
| `shared::SettingsStore` (`app/Settings.hpp:15-21`), `QtSettingsStore` (`frontends/qt6/app/AppSettings.hpp:32-43`), `WebSettingsStore` (`frontends/web/bindings.cpp:195-236`), `MemorySettingsStore` (tests) | deleted; one `calcu::settings::SettingsFile` per app |
| `shared::Settings` accessors, key strings `startup/modelPreference`, `preset/default/<model>`, `storage/instanceDirOverride`, … (`app/Settings.hpp:35-126`) | same accessor names where callers use them, over AC-26's keys: `[startup] model`, `last-model` (a model name, today an int, line 120), `[preset] PC-1500`… (today `PC1500`), `[storage] save-folder`, `tape-folder`, `[trace] folder`, `max-file-size-mb`, `[debug] dap-enabled`, `dap-port`, `[serial] link-folder`, `[folders] presets`, `basic-programs`, `machine-code`, `host-drive`, `[window] width`, `height` |
| `Settings::startupModel()` (`app/Settings.cpp:80-90`) | `calcu::settings::startupModel()`, then the name mapped to `shared::Model` |
| `Settings::dapPort()` range (`app/Settings.cpp:75-78`) | declared `Integer{1024, 65535}`, default 32168 (AC-28) |
| `integer()` reading a non-number as 0 (`app/Settings.cpp:45`) | the declared default (EC-5; decision log, AC-11 row) |
| `shared::AppPaths::defaultInstanceDir`, `instanceDir`, `tapeDir`, `traceDir`, `serialLinkDir` (`app/AppPaths.cpp:61-110`) | forward to `calcu::settings::Paths` (save folder, tapes, traces, serial links); names stay until LIB-7 1600 (M-47) renames the media callers |
| `shared::HostDirs{home, bundledResources, bundledExamples}` (`app/AppPaths.hpp:17-23`) | `home` moves to `calcu::settings::HostDirs`; the two bundle folders stay in a product struct for LIB-12/LIB-6 |
| `AppSettings::isolatedStorePath()`, `AppPaths::setIsolatedInstanceDir()` for `--shots` (`frontends/qt6/app/main.cpp:116-122`) | deleted; `--shots` passes `HostDirs` on a `QTemporaryDir` (AC-18) |
| Qt6 `AppPaths::paths()` (`frontends/qt6/app/AppPaths.cpp:40-44`) | builds `HostDirs{Qt6, home, settingsBaseFor(...)}`; on macOS the Application Support folder from `MacAppSupport.mm`, on Windows/Linux from `qEnvironmentVariable` |
| Web: settings seeded from `localStorage` `calcu1600/` and posted back (`src/engine.js:20-39`) | `WebController` opens `SettingsFile` at `<settingsBase>/Calc-U-1600/Calc-U-1600.ini` in `/persist` (IDBFS, autoPersist, `calc-engine-worker.js:144-148`); save folder stays `/persist/Calc-U-1600` (LIB-15 AC-25) |
| Web `settingsState()`, `setStartupModel`, `setDefaultPreset` (`bindings.cpp:546-582`) | stay; they go through `shared::Settings` |

Stays: the dialogs and their rows (LIB-14 changes them later); the
`OpenFolder` enum, mapped to the `[folders]` kinds.

### Calc-U-59 (M-32)

| Today | After M-32 |
|---|---|
| `cu59::settings` constants and `resolve*` (`app/SettingsKeys.hpp`) | `app/Settings.hpp`: `settingsDecl()` with AC-19's keys and typed accessors; `resolveStartupModel` → `calcu::settings::startupModel()` with fallback `TI-59` (AC-20) |
| `solidStateModuleID` stored by `saveModuleID` (Qt6 `EmulatorController.cpp:113-114`, Swift `EmulatorViewModel.swift:557`, Web `bindings.cpp:387`) and read at start | not stored, not read (AC-22, row 59-9); the module at start is the built-in default. If M-25 has already removed the call, nothing is left to do here |
| Qt6 `AppSettings::store()` = `QSettings("Calc-U-59","Calc-U-59")` (`frontends/qt6/app/AppSettings.hpp:16`) and its keys (lines 23-92) | wrappers over `cu59::settings`; `storage/cardDirOverride` → `[storage] save-folder`; `trace/dirOverride` → `[trace] folder`; `preset/…`, `asm/…` → `[folders] presets`, `asm-overlays`; `[folders] other` is declared, no dialog uses it yet (D-1); one key per kind (D-2) |
| `windowGeometry`, `splitterState` as `QByteArray` (`MainWindow.cpp:48-49, 112-113`) | `[window] x`, `y`, `width`, `height` and the three panel widths of the splitter (calculator, printer, debug; `MainWindow.cpp:30-37`) as plain numbers (AC-35); default 1280 × 860 as today (line 48); key names `x`, `y`, `width`, `height`, `calculator-width`, `printer-width`, `debug-width` (D-7) |
| Qt6 `AppPaths::defaultDir/cardDir/traceDir/constantMemoryFile` (`frontends/qt6/app/AppPaths.cpp:19-25`) | forward to `Paths` (`saveFolder`, `traceFolder`, `saveFolderFile("ti58c.mem")`); the file's new name comes with C59-3 |
| Swift `SettingsKey` and `UserDefaults` (`App/AppSettings.swift:39-217`), `@AppStorage` (`Views/SettingsView.swift:14-21`, `CalculatorView.swift:41`, `KeyboardView.swift:44`), `saveLastUsedModel` (`EmulatorViewModel.swift:556`) | an `@Observable` settings model over `TISettings` (the bridge to `SettingsFile`); views bind to it; `ledFontStyle` → `[display] led-style`, `keyboardFeedback` → `[keyboard] feedback`, `traceLocation` → `[trace] location`, `traceCustomPath` → `[trace] folder`, `portraitDebugPage` → `[window] portrait-debug-page` |
| `traceCustomPathBookmark` in `UserDefaults` (`AppSettings.swift:50, 155, 206`) | stays (AC-34); at start the frontend resolves it and passes the folder as `HostDirs::chosenTraceFolder` |
| `AppSettings.traceDirectory()` (`AppSettings.swift:143-186`) | `Paths::traceFolder()`; Application Support passed as `HostDirs::applicationSupport` |
| `CardStorage.warmUp()` resolving iCloud or local Documents (`CardStorage.swift:37-49`) | stays; it runs before `HostDirs` is built, and its folder is `HostDirs::saveFolder` (AC-14, EC-2) |
| `ScreenshotTests.settingsArguments` as `UserDefaults` launch arguments | under `--UITesting` only: `--setting <section>.<key>=<value>` launch arguments applied to a `defaultsOnly` file in memory, never written; the memory rule "screenshot settings are launch values in the tests" holds |
| Web: `localStorage` `calcu59/<key>` (`src/engine.js:20-38`), `startupPlan(settings, …)` (`bindings.cpp:845-851`) | `SettingsFile` on an Emscripten file that adapter A-5 fills from and saves to one `localStorage` entry until M-34; `startupPlan` reads the model from it. From M-34 (LIB-15 59): `/persist-calc-u-59/settings/Calc-U-59/Calc-U-59.ini`, save folder `/persist-calc-u-59/Calc-U-59` (LIB-15 §3.3, D-6) |

Stays: `TISettings` as the bridge class name until the rename steps of
M-33; the `ti58c.mem` `localStorage` key (C59-3, M-34).

## 5. Adapters

| ID | Adapter | File | Wraps | Removed in |
|---|---|---|---|---|
| A-1 | `shared::AppPaths` folder methods (`instanceDir`, `tapeDir`, `traceDir`, `serialLinkDir`) | `products/calc-u-1600/app/AppPaths.hpp` | `calcu::settings::Paths`, under today's names, for the bays, `AppSession`, `MachineSession` | M-47 (LIB-7 1600: media callers use the save folder by its term) |
| A-2 | Qt6 `AppPaths::cardDir()`, `traceDir()`, `constantMemoryFile()` | `products/calc-u-59/frontends/qt6/app/AppPaths.hpp` | `Paths` | M-55 (LIB-7 59) for cards; M-54 (C59-3) for the constant memory file |
| A-3 | Web trace folder fallback: where `Paths::traceFolder()` gives none (Web), the app layer still writes to the save folder (1600, `AppSession.cpp:212`) or MEMFS `/traces` (59, `bindings.cpp:12`) as today | `products/calc-u-1600/app/AppPaths.cpp`; `products/calc-u-59/frontends/web/bindings.cpp` | today's trace places | M-45 (LIB-10 1600), M-53 (LIB-10 59) |
| A-4 | Screenshot folder on the Web: same rule as A-3 for preset screenshots | as A-3 | today's places | M-46 (LIB-6 1600), M-54 (LIB-6 59) |
| A-5 | `WebIniMirror`: the Calc-U-59 page keeps the INI text in one `localStorage` entry, `calcu59/Calc-U-59.ini`, hands it to the worker at `init`, and stores the text the worker posts after each write | `products/calc-u-59/frontends/web/src/engine.js`, `bindings.cpp` | the persistent file system Calc-U-59's Web frontend does not have before LIB-15 59 | M-34 (AC-23) |
| A-6 | `saveModuleID` in `SessionController::Platform` kept as a no-op in the frontends, if M-25 left it | `products/calc-u-59/app/SessionController.hpp:85` and implementers | the module choice no longer stored | M-33 for Qt6 and Swift, M-34 for the Web: `Platform` goes with LIB-3 59 (LIB-3 AC-30: `saveModuleID` → gone; reconciled with LIB-3 design) |

## 6. Tasks

Tests are written first, from the criteria; group `frequent` unless
stated. Case names follow LIB-1 §2a (`acN_`/`ecN_` in the library,
`lib9_acN_` in the products; reconciled). `L` = `library/settings/tests/`, in `calcu_library_tests`.

| Task | What | ACs/ECs | Test (file, case, group) |
|---|---|---|---|
| **M-20** | | | |
| T-1 | `IniDocument` parse and look-up | AC-1, AC-33 | `L/ini_document_tests.cpp`: `ac33_grammar_comments_both_line_endings`, `ac33_value_is_rest_of_line_trimmed`, `ac33_names_case_insensitive`, `ac33_last_duplicate_wins`, `ac33_bom_or_bad_utf8_is_unreadable`; frequent |
| T-2 | Lossless change and write-back | AC-4 | `L/ini_document_tests.cpp`: `ac4_unknown_keys_sections_comments_order_survive_set`, `ac4_unchanged_document_round_trips_bytes` |
| T-3 | Declaration, family keys, value checks | AC-1, AC-3, AC-8, AC-11, EC-5 | `L/settings_file_tests.cpp`: `ac3_missing_key_has_default`, `ec5_bad_value_reads_default_and_stays_in_file`, `ac11_trace_size_missing_zero_negative_is_50`, `ac1_declaration_names_kebab_case` |
| T-4 | `SettingsFile` open, create, version | AC-5, AC-7, EC-6 | `L/settings_file_tests.cpp`: `ac5_first_start_writes_version_and_every_key`, `ac7_no_version_line_starts_with_defaults_and_says_so`, `ec6_unreadable_file_untouched_defaults_logged` |
| T-5 | Read once, write-through, atomic | AC-6, EC-7 | `L/settings_file_tests.cpp`: `ac6_change_survives_reopen`, `ac6_file_not_reread_while_running`, `ec7_interrupted_write_leaves_old_or_new` (a write that fails before the rename, by a test hook in `AtomicFile`) |
| T-6 | Two instances | EC-10 | `L/settings_file_tests.cpp`: `ec10_two_writers_last_wins_unknown_keys_kept` (merge on write, D-3) |
| T-7 | `settingsBaseFor`, `settingsFilePath` | AC-2, EC-4 | `L/paths_tests.cpp`: `ac2_settings_place_macos_ios`, `ac2_settings_place_windows_appdata`, `ac2_settings_place_linux_xdg_and_fallback`, `ac2_settings_place_web`, `ac2_product_name_as_written`, `ec4_two_settings_bases_two_files` |
| T-8 | `Paths` save folder per frontend | AC-12, AC-13, AC-14, AC-15, EC-2 | `L/paths_tests.cpp`: `ac13_qt6_save_folder_home_product_or_override`, `ac14_swift_save_folder_is_passed_folder`, `ec2_swift_save_folder_follows_each_start`, `ac15_web_save_folder_inside_passed_fs` |
| T-9 | Layout and other folders | AC-16, AC-17, EC-3 | `L/paths_tests.cpp`: `ac16_files_at_root_tapes_in_Tapes`, `ac16_tape_override`, `ec3_override_save_folder_keeps_Tapes_inside`, `ac17_qt6_traces_screenshots_links_default_to_save_folder`, `ac17_swift_screenshots_save_folder_traces_by_location`, `ac17_web_has_no_trace_or_screenshot_folder` |
| T-10 | `ensure()` | EC-8, EC-9 | `L/paths_tests.cpp`: `ec8_missing_folder_created_on_first_write`, `ec9_unusable_override_reported_no_fallback` |
| T-11 | Caller's own directories | AC-18 | `L/paths_tests.cpp`: `ac18_own_host_dirs_touch_nothing_else` (a fake home whose files are checked unchanged); `L/settings_file_tests.cpp`: `ac18_defaults_only_reads_and_writes_nothing` |
| T-12 | Startup keys | AC-9, AC-10, EC-1 | `L/startup_tests.cpp`: `ac9_model_last_or_name_else_last`, `ec1_last_model_not_a_model_gives_fallback`, `ac10_preset_per_model_empty_means_none` |
| T-13 | Open folders (library part of AC-29) | AC-29 (L part) | `L/startup_tests.cpp`: `ac29_open_folder_fixed_else_last_used_else_home` (one key per kind, `fixed:` marker, D-2) |
| T-14 | `check_platform_stores.sh` | (tool for AC-21, AC-31, AC-34) | its own self-test on a fixture tree: `LibraryPlatformStoreCheck` |
| **M-30** | | | |
| T-15 | `shared::Settings` declaration and keys | AC-26, AC-29 | `products/calc-u-1600/app/tests/shared_settings_tests.cpp`: `lib9_ac26_declaration`, `lib9_ac29_folders_four_kinds` |
| T-16 | Startup model PC-1500A | AC-27 | same file: `lib9_ac27_startup_last_without_last_model_is_pc1500a` |
| T-17 | DAP port range | AC-28 | same file: `lib9_ac28_dap_port_outside_range_is_32168` |
| T-18 | Window size | AC-30 | same file: `lib9_ac30_window_size_default_900x500_and_kept` |
| T-19 | First start, no platform store | AC-31 | same file: `lib9_ac31_first_start_defaults_and_writes_file`; CTest `CalcU1600NoPlatformSettings` (`check_platform_stores.sh` over `frontends/`; allow-list: `MacAppSupport.mm` `registerDefaults` lines, which are not settings) |
| T-20 | Same keys Qt6 and Web; unused keys kept | AC-32 | same file: `lib9_ac32_web_and_qt6_share_declaration`, `lib9_ac32_web_keeps_and_ignores_dap_serial_window_folders`; Longer: `CalcU1600WebPlaywright` adds a case "settings survive a reload" |
| T-21 | Move callers, delete stores, `--shots` | AC-26, AC-18 (product use) | existing `CoreTests` cases and `CalcU1600Shots.*` (Longer) stay green |
| **M-32** | | | |
| T-22 | `cu59::settings` declaration | AC-19, AC-25 | `products/calc-u-59/tests/shared_settings_tests.cpp`: `lib9_ac19_declaration`, `lib9_ac25_one_declaration_for_every_frontend` |
| T-23 | Startup model TI-59 | AC-20 | same file: `lib9_ac20_startup_last_without_last_model_is_ti59` |
| T-24 | First start; no platform store | AC-21 | same file: `lib9_ac21_first_start_defaults_and_writes_file`; CTest `CalcU59NoPlatformSettings` (allow-list: the bookmark key in `AppSettings.swift`, and until M-34 the A-5 entry and the `ti58c.mem` key in `src/engine.js`) |
| T-25 | No module setting | AC-22 | same file: `lib9_ac22_no_key_holds_a_module` |
| T-26 | Swift trace location | AC-24, AC-34 | same file: `lib9_ac24_trace_location_app_support_icloud_folder` (on `Paths` with `FrontendKind::Swift`); the allow-list of T-24 names the bookmark key only |
| T-27 | Qt6 window numbers | AC-35 | same file: `lib9_ac35_window_numbers_round_trip` (an app-layer `WindowState` the Qt6 code fills); Longer: a Qt6 run checked by hand at the end of C1 |
| T-28 | Web through `SettingsFile` (A-5) | AC-25 (Web), AC-23 prepared | Longer: `CalcU59WebPlaywright` adds "settings survive a reload"; AC-23's own check in M-34 |
| T-29 | Swift model and views, `ScreenshotTests` | AC-19, AC-25 | Longer: `CalcU59UIRegression`; `CalcU59XcodeProjectCheck` stays green |

Coverage: L 19 ACs + 10 ECs, 1600 7 ACs, 59 9 ACs (AC-23 prepared,
passes in M-34): all 45 assigned criteria are in a task.

## 7. Tools and agent knowledge

Today's tools:
- `products/calc-u-1600/tools/wasm_smoke.cpp` uses `MemorySettingsStore`
  and `shared::HostDirs` (lines 15, 73-75): moved onto `SettingsFile` and
  `calcu::settings::HostDirs` in M-30.
- `pc1500_cli`, `pc1600_cli`, the probes and Calc-U-59's
  `tools/headless/`, `tools/agentic/` do not use settings (checked with
  `git grep`): unchanged.

Agent knowledge that changes in the same step:

| Step | File | What |
|---|---|---|
| M-20 | none | (plan: none) |
| M-30 | `products/calc-u-1600/docs/developer/Shared-Architecture.md` lines 9-10, 45-46, 78 | `SettingsStore`, `QSettings`, `localStorage` → the settings file and `calcu::settings::HostDirs` |
| M-30 | `products/calc-u-1600/docs/developer/Web-Client.md` lines 94-95 | settings in `Calc-U-1600.ini` in `/persist` |
| M-30 | `docs/behaviour-changes.md` row 1600-2 | becomes visible (ledger) |
| M-32 | `products/calc-u-59/CLAUDE.md` lines 66, 84-85 | the `SettingsKeys.hpp` rule → `app/Settings.hpp` and the library; `traceDirectory()` → `Paths` |
| M-32 | `products/calc-u-59/.claude/skills/calc-u-59-qt/SKILL.md` line 41 | as above |
| M-32 | `products/calc-u-59/.claude/skills/calc-u-59-uitesting/SKILL.md` line 120 | no `UserDefaults` seeding; `--setting` launch arguments |
| M-32 | `products/calc-u-59/reference/SharedArchitecture.md:73`, `AppArchitecture.md:33, 121`, `FrontendParity.md:34, 83`, `NewGUIGuide.md:57` | `SettingsKeys` → the settings file |
| M-32 | memory `calc-u-59/simulator_upgrade_automation.md` | the launch values are `--setting` arguments |
| M-32 | `docs/behaviour-changes.md` rows 59-2, 59-9 | become visible (ledger) |
| M-34 | `products/calc-u-59/reference/WebClient.md:23-25` | with LIB-15 59 (plan: M-34) |

No glossary term is renamed by this feature; the code names in
`docs/glossary-mapping.md` ("Startup model": `startupModel`,
`startup/modelPreference`) describe the pins and stay; documents that
cite them in parentheses drop the parenthesis in the step that removes
the key.

## 8. Golden runs and risks

Golden runs: no core is touched. `products/calc-u-59/tests/golden-runs/`
and `products/calc-u-1600/tests/golden-runs/` run through the cores and
the CLIs, which read no settings; they must pass unchanged after M-30 and
M-32. They guard the steps only against accidental changes to the app
layer around `MachineSession`/`SessionController`.

Risks:
- **Swift start order**: `CardStorage.warmUp()` runs off the main thread
  (`EmulatorViewModel.swift:207-213`); `HostDirs` must wait for it, or
  the first start uses local Documents. Start-up time may grow by that
  wait.
- **Qt6 window state**: `restoreGeometry` keeps the maximised state and
  the screen; plain numbers do not (AC-35). A maximised window comes back
  at its last normal size.
- **Standalone Qt6 configure** of Calc-U-1600 (`cmake/CalcU.cmake:112`:
  `calcu_library` absent): with the settings code required, that path
  breaks. The root CMake says products do not configure on their own;
  M-30 removes the `if(TARGET calcu_library)` guard.
- **Web 59 adapter A-5**: a page in private mode has no `localStorage`;
  settings then start at their defaults at every load, as today.
- **iOS trace default**: today it is iCloud on iOS
  (`AppSettings.swift:95-100`; `SettingsView.swift:15` even on macOS);
  AC-24 makes it `app-support` (D-8; ledger row to add).

## 9. Decided (best guess)

- **D-1 (was OQ-1; AC-19).** `[folders] other` is declared and written
  empty; no dialog uses it yet (the spec names it; no code has a third
  dialog).
- **D-2 (was OQ-2; AC-29, AC-19).** One key per kind, as the spec's OQ-5
  rules. The value is the folder last used, updated on every pick; a
  fixed folder set in the settings dialog is written as `fixed:<folder>`
  and is not updated by picks. Keeps today's two behaviours in one key.
  Clearing the fixed folder goes back to "last used", starting where the
  fixed folder was (both products' rule, in the library since the C1
  simplification, `fixedOpenFolder`/`setFixedOpenFolder`).
- **D-3 (was OQ-3; EC-10, AC-6).** Merge on write: `set` re-reads the file
  and changes only this instance's keys; the file is not re-read
  otherwise.
- **D-4 (was OQ-4; EC-6, AC-7).** An unreadable, version-less or
  other-version file is not written in that run ("left untouched").
- **D-5 (was OQ-5).** `settingsBaseFor` stays in the library as a pure
  function fed by the caller: AC-2's rule held once, the platform calls
  stay in the frontends.
- **D-6 (was OQ-6).** Closed before: one Web mount per product, Calc-U-1600
  `/persist`, Calc-U-59 `/persist-calc-u-59` (LIB-15 §3.3).
- **D-7 (was OQ-7; AC-30, AC-35).** `[window]` keys as drafted: `width`,
  `height` (both products), `x`, `y`, `calculator-width`, `printer-width`,
  `debug-width` (Calc-U-59 Qt6), `portrait-debug-page` (Swift).
- **D-8 (was OQ-8; AC-24).** The iOS trace default becomes Application
  Support as AC-24 says; `location = folder` on iOS and a folder without a
  usable bookmark keep today's fallback (plain path, then Application
  Support, `AppSettings.swift:174-184`). Needs its own ledger row (not
  covered by 59-2, which is about stored values).
- **D-9 (was OQ-9).** No `docs/formats/settings.md` page; LIB-16's settings
  chapter and this spec describe the file (fewer documents).
- **D-10 (was OQ-10; EC-6).** "Cannot be read as INI" as proposed: a BOM,
  bad UTF-8, or a non-blank, non-comment line that is neither
  `[section]` nor holds `=` (§3).
- **D-11 (was OQ-11; AC-18; LIB-11 D-1).** No harness value in
  `FrontendKind`: the harness passes `FrontendKind::Qt6` with its own
  `HostDirs` (home, settings base and save folder inside the run folder)
  and takes Qt6's folder rules, so traces and screenshots land in the run
  folder.
- Not verified: Qt's `QStandardPaths` values per OS (the design avoids
  them for AC-2); whether M-25 already removes `saveModuleID` (A-6).

## Part E simplification E-1 (2026-11-17, branch `es-assembly`)

`docs/reviews/e-simplification.md` §1 S-1.

- `FrontendKind::Given`: no platform's rule; the save folder, the trace
  folder and the folder for screenshots are the ones `HostDirs` passes
  (`saveFolder`, the new `traceFolder` and `screenshotFolder`; empty: none),
  the tape folder as for every kind, no serial link folder. The harness
  (LIB-11 D-11) and the test rigs use it for "the folders as given"; they
  used `Swift` for it, which made the harness patch the debugger's trace
  folder afterwards. `HostDirs.hpp`'s comment said the harness passes
  `Qt6`; it says `Given` now.
- `shell::SettingsPartSetup::frontend` is gone: it was read nowhere.
- The host's `Paths` is the one input of a product's places to the
  library's parts (`parts::LibraryPartsSetup::paths`): the media's save and
  tape folders, the trace folder (`debugger::traceFolderOf`), the folder for
  screenshots and `~/` (its `home`). Calc-U-59's app layer no longer makes
  a second `Paths` of kind `Swift` over the host's save folder; it takes
  the host's (`TIAppSetup::paths`), as Calc-U-1600's takes `AppPaths`, and
  asks for `LibraryPartsSetup::placesAtStart` (its places fixed at the
  start, as before; LIB-10's E-1 section has the question for the user).
  Its Web engine sets `LibraryPartsSetup::screenshotFolder` to its save
  folder: AC-17 says a screenshot on the Web is a download and no folder
  exists for it, Calc-U-59's Web writes it into its save folder (as
  before; LIB-6's E-1 section has the question for the user).

## Part E simplification E-2 (2026-11-17, branch `es-host-session`)

`docs/reviews/e-simplification.md` §1 S-1, S-4 (survey A S-9).

- `shell::HostSettings` (`shell/HostSession.hpp`): the product's settings
  file opened at `settingsFilePath(decl, dirs)` (or a path the host names:
  the Swift UI tests' file) and the `Paths` over it, for every host that
  opened the file and made its `Paths` itself (both Web engines, the Swift
  bridge, the Qt6 hosts below). A host logs `problem()` as before.
- `qt6::HostFolders` (`library/qt6/HostFolders.hpp`): the Qt6 hosts' folders
  and settings file, which both products' `frontends/qt6/app/AppPaths.cpp`
  wrote: the platform's host folders (home, the settings base of AC-2 with
  Qt's environment), a run's own folders before the first use
  (`useHostDirs`, AC-18), the file opened once at the first use with the
  warning "Settings: <problem>" (EC-6), the places over it, `toFsPath`.
  The macOS settings base stays each product's: Calc-U-1600 passes
  `macApplicationSupportFolder()`, Calc-U-59 nothing
  (`~/Library/Application Support`); no settings file moves. Each
  product's `AppPaths` keeps its bundled-resource folders. Calc-U-59's
  `AppPaths::displayPath` and `platformHostDirs` (both read nowhere) and
  Calc-U-1600's `AppPaths::platformHostDirs` (read only inside) are gone.
- `web::WebStart::placesDirs()`: the Web's host folders with home the save
  folder's parent (`/persist`) when the page gives none, which both Web
  engines wrote.
