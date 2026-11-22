# Adding a New GUI Frontend

For anyone adding a GUI for a platform Calc-U-59 doesn't support yet
(Android, another toolkit, …): what a new GUI reuses, and what it still has
to write.

## The existing GUIs

| GUI | Language / toolkit | Uses |
|---|---|---|
| Swift app (`frontends/swift/App/`) — iOS, iPadOS | Swift + SwiftUI | `core/` + `app/` through ObjC++ wrappers in `frontends/swift/Bridge/` |
| Qt6 app (`frontends/qt6/`) — macOS, Windows, Linux | C++ + Qt Widgets | `core/` + `app/` directly |
| Web client (`frontends/web/`) — browsers, embedded by the website | JS + React, engine in WASM | `core/` + `app/` compiled with Emscripten; `frontends/web/bindings.cpp` is the adapter (`Web-Client.md`) |

All three share all non-GUI code and are tracked in `Frontend-Parity.md`. The
three are kept at feature parity, with a few deliberate exceptions in the
web client (Classic LED only, cards kept in the browser's IndexedDB, no
debugger in its portrait layout).

## If your GUI can call C++

Build on `app/` (`App-Layer.md`). The Qt app is the worked
example: `frontends/qt6/app/EmulatorController` is the whole adapter (header and
source).

1. **Host one app session** on one thread of yours: look the ROM files up
   once (`cu59::lookUpRoms` over a `calcu::roms::Bundle` of the ROM files of
   `roms/` as your packaging holds them) and make the session with
   `cu59::makeAppSession` (`app/TIAppPart.hpp`): the ROM lookup's result,
   the settings file (the startup model), your `calcu::settings::Paths`
   (the save folder: cards, `ti58c.memory.yaml`; the trace folder, the
   screenshots, `~/`) and the bundled `examples/`; `makeAppSession` sends
   the library's `start`. Its second form takes a `calcu::shell::HostSessionSetup`
   (`shell/HostSession.hpp`: your `calcu::shell::HostSettings`, the settings
   file and places opened for you, for the library's `SettingsPart`; your own
   parts, the clock, whether to send `start`). Without the full set the session runs without a
   machine and the library's StartupPart publishes `roms-missing {text}`,
   which the shell shows in place of the faceplate.
   Put the window together from the library's shell (the Qt6 component set's
   `calcu::qt6::Shell` over a `SessionHost`, the React set's
   `useCalcuShell`; Swift natively: `Bridge/TIShell` computes the
   library's models in C++ and SwiftUI draws them, the pattern for a
   toolkit without a component set of its own) with Calc-U-59's data from
   `app/TIShellProduct` (`shellProduct`, `settingsSpec`, `faceplateHitMap`):
   the menus, the control bar (the model picker, the module slot's picker,
   the card controls, the More button at its right edge with Load Preset…
   and Settings…), the "PC-100C" button on the paper panel, the settings dialog,
   About, Help, notices and "Loading…" come from the control state and the
   library's commands. A new menu item goes into that menu model, never
   into one frontend.
2. **Listen to it** (`AppSession::listen`) with the host services you offer;
   keep a `calcu::sessions::TopicMirror` of what it sends: the library's
   topics (among them `control-state`, from which every control
   about the machine is drawn, the PC-100C's PRINT, ADV and TRACE with their
   positions among them (`controls`), `papers`, `paper <owner>`,
   `paper-copies`, `run-state` and `trace`) and the product's
   `c59.*` topics (`c59.calc-indicator` (the LED frame and the C indicator),
   `c59.cue-card` (the cue card, its corner on show and lit); full speed is the library's `speed` command and `busy` topic,
   the library's `media` (the card reader `card-reader` and `slot-1`: what they
   hold, the pickers; send `media.choose`, `media.start-new`, `media.eject`),
   …) and the debug panel's product topics (`c59.calculator-debug`,
   `c59.program-listing`, `c59.cpu-debug`, `c59.heatmap`, `c59.asm-overlay`, `c59.debug-view`;
   `docs/developer/Debug-API.md`; the LOG tab keeps the last 2000 lines of the
   `log` topic). Send every file the user hands over as the library's `open`
   (a drag asks `open-would-take`). Show `notices` as alerts. Offer `clipboard-image` and
   `clipboard-text` for Copy of the paper, and `keep-file` where a trace
   cannot go into a folder.
3. **Call it about 60 times a second** (`AppSession::call(now)`); redraw
   only the topics that came.
4. **Forward input**: keys as the library's typing commands
   (`library/typing/TypingTopics.hpp`): `screen-key-down`/`screen-key-up`
   `{key, pointer}` for an on-screen key (hit test with `KeyboardLayout`,
   key ID with `keyIdAt(matrixCode(...))`; `PointerTracker` turns pointer
   moves into presses, releases and full speed), `host-key-down` with what
   the host reported (`calcu::typing::HostKeyEvent`: the physical key, a
   name for Escape/Return/Enter, the character produced, the letter the key
   carries, the modifiers; the frontend maps nothing) and `host-key-up
   {physical}`, `focus-lost` on focus loss; draw the pressed key from the
   `held-keys` topic. Holding the display is the library's `speed {full}`
   (from `PointerTracker`); a press (not a drag) over a fitted card is
   `c59.tap-card` (`cu59::commands`); a file of any way is the library's
   `open`; the cards are the library's `media.*`; the paper panel's
   `paper.copy`, `paper.cut` and `set-control`, the run-control buttons'
   `pause`, `step`, `resume` (levels `program` for the CALCULATOR tab, `cpu`
   for the CPU tab), `pause-on-start` and `trace` are the library's.
5. **Draw** with the shared rules (`App-Layer.md` § "Drawing
   Rules"): `SevenSegment` + `LedGeometry` for the LED — never a font, no
   font reproduces the hardware's lit segments — `CueCard` for cue cards
   (with `litCardCorner` and `cardCornerGeometry` for the side being read),
   the component set's paper panel and run-control buttons (`library/qt6/`,
   `library/web/components/`; the paper's marks come from the
   `paper PC-100C` topic), `DebugSnapshots`, `DebugText`, `RomHeatmap`
   (the heatmap's steps and colours), `RingLog` and `freezeButtons` for the
   debugger.

What stays per GUI: widgets and painting (glow, fonts), file dialogs and
the host folders it passes to the library (the settings file and the save
folder, `calcu::settings::HostDirs`; the keys are `app/Settings.hpp`'s),
packaging.

## If it can't call C++

Compile `core/` and `app/` to something it can call, as the web build
does with Emscripten (`frontends/web/build.sh`), and write the adapter in C++
on that side (`frontends/web/bindings.cpp`: the factory of the app
session, with `app/TIWebPart` for what the page draws):

- **No threads?** None are needed: the session runs on the one thread that
  calls it (the Web hosts it in a Web Worker; the library's
  `calcu::web::WorkerHost` translates its messages to the page).
- **Asynchronous files?** The ROM lookup reads every file once, at the
  start, synchronously. Fetch the full set (`cu59::fullRomSet()`) before
  making the session, as the library's Web worker does into `/bundle`
  (`files.json`); nothing is read later.
- **Pass drawing data, not drawing.** `TIWebPart` sends what changed as
  plain data — the topics `c59.web.display` (segment masks and brightness
  per LED cell), `c59.web.cue-card` (the cue card as a laid-out draw list)
  and `c59.web.debug-font`, and requests such as `c59.led-geometry` — and
  the GUI paints it. The paper is the library's `paper PC-100C` topic, the
  debug rows `TIDebugPart`'s topics.

What stays in the GUI's own language is the same as above: widgets and
painting, events, file pickers and storage.
