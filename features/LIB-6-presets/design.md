# LIB-6 Presets: design
Status: Library column approved at the Part C1/C2 boundary (2026-11-15); Calc-U-1600 column approved at the Part C2/D boundary (2026-11-16); Calc-U-59 column (§11): approved by the user at the Part D/E boundary (2026-11-16)

## 1. Scope

Part C2 works on the Library column only. The product columns come in
Parts D and E, worked together with C1600-4 and C59-2, whose designs
are approved at those boundaries. This design names what they plug in
(§4) so that the seams hold.

| Column | Step | Tests from (plan §3) | Done in (plan §5.1) |
|---|---|---|---|
| Library | M-38 | AC-1–34, AC-52–53, AC-55–66; EC-1–10 | M-38 by the plan; proposed M-41 (D-29) |
| Calc-U-1600 | M-46 (with C1600-4, LIB-18 1600, LIB-10 1600) | AC-44–51, AC-54, AC-67 | M-46 |
| Calc-U-59 | M-54 (with C59-2, LIB-18 59, C59-3) | AC-35–43, AC-68 | M-54 |

- M-38 waits for M-3 (FS 5: idle on a TI machine holds as AC-36 says),
  M-35 (LIB-18 L), M-36 (LIB-5, all columns). It changes `library/` and
  `docs/test-groups.md` only. No product calls the new code before M-46.
- The products keep their own readers until their column: Calc-U-1600's
  `core/Preset/` and the two family loaders until M-46, Calc-U-59's
  `app/StateFile.*`, `app/StateApplier.*` and `TIPresetOperation` until
  M-54 (§5). Their presets stay in their old formats until then (plan
  §2.3, "convergence point").

Clauses of the Library ACs that name a feature built later. M-38 tests
the seam with a section, step or rule built for the tests; the clause
itself passes in the owner's step (plan §1):

