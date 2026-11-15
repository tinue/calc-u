# LIB-11 Harness: design
Status: approved by the user at the Part B/C1 boundary (2026-11-14)

## 1. Scope

| Column | Step | Tests from (plan §3) | Done in (plan §5.1) | In this design |
|---|---|---|---|---|
| Library | M-28 | AC-1–13, AC-21, AC-22; EC-1–7 | M-28 | designed |
| Calc-U-1600 | M-51 (with C1600-5) | AC-18–20 | M-51 | listed only |
| Calc-U-59 | M-59 (with C59-9) | AC-14–17 | M-59 | listed only |

M-28 waits for M-22 (LIB-3 L: sessions) and M-26 (C1600-1: a Sharp
machine behind the interface). It also uses M-18 (LIB-1), M-20 (LIB-9),
M-23 (LIB-2: the real-time clock capability) and M-24 / M-27 (LIB-12: the
ROM lookup), which come before it.

Also in M-28, outside the ACs:
- The exit check, "`calcu` drives a TI-59 and a PC-1600 headless". M-28
  therefore adds a thin harness product entry to each product (§4). The
  product ACs (AC-14–20) are not tested until M-51 and M-59. The harness
  has no model option before LIB-18, so the Calc-U-1600 part of the check
  drives the PC-1500A it starts with; the PC-1600 follows in M-35 (D-1).
- The Python helper for `calcu serve` (plan M-28, `harness.md` Q-2). The
  plan has it in M-28; the spec's Out of Scope means only that the spec
  does not define it (D-2).
- The harness is built in every workflow on macOS, Linux and Windows
  (plan M-28, "Green after"; AC-15 itself is tested in M-59).

Some clauses of the Library ACs name a feature that is built later. Plan
§1 says such a clause passes in that feature's step. M-28 tests the
harness side of each one (the seam, a test product), and the clause
itself passes there:

| Clause | Passes in |
|---|---|
| AC-2 "the machine the product starts with (LIB-18)" | M-35 (LIB-18 L). Until then the adapter of §5, A-1 |
| AC-8 bundled media, bundled presets; the traces and screenshots of the run | M-41 (LIB-7), M-38 (LIB-6), M-40 (LIB-10). M-28 tests the folders the run hands out |
| AC-9 `wait` ends at once while the debugger has the machine paused (LIB-10 AC-21) | M-40 |
| AC-11 the topics of other features and of a product | each feature's step. M-28 tests with the test product's own topic |
| AC-12 `wait` without a duration | M-38 (LIB-6 AC-34) |
| AC-21 `syncclock:` | M-38 |
| AC-22 the real mapping commands (`floppy`, `card`, `saveas`) | M-41 (LIB-7). M-28 tests with a test product command |

Later steps of this feature, not designed here:

| Step | What | Tests from |
|---|---|---|
| M-33 (C59-5) | harness attach for the Swift frontend: the serve protocol of this design over localhost TCP, in the line format of §3.9 | C59-5's |
| M-43, M-49, M-57 (LIB-14 AC-44) | Qt6 attach; `--shots` becomes its client | LIB-14's |
| M-50 (C1600-3) | `serve --dap <port>` (`harness.md` H-1 completed) | C1600-3's |
| M-51 (C1600-5, LIB-11 1600) | `pc1600 …` commands; `pc1500_cli`, `pc1600_cli`, the three probes and `dev/tape-matrix/run.py` go | LIB-11 AC-18–20; C1600-5 AC-1–9, 11, EC-2–6 |
| M-59 (C59-9, LIB-11 59) | `ti59 …` commands; `tools/headless/` and `tools/agentic/` go, and `coverage.py`, `fastmode_matrix.py` and `asm59.py` stay as scripts on the harness | LIB-11 AC-14–17; C59-9 AC-1–11, 13–14, EC-2–6 |

## 2. Modules and files

New. The layout follows LIB-1 design §2a: the areas `library/harness/`
(`calcu::harness`) and `library/formats/` (`calcu::formats`) join
`calcu_library`, their tests join `calcu_library_tests`.

| File | Holds |
|---|---|
| `library/harness/Reply.hpp/.cpp` | `Reply`, `Value`, the plain and the JSON writers (§3.1) |
| `library/harness/Protocol.hpp/.cpp` | `CommandLine`, `parseLine()`, `splitOneShot()` (§3.2) |
| `library/harness/Commands.hpp/.cpp` | `CommandRegistry`, `CommandContext`, `Handler` (§3.3) |
| `library/harness/FirstCommands.cpp` | `addFirstCommands()`: `wait`, `key-down`, `key-up`, `lcd`, `state`, `capabilities`, `ticks`, `audio`, `quit` (AC-9) |
| `library/harness/HarnessHost.hpp/.cpp` | the harness's side of the app session: the listener that keeps the topics, the recording sound sink, operations run in one go (§3.4) |
| `library/harness/DisplayText.hpp/.cpp` | `DisplayText`, `GlyphFont`, `readDotMatrix()`, `readSegments()` (§3.5) |
| `library/harness/WavRecorder.hpp/.cpp` | 16-bit mono PCM WAV; the header is written again at close (AC-9 `audio`, EC-3) |
| `library/harness/Product.hpp` | `HarnessProduct`, the interface a product implements (§3.6) |
| `library/harness/Run.hpp/.cpp` | `RunOptions`, `RunSetup` (run folder, settings, clock), `Server` (the serve loop over a line reader and writer; no stdio inside) |
| `library/harness/cli/CMakeLists.txt` | the executable `calcu` (not under Emscripten) and the test executable `calcu_test` (the test product only, with `CALCU_TESTS`); the deferred generation of the product table |
| `library/harness/cli/Main.cpp` | `main()`: `return calcu::harness::cli::run(args, stdin, stdout, stderr, products())` |
| `library/harness/cli/Cli.hpp/.cpp` | `cli::run()`: options, product choice, one-shot or `serve`, exit status (§3.8) |
| `library/harness/cli/Products.cpp.in` | template for the generated table `products()` (§3.7) |
| `library/harness/tests/TestProduct.hpp/.cpp` | the product made for the tests (§6, T5) |
| `library/harness/tests/*_tests.cpp` | one file per area (§6) |
| `library/harness/tests/cli/*.cmake` | process tests run with `cmake -P`: argv, stdin, exit status (§6, T13) |
| `library/harness/python/calcu_serve.py`, `tests/test_calcu_serve.py` | the Python helper (D-2) |
| `library/formats/` (`Flow.hpp/.cpp`, `Duration.hpp/.cpp`, `tests/`) | `calcu::formats`: scalars, flow mappings and flow sequences of `docs/formats/README.md` §1 (`parseValue`); durations `2s`, `1.5s`, `500ms` (`preset.md` §5.3, `parseDuration`). The YAML reader (`Yaml.hpp`, from Calc-U-1600's `core/Yaml.hpp`) is already there from M-24, the first step that needs it (LIB-12 reads Calc-U-59's `.rom.yaml`; D-3); `parseValue` builds on it |

