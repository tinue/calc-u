# LIB-6 Presets: design
Status: approved by the user at the Part C1/C2 boundary (2026-11-15)

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
struct HeaderKeyDef { std::string key; std::function<void(const formats::YamlNode&, int line, ReadContext&)> read; bool afterOtherHeaderKeys = false; };
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
  virtual bool takesRegex() const { return true; }                       // AC-68: TI false
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
| `expect:`, `not-expect:` | step, raw | not empty; `parseExpectText`; a regex where `takesRegex()` is false is refused (AC-68) | `ModelRules::compare`; fails with the text and `shows`; EC-7 | TakenEffect |
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
| Calc-U-59 (M-54) | `app/StateFile.*`, `app/StateApplier.*`, `TIAppPart::loadStateFile`, `applyState`, `presetOperation`, `TIPresetOperation`, the `STATE` records of `tests/golden/formats.txt` | `cu59::TIPresetRules : ProductRules` in `app/TIPresetRules.*`: idle by AC-36 (100 ms ready, or a card wait), the segment picture (OQ-3), the X register comparison (AC-40), `takesRegex()` false, `slot-1` by `name:` only (AC-35), `type` and `syncclock` not for the TI; C59-2's sections and `printer-trace` through `addKeywords`. `TIAppPart` constructs `PresetsPart` |
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
  both products; a product refuses it by `takesRegex()`. AC-67's tests
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