| Clause | Passes in |
|---|---|
| AC-19 the debugger steps carried out while paused; AC-63 `debug-panel:` | M-40 (LIB-10). M-38: a test frontend step and a test pause through `MachineSession::setPaused` |
| AC-24, AC-40, AC-48 the product's comparison; AC-53 each product's idle, after-build and picture | M-46, M-54 (C1600-4 AC-13, C59-2 AC-8) |
| AC-25 the save folder in Qt6 and Swift, the download on the Web | M-46, M-54 (the frontends); M-38 tests both paths with the test frontend |
| AC-31 "every distribution is built from it" | M-46, M-54 (the bundle file of each product) |
| AC-34 `printer-trace` | M-54 (C59-6's step). M-38: a test product step `printer-trace` on the test printer's `trace` switch |
| AC-3 a module by `file:` | M-41 (LIB-7 resolves references by `file:`); refused with a notice until then (D-22) |

Later steps that use this design without adding a criterion: M-39
(LIB-8: `cut-paper:`; the paper at a load, AC-41), M-40 (LIB-10: the
debugger and frontend steps), M-41 (LIB-7: media steps, `saveas:`,
`magnetic-cards:`, references by `file:`), M-42 (LIB-13: "open file"
hands a preset to `load-preset`), M-43 (LIB-14: "Loading…" from busy),
M-64 (the hosted presets of AC-43 before R-59).

## 2. Modules and files

### Library (M-38): `library/presets/`, namespace `calcu::presets`

The folder LIB-1 §2a expects. Its dependency line for §2a (to be added
there with this step): `presets`: `machine`, `formats`, `settings`,
`sessions`, `peripherals`, `typing`, `startup`. Nothing includes
`presets` but `harness` (any area but `web`) and the products. `startup`
does not include it: LIB-18 declares the loader it calls, `presets`
implements it (§3.8). Tests in `calcu_library_tests`.

| File | Holds |
|---|---|
| `Source.hpp/.cpp` | `PresetSource` and its three forms (a folder on disk, the product's bundle, content without a folder; EC-5); `PresetBundle`, `DirectoryPresetBundle`; `parseBundlePath` (`examples:<path>`, AC-33) (§3.1) |
| `ProductRules.hpp` | `ProductRules`, `ModelRules`, `IdleRule`, `Applies`, `PlaceKeyRule`, `ExpectText`, `ExpectReading` (§3.4) |
| `Vocabulary.hpp/.cpp` | `HeaderKeyDef`, `SectionDef`, `StepDef`, `Item`, `After`, `Vocabulary` (§3.3) |
| `Reader.hpp/.cpp` | `ReadContext`, `ReadError`, `LoadPlan`, `PresetHeader`, `readPreset`, `readHeader` (§3.2) |
| `LibraryKeywords.cpp` | the header keys, `keys:` and the steps of this feature, registered by `addLibraryKeywords` (§3.5) |
| `Expect.hpp/.cpp` | `parseExpectText`: literal, `\/`, or `/<ECMAScript>/` compiled at read (AC-24, AC-66; AC-67's parse) |
| `Engine.hpp/.cpp` | `carryOut`: the build, the items in file order, the implicit wait, `skip-wait:`, `let-it-finish:`, `timeout:`, the end at regular speed (§3.6) |
| `IdleWatch.hpp/.cpp` | `IdleWatch` over a product's `IdleRule` (AC-53; AC-36 and AC-45 as values) |
| `BundleFile.hpp/.cpp` | `readBundleFile` (`docs/formats/bundle.md`), `PresetListEntry`, `checkBundle` (bundle.md §3) (§3.7) |
| `PresetsPart.hpp/.cpp` | `PresetsPart`: the session part with `load-preset`, `run-step`, `wait-idle`, `check-preset`, `preset-list`; implements LIB-18's `startup::PresetLoader` (§3.8) |
| `PresetChecks.cmake` | `calcu_add_preset_check(<name> PRODUCT <word> FILES … [EXCLUDE …] [FILL <script>])`: a CTest entry that runs `calcu --product <word> check-preset` over the files (AC-34, AC-43, AC-51; D-20) |
| `tests/TestPresetRules.hpp/.cpp` | the rules and vocabulary built for the tests: idle on `test-seg` and `test-dot`, a test section `test-memory:` that writes a test capability, a test header key with a block, a test step `test-panel:` (a frontend step), a test after-build step, a picture (the display as bytes) and a comparison (the display text) (§6) |
| `tests/fixtures/` | presets of the test models (`*.test-seg`, `*.test-dot`), a bundle `bundle.yaml` with startup presets, two groups and named files; bad files, one per error class |
| `tests/*_tests.cpp` | one file per area of §6 |
| `library/CMakeLists.txt` (changed) | the sources of `library/presets/`, its tests, `include(presets/PresetChecks.cmake)` |

Changed in other areas in M-38:

| File | Change |
|---|---|
| `library/formats/Yaml.hpp` | `YamlOptions{bool repeatTopLevelKeys; std::vector<std::string> rawStepVerbs;}` and an overload `parseYaml(text, out, error, options)`; the default keeps today's behaviour for every other format. `YamlNode::rawLine` for a raw value (D-4) |
| `library/formats/Number.hpp/.cpp` (new) | `parseNumber`: `&`, `0x`, `$` hex, else decimal; the whole value (README §1). `YamlNode::asInt` stays as it is (the module definition takes `0x` only) |
| `library/sessions/Operation.hpp/.cpp`, `AppSession.cpp` | `OperationContext::hostWait(WallDuration)`: an awaitable that ends when the session's wall clock has passed it; at once in `Hosting::VirtualTime` (AC-19; D-17) |
| `library/peripherals/PeripheralsPart.*` | the command `set-control {owner, control, on}` through the controls capability (LIB-2 AC-16): a switch set, a key pressed (`on: true`) or released; for the harness `switch` (AC-34; D-16) and LIB-8's paper panel (M-39) |
| `library/harness/PresetCommands.cpp` (new), `Commands.hpp` | `addPresetCommands(CommandRegistry&)`: `preset`, `check-preset`, `preset-list`, `full-speed`, `regular-speed`, `elapsed-time`, `expect`, `not-expect`, `screenshot`, `syncclock`, `switch`, and every other step with `command = true` (a product's `printer-trace`; LIB-7's media steps from M-41) (§3.9) |
| `library/harness/FirstCommands.cpp` | `wait` without a duration sends `wait-idle` (AC-34; LIB-11 AC-12) |
| `library/harness/Product.hpp` | `virtual presets::ProductRules* presetRules() { return nullptr; }`, `virtual const presets::PresetBundle* presetBundle() const { return nullptr; }` |
| `library/harness/HarnessHost.cpp`, `Run.cpp` | a `PresetsPart` in the session's parts when the product has rules, before `StartupPart`, with the run's folder for screenshots (LIB-11 AC-8), and handed to LIB-18 as `StartupSetup::loader` (LIB-18 D-9: none until M-38) |
| `library/harness/tests/TestProduct.*`, `tests/preset_commands_tests.cpp` (new), `tests/cli/cli_cases.cmake` | the test product's rules, its `printer-trace` step and bundle; AC-34's cases; the exit status of `check-preset` |
| `library/machine/tests/TestMachines.*` | build option `TestSegProgram` (`BuildRequest::content`): the key `1` starts a "program" of a settable number of ticks during which `isKeyReady()` is false; the key `0` during it stops it (AC-56, AC-58, AC-59, AC-64) |
| `docs/test-groups.md` | the new files and entries |

## 3. Seams and interfaces

Signature sketches. Single-threaded, on the session's thread, no
exception across the interface, no lock (LIB-1 §2a). `Task`,
`OperationContext`, `SessionContext` are LIB-3's.

### 3.1 Sources and bundle paths (AC-1, AC-33, EC-1, EC-5)

```cpp
namespace calcu::presets {
class PresetSource {                     // the preset's text and the files it names
public:
  virtual ~PresetSource() = default;
  virtual std::string name() const = 0;                    // for messages: path, bundle path or the given name
  virtual std::string stem() const = 0;                    // preset-name default (AC-2)
  virtual std::optional<std::string> text() const = 0;     // nullopt: cannot be read (EC-1)
  // A path a preset names: relative to the preset's folder, `~/` the home folder (README §3).
  // nullopt with a sentence: cannot be read, or relative without a folder (EC-5).
  virtual std::variant<std::vector<std::uint8_t>, std::string> readNamed(std::string_view path) const = 0;
  virtual std::string resolvedName(std::string_view path) const = 0;   // what a section's owner shows or keeps
};
std::unique_ptr<PresetSource> folderSource(std::filesystem::path file, std::string home);
std::unique_ptr<PresetSource> contentSource(std::string text, std::string name, std::string home);   // EC-5
std::unique_ptr<PresetSource> bundleSource(const PresetBundle&, std::string pathInBundle);
class PresetBundle {                     // the product's bundled presets (AC-31, AC-33)
public:
  virtual ~PresetBundle() = default;
  virtual std::optional<std::vector<std::uint8_t>> read(std::string_view pathInBundle) const = 0;
  virtual std::string where() const = 0;
};
class DirectoryPresetBundle final : public PresetBundle { /* a folder: Qt resources unpacked, the app bundle, Emscripten's preloaded tree */ };
std::optional<std::string> parseBundlePath(std::string_view ref);   // "examples:basic/hanoi.pc1600" → "basic/hanoi.pc1600"
std::unique_ptr<PresetSource> sourceFor(std::string_view ref, const PresetBundle*, std::string home);   // AC-33
}
```

- A bundle path stays inside the bundle: `..` that leaves it is an
  error (bundle.md §2). A file a bundled preset names is read from the
  bundle, relative to the preset (`examples:basic/hanoi.bas`).
- `text()` checks UTF-8. A file that is not UTF-8 is refused with its
  name (EC-1). A leading byte-order mark is dropped (D-28).

### 3.2 Reading: all or nothing (AC-1 to AC-9, AC-30, AC-52, AC-62, EC-1, EC-5, EC-8)

```cpp
struct ReadError { int line = 0; std::string message; };   // line 0: the file as a whole
struct HeaderInfo { std::string presetName; bool nameWritten; std::string description; std::int64_t timeoutSeconds = 3600; };
struct PlaceRequest { machine::PeripheralRequest request; int line; };   // ROMs filled at the build (LIB-18)
struct LoadPlan {
  machine::ModelId model;  std::optional<machine::VariantId> variant;  int modelLine;
  std::vector<PlaceRequest> places;                                  // AC-3, AC-10
  HeaderInfo header;
  std::map<std::string, std::shared_ptr<const ProductValue>> productHeader;   // a product's header keys (debug:, bus-rom:)
  std::vector<std::unique_ptr<Item>> items;                         // sections and steps in file order (AC-14)
  ModelRules* rules;                                                // the model's (AC-53)
};
std::variant<LoadPlan, ReadError> readPreset(const PresetSource&, const Vocabulary&, ProductRules&, const machine::Catalogue&);
struct PresetHeader { machine::ModelId model; std::string name, description; };
std::variant<PresetHeader, ReadError> readHeader(const PresetSource&, ProductRules&, const machine::Catalogue&);   // AC-32: no file named is read

class ReadContext {                      // what a keyword's reader sees
public:
  const machine::Catalogue& catalogue() const;
  const machine::MachineDescriptor& model() const;    // known before any keyword but format-version and model
  ModelRules& rules();
  bool plugged(const machine::PlaceName&, const machine::PeripheralId&) const;   // what the header names
  std::optional<std::vector<std::uint8_t>> readFile(std::string_view path, int line);   // reports the error itself
  const PresetSource& source() const;
  void keep(std::string key, std::shared_ptr<const ProductValue>);     // a product header key's value
  void error(int line, std::string message);                           // collected; the first line wins
};
```

`readPreset` in order (D-3):
1. `text()`; EC-1.
2. The YAML reader with `repeatTopLevelKeys` and the raw verbs `type`,
   `expect`, `not-expect` (D-4, D-5). The file is split at its lines of
   indent 0, and each top-level item is parsed on its own, so a syntax
   error refuses the file with its line without hiding an earlier error
   of another kind.
3. `format-version` before any other key (README §2): missing or below
   2 is the outdated notice (AC-8), with the product's name and
   `ProductRules::formatReference()`; above 2 "written by a newer
   release".
4. `model`: name and variant matched without regard to case
   (`MachineDescriptor::model`, `VariantDescriptor::id`; the hyphen is
   part of the ID). Unknown: the notice lists the catalogue's models
   (AC-9). A variant on a model without variants is an error.
5. Every other top-level key, in file order: a header key at most once
   (a second is an error at its line), a section or a step list by the
   vocabulary. Each keyword is first asked `ModelRules::applies()`
   (AC-62): `NotForThisModel` → "<keyword> is not available on <model>";
   `Undefined` → "<keyword> is not defined yet for <model>"; a keyword no
   part of the program registered → "unknown key <keyword>" (AC-30).
6. Place keys (AC-2, AC-3): the model's descriptor places, under their
   names. The value is a peripheral ID with an optional `:variant`, or a
   reference `{ name: <ID> }`, matched without regard to case, that
   fits the place by `Catalogue::place()` (LIB-2 AC-2);
   `ModelRules::placeKey()` narrows the forms a place takes (D-22). A
   place no key names stays empty (AC-3, AC-65).
7. Steps: `skip-wait:` as the first or last item, and `let-it-finish:`
   not last, are errors (AC-58, AC-59). An empty step list is an error
   (EC-8).
8. The result: the error with the lowest line among those collected
   (the file as a whole, line 0, counts first), or the plan.

The message is `<name>, line <n>: <what was expected>`. The reader
builds nothing and touches no session. Header keys may stand anywhere:
a key's reader that needs the plugged peripherals (a `printer-trace`
without a printer, AC-40) is run after all header keys are read (D-3).

### 3.3 The vocabulary: the place for features and products (AC-29, AC-30)

```cpp
enum class After { Idle, TakenEffect, Own };   // AC-56: what "finished" means for the item
class Item {                                   // a checked section or step, ready to be carried out
public:
  virtual ~Item() = default;
  int line = 0;
  virtual Task<ItemResult> carryOut(RunContext&) = 0;   // until "carried out" (AC-58)
  virtual After after() const = 0;               // Idle: key, type. Own: its carryOut includes its finish (a section, card:). TakenEffect: the rest
  virtual bool needsRunningMachine() const = 0;  // AC-19: key, type, card, a section that writes memory
  virtual std::string logText() const = 0;       // AC-18
};
struct ItemResult { bool ok = true; std::string reason; };   // !ok: the preset stops here (AC-17)
using Where = std::function<Applies(const machine::MachineDescriptor&)>;   // optional; the rules decide too
struct HeaderKeyDef { std::string key; std::function<void(const formats::YamlNode&, int line, ReadContext&)> read; };
struct SectionDef   { std::string key; std::function<std::unique_ptr<Item>(const formats::YamlNode&, ReadContext&)> read; };   // a block, or a value where the owner says so (partition: 479)
struct StepDef {
  std::string verb;
  enum class Value { None, Scalar, Raw, Any } value;          // `- full-speed:`, `- wait: 2s`, `- expect: …`, `- floppy: {…}`
  std::function<std::unique_ptr<Item>(const formats::YamlNode&, ReadContext&)> read;
  bool command = true;                                        // also a harness command of the same name (§3.9)
  bool frontend = false;                                      // AC-63: never fails; passed over where nobody has the panel
};
class Vocabulary {
public:
  void add(HeaderKeyDef);  void add(SectionDef);  void add(StepDef);   // a name twice is a programming error, reported at attach
  const HeaderKeyDef* header(std::string_view) const;  const SectionDef* section(std::string_view) const;  const StepDef* step(std::string_view) const;
};
```

- A feature adds its keywords in its step: LIB-8 `cut-paper`, LIB-10 the
  debugger and frontend steps, LIB-7 media steps, `saveas`,
  `magnetic-cards`, the `file:` resolver; a product adds its own
  through `ProductRules::addKeywords()` (§3.4). They are read by the
  same YAML, checked with the rest, carried out in file order and give
  their errors with the line (AC-29). No block is handed over as raw
  lines (AC-1).
- `skip-wait:` and `let-it-finish:` are engine markers, not items.

### 3.4 Product rules (AC-53, AC-24, AC-25, AC-62)

```cpp
enum class Applies { Yes, NotForThisModel, Undefined };
struct IdleRule {                                  // AC-53: idle from what the machine reports
  std::function<bool(const machine::Machine&)> readyNow;   // default isKeyReady(); TI: or waits for a card (AC-36)
  machine::EmulatedDuration stretch;               // ready without a break this long: 100 ms TI, 20 frames Sharp (M-46, M-54)
  machine::Ticks sample;                           // how often readyNow is asked while waiting
};
struct ExpectText { std::string text; std::optional<std::regex> regex; std::string written; };   // Expect.hpp
struct ExpectReading { bool displayOff = false; bool found = false; std::string shows; };       // shows: for the reason
struct PlaceKeyRule { bool bare = true; bool byName = true; bool byFile = false; };              // AC-35: TI slot-1 by name: only
class ModelRules {
public:
  virtual ~ModelRules() = default;
  virtual IdleRule idle() const = 0;
  virtual Task<ItemResult> afterBuild(RunContext&) { co_return {}; }    // AC-46: `cl` at NEW0? :CHECK
  virtual std::optional<std::vector<std::uint8_t>> screenshot(machine::Machine&, const machine::DisplayOutput&) = 0;   // PNG bytes (AC-25)
  virtual ExpectReading compare(machine::Machine&, const machine::DisplayOutput&, const ExpectText&) = 0;          // AC-24, AC-66
  virtual std::optional<std::string> checkExpect(const ExpectText&, std::string_view keyword) const { return std::nullopt; }   // AC-68: the refusal when read; TI: a number only (P-6)
  virtual Applies applies(std::string_view keyword) const { return Applies::Yes; }   // AC-62, the markers of preset.md §7
  virtual PlaceKeyRule placeKey(const machine::PlaceName&) const { return {}; }
};
class ProductRules {
public:
  virtual ~ProductRules() = default;
  virtual std::string_view productName() const = 0;          // for the notices
  virtual std::string_view formatReference() const = 0;      // where the preset format is described (AC-8)
  virtual ModelRules& forModel(const machine::ModelId&) = 0;
  virtual void addKeywords(Vocabulary&) {}                   // AC-29: sections, keys, steps
};
}
```

- The library names no model and no product (TR; `presets.md` PR-11).
  The markers of `preset.md` §7 are each product's `applies()`; a
  keyword the running program does not know is unknown (AC-30), so a
  section of the other product is refused (D-7).
- The text that `expect` compares on a Sharp model is LIB-11 AC-10's
  display text. The product's `compare` uses the same rule as its
  `HarnessProduct::readDisplay` (one function in the product, M-46).

### 3.5 This feature's keywords (AC-2, AC-4, AC-20 to AC-28, AC-57, AC-66)

| Keyword | Form | Read checks | Carried out | After |
|---|---|---|---|---|
| `format-version`, `model`, `preset-name`, `description`, `timeout` | header | §3.2; name ≤ 18 code points (D-12); description one line (D-26); timeout a whole number of seconds ≥ 1 (D-11) | build plan, header | — |
| place keys | header | §3.2 step 6 | build plan | — |
| `keys:` | section, step list | not empty (EC-8); repeats (AC-4) | its steps, each an item | — |
| `key:` | step, scalar | `TypingProfile::name` per name (AC-15, AC-26, AC-41) | `typeAndWait`, until the last press is up | Idle |
| `type:` | step, raw | the profile has `charKeys` and `enter`, else not on the model (AC-40); `charKeys` per character (AC-16) | `typeTextAndWait`, until ENTER is up | Idle |
| `wait:` | step, scalar | `formats::parseDuration`; none: refused (AC-21) | the duration in emulated time; while paused in host time (AC-19, AC-20) | TakenEffect |
| `regular-speed:`, `full-speed:` | step, none | in `keys:` only | `OperationContext::setSpeed` (AC-22) | TakenEffect |
| `skip-wait:`, `let-it-finish:` | marker, none | places (AC-58, AC-59) | §3.6 | — |
| `elapsed-time:` | step, scalar `start` / `report` | the two words | baseline = tick count; report logs `Elapsed: m:ss.mmm` (AC-23; D-10) | TakenEffect |
| `expect:`, `not-expect:` | step, raw | not empty; `parseExpectText`; a text `ModelRules::checkExpect` refuses is refused with its sentence (AC-68) | `ModelRules::compare`; fails with the text and `shows`; EC-7 | TakenEffect |
| `screenshot:` | step, scalar | not empty, no `/` or `\` | the product's picture into the folder, or `KeepFile` (AC-25; D-13) | TakenEffect |
| `syncclock:` | step, none | `applies()` | `peripherals::setClocksFromHost(machine, hostDateTime(session.hostTime()))`; logs the time set (AC-27) | TakenEffect |
| `switch:` | — | not registered: unknown (AC-26) | — | — |

`elapsed-time: report` counts from the last `start`, else from tick 0
of the machine the preset built (D-10). The minutes count on beyond 59.

`StepDef::command` (§3.9): `elapsed-time`, `expect`, `not-expect`,
`screenshot` and `syncclock` are `true`; `key` and `type` (LIB-5's own
harness commands, M-36), `wait`, `regular-speed` and `full-speed`
(harness commands of their own, §3.9) are `false`. Later features set it
as their designs say: LIB-8's `cut-paper` and LIB-10's steps `false`,
LIB-7's media steps `true`.

### 3.6 Carrying it out (AC-10 to AC-19, AC-55 to AC-61, EC-2 to EC-4, EC-6, EC-10)

```cpp
struct LoadOptions { bool buildOnly = false; bool startup = false; };   // AC-55; startup: through PresetLoader::load (AC-41, D-24)
class RunContext {                    // what an item sees while it is carried out
public:
  sessions::OperationContext& op();
  machine::Machine& machine();
  const LoadPlan& plan() const;
  bool firstPressAtOnce() const;      // the item before was followed by skip-wait: (AC-58; LIB-5 AC-14)
  machine::Ticks deadline() const;    // timeout: for an Own item's wait (AC-57)
  Task<void> untilRunning();          // AC-19
  void log(std::string);
  typing::TypingEngine& typing();
};
Task<sessions::OperationResult> carryOut(sessions::OperationContext&, std::shared_ptr<LoadPlan>, LoadOptions, Deps&);
```

The operation, in order:
1. **Build** (AC-10, AC-16, AC-52, EC-10): `startup::MachineControl::build(session,
   MachinePlan{model, variant, peripherals}, origin)` (LIB-18 §3.4), with
   `BuildOrigin::Preset` for a user's load and `StartupPreset` for one
   through `PresetLoader::load` (D-24), then `co_await
   control.untilKeyReady(op)`. At full speed, the operation's default
   (LIB-3 AC-14). A refused build changes nothing (LIB-18 §3.4); the reply
   and a notice give `BuildRefusal::reason` with the line of the key of
   `BuildRefusal::place` (the line of `model:` when it names none), the
   machine before untouched. `untilKeyReady` false (LIB-18 EC-7): the load fails
   there, no item runs (EC-10). Logged: the file, the machine, one line
   per peripheral (AC-18).
2. `buildOnly`: the load ends here (AC-55).
3. `ModelRules::afterBuild` (AC-46, AC-53).
4. **Items in file order** (AC-14). Before an item with
   `needsRunningMachine()`, `untilRunning()` (paused: it waits, AC-19).
   Then `carryOut`; a failure stops the preset with the line and the
   reason, the machine as it is (AC-17). Then, unless it is the last
   item or a `skip-wait:` follows:
   - `After::Idle`: the **implicit wait**. An `IdleWatch` on the model's
     rule; the machine runs at the speed in force in pieces of
     `min(rule.sample, OperationContext::runPiece)`, `readyNow` asked
     after each; idle when ready for `stretch` without a break (AC-56).
     The emulated ticks waited are bounded by `timeout:`: past it the
     item fails (AC-57). While paused no tick passes, so the wait and the
     bound stand still (AC-19).
   - `Own`: the item waited itself, with `deadline()`.
   - `TakenEffect`: nothing.
5. **The last item** is carried out and not waited for (AC-60, EC-2,
   EC-4). With `let-it-finish:` after it, the implicit wait runs without
   a bound (AC-59).
6. **End**: `setCommandedSpeed(Regular)`, also after a failure (AC-22,
   AC-60; D-19). A paused machine stays paused. The last log line says
   whether the preset was carried out to its end (AC-18).

- **`wait:`** (AC-20, AC-19): in pieces; a piece is `op.run(ticks)` at
  the speed in force, or `op.hostWait(µs)` while paused. While a
  `wait:` runs at full speed the operation reports "not waiting"
  (`setReportsWaiting(false)`; LIB-17 AC-16).
- **One operation** (AC-12): the load is one `StartOperation`. Busy,
  dropped live input (LIB-5 AC-18), a second load refused, the speed
  the preset's (AC-61; LIB-3 D-6), Reset, Cold boot and a model switch
  end it with the reply "Stopped." (EC-3; LIB-3 D-7, LIB-18 AC-16).
- **In parts and in one go** are the same by LIB-3's run requests
  (AC-13). The engine reads no wall clock; `hostWait` is the only wall
  time and ends at once in the harness (AC-19).
- **Suspended** (EC-6): the session makes no host call, so nothing
  advances.
- AC-11 holds because the plan is a pure function of the file, the
  files it names and the catalogue, and the host time comes from the
  session (A-7 of LIB-3).

### 3.7 The preset list (AC-31, AC-32, AC-33, EC-9)

```cpp
struct PresetListEntry {
  std::string bundlePath;  machine::ModelId model;  std::string name, description;
  enum class Kind { Startup, Example } kind;  std::string group;    // group: examples only
  std::vector<std::string> files;                                   // bundle.md §2
};
std::variant<std::vector<PresetListEntry>, ReadError> readBundleFile(const PresetBundle&);   // the entries' order, without header reads
std::vector<std::string> checkBundle(const PresetBundle&, ProductRules&, const Vocabulary&, const machine::Catalogue&);   // bundle.md §3: one line per problem
```

- `bundle.yaml` is read with the same YAML (`format: example-bundle`,
  `format-version: 1`; unknown keys refused).
- The request `preset-list` reads `bundle.yaml`, then only the header
  keys of each preset (`readHeader`). An entry whose preset is missing
  or whose header cannot be read is left out, with a log line (EC-9). A
  missing file it names does not matter (AC-32).
- `checkBundle` lists every listed file that is missing, a preset that
  is listed twice or both as startup and example, a path that leaves the
  bundle, and a file a listed preset names that is not listed with it.
  The products' suite checks call it (M-46, M-54); `presetNameProblems`
  (a name twice per model, C59-2 AC-15, C1600-4 AC-18) moves with
  Calc-U-1600's `app/PresetCatalog.*` in M-46.

### 3.8 The session part (AC-6, AC-12, AC-34, AC-55)

```cpp
struct PresetsSetup {
  const machine::Catalogue* catalogue;  ProductRules* rules;
  startup::MachineControl* control;    typing::TypingPart* typing;       // LIB-18's StartupPart::control(); LIB-5's part, its engine and TypingProduct (§3.10)
  const PresetBundle* bundle;           std::string home;
  std::function<std::optional<std::string>()> screenshotFolder;           // LIB-9 Paths::screenshotFolder(); the harness: the run's
};
class PresetsPart final : public sessions::SessionPart, public startup::PresetLoader {
public:
  explicit PresetsPart(PresetsSetup);
  Vocabulary& vocabulary();             // library keywords, then the product's, then later features'
  void attach(sessions::SessionSetup&) override;
  // startup::PresetLoader (LIB-18 §3.3; AC-4, EC-1, EC-6): readPreset, then carryOut with LoadOptions{startup = true}.
  // A read error, an outdated file, another model than forModel or a refused build:
  // CannotLoad, nothing built (LIB-18 EC-1); a failed item: FailedWhileRunning (EC-6); else Built.
  Task<startup::PresetLoad> load(std::string ref, std::optional<machine::ModelId> forModel,
                                 startup::MachineControl&, sessions::OperationContext&) override;
};
```

| Name | Shape | Args | Reply |
|---|---|---|---|
| `load-preset` | command → operation | `{ref}` (a path or a bundle path, AC-33) or `{content, name}` (EC-5, AC-42); `buildOnly`; `cause` (`user` default, `startup`) | done; or refused with `<name>, line <n>: …` and a notice (AC-6); failed with the line and the reason (AC-17); "Stopped." (EC-3) |
| `run-step` | command → operation, or done | `{verb, value}`: one step read by the vocabulary against the running machine, carried out, not waited for | as the step: done, or refused with the reason and what the display shows (AC-24, AC-66) |
| `wait-idle` | command → operation | — | `{ticks, seconds, hourReached}`: idle by the model's rule, at most one hour of emulated time (AC-34) |
| `check-preset` | request | `{ref}` or `{content, name}` | `ok`, or the first error (AC-34) |
| `preset-list` | request | — | the entries (AC-32) |

- `load-preset` is refused at once while an operation runs ("Another
  operation is running.", LIB-3 D-7). Otherwise the handler reads and
  checks the whole file before it returns `StartOperation`, so a refused
  preset never makes the session busy and never touches the machine
  (AC-6; D-6).
- `run-step` reads the step with the running machine in place of a
  header: `ReadContext::plugged()` asks `Machine::plugged()`.

### 3.9 Harness commands (AC-34)

`library/harness/PresetCommands.cpp`, on the registry of LIB-11:

| Command | Sends |
|---|---|
| `preset <path or bundle path>` | `load-preset {ref}`; the path relative to the working directory |
| `check-preset <file>…` | `check-preset` per file; one line `ok` or `FAIL <error>` each; the run ends as failed if one failed |
| `preset-list` | `preset-list`, one line per entry |
| `wait` (no duration) | `wait-idle` |
| `full-speed`, `regular-speed` | LIB-3's `speed {full}` |
| `elapsed-time start\|report`, `expect <text>`, `not-expect <text>`, `screenshot <name>`, `syncclock`, and each other step with `command = true` (a product's, a later feature's) | `run-step {verb, value}`; the value is `argText` (LIB-11 §3.2) |
| `switch <peripheral> <control> on\|off` | `set-control` of LIB-2's part (D-16) |

The screenshot folder of a run is `RunSetup::screenshotFolder()`
(LIB-11 AC-8). `syncclock` takes the run's clock (LIB-11 AC-21). `key`
and `type` stay LIB-5's commands (M-36).

### 3.10 Seams of LIB-18 and LIB-5

LIB-18's design (M-35, `features/LIB-18-machine-control-and-startup/design.md`
§3) gives: `startup::MachinePlan`, `BuildOrigin` (with `Preset` and
`StartupPreset`), `MachineControl::build(SessionContext&, MachinePlan,
BuildOrigin)` → `std::optional<BuildRefusal>`, where `BuildRefusal{reason,
place}` names the refused place (AC-52), `MachineControl::untilKeyReady(OperationContext&)`
→ `Task<bool>`, `PresetLoader::load(ref, forModel, control, op)` →
`Task<PresetLoad>`, the commands `reset`, `cold-boot`, `model` declared
`endsRunningOperation` (LIB-18 AC-16), `ProductMachines::startUp()` before
LIB-6's `ModelRules::afterBuild()`. `MachineControl::build` drops input and
sets the commanded speed to regular before the build (LIB-18 AC-2); the
operation's own speed is full until a speed step (AC-16). A build through
`PresetLoader::load` is `StartupPreset`, a user's load `Preset`, so that
LIB-8 (M-39) empties the paper on a user's load and keeps it at a model
switch without reading presets (D-24; LIB-18 `lastBuild()`).

LIB-5's design (M-36, `features/LIB-5-typing/design.md` §3.2, §3.3,
§3.6) gives:

- **When read** (AC-7, AC-28; LIB-5 AC-15, AC-16, AC-26, AC-41): the
  model's `TypingProfile` from `TypingProduct::profile(model)`. Each name
  of a `key:` goes through `profile.name`; `nullopt` refuses it at its
  line. `type:` exists on a model only when the profile has `charKeys`
  and `enter` (AC-40); each character goes through
  `charKeys(c, nullptr)`, which is `nullopt` only for a character without
  a key (LIB-5 D-14), so a character without a key refuses the preset
  when it is read. A character whose presses depend on latches is
  decided again when typed (LIB-5 E13).
- **When carried out**: `typeAndWait(op, engine, presses, options)` and
  `typeTextAndWait(op, engine, text, true, options)`, with
  `TypeOptions{Origin::Preset, firstPressAtOnce = run.firstPressAtOnce()}`:
  after `skip-wait:` the next step's first press goes down at once
  (AC-58; LIB-5 §3.6). They end when the engine is idle (last key up,
  ENTER up; LIB-5 AC-17), and the implicit wait follows (§3.6).
- A held key on a paused machine comes up in host time (LIB-5 AC-20,
  `TypingEngine::setPaused`, `hostCall`), so a `key:` followed by
  `skip-wait:` has been carried out while paused (AC-19).
- LIB-5 keeps today's playback until LIB-6's columns:
  `TIKeystrokePlayback` (removed M-54) and the Sharp preset typers
  (removed M-46). This matches §5.

## 4. Product side

Nothing changes in either product in M-38. What the product columns
plug in (designed with C1600-4 and C59-2):

| Product | Today | In its column |
|---|---|---|
| Calc-U-1600 (M-46) | `core/Preset/PresetFile.*` (`parsePresetFile`, `readPresetHeader`, `kPresetFormatVersion` 1), `PresetRunner.*`, `PresetRoms.*`, `PresetMedia.hpp`, `PresetBusRomLoader.hpp`, `PresetDebugBlock.hpp`, `core/PC1500/PC1500PresetLoader.*`, `core/PC1600/PC1600PresetLoader.*`, `SharpAppPart::loadPresetJob` | `shared::SharpPresetRules : ProductRules` in `app/SharpPresetRules.*`: idle by AC-45, `cl` after the build by AC-46, the Copy Screen picture (AC-48, today `core/Display/LcdScreenshot`), the line comparison with `\\`, `\xHH` and the regex (AC-48, AC-67); C1600-4's keys and sections through `addKeywords`. `SharpAppPart` constructs `PresetsPart`; the readers of version 1 go |
| Calc-U-1600 | `app/PresetCatalog.*`, `examples/bundled.yaml` | `examples/bundle.yaml`, `preset-list`; `presetNameProblems` into `library/presets/` |
| Calc-U-1600 | `tools/pc1500_cli.cpp`, `tools/pc1600_cli.cpp` (`--check-preset`), `tools/check_presets.sh`, CTest `CalcU1600CheckPresets` | the CLIs load through the library's loader; `calcu_add_preset_check(CalcU1600Presets …)` over the files of AC-51 with the VS Code templates filled by the extension's rule (C1600-3 AC-19), the three waiting presets excluded by name; the script and the old entry go |
| Calc-U-59 (M-54) | `app/StateFile.*`, `app/StateApplier.*`, `TIAppPart::loadStateFile`, `applyState`, `presetOperation`, `TIPresetOperation`, the `STATE` records of `tests/golden/formats.txt` | `cu59::TIPresetRules : ProductRules` in `app/TIPresetRules.*`: idle by AC-36 (100 ms ready, or a card wait), the segment picture (OQ-3), the X register comparison (AC-40), `checkExpect` taking a number only, `slot-1` by `name:` only (AC-35), `type` and `syncclock` not for the TI; C59-2's sections and `printer-trace` through `addKeywords`. `TIAppPart` constructs `PresetsPart` |
| Calc-U-59 | `frontends/web/presets.json` (read by `frontends/web/build.js`) | `examples/bundle.yaml` in all three frontends |
| Calc-U-59 | `tools/headless/capi.cpp`, `tools/agentic/ti59file.py` | loads through `calcu`; `ti59file.py` goes (plan M-54); `calcu_add_preset_check(CalcU59Presets …)` over the shipped presets (AC-43); a Checkpoint entry over calc-u-site's hosted presets, named in the release checklist (M-64) |

Each product's `HarnessProduct` returns its rules and bundle from
`presetRules()` and `presetBundle()` in that step.

## 5. Adapters

LIB-6 L takes no old call behind a seam: the products call nothing of
it before their columns. The old readers stay as they are, behind the
adapters other designs already name:

| Adapter | Owner | Wraps | Removed in |
|---|---|---|---|
| `TIPresetOperation` | LIB-3 design §5 | `applyState` and the KEYSTROKES playback as one operation | M-54 (LIB-6 59) |
| `JobForward` | LIB-3 design §5 | `namespace job` for `PresetRunner` and the family loaders | M-46 (LIB-6 1600) at the latest |
| `c1600.*` preset commands (`loadPresetJob`) | LIB-3 design §5 | Calc-U-1600's load as a product command | M-46 |
| LIB-18's stand-in for the loader | LIB-18 design (M-35) | `TestPresetLoader : startup::PresetLoader` | M-38 for the library's own start tests, which then also run with `PresetsPart`; the stand-in may stay as a test double |

New in M-38: none. `HarnessProduct::presetRules()` returning `nullptr`
is not an adapter: until M-46 and M-54 `calcu --product 59|1600 preset …`
is refused with "This build reads no presets of format 2 yet." and
today's tools load presets as before (plan §1, "Today's tools").

## 6. Tasks

Tests come before the code. The library tests use the test machines
(LIB-1 AC-24), with `TestSegProgram`, and `TestPresetRules`: idle is
"key ready for 2 ms" on `test-seg` (sample 0.1 ms) and "for 20 frames"
on `test-dot`; the picture is the display output as bytes; the
comparison finds the text in the display as text of LIB-11's test
product. Session tests run on LIB-3's test frontend, with
`Hosting::VirtualTime` unless a case says Paced (`ScriptedClock`).
Every case is Frequent. Fixtures in `library/presets/tests/fixtures/`.

| Task | What | ACs/ECs | Test (file: cases) |
|---|---|---|---|
| L1 | YAML options, numbers, sources | AC-1, EC-1, EC-8 | `reader_tests.cpp`: `ac1_block_and_flow_forms_of_readme_section_1`, `ac1_quotes_without_escapes`, `ac1_block_scalar_skips_blank_and_hash_lines`, `ac1_hash_without_space_is_text`, `ac1_raw_values_of_type_expect_not_expect`, `ac1_tab_anywhere_refused`, `ac1_unknown_key_refused_at_every_level`, `ac1_hex_forms_and_booleans`, `ac1_paths_from_preset_folder_and_home`, `ac1_names_any_case_hyphen_required`, `ac1_header_key_once_sections_repeat`, `ec1_unreadable_or_not_utf8_names_the_file`, `ec8_empty_step_list_refused` |
| L2 | Header keys and places | AC-2, AC-3, AC-4, AC-5, AC-8, AC-9 | `header_tests.cpp`: `ac2_header_keys_and_defaults`, `ac2_preset_name_at_most_18_written_stem_any_length`, `ac2_place_keys_from_the_descriptor`, `ac3_peripheral_fits_place_with_variant`, `ac3_module_by_name_reference`, `ac3_module_by_file_refused_until_lib7`, `ac3_place_not_named_is_empty`, `ac4_keys_repeats_in_file_order`, `ac5_model_from_model_key_not_extension`, `ac8_missing_or_old_version_refused_as_outdated`, `ac8_newer_version_refused`, `ac9_unknown_model_lists_the_models` |
| L3 | Check before anything is built | AC-6, AC-7, AC-30, AC-62, EC-5 | `check_tests.cpp`: `ac6_first_error_in_file_order_with_line`, `ac6_syntax_error_after_semantic_error_reports_the_earlier`, `ac6_refused_load_leaves_machine_untouched_reply_and_notice`, `ac7_keys_text_places_and_files_checked_before_build`, `ac30_unknown_section_or_step_refused`, `ac62_keyword_not_for_model_refused`, `ac62_undefined_keyword_refused`, `ec5_content_without_folder_naming_relative_file_refused` |
| L4 | Building | AC-10, AC-52, AC-55, EC-2, EC-10 | `build_tests.cpp`: `ac10_one_machine_from_cold_boot_with_its_places`, `ac10_old_machine_gone_items_start_at_key_readiness`, `ac52_machine_refusal_reason_and_key_line_old_machine_kept`, `ac55_build_only_ends_without_items_or_readiness`, `ec2_header_only_ends_at_key_readiness`, `ec10_no_key_readiness_fails_before_any_item` |
| L5 | One operation | AC-12, AC-13, AC-16, AC-61, EC-3, EC-4, EC-6 | `operation_tests.cpp`: `ac12_busy_live_input_dropped_second_load_refused`, `ac12_reset_cold_boot_switch_end_it_without_error`, `ac13_in_parts_paced_equals_one_go_harness` (log, reply, test memory, display), `ac16_full_speed_while_built_and_carried_out`, `ac61_speed_command_changes_nothing_while_it_runs`, `ec3_stopped_reply_machine_as_the_command_leaves_it`, `ec4_let_it_finish_without_limit_ended_by_reset`, `ec6_suspended_makes_no_progress` |
| L6 | Order and waits | AC-14, AC-15, AC-56, AC-57, AC-58, AC-59, AC-60 | `waits_tests.cpp`: `ac14_sections_in_file_order_header_anywhere`, `ac15_section_finished_when_written`, `ac56_key_finishes_when_the_program_stops`, `ac56_wait_runs_at_the_speed_in_force`, `ac57_timeout_fails_the_item_at_both_speeds`, `ac57_last_item_and_let_it_finish_not_bounded`, `ac58_skip_wait_next_starts_first_press_at_once`, `ac58_skip_wait_first_or_last_refused`, `ac59_let_it_finish_last_waits_until_idle`, `ac59_let_it_finish_elsewhere_refused`, `ac60_last_item_not_waited_for_regular_speed_after` |
| L7 | Failure and log | AC-17, AC-18 | `waits_tests.cpp`: `ac17_failure_stops_machine_as_is_reply_notice_line`; `log_tests.cpp`: `ac18_log_lines_file_machine_peripherals_items_end`, `ac18_failure_line_holds_the_error` |
| L8 | Paused | AC-19 | `pause_tests.cpp`: `ac19_key_waits_until_running_frontend_step_goes_on`, `ac19_wait_passes_in_host_time_while_paused` (Paced), `ac19_wait_ends_at_once_in_virtual_time`, `ac19_implicit_wait_and_timeout_stand_still_while_paused`, `ac19_preset_that_ends_paused_stays_paused` |
| L9 | The steps | AC-20–28, AC-63, AC-66, EC-7 | `steps_tests.cpp`: `ac20_wait_durations_in_emulated_time`, `ac20_wait_without_unit_refused`, `ac21_wait_without_value_refused`, `ac22_regular_speed_busy_says_so_last_counts`, `ac22_ends_at_regular_speed_whatever_before`, `ac23_elapsed_start_report_format_minutes_beyond_59`, `ac23_report_without_start_counts_from_build`, `ac24_expect_after_item_finished_by_product_comparison`, `ac24_expect_failure_reason_holds_text_and_display`, `ac66_not_expect_passes_and_fails`, `ec7_expect_display_off_fails`, `ac25_screenshot_into_the_folder`, `ac25_screenshot_kept_by_host_without_folder`, `ac25_name_empty_or_with_separator_refused`, `ac25_unwritable_folder_fails`, `ac26_switch_step_refused`, `ac27_syncclock_sets_every_clock_and_logs`, `ac27_syncclock_refused_where_not_applies`, `ac28_key_and_type_finish_at_idle`, `ac63_frontend_step_never_fails_passed_over_without_panel` |
| L10 | The place for others; product rules | AC-29, AC-53, AC-65 | `place_tests.cpp`: `ac29_header_key_section_step_of_a_product_read_checked_carried_out`, `ac29_their_errors_with_line`, `ac53_idle_after_build_picture_from_the_rules`, `ac65_absent_keyword_default_nothing_carried_over` (a test printer's `trace` switch on before, off after a preset that names the printer) |
| L11 | Determinism, elapsed time | AC-11, AC-64 | `determinism_tests.cpp`: `ac11_same_preset_twice_same_memory_display_ticks_log`, `ac64_elapsed_is_compute_holds_gaps_waits` (bound: D-9) |
| L12 | Preset list | AC-31, AC-32, AC-33, EC-9 | `list_tests.cpp`: `ac31_bundle_order_groups_startup_files`, `ac31_check_bundle_finds_missing_and_unlisted_files`, `ac32_entries_with_bundle_path_model_name_description_kind_group`, `ac32_header_only_entry_naming_missing_file_listed`, `ac33_bundle_path_in_load_and_startup_setting`, `ec9_missing_entry_left_out_and_logged` |
| L13 | Harness commands | AC-34 | `library/harness/tests/preset_commands_tests.cpp`: `ac34_preset_done_or_refused_with_line`, `ac34_check_preset_one_line_per_file_run_fails`, `ac34_preset_list`, `ac34_wait_until_idle_reports_time_and_hour`, `ac34_full_and_regular_speed`, `ac34_elapsed_time`, `ac34_expect_and_not_expect_refused_with_display`, `ac34_screenshot_into_the_run_folder`, `ac34_printer_trace_refused_without_printer` (test product step), `ac34_switch_sets_a_control`, `ac34_syncclock_takes_the_run_clock`; `cli_cases.cmake`: `calcu_cli.ac34_check_preset_exit_status` |
| L14 | The suite check | (AC-43, AC-51 mechanism) | `calcu_add_preset_check(LibraryPresetFixtures PRODUCT test FILES tests/fixtures/good/*)`: CTest `calcu_preset_check.LibraryPresetFixtures`, Frequent; a support check, its ACs pass in M-46 and M-54 |
| L15 | LIB-18's start with the real loader | (LIB-18 AC-4, EC-1, EC-6) | `startup_tests.cpp`: `startup_preset_built_by_the_loader`, `startup_preset_outdated_gives_default_and_notice`, `startup_preset_failing_while_run_keeps_machine`; support cases |

Coverage: L1 to L13 cover AC-1 to AC-34, AC-52, AC-53, AC-55 to AC-66
(48 of 48) and EC-1 to EC-10 (10 of 10). Case names follow LIB-1 §2a.

## 7. Tools and agent knowledge

M-38 changes no tool: `pc1500_cli`, `pc1600_cli`, `tools/check_presets.sh`,
`tools/headless/`, `tools/agentic/` read the old formats until M-46 and
M-54 (plan §1). Agent knowledge per step:

| Step | Files |
|---|---|
| M-38 | `docs/test-groups.md` (library rows); `library/harness/python/` README and `calcu --help` usage lines if they list the commands; LIB-11's qa-report rows AC-8 (bundled presets, screenshots), AC-12, AC-21 move to Pass. No product knowledge (the products still read their old formats) |
| M-46 | Calc-U-1600 `CLAUDE.md`, `docs/developer/*.md` on presets, `screenshots/README.md`, the memory files on presets (`project_preset_parameterless_wait` and the like; index `calc-u-1600-memories.md`), the VS Code templates (plan M-46) |
| M-54 | `calc-u-59-statefiles` skill, `reference/StateFileFormat.md`, `prompt/ti59-agent-prompt.md`, memory `state_vs_preset_terminology` (`StateFile` → preset), `docs/glossary-mapping.md` rows `FullSpeed:`, `RegularSpeed:` (plan M-54) |

## 8. Golden runs and risks

- **Golden runs** (M-8) drive cores directly: M-38 touches no core and
  no product, so they cannot differ. The reference runs of M-4 (32 Sharp
  examples) and M-9 (57 TI examples) compare the upgraded presets in
  M-46 and M-54; a difference there is a finding.
- **The YAML reader** is shared with Calc-U-1600's module definitions
  and floppies and Calc-U-59's `.rom.yaml`. The new options are off by
  default; the existing format tests of both products guard it.
- **Frequent budget.** `timeout:` defaults to an hour of emulated time.
  The tests set `timeout: 1` or less, and the test machines at 100 kHz
  and 1 MHz keep each case short. The 3600 s default is checked as a
  value, not run.
- **Pause in virtual time.** An operation on a paused machine never
  ends in `Hosting::VirtualTime` (the run requests give no ticks). M-38
  pauses only Paced tests; LIB-10 (M-40) decides what the harness does
  (LIB-11 AC-9: `wait` ends at once while paused).
- **`std::regex`** (AC-67) adds to the Web engines' size and throws on
  a bad expression; it is caught inside `parseExpectText`.
- **LIB-5 in M-36** removes the products' key sequencers; its design
  keeps `TIKeystrokePlayback` (to M-54) and the Sharp preset typers (to
  M-46), so the old presets keep working until LIB-6's columns.
- **AC-64.** The implicit wait spends one idle stretch (100 ms on a TI,
  20 frames on a Sharp machine) after each waited item, in emulated
  time, so `Elapsed:` includes it (D-9).

## 9. Decided (best guess)

- **D-1 (layout).** `library/presets/`, `calcu::presets`, depending on
  `machine`, `formats`, `settings`, `sessions`, `peripherals`, `typing`,
  `startup`; the harness commands in `library/harness/`. LIB-1 §2a gets
  the dependency line in M-38.
- **D-2 (read = compile).** The reader turns a file into a `LoadPlan` of
  checked items; the engine carries out only a plan. All or nothing
  (AC-6) then holds by construction.
- **D-3 (first error, AC-6).** `format-version` first (README §2), then
  `model` (nothing else can be checked without it), then the lowest line
  of all errors found, syntax errors included: each top-level item is
  parsed on its own. A check that needs every header key (a step that
  needs a printer) runs after the header is read and reports at the
  step's line.
- **D-4 (YAML).** Options on the library reader, not a second reader:
  repeated top-level keys; raw values for the step verbs `type`,
  `expect`, `not-expect`. Numbers by `formats::parseNumber`; the module
  definition's `asInt` is unchanged.
- **D-5 (`not-expect:` raw).** Read raw like `expect:`, which it mirrors
  (AC-66). README §1 names only `type:` and `expect:`.
- **D-6 (where the file is read).** In the `load-preset` handler, before
  `StartOperation`, after the busy check: a refused preset never shows
  busy. A large file is read in one host call.
- **D-7 (markers).** The library holds no product or model names; each
  product's `ModelRules::applies()` carries `preset.md` §7. A keyword of
  the other product that the running program does not register is
  "unknown" (AC-30, AC-62).
- **D-8 (idle).** One library helper, "ready without a break for a
  stretch, asked every `sample` ticks"; the values (100 ms for the TI,
  20 frames for the Sharp models) and the TI's card wait are the
  products'.
- **D-9 (AC-64).** Each waited item adds one idle stretch to the elapsed
  time; the library test bounds `Elapsed:` by compute + holds + gaps +
  `wait:` + one stretch per waited item. The bound against the products'
  examples is B-10's, with the reference runs (M-46, M-54).
- **D-10 (`elapsed-time` baseline).** Without a `start`, from tick 0 of
  the machine the preset built, as 2.3.0's baseline at the reset.
- **D-11 (`timeout:`).** A whole number of seconds, at least 1.
- **D-12 (`preset-name` length).** 18 Unicode code points.
- **D-13 (screenshot).** The product returns PNG bytes; the library
  writes them to `PresetsSetup::screenshotFolder()` (LIB-9's
  `Paths::screenshotFolder()`), or hands them to `KeepFile` when that
  gives none (the Web), and fails when neither is there. The PNG writer
  is one: `formats/Png` (`calcu::formats::PngWriter`), which LIB-8 adds
  in M-39 for its pictures. The products' `screenshot` rules write
  through it from their columns (M-46, M-54); Calc-U-1600's
  `core/Display/PngWriter.hpp` goes in M-46. The library's tests of
  M-38 need no PNG (the test picture is the display as bytes).
- **D-14 (`not-expect:` on a display that is off).** It passes: the
  comparison finds nothing (AC-66, "whatever else the display shows").
  `expect:` fails with "the display is off" (EC-7).
- **D-15 (one vocabulary).** The harness commands of the steps go
  through `run-step`, so a command and a step have one reader and one
  meaning; `full-speed`/`regular-speed` are LIB-3's `speed`.
- **D-16 (`switch` command).** In LIB-2's `PeripheralsPart`
  (`set-control {owner, control, on}`), next to the controls it uses. It
  sets a switch and presses or releases a key, so LIB-8's paper panel
  (M-39) sends the same command for the printer's controls.
- **D-17 (wall time in an operation).** `OperationContext::hostWait`,
  added to `sessions` in M-38, is the only wall time a preset uses
  (`wait:` while paused); at once in the harness.
- **D-18 (regex).** Parsed and compiled when read, in the library, for
  both products; a product refuses a text by `checkExpect()` (P-6). AC-67's tests
  are in M-46 with the Sharp comparison.
- **D-19 (speed at the end).** Regular also after a failure: "the
  machine stays as it is" (AC-17) is its memory and display; the speed
  is AC-60's.
- **D-20 (suite check).** `calcu_add_preset_check()`, one CTest entry
  per list, run through `calcu check-preset`; `EXCLUDE` names the
  waiting presets (AC-43, AC-51), `FILL` a script that fills
  placeholders. Calc-U-1600's `CalcU1600CheckPresets` stays until M-46.
- **D-21 (preset list).** The order of the bundle file; only header keys
  read per entry.
- **D-22 (place values).** A scalar `<ID>[:<variant>]`, or `{ name: <ID> }`
  matched to a peripheral ID without regard to case; `{ file: … }` is
  refused with a notice until LIB-7 registers its resolver (M-41). A
  product narrows a place by `placeKey()`. A product key with a form of
  its own (`host-drive: { folder: }`) is the product's header key.
- **D-23 (log wording).** Not part of the contract (AC-18); e.g.
  `Preset <name>: <model>[:<variant>]`, `  slot-1: ML`,
  `  line 12 key: 2nd Pgm 01: done`, `Preset carried out to its end.`
- **D-24 (user or startup load).** LIB-6 passes `BuildOrigin::StartupPreset`
  (LIB-18 §3.1, M-35) for a load through `PresetLoader::load` and
  `Preset` for a user's `load-preset`, so that LIB-8 can empty the paper
  or keep it (AC-41) without reading presets.
- **D-25 (test machines).** `TestSegProgram` on `test-seg`, in the
  pattern of `TestSegTiming`: a program the tests can start and stop.
- **D-26 (`description`).** A one-line scalar; a block scalar is
  refused.
- **D-27 (sections and pause).** Every section waits for a running
  machine (AC-19, "a section that writes memory"); a product section
  that does not write memory can say otherwise.
- **D-28 (byte-order mark).** A UTF-8 BOM at the start is dropped,
  as editors on Windows write one.
- **D-29 (Done in).** Clauses of the Library criteria pass in later
  steps (§1): AC-19's debugger steps and AC-63's `debug-panel:` in M-40,
  AC-3's module by `file:` in M-41. By plan §1 ("Completes" and Done)
  the Library column is then Done in M-41, not M-38 (proposal to the
  user for plan §5.1). The products' clauses (AC-24, AC-25, AC-31, AC-34,
  AC-53) are tested again in M-46 and M-54 with the product rows.
- **Open point (not a fact this design may invent):** the place where
  each product's preset format is described for the outdated notice
  (AC-8) is `ProductRules::formatReference()`; its text (a URL of the
  help, M-52 / M-60) is the product's and is set in M-46 and M-54.

## 10. Part D: the Calc-U-1600 column

Draft for the Part C2/D boundary review (2026-11-16). Paths without a
prefix are under `products/calc-u-1600/`; line numbers at `60e5b09`.

### 10.1 Scope

| Column | Step | Tests from (plan §3) | Done in |
|---|---|---|---|
| Calc-U-1600 | M-46, with C1600-4 (`features/C1600-4-sharp-preset-content/design.md`) and LIB-18 1600 (§10 of its design) | AC-44–51, AC-54, AC-67 | M-46 (plan §5.1) |

- Clauses of the Library column that are a product's and pass here
  (§1's table): AC-24 and AC-48 the product's comparison; AC-53 the
  idle, the after-build and the picture of the Sharp models; AC-25 the
  save folder in Qt6 and the download on the Web; AC-31 "every
  distribution is built from it"; AC-34 through `calcu --product 1600`.
- Clauses of other columns that need presets of version 2 and pass
  here: LIB-5 1600 AC-14, AC-28, AC-41 "in a preset" (LIB-5 D-1; LIB-5
  1600 becomes Done here); LIB-15 1600 AC-16's bundle-file clause
  (LIB-15 D-4; LIB-15 1600 Done here); the LIB-8 1600 and LIB-10 1600
  criteria that name upgraded presets (their M-45 designs).
- The presets that wait for M-47 and the rule: C1600-4 design §1.
  AC-51 passes in M-46 with them left out by name, as it says.

**Where the code built in C2 differs from §2–§9** (the record is the
qa-report's "Deviations" of M-38 and M-41): `LoadPlan::items` are
`PlannedItem`s and `keys:` is the reader's own section; `readStep` and
`PresetSource::readError()` are new; `ReadContext` has `kept()`,
`pluggedAny()` and `typingProfile()`; `PlaceKeyRule::byFile` is on by
default and LIB-7's resolver reads `{ file: }`; `HarnessProduct::typingPart()`
hands the loader LIB-5's part; `expect:` and `screenshot:` read the
display from the machine at the step. This column builds on those
shapes. The library's `machinePlan` (`library/presets/Engine.cpp:26-32`)
turns only place keys into requests, and the reader takes a key under a
place's name as a place key (`library/presets/Reader.cpp:462-472`);
§10.3 adds what C1600-4 needs (D-31 to D-33).

### 10.2 Modules and files

**New**

| File | Holds |
|---|---|
| `app/SharpPresetRules.hpp/.cpp` | `shared::SharpPresetRules : presets::ProductRules` and one `SharpModelRules : ModelRules` per model (§10.3); `productName()` "Calc-U-1600", `formatReference()` (D-30); `addKeywords` calls C1600-4's `addSharpKeywords` |
| `app/SharpDisplayLines.hpp/.cpp` | `shared::displayLines(const DisplayOutput&, const MachineDescriptor&, const SharpFont&)`: the display as text by LIB-11 AC-10 and AC-19, one function for `expect:` and the harness's `readDisplay` (§3.4: "one function in the product") |
| `tools/CliPresetRun.hpp/.cpp` | the adapter of the CLIs (§10.5) |
| `tests/sharp_presets_tests.cpp` | AC-44–50, AC-67 and the LIB-5 clauses (§10.6) |

**Changed**

| File | Change |
|---|---|
| `app/SharpSession.hpp/.cpp` | `shared::addLibraryParts(AppSessionSetup&, LibraryPartsSetup)`: `PeripheralsPart`, `MachineStatePart`, `TypingPart`, `PresetsPart`, `StartupPart` (LIB-18 §10), in that order, then `SharpAppPart`; used by `makeSharpSession` and `frontends/web/bindings.cpp:35-60` |
| `app/SharpAppPart.hpp/.cpp` | `loadPresetJob` (367-420), `loadDefaultPresetJob` (422-432), `defaultPresetJob` (453-458), `resetBareForPreset` (651-663), `finishPresetLoad` (665-669) and the commands `c1600.load-preset`, `c1600.load-default-preset`, `c1600.default-preset` (`app/SharpTopics.hpp:35-50`; registered 220-233) go: LIB-6's `load-preset` and LIB-18's start and model switch replace them. The bays (until M-47, LIB-7 A-1) bind in `machineChanged` to what a preset built (§10.5) |
| `app/AppPaths.hpp/.cpp` | `resolvePresetRef` (46; `.cpp` 55-59) and `kExamplePresetPrefix` (32) go: `presets::sourceFor` and `parseBundlePath`; `bundledExamplesDir` (41) stays as the folder of the `DirectoryPresetBundle` |
| `app/PresetCatalog.*` | deleted: the preset list is `preset-list` (AC-31, AC-32); `presetNameProblems` into `checkBundle` (C1600-4 AC-18) |
| `app/SharpWebPart.cpp` | the request `c1600.preset-examples` (382) and the command `c1600.set-default-preset` (391) go: the page asks `preset-list`; the startup preset is LIB-9's key `[preset] <model>`, set with LIB-9's settings command; its choices are the `Startup` entries of `preset-list` |
| `app/HarnessProduct.cpp` | `presetRules()` and `presetBundle()` (`library/harness/Product.hpp:81-82`); `readDisplay` (112-116) calls `displayLines` |
| `frontends/qt6/app/MainWindow.cpp` | Load Preset… (162-167) and a dropped preset (809) send `load-preset {ref}` (AC-54: a file in its folder); the Examples menu (845-870) from `preset-list`, each item `examples:<path>`; `loadPresetForShots` (834-842) the same command |
| `frontends/qt6/app/Operations.cpp:64-66` | `loadPreset` sends `load-preset` |
| `frontends/qt6/app/SettingsDialog.cpp:164-177` | the startup-preset pickers from `preset-list`; "Default presets" reads "Startup presets" (`docs/glossary-mapping.md`, row "Startup preset"); applying a changed choice now sends `load-preset {ref, cause: startup}` |
| `frontends/qt6/app/debug/DebugController.cpp:193-235` | `loadPreset` (armOnly) → `load-preset {ref, buildOnly: true}` (AC-55); `cleanStart` → `load-preset`, or LIB-18's `cold-boot` |
| `frontends/qt6/app/debug/DapSession.cpp:212-248` | the `debug:` block through `shared::debugBlockOf` (C1600-4 §3.4) |
| `frontends/qt6/app/screenshots/ShotScenario.cpp:142, 263`, `ShotRunner.cpp:234` | the scenario's `preset:` step sends `load-preset` |
| `frontends/web/src/engine.js:61-137`, `src/CalcU1600.jsx:80-82, 138-147, 244-268` | the Examples menu from `preset-list`; a bundled example by `load-preset {ref: "examples:<path>"}`; an uploaded preset by `load-preset {content, name}` (AC-54, EC-5); `?preset=` as LIB-15 AC-16's bundle path |
| `frontends/qt6/CMakeLists.txt:214-252`, `frontends/web/build.js:60-79` | bundle the files of `bundle.yaml` (C1600-4 §4) |
| `CMakeLists.txt:137-138` | CTest `CalcU1600CheckPresets` goes; `calcu_add_preset_check(CalcU1600Presets PRODUCT 1600 FILES … EXCLUDE … FILL tests/fill_vscode_templates.cmake)` |
| `tools/pc1500_cli.cpp:150-240`, `tools/pc1600_cli.cpp:82-258`, `tools/pc1600_plotter_probe.cpp:24-73`, `dev/loader-matrix/LoaderMatrixCommon.hpp:86-112`, `tools/wasm_smoke.cpp:70-73` | presets through `CliPresetRun` (§10.5); `--check-preset` sends `check-preset` |
| `library/presets/Reader.cpp:462-472` | a product header key under a place's name is read as the header key (D-31) |
| `library/presets/ProductRules.hpp:67-84`, `Reader.cpp`, `Engine.cpp:26-32` | `ModelRules::checkHeader(ReadContext&) const`, called once every header key is read (D-32), and (P-6, D-33) `ReadContext::addPeripheral` / `setContentAt`, the header key's requests in `LoadPlan::headerPeripherals`, after the places in `machinePlan`; empty by default, so no other product and no library test changes |

**Deleted** (the reader of version 1; C1600-4 §2 names its own parts):
`core/Preset/PresetFile.hpp/.cpp`, `PresetRunner.hpp/.cpp`,
`PresetRoms.hpp/.cpp` (its `presetroms::` helpers go to LIB-18's
`SharpProductMachines` and the builder), `core/PC1500/PC1500PresetLoader.*`,
`core/PC1600/PC1600PresetLoader.*`, `core/Display/PngWriter.hpp` (D-13:
`calcu::formats::PngWriter`), `core/HostClock.hpp` (C1600-1 qa-report:
"stays until M-46"), `JobForward` (LIB-3 §5; LIB-5 D-2), the preset
parts of the Sharp typers (`PC1500BasicTyper::settleUntilIdle`,
`PC1600BasicTyper::waitUntilBasicIdle`, `tapKeyJob`, `typeLineJob`;
what C1600-2 kept of them for typing a program in stays C1600-2's),
`tests/PresetTestSupport.hpp`, `tools/check_presets.sh`.
`core/DropFile.cpp:15, 28` keeps `opening::looksLikePreset`.

### 10.3 The product's rules (AC-44–48, AC-53, AC-62, AC-67)

```cpp
namespace shared {
class SharpModelRules final : public calcu::presets::ModelRules {
public:
    IdleRule idle() const override;                                  // AC-45: isKeyReady() for 20 frames, asked every frame
    Task<ItemResult> afterBuild(RunContext&) override;               // AC-46
    std::optional<std::vector<std::uint8_t>> screenshot(Machine&, const DisplayOutput&) override;   // AC-48
    ExpectReading compare(Machine&, const DisplayOutput&, const ExpectText&) override;          // AC-48, AC-67
    Applies applies(std::string_view keyword) const override;        // preset.md §7, the three Sharp columns
    void checkHeader(ReadContext&) const override;                   // AC-47 (D-32)
};
class SharpPresetRules final : public calcu::presets::ProductRules { /* a SharpModelRules per model; addKeywords */ };
}
```

- **AC-44.** The extensions and models are the catalogue's (`.pc1500`,
  `.pc1500a`, `.pc1600`; the variants of `core/Machine/SharpCatalogue.cpp`);
  the model comes from `model:` only (AC-5). The place keys `slot-1`,
  `slot-2`, `printer`, `interface` are the descriptors' (C1600-1);
  `host-drive` is C1600-4's header key (D-31). `plotter:` is no keyword:
  "Unknown key plotter." (row 1600-3). `ctrl`, `kbii` and `bs` are
  refused by the typing profile's `name` (LIB-5 AC-41, `app/SharpTyping.cpp`).
- **AC-45, AC-53 idle.** `readyNow` is `Machine::isKeyReady()`;
  `stretch` 20 frames and `sample` one frame, frames as the cores count
  them (`kPC1500CyclesPerFrame`; LIB-5 qa-report, deviations). First
  step C-2 (M-4) confirmed the RESERVE mode and MODE 1.
- **AC-46 after the build.** On the PC-1500 and the PC-1500A: while the
  display text shows `NEW0? :CHECK`, `typeAndWait(cl)` and the idle
  wait, at most five times (today `core/PC1500/PC1500PresetLoader.cpp:156-168`);
  the PC-1600 presses nothing.
- **AC-47.** A CE-1600P on a PC-1500 does not fit the place (the
  reader's own check); a CE-158 with a CE-1600P is refused by
  `checkHeader` with row 1600-22's sentence, at the line of the later
  key, naming both; `slot-2:` on a PC-1500 is "not available" (AC-62).
  A `floppy:` step without a CE-1600P is the check of LIB-7's step
  (M-47; `applies` and `ReadContext::plugged`).
- **AC-48 picture and text.** `screenshot` is Copy Screen's picture
  (`core/Display/LcdScreenshot.*`, LIB-14 AC-41) written by
  `formats::PngWriter`, on the PC-1600 the graphics area without the
  indicator row. `compare` takes `displayLines` and finds the text in
  one line, case and spaces as written; `\\` and `\xHH` are decoded by
  the library's `parseExpectText`; AC-67's expression is searched in
  each line (`std::regex_search`, ECMAScript).
- **Screenshots** go to LIB-9's `Paths::screenshotFolder()` (the save
  folder in Qt6; row 1600-30), on the Web to `KeepFile` (a download).

### 10.4 Frontends (AC-49, AC-54)

- **AC-54.** Qt6 hands a file in its folder (`load-preset {ref: path}`,
  `folderSource`); the Web hands a bundled example by its bundle path
  and an uploaded file by its content (`contentSource`; EC-5: a relative
  file it names is refused). Nothing else of a load is in a frontend.
- **AC-49.** The log lines are on the `log` topic: Qt6 writes that topic
  to the platform log already (`frontends/qt6/app/MachineController.cpp:181`);
  the Web keeps it in its `log` stream (`frontends/web/src/engine.js:126, 175`).
  Where the log is shown is LIB-14's.
- **The start** and the startup presets are LIB-18's (§10 of its
  design); `StartupPart` calls this part's `PresetLoader::load`.

### 10.5 Adapters

| Adapter | File | Wraps | Removed in |
|---|---|---|---|
| `CliPresetRun` (new) | `tools/CliPresetRun.hpp/.cpp` | for `pc1500_cli`, `pc1600_cli`, `pc1600_plotter_probe`, the loader matrices and `wasm_smoke`: an app session in `Hosting::VirtualTime` with `addLibraryParts`, `start {preset}` run to its end, then the machine handed back for the tool's own dumps (`LegacyCalls::coreParts`) | M-51 (the CLIs and probes go) |
| bays on `machineChanged` | `app/SharpAppPart.cpp` (`ModuleBay::syncFromPresetLoad` 155-165, `FloppyBay::syncFromPresetLoad` 93-103 called from `machineChanged` 307-313) | the bays read the slots of a machine that a preset or LIB-18 built, as templates where named by name | M-47 (LIB-7 A-1) |

Removed in M-46: `LegacyCalls::applyPresetJob` and `syncPlugged`
(`core/Machine/LegacyCalls.hpp:49-59`); `JobForward`; the `c1600.*`
preset commands (LIB-3 §5; the adapter of §5 above); `CalcU1600CheckPresets`. The CLIs'
`saveas:` (`LegacyCalls::presetMedia`, `core/Preset/PresetMedia.hpp`)
waits with the presets that use it (M-47, LIB-7 A-3).

### 10.6 Tasks

Tests come before the code. `CoreTests` (Frequent) unless the row says
otherwise; ROM-gated cases `SKIP_UNLESS` the ROMs. The cases run the
product's harness entry in virtual time, or `presets::readPreset` with
`SharpPresetRules`. Names `lib6_acN_…`, `lib5_acN_…`, `lib15_acN_…`.

| Task | What | ACs | Test (file: cases, group) |
|---|---|---|---|
| P1 | Models, places, keys | AC-44 | `sharp_presets_tests.cpp`: `lib6_ac44_extensions_models_variants_any_case`, `lib6_ac44_place_keys_per_model`, `lib6_ac44_plotter_key_unknown`, `lib6_ac44_floppy_and_tape_not_header_keys`, `lib6_ac44_ctrl_kbii_bs_refused_in_key` |
| P2 | Idle | AC-45, AC-53 | `lib6_ac45_idle_at_prompt_input_and_reserve`, `lib6_ac45_running_program_not_idle`, `lib6_ac53_sharp_idle_rule_values` |
| P3 | After the build | AC-46 | `lib6_ac46_cl_at_new0_check_at_most_five_pc1500a`, `lib6_ac46_pc1600_presses_nothing` |
| P4 | Hardware together | AC-47 | `lib6_ac47_ce1600p_on_pc1500_refused`, `lib6_ac47_ce158_with_ce1600p_names_both`, `lib6_ac47_slot2_on_pc1500_not_available` (the `floppy:` clause: LIB-7 §10.6, M-47) |
| P5 | Picture and text | AC-48, AC-67, AC-24, AC-25 | `lib6_ac48_screenshot_is_copy_screen_picture` (byte-equal with Copy Screen's PNG; the PC-1600 without the indicator row), `lib6_ac48_expect_one_line_case_and_spaces`, `lib6_ac48_backslash_and_xhh`, `lib6_ac67_regex_error_1_not_error_10`, `lib6_ac67_bad_regex_refused_at_read`, `lib6_ac67_leading_slash_escaped_literal`, `lib6_ac25_screenshot_into_save_folder` |
| P6 | Log | AC-49 | `lib6_ac49_log_lines_on_log_topic` (`app/tests/SessionFixture.hpp`); Web `session.spec.js` `lib6_ac49 preset log in the log stream` (Longer) |
| P7 | Today's tests | AC-50 | the cases of AC-50's list ported onto the library's reader and steps as `lib6_ac50_<old name>` (`preset_tests.cpp`, `lcd_text_tests.cpp`, `lcd_screenshot_tests.cpp`; `job_tests.cpp:217` as "in parts against in one go"); the old cases go with the old reader |
| P8 | The suite check | AC-51 | `calcu_preset_check.CalcU1600Presets` (Frequent): every preset of C1600-4 §2's table, the VS Code templates filled by `tests/fill_vscode_templates.cmake` (the rule of `vscode/calcu1600-debug/scaffold.js:58`, C1600-3 AC-19), `EXCLUDE` the five waiting presets; `checkBundle` over `examples/bundle.yaml` in the same entry |
| P9 | Frontends | AC-54 | Qt6: a screenshot scenario `lib6-ac54-example-loads` run as a check (`CalcU1600Shots.lib6-ac54-example-loads`, Longer: `examples:plotter/lissajou-1600.pc1600`, then the display text and the paper); Web: `play.spec.js:208` renamed `lib6_ac54 bundled example loads and its result shows`, and `lib6_ac54 uploaded preset by its content` (Longer) |
| P10 | Typing in a preset | LIB-5 AC-14, AC-28, AC-41 | `lib5_ac14_in_a_preset_type_at_readiness_pc1600_at_once_pc1500`, `lib5_ac28_in_a_preset_cadence`, `lib5_ac41_ent_refused_in_a_preset` |
| P11 | Bundle path on the Web | LIB-15 AC-16 | `embed.spec.js` `lib15_ac16 preset prop takes a path of bundle.yaml` (Longer) |
| P12 | Harness | AC-34 | `harness_tests.cpp`: `lib6_ac34_preset_wait_expect_screenshot_on_pc1600` (`calcu --product 1600 preset examples:basic/hanoi.pc1600 then wait then expect …`) |

Coverage: P1–P9 cover AC-44 to AC-51, AC-54 and AC-67.

### 10.7 Tools and agent knowledge (M-46)

| File | Change |
|---|---|
| `products/calc-u-1600/CLAUDE.md` | the Harness section: presets are read ("Not yet: … presets" goes; `preset`, `check-preset`, `preset-list`, `wait` without a duration, `expect`, `screenshot` listed); "until M-51, `pc1500_cli` … stay the tools for" memory dumps and media only; the `LegacyCalls` sentence without the preset adapters |
| `products/calc-u-1600/docs/developer/Shared-Architecture.md`, `Web-Client.md`, `Building.md` | the preset path (`load-preset`, `SharpPresetRules`, the bundle file); `check_presets.sh` → `calcu_preset_check.CalcU1600Presets` |
| `products/calc-u-1600/docs/developer/screenshots/README.md` | presets of version 2; the `preset:` step sends `load-preset` |
| `products/calc-u-1600/docs/background/Decisions.md` | a row on a bare `- wait:`, where there is one, points to `let-it-finish:` and the implicit wait |
| `docs/test-groups.md` | `CalcU1600CheckPresets` out; `calcu_preset_check.CalcU1600Presets`, `CalcU1600ExampleReferences`, `CalcU1600BundledFiles` in; the new `CoreTests` files |
| `docs/glossary-mapping.md` | row "Startup preset": "Default presets" → "Startup presets" (visible, Qt6 and Web); row "Preset, preset format": `PresetFile` → `calcu::presets`; `bundled.yaml` → `bundle.yaml`; `PresetCatalog` → the preset list |
| memory `calc-u-1600/project_preset_parameterless_wait.md` | rewritten: the bare `- wait:` is gone; every item but the last waits until idle (20 frames of key readiness), `let-it-finish:` waits for the last; the PC-1600's `$92xx` rule is no longer used (key readiness, C1600-1 AC-21) |
| memory `calc-u-1600/feedback_fix_loader_not_presets.md` | "the Core loader" → the product's rules and the library's engine; the rule stays (fix the rules, not the presets) |
| memory `calc-u-1600/project_scripted_screenshots.md` | the presets are version 2 |
| memory `calc-u-1600/feedback_paste_vs_inbound_typing.md` | step 1 "clean start from a preset" is `load-preset`, or `cold-boot` |
| `calc-u-1600-memories.md` (index) | the descriptions of the four entries above |

**Ledger rows that become visible in M-46** (with C1600-4 and LIB-18
1600): 1600-1 (presets of version 1 refused with the hint), 1600-3
(the syntax), 1600-11 (checked before the machine is given up),
1600-21 (only the bundle file's files are bundled; its examples zip is
M-61's), 1600-30 (waits, regular speed afterwards, screenshots in the
save folder, `not-expect:` and expressions). No new row: "Default
presets" → "Startup presets" is a visible rename of
`docs/glossary-mapping.md`.

### 10.8 Golden and reference runs, risks

- **Golden runs** (`tests/golden-runs/`, `golden_runs_tests.cpp`) build
  and drive the machine directly; this column changes no path they use.
  LIB-18 §10.8 names what its column changes for them.
- **Reference runs.** C1600-4 AC-16 (its design §6 T8, §8) is the check
  of this column as well: the 28 examples upgraded in M-46 (32 less the
  four examples that wait) go through this loader.
- **Waits.** Every item now waits until idle (row 1600-30); a `type:`
  that version 1 left after one second now waits for its command, so
  a startup preset with a long `INIT` takes longer. Measured with the
  reference runs; intended (conversion check §4).
- **`std::regex` in the Web engine** (§8) is used by a product for the
  first time; `tools/build_wasm_check.sh` and the Web build show the size.
- **`CliPresetRun`** gives the CLIs the library's load, so a CLI run of
  a preset waits as the GUI does; `dev/tape-matrix/run.py` and the loader
  matrices are run once in the step (memory one-time verification).

### 10.9 Decided (best guess)

- **D-30 (`formatReference`).** "the preset format in the User Guide,
  chapter Presets" until M-52 gives the help topic's address. Settles
  §9's open point for Calc-U-1600.
- **D-31 (a header key under a place's name).** The reader asks the
  vocabulary first, then the descriptor's places; so `host-drive:` is
  C1600-4's header key (D-22 already gives such a key to the product).
  No other key of either product stands under a place's name.
- **D-32 (`checkHeader`).** One product check after every header key is
  read, for AC-47's pairs, so that the error names both keys at the
  line of the later one.
- **D-33 (header keys into the plan; P-6).** A product's header key puts
  its requests into the plan as it is read: `ReadContext::addPeripheral`
  (host drive, bus ROMs; into `LoadPlan::headerPeripherals`, which
  `machinePlan` puts after the places, in the order of the model's places
  and, within a place, as added) and `ReadContext::setContentAt` (a slot
  ROM: the content of the module's request unless its reference gave one);
  the reader stays the only writer of a `LoadPlan`. Before P-6 the
  product kept its values and turned them into requests in
  `ModelRules::amendPlan` with a cast per value; the order of the requests
  is the same (C1600-4 D-2).
- **D-34 (waiting presets).** AC-51's rule decides which presets wait,
  not only the three names in its parenthesis (C1600-4 D-3, Plan note
  P-1 there).
- **D-35 (tools).** The CLIs reach the loader through one adapter that
  hosts the product's session in virtual time, not through a copy of
  the engine; their `saveas:` stays old until M-47.

### 10.9a Part D simplification P-6 (2026-11-16, branch `ds-names`)

Departures from the text above (`docs/reviews/d-simplification.md` S-8;
no user-visible change, every sentence as before):

- `ModelRules::takesRegex()` is `checkExpect(const ExpectText&,
  std::string_view keyword)`: the rules return the refusal sentence of an
  `expect:` / `not-expect:` text when the preset is read, so a TI model
  can refuse what is not a number (AC-40, AC-68) when the TI rules come
  (Part E). Calc-U-1600's rules take every text, as before; the library's
  test rules refuse what a test asks (`ac68_expect_text_refused_by_the_model_rules_when_read`).
- `ModelRules::amendPlan` is gone (D-33 above): `ReadContext::addPeripheral`
  and `setContentAt`, `LoadPlan::headerPeripherals`.
- Key names to presses are `typing::pressesOfNames` (LIB-5 design, P-6);
  the `key:` step keeps its two sentences.
- Model, peripheral and variant names in any case are
  `machine::modelNamed`, `peripheralNamed`, `variantNamed` and
  `machine::lowerAscii` (`machine/Names.hpp`); `presets::lowerAscii` is
  that function under its old name (library/debugger uses it). It lowers
  A to Z only; the copies before used `std::tolower`, which is the same
  for ASCII and under the "C" locale, and could change the bytes of a
  UTF-8 name under a frontend's locale.
- `scalarText`: the reader's and the bundle file's own copies went for
  `formats::scalarText`; they differed only for a node that is not a
  scalar, whose `scalar` the YAML reader leaves empty, so both give "".
  `LibraryKeywords.cpp` keeps `rawOrScalarText` for the raw steps (D-4:
  the text as written, quotes and `#` included).
- `PresetsPart::load` and `loadWithin` read and check the model in one
  private `readFor`.
- `HeaderKeyDef::afterOtherHeaderKeys` (§3.5) had no user and was already
  gone from the code; the text above no longer names it.

### 10.10 Questions for the user

1. **The golden driver's key timing** (finding of M-36, LIB-5
   qa-report). The Sharp golden driver taps keys with 0.8.1's queue
   timing (one instruction before the press, the PC-1500's readiness
   wait off). With the real PC-1500 typing profile (LIB-5 AC-31),
   `pc1500a-ce150` differs in two cycle counts and one audio-sample
   count; the display, the paper and the memory are equal. This design
   keeps the driver as it is. *Keep the driver on 0.8.1's timing
   (recommended), or move it to the real profile and record
   `pc1500a-ce150` anew as the accepted difference?*
   Answered (user, 2026-11-16, with the approval of the Part D designs): the driver
   stays on 0.8.1's timing; no reference is recorded anew.