Changed:

| File | Change |
|---|---|
| `library/CMakeLists.txt` | the sources of `library/harness/` and `library/formats/` join `calcu_library`, their tests `calcu_library_tests`; `add_subdirectory(harness/cli)`; the CMake function `calcu_harness_add_product(<target> <factory>)` (§3.7), defined before the product directories are read |
| `products/calc-u-59/app/HarnessProduct.hpp/.cpp` (new), `products/calc-u-59/CMakeLists.txt` | `cu59::HarnessProduct` (§4); `calcu_harness_add_product(calcu59_shared cu59::makeHarnessProduct)` |
| `products/calc-u-1600/app/HarnessProduct.hpp/.cpp` (new), `products/calc-u-1600/cmake/CalcU.cmake` | `shared::HarnessProduct` (§4); `calcu_harness_add_product(calcu_shared shared::makeHarnessProduct)` |
| `products/calc-u-59/tests/harness_tests.cpp`, `products/calc-u-1600/tests/harness_tests.cpp` (new) | the exit check (§6, T14) |
| `.github/workflows/calc-u-59.yml`, `calc-u-1600.yml` | build the target `calcu` in every desktop job; the `library` job (LIB-1 design T1) runs `calcu_library_tests` and the process tests |
| `docs/test-groups.md` | the new entries, all Frequent |
| `products/calc-u-59/CLAUDE.md`, `products/calc-u-1600/CLAUDE.md` | a harness section (§7) |

## 3. Seams and interfaces

Sketches. Names in `calcu::machine` are LIB-1's (design §3). Names in
`calcu::sessions` are LIB-3's (design §3; reconciled). What the harness
needs from LIB-3 is listed at the end of §9 (S-list; all met).

### 3.1 Replies (AC-4, AC-5)

```cpp
namespace calcu::harness {
class Value {             // a JSON value with ordered object keys
public:                   // null, bool, integer (int64), Decimal, string, array, object
  static Value object(); Value& set(std::string key, Value); Value& push(Value);
};
struct Decimal { std::int64_t units; int places; };   // exact, written without a double (AC-6)
struct Reply {
  bool done; Value result; std::string reason;         // reason: one line, empty when done
  std::vector<std::string> lines;                      // the plain form of `result`, written by the command
};
std::string writePlain(const Reply&);      // see below
std::string writeJson(const Reply&);       // {"done":true,"result":…,"reason":null} + "\n"
}
```

- **Plain** (the default): the first line is `done`, `done <n>` followed by
  n result lines, or `refused: <reason>`. Because of the count, a reader
  never mistakes a display row for the end of a reply.
- **JSON** (`--json`): one object on one line with the three keys `done`,
  `result` and `reason`, the same for every command (AC-4, `harness.md`
  H-3). `result` is `null` when the command reads nothing. `reason` is
  `null` when the command was done.
- Seconds are written from ticks and the rate in integers, with nine
  decimals, truncated (`Decimal`). Brightness is written with three
  decimals. No `double` reaches the output, so two runs agree byte for
  byte (AC-6).
