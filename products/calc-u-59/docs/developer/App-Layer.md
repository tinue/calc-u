# App Layer (`app/`)

`app/` holds everything the frontends need that is neither hardware
emulation nor drawing. It sits between `core/` (the emulated hardware, see
`Core-Architecture.md`) and the three GUIs — the two apps and the web client
(`Web-Client.md`):

```
frontends/swift/App/ (SwiftUI) ── frontends/swift/Bridge/ (ObjC++) ─────────┐
frontends/qt6/app/ (Qt Widgets) ──────────────────────┼── app/ (C++20, std only) ── core/
frontends/web/ (React) ── frontends/web/bindings.cpp ───────────┘   (WebAssembly, no threads)
```

- **C++20, standard library only** — no Foundation, no Qt, no
  iCloud. CMake builds it as C++20, the Xcode project as gnu++20.
- **No locations.** `app/` never resolves where files live: it takes paths
  (`std::filesystem::path`), and ROM files only through the library's ROM
  lookup over a `calcu::roms::Bundle` the frontend makes (Swift: the
  bundle's resource folder; Qt: the `:/roms/` qrc, `QtResourceBundle`; Web:
  `/bundle` in Emscripten's file system; tools and tests: `roms/`).
- **One thread.** Everything runs on the app session's thread; `app/`
  has no thread and no lock (the Swift frontend's thread and its locks are
  the library's host adapter `shell/SessionThread`).
- **Namespace `cu59`.** Headers include the core relatively (`../core/...`)
  and the library by area (`"media/MediaPart.hpp"`).

The Qt app calls `app/` directly. The Swift app goes through the ObjC++
wrappers in `frontends/swift/Bridge/`, which only translate types (§ "Bridge Layer" under "The Swift frontend").

## Modules

| Module | What it does |
|---|---|
| **Orchestration and runtime** | |
| `TIAppPart` (`TIAppPart.cpp`, `TIAppPartFlows.cpp`) | Calc-U-59 in the library's app session: everything a frontend does with a calculator that isn't drawing (below); the library's parts it brings (`parts::LibraryParts`), the interim topics and commands, `makeAppSession` |
| `TIProductMachines` | What Calc-U-59 hands the library's machine control: the catalogue and builder, the built-in default of each model (the Master Library module, no printer; a TI-58C with the constant memory file's content), the pacing profile, no sound, the 25 s readiness bound |
| `TIPresetRules`, `TIPresetKeys`, `TIPresetCommands` | The TI rules of the library's preset loader (idle, `expect:`, `screenshot:`, `slot-1` by `name:`), the sections `partition:`, `program:`, `data:`, `cue-card:` and the step `printer-trace:`; the commands `c59.partition`, `c59.program`, `c59.data-registers` (`ProgramMemoryPart`) |
| `TIDisplayPicture` | The classic segment picture a preset's `screenshot:` writes |
| `ConstantMemoryKeeper`, `ConstantMemoryFile` | The TI-58C's `ti58c.memory.yaml` (constant memory format 1): what a TI-58C built without a preset gets, when the file is written, the notices |
| `TIFileAccess`, `PacingProfiles` | The card and `ti58c.memory.yaml` files on the save folder (read at each use: a change in Settings applies at once); the TI pacing profiles |
| `TIShellProduct` | What Calc-U-59 gives the library's shell, one source for Qt6, the Web and (through the bridge) Swift: the name, help address, About texts and links, the preset filter, the module slot's pin-out, the two card items (`arm-card`, `arm-fresh-card` on the media place `card-reader`); the settings dialog per frontend and layout (`settingsSpec`); the faceplate's hit map in base-image pixels (`faceplateHitMap`, `faceplateMapValue`) |
| `TIWebPart` | What the Web page draws, as `c59.web.*` topics and product requests (`c59.shell-product`, `c59.faceplate-map`; the LED shapes, the debug tabs' headers) |
| `TILevelRules` | The levels `program` and `cpu` of the run control: where a pause lands (a step boundary, or 50000 weights), one step, pause-on-start |
| `TIDebuggerSetup` | The `DebuggerPart`'s set-up: the rules, the trace session on `TITraceRecorder`, the trace folder (none on the Web: hand-over), the panel views |
| `TIPrinters` | The PC-100C with its paper description, for the `PaperPart` |
| `TITyping` | The TI typing profile per model (100 ms of emulated time, held until the ROM read it), the keyboard map (`TIKeyboardMap`), the names `key` takes; handed to the library's `TypingPart` |
| `FrameSampler`, `DisplayPov` | The 60 Hz frame (one sampler per machine, made at `machineChanged`): display, C indicator, card auto-eject; `DisplayPov`: LED brightness per position from the core's duty cycles (persistence of vision) |
| **Debugger** | |
| `TIDebugPart` | The debug panel below the frontends: a session part with the six product topics `c59.calculator-debug`, `c59.program-listing`, `c59.cpu-debug`, `c59.heatmap`, `c59.asm-overlay`, `c59.debug-view`, the commands `c59.show-calculator-debug`, `c59.show-cpu-debug`, `c59.heatmap-reset`, `c59.dump`, `c59.asm-load`, `c59.asm-run`, `c59.asm-clear`, and the product entries `c59.asm-run`, `c59.calculator-tab` (given by `TIControlRules`); `freezeButtons` (FREEZE and FREEZE ON START of a tab) |
| `TIControlRules` | Calc-U-59's side of the library's control state: the debug panel's product entries (`TIDebugPart`) and the bar's card controls, `c59.card-controls` (`{controls: [{id, icon, label, name, available, reason}]}`, while the control state lists `card-reader`): Arm Card and Arm Fresh Card (both "Crd") while no card passes, following the reader's `insert` (disabled with its reason), Eject Card while one does (`cardControls`, `cardControlsOf`); every GUI draws them from it |
| `TIFileKinds` | Calc-U-59's file kinds of the library's `open`: `asm-overlay`, `keystroke-program`, `rom-module-file`, `constant-memory-file`; `tiPresetExtensions`, `programArguments` (the Qt6 start file) |
| `DebugModel` | The panels' data: CALCULATOR snapshot and listing, CPU history (TIDebugPart owns it) |
| `DebugSnapshots` | Builds the CALCULATOR snapshot and listing and the CPU history entries |
| `TIChrome` | The GUIs' look and layout values (the library's `shell::Chrome`, dark only): the colours by name, the panel header and button-bar heights, the inset, the pill's shape, the bar, the printer column's width range (desktop, embedded), the view rule's numbers for the library's `shell::view` (the printer column's least room beside the faceplate, none below it; the debugger's least width and the window's least height for it), the text scale with the faceplate; every GUI reads it, the Web through `c59.shell-product` |
| `DebugText` | The panels' shared text and colours; `mobileTopMargin` (iOS) |
| `DebugLog` | The RUN LOG tab's view (`RingLog`, the last 2000 lines; kept by each frontend) and the Vars / SCOM / Prog / Memory dumps |
| `RomHeatmap` | How the heatmap is drawn: the grid, heat steps and colours (the counts are the core's `c59.rom-heatmap`) |
| `ModuleImage` | PRG SOURCE decoding, the module's program table, source labels |
| `AsmOverlay` | The reader of the ASM overlay format 2.0 (`readAsmOverlay`) |
| `TraceWriter` | The records of the binary trace format v2 (`docs/formats/trace.md`), encoded into a buffer |
| `TITraceRecorder` | The trace session's records of a TI machine: the frames of `c59.trace-records` as `TraceWriter` records, no key or card records |
| **Files** | |
| `Keycodes` | The keycode notation of `program:` and keystroke program files (`docs/formats/preset.md` §4.1) |
| `CueCardYaml` | The stand-alone cue card block of a preset and of the constant memory file (`docs/formats/cue-card.md`) |
| `TIMagneticCardKind` | The kind `magnetic-card` of the library's media part and its codec for magnetic card format 3 (`.magnetcard.yaml`, `docs/formats/media.md` §1); the card's label (name, description, cue card); the fitted card's cue card |
| `TIMagneticCardsKey` | The preset header key `magnetic-cards:` (in-file cards, `docs/formats/preset.md` §3.1) |
| `ModuleCueCards` | The module cue cards from the `cue-cards:` of a module file (`docs/formats/modules.md` §2.3) |
| `ProductRoms` | The ROM lookup of Calc-U-59 (`lookUpRoms`, the full set), `buildMachine` from the `RomSet` (the core decodes), a module's ROM image |
| `ModuleCatalog` | The module list from the module files (`allModuleMetadata`), a module's file name, slot-1's listing for the media part (`tiModuleListing`) |
| `CueCard` | Cue-card model, encode/parse, spans, layout, geometry, inks, which card shows |
| **Tables and drawing rules** | |
| `MachineModelInfo` | Per-model facts in a table keyed by the catalogue's `ModelId` (`kModelFacts`, `info`, `findModel`): display name, RAM, card reader, the trace file's model code and name (the tick rate is the descriptor's, key readiness the machine's) |
| `KeyIds` | Matrix code ↔ key ID of the machine's descriptor |
| `KeyboardLayout` | Canvas geometry, key rects, widened hit test; `PointerTracker` (hold the display for full speed, one held key, drag between keys — Qt and web) |
| `KeyNames` | Keycode mnemonics, operand-step grammar |
| `SevenSegment` | 7-segment pattern of every display glyph |
| `LedGeometry` | Segment polygons (modernized), dot grids (classic), decimal point, style parameters |
| `Bcd` | Register BCD encoding, register lines, `parseDecimal` (a `data:` value) |
| `Settings` | The settings file `Calc-U-59.ini`: `settingsDecl()` with every key, default and accepted value (read and written through the library's `calcu::settings::SettingsFile`), and typed calls for what a frontend reads itself: the Qt6 window state (`windowState`) |
| `TextUtil` | String helpers that behave as Foundation's (`.newlines` splitting, whitespace trimming, `Int(_:radix:)`), edge cases included |

Each header's top comment says what the module is and where its format or
rules are documented.

## The app session (`TIAppPart`)

Every frontend hosts the library's app session (`calcu::sessions::AppSession`)
made by `cu59::makeAppSession` (`app/TIAppPart.hpp`) or, on the Web,
by `frontends/web/bindings.cpp`, both on the library's
`calcu::shell::makeHostSession` (the library's `SettingsPart` over the host's
settings file, the order, the start), for the library's Web Worker host
(with `TIWebPart` behind the part). The session holds the machine session
(the machine, the library's pacer with the TI profiles of
`app/PacingProfiles.hpp`) and Calc-U-59's parts: `TIAppPart::sessionParts()`
gives the library's parts in their order (`parts::LibraryParts`, as
Calc-U-1600's `SharpAppPart`): `TypingPart`,
`PeripheralsPart`, `PresetsPart` (on `TIPresetRules` and the bundle
of `examples/`), `StartupPart` (on `TIProductMachines`),
`OpenPart`, `PaperPart` (on `tiPrinterPapers()`),
`DebuggerPart` (on `tiDebuggerSetup()`); then `ProgramMemoryPart` (the
`c59.partition`, `c59.program`, `c59.data-registers` commands) and `TIAppPart`.
`makeAppSession` sends the library's `start` once (the startup model with its
startup preset, else its built-in default); the Web page sends `start
{model, preset}` itself. A frontend sends commands (the library's: `start`,
`model`, `reset`, `cold-boot`, `attach`, `detach`, `load-preset`,
`speed`, `suspend`, `continue`, …; `cu59::commands`, `c59.*`) and gets
messages: the library's topics (`display`, `busy`, `notices`, `log`,
`papers`, `paper PC-100C`, `run-state`, `trace`, …) and the product
topics `c59.calc-indicator` (the LED frame of the frame sampler and the C
indicator), `c59.cue-card` (the cue card
shown, whether the fitted card is on show, its lit corner). The PC-100C in
`printer` and its PRINT, TRACE and ADV (labels PRINT/PRN, T, ADV; positions) are the library's
`control-state` (`places`, `controls`). The part sets them after
each flow from what it left, and the session sends one when its value
changed (their payloads compare by content). Their value forms are fixed:
`calcu state` reads them. Every frontend draws the model, the module, the
printer and the card controls from `control-state`; full speed is the
library's `speed` command and `busy` topic. Then
the debug panel's product topics (`TIDebugPart`). Qt6
calls the session on its GUI thread, Swift on `calcu::shell::SessionThread`, the
Web in its worker (§ "Threads"); each talks to it through the library's
`calcu::sessions::Listener`. The session owns the parts
(`AppSessionSetup::owned`) and lets them go first.

The machine is built by the library's machine control and held by the machine
session; `TIAppPart` never builds or owns one. At `machineChanged` it
makes a new `FrameSampler` for the session's machine, reads the module, the printer and
the PC-100C's TRACE switch from `Machine::plugged()` and its controls (no copies of its own), fits the own cue
card the build gives (a preset's `cue-card:`, or the constant memory
file's) and tells `ConstantMemoryKeeper` the build. What stays the part's:
the PC-100C's TRACE position, the cue card
shown. The debug panel's data and the ASM overlay are
`TIDebugPart`'s (the log level is the session's), a part of the same session. Every
pause, step, breakpoint and trace is the `DebuggerPart`'s, the paper the
`PaperPart`'s. An error is a notice ("title: message"); what the calculator
ran (a reset, the debugger's diagnostics and dumps) is a line of the
session's `log`; how the app ran (a file that does not read or write, with
its path and cause) is a line of the app log (`SessionContext::appLog`); the
card and `ti58c.memory.yaml` files go through `TIFileAccess` in the save
folder.

### The 60 Hz frame

After each host call (`TIAppPart::afterCall`):

1. **Frame** (`FrameSampler`): each position's glyph and brightness
   (`DisplayPov`), the C indicator's opacity (it follows the core's drive at
   once, holding only a single dark frame), core log lines, and card
   auto-eject (the core executed `CRD_OFF`; written data
   is saved).
2. **Debugger** (`DebugModel::tick`): the CALCULATOR snapshot while its panel
   shows; the CPU history and heatmap while the CPU panel shows or the
   machine is paused. At every settle `TIDebugPart::follow` reads the run
   control: a pause or step that landed gets the frozen view of where the
   machine stands (rebuilt when the tick count moved), and the core's frame
   ring is on while the CPU tab shows, the machine is paused or a start is
   armed. (The trace is pumped by the library's trace session inside the
   gate.)
3. **Slower checks**: the cue card's library program (2 Hz), the LOG text
   (1 Hz); the TI-58C's constant memory file (`ConstantMemoryKeeper`, at
   the latest 0.5 s after a change).

