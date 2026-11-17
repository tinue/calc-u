# LIB-12 ROM lookup: design
Status: approved by the user at the Part B/C1 boundary (2026-11-14); §10 (Calc-U-59 cue cards, M-55): approved by the user at the Part D/E boundary (2026-11-16)

## 1. Scope

| Column | Step | Tests from (plan §3) | Done in (plan §5.1) |
|---|---|---|---|
| Library | M-24 | AC-1–7; EC-2, EC-3 | M-28 (AC-7's harness clause; D-1) |
| Calc-U-59 | M-24 | AC-8–10, AC-14 (the 14 Solid State ROM modules); EC-2, EC-3; "a build with one ROM file taken away" (Technical Requirements) | M-24 |
| Calc-U-1600 | M-27 | AC-11–13, AC-14 (CE-502B); EC-1 | M-27 |

- M-24 waits for M-19 (C59-1: the TI machines behind the machine
  interface, the Calc-U-59 catalogue with its ROM entries, LIB-1 P1) and
  M-22 (LIB-3 Library). M-27 waits for M-24 and M-26 (C1600-1).
- EC-1 is a Calc-U-1600 case (the CE-1600P's second file). Its library
  half (one file that two entries need) is tested in M-24 as well, with
  test descriptors. The product case is tested in M-27.
- Every AC and EC of this spec is in Part C1. Nothing lands later. Two
  criteria name features built later. AC-5's message is drawn by LIB-14
  and C59-8, and AC-7 names the harness, which LIB-11 builds in M-28.
  By plan §1, AC-7's harness clause passes in M-28 and LIB-12 L becomes
  Done there (D-1); until LIB-14 and C59-8 the frontends draw the message
  as §4 says (D-8).
- The ROM folder of the user, own dumps, a start with some ROMs missing
  and "Check again" are not built (spec, Out of Scope). §3 keeps the one
  place where they would go: `lookUp()` reads only from `Bundle`, and a
  second source would be added there.
- M-24 also has to "correct AC-9's md5 table" (plan M-24, §2.3). That
  was already done in `76e9452` (decision-log row of 2026-11-14), so no
  task is left (§9).

## 2. Modules and files

### Library (M-24): `library/roms/`, namespace `calcu::roms`

The area named in LIB-1 design §2a, part of `calcu_library`. It uses
`machine` and `sessions` (for the topic, §3.5). M-24 also starts
`library/formats/` with the YAML reader (D-4).

| File | Holds |
|---|---|
| `library/roms/Bundle.hpp`, `.cpp` | `Bundle` (the bundled files, read by exact name), `DirectoryBundle` (§3.1) |
| `library/roms/RomReader.hpp` | `ProductRomReader`, the text reader and added checks that a product supplies (§3.2) |
| `library/roms/FullSet.hpp`, `.cpp` | `NeededBy`, `FullSetEntry`, `fullSet()` (§3.3) |
| `library/roms/RomSet.hpp`, `.cpp` | `RomSet`: every file of the full set, read once; images per descriptor (§3.4) |
| `library/roms/Lookup.hpp`, `.cpp` | `FileProblem`, `RomsMissing`, `LookupResult`, `lookUp()`, `describe()` (§3.5) |
| `library/roms/tests/TestRomFiles.hpp` | writes test ROM files into a temporary folder; a test catalogue with binary and text entries, one file shared by two entries, and a test text reader |
| `library/roms/tests/bundle_tests.cpp`, `full_set_tests.cpp`, `lookup_tests.cpp`, `rom_set_tests.cpp` | §6, T2–T6 |
| `library/roms/tests/check_no_own_rom_lookup.sh` | the source check of AC-2, run once per product (§6, T7) |
| `library/formats/Yaml.hpp` | moved from `products/calc-u-1600/core/Yaml.hpp` (D-4), namespace `calcu::formats`: `YamlNode`, `parseYaml`, `requireFormatVersion`; header only, no exceptions, unchanged otherwise. LIB-11 adds `parseValue` and `parseDuration` beside it in M-28 |
| `library/formats/tests/yaml_tests.cpp` | the cases of the YAML reader that Calc-U-1600's `tests/memory_card_tests.cpp` covers today, moved (support, no AC) |

Changed: `library/CMakeLists.txt` (the `library/roms/` sources in
`calcu_library`; the tests in `calcu_library_tests`).
`docs/test-groups.md` gets the new entries. In Calc-U-1600, the users of
`core/Yaml.hpp` (`core/Preset/PresetDebugBlock.hpp`, `PresetFile.hpp`,
`core/Connector/MemoryCardDefinition.hpp`, `FloppyImageFile.hpp`,
`app/PresetCatalog.cpp`, `frontends/qt6/app/screenshots/ShotScenario.*`,
`tests/memory_card_tests.cpp`) include `formats/Yaml.hpp` and name
`calcu::formats`, in M-24.

### Calc-U-59 (M-24)

| File | Change |
|---|---|
| `products/calc-u-59/core/TIRomReader.hpp`, `.cpp` | new: `TIRomReader : calcu::roms::ProductRomReader` (AC-8), in the core's style (global namespace, `TI` prefix; C59-1 §2). It uses the core's Datamath text decoders, which C59-1 moves into `core/TIRomText.hpp/.cpp` in M-19 (reconciled with C59-1 design) |
| `products/calc-u-59/core/ModuleFile.hpp`, `.cpp` | new: reads the ROM of a `.rom.yaml` (`docs/formats/modules.md` §2.1, §2.2: `format`, `format-version: 1`, `module-name`, `rom`), giving the ROM bytes (AC-14), with `calcu::formats::parseYaml`; it checks `format-version` with `requireFormatVersion` (D-4) |
| `products/calc-u-59/app/ProductRoms.hpp`, `.cpp` | new: `cu59::lookUpRoms(const calcu::roms::Bundle&)`, which takes the catalogue, `TIRomReader` and adapter A-2 to `lookUp()` |
| `products/calc-u-59/app/RomLoader.hpp`, `.cpp` | deleted: `ResourceReader`, `directoryReader`, `loadRom`, `loadConstants`, `loadModule`, `parseModuleTxt`, `RomError` |
| `products/calc-u-59/app/MachineModelInfo.hpp` | `romFiles`, `constFiles` removed (LIB-1 design §5, its adapter) |
| `products/calc-u-59/app/SessionController.hpp`, `.cpp`; `app/EmulatorSession.hpp`, `.cpp` | take a `std::shared_ptr<const calcu::roms::RomSet>` in place of `ResourceReader`. The "ROM load error" and "Module error" paths and the "module library was not loaded" diagnostic go (`SessionController.cpp:105-111, 129-130, 220-226`). `Platform::romsMissing` is new (A-4) |
| `products/calc-u-59/app/ModuleCatalog.hpp`, `.cpp` | `moduleRomFile` gives `<ID>.rom.yaml` and no longer the `MODULE-ROM` line of `cuecards.txt` (A-3) |
| `products/calc-u-59/frontends/qt6/app/QtResourceBundle.hpp`, `.cpp` | new: `Bundle` over `:/roms/` (D-2) |
| `products/calc-u-59/frontends/qt6/app/EmulatorController.cpp` | `lookUpRoms` before the `SessionController`. On failure, `QMessageBox::critical` with `describe()`, then exit 1 (D-8) |
| `products/calc-u-59/frontends/qt6/CMakeLists.txt` | the resource glob (`:107-115`) takes `roms/*.txt`, `roms/*.rom.yaml` and `roms/solid-state/cuecards.txt` |
| `products/calc-u-59/frontends/swift/Bridge/TISessionController.h`, `.mm` | init with the bundle's resource path (`DirectoryBundle`), not a `TIResourceReader` block. On failure, nil plus the `describe()` text |
| `products/calc-u-59/frontends/swift/App/ROMLoader.swift`, `App/EmulatorViewModel.swift` | `bundleReader` is used only for `cuecards.txt` (until M-55). The view model holds a "ROMs missing" text, and the window shows it in place of the calculator (D-8) |
| `products/calc-u-59/frontends/swift/Calc-U-59.xcodeproj/project.pbxproj` | resources: the 10 calculator files from `roms/`, the 14 `.rom.yaml` in place of `roms/solid-state/TMC05xx.txt`; checked by `check_xcode_project.sh` |
| `products/calc-u-59/frontends/web/bindings.cpp` | `startupPlan` (`:845-866`) no longer lists files. A new export `romFileNames()` gives the full set. `WebController` builds on `DirectoryBundle("/roms")` |
| `products/calc-u-59/frontends/web/calc-engine-worker.js` | `init` (`:124-139`) fetches every name of `romFileNames()` into the Emscripten file system under `/roms` (as Calc-U-1600 does for `/bundle`). It posts `romsMissing` with the text, or `error` for a failed fetch (EC-3) |
| `products/calc-u-59/frontends/web/build.sh` | copies the 24 files and `cuecards.txt` (`:39-41`) |
| `products/calc-u-59/tools/headless/capi.cpp`, `ti59.py` | `DirectoryBundle(romsDir)` + `lookUpRoms`. Init fails with `describe()` (AC-7) |
| `products/calc-u-59/tools/disasm.py` | reads `roms/` in place of `roms/calculator/` (D-7) |
| `products/calc-u-59/roms/fetch_roms.sh`, `.gitignore`, `README.md` | 24 files, all directly in `roms/` (no `calculator/`). The 14 `solid-state/TMC05xx.txt` lines go, and `solid-state/cuecards.txt` stays (until M-55) |
| `products/calc-u-59/tests/TestRoms.hpp` | `reader()` gives way to `romSet()` (`DirectoryBundle("roms")` + `lookUpRoms`). `makeMachine` builds from it with the ML module from `ML.rom.yaml` |
| `products/calc-u-59/tests/rom_lookup_tests.cpp` | new: §6, P1–P4 |
| `products/calc-u-59/tests/rom_missing_build.sh` | new, Longer: the build with one ROM file taken away (§6, P6) |
| `products/calc-u-59/tests/headless_rom_tests.py` | new: `ti59.py` with a missing file (§6, P5) |

### Calc-U-1600 (M-27)

| File | Change |
|---|---|
| `products/calc-u-1600/core/Resources/BundledRomCatalog.hpp` | deleted. The core reads no file (I-2) |
| `products/calc-u-1600/core/SharpRomReader.hpp`, `.cpp` | new: `ProductRomReader` for the one text entry, `CE-502B.module.yaml`, read by the module definition reader of `core/Connector/` |
| `products/calc-u-1600/app/ProductRoms.hpp`, `.cpp` | new: `shared::lookUpRoms(const calcu::roms::Bundle&)` |
| `products/calc-u-1600/app/MachineSession.hpp`, `.cpp` | builds and attaches from the `RomSet`. These go: `romMissing` (`:77-79`), `bundledRomDirs`, `makePC1600WithRomFallback` (`:146-158`), the CE-1600P fallback (`:706-722`), `Notice::OldPC1600RomUnavailable`, `OldCE1600PRomUnavailable` |
| `products/calc-u-1600/app/AppSession.cpp` | `romDirs` (`:208-209`) gives way to the session's `RomSet` |
| `products/calc-u-1600/core/PC1500/PC1500PresetLoader.*`, `core/PC1600/PC1600PresetLoader.*`, `core/Preset/PresetInterface.hpp` | `romDirs` parameters give way to `const calcu::roms::RomSet&` |
| `products/calc-u-1600/app/ModuleBay.cpp` | the CE-502B definition comes from the `RomSet`, not from scanning the bundled folder |
| `products/calc-u-1600/frontends/qt6/app/MachineController.cpp` | `romMissing` (`:37-48`, which names a stale `tools/fetch_roms.sh`) shows `describe()`, then exits as today |
| `products/calc-u-1600/frontends/qt6/CMakeLists.txt` | bundles `roms/CE-502B.module.yaml` with the ROMs (`:183-198`) |
| `products/calc-u-1600/frontends/web/build.sh`, `bindings.cpp` | CE-502B copied from `roms/`. `DirectoryBundle("/bundle")` |
| `products/calc-u-1600/tools/pc1500_cli.cpp`, `pc1600_cli.cpp`, `pc1600_*_probe.cpp`, `wasm_smoke.cpp`, `dev/loader-matrix/pc1600_loader_matrix.cpp` | `DirectoryBundle` of the folder they use today (`roms`, relative to the working directory) + `lookUpRoms` (LIB-11 AC-20). The bare `<rom-file>` mode of `pc1500_cli` stays (D-7) |
| `products/calc-u-1600/roms/fetch_roms.sh`, `.gitignore` | the copy into `frontends/qt6/resources/cards/` (`:146-149`) goes, and so does its ignore line (`.gitignore:84`) |
| `products/calc-u-1600/tests/TestRoms.hpp`, `golden_runs_tests.cpp`, the 9 test files that call `BundledRoms::` | through `romSet()` |
| `products/calc-u-1600/tests/rom_lookup_tests.cpp` | new: §6, Q1–Q4 |
| `products/calc-u-1600/tests/rom_missing_build.sh` | new, Longer (§6, Q5) |

## 3. Seams and interfaces

`RomEntry`, `RomImage`, `Catalogue`, `MachineDescriptor` and
`PeripheralDescriptor` are LIB-1's (design §3.3, §3.8). `RomEntry` has
the form of AC-1: `fileName`, `encoding` (`Binary`/`Text`),
`decodedAmount`, `unit` (`Bytes`/`Words`/`Rows`), `md5`. This design adds
no field.

**Answer to LIB-1 OQ-4 (now LIB-1 D-4).** The machine gets the file as bundled, raw
bytes in `RomImage::bytes`. Reasons:
- The unit of the lookup is the file (`roms.md` R-1).
- `roms.md` says that interpreting ROM content is the core's job, and R-4
  says the core's reader decodes a text file. So the lookup calls the
  product's reader to check the amount (AC-3), and the core calls the
  same reader again when it builds.
- One image type serves both products. The 24 TI text files are decoded
  twice at the start, which costs little.

### 3.1 Bundle: AC-2, EC-3

```cpp
class Bundle {                                     // the product's bundled files; knows no ROM name
public:
  virtual ~Bundle() = default;
  virtual std::optional<std::vector<std::uint8_t>> read(std::string_view fileName) const = 0;  // exactly that name
  virtual std::string where() const = 0;           // for the message: the folder or "the app bundle"
};
class DirectoryBundle final : public Bundle {      // one folder, no subfolders
public:
  explicit DirectoryBundle(std::filesystem::path folder);
  // read(): lists the folder and compares names byte for byte, so on a case-insensitive
  // file system "ce-150.rom" is not found for "CE-150.ROM" (AC-2 "exactly that name")
};
```

- Only `lookUp()` calls `read()`. Nothing else in a product gets a
  `Bundle`, apart from the frontend that makes it.
- A frontend gives a `DirectoryBundle` wherever the files are a folder:
  - the Swift resource path;
  - Calc-U-1600's `bundledResourcesDir()`;
  - `/roms` (Calc-U-59 Web) and `/bundle` (Calc-U-1600 Web) in the
    Emscripten file system;
  - `roms/` for tests and tools.

  Calc-U-59's Qt6 frontend bundles them in a `.qrc` and has
  `QtResourceBundle` (D-2).
- EC-3: a file the Web worker cannot fetch ends the worker's `init`
  before the engine starts (LIB-15 AC-3). The lookup never sees it.

### 3.2 Product reader: AC-1, AC-3, AC-8, EC-2

```cpp
class RomSet;
struct FileProblem;
class ProductRomReader {
public:
  virtual ~ProductRomReader() = default;
  struct Decoded { std::uint64_t amount = 0; std::string error; };          // error set: the reader cannot read it
  virtual Decoded decode(const RomEntry&, std::span<const std::uint8_t> bytes) const = 0;  // Text entries only
  virtual std::vector<FileProblem> checkSet(const RomSet&) const { return {}; }            // the product's added checks
};
```

- A `Binary` entry is checked by `bytes.size() == decodedAmount` with
  unit `Bytes`. No reader is called (AC-3, AC-11).
- A `Text` entry is checked by `decode().amount == decodedAmount`. A
  `Text` entry with no reader is a problem of that file, not a crash.
- The bytes reach the reader unchanged. Line endings are the reader's
  business (EC-2). Today's `splitLines` takes `\n` and `\r`
  (`app/TextUtil.hpp:19-27`).
- `checkSet()` runs only when every file passed its own check.

### 3.3 Full set: AC-4, EC-1

```cpp
struct NeededBy { std::variant<machine::ModelId, machine::PeripheralId> who; std::optional<machine::VariantId> variant; };
struct FullSetEntry { machine::RomEntry entry; std::vector<NeededBy> neededBy; };
struct FullSet { std::vector<FullSetEntry> files; std::vector<std::string> catalogueErrors; };
FullSet fullSet(const machine::Catalogue&, std::span<const FullSetEntry> extra = {});  // extra: adapter A-2 only
```

- The full set contains every `RomEntry` of every model, every variant
  of a model, every peripheral and every variant of a peripheral, in
  catalogue order, with no file twice (EC-1).
- Two entries with the same `fileName` and a different encoding,
  amount, unit or md5 give a catalogue error. `lookUp()` then fails like
  a missing file, naming that file.

### 3.4 ROM set: AC-6

```cpp
class RomSet {                                     // made only by lookUp(); immutable; shared by the app session
public:
  const std::vector<std::uint8_t>* bytes(std::string_view fileName) const;   // nullptr: not in the set
  std::vector<machine::RomImage> imagesFor(const machine::MachineDescriptor&, const std::optional<machine::VariantId>&) const;
  std::vector<machine::RomImage> imagesFor(const machine::PeripheralDescriptor&, const std::optional<machine::VariantId>&) const;
  std::vector<std::string> fileNames() const;      // the full set, for the Web worker and the tests
};
```

- Every file is read once, at the start, and held in memory: about
  0.4 MB for Calc-U-1600, which has 21 files of up to 16 KiB plus the
  CE-502B. No file is read while the product runs. So AC-6 holds by
  construction: `imagesFor()` can never miss a file of the full set, and
  `machine::build()` never reports `missingRoms` for one (LIB-1 EC-1).
- Ownership: `std::shared_ptr<const RomSet>`, held by the app session
  and handed to whatever builds or attaches. It is read-only, so it
  needs no lock (ADR 0009).

### 3.5 Lookup and the message: AC-2–AC-7

```cpp
enum class Problem { Missing, WrongAmount, Unreadable, ProductCheck, Catalogue };
struct FileProblem { std::string fileName; Problem problem; std::string detail; std::vector<NeededBy> neededBy; };
struct RomsMissing { std::vector<FileProblem> files; std::string where; };       // one row per file (EC-1)
struct LookupResult { std::shared_ptr<const RomSet> set; RomsMissing missing;    // exactly one of the two
                      bool ok() const { return set != nullptr; } };
LookupResult lookUp(const FullSet&, const Bundle&, const ProductRomReader*);
std::string describe(const RomsMissing&);        // names every file and its problem, says the full set is needed,
                                                 // and how to get it in a local build (fetch_roms.sh)
```

- `lookUp()` checks every file and does not stop at the first problem,
  so the message names all of them (AC-5). It never checks the md5
  (AC-3). The md5 of the entry is what `fetch_roms.sh` checks, and a
  product test keeps the two equal (P2, Q1).
- Without a `RomSet` nothing can be built: `imagesFor()` exists only on
  a set. The start code builds no machine and shows `RomsMissing`
  (AC-5).
- `describe()` gives the text that the Qt6 dialogs, the Swift window,
  the Web page, `tools/headless` and later `calcu` (LIB-11 EC-1) show.
  The drawn form is LIB-14's and C59-8's.
- LIB-3: `RomsMissing` is the content of the message that LIB-3 AC-38
  maps from `romMissing`. It is sent as a `StateTopic` named
  `roms-missing` with a `RomsMissing` payload (LIB-3 design §3.3,
  `stateTopic(TopicName)`), in place of any machine topic (§9,
  assumptions). Until the products' sessions are on LIB-3 (LIB-3 59 in
  M-33/M-34, LIB-3 1600 in M-31), adapters A-4 and A-5 carry it.
- No exceptions, threads or locks (LIB-1 design §2a). File reads are
  synchronous on the caller's thread, once, at the start.

## 4. Product side

### Calc-U-59 (M-24)

The spec cites `Shared/` of 2.3.0. Here that is `products/calc-u-59/app/`
(`docs/glossary-mapping.md`, "App layer").

| Today | After M-24 |
|---|---|
| Each frontend hands a `ResourceReader` (Qt6 `EmulatorController.cpp:34-36`, Swift `ROMLoader.swift:17-22` through `TISessionController.mm:80-86`, Web `bindings.cpp:120`, headless `capi.cpp:41-42`, tests `TestRoms.hpp:22`). `SessionController` reads files on every model switch and module change | The frontend makes a `Bundle` and calls `cu59::lookUpRoms` once. `SessionController` gets the `RomSet` and reads no file |
| `loadRom`, `loadConstants` (`app/RomLoader.cpp:116-150`): 6144 words, 64 rows, TI-59 words 0 and 6143 | `TIRomReader::decode` per entry, measured with today's parser on 2026-11-14: `TMC0582.txt`, `TMC0583.txt`, `CD2400.txt`, `CD2401.txt` 2560 words; `TMC0571B.txt`, `TMC0573.txt` 1024 words; each `*-CONST-K.txt` 32 rows. `checkSet`: per model the instruction files add up to 6144 words and the constant files to 64 rows; the first word of `TMC0582.txt` is `0A01` and the last of `TMC0571B.txt` is `1987` (AC-8, spec OQ-1) |
| Module ROM from `roms/solid-state/TMC05xx.txt` through `cuecards.txt` `MODULE-ROM` (`parseModuleTxt`, up to the highest address, 5000 bytes for all 14 today) | from `<ID>.rom.yaml` through `core/ModuleFile`: `rom.size` bytes, 5000 for all 14 (AC-14; the entry states 5000, D-3). Module list, names and cue cards still come from `cuecards.txt` (plan §2.3) |
| A bad machine ROM: an alert, and the previous machine keeps running. A module that fails: a line in the log (`SessionController.cpp:105-111, 129-130, 220-226`) | not reachable while running (AC-6). At the start: the frontend shows `describe()`, and no `SessionController` is made (AC-10) |
| Web fetches the model's files and the module's file at init, the rest when needed (`startupPlan`, `bindings.cpp:845-866`) | the worker fetches the full set at init (AC-4; D-10) |
| Catalogue ROM entries for the three models (from M-19) | unchanged. The 14 module entries come from A-2 until M-25 |

Deleted: `app/RomLoader.*`, `ModelInfo::romFiles`/`constFiles`, the
14 module dump lines of `fetch_roms.sh`, the `TIResourceReader` type in
`Bridge/TIFileFormats.h:41` (if nothing else uses it). Kept until
M-55: `roms/solid-state/cuecards.txt`, which is not a ROM file and not
looked up.

### Calc-U-1600 (M-27)

| Today | After M-27 |
|---|---|
| The core finds and reads ROM files itself (`core/Resources/BundledRomCatalog.hpp`, from `MachineSession`, `AppSession` and the preset loaders with `romDirs`) | `shared::lookUpRoms` at the start of the app session. The core gets `RomImage`s through `BuildRequest`/`PeripheralRequest` (C1600-1, LIB-2) and a `const RomSet&` in the preset loaders |
| Missing PC-1500 or New PC-1600 file: `romMissing` ends the Qt6 frontend, also in the middle of a session. Missing Old set (PC-1600, CE-1600P): replaced by New, with a notice (`MachineSession.cpp:146-158, 706-722`) | checked once at the start; no substitution (AC-13). The golden run `pc1600-rom-old` keeps the Old set in use |
| CLI tools: `{"roms"}` relative to the working directory (`pc1600_cli.cpp:110-141`) | `DirectoryBundle("roms")` + `lookUpRoms` (LIB-11 AC-20). The folder is still the tool's argument, but the looking is the library's |
| CE-502B: `frontends/qt6/resources/cards/ce502b.card.yaml`, copied there by `fetch_roms.sh:146-149`, found by the module scan | `roms/CE-502B.module.yaml`, a `Text` entry of the full set, 16384 bytes (`capacity: 0x4000`, not verified as the reader's yield, §9). Added to the catalogue from the `RomSet` (`Catalogue::addPeripheral`, LIB-1 §3.4) |
| Host-drive ROM `PC1600-P1-B7-HOSTDRIVE.bin`, read by `BundledRoms::attachHostDrive` | not in the full set (`roms.md` Q-1). `app/MachineSession` reads it from the bundle folder as today: the one named exception to I-2, which Q-1 implies; `check_no_own_rom_lookup.sh` lists it (D-6) |

## 5. Adapters

| Adapter | File | Wraps | Removed in |
|---|---|---|---|
| A-1 (LIB-1 design §5) ROM file names in the model info | `products/calc-u-59/app/MachineModelInfo.hpp` | 2.3.0's ROM loading | M-24 (this step) |
| A-2 the 14 module `RomEntry`s outside the catalogue | `products/calc-u-59/app/ProductRoms.cpp` (`moduleRomEntries()`, passed as `extra` to `fullSet`) | the module peripheral descriptors that C59-6 adds | M-25 (C59-6, LIB-1 P3) |
| A-3 module ID to `<ID>.rom.yaml` | `products/calc-u-59/app/ModuleCatalog.cpp` (`moduleRomFile`) | `cuecards.txt`'s `MODULE-ROM`, still the module list | M-25 (C59-6 AC-8: names from the module files) |
| A-4 `Platform::romsMissing(const calcu::roms::RomsMissing&)` | `products/calc-u-59/app/SessionController.hpp` and the three frontends | the "ROMs missing" topic of the app session (LIB-3 AC-38) | M-33/M-34 (LIB-3 59); the drawing moves with LIB-14 59 (M-57) and C59-8 (M-58) |
| A-5 `MachineSession::Listener::romMissing` carrying `RomsMissing` | `products/calc-u-1600/app/MachineSession.hpp:72-76`, `frontends/qt6/app/MachineController.cpp` | the same topic | M-31 (LIB-3 1600) |

No adapter is left once M-34 is done.

## 6. Tasks

Tests come first and are Frequent unless a row says otherwise. Library
cases are in `library/roms/tests/`, in `calcu_library_tests`. Case
names follow LIB-1 design §2a.

**Library, M-24**

| Task | What | ACs/ECs | Test (file, case, group) |
|---|---|---|---|
| T1 | CMake: `library/roms/` and `library/formats/Yaml.hpp` in `calcu_library`, the tests in `calcu_library_tests`; Calc-U-1600's includes of the YAML reader; `docs/test-groups.md` | (support) | builds in the CI `library` job (LIB-1 T1); `yaml_tests.cpp`; Calc-U-1600's Frequent suite |
| T2 | `RomEntry` as the lookup reads it | AC-1 | `full_set_tests.cpp`: `ac1_entry_states_bundle_name_encoding_amount_unit_md5` |
| T3 | `Bundle`, `DirectoryBundle` | AC-2 | `bundle_tests.cpp`: `ac2_file_of_exactly_that_name_is_read`, `ac2_name_differing_in_case_is_missing`, `ac2_subfolders_are_not_searched` |
| T4 | `fullSet()` | AC-4, EC-1 (library half) | `full_set_tests.cpp`: `ac4_full_set_has_every_model_variant_and_peripheral_file`, `ec1_file_of_two_entries_is_in_the_set_once_with_both`, `fullset_conflicting_entries_for_one_name_are_a_catalogue_error` |
| T5 | `lookUp()` checks | AC-3 | `lookup_tests.cpp`: `ac3_binary_checked_by_byte_size`, `ac3_text_checked_by_reader_amount`, `ac3_product_set_checks_run_after_file_checks`, `ac3_md5_is_not_checked_at_start` (right amount, other bytes: passes) |
| T6 | `RomsMissing`, `describe()`, `RomSet` | AC-5, AC-6, AC-7 | `lookup_tests.cpp`: `ac5_every_missing_or_bad_file_named_and_no_set`, `ac5_message_says_full_set_needed_and_offers_nothing_else`; `rom_set_tests.cpp`: `ac6_images_for_every_model_variant_peripheral`, `ac6_build_from_set_never_misses_a_rom` (LIB-1 `build()` with `TestMachines`), `ac6_no_file_read_after_lookup` (file deleted after `lookUp`, build still works); `lookup_tests.cpp`: `ac7_harness_text_names_the_files` (the text; the harness clause passes with LIB-11 EC-1 in M-28, D-1) |
| T7 | AC-2's "no other part of a product looks for a ROM file" | AC-2 | `check_no_own_rom_lookup.sh <product>`: fails if product sources outside `app/ProductRoms.*` and the catalogue data open a file by a ROM name or extension, or use `directoryReader`, `ResourceReader`, `BundledRoms`; the host-drive ROM read is the one listed exception (D-6). CTest `CalcU59NoOwnRomLookup` (M-24), `CalcU1600NoOwnRomLookup` (M-27) |

**Calc-U-59, M-24** (`products/calc-u-59/tests/rom_lookup_tests.cpp`, in `CalcU59Tests`)

| Task | What | ACs/ECs | Test (case, group) |
|---|---|---|---|
| P1 | `TIRomReader` (and the move of the decoders if M-19 left them) | AC-8, EC-2, AC-1 | `lib12_ac1_ti_entries_have_the_form`, `lib12_ac8_amounts_per_file_and_per_model`, `lib12_ac8_ti59_first_and_last_word`, `lib12_ac8_wrong_sentinel_is_bad`, `lib12_ac8_module_without_data_is_bad`, `lib12_ec2_crlf_and_cr_dumps_check_alike` (the fetched file with its line endings changed in a temporary copy) |
| P2 | Full set and fetch list | AC-9, AC-4 | `lib12_ac9_full_set_is_the_24_files_with_their_md5` (the spec's table copied into the test), `lib12_ac9_fetch_script_has_the_same_names_and_md5` (parses `roms/fetch_roms.sh`), `lib12_ac4_every_model_and_module_needs_only_set_files` |
| P3 | `ModuleFile`, module bytes from `.rom.yaml`, A-2, A-3 | AC-14 (TI) | `lib12_ac14_module_rom_from_rom_yaml` (5000 bytes, `format-version: 1`, `module-name` = file name), `lib12_ac14_module_images_equal_the_2_3_0_dumps` (md5 of each decoded 5000-byte image, computed in this step from TI-59-ROM `dumps/modules/` at `90cd4a7` before those lines leave the fetch list), `lib12_ac14_unknown_format_version_is_bad` |
| P4 | Start without a `SessionController`; A-4; the old error paths removed | AC-5, AC-6, AC-10 | `lib12_ac10_missing_file_no_machine_and_files_named` (FakePlatform, `DirectoryBundle` of a temporary copy of `roms/` without `TMC0583.txt`), `lib12_ac10_bad_file_no_machine`, `lib12_ac6_model_and_module_changes_read_no_file` (switch through all models and the 14 modules with the bundle folder removed after the start) |
| P5 | Frontends and tools onto the lookup | AC-10, EC-3, AC-7 | Web: `CalcU59WebPlaywright` (Longer) gets `rom-missing.spec.js`: a truncated ROM response shows the text and no calculator (AC-10), and an aborted ROM request is a failed start (EC-3). Swift: `UIRegressionTests` (Longer) `testRomMissingShowsMessage`, with a DEBUG-only launch environment variable that leaves one file out of the bundle (D-8). Qt6: checked by hand, recorded in `qa-report.md` (D-8). Headless: `tests/headless_rom_tests.py` through `ti59.py`, a missing file gives an error naming it (Frequent, in the `CalcU59TraceReader` Python entry, or its own entry) |
| P6 | The build with one ROM file taken away | Technical Requirements, AC-5 | `tests/rom_missing_build.sh` (CTest `CalcU59RomMissingBuild`, Longer). It copies `roms/` to a temporary folder without one file, configures with `-DCALCU59_ROMS_DIR=<copy>`, and builds the headless library and the Web `dist/`. The build succeeds, the headless start fails naming the file, and Playwright on that `dist/` shows the message (D-5) |
| P7 | Fetch list, `.gitignore`, `README.md`, Xcode resources, Qt6 glob, `build.sh` | AC-9, AC-14 | `CalcU59XcodeProjectCheck` (Frequent, extended: no `solid-state/TMC05` resource, 14 `.rom.yaml`); golden runs (§8) |

**Calc-U-1600, M-27** (`products/calc-u-1600/tests/rom_lookup_tests.cpp`)

| Task | What | ACs/ECs | Test (case, group) |
|---|---|---|---|
| Q1 | Catalogue entries and full set; `SharpRomReader` | AC-11, AC-12, EC-1 | `lib12_ac11_binary_dumps_checked_by_byte_size` (a 16383-byte copy is bad), `lib12_ac12_full_set_is_the_21_files_and_ce502b`, `lib12_ac12_fetch_script_has_the_same_names_and_md5`, `lib12_ec1_ce1600p_b5_file_once_in_set_and_message` |
| Q2 | `BundledRomCatalog` removed; sessions, preset loaders and tools on the `RomSet`; no substitution; A-5 | AC-13 | `lib12_ac13_missing_old_set_does_not_start` (no `OldPC1600RomUnavailable` notice, file named), `lib12_ac13_missing_ce1600p_old_file_does_not_start`, `lib12_ac13_no_rom_read_during_session` (model and ROM changes after the bundle folder is removed); `CalcU1600NoOwnRomLookup` (T7); the CLI: `tests/cli_rom_tests.sh`, `pc1600_cli` with a bundle missing one file exits 1 naming it (Frequent) |
| Q3 | CE-502B from the ROM repository | AC-14 (CE-502B) | `lib12_ac14_ce502b_definition_from_the_set` (16384 bytes, `format-version: 1`, `module-name: CE-502B`), `lib12_ac14_ce502b_not_in_qt6_resources` |
| Q4 | Frontends | AC-13 | Web: `CalcU1600WebPlaywright` (Longer) `rom-missing.spec.js` as in P5. Qt6: checked by hand, in `qa-report.md` |
| Q5 | The build with one ROM file taken away | Technical Requirements | `tests/rom_missing_build.sh` (CTest `CalcU1600RomMissingBuild`, Longer), as in P6 |

Coverage: 14 ACs and 3 ECs are assigned to C1, and all 17 are in a
task: AC-1 T2, P1; AC-2 T3, T7; AC-3 T5; AC-4 T4, P2; AC-5 T6, P4, P6;
AC-6 T6, P4; AC-7 T6, P5; AC-8 P1; AC-9 P2, P7; AC-10 P4, P5; AC-11 Q1;
AC-12 Q1; AC-13 Q2, Q4; AC-14 P3, Q3; EC-1 T4, Q1; EC-2 P1; EC-3 P5.
The Technical Requirements' build with one file taken away is in P6
and Q5.

## 7. Tools and agent knowledge

- Tools moved in M-24: `tools/headless/` (`capi.cpp`, `ti59.py`) onto
  the lookup (plan M-24). `tools/disasm.py` reads the new flat `roms/`
  (D-7).
- Tools moved in M-27: `pc1500_cli`, `pc1600_cli`, the three probes,
  `wasm_smoke`, `dev/loader-matrix` (LIB-11 AC-20 for the CLIs).
  `tools/check_presets.sh` runs `pc1600_cli`, so it needs no change of
  its own. Both CLIs stay usable until M-51.
- No rename of a glossary term. The new code names (ROM lookup:
  `calcu::roms::lookUp`; full set; ROM set: `RomSet`) get rows in
  `docs/glossary-mapping.md` "Code names" in M-24. The Calc-U-59 row for
  model loses `ModelInfo`'s ROM file names.
- Knowledge changed in M-24:
  - `products/calc-u-59/roms/README.md`: the flat layout, 24 files.
  - `products/calc-u-59/.claude/skills/calc-u-59-shared/SKILL.md`,
    `products/calc-u-59/.claude/skills/calc-u-59-core/SKILL.md`: ROM
    loading, the reader in the core.
  - `products/calc-u-59/.claude/skills/calc-u-59-web/SKILL.md`: all ROM
    files fetched at init.
  - `products/calc-u-59/CLAUDE.md`.
  - `products/calc-u-59/reference/SharedArchitecture.md`,
    `AppArchitecture.md`, `CoreArchitecture.md`, `NewGUIGuide.md`:
    `RomLoader`, `ResourceReader`, `roms/calculator/`.
  - `products/calc-u-59/memory/`: no file names `RomLoader` (checked
    2026-11-14).
- Knowledge changed in M-27:
  - `products/calc-u-1600/docs/developer/Building.md` (ROMs, plan M-27).
  - `docs/developer/README.md`.
  - `roms/README.md`: the CE-502B row no longer names the Qt6 resources.
  - `CLAUDE.md`, if it names `BundledRomCatalog` (grep in the step).
- Root `CLAUDE.md`: nothing. The library layout line is LIB-1's.
- M-24 moves `core/Yaml.hpp` out of Calc-U-1600: `products/calc-u-1600/CLAUDE.md`
  and the `docs/developer/` pages that name it change in the same step
  (grep in the step).

## 8. Golden runs and risks

- **Golden runs.** M-24 changes where every TI machine gets its ROM, so
  all 17 runs of `products/calc-u-59/tests/golden-runs/` guard it:
  - `ti59-module-ml`, `ti59-module-le`: module bytes from `.rom.yaml`
    instead of the dumps;
  - `ti58c-calculation`, `ti58c-constant-memory`: the TI-58C set;
  - `ti58-no-module`, `ti59-no-module`.

  M-27 is guarded by all 20 runs of
  `products/calc-u-1600/tests/golden-runs/`:
  - `pc1500-rom-a01`: a variant file;
  - `pc1600-rom-old`: the Old set, with no substitution;
  - `pc1600-ce1600p`: the shared B5 file;
  - `pc1600-ce150`, `pc1600-ce158`, `pc1500a-ce150`, `pc1500a-ce158`.

  A difference is a finding, never a new reference.
- **No golden run has the CE-502B.** Q3 checks its definition only.
  Running from it is LIB-2 1600 and C1600-4's.
- **Case-insensitive file systems** (macOS, Windows). A stale file in
  other letter case would be found by `open()`. `DirectoryBundle`
  compares listed names (T3), as `fetch_roms.sh` already guards against.
- **Order with M-19 and M-26.** This design assumes M-19 has the
  Calc-U-59 catalogue with ROM entries and M-26 builds Sharp machines
  through `BuildRequest`. If M-26 still calls `BundledRoms::` (its
  adapters), M-27 removes those calls, and the step is larger.
- **Web start.** Calc-U-59 now fetches about 24 text files at init
  instead of 6–7. They are small, but the offline service worker
  (`app/sw.js`) must cache them all (D-10).
- **Xcode project.** `project.pbxproj` is edited by hand for 14 resource
  entries. `check_xcode_project.sh` and the XCUITest run catch a
  missing one.
- **YAML reader move.** Calc-U-1600's memory card, floppy and preset
  readers use it; its Frequent suite and golden runs guard the move in
  M-24, which changes only the include and the namespace.

## 9. Decided (best guess)

- **D-1 (was OQ-1; AC-7).** `describe()` is tested in M-24 (T6, P5); the
  harness clause of AC-7 passes in M-28 with LIB-11 EC-1, and LIB-12 L is
  Done there (plan §1). Plan §5.1 still shows M-24 and follows this.
- **D-2 (was OQ-2; AC-2).** A frontend hands the lookup a `Bundle` of its
  bundled files (folder, Qt resources, Emscripten file system); today's
  packaging stays.
- **D-3 (was OQ-3; AC-8, AC-14).** A module entry states 5000 bytes and is
  checked exactly; "at least one row" then always holds.
- **D-4 (was OQ-4 = LIB-11 OQ-3).** Calc-U-1600's `core/Yaml.hpp` moves
  into `library/formats/` (`calcu::formats`) in M-24, the first step that
  needs it (`.rom.yaml`); LIB-11 adds `parseValue` and `parseDuration`
  there in M-28. The product's reader (`ModuleFile`) checks
  `format-version` with `requireFormatVersion`.
- **D-5 (was OQ-5).** The build without one ROM file succeeds and the
  built product fails at its start naming the file (AC-5's reachable
  state); P6, Q5.
- **D-6 (was OQ-6).** The host-drive ROM stays a bundled file that
  Calc-U-1600 reads outside the lookup, as today (`roms.md` Q-1); the one
  listed exception in T7's check.
- **D-7 (was OQ-7).** Developer tools (`pc1500_cli <rom-file>`,
  `disasm.py`) are not part of a product and keep reading files.
- **D-8 (was OQ-8).** Until LIB-14 and C59-8: Qt6 a dialog then exit 1,
  Swift the text in place of the calculator, Web the text in the page; the
  Swift test uses a DEBUG-only launch variable; Qt6 is checked by hand.
  No ledger row: a release cannot reach this state (AC-5).
- **D-9 (was OQ-9).** C59-6 takes over adapters A-2 and A-3 in M-25 (C59-6
  §3.6, its D-3).
- **D-10 (was OQ-10; AC-4).** The Calc-U-59 Web worker fetches all 24
  files at init and the service worker caches them; a ledger row is
  added (Calc-U-59).
- **HANDOVER note on AC-9.** The md5 table of AC-9 already lists the
  14 `.rom.yaml` files, with the values `fetch_roms.sh` checks (spec
  decision log 2026-11-14, `76e9452`, plan question 9). It does not
  affect the tasks. P2 tests the table as it stands, and M-24's "AC-9's
  md5 table is corrected" has nothing left to do. The open line in
  `HANDOVER.md` is stale.
- **Not verified.**
  - That the CE-502B module definition reader yields 16384 bytes. Its
    region says `capacity: 0x4000`, but the `initial-content` block's
    extent was not decoded.
  - That `std::filesystem` directory listing works in the Emscripten
    file system as `DirectoryBundle` needs it.
  - That the 14 `.rom.yaml` images equal today's 5000-byte dumps byte
    for byte (P3 checks it).
- **Assumed from other designs** (for the orchestrator to reconcile):
  - LIB-1: `RomEntry`, `RomImage`, `Catalogue::addPeripheral`,
    `MachineDescriptor::romsFor`, `build()` with `missingRoms`, and the
    `TestMachines` catalogue, as in its design.
  - LIB-3 (M-22): an app session that runs a start step before any
    machine is built, and `stateTopic("roms-missing")` for `RomsMissing`
    (LIB-3 AC-38; its design maps `romMissing` to it in M-27). It sends
    that topic in place of the machine topics. `roms` may use `sessions`
    (LIB-1 §2a).
  - C59-1 (M-19): the Calc-U-59 catalogue with ROM entries per model
    (LIB-1 P1), and the Datamath decoders in the core (LIB-1 D-4).
  - C59-6 (M-25): the module peripheral descriptors carry the module
    `RomEntry`s, and A-2 and A-3 go.
  - C1600-1 (M-26): Sharp machines built through `BuildRequest`. The
    callers of `BundledRoms::` that remain are named as adapters for
    M-27.
  - LIB-11 (M-28): `calcu` calls `lookUpRoms` through the product and
    ends with `describe()` (EC-1). `HarnessProduct::bundle()` returns
    `std::unique_ptr<roms::Bundle>`, normally a `DirectoryBundle` of the
    run's ROM folder; LIB-11 O-2 is met by `DirectoryBundle` + `lookUp`
    + `describe` (reconciled: the LIB-11 design now says so).
  - C59-6 (M-25; its C-1 now agrees, reconciled): the machine gets the module file as
    bundled (raw `.rom.yaml` bytes), and the core decodes it with
    `core/ModuleFile`. `ModuleFile` also gives `module-name`,
    `description` and `menu-sort` for `moduleDescriptors()`.
  - LIB-15 (M-29): EC-3's failed start.
  - LIB-14, C59-8: draw `RomsMissing`.

## 10. Part E: `cuecards.txt` leaves the source repository (M-55)

Approved by the user at the Part D/E boundary (2026-11-16). Paths without a prefix are
under `products/calc-u-59/`; line numbers at `7bdd51c`.

### 10.1 Scope

| Column | Step | Clause | Done in |
|---|---|---|---|
| Calc-U-59 | M-55 (C59-7 AC-22, AC-23) | AC-14: the module files are "made once from … the cue cards of today's `roms/solid-state/cuecards.txt`, which then leaves the source repository" | M-24 (unchanged; the column stays Done) |

The 14 `.rom.yaml` files with their `cue-cards:` were made in the ROM
repository before the plan (plan §1.2) and are fetched and checked since
M-24. What is left of AC-14 is the product's side: `cuecards.txt` is
still read for the cue cards (§4, "Kept until M-55";
`app/ModuleCatalog.hpp:10-16`, `core/ModuleFile.hpp:7-12`). C59-7's
design (§3.5) reads them from the module files; this section names what
leaves with the file, so that LIB-12's claim holds. No new criterion is
tested here; the column does not reopen.

### 10.2 Files

| File | Change |
|---|---|
| `roms/solid-state/cuecards.txt` | deleted (with the folder `roms/solid-state/`) |
| `roms/fetch_roms.sh:26`, `roms/README.md:25-31` | the lines on `cuecards.txt` go |
| `frontends/qt6/CMakeLists.txt:107-111` | the glob takes `roms/*.txt` and `roms/*.rom.yaml` only |
| `frontends/web/build.sh:43-48`, `calc-engine-worker.js:6`, `bindings.cpp:9, 60` | `bundle/` without `cuecards.txt` |
| `frontends/swift/Calc-U-59.xcodeproj/project.pbxproj:31, 89, 184`, `App/ROMLoader.swift:10, 36-41` | the resource and `cuecardsText` go; `bundleReader` (§4, M-24 row) has no other reader and goes |
| `tests/rom_missing_build.sh:22` | no copy of `cuecards.txt` |
| `app/ModuleCatalog.hpp:10-16` | the comment on `cuecards.txt` goes |

### 10.3 Task

| Task | What | Test |
|---|---|---|
| P10 | AC-14's last clause | before the file is deleted, a one-time check in M-55: each module file's `cue-cards:` equals `cuecards.txt`'s cards of that module field by field, plus the stand-ins (`modules.md` §2.3); the result goes into C59-7's qa-report. The lasting test is C59-7's T4. `CalcU59RomMissingBuild` and the Swift and Web builds run without the file |

### 10.4 Decided (best guess)

- **D-11 (AC-14).** The clause "which then leaves the source repository"
  is carried out in M-55, with C59-7 AC-22 and AC-23, the step that
  stops reading the file; LIB-12 Calc-U-59 stays Done (M-24), since
  every criterion of the column passed there and the file's last reader
  is a C59-7 matter.

## Part E simplification E-1 (2026-11-17, branch `es-assembly`)

`docs/reviews/e-simplification.md` §1 S-1. The lookup's result reaches the
library's parts whole: `parts::LibraryPartsSetup::roms` is the
`roms::LookupResult` (before: the set and a sentence), and LIB-18's
`StartupPart` publishes the topic `roms-missing` `{text}` from it (LIB-18,
this step). Calc-U-59's `TIAppSetup::roms` is the host's `LookupResult` too;
`TIAppSetup::romsMissing` and Calc-U-1600's `RomsMissingPayload` are gone.
The value form of the topic is unchanged.

## Part E simplification E-8 (2026-11-17, branch `es-dead`)

`docs/reviews/e-simplification.md` §1 S-8 (item 5 of the step). Calc-U-1600's ROM lookup stays in
the core (`core/Machine/SharpRoms`), against §2's `app/`: the reason its
comment gave (the CLIs and probes linked the core only until M-51) is
gone, but the core's scan of module definition folders
(`core/Connector/MemoryCardCatalog.hpp`, used by `SharpCatalogue`'s
`addModuleDefinitions`) passes over the files of the full set through
`sharp::isFullSetFile`, which needs `fullRomSet()`. Moving the lookup to
`app/` would split the full set from that check or make the core call
`app/`; the comment now gives this reason.