- The path of the run's save folder is replaced by `<save folder>` in
  every reply, notices and the log included (AC-8: "whose path appears in
  no reply").

### 3.2 Lines and the one-shot form (AC-2, AC-3, AC-13, AC-22)

```cpp
struct CommandLine { std::vector<std::string> path;   // {"lcd"} or {"test", "count"}
                     std::string argText; };          // the rest, as written, trimmed
std::variant<CommandLine, std::string /*refusal*/> parseLine(std::string_view, const CommandRegistry&);
std::vector<std::string> splitOneShot(std::span<const std::string> words);  // AC-2, OQ-7 of the spec
}
```

- **The word between commands is `then`.** `calcu --product 1600 wait
  2s then lcd then ticks`. The words of each command are joined with one
  space into a line, so the one-shot form and `serve` share one parser.
  A literal argument `then` cannot be written in the one-shot form. Use
  `serve` for that.
- **A product command** is reached by its product word first (`test
  count`, later `ti59 …`, `pc1600 …`). The word is looked up only among
  the product words, and a common command is looked up at the top level
  (AC-13).
- **The value of a step** (AC-22): `argText` is the step's value exactly.
  A preset step `- floppy: { name: Disk2, side: 1 }` becomes the line
  `floppy { name: Disk2, side: 1 }`. A handler reads `argText` with
  `calcu::formats::parseValue()` (a scalar, a flow mapping or a flow
  sequence by §1 of `docs/formats/README.md`), the same reader LIB-6
  uses for the step (D-3). In the one-shot form the shell needs quotes
  around a mapping: `floppy '{ name: Disk2, side: 1 }'`.
- A line ends at `\n`, and a `\r` before it is dropped (Windows). An empty
  line is ignored. A line whose first word is no command gets
  `refused: unknown command <word>` (AC-5).

### 3.3 Commands (AC-9, AC-13)

```cpp
struct CommandContext { HarnessHost& host; const CommandLine& line; };
using Handler = std::function<Reply(CommandContext&)>;
class CommandRegistry {
public:
  void add(std::string name, Handler, std::string usage);                         // a common command
  void addProduct(std::string productWord, std::string name, Handler, std::string usage);  // AC-13
  const Handler* find(const std::vector<std::string>& path) const;
  std::vector<std::string> usage() const;                                         // for --help
};
void addFirstCommands(CommandRegistry&);   // the table of AC-9
```

Each later feature adds its commands to the registry in its own step,
from its own area: `key`, `type` (LIB-5); `preset`, the bare `wait`,
`expect`, `screenshot` (LIB-6); and so on (spec, Out of Scope). The
registry is the place AC-13 asks for: product commands under the
product's word, common commands at the top level. A common command with
product-specific output asks the `HarnessProduct` for the product part,
as `lcd` does (§3.5).

The first commands:

| Command | What the handler does | Plain result | `result` (JSON) |
|---|---|---|---|
| `wait <duration>` | `formats::parseDuration` (µs, integer; EC-2 when there is no unit). Ticks = ⌈µs × rate / 10⁶⌉. `host.advance(ticks)`, which is LIB-3's `AppSession::advanceUnpaced` | `ticks: <n>`, `seconds: <s>` (what passed) | `{"ticks":n,"seconds":s}` |
| `key-down <key ID>`, `key-up <key ID>` | the key resolved as the `key` step does (other spellings of the descriptor, any case; LIB-1 D-2; D-9), then LIB-3's commands `key-down`/`key-up` `{key}` with the key ID (EC-7: LIB-1 EC-3) | — | `null` |
| `lcd` | the last display state, through the product's rule (§3.5) | rows; `indicators: <names>`; `not text: <n>` (dot matrix); or `display off` | `{"on":…,"rows":[…],"indicators":[{"name":…,"brightness":…}],"notText":n}` |
| `state <topic>` | `host.readTopic(name)`; EC-5 when it is no topic | a state topic: its text form; a stream topic: `reset` if it started over, then what was appended, one item per line | `{"topic":…,"shape":"state","state":…}` or `{"topic":…,"shape":"stream","reset":b,"items":[…]}` |
| `capabilities` | LIB-3's request `capabilities`, reply `MachineOffer` (LIB-3 AC-39) | `capability <kind> <id> <owner>` per capability; `place <path> <peripheral>[:<variant>]` or `place <path> empty` per place | `{"capabilities":[{"kind","id","owner"}],"places":[{"place","plugged":[…]}]}` |
| `ticks` | LIB-3's request `ticks`, reply `EmulatedTimeReply` (LIB-3 AC-39; LIB-17 design §6) | `ticks: <n>`, `seconds: <s>` | `{"ticks":n,"seconds":s}` |
| `audio <file>` | refused when `descriptor().hasSound` is false (EC-6). Otherwise `host.record(path)`, relative to the working directory; a second `audio` completes the first file and starts the new one (D-8) | — | `null` |
| `quit` | ends `serve`. In the one-shot form it ends the run and the commands after it are not carried out | none (`serve` writes nothing more) | — |

The owner is written `component:<id>` or `peripheral:<id>@<place path>`.

### 3.4 The harness host (AC-5–7, AC-11, AC-21; I-1)

```cpp
class HarnessHost {    // talks to the session through a sessions::Listener (LIB-3 §3.10; C1 simplification)
public:
  // session: built with AppSessionSetup{hosting = Hosting::VirtualTime, parts}; the host's
  // listener listens once (no services stated: LIB-3 A-2)
  HarnessHost(sessions::AppSession&, HarnessProduct&, const RunSetup&);
  Reply command(sessions::Command);               // session.send(me, cmd); in VirtualTime an operation it
                                                  // starts runs to its end inside send() (LIB-3 §3.5, AC-19),
                                                  // so the ReplyMessage is delivered before send() returns (AC-7)
  machine::EmulatedTime advance(machine::Ticks);  // session.advanceUnpaced(n); returns what passed (AC-6, AC-9)
  sessions::EmulatedTimeReply emulatedTime();     // session.ask(me, Request{"ticks"}) (LIB-3 AC-39)
  sessions::MachineOffer machineOffer();          // session.ask(me, Request{"capabilities"}) (LIB-3 AC-39)
  std::optional<TopicRead> readTopic(std::string_view name);   // from the mirror (AC-11, EC-5)
  const machine::DisplayOutput* display() const;  // the mirror's `display` state (`DisplayState`)
  Reply record(std::string path);                 // AC-9 `audio`: a second listen(wavSink, {recordsSound = true})
  void finish();                                  // stopListening; closes the WAV file (EC-3), removes the run folder (AC-8)
  void deliver(const sessions::Message&) override; // State/StreamMessage → mirror; ReplyMessage → the pending command
private:
  sessions::TopicMirror m_mirror;                 // last state per state topic; takeAppended per stream (LIB-3 §3.10)
  sessions::ListenerId m_me;
};
```

- **No wall clock** (AC-6, `harness.md` I-2). The host never makes the
  wall-timed host call of LIB-3 AC-2, and it creates no pacer. Emulated
  time passes only through `advance` (from `wait`) and through
  operations run in one go (LIB-3 AC-19). The session is hosted with
  messages delivered at once, on the caller's thread (LIB-3 AC-4).
  There is no thread and no lock (ADR 0009).
- **Operations** (AC-7): when a command's reply says that it started an
  operation, `command()` runs it in one go and returns the reply that
  the session gives when the operation ends (LIB-3 AC-18). The busy
  topic is idle again before the harness replies.
- **Topics** (AC-11): a name is any topic the session announces: LIB-3's
  `display`, `busy`, `notices` and `log`, then those of other features,
  then the product's own. The harness reads none of them except
  `display`. For the text form and `--json` it uses each payload's
  `Payload::toValue()` (LIB-3 §3.1), converted from `sessions::Value` to
  this area's `Value` (whose `Decimal` keeps AC-6's exact seconds; two
  types, D-10). A stream read is `TopicMirror::takeAppended`. A topic
  that was announced but never sent is known from the session's topic
  list (S-6), so EC-5 refuses only a name that is no topic.