Then only the interim topics of what changed are published.

## Threads

The app layer has no thread of its own but one host adapter:

| Frontend | Where the session runs |
|---|---|
| Swift | the library's native threaded host `calcu::shell::SessionThread` (`library/shell/SessionThread.*`): builds, calls about 60 times a second and destroys the session on one thread (QoS user-initiated). Posts in one FIFO, messages out in one FIFO to the main thread, one wake per batch. In Debug builds for the simulator it also hosts harness attach (`library/harness/attach/`) |
| Qt6 | the GUI thread: a 60 Hz `QTimer` calls the session |
| Web | the library's Web Worker (`library/web/src/worker.js`): a 60 Hz `setInterval` calls the session through the library's `WorkerHost` |

Nothing else in the app layer locks: `FrameSampler`,
`TIAppPart`, `TILevelRules` and `TITraceRecorder` are single-threaded.

### The Swift frontend's threads

| Thread | What runs there |
|---|---|
| Main | SwiftUI, every `TIAppSession` call (each posts a command and returns at once), `SessionDelegate` callbacks, all `@Observable` writes |
| Session thread (`calcu::shell::SessionThread`) | The app session: built, called about 60 times a second, destroyed there; the machine, the presets, the card and `ti58c.memory.yaml` files (through `NSFileCoordinator`) |
| Attach I/O thread (Debug, simulator) | `calcu::harness::AttachEndpoint`: the socket on 127.0.0.1; lines go to the session thread |
| Global utility queue | `CardStorage.warmUp()` before the session is made — `url(forUbiquityContainerIdentifier:)` blocks and returns nil on the main thread |

