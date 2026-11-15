# LIB-1 Machine interface and descriptors: design
Status: approved by the user at the Part B/C1 boundary (2026-11-14)

## 1. Scope

| Column | Step | Tests from | Done in (plan §5.1) |
|---|---|---|---|
| Library | M-18 | AC-1–25, AC-42, AC-43; EC-1–8 | M-18 |
| Calc-U-59 | M-19 (with C59-1), M-25 (with C59-6) | AC-26–33 | M-25 |
| Calc-U-1600 | M-26 (with C1600-1) | AC-34–41 | M-26 |

- M-18 changes only `library/` and the CI workflows. No product code
  calls the new code until M-19.
- Split of the Calc-U-59 criteria between its two steps (§5.1 tests them
  in "M-19, M-25"): M-19 tests AC-26, 27, 28, 29 (keys, connectors,
  places, debug target as descriptor data), 30, 31 and 33 for a machine
  without peripherals; M-25 tests AC-32 and AC-33 with the PC-100C and a
  module plugged in. The media place clause of AC-33 and AC-40 names
  LIB-7's kind, so by plan §1 it passes in LIB-7's steps (M-55 for
  Calc-U-59, M-47 for Calc-U-1600), and LIB-1 59 / 1600 become Done there
  (§9, D-1).
- No AC of this spec lands outside these steps. Waits for: M-3 (FS 5).
  Result: idle is not a state of its own (`docs/walkthroughs/first-steps.md`
  §5, "Idle"), so AC-11 and AC-42 stand as written and the interface has
  key readiness only. M-19 also waits for M-8 (FS 2, 7) and FS 11. M-26
  waits for M-4, M-5, M-8 and M-23.
- Not here: attach and detach (LIB-2, M-23), the typed interfaces of the
  generic kinds (LIB-2, 5, 7, 8), the debug target interface (LIB-10,
  M-40), and how a TI or Sharp machine behaves behind the interface
  (C59-1, C1600-1).

## 2. Modules and files

All new, in `library/machine/` (namespace `calcu::machine`), part of
the one target `calcu_library`. See §2a for the convention.

| File | Holds |
|---|---|
| `library/machine/Ids.hpp` | the strong ID types (§3.1) |
| `library/machine/EmulatedTime.hpp`, `.cpp` | `Ticks`, `TickRate`, `EmulatedTime`, `Seconds` (§3.2) |
| `library/machine/Descriptor.hpp`, `.cpp` | `MachineDescriptor`, `PeripheralDescriptor` and their parts, `RomEntry` (§3.3) |
| `library/machine/Catalogue.hpp`, `.cpp` | `Catalogue`, `PlaceInfo` (§3.4) |
| `library/machine/Display.hpp` | `DisplayOutput`, `SegmentFrame`, `DotMatrixFrame`, `IndicatorOutput` (§3.6) |
| `library/machine/Capability.hpp`, `.cpp` | `Capability`, `KindId`, `CapabilityEntry`, `Owner`, `capabilityAs<K>`, `capabilitiesOf<K>` (§3.7) |
| `library/machine/Machine.hpp`, `.cpp` | the machine interface `Machine`, `KeyCall`, `Plugged` (§3.5) |
| `library/machine/Build.hpp`, `.cpp` | `BuildRequest`, `RomImage`, `PeripheralRequest`, `BuildResult`, `MachineBuilder`, `build()` (§3.8) |
| `library/machine/ProductData.hpp` | `ProductData`, the opaque payload a product hands through the library (§3.9) |
| `library/machine/Conformance.hpp`, `.cpp` | `checkAgainstDescriptor()`, used by the product tests of AC-33 and AC-40 (§3.10) |
| `library/machine/tests/TestMachines.hpp`, `.cpp` | the machines built for the tests: a test catalogue with two models (at different tick rates) and the test peripherals, its builder, and a product kind (§6, T4; LIB-2 adds to them in M-23) |
| `library/machine/tests/*_tests.cpp` | one file per area (§6) |

Changed:

| File | Change |
|---|---|
| `library/CMakeLists.txt` | `calcu_library` becomes a static library (today INTERFACE) with the sources of `library/machine/`; the test program `calcu_library_tests` and its CTest entry (label `frequent`); §2a |
| `.github/workflows/calc-u-59.yml`, `calc-u-1600.yml` | a job `library` that configures without either product and runs the library tests (AC-24; §9, D-5) |
| `CLAUDE.md` (root) | one line under "Key conventions" that points to §2a of this design for the library layout (§7) |

## 2a. Library layout

The convention every Part C1 design follows. A design that needs an area
not named here adds a folder in the same pattern.

**Rules**
- **One target.** The library is one CMake static library `calcu_library`
  (user, 2026-11-14), C++20 (`target_compile_features(calcu_library
  PUBLIC cxx_std_20)`, ADR 0005), defined in `library/CMakeLists.txt`,
  which today holds it as an INTERFACE target. Each step adds its area's
  sources to it (`file(GLOB ... CONFIGURE_DEPENDS library/<area>/*.cpp)`
  or a source list per area). A product links `calcu_library` (as today:
  `products/calc-u-59/CMakeLists.txt:52`,
  `products/calc-u-1600/cmake/CalcU.cmake:112-113`).
- **One folder per area**, headers and sources together, file names in
  PascalCase (`Machine.hpp`), as in both products: `library/machine/`,
  `peripherals/`, `sessions/`, `settings/`, `pacing/`, `harness/`,
  `roms/`, `formats/`, `typing/`, `shell/`, `web/`.
- Namespace `calcu::<area>`. Nothing at namespace `calcu` itself.
- Includes are rooted at `library/`: `#include "machine/Machine.hpp"`.
  `calcu_library` exports `${PROJECT_SOURCE_DIR}/library` as its PUBLIC
  include directory. No bare `#include "Machine.hpp"` from outside the
  folder, so a library header never collides with a product header of
  the same name.