- **Nothing but replies** (AC-5): in `serve` and the one-shot form,
  notices and log lines are kept for `state` and never written by
  themselves. Attach is the one exception (§3.9).
- **Sound** (LIB-3 AC-20, §3.6): after `audio` the host listens a second
  time with `HostServices{recordsSound = true}`; that listener gets
  `SoundStart{rate}` and every `SoundSamples`, at every speed, and
  `WavRecorder` writes them at that rate.
- **The real-time clock** (AC-21): the harness hands the instant to the
  session as `AppSessionSetup::hostTime` (LIB-3 A-7), which sets the
  real-time clocks at build and after each operation (S-7). The default is the fixed instant
  `2000-01-01T00:00:00`, in no time zone (the real-time clock gets a
  local date and time; D-4).
  `--clock <date-time>` gives another instant, and `--clock host` gives
  the host's.

### 3.5 Display text (AC-10)

```cpp
struct DisplayText { bool on; std::vector<std::string> rows;
                     std::vector<std::pair<std::string, std::optional<Decimal>>> indicators;  // those on, in descriptor order
                     int notText; };
struct Glyph { std::string text; std::vector<std::uint8_t> columns; };  // a cell column-wise, as LcdCell today
struct GlyphFont { int cellWidth, cellHeight; std::vector<Glyph> glyphs; std::vector<Glyph> cursorShapes; };
DisplayText readDotMatrix(const machine::DotMatrixFrame&, const machine::DotMatrixGeometry&,
                          const GlyphFont&, std::span<const machine::IndicatorOutput>);
using SegmentRule = std::function<std::string(const machine::SegmentPosition&, const machine::SegmentLayout&)>;
DisplayText readSegments(const machine::SegmentFrame&, const machine::SegmentGeometry&,
                         const SegmentRule&, std::span<const machine::IndicatorOutput>);
```

- `readDotMatrix` follows today's `products/calc-u-1600/core/Display/LcdText.cpp`
  (`parseLcdText`), without any model in it. Per cell: all dots off
  gives a blank, an exact glyph gives its text, an inverted glyph gives
  its text, and anything else gives U+FFFD and counts in `notText`. The
  font is the product's, because it comes from the machine's ROM
  (`core/PC1500/PC1500LcdText.hpp`, `core/PC1600/PC1600LcdText.hpp`).
- `readSegments` writes one string per position, from left to right,
  with the product's rule (AC-10: "a rule the product supplies").
- `on == false` gives the single line `display off`.

### 3.6 The product interface

```cpp
class HarnessProduct {
public:
  virtual ~HarnessProduct() = default;
  virtual std::string_view name() const = 0;          // "Calc-U-59" (AC-1)
  virtual std::string_view shortName() const = 0;     // "59"
  virtual std::string_view commandWord() const = 0;   // "ti59", "pc1600" (AC-13)
  virtual const machine::Catalogue& catalogue() const = 0;
  virtual machine::MachineBuilder& builder() = 0;
  virtual const settings::ProductDecl& settingsDecl() const = 0;      // A-3 in §5 until LIB-9 59/1600
  virtual std::unique_ptr<roms::Bundle> bundle(const RunSetup&) const = 0;  // normally a roms::DirectoryBundle of the
                                                                            // ROM folder; roms::lookUp + roms::describe (LIB-12; O-2)
  virtual machine::BuildRequest startupRequest(const settings::SettingsFile&) = 0;   // adapter A-1
  virtual DisplayText readDisplay(const machine::DisplayOutput&, const machine::MachineDescriptor&) = 0;  // AC-10
  virtual void addCommands(CommandRegistry&) {}       // AC-13; C59-9, C1600-5
  virtual void addToSession(sessions::AppSession&) {} // the product's app layer: topics, commands (LIB-3 AC-12)
};
using ProductFactory = std::unique_ptr<HarnessProduct> (*)();
```

### 3.7 Registering the products (AC-1, EC-4)

The library may not depend on product code (LIB-1 AC-24). Yet `calcu` is
one program for every product built. Here is how both hold:
- Each product calls `calcu_harness_add_product(<its app target>
  <factory>)`, which appends to a global CMake property.
- `library/harness/cli/CMakeLists.txt` registers
  `cmake_language(DEFER DIRECTORY ${CMAKE_SOURCE_DIR} CALL …)`, which
  runs after every product directory has been read (CMake ≥ 3.19, and
  the tree needs 3.21). It writes `Products.cpp` from `Products.cpp.in`
  (declarations, then a `std::vector<ProductFactory>`) and links each
  product target into `calcu`.
- No static initialiser is used, because one in a static library is
  dropped by the linker.