#### Messages, not a refresh timer

There is no display timer. The session thread wakes the main thread once
per batch of messages (`dispatch_async`); `TIAppSession` keeps the last
message of each topic and tells the view model which topics changed
(`TIAppSessionDelegate.changed(_:)`, `TIChange`). `apply(_:)` reads only
those and assigns a property only when it differs, because `@Observable`
notifies on equal writes too.

On `.background` the app sends suspend (`suspend(completion:)`: carried out
— the TI-58C's memory written — then the thread stops calling, inside
a background task); on `.active` it sends continue. A debug freeze survives.

#### Harness attach (Debug)

A Debug build for the simulator opens its session to the harness
on 127.0.0.1, the port of the launch argument `--attach-port N`, else
59059 (`openAttach(withArguments:)`); `calcu attach [--port N]` on the Mac
that runs the simulator drives it, and `Calc-U-59UITests/HarnessClient.swift` from an XCUITest.
Release and device builds have no attach; the simulator needs no
entitlement for it, so every configuration signs with `Calc-U-59.entitlements`.

### Key input

Every key goes through the library's typing engine (`library/typing/`):
host keys (`host-key-down`/`-up` with what the host reported),
on-screen keys (`screen-key-down`/`-up`), focus loss (`focus-lost`), the
harness's `key`. The engine is a slice hook of the machine session, so its
holds and waits are counted in emulated time and land on their tick
whatever the pacing. A preset's `key:` step is pressed through the same
engine (origin Preset; `typing::typeAndWait`), and the loader waits
for idle after it.

The pause and the steps are the library's run control, the
machine session's gate, with the TI's levels (`TILevelRules`). A pause or
a program step waiting to land runs on inside the host call that asked for
it (`TIDebugging::setRunOn`, at most 50000 weights), not in paced emulated
time. Details: `Debug-API.md` § "Freeze /
Step Controls".
Core locking: `Core-Architecture.md` § "Threading Model".

## Emulation Loop and Timing

Every frontend: the library's pacer in the machine session, with the TI
profiles (`app/PacingProfiles.hpp`): a fixed 20 ms slice of the model's
tick rate and jittered slice ends (for the TI-58's error blink). Full
speed is paced at regular speed while the machine waits for input (key
readiness, or on the TI-59 a `Write` waiting for a card; a printer wait
does not count), as on every product; an operation (a preset, a reset)
never reports waiting and runs as fast as the host allows. A call drops a lag over 100 ms; full speed runs at most
14 ms of host time per call. The long-run rate is exact.

`step()` returns the cycle weight in the low 8 bits (`kWeightMask`). Bit 29
marks a step boundary (a `program` pause or step lands on it). Bit 30 marks a confirmed
hardware poll (a key or card-switch wait; not a printer BUSY wait, which is
counted in emulated cycles, so full speed speeds up printing). Bit 31 marks
a breakpoint of the core's own (`TRACE_BREAKPOINTS`; the tests
only). The run control's breakpoints (set through
the harness) stop the advance before the instruction (`TIDebugging`); an
armed start that fires stops it after one, and the run control pauses
there.

## Key Input Coordinate Systems

| Layer | Coordinates |
|---|---|
| Frontend hit test (`KeyboardLayout`) | Visual row 0–8, col 0–4 from the top left |
| `matrixCode(row, col)` (`KeyIds`) | `(row+1)×10 + (col+1)`, 11–95 — the key positions of `docs/formats/preset.md` §8.1 |
| `keyIdAt(code)` (`KeyIds`) | the descriptor's key `(row−1)×5 + (col−1)`: a key ID |
| `Machine::keyDown(keyId)` (`TIMachine`, `core/TIKeys.hpp`) | row → digit slot, column → K-line bit via `kTIKeyLineBits`; `key[digitSlot] \|= 1 << kbit` in the CPU |