## 11. Part E: the Calc-U-59 column

Approved by the user at the Part D/E boundary (2026-11-16). Paths without a prefix are
under `products/calc-u-59/`; line numbers at `7bdd51c`.

### 11.1 Scope

| Column | Step | Tests from (plan §3) | Done in |
|---|---|---|---|
| Calc-U-59 | M-54, with C59-2 (`features/C59-2-ti-preset-content/design.md`), LIB-18 59 (§11 of its design) and C59-3 (`features/C59-3-ti-58c-constant-memory/design.md`) | AC-35–43, AC-68 | M-58 by the rule of plan §1 (D-36): AC-42's GUI scenario needs the Examples menu of LIB-14 59 (Qt6, M-57) and C59-8 (Swift, M-58); plan §5.1 says M-54 |

- Clauses of the Library column that are a product's and pass here (§1's
  table): AC-24, AC-40 and AC-66 the TI comparison; AC-53 the TI idle, no
  after-build step and the segment picture; AC-25 the save folder in
  Qt6 and Swift (Swift: the iCloud Drive folder, OQ-4) and the download
  on the Web; AC-31 "every distribution is built from it"; AC-34 through
  `calcu --product 59`, `printer-trace` included.
- Clauses of other columns that need presets of version 2 and pass here:
  LIB-5 59 AC-17 and AC-26 "in a preset" (LIB-5 §10); LIB-15 59 AC-16's
  bundle path and AC-20's constant memory clause (LIB-15 59 becomes Done
  in M-55 with the cards, plan §3 M-34); the LIB-8 59 and LIB-10 59
  criteria that name upgraded presets (their M-53 designs).