- `calcu_test` gets the same main with the test product only, so the CLI
  is tested without product code.

`--product` matches `name()` or `shortName()` without regard to case.
With no `--product`, or with one that names no product of the build,
`calcu` ends with exit status 2 and lists the products by name and short
form (EC-4), also in a build with one product (D-5).

### 3.8 Options and exit status (AC-1, AC-2, AC-8, AC-21)

`calcu [options] <command> [then <command> …]` or `calcu [options] serve`.

| Option | Meaning |
|---|---|
| `--product <name>` | AC-1 |
| `--json` | AC-4 |
| `--settings <file>` | open that settings file (`SettingsFile::open`). Without it, `SettingsFile::defaultsOnly` is used, and nothing is read or written (AC-8; LIB-9 AC-18) |
| `--save-folder <dir>` | use that save folder. Without it, an empty folder is made under the system temp folder for the run and removed at the end (AC-8) |
| `--output-folder <dir>` | where traces and screenshots go. Without it, they go to the save folder (AC-8) |
| `--clock <YYYY-MM-DDThh:mm:ss>` / `--clock host` | AC-21 |
| `--help` | the usage and `CommandRegistry::usage()` |

The run's `calcu::settings::HostDirs` hold the temp folder (or the named
one) as `saveFolder`, with `settingsBase` and `home` inside the run
folder, so that nothing of the user's is reached (LIB-9 AC-18). It
passes `FrontendKind::Qt6` (no harness value; LIB-9 D-11), so traces and
screenshots go to the save folder of the run, or to `--output-folder`.

Exit status: 0 when every command was done; 1 at the first refused
command, after its reply, and the commands after it are not carried
out (AC-2); 2 when the run cannot start (usage, EC-1, EC-4), with the
message on standard error. `serve` gives 0 at `quit` or at the end of
its input (AC-3, EC-3).

### 3.9 Attach line format (C59-5, M-33)