- **Dependency direction** between areas is a rule, not enforced by
  CMake (it follows the graph of `docs/migration-plan.md` §2.1). An area
  includes only the areas it names here:
  - `machine`, `formats`, `settings`: the standard library only;
  - `pacing`: `machine`;
  - `peripherals`: `machine`, `sessions` (the host-link session part,
    LIB-2 and LIB-3 designs; settled at M-23, 2026-11-15);
  - `sessions`: `machine`, `pacing`;
  - `roms`: `machine`, `sessions` (the `roms-missing` topic);
  - `control`: `machine`, `sessions` (LIB-4 design §2; M-37);
  - `typing`: `machine`, `pacing`, `sessions` (LIB-5, M-36);
  - `presets`: `machine`, `formats`, `sessions`, `peripherals`, `typing`,
    `startup` (LIB-6 design D-1; M-38). Nothing includes it but `harness`
    and the products; `startup` declares the loader `presets` implements;
  - `paper`: `machine`, `sessions`, `formats`, `presets`, `startup`
    (LIB-8 design §2; M-39). Nothing includes it but `harness`, `qt6`
    and the products;
  - `debugger`: `machine`, `pacing`, `sessions`, `settings`, `formats`,
    `peripherals` (AC-16's clocks only), `presets` (the step place) and,
    through `presets`, `startup` (LIB-10 design §2; M-40). The interface a
    machine implements (`DebugTarget.hpp`) is in `machine`. Nothing
    includes it but `harness`, `qt6` and the products;
  - `media`: `machine`, `formats`, `settings`, `sessions`, `peripherals`,
    `presets`, `startup` (LIB-7 design §2; M-41; `control` not needed: the
    part computes what fits from the catalogue, its S-1 fallback). Nothing
    includes it but `harness`, `qt6` and the products;
  - `harness`: any area but `web`, `qt6` and `shell`;
  - `shell`: `machine`, `pacing`, `sessions`, `settings` (LIB-14 design
    §2.1; M-43). It reads the topics of LIB-4, LIB-6, LIB-7, LIB-10 and
    LIB-12 in their Value form (LIB-14 D-2), so it includes none of their
    areas; its tests build those Values from `control` and `presets`
    payloads. Nothing includes it but `qt6` and the products;
  - `qt6` (the Qt6 component set, its own target, below): any area but
    `web`, and Qt; `harness` only for harness attach
    (`SessionHost::openAttach`, LIB-14 §3.9, with `CALCU_ATTACH`);
  - `web` (C++ glue): `sessions`, `settings`.
  Review checks it; a new include against it is a finding.
- **Exceptions to the one target, and only these:**
  - the `calcu` CLI, an executable target in `library/harness/cli/`
    (LIB-11), linking `calcu_library`;
  - `calcu_web_glue`, a small INTERFACE target that lists the C++ sources
    of `library/web/glue/`, which each product's Web engine compiles
    with its own flags (LIB-15 §3.6);
  - the attach endpoint `library/harness/attach/` (C59-5) is compiled
    into `calcu_library` only with the CMake option `CALCU_ATTACH`
    (default OFF; on in Debug builds of the Swift app);
  - `calcu_qt6`, the Qt6 component set in `library/qt6/` (namespace
    `calcu::qt6`; LIB-8 D-1, LIB-14 D-1, D-16; created in M-39): a static
    library on `Qt6::Widgets` and `calcu_library`, defined only with
    `CALCU_FRONTEND_QT` when `find_package(Qt6 COMPONENTS Widgets)`
    succeeds, never in an Emscripten or iOS tree, so that `calcu_library`
    stays free of Qt for the Web and Swift builds. Its tests are one
    program, `calcu_qt6_tests` (`library/qt6/tests/*_tests.cpp` with
    `QtTestMain.cpp`, the one runner's `TEST`s under a `QApplication`),
    Frequent, offscreen. The components of LIB-7, LIB-10 and LIB-14 are
    added to the same folder, target and program; LIB-14 (M-43) adds the
    shell (`SessionHost`, `Shell` and its widgets), a second program
    `calcu_qt6_scenarios` (its AC-25 scenarios) and, with `CALCU_ATTACH`,
    the harness's test product to the test programs. The React component
    set is JavaScript in `library/web/components/` (`COMPONENT_SOURCES`
    of `library/web/build/dist.js`), not a C++ target.
- Library code throws no exceptions across its interface, needs no RTTI,
  starts no thread and takes no lock, except a host adapter at the
  boundary of ADR 0009 (the host link of a serial port, an audio
  callback). Results that can fail are return values.
- **Tests:** one library test program `calcu_library_tests`, sources
  `library/<area>/tests/*.cpp` (`<topic>_tests.cpp`), using the one runner
  (`library/testing/TestHarness.hpp`, ADR 0038) and linked with
  `calcu_test_runner`. It is registered in CTest under the same name with
  the label `frequent`. A test that needs another group (Longer,
  Checkpoint; `docs/test-groups.md`, ADR 0031) or runs only on some
  platforms (the pseudo-terminal tests, macOS and Linux; Playwright) gets
  its own CTest entry. Each step adds its tests to `docs/test-groups.md`.
- Test case names start with the criterion they check, lower case, so
  that `qa-report.md` can report per AC ID (reconciled with every C1
  design):
  - library tests of the library's own criteria: `acN_…`, `ecN_…`
    (`ac7_advance_runs_whole_instructions`, `ec2_unknown_key_is_refused`);
  - product tests of a product row: `<row>_acN_…` (`c59_1_`, `c59_5_`,
    `c59_6_`, `c1600_1_`);
  - product tests of a library feature's product column: `libN_acN_…`
    (`lib1_ac26_…`, `lib2_ac24_…`, `lib3_ac35_…`, `lib9_ac23_…`,
    `lib12_ac8_…`);
  - Playwright titles (LIB-15) start the same way, with a space:
    `ac1 the session runs in a worker…`;
  - support cases that check no criterion carry no prefix.
- Library tests use machines built for the tests, never product code
  (AC-24). The test machines (`library/machine/tests/TestMachines.hpp`,
  `.cpp`) are compiled into `calcu_library_tests`, and every area's tests
  use them.
- Product code that implements a library interface stays in the product
  (`products/<p>/core/`, `products/<p>/app/`), named in the product's
  style. Code only one product uses lives in that product (so C59-5's
  session thread and queues live in `products/calc-u-59/app/`).
- **Emscripten.** `calcu_library` is built in `build/web` like the
  product cores, with `-fwasm-exceptions` (as Calc-U-1600's core already
  is, `products/calc-u-1600/CMakeLists.txt:57-59`). Calc-U-59's Web engine
  moves from `-fexceptions` to `-fwasm-exceptions` in M-19, the first step
  that links library code into it (C59-1), so one `build/web` tree holds
  both engines (§8). `library/testing` stays outside Emscripten (as
  today).
- **XCFramework.** `products/calc-u-59/frontends/swift/build_xcframework.sh`
  merges `libcalcu_library.a` into `libCalcU59.a` with the core and app
  archives, and copies `library/**/*.hpp` to `headers/` keeping the area
  folders (`headers/machine/Machine.hpp`), from M-19 on (C59-1). Nothing
  else changes in the script when a later step adds an area.

**Areas of Part C1**

| Feature | Folder | Namespace | Status |
|---|---|---|---|
| LIB-1 machine interface, descriptors; LIB-2's attach members (`library/machine/Attach.*`) | `library/machine/` | `calcu::machine` | this design |
| LIB-2 peripherals, connectors, controls, serial port, real-time clock | `library/peripherals/` | `calcu::peripherals` | LIB-2 design |
| LIB-3 sessions, operations, messages | `library/sessions/` | `calcu::sessions` | LIB-3 design |
| LIB-9 settings and paths | `library/settings/` | `calcu::settings` | LIB-9 design |
| LIB-17 pacing | `library/pacing/` | `calcu::pacing` | LIB-17 design |
| LIB-11 harness (the `calcu` executable in `library/harness/cli/`); C59-5's attach endpoint in `library/harness/attach/` (`CALCU_ATTACH`) | `library/harness/` | `calcu::harness` | LIB-11, C59-5 designs |
| the YAML reader (Calc-U-1600's `core/Yaml.hpp`, moved in M-24, LIB-12 D-4); flow values and durations (M-28, LIB-11) | `library/formats/` | `calcu::formats` | LIB-12, LIB-11 designs |
| LIB-12 ROM lookup | `library/roms/` | `calcu::roms` | LIB-12 design |
| LIB-15 Web frontend shell (JavaScript in `library/web/src/`, tests in `library/web/tests/`; C++ glue in `library/web/glue/`, `calcu_web_glue`) | `library/web/` | `calcu::web` (C++ only) | LIB-15 design |
| LIB-8 paper output: the kind, the paper, the picture, the replies, the session part (M-39); the PNG writer in `library/formats/Png.*` | `library/paper/` | `calcu::paper` | LIB-8 design |
| LIB-7 media and save folder: the kinds behind one seam, names, the media kind (`MediaPlace`, `ModuleMedium`), the shelf, the save rules, the media part and topic, media in presets, the media place check of LIB-1 D-1 (`Conformance.*`; M-41) | `library/media/` | `calcu::media` | LIB-7 design |
| the Qt6 component set (`calcu_qt6`; LIB-8's paper panel from M-39, LIB-7's media picker and Name & Save dialog from M-41, then LIB-10, LIB-14) | `library/qt6/` | `calcu::qt6` | LIB-8 D-1, LIB-14 §2.2 |
| the React component set (classic scripts in `COMPONENT_SOURCES`, names prefixed `Calcu`/`calcu`/`Shell`; LIB-8's paper panel from M-39) | `library/web/components/` | (JavaScript) | LIB-8 D-1, LIB-14 §2.3 |

(Table reconciled with the C1 designs, 2026-11-14; the C2 rows from M-39.)

The C2 designs are expected to follow the same pattern (new folders in
`calcu_library`): `library/typing/`
(LIB-5), `library/presets/` (LIB-6), `library/media/` (LIB-7),
`library/paper/` (LIB-8), `library/debugger/` (LIB-10), `library/control/`
(LIB-4), `library/startup/` (LIB-18). A generic kind's typed interface
lives in the area of the feature that builds it: controls, serial port
and real-time clock in `calcu::peripherals`, typing support in
`calcu::typing`, media place in `calcu::media`, paper output in
`calcu::paper`.

## 3. Seams and interfaces

Signature sketches. `[[nodiscard]]`, `noexcept` and the like are left
out.

### 3.1 IDs (`Ids.hpp`)

```cpp
template <class Tag> struct Id {             // a string with a type
  std::string value;
  auto operator<=>(const Id&) const = default;
};
using ModelId       = Id<struct ModelTag>;        // "TI-59", "PC-1500A"
using ModelFamilyId = Id<struct FamilyTag>;
using VariantId     = Id<struct VariantTag>;      // "A04", "New"
using KeyId         = Id<struct KeyTag>;          // "2nd", "enter"
using PlaceName     = Id<struct PlaceTag>;        // "slot-1", "printer"
using ConnectorId   = Id<struct ConnectorTag>;
using PinOutId      = Id<struct PinOutTag>;
using ComponentId   = Id<struct ComponentTag>;
using PeripheralId  = Id<struct PeripheralTag>;   // "PC-100C", "CE-161"
using MediaPlaceId  = Id<struct MediaPlaceTag>;
using ControlId     = Id<struct ControlTag>;
using CapabilityId  = Id<struct CapabilityTag>;
using DebugTargetId = Id<struct DebugTargetTag>;  // "TMC0501", "LH5801"

struct PlacePath { std::vector<PlaceName> steps; };  // {"printer"}; deeper for a place on a peripheral (LIB-2 AC-9)
```

The IDs are strings because presets, the harness and the control bar
name them (`docs/formats/preset.md`). The advance loop never touches
them.

### 3.2 Emulated time (`EmulatedTime.hpp`): AC-10, EC-4

```cpp
using Ticks    = std::uint64_t;
using TickRate = std::uint32_t;                       // ticks per second, whole number (machine.md M-1)
using Seconds  = std::chrono::duration<double>;

class EmulatedTime {
public:
  EmulatedTime(Ticks ticks, TickRate rate);
  Ticks ticks() const;  TickRate rate() const;
  Seconds seconds() const;                            // ticks / rate
  static EmulatedTime fromSeconds(Seconds, TickRate); // rounds to the nearest tick
  friend std::strong_ordering operator<=>(const EmulatedTime&, const EmulatedTime&); // exact: ticks1*rate2 vs ticks2*rate1 in 96 bits
  friend bool operator==(const EmulatedTime&, const EmulatedTime&);
  friend Seconds operator+(const EmulatedTime&, const EmulatedTime&);  // in seconds (EC-4)
  friend Seconds operator-(const EmulatedTime&, const EmulatedTime&);
};
```

- No arithmetic on `Ticks` of two machines is offered. Ticks of one
  machine are added as plain integers by that machine's own caller.
- Exactness (AC-10): one year at 3,580,000 per second is about 1.13·10¹⁴
  ticks, below 2⁵³, so `seconds()` and `fromSeconds()` round-trip. The
  comparison avoids doubles: it multiplies a 64-bit count by a 32-bit
  rate into a 96-bit product, split by hand because MSVC has no
  `__int128`.

### 3.3 Descriptors (`Descriptor.hpp`): AC-1, AC-2, AC-3, AC-14, EC-6

```cpp
enum class RomEncoding { Binary, Text };
enum class ContentUnit { Bytes, Words, Rows };
struct RomEntry {            // the form is LIB-12 AC-1's; defined here because descriptors carry it from M-18
  std::string fileName; RomEncoding encoding; std::uint64_t decodedAmount; ContentUnit unit; std::string md5;
};
struct VariantDescriptor { VariantId id; std::vector<RomEntry> roms; };

struct SegmentDef   { std::string name; bool ownBrightness; };           // "A".."G", "DP" (own brightness: AC-15)
struct SegmentLayout { std::vector<SegmentDef> segments; };              // any number (machine.md M-2); names and order, no positions (D-3)
struct IndicatorDef { std::string name; bool hasBrightness; };
struct SegmentGeometry   { int positions; SegmentLayout layout; bool positionBrightness; };
struct CellGrouping      { int cellWidth, cellHeight, columns, rows; };
struct DotMatrixGeometry { int width, height; std::optional<CellGrouping> cells; };
struct DisplayDescriptor {
  std::variant<SegmentGeometry, DotMatrixGeometry> geometry;             // the display kind
  std::vector<IndicatorDef> indicators;
};

struct KeyDescriptor { KeyId id; std::vector<std::string> otherSpellings; };  // C59-1 AC-9; resolved by presets and the harness, not by the machine (D-2)

enum class PinOutLevel { ContactLevel, Functional };
enum class Takes { One, Chain };
enum class AfterChange { PowerCycle, Nothing };                          // LIB-2 AC-1; semantics LIB-2's
struct PinOutDescriptor   { PinOutId id; PinOutLevel level; Takes takes; AfterChange after; };
struct ConnectorDescriptor { ConnectorId id; std::string form; std::vector<PinOutDescriptor> pinOuts; };
struct PlaceDescriptor    { PlaceName name; ConnectorId connector; PinOutId pinOut; int takesAtOnce; };

enum class Depth { Separate, Folded, Functional };
struct ComponentDescriptor { ComponentId id; std::string standsFor; Depth depth; std::optional<ComponentId> foldedInto; };

enum class MediaPassage { PassThrough, Holding };
struct MediaPlaceDescriptor { MediaPlaceId id; std::string mediumKind; MediaPassage passage; std::vector<int> sides; };

enum class ControlForm { Key, Switch };
struct ControlDescriptor { ControlId id; ControlForm form; };            // LIB-2 AC-14

struct MachineDescriptor {
  ModelId model; ModelFamilyId family;
  std::vector<VariantDescriptor> variants;          // empty: no variant (EC-6)
  std::optional<VariantId> defaultVariant;          // the only one when there is one (EC-6)
  std::vector<RomEntry> roms;                        // used when `variants` is empty
  TickRate tickRate;
  DisplayDescriptor display;
  bool hasSound;
  std::vector<KeyDescriptor> keys;
  std::vector<ConnectorDescriptor> connectors;
  std::vector<PlaceDescriptor> places;
  std::vector<ComponentDescriptor> components;
  std::vector<MediaPlaceDescriptor> mediaPlaces;
  std::vector<DebugTargetId> debugTargets;
  const std::vector<RomEntry>& romsFor(const std::optional<VariantId>&) const;
  const PlaceDescriptor* place(const PlaceName&) const;
  bool hasKey(const KeyId&) const;
};

struct PeripheralDescriptor {
  PeripheralId id; std::string name;
  std::vector<VariantDescriptor> variants; std::optional<VariantId> defaultVariant; std::vector<RomEntry> roms;
  std::vector<PinOutId> fits;                       // the pin-outs it fits (LIB-2 AC-2)
  std::vector<ConnectorDescriptor> connectors; std::vector<PlaceDescriptor> places;   // its own (P-3)
  std::vector<MediaPlaceDescriptor> mediaPlaces;
  std::vector<ControlDescriptor> controls;          // its keys and switches; never machine keys (AC-14)
  std::vector<ComponentDescriptor> components;
  std::shared_ptr<const ProductData> productData;   // e.g. the parsed module definition (§3.9)
};
```

- Descriptors are plain values. They are built by product code without a
  machine and without ROM files (AC-1). `mediumKind` is a string that
  LIB-7 names ("magnetic-card", "floppy-disk", …).
- A peripheral's `PinOutId` and a machine's `PinOutId` are compared as
  strings, so pin-out IDs are unique within a product. The PC-1500
  memory slot, Slot 1 and Slot 2 are three IDs on one connector form
  (AC-36, AC-41).

### 3.4 Catalogue (`Catalogue.hpp`): AC-1, AC-3, AC-4

```cpp
struct PlaceInfo { const PlaceDescriptor* place; const ConnectorDescriptor* connector;
                   const PinOutDescriptor* pinOut; std::vector<PeripheralId> fitting; };
class Catalogue {
public:
  Catalogue(std::vector<MachineDescriptor>, std::vector<PeripheralDescriptor>);
  const std::vector<MachineDescriptor>& models() const;
  const std::vector<PeripheralDescriptor>& peripherals() const;
  const MachineDescriptor* model(const ModelId&) const;
  const PeripheralDescriptor* peripheral(const PeripheralId&) const;
  std::optional<PlaceInfo> place(const ModelId&, const PlaceName&) const;  // AC-4; `fitting` by the pin-out rule (LIB-2 AC-2)
  void addPeripheral(PeripheralDescriptor);       // a module definition read after the catalogue was built (AC-3)
  std::optional<PlaceInfo> place(const PeripheralId&, const PlaceName&) const;  // a place on a peripheral (LIB-2 AC-9; added by LIB-2 in M-23)
};
```

A product hands out its catalogue from its core (`const Catalogue&`
from one function, §4). The ROM lookup, the control state, the preset
loader and the harness read it. AC-4 asks "the descriptor": the
machine descriptor gives the connector (`MachineDescriptor::place`),
and the catalogue adds the peripherals that fit, because a module
definition read later must count too (LIB-2 AC-2).

### 3.5 The machine interface (`Machine.hpp`)

```cpp
enum class KeyCall { Done, UnknownKey };               // EC-2: refused, nothing changes
struct PlugEntry { PlacePath place; PeripheralId peripheral; std::optional<VariantId> variant; };
struct ConnectorPlugs { PlacePath owner; ConnectorId connector; std::vector<PlugEntry> plugged; };
struct Plugged { std::vector<PlugEntry> byPlace; std::vector<ConnectorPlugs> byConnector; };

class Machine {
public:
  virtual ~Machine() = default;
  // identity — AC-5, EC-6
  virtual const MachineDescriptor& descriptor() const = 0;
  virtual std::optional<VariantId> variant() const = 0;
  ModelId model() const;                                // descriptor().model
  // emulated time — AC-7, AC-8, AC-9, EC-8, EC-7
  virtual Ticks advance(Ticks atLeast) = 0;             // whole instructions; returns ticks run (>= atLeast, 0 for 0)
  virtual Ticks tickCount() const = 0;                  // monotonic for the machine's life, 0 when built
  EmulatedTime elapsed() const;                         // {tickCount(), descriptor().tickRate}
  // operations — AC-11, AC-42
  virtual void reset() = 0;  virtual void powerCycle() = 0;  virtual void coldBoot() = 0;   // return at once
  virtual bool isKeyReady() const = 0;
  // keys — AC-12, AC-13, EC-2, EC-3
  virtual KeyCall keyDown(const KeyId&) = 0;            // at the current tick count
  virtual KeyCall keyUp(const KeyId&) = 0;
  // display and sound — AC-15–19, AC-43
  virtual DisplayOutput readDisplay() = 0;              // one reader: the machine session (machine.md log, 2026-11-11)
  virtual std::size_t takeSound(std::span<std::int16_t> out) = 0;   // mono, at BuildRequest::soundRate; 0 without sound
  // connectors and capabilities — AC-20–23, AC-25, EC-5
  virtual Plugged plugged() const = 0;
  virtual std::vector<CapabilityEntry> capabilities() const = 0;
  virtual Capability* capability(const CapabilityId&) = 0;    // nullptr if not present now
  std::vector<CapabilityEntry> capabilities(const KindId&) const;  // filtered; empty if none (EC-5)
  virtual std::vector<DebugTargetId> debugTargets() const = 0;     // AC-25; the interface: LIB-10
};
```

- Threading and ownership (I-2, I-3, ADR 0009): a `Machine` is owned by
  one `std::unique_ptr` (the machine session, LIB-3) and used from one
  thread at a time. It holds no lock, starts no thread and reads no
  clock. Pointers from `capability()` stay valid until the next attach or
  detach (LIB-2) or until the machine is destroyed.
- `readDisplay()` is non-const: a core whose brightness is the duty
  since the last reading (C59-1 AC-13) restarts its window there. A
  second reading with no advance in between gives the same output
  (C59-1 EC-2).
- An advance that a debug target stops early (AC-7) is LIB-10's.
  `advance` keeps its signature, and LIB-10 adds how the stop is
  reported.
- Names reserved for LIB-2 (M-23), which adds them to `Machine` in the
  forms of the LIB-2 design §3.1 (reconciled with LIB-2 design):
  - public: `canAttach(const PeripheralRequest&, const Catalogue&) const
    -> std::optional<Refusal>`, `attach(PeripheralRequest, const
    Catalogue&) -> ChangeResult`, `detach(const PlacePath&, const
    PeripheralId&) -> ChangeResult`, `replace(const PeripheralId& old,
    PeripheralRequest, const Catalogue&) -> ChangeResult`;
  - `Refusal{reason, missingRoms}`, `ChangeResult{refused, follows}`;
  - protected hooks with defaults: `productRefusal`, `plug`, `unplug`,
    `followsNow`;
  - `SwitchPosition{ControlId, on}` and `PeripheralRequest::switches`
    (LIB-2 D-3).

  Their code lives in `library/machine/Attach.*` (D-7). LIB-10 (M-40)
  adds `debugTarget(DebugTargetId)`.

### 3.6 Display output (`Display.hpp`): AC-15–18, AC-43

```cpp
struct SegmentPosition { std::vector<bool> lit;                      // per SegmentDef of the layout
                         std::optional<float> brightness;            // 0..1, if positionBrightness
                         std::vector<std::optional<float>> segmentBrightness; };  // set for ownBrightness segments
struct SegmentFrame   { std::vector<SegmentPosition> positions; };   // order: the product's (C59-1 AC-13)
struct DotMatrixFrame { int width, height; std::vector<std::uint8_t> dots; };   // row-major, 0/1; cells from the descriptor
struct IndicatorOutput { std::string name; bool on; std::optional<float> brightness; };  // name copied: the display topic's payload outlives the machine (C1 simplification, 2026-11-15)
struct DisplayOutput  { bool on; std::variant<SegmentFrame, DotMatrixFrame> frame; std::vector<IndicatorOutput> indicators; };
```

No pixel format, glyph, colour or power state (AC-18, AC-43). Brightness
is the duty the core measures (decision log 2026-11-10).

### 3.7 Capabilities (`Capability.hpp`): AC-21–23, EC-5

```cpp
struct KindId { std::string_view value; auto operator<=>(const KindId&) const = default; };
class Capability { public: virtual ~Capability() = default; virtual KindId kind() const = 0; };
struct ComponentOwner  { ComponentId component; };
struct PeripheralOwner { PeripheralId peripheral; PlacePath place; };
using Owner = std::variant<ComponentOwner, PeripheralOwner>;
struct CapabilityEntry { KindId kind; CapabilityId id; Owner owner; };

template <class K> K* capabilityAs(Machine&, const CapabilityId&);          // K::kKind checked, then static_cast
template <class K> std::vector<CapabilityEntry> capabilitiesOf(const Machine&);
```

- A kind is a class that derives from `Capability` and has `static
  constexpr KindId kKind`. A generic kind is defined in its feature's
  area (§2a). A product kind is defined in the product's core, with
  the product's prefix (`"c59.program-memory"`, `"c1600.loader-memory"`).
  The library compares `KindId` values only and names no kind
  (capabilities.md I-1, AC-23).
- An ID is unique within a machine and the same for machines built
  alike (AC-21). The core composes it from its owner and the kind:
  `"<place-path>/<kind>"` for a peripheral, `"<component>/<kind>"` for a
  component, with an index only where an owner has two of one kind.

### 3.8 Building (`Build.hpp`): AC-5, AC-6, EC-1

```cpp
struct RomImage { std::string fileName; std::vector<std::uint8_t> bytes; };   // the file as bundled (D-4, LIB-12 §3)
struct PeripheralRequest { PlacePath place; PeripheralId peripheral; std::optional<VariantId> variant;
                           std::vector<RomImage> roms; std::shared_ptr<const ProductData> content; };
struct BuildRequest { ModelId model; std::optional<VariantId> variant;
                      std::vector<RomImage> roms; std::vector<PeripheralRequest> peripherals;
                      int soundRate;                                          // AC-19
                      std::shared_ptr<const ProductData> content; };          // handed at build (AC-6; TI-58C constant memory, C59-3)
struct BuildResult { std::unique_ptr<Machine> machine; std::vector<std::string> missingRoms; std::string refusal; };
class MachineBuilder { public: virtual ~MachineBuilder() = default;
                       virtual BuildResult build(const BuildRequest&, const Catalogue&) = 0; };
BuildResult build(const Catalogue&, MachineBuilder&, const BuildRequest&);
```

`calcu::machine::build()` checks the request against the catalogue
before it calls the product's builder: the model is known, the variant
belongs to it (EC-6), every `RomEntry` of `romsFor(variant)` and of each
requested peripheral has a `RomImage` of that file name. If one is
missing, nothing is built and `missingRoms` names the files (EC-1).
Checking the content of the images is LIB-12's (AC-3 there).

### 3.9 Product data (`ProductData.hpp`)

```cpp
class ProductData { public: virtual ~ProductData() = default; };
```

The library carries it without looking inside (capabilities.md Q-2
answer; machine.md M-7). Uses: a module definition behind a
`PeripheralDescriptor`, the content of a memory module or a TI-58C
constant memory handed in at build.

### 3.10 Conformance (`Conformance.hpp`): AC-33, AC-40

```cpp
std::vector<std::string> checkAgainstDescriptor(Machine&, const Catalogue&);   // empty: conforms
```

It reads only what the machine reports at run time and compares it with
the catalogue: `variant()`, the shape of `readDisplay()` (kind,
positions × segments or width × height, indicator names), that
`keyDown`/`keyUp` take every descriptor key and refuse a made-up one,
the places in `plugged()` and the fit of each peripheral
(`Catalogue::place`) and `debugTargets()`. LIB-7 adds the check of one
capability of the media place kind per media place in M-41 (D-1), as
`media::checkMediaPlaces(Machine&, const Catalogue&)` in `library/media/`
(the machine area names no kind); product tests call both. Every key it presses it releases
again. Product tests call it for every model and variant.

## 4. Product side

M-18 changes no product code. Calc-U-59's core already links
`calcu_library` (`products/calc-u-59/CMakeLists.txt:52`), and so does
Calc-U-1600's (`products/calc-u-1600/cmake/CalcU.cmake:112-113`). The
headers become visible to them, but nothing uses them yet.

**Calc-U-59 (M-19 with C59-1, M-25 with C59-6).** The machine class,
its file names and the adapters are C59-1's and C59-6's designs. This
design fixes only what the interface needs from them:

| Interface | From today's code |
|---|---|
| `Catalogue` of the three models (AC-26–31) | `products/calc-u-59/app/MachineModelInfo.hpp` (`cu59::ModelInfo`: ROM and constant file names, `hasCardReader`) and `core/MachineVariant.hpp` (`resonatorHz`: 455,000 / 384,000 / 394,000). Moves into the core as descriptor data. `ModelInfo::scanLoop`/`idleAndKeyDetect`/`cardWait` become core-internal (key readiness, C59-1 AC-18) |
| `advance`, `tickCount` | `TI59Machine::step()` weight × 32 ticks (C59-1 AC-1). Today `TMC0501::elapsedTicks()` counts weighted steps and starts again at a reset (`core/TMC0501.hpp:300`; C59-1 AC-3). The machine keeps its own monotonic count |
| `keyDown`/`keyUp` | `TI59Machine::pressKey(row, col)`. Key IDs to row/column from `app/KeyMatrix.hpp`, `app/KeyboardLayout.hpp:55-75` (C59-1 AC-9, AC-10) |
| `readDisplay` | `TI59Machine::takeDisplay()` → `DisplaySnapshot` (`core/TMC0501.hpp:36-49`), segments from `app/SevenSegment.hpp`. Segment layout A–G plus DP with its own brightness; indicator C with brightness (AC-27). `on` is always true (C59-1 AC-16) |
| `reset`/`powerCycle`/`coldBoot` | `app/StateApplier.cpp:95-115` (`softReset`, `cleanReset`) move into the machine (C59-1 AC-6, AC-19) |
| `takeSound` | returns 0, `hasSound = false` |
| `plugged`, places `slot-1`, `printer` | M-19: data only. M-25 (C59-6): the PC-100C and the 14 modules become peripherals; today they are `setPrinterConnected`, `loadLibrary` on `TI59Machine` |
| media place capability (card reader) | `insertCard`/`cardEject`/`isWaitingForCard`; the typed interface is LIB-7's; listed from M-55 (D-1) |
| debug target `TMC0501` | listed only. The debug calls stay behind C59-1's adapter for LIB-10 |

The four mutexes go (`core/TI59Machine.hpp`, lock policy comment;
ADR 0009). In M-19 the XCFramework starts to merge
`libcalcu_library.a` and copy the library headers (§2a), or the Swift app
does not link.

**Calc-U-1600 (M-26 with C1600-1).** The machine classes and adapters
are C1600-1's design.

| Interface | From today's code |
|---|---|
| `Catalogue` (AC-34–39, 41) | `app/MachineModel.hpp` (`shared::Model`, `PC1600RomVersion`, `CE1600PRomVersion`), `core/PC1500/PC1500Variant.hpp`, `core/Connector/MemoryCardCatalog.hpp` and `Qt6/resources/cards/*.card.yaml` (`compatible-hosts` → `fits`). Tick rates from `core/PC1500/PC1500Clocks.hpp:9`, `core/PC1600/PC1600Clocks.hpp:8`. Indicators from `core/Display/StatusLine.hpp:86-132` (14 / 18) |
| `advance`, `tickCount` | `PC1500Machine::runCycles`, `PC1600Machine::runCycles` (T-states; `toTStates` keeps the LH5803 inside, EC-7) |
| `keyDown`/`keyUp` | `pressKey(const std::string&)`/`releaseKey`, plus `setOnKeyPressed` behind the key ID `on` (C1600-1 AC-11) |
| `readDisplay` | `PC1500Machine::display()` + `isDisplayOn()`; `PC1600Machine::displaySnapshot()` (`clockEnabled`). Today both are flattened into `app/DisplayFrame.hpp` (`shared::DisplayFrame`), which `DisplayOutput` replaces |
| `takeSound` | `drainAudio` (`PiezoSampler`, 48,000, `core/Audio/PiezoSampler.hpp:38`); the rate comes from `BuildRequest::soundRate` |
| `reset`/`powerCycle`/`coldBoot` | `reset()`, `allReset()`, and the power cycle that 0.8.1 makes in `app/MachineSession.cpp` (`powerCycle`) moves into the machine |
| `plugged`, places, product kinds | `attachSlot1Card`, `attachCE150`, `attachCE158`, `attachCE1600P`, `attachHostDrive`, `systemBus()`; `debugInspect`, `memory()`, `drainCE158ParallelOutput`, `setHostDriveDirectory` as product kinds (C1600-1 AC-19) |
| debug targets | `LH5801`; `SC7852`, `LH5803` (`core/Debug/MachineDebugTargets.hpp`), listed only |

## 5. Adapters

M-18 adds no adapter: it adds new code and takes no old call behind the
seam.

The adapters of M-19, M-25 and M-26 belong to C59-1, C59-6 and C1600-1.
The plan names them per row (M-19: C59-6, C59-7, C59-2, C59-3, LIB-10,
C59-4; M-26: the ~60 places and 27 dispatches of
`products/calc-u-1600/app/MachineSession.cpp`). Two adapters touch this
feature's descriptor content, so they are listed here:

| Adapter | File | Wraps | Removed in |
|---|---|---|---|
| ROM file names read from the model info instead of the descriptor's `RomEntry` list | `products/calc-u-59/app/MachineModelInfo.hpp` (`romFiles`, `constFiles`), read by `app/RomLoader.cpp` | the ROM loading of 2.3.0 | M-24 (LIB-12 59: the lookup reads `romsFor()`) |
| Model and ROM version enums mapped to `ModelId`/`VariantId` | `products/calc-u-1600/app/MachineModel.hpp` (`shared::Model`, `PC1600RomVersion`, `CE1600PRomVersion`, `modelKey`) | settings keys and preset extensions that use the enums | M-49 (LIB-14 1600): the last users are the frontends and `app/ControlState`, `KeyLayout/FaceplateLayout`, `LcdLayout`. C1600-1's design names the adapter (`modelIdOf`, `modelOf`, `variantIdOf`) and the step (reconciled with C1600-1 design §5) |

## 6. Tasks

Library, M-18. The test files are in `library/machine/tests/`, the
program is `calcu_library_tests`, and the group is Frequent unless
the row says otherwise. Tests are written before the code of each task.

| Task | What | ACs/ECs | Test (file, case, group) |
|---|---|---|---|
| T1 | CMake: `calcu_library` as a static library with `library/machine/`, `calcu_library_tests` and its CTest entry; `-fwasm-exceptions` under Emscripten; the entry in `docs/test-groups.md` | AC-24 | `build_alone_tests.cpp`, `ac24_library_tests_run_without_products`; the CI job `library` (configure `-DCALCU_PRODUCT_59=OFF -DCALCU_PRODUCT_1600=OFF -DCALCU_FRONTEND_QT=OFF`, build, ctest) |
| T2 | `Ids.hpp`, `EmulatedTime` | AC-10, EC-4 | `emulated_time_tests.cpp`: `ac10_round_trip_every_catalogue_rate` (the five rates; counts 0, 1, rate−1, rate, k·rate, 2⁴⁰, 366 days and a fixed pseudo-random set below it), `ac10_whole_multiples_give_whole_seconds`, `ec4_compare_and_add_across_rates` |
| T3 | Descriptor types, `Catalogue`, `place()`, `addPeripheral()` | AC-1, AC-2, AC-3, AC-4, AC-14, EC-6 | `descriptor_tests.cpp`: `ac1_catalogue_gives_descriptors_without_machine_or_rom`, `ac2_machine_descriptor_states_every_part`, `ac3_peripheral_descriptor_and_one_added_later`, `ac4_place_gives_connector_and_fitting_peripherals`, `ac14_peripheral_controls_are_not_machine_keys`, `ec6_no_variant_and_single_variant` |
| T4 | Test machines (`TestMachines.hpp/.cpp`): model `test-seg` (3 positions, segments A–G + DP with its own brightness, indicator `C` with brightness, no sound, instructions of 3 and 5 ticks, a start-up of 50 ticks before key readiness); model `test-dot` (12 × 8 dots in cells of 6 × 8, indicators `RUN`, `BUSY`, a square-wave sound from the tick count, a display that the key `off` switches off); peripherals `test-printer` (a switch control and a capability of a test product kind) and `test-module` (ROM entry); product kind `"test.counter"`. `test-seg` and `test-dot` have different tick rates. LIB-2 adds in M-23 (its T2, reconciled with LIB-2 design): on `test-dot` a chain connector (`Takes::Chain`, places `a`, `b`, pin-out `test.chain`, `PowerCycle`) and a one-module slot (`Takes::One`); `test-dot` switched off by `off` answers `Nothing`; a component `rtc` with a real-time clock; peripheral `test-hub` (fits `test.chain`, offers place `jack`, pin-out `test.jack`, `Nothing`); peripheral `test-uart` (fits both models, a serial port, sends one byte every 10 ms of emulated time, counts operations); `test-printer` gets the keys `feed` and the switch `trace`; a product rule refusing `test-printer` with `test-uart`. For LIB-17 (M-21), `test-seg` gets a build option for a fixed instruction length and a loop of settable length (D-8) | (support for AC-5–25) | exercised by the cases below |
| T5 | `build()`, `BuildResult` | AC-5, EC-1 | `build_tests.cpp`: `ac5_machine_reports_model_and_variant_for_its_life` (also after the three operations), `ec1_missing_rom_names_the_file`, `ec1_missing_peripheral_rom_names_the_file` |
| T6 | Advance and tick count in the test machines (`test-dot` also runs an inner block on a second clock of its own, converted inside it) | AC-7, AC-8, AC-9, EC-7, EC-8 | `time_tests.cpp`: `ac7_advance_runs_whole_instructions_and_reports_them`, `ac8_tick_count_never_goes_back` (across reset, power cycle, cold boot, keys down; new machine at 0), `ac9_no_advance_no_change` (with a 50 ms wall-time sleep), `ec7_one_tick_whatever_clocks_inside` (the PC-1600 case itself: C1600-1 AC-1 in M-26), `ec8_advance_by_zero_runs_nothing` |
| T7 | Operations and key readiness | AC-11, AC-42 | `operations_tests.cpp`: `ac11_operations_return_at_once_and_end_at_key_readiness`, `ac42_key_readiness_only_after_start_up` |
| T8 | Keys | AC-12, AC-13, EC-2, EC-3 | `keys_tests.cpp`: `ac12_key_down_up_at_tick_counts`, `ac13_two_keys_down_together`, `ec2_unknown_key_is_refused_state_unchanged`, `ec3_repeated_down_or_up_changes_nothing` |
| T9 | Display output | AC-15, AC-16, AC-17, AC-18, AC-43 | `display_tests.cpp`: `ac15_segments_and_brightness_per_position_and_dp`, `ac16_dots_of_descriptor_size_in_cells`, `ac17_indicators_by_name_with_state`, `ac18_output_has_no_host_pixels_glyphs_colours` (the shape of the types, plus review), `ac43_display_off_shows_nothing` |
| T10 | Sound | AC-19 | `sound_tests.cpp`: `ac19_samples_at_named_rate_independent_of_slicing`, `ac19_machine_without_sound_gives_none` |
| T11 | Determinism | AC-6 | `determinism_tests.cpp`: `ac6_two_machines_built_alike_agree` (with different wall-time gaps between the calls) |
| T12 | `plugged()`, capabilities, `capabilityAs`, debug targets | AC-20, AC-21, AC-22, AC-23, AC-25, EC-5 | `capability_tests.cpp`: `ac20_plugged_per_place_and_per_connector`, `ac21_capabilities_listed_with_kind_id_owner`, `ac22_peripheral_capabilities_only_with_peripheral`, `ac23_product_kind_handed_out_like_generic`, `ac25_debug_targets_from_descriptor`, `ec5_absent_kind_gives_empty_list` |
| T13 | `checkAgainstDescriptor()` | (AC-33, AC-40, used from M-19) | `conformance_tests.cpp`: `conformance_passes_for_test_machines`, `conformance_names_a_wrong_key_list` |

Product columns. The tests come from this spec; the machine code comes
from C59-1, C59-6 and C1600-1.

| Task | Step | What | ACs | Test (file, case, group) |
|---|---|---|---|---|
| P1 | M-19 | Calc-U-59 catalogue: three models, rates, display, ROM entries, keys, connectors, places, components, media place, debug target | AC-26–31 | `products/calc-u-59/tests/descriptor_tests.cpp`: `lib1_ac26_models_and_tick_rates`, `lib1_ac27_segment_display_and_no_sound`, `lib1_ac28_rom_entries_per_model`, `lib1_ac29_keys_connectors_places_debug_target`, `lib1_ac30_card_reader_on_ti59_only`, `lib1_ac31_components_and_depth`; Frequent |
| P2 | M-19 | Running TI machine against its descriptor, no peripheral | AC-33 (without peripherals) | same file, `lib1_ac33_machine_conforms_without_peripherals` (`checkAgainstDescriptor`) |
| P3 | M-25 | PC-100C and the 14 Solid State ROM modules as peripheral descriptors; conformance with them plugged in | AC-32, AC-33 | same file, `lib1_ac32_pc100c_and_14_modules`, `lib1_ac33_machine_conforms_with_printer_and_module` |
| P4 | M-26 | Calc-U-1600 catalogue | AC-34–39, AC-41 | `products/calc-u-1600/tests/descriptor_tests.cpp`: `lib1_ac34_models_variants_rates`, `lib1_ac35_dot_matrix_indicators_sound`, `lib1_ac36_connectors_pinouts_places_debug_targets`, `lib1_ac37_module_slots_and_components`, `lib1_ac38_rom_entries_per_variant`, `lib1_ac39_peripheral_descriptors`, `lib1_ac41_module_fit_table`; Frequent |
| P5 | M-26 | Running Sharp machines against their descriptors, every model and variant, with the peripherals of AC-39 | AC-40 | same file, `lib1_ac40_machines_conform` |

Coverage: Library 27 ACs (AC-1–25, 42, 43) and 8 ECs (EC-1–8), all in T1–T12.
Products: AC-26–41, 16 ACs, in P1–P5.

## 7. Tools and agent knowledge

- M-18 moves no tool. `tools/headless/` moves onto the interface in
  M-19 (C59-1), and `pc1500_cli`, `pc1600_cli` and the probes in M-26
  (C1600-1). Those steps use this design's `Catalogue` and
  `build()` to build their machines.
- No rename in M-18: the new names are glossary terms already (machine
  descriptor, catalogue, place, key ID, tick, emulated time, display
  kind, capability, product kind, debug target). In
  `docs/glossary-mapping.md`, the "Code names" rows for model
  (`MachineModel`, `ModelInfo`) and the code names that follow the
  machine rows get their new code names in M-19 and M-26, with those
  rows' designs.
- Root `CLAUDE.md`: one line in "Key conventions" that library code
  follows §2a of this design (path, namespace, target, tests). This keeps
  agents from inventing a layout per module.
- Knowledge that changes with the machine rows, per the plan:
  `products/calc-u-59/.claude/skills/calc-u-59-core/SKILL.md` and
  `products/calc-u-59/reference/CoreArchitecture.md` (M-19, M-25);
  `products/calc-u-1600/docs/developer/PC1600-Core-Limitations.md`,
  `Shared-Architecture.md` and `products/calc-u-1600/CLAUDE.md` (M-26).
  Each says where the descriptor content lives and that the app layer
  reaches the machine only through `calcu::machine::Machine`.

## 8. Golden runs and risks

- **Golden runs.** M-18 touches no core, so both products' replays run
  unchanged in their Frequent suites
  (`products/calc-u-59/tests/golden_runs_tests.cpp`,
  `products/calc-u-1600/tests/golden_runs_tests.cpp`). In M-19 the 17
  runs of `products/calc-u-59/tests/golden-runs/` replay through
  `Machine`, the printer and module runs again in M-25, and in M-26 the
  20 runs of `products/calc-u-1600/tests/golden-runs/` do. A difference
  is a finding, never a new reference.
- **TI tick unit.** Today the TI count is weighted steps (1 or 4). A tick
  is 32 times that. The golden scripts' `run <seconds>` must turn into
  the same number of instructions through `EmulatedTime::fromSeconds`
  at the resonator rate. Otherwise every TI run differs by rounding.
- **Sharp power cycle.** It was not recorded as a golden run (FS 2
  result: 0.8.1 does it in `MachineSession`). M-26 has no reference for
  `powerCycle()` beyond C1600-1 AC-6.
- **Emscripten exception modes.** Calc-U-59's Web engine builds with
  `-fexceptions` today (`products/calc-u-59/CMakeLists.txt:76-82`) and
  Calc-U-1600's core with `-fwasm-exceptions`
  (`products/calc-u-1600/CMakeLists.txt:57-59`), both in one `build/web`
  tree. `calcu_library` is compiled once there with `-fwasm-exceptions`;
  in M-18 no Web engine references its code yet, and M-19 moves
  Calc-U-59's Web engine to `-fwasm-exceptions` (§2a, D-6). The Web smoke
  and Playwright runs of Calc-U-59 in M-19 check that its file errors
  still reach the user.
- **XCFramework.** `products/calc-u-59/frontends/swift/build_xcframework.sh:49-54`
  merges named `.a` files. M-19 adds `libcalcu_library.a` and the header
  copy of §2a (C59-1 §2), or the Swift app fails to link. Later steps
  change nothing there.
- **Allocation in the hot path.** `advance` must not allocate. IDs and
  vectors appear only in descriptor, plugging and capability calls, and
  in `readDisplay` (60 Hz).
- **Interface churn.** LIB-2 and LIB-10 add members to `Machine`
  (§3.5). Each such step rebuilds both products' machines. The reserved
  names keep that additive.

## 9. Decided (best guess)

- **D-1 (was OQ-1; AC-33, AC-40).** The media place clause names LIB-7's
  kind, so by plan §1 it passes in LIB-7's steps (M-55 for Calc-U-59,
  M-47 for Calc-U-1600) and LIB-1 59 / 1600 become Done there; no
  placeholder kind in M-18. Plan §5.1 still shows M-25 / M-26 as "Done in"
  for LIB-1 and follows this.
- **D-2 (was OQ-2; AC-12, EC-2).** The machine takes the key IDs only;
  other spellings are descriptor data that presets and the harness
  resolve (decision log of C59-1: "the machine takes the 45 key IDs
  exactly").
- **D-3 (was OQ-3; AC-2).** The segment layout carries names, order and
  the own-brightness flag, no positions: no spec gives a form for them.
- **D-4 (was OQ-4; AC-5).** The machine gets the ROM file as bundled
  (LIB-12 §3); the core decodes.
- **D-5 (was OQ-5; AC-24).** A `library` job in each product workflow; no
  third workflow (ADR 0004 stands).
- **D-6 (was OQ-6).** Both Web engines use `-fwasm-exceptions`, so one
  `build/web` tree and one `calcu_library` serve both; Calc-U-59's Web
  moves in M-19 (§2a).
- **D-7 (was OQ-7 = LIB-2 OQ-2).** The attach members live in
  `library/machine/Attach.*`; with one library target the question of
  which target holds them is gone.
- **D-8 (was OQ-8).** `test-seg` gets a build option for a fixed
  instruction length and a settable loop, used by LIB-17's tests; no third
  test model (fewer test machines).
- **Not verified.** That `std::span`, `<=>` and `std::chrono` as used
  above build with the CI compilers of every workflow. Both products
  build C++20 today (M-14), but they use no `<=>`. M-18's CI run shows it.