`kTIKeyLineBits` maps column 1–5 to K-line bits `{1, 2, 3, 5, 6}` (index 0 unused);
bit 4 is the card-detect line. Scanning: `Core-Architecture.md`
§ "Keyboard Matrix and Scanning".

## Physical Keyboard Mapping

Computer keyboards drive the calculator in the Qt6 app and the web client
(`frontends/web/`, with its `keyboard` prop). The Swift app (iPhone and
iPad) has no keyboard input. The debugger and printer panels take no calculator
keys: Space (R/S vs. STEP) and Cmd/Ctrl+C (printer tape vs. selected text)
would have no single owner.

Values below are matrix codes. The website's keyboard legend
(`TI59_KEYBOARD_LEGEND` in `calc-u-site`'s `59/Pages.jsx`, prerendered without the engine)
lists the same bindings by hand.

### What is bound

**26 of the 45 keys, deliberately**: the white (digit) keys, the yellow keys
except 2nd, A–E, and EE / `(` / `)`. 2nd, INV, lnx, CE, LRN, x⇄t, x², √x, 1/x,
SST, STO, RCL, SUM, yˣ, BST, GTO, SBR, RST and R/S are pointer-only on
purpose. Do not "complete" the table without asking. With 2nd unbound, the
only second functions a keyboard reaches are A′–E′.

### Rules

1. A binding resolves to one or two matrix codes. 2nd functions are reached
   as on hardware: 2nd, then the key.
2. Shift + letter means "2nd, then that letter's key". Only `a`–`e` and
   `x` (×) are bound, so this gives A′–E′ and, with `x`, Deg.
3. Non-letters resolve on the character the keyboard produced, so Shift is
   not a 2nd prefix for them: `Shift+.` is `>` → EE. Lookup goes by produced
   character, never by scan code, so non-US layouts work wherever they put
   these characters.
4. With Command, Control or Option/Alt held, no key is the calculator's.
5. A letter is the letter the key carries on the user's layout, in every
   frontend; the frontend sends what the host reported and the
   map (`TIKeyboardMap`, `app/TITyping.cpp`) decides.

### The table

| Keyboard | TI key | Matrix | | Keyboard | TI key | Matrix |
|---|---|---|---|---|---|---|
| `0`–`9` | 0–9 | 92 82 83 84 72 73 74 62 63 64 | | `Escape` | CLR | 25 |
| `.` `,` | . | 93 | | `/` | ÷ | 55 |
| `<` | +/− | 94 | | `*` `x` | × | 65 |
| `a`–`e` | A–E | 11–15 | | `-` | − | 75 |
| `A`–`E` (Shift) | A′–E′ | 21 + 11–15 | | `+` | + | 85 |
| `>` | EE | 52 | | `=` `Return`/`Enter` | = | 95 |
| `(` `)` | ( ) | 53 54 | | | | |

Numeric-keypad keys produce the same characters and need no entries. `x` is
× as well as `*`, so a keyboard without a numpad needs no Shift for
multiplication.

### Injection timing

`key[]` is a wire level, not an event; the ROM accepts a key only when two
scans one display sweep apart see it, and drops a shorter press as bounce.
Every frontend goes through the library's typing
engine with the TI profile (`app/TITyping.hpp`):

- A press is held at least 100 ms of emulated time, and until the ROM's key
  scan has read it once (at most 1 s: a printing loop reads R/S rarely;
  measured in `tests/typing_tests.cpp`). A key-up that comes sooner
  defers the release; it is never dropped. On-screen keys of every frontend
  go this way, so a click shorter than that still registers.
- A live key goes down at once, whatever the machine does (R/S and GTO on a
  running program); a press that waits (a two-code binding, a preset's
  key) waits for the steady scan loop before it presses, and again after it
  releases, so a key never lands in the ROM's release gate or debounce.
- Two keys held together are both down. Two keys of
  one matrix row are rejected by the ROM, which reads neither.
- OS auto-repeat is ignored.
- A two-code binding is one action played as two full presses; its key-up is
  ignored, because the ROM must see 2nd released and rescanned first. A new
  key drops the second press when it has not started.
- Losing focus releases the held keys, after their minimum hold.
- The pressed key is drawn from the library's `held-keys` topic.

### Focus

Keys reach the calculator only while it has focus. It takes focus at launch
and on every click on the calculator. There is no focus indicator, so
clicking into the debug panel silently stops keys reaching the calculator.
Panels that need keys
themselves (the CPU inspector's arrow keys) take focus on click, never on
appear. The web client is focusable (`tabIndex=0`, focused on
pointer-down with `preventScroll`) but not focused on load, so Tab still
works on the page.

## Card Reader Flow

The core side (TST BUSY wait, `CRD_READ`/`CRD_WRITE`, auto-eject at
`CRD_OFF`) is in `Core-Architecture.md` § "Magnetic Card Reader". The card reader
is the library's media part:

- **The place.** The TI-59 offers `card-reader/media-place`, a
  `calcu::media::MediaPlace` (`core/TICardReaderPlace`): pass-through, the
  sides 0 and 1; the TI-58/58C have none. Its content is a
  `TIMagneticCardData` (`core/TIMagneticCard`): per side nothing (fresh), the
  246 bytes of a bank, or an in-file definition that `buildCardBank` turns
  into the bank the ROM writes when the side goes in (with the definition's
  partition or the machine's then). The place gives the card back when the
  core ejects it at `CRD_OFF`; Eject Card keeps a whole written bank only;
  a Reset, power cycle or Cold boot takes a card in the reader out as Eject
  Card does. `waitsForMedium()` is the card wait of `n Write` / `n INV Write`
  (the media state's `waits`; the machine's `awaitsMedium()`, the idle
  signal, asks it).
- **The kind.** `app/TIMagneticCardKind`: `magnetic-card`, `.magnetcard.yaml`,
  format 3, the implicit template "Fresh card", the steps `card:` / `arm:`
  and `saveas: { card: }`; it asks the media part to read the card's file
  again just before each write (`rereadBeforeWrite`), so a write replaces
  only the side that was written; no Name & Save; the replace question of
  Arm Fresh Card.
- **The flows** are the library's commands on the place `card-reader`:
  `media.choose {entry: saved:<file>}` (Arm Card: fitted, not fed),
  `media.start-new {template: "implicit:Fresh card", name}` (Arm Fresh
  Card: Name & Save up front, the file written at the first write),
  `media.feed {side}`, `media.eject`, and the topic `media` (the fitted card,
  the side passing, the picker). The corner rule stays the product's:
  `c59.tap-card {x, y}` (`cardCornerAt`, `KeyboardLayout`) sends a feed while
  the fitted card is on show. A written card is saved when it is given back
  (the media part's autosave); a write that fails is a notice.
- **The cue card shown** (`TIAppPart::refreshCueCard`, at each call): one
  fitted card at a time, the last of a fitted magnetic card, a preset's
  `cue-card:` and the constant memory file's; while a library program is
  selected (the product kind `c59.library-program`), the module cue card of
  that program from the module file and the fitted card lie on top of each
  other, the one put there last on top: a change of the selected program
  puts the module card on top, a card fitted the fitted card (the
  registers show only which program is selected, so selecting the same one
  again changes nothing). While the module card covers it, the fitted
  card's corners do nothing.
  A fitted magnetic card shows its own cue card or one titled with its name,
  the banks of its sides in the corners. A new machine has nothing fitted;
  a Reset, power cycle or Cold boot keep it.
- **Presets**: `card: { name: | file: | mem: , side: }`, `arm:`,
  `magnetic-cards:` (`TIMagneticCardsKey`) and `saveas: { card: }` are the library's
  media steps (`docs/formats/preset.md` §3.1, §5.2).

### Opened Files

Every way a file reaches Calc-U-59 (a drop, Open With, a file named at the
start of the Qt6 program, File ▸ Load Preset…'s dialog, the harness's
`open <path>`) sends the library's `open {files, way}`; a drag first asks
`open-would-take {name, size}` (Qt6: the library's `library/qt6/FileOpening`). The library tells presets (`.ti59`, `.ti58`,
`.ti58c`, or a top-level `model:` line) and media; then the product's file
kinds (`app/TIFileKinds`, in this order): an `.asm` file is an ASM overlay
(read as by Select File), a `.keystrokes.yaml` file's program is written
into the running machine from step 000 (no cold boot), a `.rom.yaml` and
`ti58c.memory.yaml` are refused with a notice. A magnetic card
(`.magnetcard.yaml`, a medium of the library's media part) is opened by the library's open
part through the media part: fitted where it lies on the TI-59 and written back there (an
upload on the Web is imported into the save folder first), refused with a
notice on the TI-58/58C and while a card passes; a `.u59` or `.bin` file is no file
of the product. A refusal is the session's notice.

Card file format: magnetic card format 3 (`docs/formats/media.md` §1;
`app/TIMagneticCardKind`).

## Presets

A preset (`docs/formats/preset.md`, format 2) is the library's: `load-preset
{ref}` (Qt6: a file in its folder; a bundled example by its bundle path
`examples:<path>`, in `examples/` of the app's data, the library's
`calcu::qt6::bundledDataDir`: `Contents/Resources` of the macOS bundle,
`share/calc-u-59` of a Linux install, else `resources` beside the
program) or `load-preset {content, name}` (Swift, an upload or a
drop on the Web) is one operation of the session. The library's loader
reads it all or nothing with Calc-U-59's rules (`TIPresetRules`: idle is key
readiness, or the card wait of Write, without a break for 100 ms; `expect:`
compares a number with X in 13 digits; `screenshot:` is `TIDisplayPicture`;
`slot-1` by `name:` only) and keywords (`TIPresetKeys`: `partition:`,
`program:`, `data:`, `cue-card:`, `printer-trace:`), builds the machine
through the library's machine control from a cold boot, and carries out the items in file order,
each written through the product kind `c59.program-memory`
(`core/TIProductKinds.hpp`). A preset of another format than 2 is
refused with the format this release reads. The preset list
(`preset-list`) and the startup presets come from
`examples/bundle.yaml`; a startup preset is set in `Calc-U-59.ini`
(`[preset] <model>`; the Settings' Startup presets).

## TI-58C Constant Memory

The TI-58C keeps its RAM through power-off. `ConstantMemoryKeeper` keeps
its 64 raw registers (`R000`–`R063`, H00–H03 as `R060`–`R063`) and the own
cue card as `ti58c.memory.yaml` (constant memory format 1,
`docs/formats/media.md` §4; `ConstantMemoryFile`) through `TIFileAccess`, in
the save folder the frontend names (the Web's: the persistent file system).
It reads the machine through the kind `c59.constant-memory`.

| When | What |
|---|---|
| A TI-58C built at the start or a model switch, no startup preset | The file's registers are the build's content, in RAM before the processor starts; the ROM restores the partition and Fix from them; the file's cue card is fitted |
| Built from a preset or a startup preset | Zeroed memory; the machine never writes the file |
| A change while a machine from the file runs | Written at the latest 0.5 s of wall time later, not when nothing changed |
| Before the machine is replaced, suspend, quit | Written at once if changed |
| Reset, power cycle | The memory stays in the machine; the file is not read |
| Cold boot | RAM zeroed before the processor; the file follows |
| No file, a damaged or a newer one; a failed write | Zeroed memory; a notice (damaged, newer; a failed write once per start) |

## ROM Loading

A frontend looks the ROM files up once at its start:
it calls `cu59::lookUpRoms` (`app/ProductRoms.hpp`; Qt6, the Swift bridge,
the Web's `bindings.cpp`), the library's `calcu::roms::lookUp` over the full
set of 24 files with the core's `TIRomReader`. Without the full set the app
session is made with the lookup's result and no ROMs (`TIAppSetup::roms`):
it builds no machine, refuses `start`, and the library's StartupPart publishes
`roms-missing {text}`, which the library's shell shows in place of the faceplate while Help and
About still work; there is no other ROM error. With it, every model switch and module change takes
its files from the `RomSet` in memory and reads no file. The catalogue's ROM
entries (`core/TICatalogue.cpp`) name each model's files: three ROM chips
(Datamath text dumps: header, a `---` line, then `AAAA: WWWW …` rows, words
masked to 13 bits) concatenated into 6144 words and two `*-CONST-K.txt`
files for the 64 constant rows.

| Model | ROM chips | Constants |
|---|---|---|
| TI-59 / TI-58 | `TMC0582.txt`, `TMC0583.txt`, `TMC0571B.txt` | `TMC0582-CONST-K.txt`, `TMC0583-CONST-K.txt` |
| TI-58C | `CD2400.txt`, `CD2401.txt`, `TMC0573.txt` | `CD2400-CONST-K.txt`, `CD2401-CONST-K.txt` |

The TI-59 image is checked against two sentinels: word 0 = `0x0A01`
(`CLR IDL`) and word 6143 = `0x1987`. A Solid State module's ROM is its
`roms/<ID>.rom.yaml` (`docs/formats/modules.md` §2, 5000 bytes, read by
`core/ModuleFile`). The module list comes from the
14 module files (`allModuleMetadata(RomSet)`: `module-name`, `description`,
`menu-sort`), and so do the module cue cards (`ModuleCueCards`).

## Places: the module and the PC-100C

The module and the PC-100C are peripherals in the machine's places `slot-1`
and `printer` (`docs/developer/Core-Architecture.md` § "Component Overview"). The
build (`TIProductMachines`, a preset's header) plugs them in; the library's
`attach {place, peripheral}` / `detach {place}` change them, each followed by
the power cycle of the pin-out (the TI-58 and TI-59 lose program and data
registers, the TI-58C keeps its constant memory). `TIAppPart` reads what is
plugged from the machine and keeps TRACE's position (`m_printerTrace`: off
at the start, never saved, kept across Reset, power cycle, Cold boot and a
detach): it follows the plugged PC-100C's switch,
and the library's `attach` puts it into a new PC-100C through the product hook
`TIProductMachines::attaching` (the request's switch positions); a preset's
build sets it off unless `printer-trace: on`. Before a detach the
PC-100C ends the line it prints where the head is, and its rows go onto the
paper. The paper is the library's `PaperPart`'s: it binds the
`printer/paper-output` capability whenever a PC-100C is plugged in, keeps
the paper while it is detached and gives it back on any model; a user's
preset load empties it, a startup preset's keeps it. The paper
panel's `set-control {peripheral: PC-100C, control, on}` is the library's
`PeripheralsPart`'s (its one owner: the session refuses a second
declaration of a command name).

## Drawing Rules

All three GUIs draw with the same shapes and numbers; only the painting calls
differ.

### LED Display Rendering

- Glyphs: `SevenSegment` (`ledCell(i, glyph, level, calcOpacity)`, the glyph being `LedFrame::segments[i]` from the machine's display output:
  segments and opacity per cell, in cell 11 the "C" annunciator when it is
  brighter than the glyph there), never a
  font. Zero suppression happens in the core (`TMC0501`), which reports
  suppressed positions as not driven; `ledCell` blanks a position at
  level 0. All three renderers call it.
- Shapes: `LedGeometry` — hexagonal segments (modernized) or dot grids
  (classic), the decimal point, and per style the slant (bottom fixed; shear
  0.14, about 8°, modernized; 0.1139, about 6.5°, classic), colours and glow radii. The decimal point is not
  sheared.
- Brightness: `LedFrame` gives 0–255 per digit and decimal point
  (`DisplayPov`).
- Each frontend paints a blurred glow (and, modernized, a tighter bloom)
  under the crisp segments.

### Cue Card Rendering

The block: `docs/formats/cue-card.md` (a preset's and the constant memory file's, `app/CueCardYaml`). A preset's own cue card always draws as the plain `CueCard`.

- **Templates.** `CueCard`, `MagnetCard` and `SolidStateCard`, each with its
  own artwork (`frontends/swift/App/Assets.xcassets`) and fractional layout
  (`cueCardLayout`). Only `SolidStateCard` has an ID and column dividers, and
  is printed gold (`0xC49223`); the others use the card's ink (black, pencil,
  sharpie). Only `MagnetCard` has bank badges.
- **Which card shows** (`TIAppPart::refreshCueCard`): of the running library
  program's card (else a generic card for the module) and the user's card
  (card file, preset, `ti58c.memory.yaml`), the one put there last.
- **Column spans** (`labelSpans`, `dividersBetween`). A grid row has 5
  cells. A `\blank` cell extends the preceding label's span; empty cells
  draw nothing.
- **Shrink-to-fit** (`cellFittingFontSize`). Each grid row gets one font
  size, the largest at which every span fits on one line (bold character
  width 0.64 × size), never below 40 % of the template's size. Free-text
  rows truncate instead.

The web client gets the card as a draw list `app/TIWebPart` builds with
these functions for the size the page reports, and paints it on a canvas
(`frontends/web/src/painters.js`), badges included.

### Printer

The PC-100C gives its paper as marks: the font, the
geometry measured from a real unit (dot pitch ≈ 0.430 mm, line heights
including lines cut short) and the dot rules are the core's
(`core/PC100CPaper.hpp`), the paper is the session's `PaperPart`, and the
Qt6 and Web frontends draw it in the library's paper panel; the picture of
Copy (600 dpi) is the library's (`library/paper/Picture`). The Swift paper
panel draws from the same marks.

## Solid-State Program Display

The CALCULATOR tab's program view resolves where the running program comes
from with `programSourceFrom` (`ModuleImage`), which decodes the PRG SOURCE
flag as the firmware does (`Core-Architecture.md` § "PRG SOURCE flag") plus
the CPU PC: bit 0 → `SolidState`; else bit 3 → `Rom`; 0 → `UserProgram`;
2 with the PC inside the RTN → LIB_RETURN window (ROM 1193–1194, 14F9–1506)
→ `SolidStateReturn`; any other 2, 4 or 6 → `FastMode`.

- `ModuleImage::parse` reads the module header into program ranges;
  `DebugModel` re-parses it when the machine or module changes.
- `ModuleImage::libraryStep()` maps the core's latched `libExecPC` (`0xFFFF`
  = none) to program, program-relative step (000 = the program's first
  keycode) and range. The program is cross-checked against SCOM[9] digits
  4/3 (mismatches go to the session's `log`).
- During `SolidStateReturn`, SCOM[0] nibbles 4–7 hold a raw CROM address,
  not a step. A listing is never built from it; the previous one is kept
  (`DebugModel::rebuildLive`).
- Colours: solid-state steps highlight blue; return levels with source 2
  show purple, like library levels (`DebugText`).

Snapshot fields and freeze semantics: `Debug-API.md`.

## The Swift frontend

The Swift app (`frontends/swift/App/`, `frontends/swift/Bridge/`) for iOS and iPadOS
(the Mac app is the Qt6 app) is a renderer over the app session with `TIAppPart` (through
`frontends/swift/Bridge/TIAppSession` on the library's `calcu::shell::SessionThread`);
its threads are under "Threads" above.

### Layer Overview

```
CalcU59App                    (Calc-U-59App.swift: scene, background/active)
└─ CalculatorView             (adaptive layout, toolbar, card bar, file importer, alerts)
    ├─ KeyboardView           (calculator art, hit test, LED + cue-card overlays)
    │   ├─ LEDDisplayView
    │   └─ CueCardView
    ├─ PrinterView            (the PC-100C's paper, PRINT / T / ADV, copy, cut)
    ├─ CardPickerView         (Arm Card / Arm Fresh Card sheet)
    ├─ DebugView              (CALCULATOR / CPU / RUN LOG tabs)
    │   ├─ LiveDebugView      (CALCULATOR)
    │   ├─ CPUInspectorView   (CPU, with the ROM heatmap) + ASMDebugContent
    │   └─ StaticDebugContent (LOG)
    └─ SettingsView

EmulatorViewModel (@Observable, main thread)
├─ TIAppSession               (frontends/swift/Bridge/ → library/shell/SessionThread, app/TIAppPart)
└─ SessionDelegate            (TIAppSessionDelegate: the debug tab, notices, changes, written files)
```

### File Responsibilities

| File | Role |
|---|---|
| `Calc-U-59App.swift` | App entry; suspends on background, resumes on active; `RomsMissingView` with the Help item |
| `EmulatorViewModel.swift` | The `@Observable` face of the app session: sends the commands, copies the topics into observable properties; holds `TIShell` and copies the shell's models (menus, More items, bar, settings rows, the paper's owner, "Loading…", the notice shown) at each change of their inputs; the pointer on the faceplate through the shell's rule; Swift mirrors of the debug snapshot and `ProgramSource` |
| `ROMLoader.swift` | The ROM lookup over the bundle's resource folder (`TIRomSet`; a "ROMs missing" text in place of the calculator) |
| `CardStorage.swift` | The save folder (cards, `ti58c.memory.yaml`): iCloud Documents container, else local Documents |
| `AppSettings.swift` | Where the settings file lives (Application Support), the Debug-only `--settings-file`; the settings themselves are the session's |
| `CueCardContent.swift` | Swift cue-card value type and its `TICueCard` conversion |
| `Views/CalculatorView.swift` | Root view; the layouts — pages, or side by side with or without the debugger, as the library's view rule says for the window's size (`TIViewRules.view`: `shell::view` on `cu59::viewInputs`; the pages over an active fold of the iPhone Duo), the printer's width the chrome's, a top margin for windows without a top inset (`mobileTopMargin`); the control bar (`ControlBarView`); the notice alert; Load Preset…'s file importer (the preset types, the library's `open`), the settings sheet and the ASM file importer |
| `Views/ControlBarView.swift` | The control bar from `TIShell.bar`: the model picker, the picker of `slot-1`, the card controls (`CardControls`); More at the right edge (Load Preset…, Settings…, Reset; Cold boot asks first, its question on the More button); `PlaceButton`, the place button of the bar and the paper panel |
| `Views/KeyboardView.swift` | Calculator art; the pointer through the library's rule (`TIShell` on `cu59::faceplateHitMap`: the key, the display for full speed; the card's corners first), pressed-key effect, haptics / click, "Loading…" below the display |
| `Views/UIKitTouchTracking.swift` | Raw-touch tracker that replaces `DragGesture` for keys (iOS 27 fires `onEnded` early) |
| `Views/LEDDisplayView.swift` | 12-digit LED in a SwiftUI `Canvas` |
| `Views/CueCardView.swift` | Cue card on its template art |
| `Views/PrinterView.swift` | The paper panel: the owner's paper under its name, an empty paper under none; dots by band (`PaperBand`) or text; Copy and Cut are the session's |
| `Views/CardPickerView.swift` | Arm Card (the saved cards by name, from the library's `media` topic) and Arm Fresh Card (a name; the replace question) |
| `Views/DebugView.swift` | Debug tabs; `ASMDebugContent`; RUN LOG tab (`StaticDebugContent`) |
| `Views/LiveDebugView.swift`, `Views/CPUInspectorView.swift` | CALCULATOR and CPU tabs |
| `Views/DebugHeaderView.swift` | Both tabs' header as `DebugText`'s `debugHeader` describes it (`TIViewRules.debugHeaderButtons`) |
| `Views/HoldRepeatButton.swift` | Fires on press, repeats every 0.25 s while held and while step can be used (STEP) |
| `Views/ChromeControlStyles.swift` | Dark chrome button and menu styles; `Chrome`, app/'s `TIChrome` through the bridge, the pill and the plugged dot |
| `Views/SettingsView.swift` | The settings from `TIShell.settingsDialog` (`cu59::settingsSpec(SwiftIOS)`): a choice is a picker (true and false a toggle), a number a field; a change is `set-setting` at once; the notice alert over the sheet |

### Build

Xcode compiles only Swift and the bridge (`frontends/swift/Bridge/`). The
C++ below them, the library, `core/` and `app/`, comes as
`CalcU59.xcframework`: the project's aggregate target
`CalcU59Core`, which the app depends on, runs
`frontends/swift/build_xcframework.sh`, which builds it with the
repository's root CMake project for iOS devices and the iOS
simulator into `build/xcframework-59/` (Release or Debug, as the Xcode
build). So every Xcode build is built from the current sources; without
CMake and Ninja the build stops with an error. The bridge includes the
headers from the XCFramework (`<calcu59/app/…>`, `<calcu59/core/…>`).
`frontends/swift/check_xcode_project.sh` (CTest, Frequent) fails when the
project takes in a C++ source or the bridge includes from the source
tree.

### Bridge Layer

The wrappers translate types only. `frontends/swift/Bridge/Calc-U-59-Bridging-Header.h`
imports the public ones.

| Wrapper | Wraps |
|---|---|
| `TIShell.{h,mm}` | The library's shell models for Swift: the last Value of each input topic (`control-state`, `busy`, `run-state`, `roms-missing`, `settings`, `media`, the `preset-list` reply) gives `calcu::shell::menus`, `moreItems`, `bar`, `dialog` over `cu59::settingsSpec`, `panels`, `LoadingRule`, `NoticeQueue` and `PointerTracker` on `cu59::faceplateHitMap`, with the traits of the mobile platform (no About, no Quit); `choose:` sends an item's command (a refusal is a notice) or hands back a shell action; value objects `TIMenu`, `TIMenuItem`, `TIBarControl`, `TIDialogSection`, `TIDialogRow` |
| `TIShell+Session.h` | ObjC++-only: what `TIAppSession` and `TIShell` tell each other (the inputs, the notices, the busy changes) |
| `TIAppSession.{h,mm}` | The app session on `calcu::shell::SessionThread`, with the library's `SettingsPart` and the settings file it owns (opened here, then the session thread's only; `--settings-file` in Debug builds): commands out, messages in on the main thread (the last of each topic kept); `TIAppSessionDelegate` (alerts, the tab of `debug-panel:` from the topic `c59.debug-view`, `changed`, `cardWritten`); `TIAppLog` (the app log: the library's `shell::HostAppLog` with the system log as its mirror; the bridge's own lines through `calcu::shell::hostAppLog()`); the debug panel's product topics (`c59.calculator-debug`, `c59.program-listing`, `c59.cpu-debug`, `c59.heatmap`, `c59.asm-overlay`, `c59.debug-view`), the RUN LOG tab's `cu59::RingLog` of the library's `log`, the `control-state` entries (`asmRunAvailable`, `calculatorTabAvailable`, `cardControls` as `TICardControl`s), `freezeButtonsFor:` (`cu59::freezeButtons`) and the library's `open` / `open-would-take` (`openFile:…way:`, `wouldOpenFile:`); `TISessionStart`, `TIRomSet`; the coordinated save folder files (`CoordinatedFiles`, a `calcu::media::FileAccess`: `TIAppSetup::files`, the constant memory file's and the media part's file access); the cards on the library's `media` topic and `media.choose`, `media.start-new`, `media.eject` (`savedCards`, `armCard:`, `armFreshCard:`, `TISavedCard`); value objects `TILiveDebugSnapshot`, `TIStepEntry`, `TIRegEntry`, `TIInspectorEntry`, built when Swift reads them; also the library's `paper PC-100C` (kept as text lines and `TIPaperDot`s), `paper-copies`, `run-state` (`buttonUseFor:`, calcu::debugger::buttonUse) and `trace`, the run control's commands, and the clipboard host services (`clipboard-image`, `clipboard-text`) answered on the pasteboard |
| `TISharedTables.{h,mm}` | `SevenSegment`, `KeyboardLayout`, `KeyIds`, the base image's width (the hit map's unit) |
| `TIViewRules.{h,mm}` | `CueCard` (spans, geometry, inks), `LedGeometry`, the PC-100C's paper width and dot size (`core/PC100CPaper.hpp`), `RomHeatmap`, `ModuleImage` (program source), `DebugText` (incl. the iOS `mobileTopMargin`), the view rule (`shell::view` on `cu59::viewInputs`), the paper row's rule (`shell::paperRowCompact` on the TI chrome) |
| `TIFileFormats.{h,mm}` | `TICueCard` |
| `TITypes.h` | `TIDisplaySnapshot` (mirrors `LedFrame`: the glyph's segments A–G and brightness 0–255 per digit and decimal point) and `TICpuFrame` (mirrors `CpuFrame`) |
| `BridgeConvert.h` | ObjC++-only: string / data conversions; `static_assert`s that `TICpuFrame` matches `CpuFrame` field for field, so a frame converts with one `memcpy` |
| `TIFileFormats+Cpp.h` | ObjC++-only: `TICueCard` ↔ `CueCardContent` |

Never import the three ObjC++-only headers from the bridging header.

### Swift-specific details

**The family structure.** The window is drawn from the library's
shell models, computed by `TIShell` in the bridge from the same
topics the Qt6 and React sets read: the control bar
(`ControlBarView`, with More at its right edge, which holds every menu
item under its menu's heading; `CalculatorView` measures it with and without its labels and the library's `barFit` (`TIBarFit`) gives its width and labels), the PC-100C's button on the paper panel (`TIShell.paperPlaces`), the settings (`SettingsView`), "Loading…" and
the notices (one alert after the other). No Swift view decides what the
machine offers; a control or item is enabled exactly when its entry is.

**Keys.** `KeyboardView` hands the pointer (in points, with the points per
pixel of the base image) to `TIShell`, whose library rule finds the key on
`cu59::faceplateHitMap` and sends `screen-key-down`/`-up` (a held display
is `speed {full}`); a press on the fitted card's corner goes first to
`tapCard`. Key touches go through `UIKitTouchTracker`. Timing rules:
§ "Physical Keyboard Mapping" ("Injection timing").

**LED.** `LEDDisplayView` draws each cell in a `Canvas` with the shared
segments and shear, the glow and bloom as blurred copies under the crisp
fill, each at the cell's `LedFrame` brightness. Rules:
§ "LED Display Rendering".

**Cue card.** `CueCardView` places the artwork `Image(template)`, then text
at the frames and font sizes `TIViewRules` returns, with `.lineLimit(1)`.
Rules: § "Cue Card Rendering". A press on the card goes
first to `tapCard` (`KeyboardView`), and `CueCardView` draws the side being read
from `litCardCorner`: § "Card Reader Flow".

**Printer.** The paper panel is always there: the owner's
paper under its name while the PC-100C is attached (its button at the
left of the panel's header attaches it), an empty paper under none. The paper is the library's: the bridge keeps what the topic
`paper PC-100C` sends (text lines, dots in the dot pitch with their ink; the
reset marker empties it), the view model files the dots by band of 10 units,
and `PrinterView` draws the bands in view (`PaperBand`) or the selectable
text. Copy sends `paper.copy`: the library makes the picture at 600 dpi and
asks for `clipboard-image`, which the bridge puts on the pasteboard
(a PNG) with the text; the toast
follows `paper-copies` ("Copied to clipboard" / "Copy failed"). Cut sends
`paper.cut {copy: true}`; `printerClearID` resets the text view's identity
to drop a selection. PRINT, ADV and TRACE send `set-control`.

**Debugger buttons.** FREEZE, STEP and RESUME of the CALCULATOR tab are the
run control's level `program`, those of the CPU tab `cpu`;
which can be used is the library's rule (`buttonUseFor:`, and
`freezeButtonsFor:` for FREEZE and FREEZE ON START), FREEZE ON
START is `pause-on-start`, the RUN LOG tab's TRACE the trace session's button
trace. What the tabs show is the session's: the product topics
of `app/TIDebugPart` and the library's `log`; the buttons send its commands
(`c59.dump`, `c59.heatmap-reset`, `c59.asm-load`,
`c59.asm-run`, `c59.asm-clear`); Run, TRACE and the CALCULATOR tab follow
the control state's product entries.

**Opening files.** A file picked with the file importer of Load Preset…
(More ▸ Load Preset…; the preset types only) goes to the session's `open` (way `dialog`) with its
bytes and path; the session finds out what it is, and a refusal arrives as
its notice (an alert).

**Storage.** Cards (`.magnetcard.yaml`, the library's media part) and
`ti58c.memory.yaml` live in the `CardStorage` folder (iCloud Documents when
available); reading and writing them go through `NSFileCoordinator` (the
bridge's `CoordinatedFiles`, a `calcu::media::FileAccess` that the app part
hands `TIFileAccess` and the media part as their file access). The settings file is `Calc-U-59.ini` in Application
Support, opened by the bridge and then the session thread's (the library's
`SettingsPart`); a Debug build takes `--settings-file <path>` (a relative
path in the app's temporary folder), the one launch argument of the UI and
unit tests. Traces go to the trace location from Settings (read at each
trace start): Local
(`Calc-U-59` in the app's Application Support) or iCloud (the card folder;
refused while signed out, `HostDirs::iCloudAvailable`).
The save folder and the folder for screenshots are read when used too, so a
change in Settings applies at once.

## Tests and goldens

`tests/` (CTest target `CalcU59Tests`, run from the product folder) covers
the modules, with the ROMs from `roms/` (`LedGeometry` has no test of its
own). A second target,
`CalcU59TraceReader`, runs `tools/read_trace.py` on the trace the tests wrote
(`build/trace-test.bin`, `tests/trace_tests.cpp`), so the reader stays in
step with `TraceWriter`. The key names and the file formats are checked
against recorded goldens (`tests/golden/`); a difference is a finding.