The attached connection (C59-5's endpoint in `library/harness/attach/`,
built into `calcu_library` only with `CALCU_ATTACH`) is the serve
protocol with one addition: the session's messages also come as lines.
- Client to app: command lines, as in `serve` (§3.2).
- App to client, each line one of:
  - a reply, exactly as in `serve`: `done`, `done <n>` and n result lines,
    or `refused: <reason>`; with `--json`, the one JSON object (§3.1);
  - a message: `message <topic> <text>`, the payload's text form on one
    line (newlines escaped as `\n`); with `--json`, one line
    `{"message":{"topic":…,"value":…}}`. Messages are what the attached
    listener gets: every state topic once at attach and on each change
    (C59-5 AC-12, LIB-3 AC-6), and the stream topics' appended items
    (`notices`, `log`, a product's streams) as they come.
- A message line never falls inside the n lines of a reply, so a reader
  tells them apart by the first word (`done`, `refused:`, `message`).
- `calcu attach` (C59-5) relays the replies unchanged, and the message
  lines only with `--messages` (C59-5 D-1).

## 4. Product side (M-28)

M-28 changes no core. Each product gets a harness product entry in its
app layer. In M-28 the harness runs the library's app session directly
on the product's machine, through the machine interface. The product's
own session classes (`products/calc-u-59/app/EmulatorSession.*`,
`products/calc-u-1600/app/AppSession.*`, `MachineSession.*`) are not
used. `addToSession()` stays empty until the product moves onto the
library's sessions: M-31 for Calc-U-1600 ("Qt6 and the harness first")
and M-33 for Calc-U-59 (D-6).

| | Calc-U-59 (`cu59::HarnessProduct`) | Calc-U-1600 (`shared::HarnessProduct`) |
|---|---|---|
| name, short, word | `Calc-U-59`, `59`, `ti59` | `Calc-U-1600`, `1600`, `pc1600` |
| catalogue, builder | the C59-1 ones (M-19) | the C1600-1 ones (M-26) |
| bundle | LIB-12's lookup (M-24) | LIB-12's lookup (M-27). The CLI's own lookup is gone by then (`tools/pc1600_cli.cpp:110-141`; LIB-12 1600) |
| startup (A-1) | TI-59, its default variant (LIB-9 AC-20's fallback) | PC-1500A (LIB-9 AC-27's fallback) |
| display rule | segments to characters: the inverse of `app/SevenSegment.hpp` (`kDigitSegments`, `kSegmentsMinus`, `kSegmentsDegree`), plus a `.` for a lit DP. Quote, apostrophe and the hex digits that `tools/headless/ti59.py:268-289` prints are taken from the same table once C59-1 has fixed the segment patterns (checked in M-59, AC-16) | the font of `core/PC1500/PC1500LcdText.hpp` or `core/PC1600/PC1600LcdText.hpp`, built from the ROM images the session was given, and `readDotMatrix`. The cursor cell of `LcdCursor` needs RAM that is not in the display output: from M-51 the product's rule reads it from a product topic (D-7) |
| commands | none until C59-9 (M-59) | none until C1600-5 (M-51) |

Stays: `tools/headless/` with `capi.cpp`, `ti59.py`, `coverage.py`,
`tools/agentic/`, `pc1500_cli`, `pc1600_cli`, the probes and
`tools/check_presets.sh`, all unchanged (plan §1, "Today's tools").
Nothing is deleted in M-28.

## 5. Adapters

| # | Adapter | File | Wraps | Removed in |
|---|---|---|---|---|
| A-1 | `HarnessProduct::startupRequest()` | `products/*/app/HarnessProduct.cpp` | the product's start (startup model, default variant) until LIB-18's start of a product exists | LIB-18 1600 (M-46), LIB-18 59 (M-54). The library side arrives in M-35 |
| A-2 | withdrawn: LIB-3 A-7's `hostTime` sets the clocks, so no hand-over adapter is needed | — | — | — |
| A-3 | a minimal `ProductDecl` (`[startup] model` only) | `products/*/app/HarnessProduct.cpp` | the product's settings declaration, which does not exist before LIB-9's product columns | M-30 (1600), M-32 (59), where `settingsDecl()` returns the product's declaration |
| A-4 | the Calc-U-1600 font built from `core/PC1500/PC1500LcdText.hpp` and `core/PC1600/PC1600LcdText.hpp` (`LcdFont` to `GlyphFont`) | `products/calc-u-1600/app/HarnessProduct.cpp` | the model fonts of `core/Display/LcdText.hpp`, which `--lcd-text`, `expect:` and the DAP still use | M-51: `--lcd-text` goes; `parseLcdText` stays as the product's font source only (D-7) |

The adapter check of plan §1 runs in M-51 and M-59. By then A-1 to A-4
are gone.

## 6. Tasks

Library, M-28. Files in `library/harness/tests/` (and
`library/formats/tests/` for T2), in `calcu_library_tests`, group
Frequent. The process tests of T13 and the Python helper's test (T15)
are CTest entries of their own, because they run a program.
Each test is written before the code of its task.

| Task | What | ACs/ECs | Test (file, case, group) |
|---|---|---|---|
| T1 | CMake: the two areas in `calcu_library`, `calcu`, `calcu_test`, `calcu_harness_add_product`, the deferred table; the `docs/test-groups.md` entries | AC-1 | `tests/cli/product_names.cmake`: `ac1_product_by_name_or_short_form_any_case` (`Calc-U-Test`, `calc-u-test`, `TEST`, `test`) |
| T2 | `calcu::formats`: `parseValue`, `parseDuration` | AC-12, AC-22, EC-2 | `library/formats/tests/formats_tests.cpp`: `ac12_wait_takes_the_steps_duration_forms` (`2s`, `1.5s`, `500ms`), `ac22_flow_mapping_as_in_a_preset` (the three examples of AC-22), `ec2_duration_without_unit_is_refused` |
| T3 | `Reply`, `Value`, the writers | AC-4, AC-5 | `reply_tests.cpp`: `ac4_plain_reply_is_lines`, `ac4_json_reply_has_the_three_keys_for_every_command` (every registered command, done and refused), `ac5_refusal_carries_the_reason` |
| T4 | `parseLine`, `splitOneShot`, `CommandRegistry` | AC-2, AC-5, AC-13, AC-22 | `protocol_tests.cpp`: `ac2_commands_part_on_then`, `ac5_line_that_is_no_command_is_refused`, `ac13_product_command_under_product_word`, `ac13_common_command_at_top_level`, `ac22_step_becomes_command_by_dropping_dash_and_colon` (`test mapping { a: 1, b: x }`) |
| T5 | Test product (`TestProduct.*`): `Calc-U-Test`/`test`/word `test`; models `test-seg` and `test-dot` (LIB-1 `TestMachines.hpp`); a segment rule and a 3-glyph font; a product topic `test.count`; commands `test mapping` (echoes its parsed value), `test operation <ticks>` (starts an operation of that length), `test fail` (refused); a ROM full set of two files in a temp bundle; the real-time clock of the test machines' component `rtc` (LIB-2 T2; O-3) | (support) | used by T6 to T13 |
| T6 | `HarnessHost`: topics, `state` | AC-5, AC-11, EC-5 | `host_tests.cpp`: `ac11_state_reads_library_and_product_topics`, `ac11_stream_gives_what_was_appended_since_last_read`, `ac5_notices_only_through_state`, `ec5_state_of_no_topic_is_refused` |
| T7 | `wait`, `key-down`, `key-up`, `ticks`, `capabilities`, `quit`; operations | AC-7, AC-9, EC-2, EC-7 | `commands_tests.cpp`: `ac9_wait_reports_time_passed_within_one_instruction`, `ac9_key_down_and_up`, `ac9_unknown_key_is_refused`, `ac9_ticks_gives_count_and_seconds`, `ac9_capabilities_with_kind_id_owner_and_places`, `ac9_quit_ends_serve`, `ac7_operation_runs_to_its_end_before_the_reply`, `ec2_wait_without_unit_is_refused`, `ec7_repeated_key_down_or_up_is_done_and_changes_nothing` |
| T8 | `readDotMatrix`, `readSegments`, `lcd` | AC-9, AC-10 | `display_text_tests.cpp`: `ac10_dot_matrix_rows_indicators_and_cells_not_text` (synthetic frames), `ac10_inverted_glyph_is_text`, `ac10_segment_positions_left_to_right_by_product_rule`, `ac10_display_off_says_so` (`test-dot`, key `off`), `ac9_lcd_reads_display_topic` |
| T9 | `audio`, `WavRecorder` | AC-9, EC-3, EC-6 | `audio_tests.cpp`: `ac9_audio_records_all_sound_also_of_an_operation`, `ec3_wav_complete_when_input_ends` (header sizes = data), `ec6_audio_on_machine_without_sound_is_refused` |
| T10 | `RunSetup`: run folder, settings, ROMs | AC-8, EC-1 | `run_tests.cpp`: `ac8_defaults_and_no_settings_file_read_or_written` (a fake home checked unchanged), `ac8_run_folder_empty_removed_and_in_no_reply` (a `test fail` notice that names a file in it), `ac8_traces_and_screenshots_go_to_the_run_folder` (`Paths::traceFolder`, `screenshotFolder`), `ac8_options_name_settings_save_and_output_folders`, `ac8_bundled_roms_through_the_lookup`, `ec1_missing_rom_ends_at_start_naming_files` |
| T11 | the date and time | AC-21 | `clock_tests.cpp`: `ac21_fixed_instant_by_default_at_build_and_after_operation`, `ac21_clock_option_names_instant`, `ac21_clock_host_hands_host_time` |
| T12 | determinism | AC-6 | `determinism_tests.cpp`: `ac6_two_runs_give_the_same_replies` (a script over every first command, `--json` and plain, with a wall-time sleep between the runs, compared byte for byte), `ac6_no_pacer_and_no_wall_clock` (the session is created with no pacer, and the host call is never made) |
| T13 | `cli::run`, process level | AC-2, AC-3, EC-3, EC-4 | `tests/cli/*.cmake` with `calcu_test`: `ac2_one_shot_runs_in_order_and_exits_zero`, `ac2_first_refusal_exits_nonzero_with_reason` (`wait 1s then test fail then ticks`: `ticks` not run), `ac3_serve_one_reply_per_line_until_quit`, `ec3_serve_ends_at_end_of_input` (with `audio`), `ec4_unknown_product_lists_products` |
| T17 | `display`: the raw display output (added with AC-23, user 2026-11-14) | AC-23 | `display_tests.cpp`: `ac23_dot_matrix_rows_of_ones_and_zeros`, `ac23_segment_positions`, `ac23_indicator_bits_in_descriptor_order`, `ac23_display_off`, `ac23_json_arrays` (test machines `test-dot`, `test-seg`) |
| T18 | `state-save`, `state-restore` (added with AC-24, user 2026-11-14): the generic capability kind machine state (`save() -> bytes`, `restore(bytes) -> refused or done`, opaque to the library) and a header the library writes (product, version, model, variant, plugged peripherals) to refuse a foreign file | AC-24 | `state_tests.cpp`: `ac24_restore_continues_identically` (test machine `test-seg` offers the kind), `ac24_restore_refused_for_other_configuration_changes_nothing`, `ac24_refused_without_the_kind`, `ac24_host_files_not_in_state` |
| T14 | product entries (§4) | (exit check of M-28) | `products/calc-u-59/tests/harness_tests.cpp`: `harness_drives_a_ti59` (`wait 2s`, `key-down`/`key-up` of a digit, `lcd`, `ticks`); `products/calc-u-1600/tests/harness_tests.cpp`: `harness_drives_a_sharp_machine` (the PC-1500A of the default start, D-1; the same with `lcd` rows). Frequent, `SKIP_UNLESS` the ROMs are present |
| T15 | Python helper (D-2) | (none) | `library/harness/python/tests/test_calcu_serve.py`, run as `uv run python -I …` by a CTest entry. Frequent |
| T16 | CI and docs | (plan M-28) | `calcu` built in both workflows on every desktop platform. The `library` job runs T1 to T13 |

Coverage: AC-1–13, AC-21, AC-22 (15 of 15) and EC-1–7 (7 of 7) appear in
T1 to T13.

## 7. Tools and agent knowledge

- **Today's tools.** M-28 moves none. They stay usable and unchanged
  until M-51 (Calc-U-1600) and M-59 (Calc-U-59).
- **Agent knowledge** (plan M-28: "a harness section for agents in both
  products' guidance"):
  - `products/calc-u-59/CLAUDE.md`: near lines 44-47, where `.ti59` runs
    and `tools/headless/` are named, add a short "Harness" section: how
    to build and run `calcu --product 59`, the first commands, `serve`,
    `--json`, and the fact that `tools/headless/` and `tools/agentic/`
    stay until M-59.
  - `products/calc-u-1600/CLAUDE.md`: the same, next to the test
    paragraph (line 16), with `pc1500_cli`/`pc1600_cli` staying until
    M-51.
- **No rename** in M-28. The "Harness" row of `docs/glossary-mapping.md`
  (code names `tools/headless/`, `tools/agentic/`; `pc1500_cli`,
  `pc1600_cli`, probes) gets `calcu` when the old tools go (M-51, M-59),
  with the memory files the plan names there.
- No skill names the harness yet. The Python helper (D-2) is the first
  thing a skill would point to.

## 8. Golden runs and risks

- **Golden runs.** M-28 touches no core. The replays of
  `products/calc-u-59/tests/golden-runs/` and
  `products/calc-u-1600/tests/golden-runs/` run unchanged in their
  Frequent suites. The harness does not replay them. A replay through
  `calcu` would be a later choice and is not part of M-28.
- **LIB-3's session API** (reconciled with the LIB-3 design, §9 S-list).
  `HarnessHost` uses LIB-3's names; S-6 rests on LIB-3's request `topics` (its A-8).
  If it lands in another shape, `HarnessHost` changes, and the protocol
  does not.
- **Determinism.** Sources of difference: the temp folder name (never in
  a reply, §3.1), hash order (none: ordered containers only), floating
  point (none in output), and the wall clock (none). T12 guards them.
- **Windows.** Standard input and output in text mode would turn `\n`
  into `\r\n`. `Main.cpp` sets binary mode (`_setmode`), and the reader
  drops `\r`. Temp folders use `std::filesystem::temp_directory_path`.
- **Linking.** `calcu` links both products' app and core targets: ZLIB,
  and libsharpdx for Calc-U-1600 (`cmake/CalcU.cmake:56`). Not under
  Emscripten. `library/harness/` must stay free of stdio and of the file
  system outside `Run.cpp`, so that the Swift app (through
  `calcu_library`, M-33) can use it with C59-5's TCP transport.
- **Finding bundled files from a build tree.** "As a frontend finds them"
  (AC-8) for a program run from `build/…` depends on LIB-12's lookup
  (O-2): `bundle()` gives a `roms::DirectoryBundle` of the ROM folder
  the product's tests use today; no new option.
- **Exception modes** (LIB-1 D-6): not affected, because `calcu` is not
  built for the Web.

## 9. Decided (best guess)

- **D-1 (was OQ-1; plan M-28 exit check).** The harness has no model
  option before LIB-18, so the Calc-U-1600 half of the check drives the
  PC-1500A of the default start; the PC-1600 is driven from M-35, when a
  model switch exists. Noted in M-28's commit.
- **D-2 (was OQ-2).** The Python helper is made in M-28 (T15), as the plan
  says; it has no AC, because the spec does not define it.
- **D-3 (was OQ-3 = LIB-12 OQ-4).** Calc-U-1600's `core/Yaml.hpp` moves
  into `library/formats/` in the first step that needs it, M-24 (LIB-12
  reads Calc-U-59's `.rom.yaml`), and serves both products and the
  harness; `parseValue` and `parseDuration` join it in M-28. Each format's
  reader checks its own `format-version`.
- **D-4 (was OQ-4; AC-21).** Fixed instant `2000-01-01T00:00:00`, no time
  zone; option `--clock <YYYY-MM-DDThh:mm:ss>` / `--clock host`.
- **D-5 (was OQ-5; AC-1, EC-4).** A run without `--product` is EC-4, also
  in a build with one product (one rule).
- **D-6 (was OQ-6).** As designed: the library app session runs on the
  product's machine; the product's app layer joins through
  `addToSession()` in M-31 and M-33 (keeps `harness.md` I-1, no adapter).
- **D-7 (was OQ-7; AC-10, AC-19).** From M-51 the Calc-U-1600 product puts
  the cursor cell into a product topic that its display rule reads, so
  `lcd` shows the cell under the cursor as `--lcd-text` does today. The
  lines carry this design's labels (`indicators:`, `not text:`), the same
  for every product (AC-4, AC-10); `parseLcdText` stays as the font source.
- **D-8 (was OQ-8).** A second `audio` completes the first file and starts
  the new one.
- **D-9 (was OQ-9).** `key-down`/`key-up` resolve keys as the `key` step
  does (other spellings, any case), because LIB-1 D-2 leaves spellings to
  presets and the harness.
- **D-10 (was OQ-10).** Two `Value` types: the harness converts LIB-3's
  payload values and writes their doubles with fixed decimals; LIB-3
  stays unchanged.
- **Not verified.** Whether `cmake_language(DEFER)` with
  `target_link_libraries` on targets of other directories works with
  every generator in CI (Ninja, Visual Studio, Xcode is not used).

**What this design takes from other designs** (reconciled with the
LIB-3, LIB-9, LIB-12, LIB-2, LIB-1 and C59-5 designs, 2026-11-14).
LIB-3 (`calcu::sessions`):
- S-1. Met: `AppSession(AppSessionSetup{hosting = Hosting::VirtualTime})`
  makes no host call and runs no pacer; the machine goes in through
  `context().machineSession().setMachine(build result, profile,
  soundRate)`.
- S-2. Met: `AppSession::advanceUnpaced(Ticks) -> Ticks`.
- S-3. Met: `send(ListenerId, Command) -> CallId`; one `ReplyMessage{call,
  done, reason, result}` to the sender only; in `Hosting::VirtualTime` an
  operation started by a command runs to its end inside `send()`, and
  its result is the reply (LIB-3 §3.5, AC-18, AC-19).
- S-4. Met: the commands `key-down`, `key-up` `{key}` (LIB-3 §3.4).
- S-5. Met: `ask(ListenerId, Request)` with the requests `ticks`
  (`EmulatedTimeReply{ticks, seconds}`) and `capabilities`
  (`MachineOffer{capabilities, plugged, places}`).
- S-6. Met: the listener is a `MessageSink` registered with
  `listen(MessageSink&, HostServices) -> ListenerId`, messages are
  delivered at once on the caller's thread, `TopicMirror` keeps them,
  and every payload has `toValue()`. The list of all topic names and
  their shapes before one is sent, needed for `state <topic>` and EC-5,
  is LIB-3's (A-8 there, the request `topics`): the harness reads it from the session.
- S-7. Met: `AppSessionSetup::hostTime` (LIB-3 A-7; LIB-2 A-4); the
  harness passes its fixed instant or `--clock`'s.
- S-8. Met: a listener with `HostServices{recordsSound = true}` gets
  every sample (LIB-3 §3.6).

Others:
- O-1. LIB-9 (`calcu::settings`): `SettingsFile::defaultsOnly`, `open`,
  `HostDirs`, `Paths::traceFolder`/`screenshotFolder` are in its design.
  The harness passes `FrontendKind::Qt6` (LIB-9 D-11).
- O-2. LIB-12 (`calcu::roms`): met by `DirectoryBundle`, `lookUp`,
  `describe` (LIB-12 §3; reconciled). `bundle()` returns
  `std::unique_ptr<roms::Bundle>`.
- O-3. LIB-2 (`calcu::peripherals`): `RealTimeClock::set` and
  `setClocksFromHost`; the test machines' component `rtc` with a
  real-time clock (LIB-2 T2, now in LIB-1 T4), used by T5 and T11.
- O-4. LIB-1: the test machines of `library/machine/tests/TestMachines.hpp`
  as listed in its T4 (`test-seg` without sound, `test-dot` with sound
  and an `off` key).
- O-5 (from C59-5's A-9): attach runs this design's commands as a second
  listener on an app session hosted in `Hosting::Paced`, with `wait`
  waiting for the app's host calls instead of `advanceUnpaced` (C59-5
  AC-14). Decided: C59-5's `AttachedHarness` uses `CommandRegistry` and
  registers its own `wait` handler; `HarnessHost` gets no time policy
  (the smaller change). Its lines follow §3.9.