- The presets that wait for M-55 and the rule: C59-2 design §1 (the four
  card examples, named by AC-10). AC-43 passes in M-54 with them left out
  by name, as it says.
- The code built in C2 and D differs from §2–§9 as the qa-report's
  "Deviations" say (M-38, M-41, M-46) and as §10.9a says (P-6): `LoadPlan`
  holds `PlannedItem`s; `ModelRules::checkExpect` refuses a text when the
  preset is read (`library/presets/ProductRules.hpp:80-86`);
  `ReadContext::addPeripheral`; the library's parts are built and ordered
  by `parts::LibraryParts` (`library/parts/LibraryParts.hpp:51-134`),
  which a product fills through `LibraryPartsSetup` (`presetRules`,
  `bundle`, `home`, `screenshotFolder`). This column builds on those
  shapes and adds nothing to `library/presets/` (D-37).

### 11.2 Modules and files

**New**

| File | Holds |
|---|---|
| `app/TIPresetRules.hpp/.cpp` | `cu59::TIPresetRules : presets::ProductRules` and one `TIModelRules : ModelRules` per model (§11.3); `productName()` "Calc-U-59", `formatReference()` (D-38); `addKeywords` calls C59-2's `addTIKeywords` |
| `app/TIDisplayPicture.hpp/.cpp` | `cu59::displayPicture(const DisplayOutput&, const MachineDescriptor&) -> std::vector<std::uint8_t>`: the twelve positions and the indicator in the classic segment style (`app/LedGeometry.hpp`, `LedStyle::Classic`; `app/SevenSegment.hpp`), written by `formats::PngWriter` (`library/formats/Png.hpp:15`) (C59-2 AC-8; OQ-3; D-39) |
| `tests/ti_presets_tests.cpp` | AC-35–43, AC-68 and the LIB-5 clauses (§11.6) |
| `tests/reference/display/` | the reference pictures of AC-40's `screenshot:` (one per model; D-39) |

**Changed**

| File | Change |
|---|---|
| `app/TIAppPart.hpp/.cpp` | `TIAppPart` owns a `parts::LibraryParts` built from `librarySetup()` (as Calc-U-1600's `app/SharpAppPart.cpp:103-146`): the catalogue, `TIProductMachines` (LIB-18 §11), the settings, the ROM set, `typingProduct()`, the rules of §11.3, the bundle (`DirectoryPresetBundle` over the bundled `examples/`), `home`, `screenshotFolder` (LIB-9 `Paths::screenshotFolder()`), and what M-53 adds (papers, debugger); `sessionParts()` gives them in the library's order with the part last (D-40). The command `c59.load-state` (`app/TIAppPart.hpp:225`, `app/TIAppPart.cpp:168-172`) goes: LIB-6's `load-preset` replaces it. `c59.open-dropped` (`:173`) hands a preset to `load-preset {content, name}` until LIB-13 59's `open` (M-56). `presetOperation`, `untilIdle(true)` and `runUntilIdle`'s preset branch (`app/TIAppPart.cpp:237-261`) go. `makeAppSession` (`:424-449`) builds the session from `sessionParts()` (its own `TypingPart` goes: it is among the library's parts) |
| `app/TIAppPartFlows.cpp` | `loadStateFile` (565-595), `applyState` (620-654), `applyControl` (656-694), `applyReloadStage` (696-715), `dropPresetCards`, the `KeystrokeEvent` controls and the preset half of `openDroppedFile` (605-619) go; C59-2 and C59-3 name what replaces the memory writes and the constant memory calls |
| `app/TIAppPartInternal.hpp` | `Hooks::act`'s call of `EmulatorSession::sliceEnded` (`app/EmulatorSession.hpp:189`) goes with `TIKeystrokePlayback` (LIB-5 §10) |
| `app/EmulatorSession.hpp/.cpp` | `playKeystrokes`, `cancelKeystrokes`, `keystrokesPending`, `sliceEnded`, the `TIKeystrokePlayback::Host` base and the `Listener` calls `playbackStarted`, `playbackFinished`, `reloadRequested`, `controlRequested` (`app/EmulatorSession.hpp:95-112, 189-231`) go |
| `app/TIWebPart.hpp/.cpp` | the preset half of `start` (`app/TIWebPart.cpp:113-135`) goes to LIB-18's `start {model, preset}` with the preset as a bundle path `examples:<path>` (LIB-15 AC-16); `m_presetsDir` goes; `c59.web.drop` (199-213) hands a preset to `load-preset {content, name}` |
| `app/HarnessProduct.cpp` | `prepare` (66-75) sets `presetRules` and `bundle` in the `LibraryPartsSetup` (a `DirectoryPresetBundle` over the source tree's `examples/`); `HarnessMachines` goes (LIB-18 §11) |
| `frontends/web/bindings.cpp:31-79` | the session from `TIAppPart::sessionParts()`, `start.shellPart` first; the bundle is the preloaded `examples/` of `dist/` |
| `frontends/web/src/engine.js:225`, `src/Dialogs.jsx:113-117` | the preset list from the request `preset-list` (the `Example` entries in their groups); a bundled example by `load-preset {ref: "examples:<path>"}`; an uploaded or dropped preset by `load-preset {content, name}` (AC-42, EC-5) |
| `frontends/web/build.js:66-71`, `frontends/web/presets.json` (deleted) | the files of `examples/bundle.yaml` into `dist/examples/` (C59-2 §4) |
| `frontends/qt6/app/EmulatorController.cpp:260-262`, `MainWindow.cpp:87` | Open Preset… and a dropped preset send `load-preset {ref: path}` (AC-42: a file in its folder) |
| `frontends/qt6/CMakeLists.txt` | the files of `bundle.yaml` beside the program, the folder of the `DirectoryPresetBundle` (as `products/calc-u-1600/frontends/qt6/CMakeLists.txt:219-252`) |
| `frontends/swift/App/EmulatorViewModel.swift:394-398`, `Bridge/TIAppSession.mm:581` | `loadStateFile` sends `load-preset {content, name}` (D-41); a build phase copies the files of `bundle.yaml` into the app's `examples/` (C59-2 §4) |
| `CMakeLists.txt` (the product's) | `calcu_add_preset_check(CalcU59Presets PRODUCT 59 FILES … EXCLUDE …)` (AC-43; D-20); `CalcU59BundledFiles` (Longer; C59-2 T11) |
| `docs/test-groups.md` | the new entries; `CalcU59ReferenceRuns` described anew (C59-2 §6 T10) |

**Deleted:** `app/StateFile.hpp/.cpp`, `app/StateApplier.hpp/.cpp`
(C59-2 §2), `app/TIKeystrokePlayback.hpp/.cpp` (LIB-5 §10), the preset
cases of the tests listed in §11.6 P7 once ported, and the preset-card
cases listed there for M-55.

### 11.3 The product's rules (AC-35–40, AC-53, AC-62, AC-68)

```cpp
namespace cu59 {
class TIModelRules final : public calcu::presets::ModelRules {
public:
    IdleRule idle() const override;                       // AC-36: key ready, or waiting for a card, 100 ms, asked every 1 ms
    std::optional<std::vector<std::uint8_t>> screenshot(Machine&, const DisplayOutput&) override;   // AC-40, C59-2 AC-8
    ExpectReading compare(Machine&, const DisplayOutput&, const ExpectText&) override;          // AC-40
    std::optional<std::string> checkExpect(const ExpectText&, std::string_view keyword) const override;   // AC-40, AC-68
    Applies applies(std::string_view keyword) const override;   // preset.md §7, the three TI columns
    PlaceKeyRule placeKey(const PlaceName&) const override;      // AC-35: slot-1 by name: only
};
class TIPresetRules final : public calcu::presets::ProductRules { /* a TIModelRules per model; addKeywords */ };
}
```

- **AC-35.** The extensions and models are the catalogue's (`.ti59`,
  `.ti58`, `.ti58c`; `core/TICatalogue.cpp`), no variant; the model comes
  from `model:` only (AC-5). The place keys are the descriptors'
  `slot-1` and `printer` (C59-6). `placeKey(slot-1)` is `{bare = false,
  byName = true, byFile = false}`: `slot-1: ML` and `slot-1: { file: … }`
  are refused, the latter with "A Solid State ROM module is named by
  `name:` in this release." (OQ-8; C59-6 AC-8; D-42). `printer: PC-100C`
  takes the bare form.
- **AC-36, AC-53 idle.** `readyNow` is `Machine::isKeyReady()` (C59-1
  AC-18; `core/TIMachine.cpp:166`) or the card wait of `Write`/`INV
  Write` (`TICardReaderAdapter::isInCardWait`, `core/TIAdapters.hpp:53`);
  `stretch` 100 ms of emulated time (LIB-5 AC-20's minimum hold), `sample`
  1 ms of emulated time (D-43). First step FS 5 (M-3) showed that key
  readiness tells a stopped machine from a running program
  (`docs/walkthroughs/first-steps.md` §5), as the column waited for (OQ-1).
- **After the build** nothing is pressed (C59-2 AC-8): `afterBuild` is
  the library's default.
- **AC-37.** A place no key names is empty (the library's AC-3, AC-65);
  the upgraded examples name what a fresh 2.3.0 had (C59-2 §4).
- **AC-38.** A file without `format-version` is outdated (the library's
  AC-8) with "Calc-U-59" and `formatReference()`; any error refuses the
  file whole (the library's AC-6).
- **AC-39.** The library's AC-16, AC-12 and AC-61: full speed unless a
  `regular-speed:` comes first; live input dropped; a second load
  refused.
- **AC-40.** `applies("type")` and `applies("syncclock")` are
  `NotForThisModel` ([1600]); `printer-trace:` is C59-2's step;
  `elapsed-time:` the library's; `screenshot` is `displayPicture` (D-39);
  `compare` reads X, the value in the display register, in its 13 digits
  and its exponent, not rounded and whatever the display format or
  blinking, and finds the text when its value, read as C59-2's `data:`
  reads a value (13 significant digits, a whole number cut after the
  13th), equals X (D-44). Where the ROM keeps X is the ROM's
  (`rom/TI59-ROM-Guide.md:177`: in register C while a result is shown,
  `fB[5]`); the first task reads it through the core's CPU snapshot
  (`TIDebugTargetAdapter::snapshotCPU`, or LIB-10 59's debug target after
  M-53) for a result and for a number being keyed in, and the test of
  AC-40 (`π`: `3.14159265359` passes, `3.141592654` fails) fixes it.
  `ExpectReading::shows` is the display text as the harness's `lcd`
  gives it (`HarnessProduct::readDisplay`, `app/HarnessProduct.cpp:79-88`),
  so the reason holds what the user sees. A display that is off: EC-7.
- **AC-68, AC-40 when read.** `checkExpect` refuses a regular expression
  ("A TI model compares expect: with a number; a regular expression is
  not taken.") and a text that is not a number by C59-2's
  `parseDecimal` ("A TI model compares expect: with a number."), each
  with the step's line (D-45).
- **AC-62 markers.** `applies` gives `NotForThisModel` for `type` and
  `syncclock` on every TI model and for `magnetic-cards`, `card`, `arm`
  on the TI-58 and TI-58C (registered from M-55); `Undefined` for none
  (`preset.md` §7 marks `data` undefined only on the Sharp models). The
  Sharp keys (`slot-2`, `host-drive`, `bus-rom`, `debug`, …) are not
  registered in Calc-U-59 and are "unknown" (D-7).
- **AC-41.** The paper of the PC-100C is emptied at a user's load and
  kept at a startup preset's (LIB-8 AC-8, `BuildOrigin`; LIB-8 59, M-53).

### 11.4 Frontends (AC-42)

- **AC-42.** Qt6 hands a file in its folder (`load-preset {ref: path}`,
  `folderSource`); the Web hands a bundled example by its bundle path and
  an uploaded or dropped file by its content (`contentSource`; EC-5);
  Swift hands the content on iOS and on macOS (D-41). Nothing else of a
  load is in a frontend.
- **The GUI scenario of AC-42** (a bundled example is loaded and its cue
  card "Sum 1..N" shows): the Web in this step
  (`frontends/web/tests/play-calculator.spec.js:282`, renamed `lib6_ac42
  bundled example sum shows its cue card`, Longer); Qt6 with LIB-14 59
  (M-57), whose Examples menu (LIB-14 AC-26) is the way to a bundled
  example and whose step brings Calc-U-59's first Qt6 scenarios; Swift
  with C59-8 (M-58), which replaces `PresetLoadTests.testLoadDiagPreset`
  (`frontends/swift/Calc-U-59UITests/PresetLoadTests.swift:34-60`; it
  waits for the `keystroke-playback-status` element, which FS 4 and
  M-58 remove). Until then `PresetLoadTests` is skipped (D-46).
- **The start** and the startup presets are LIB-18's (§11 of its design).

### 11.5 Adapters

| Adapter | Removed here | Was |
|---|---|---|
| `TIPresetOperation` (the load as one product operation, `presetOperation`) | yes | LIB-3 §5; §5 above |
| `TIKeystrokePlayback` and `SliceRequest::due` | yes | LIB-5 §5 (LIB-5 §10) |
| the reader of 2.3.0 (`app/StateFile.*`, `app/StateApplier.*`) | yes | §4 |
| `HarnessMachines` | yes | LIB-18 §5 (LIB-18 §11) |
| `c59.open-dropped`'s preset half | M-56 (LIB-13 59: `open`) | — |

New: none. Calc-U-59's harness entry still makes its own
`LibraryParts` until M-59 (plan M-59: "the entry's other duties go"),
with the rules and the bundle the app layer uses.

### 11.6 Tasks

Tests come before the code. `CalcU59Tests` (Frequent) unless the row
says otherwise; ROM-gated cases `SKIP_UNLESS` the ROMs. The cases run the
product's harness entry in virtual time, the app session with
`tests/SessionRig.hpp`, or `presets::readPreset` with `TIPresetRules`.
Names `lib6_acN_…`, `lib5_acN_…`, `lib15_acN_…`.

| Task | What | ACs | Test (file: cases) |
|---|---|---|---|
| P1 | Models, places | AC-35, AC-37 | `ti_presets_tests.cpp`: `lib6_ac35_extensions_models_any_case_no_variant`, `lib6_ac35_slot1_by_name_only_file_refused_with_notice`, `lib6_ac35_printer_pc100c`, `lib6_ac37_no_slot1_no_printer_means_none` |
| P2 | Idle | AC-36, AC-53 | `lib6_ac36_running_program_not_idle_each_model_with_and_without_printer`, `lib6_ac36_idle_after_100ms_of_key_readiness`, `lib6_ac36_card_wait_after_write_is_idle` (TI-59), `lib6_ac53_ti_idle_rule_values` |
| P3 | Outdated, all or nothing | AC-38 | `lib6_ac38_2_3_0_file_refused_as_outdated_with_hint` (a 2.3.0 example's text kept as a fixture), `lib6_ac38_any_error_refuses_whole_machine_untouched` |
| P4 | Speed, input | AC-39 | `lib6_ac39_full_speed_unless_regular_speed_first`, `lib6_ac39_user_keys_dropped_second_load_refused` |
| P5 | The TI steps | AC-40, AC-68, AC-24, AC-25, AC-66, EC-7 | `lib6_ac40_type_and_syncclock_refused_when_read`, `lib6_ac40_printer_trace_sets_trace_refused_without_pc100c`, `lib6_ac40_elapsed_counts_emulated_time`, `lib6_ac40_screenshot_is_the_classic_segment_picture` (byte-equal with `tests/reference/display/`), `lib6_ac40_expect_pi_13_digits_passes_10_digits_fails`, `lib6_ac40_expect_any_display_format_and_blinking`, `lib6_ac40_not_expect_by_the_same_comparison`, `lib6_ac40_expect_not_a_number_refused_when_read`, `lib6_ac68_regex_refused_when_read`, `lib6_ac25_screenshot_into_save_folder`, `lib6_ec7_expect_display_off_fails` |
| P6 | Paper at a load | AC-41 | `lib6_ac41_user_load_empties_paper_startup_preset_keeps_it` (with LIB-8 59's paper, M-53) |
| P7 | Today's tests | (AC-38, AC-39, AC-43 as today checks them) | `tests/part_flows_tests.cpp`, `tests/session_tests.cpp`, `tests/typing_tests.cpp` and every other test that writes a 2.3.0 preset as text write version 2: `part_loads_a_preset_for_another_model` → `lib18_ac24_…` (LIB-18 §11.6), `part_reports_parse_errors_and_partition_errors` → `lib6_ac38_…`, `part_plays_a_preset`, `part_reload_keeps_the_program`, `part_reload_after_the_first_run` → `c59_2_ac7_…`, `c59_6_ac6_state_file_sets_trace_off_unless_it_sets_it` → `c59_6_ac6_preset_sets_trace_off_unless_printer_trace_on`, `lib3_ac32_preset_load_is_busy_and_refuses_second` and `lib3_ac32_updates_during_preset` on `load-preset`, `lib5_exit_printer_timing_same_paper_at_regular_and_full_speed` on the version 2 `printer-timing.ti59`. `keys_script_waits_out_a_freeze_but_runs_its_directives`, `session_plays_keystrokes_in_real_time` and `part_keystroke_controls_drive_the_gui` go with `TIKeystrokePlayback` (LIB-6 AC-19 and LIB-10 59's steps cover them). The cases whose subject is a preset's magnetic card (`part_card_file_written_then_inserted`, `part_cards_need_a_card_reader`, `part_insert_card_passes_an_unused_armed_card`, `part_program_reads_a_defined_card`, `part_program_writes_and_reads_a_memory_card`, `part_reload_and_memory_card`, `part_unused_card_is_logged_on_the_next_load`, `preset_cannot_feed_an_unreadable_file`, `preset_creates_a_full_two_sided_card`, `preset_side_on_a_version_1_file_feeds_its_one_side`, and `tests/shared_preset_stages_tests.cpp`'s `stages_card_*`, `card_bank_matches_the_rom_*`) go with the reader and are listed for C59-7 (M-55) to write anew on `magnetic-cards:`, `card:`, `arm:` (D-47) |
| P8 | The suite check | AC-43 | `calcu_preset_check.CalcU59Presets` (Frequent): the 53 examples and the three startup presets, `EXCLUDE` the four card examples (C59-2 §1); `checkBundle` over `examples/bundle.yaml` in the same entry; the release checklist's Checkpoint entry over calc-u.org's hosted presets (`59/software-files/`, `59/calc/presets/`) is M-64's (§4) |
| P9 | Frontends | AC-42 | Web: `play-calculator.spec.js` `lib6_ac42 bundled example sum shows its cue card` (`:282`), `lib6_ac42 uploaded preset by its content` (`:289`), `lib6_ac42 dropped preset loads` (`:297`) (Longer); Qt6 (M-57) and Swift (M-58): §11.4 |
| P10 | Typing in a preset | LIB-5 AC-17, AC-26 | `lib5_ac17_in_a_preset_key_finishes_at_idle`, `lib5_ac26_names_of_8_1_in_a_preset`, `lib5_ac26_2nd_before_second_function_once_in_a_preset`, `lib5_ac26_old_names_refused_when_read` |
| P11 | Bundle path on the Web | LIB-15 AC-16 | `web_part_tests.cpp:65` `lib15_ac16_start_takes_the_model_by_name_and_a_bundled_preset` on `start {preset: "sum.ti59"}` as the bundle path `examples:sum.ti59`; `:76` `lib15_ec4_…` unchanged; Web `session.spec.js` `lib15_ac16 preset prop takes a path of bundle.yaml` (Longer) |
| P12 | Harness | AC-34 | `harness_tests.cpp`: `lib6_ac34_preset_wait_expect_screenshot_on_ti59` (`calcu --product 59 preset examples:sum.ti59 then key 5 A then wait then expect 15 then screenshot sum.png`), `lib6_ac34_printer_trace_on_off` |

Coverage: P1–P9 cover AC-35 to AC-43 and AC-68 (AC-42's Qt6 and Swift
scenarios in M-57, M-58).

### 11.7 Tools and agent knowledge (M-54)

**Tools.**

- `tools/agentic/ti59file.py` goes (plan M-54): its reader of 2.3.0
  (`parse`, `load`, `tools/agentic/ti59file.py:43-151`) and its key
  playback. The scripts that loaded a preset through it run `calcu
  --product 59 serve` through `library/harness/python/calcu_serve.py`:
  `preset <file>` loads it (the keys of its `keys:` included), `wait`
  waits for idle, `paper text` gives the printout, `ti59 program`
  (C59-2 AC-9) the keycodes:
  - `kdis.py` (`:53`) lists the keycodes of a preset by `preset <file>`
    and `ti59 program 000 count=<n>`;
  - `runtime.py` times keys given with `--keys` (key IDs now, not matrix
    codes) after the preset is loaded, each by `dark_time`'s rule
    (memory `run_time_measurement`: the display lit again and no program
    running, never idle settling), sampled through `calcu` by
    `wait 10ms then lcd` (D-48);
  - `linetimes.py` and `calendar/compare.py`, `calendar/years.py` time
    the printed lines and the last key the same way, with `paper text`
    after each sample;
  - `stepcost.py` and `fastmode_matrix.py` use only the headless machine
    and the helpers `regbytes`, `press`, `settle`, `idle`, which move
    into `tools/headless/ti59.py` until M-59 replaces both scripts
    (C59-9; plan M-59).
  The calendar project's numbers (`calendar-09` 76.1 s for 2026, memory
  `calendar_speed_project`) are measured once with the old tool and the
  new before `ti59file.py` goes; a difference is a finding of the step.
- `tools/headless/capi.cpp` reads no preset (§4's table named it); it is
  unchanged until M-59.
- `tools/agentic/README.md`, `tools/headless/README.md:49`: the presets
  are loaded through `calcu`.

**Agent knowledge.**

| File | Change |
|---|---|
| `products/calc-u-59/CLAUDE.md` | the Harness section: "Not yet: `ti59 …` commands, presets" goes; `preset`, `check-preset`, `preset-list`, `wait` without a duration, `expect`, `not-expect`, `screenshot`, `printer-trace`, `elapsed-time`, `ti59 partition`, `ti59 program`, `ti59 registers` listed; the start is "the startup model with its startup preset or its built-in default (the TI-59 with the Master Library module, no printer)" (LIB-18 AC-18); the skill table and the architecture list as C59-2 §7 says; "Agentic tools" says the tools load presets through `calcu` |
| `products/calc-u-59/reference/SharedArchitecture.md` | § "State File Loading" and § "Dropped Files": the load is LIB-6's `load-preset` with `TIPresetRules`; `KeySequencer`/`TIKeystrokePlayback` gone |
| `products/calc-u-59/reference/WebClient.md`, `FrontendParity.md` | the preset list from `examples/bundle.yaml`; `presets.json` gone |
| skills `calc-u-59-shared`, `calc-u-59-web` (`products/calc-u-59/.claude/skills/`) | the load path and the bundle file where they name `StateFile`, `loadStateFile`, `presets.json` |
| `docs/test-groups.md` | `calcu_preset_check.CalcU59Presets`, `CalcU59BundledFiles`, `CalcU59CalendarScripts` in; `CalcU59ReferenceRuns` described anew; the new `CalcU59Tests` files |
| memory `calc-u-59/run_time_measurement.md` | `dark_time` is now the tools' rule over `calcu` (no longer `ti59file.py`); the rule stays |
| memory `calc-u-59/headless_printer_lines.md` | "replay KEYSTROKES like the app" → load the preset through `calcu` |
| memory `calc-u-59/fullspeed_scanloop_range_fix.md` | `FullSpeed:` and `Trace:` directives → the steps `full-speed:`, `trace:` |
| memory `calc-u-59/file_importer_architecture.md` | `loadStateFile` sends `load-preset` |
| `calc-u-59-memories.md` (the index) | the descriptions of the entries above |

**Ledger rows that become visible in M-54** (with C59-2, LIB-18 59 and
C59-3): 59-1 (presets, constant memory), 59-3, 59-4, 59-5, 59-26 and
59-42 (the waits, the first error only, `expect:`, `not-expect:`,
`screenshot:`). New rows: §11.11.

### 11.8 Golden and reference runs, risks

- **Golden runs** (`tests/golden_runs_tests.cpp`) drive the core
  directly; this column changes no path they use.
- **Reference runs.** C59-2 AC-11 (its design §6 T10, §8) is the check
  of this column as well: the 53 examples upgraded in M-54 go through
  this loader; the four card examples wait for M-55.
- **Waits.** Every item now waits until idle (row 59-42); a `Wait:` of
  2.3.0 that covered a run in wall time now counts emulated time, and an
  item that 2.3.0 played on while a program still ran now waits for it.
  The reference runs compare at the idle machine.
- **`dark_time` over `calcu`** samples every 10 ms of emulated time where
  the old tool sampled every 100 instructions; the measured times move by
  at most one sample. The one-time comparison of §11.7 shows it.
- **The Swift app** may not be testable beyond its build (LIB-5
  qa-report: the XCFramework build did not run for the agent in M-36);
  its load path is two lines of the bridge.

### 11.9 Decided (best guess)

- **D-36 (Done in).** AC-42's GUI scenario needs a way to a bundled
  example in each frontend; Qt6 and Swift get it with LIB-14 59 (M-57)
  and C59-8 (M-58). By plan §1 the column is then Done in M-58, not M-54
  (Plan note P-2).
- **D-37 (no library change).** The column needs nothing new of
  `library/presets/`: `checkExpect`, `placeKey`, `applies`, `IdleRule`
  and the section value form cover the TI.
- **D-38 (`formatReference`).** "the preset format in the User Guide,
  chapter Presets" until M-60 gives the help topic's address (as D-30).
- **D-39 (the TI picture).** Drawn in the app layer from the frontends'
  segment geometry in the classic style, flat (no glow, which each
  frontend paints its own way), at one pixel per unit of `LedGeometry`'s
  cell; the reference pictures are made by this code in the step and
  reviewed by the user at the E/F boundary (as LIB-8's reference
  pictures). LIB-14 59's Copy Screen (M-57) can take the same function.
- **D-40 (the app part holds the library's parts).** As Calc-U-1600's
  `SharpAppPart`: one `LibraryParts` per app session, owned by
  `TIAppPart`, which every host and the tests use; `makeAppSession` and
  the Web bindings stop adding a `TypingPart` of their own.
- **D-41 (Swift hands content).** The picked file is readable only
  inside its security scope and its folder is not granted, so Swift
  hands the content on macOS too; a preset that names a relative file is
  then refused with EC-5's notice (no TI preset of today names one).
- **D-42 (`slot-1` sentence).** "A Solid State ROM module is named by
  `name:` in this release." for `{ file: }`; a bare ID gets the reader's
  sentence for a form the place does not take.
- **D-43 (sampling).** 1 ms of emulated time between two questions of
  `readyNow`, short beside the 100 ms stretch, so a break in readiness of
  a running program is seen.
- **D-44 (the number compared).** The text is read as `data:` reads a
  value, so `expect:` and `data:` agree on what "13 digits" means; equal
  means the same sign, 13 mantissa digits and exponent.
- **D-45 (a non-number refused when read).** `checkExpect` refuses any
  text `parseDecimal` does not take (`docs/reviews/d-simplification.md`
  §9: "say whether a non-number is refused when read"): AC-40 says
  `expect:` takes a number.
- **D-46 (Swift's preset test).** `PresetLoadTests.testLoadDiagPreset`
  waits for an element FS 4 removes and loads a file through the
  document picker, not a bundled example; it is skipped from M-54 and
  replaced by C59-8's scenario (M-58).
- **D-47 (the preset-card tests).** They test behaviour of the 2.3.0
  reader (`CARD:`, `ArmCard:`, `InsertCard:`) that goes in M-54 and comes
  back in a new form in M-55; their names are listed (P7) so that M-55's
  tests cover each.
- **D-48 (`dark_time` over `calcu`).** The tools keep the user's timing
  rule and get it from the harness by sampling the display every 10 ms of
  emulated time, until C59-9's `ti59 run-time` and `ti59 line-times`
  (M-59) measure inside the product.

### 11.10 Questions for the user

None that splits this column. (C59-2 §10 asks how `ti59 program` tells a
count from a keycode.)

### 11.11 Proposed ledger rows and plan notes

**Ledger rows:** none new. Checked and not proposed: the nine examples
the Web offers between M-54 and M-55 (no release in between, plan §4);
`expect:` taking a number only (row 59-42 and the spec, AC-40, AC-68);
the agent tools taking key IDs (`runtime.py --keys`) and loading through
`calcu` (developer tools, not user-visible).

**Plan note P-2.** §5.1: LIB-6's Calc-U-59 column is tested in M-54 and
Done in M-58 (D-36): AC-42's scenario passes on the Web in M-54, in Qt6
in M-57 and in Swift in M-58. M-57's "Tests from" gains LIB-6 AC-42
(Qt6), M-58's LIB-6 AC-42 (Swift).

**Plan note P-3.** M-54's "Knowledge, ledger" names the skill
`calc-u-59-statefiles`: it is renamed `calc-u-59-presets` in the step
(C59-2 §7, D-18 there); M-55's line "`calc-u-59-statefiles` skill
(cards)" reads `calc-u-59-presets`.

## Part E simplification E-1 (2026-11-17, branch `es-assembly`)

`docs/reviews/e-simplification.md` §1 S-1. The PresetsPart is always made
(`parts::LibraryParts`), so the harness's refusal "This build reads no
presets of format 2 yet." went (LIB-11, this step). Its `home` and
`screenshotFolder` come from the host's places (`Paths::hostDirs().home`,
`Paths::screenshotFolder()`, made when a step needs it); the products'
screenshot lambdas are gone. Calc-U-59's Web engine therefore hands a
screenshot to KeepFile as Calc-U-1600's does (LIB-9 AC-17), instead of
writing it into the save folder of its file system (ledger 59-58); its
`~/` is the Web's home (`/persist`), as Calc-U-1600's, not empty.
