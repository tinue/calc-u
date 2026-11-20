# Phase F: the road to the first releases

The first release of each product from this repository: Calc-U-1600
0.9.0, then Calc-U-59 3.0.0. Before them comes a longer period of
pre-releases and testing. This document applies on top of `CLAUDE.md`
and `docs/Process.md` until the closing check at its end has passed.

## Where things stand

- **Published pre-releases** on tinue/calc-u, both marked pre-release,
  never "Latest":
  - `calc-u-1600-v0.9.0-pre`: seven assets (Linux x86_64 and aarch64
    AppImages, macOS DMG, Windows x86_64 and arm64 installers, the Web
    frontend zip, the help zip). No examples zip: the help says how to get
    the examples from the repository.
  - `calc-u-59-v3.0.0-pre`: seven assets; no iOS build yet, nothing
    promoted to the App Store.
- **CHANGELOGs**: `## [0.9.0] - work in progress` and
  `## [3.0.0] - work in progress`, written from the release-notes seed
  below.
- **Builds and tests**: every product builds; the Frequent group, the
  local Linux CI (base and web jobs) and the GitHub CI of both products
  are green. In the Longer group the three Calc-U-1600 screenshot
  entries `CalcU1600Shots.01/02/05` fail when run from an agent: they need
  the macOS screen-recording and Automation permissions, so run them from
  iTerm2 with the screen unlocked.
- **ReleaseIt** runs from its unmerged branch `several-products`
  (products with prefixed tags, `branching: trunk`). It has to be merged
  and released in the ReleaseIt repository before the first release.
- **Notarisation**: the notarytool keychain profile has to exist on the
  release machine (it was lost once and stored again).

### Open items

- **App Store metadata**: the worktree `products/calc-u-59/appstore`
  that `release.yaml` names does not exist yet, so `whatsnew.txt` was
  skipped at the pre-release.
- **Help pictures**: the pictures to retake are listed by name in
  `docs/help-site.md`, "Help content" (Calc-U-59: six, from the Swift app;
  Calc-U-1600: ten, `tools/make_screenshots.sh <chapter>` from iTerm2).
- **Glossary warnings** of the help check (several hundred): the check
  takes every word after `_Avoid_:`, also those the glossary avoids only
  in one sense ("slot (for this)"), and many common words there carry no
  qualifier. Proposed: warn only on unqualified words, and qualify the
  sense-bound ones. The user decides.
- **Documentation sweep, for the user to decide** (the documents were
  left as they are):
  - Load Preset's dialog on Windows and Linux offers presets only; the
    help says it offers all files (Swift and Web do), and the Calc-U-59
    tutorial and FAQ describe a greyed-out `.ti59.txt`.
  - Swift Settings: no Choose… for a startup preset, no Reset for the
    trace folder (the shared help describes both).
  - Calc-U-1600: the Debug server (DAP) row shows `true`/`false`; the
    empty cassette's Name & Save (09-basic-programs); `pc1600-module-class`
    is not checked against its four values or the header bytes
    (modules.md, help chapter 10); help chapter 10's second copy of the
    module definition format; no installing chapter or FAQ; the
    screenshot names carry other chapter numbers.
  - Harness: `--help` lists only part of the commands (harness.md says
    every one); attach refuses `--rom-folder`/`--roms`, options `calcu`
    does not have; `saveas: { card: }` is accepted on a TI-58 (preset.md:
    TI-59 only); module cue cards accept `pencil-color` (cue-card.md: not).
  - Test driver: the API names `card()`, `cardimage`, `read_card` (the
    glossary says module); `machine.py`'s `app_save_folder()` differs from
    `settings/Paths` for `~` alone, empty APPDATA/XDG_CONFIG_HOME and
    Windows without APPDATA; `calcu59.py` configures `build/tools` with
    `-DCALCU_PRODUCT_1600=OFF`.
  - Smaller: `tools/run_tests.sh` configures `build/tests` as Debug while
    the documents' recipe is RelWithDebInfo; `kPolledHardware` is set by
    the TI core but read by nothing; the Web storage mounts
    `/persist-calc-u-59` and `/persist`; `tools/refresh_sharpdx.sh`'s
    default source resolves inside `products/`; "library module" and "the
    emulator" in the ROM articles and the agent prompt; the README's
    History paragraph; Calc-U-59 TODO's REST API and polling items; the
    pre-release-cleanup skill's `appstore` branch.
- **External tester**: their superRAM suite's `test_title_in_s2_is_refused`
  now fails every run. On the card as their suite ships it (blank, no
  module header) `INIT"S2:","P"` leaves slot 2 a RAM disk (S2MTB FEH), so
  the ROM refuses `TITLE"S2:"` with ERROR 101 (routine 1EEFH). It passed
  only when an earlier test had written the card file: their seven cards
  are module instances, so every test writes them back. Tell spellbound:
  give the fixtures `template: true` (never written) and prepare the card
  in that test.
- **Specifications not written**: Calc-U-1600's trace file (`CALCU1600_TRACE.bin` and its siblings by model) has
  no page in `docs/formats/` (Calc-U-59's is `docs/formats/trace.md`).
- **Media topic and the Sharp media places**: the machine knows when a
  Sharp tape or serial port is awaited (`awaitsMedium()`), but the places'
  `waitsForMedium()` stays false, so the media topic never shows them as
  waiting.
- **Calc-U-59 checks by hand**: a loaded ASM overlay on a model switch
  (removed or kept); the preset format's speed-dependent `Adv` text
  against the behaviour; the tutorial's iCloud advice; `.keystrokes.yaml`
  files in Load Preset.
- **Other checkouts** need the ROM file rename `roms/CE-502B.module.yaml`
  (`fetch_roms.sh` stores it so).
- **Outside this repository**, edited but not committed: the
  `Sharp1500-1600-Ref` skill (a line still names the old CLI) and the
  calc-u-site skill `site-59` (a line names `StateFileFormat.md`). VS Code
  users of Calc-U-1600 re-create `launch.json`/`tasks.json` for the debug
  type `c1600`.
- **Harness attach**: `json on` over attach is refused ("unknown command
  json", the line is parsed before `json` is checked), and `calcu attach
  --json` then reads plain replies as JSON. The attach CLI test uses a
  fake app and does not catch it.
- **Smaller findings**: Swift `xcodebuild` fails from a worktree under
  `.claude/worktrees/` (works from the main checkout); the Playwright drag
  test is flaky locally; `examples/memory/maxed-out-mem.pc1600` says INIT "F" is needed
  before "M"/"P", which the research says is not true; the Duo debug
  header wraps mid-word.

## The coming period: pre-releases and testing

Pre-releases are cut as often as needed (`release -C products/<product>`
on `main`, `docs/Process.md` step 3). Each pre-release moves the `-pre`
tag; its notes are the CHANGELOG entry.

### Testing to be done

- [ ] Every Checkpoint entry of both products (`ctest -L checkpoint`),
      and the Longer group in full, with the screen unlocked.
- [ ] The Swift tests from the main checkout: Mac build, UnitTests
      (macOS and iOS), UIRegressionTests on the simulators of
      `bin/simulators.yaml` (`bin/simulators setup` first), CardTests.
- [ ] The Qt6 apps by hand on macOS, Windows and Linux from the
      pre-release assets: start, presets, media, paper, debugger, settings,
      drag and drop.
- [ ] The Web frontends from the pre-release zip, embedded in a test page
      and installed as an app; storage of the two products apart.
- [ ] The help zips read as the site will render them (`docs/help-site.md`).
- [ ] Calc-U-59 on a real iPhone/iPad through TestFlight.
- [ ] The external tester runs their PC-1600 suite on the pre-release.

### Test drivers

What exists:
- `calcu`, the harness of both products: one-shot commands, `calcu
  serve` (one command per line), attach to a running app, `serve --dap`
  for Calc-U-1600 (`docs/formats/harness.md`; the product commands in
  `products/calc-u-59/docs/developer/Harness.md` and
  `products/calc-u-1600/docs/developer/Harness.md`).
- Python: `library/harness/python/calcu_serve.py` (a `calcu serve`
  process) and `calcu_machine.py` (`Session`, product-neutral).
- The pytest driver `products/calc-u-1600/tools/machine/` for the
  PC-1500, PC-1500A and PC-1600, and its user guide
  `products/calc-u-1600/docs/testing/README.md` (CTest
  `CalcU1600MachineDriver` and `CalcU1600TestingExamples`, Longer). It
  began as the external tester's driver; their suite is a source of
  requirements, and gets rewritten on the driver.
- Golden runs and reference runs of both products, the screenshot
  scenarios, Playwright, XCUITest, the DAP smoke test.

Still to develop, for the external tester's suite and its follow-up
project (their suite and its files are private: never commit, copy into
the repository or publish them):
- [x] The serial ports in the guide: how a test drives the PC-1600's
      RS-232C (COM1) and the CE-158 with today's `c1600 serial` commands,
      for "tests of a console and of file transfer over the serial port".
      No driver change.
- [x] A card as a raw image, "the counterpart of `encoding: file`" (0.7.x;
      today `slot-N-rom:`): the guide shows `card()`'s bytes written to a
      `.bin`.
- [x] `peek`/`poke` of the ROM variants a port selects: bank 3 and 3b (port
      3DH b2), and the variant of 3DH b1 if the emulator has it.
- [x] The idle rule: a PC-1600 `LOAD` or `COPY` reads the keyboard for
      BREAK and so looks idle (`app/SharpPresetRules.cpp`). And `key-down
      break` is refused while `key break` is accepted.

Later, not now: the ROM version of a run (OLD/NEW, A01-A04), by version,
not by path; the open-bus value. Decided: no release package for testers
(the guide says how to build), and no sample program of theirs in the
repository.

## The releases

### Calc-U-1600 0.9.0

- [ ] The Checkpoint group of Calc-U-1600 green.
- [ ] CHANGELOG entry complete (release-notes seed, including its
      "Still to add" items), help pictures current.
- [ ] `release -C products/calc-u-1600 --rc`, then `--release`: tag
      `calc-u-1600-v0.9.0` on the commit of the `-pre` tag; the GitHub
      release with its seven assets, not "Latest".

### `/1600/` on calc-u.org

- [ ] Per `docs/help-site.md`: the help pages from the pinned help asset,
      the Web frontend at `/1600/play/`, the installable app, the site's
      checks and tests.

### Archive the old Calc-U-1600 repository

- [ ] tinue/Calc-U-1600 gets a README pointing to tinue/calc-u and is
      archived on GitHub (read only; its history stays).
- [ ] Its old memory directory (`sharp/Calc-U-1600`) is deleted; what is
      still valid is already in this repository's memory.

### Calc-U-59 3.0.0

- [ ] App Store metadata: the orphan branch `appstore` of tinue/Calc-U-59
      (description, keywords, screenshots, `whatsnew.txt`) pushed to
      tinue/calc-u and checked out as the worktree
      `products/calc-u-59/appstore`.
- [ ] Screenshots: the App Store and help screenshots retaken where
      something visible changed (`bin/run-screenshots`).
- [ ] The hosted presets and ASM overlays pass `calcu check-preset`.
- [ ] CHANGELOG entry complete (release-notes seed, including its
      "Still to add" items).
- [ ] The Checkpoint group of Calc-U-59 green.
- [ ] macOS DMG notarised locally; iOS through TestFlight, then submitted
      to the App Store.
- [ ] `release -C products/calc-u-59 --rc`, then `--release`: tag
      `calc-u-59-v3.0.0`; the GitHub release, not "Latest"; the version
      live in the App Store.

### `/59/` on calc-u.org

- [ ] Per `docs/help-site.md`: the pinned help asset and 3.0.0 Web
      frontend, redirects for every address of today, the upgraded hosted
      files, exact release links, the new embed code.

### Archive the old Calc-U-59 repository

- [ ] tinue/Calc-U-59 gets a README pointing to tinue/calc-u and is
      archived on GitHub.
- [ ] Its old memory directory is deleted.

## Release-notes seed

The user-visible changes of the first release against the last release
from the old repositories. The CHANGELOG entries are written from them.

### Calc-U-59 3.0.0 (against 2.3.0)

- Files written for 2.3.0 are not read: presets (`.ti59`, `.ti58`, `.ti58c`) and ASM overlays are refused as files that cannot be read; magnetic cards (`.u59`, every layout) and `ti58c.mem` are no files of the product. You convert them by hand (help: "Media files", "From 2.3.0"). The new files are `.magnetcard.yaml` for magnetic cards (with the card's name inside) and `ti58c.memory.yaml` for the TI-58C's constant memory. This is a one-time break for this release. The ASM overlay format is version 2.0: the first line is `Calc-U-59 ASM 2.0`, a `;` starts a comment after `HEX:`, and an instruction above 1FFF, a run of fewer than four digits or a word after `HEX:` refuses the file (2.3.0: cut to 13 bits, or passed over). A Cold boot or a new machine removes a loaded overlay (2.3.0: it outlived a model switch and a preset). An `.asm` file can be dropped and opened like any file (2.3.0: only chosen in the debug panel).
- Settings start at their defaults; the settings of 2.3.0 are not carried over.
- The preset format is new: YAML-shaped, key IDs instead of matrix codes, `format-version: 2`. A preset with any error is refused whole.
- A preset always builds its calculator from a cold boot. It no longer adds to the running calculator, and it no longer keeps a module or printer it does not name. A preset names its model: a 2.3.0 preset without `MODEL:` loaded into the running model, while its upgraded file names the model of its extension, so `diag.ti58` now switches to a TI-58.
- A preset's steps run at full speed unless a speed step asks for regular speed; keys you press meanwhile are dropped. While a preset or another operation runs at full speed for more than half a second, "Loading…" is shown.
- Magnetic cards the app makes live in the save folder, also in the web calculator, where they are now kept between visits. A card has a name inside its file; `.bin` is no longer a card. A card file opened from elsewhere is still used and written back where it lies. A write rewrites the card file but replaces only the side written; the name, description and cue card stay.
- On the web, magnetic cards persist across page loads. When the calculator is open twice in one browser, only the first copy keeps what is saved; the second says so.
- The Solid State ROM module is chosen on the control bar, not in Settings, and the choice is not remembered across starts; a startup preset sets it. The picker also offers an empty slot.
- Without a startup preset the app starts a TI-59 with the Master Library module and no printer (2.3.0: printer connected).
- Changing the module, or connecting or disconnecting the PC-100C, power-cycles the calculator: the TI-58 and TI-59 lose program and data registers; the TI-58C keeps its constant memory.
- The TI-58C keeps its constant memory across starts only while no startup preset is set for it; a startup preset cold boots it. A TI-58C built from a preset or a startup preset never writes the constant memory file, so loading an example keeps your constant memory for the next TI-58C started from the file (2.3.0: a load kept registers H00 to H03, and the file mirrored whatever ran).
- A second key pressed while another is held no longer releases the first; both reach the calculator.
- Menus and the control bar follow the family structure. Where there is a menu bar (Windows and Linux, macOS, the web calculator's wide layout), Reset, Cold boot (was "Clean"), Load Preset and Settings are menu items and leave the bar; "Clean" with Ctrl or Command held is gone. Where there is none (iOS, the web calculator's portrait layout), the three buttons stay, a More button holds Examples, Cold boot and Help, and the long press for "Clean" is gone. The PC-100C is connected with a button on the bar, not in the printer panel. On macOS the model picker moves from the window toolbar to the bar, and Help is new. The Machine menu has Resume. The web calculator shows a notice as a dialog instead of a line below the calculator.
- The TI-58C's constant memory is in place before the ROM starts, so the partition and the display format come back at a start, as on the real calculator (2.3.0 restored the registers after the ROM's start-up).
- A damaged constant memory file and a failed write of it are reported with a notice (2.3.0: silent).
- A Reset, a power cycle (a module change, the PC-100C connected or disconnected) and a Cold boot keep the fitted cue card, on every model (2.3.0: it was taken away); a fitted magnetic card can still be fed afterwards. The constant memory file remembers the fitted cue card.
- While the PC-100C is disconnected, the paper panel shows an empty paper without a name (2.3.0: the paper stayed in view); the paper is back when the printer is connected again.
- Traces: one at a time (2.3.0: the button's and a preset's could run side by side); a trace holds the CPU's instructions and gaps only (2.3.0: the button's trace also recorded keys and cards); a trace that cannot be started fails its preset (2.3.0: an alert, and the preset went on); a notice says when the size limit ends a preset's trace (2.3.0: only the button's).
- Files: a file named at the start of the program, or opened with Open With, is opened as what it is (2.3.0: always as a preset). Refusals that were silent are now notices: a file the app cannot open, a card on a model without a card reader, a card while one is in the reader. A card the ROM does not take at once stays at the reader until it does; an empty side gives the ROM zeroes, and the ROM ends the read with an error.
- An ASM overlay runs until a Reset, a Cold boot or Clear: FREEZE no longer opens the CALCULATOR tab again while the processor is inside the overlay, and FREEZE, STEP and RESUME work on its instructions.
- A Reset keeps the partition on the TI-59 and TI-58 (2.3.0: the partition went back to the ROM's default).
- A key that a preset or a host key sequence presses is held for 100 ms of emulated time (2.3.0: of wall time). At full speed a key therefore no longer keeps the calculator running longer: Adv feeds the same lines at every speed (2.3.0: one press fed many lines at full speed), and an `Elapsed:` line over a full-speed stretch counts less.
- The sides of a magnetic card are 0 and 1 (2.3.0: 1 and 2), in presets, in the card file and on screen.
- The PC-100C's TRACE switch keeps its position when the printer is disconnected and connected again and through a model switch, as the real switch does (2.3.0: off after each). A preset sets it off unless it says `printer-trace: on` (2.3.0: a preset without `PRINTER: on` kept it).
- Under Examples, `fastmode-sst` is the last of "Programs", not tenth after the printer pictures.
- A magnetic card carries its own cue card (from `magnetic-cards:` or from its card file), never the preset's `cue-card:`; a card without one shows a plain card titled with its name. A card a preset feeds is fitted after it has passed through the reader, in-file cards too.
- A keystroke program file (`.keystrokes.yaml`) can be opened or dropped: its program goes into the running calculator without a cold boot. A Solid State ROM module file or the constant memory file is refused with a notice.
- On the web, the calculator fetches all 24 ROM files at the start (2.3.0: 6 or 7), and the installed app caches them all.
- On iOS, traces are written to Application Support by default, no longer to iCloud.
- On the web, a new release takes over at the next start, not while a page or the installed app is open.
- Full speed on macOS, iOS, Windows and Linux is paced at the app's tick, so its top speed is a little lower; on the web the time allowed per tick goes from 10 to 14 ms. A held key's release wait no longer counts as idle waiting.
- At regular speed the TI-59 and TI-58C no longer run 0.13 % and 0.10 % slow.
- On the TI-59 and TI-58, an armed CPU FREEZE ON START also fires on a Reset, not only on a Cold boot.
- On the web, the calculator fetches all 24 ROM files when it starts (2.3.0: the model's and the module's files, the rest when chosen); the installed app had them cached already. A module or model chosen later is no longer fetched then, so it is never missing offline.
- Windows and Linux: a folder picked in the Open Preset or ASM overlay dialog while a fixed folder is set for that kind is no longer remembered; after the fixed folder is reset, the dialog starts in the former fixed folder (2.3.0: it went back to the last pick).
- Windows and Linux: the window comes back at its last normal size and place, not maximised or full screen, on the screen where that place lies (2.3.0: the maximised state and the screen were restored).
- Web: the status line says "Starting Calc-U-59…" while the calculator starts (2.3.0: "Starting the emulator…"), and a calculator that cannot start says "Calc-U-59 could not start: " and why, missing or bad ROM files included (2.3.0: "Calc-U-59 cannot start." above the list). An error keeps its title, such as "State file load error: …" (2.3.0: the message alone for a preset). Site authors: the component's `keyboard` prop is on by default (2.3.0: off), `model` takes the model's name only, and the `module` prop and the demo page's `?module=` are gone (a preset sets the module); an unknown `model` or `preset` gives a notice.
- Reset, Cold boot, a model switch, a module change and connecting or disconnecting the PC-100C run as operations, shown as busy; keys pressed meanwhile are dropped.
- An on-screen key is held for 100 ms of emulated time and until the ROM has read it, so at full speed a held Adv feeds the lines it feeds at regular speed, and a tap of R/S during printing stops the program. A key-up releases only its own key; when the window loses focus, the keys come up after their 100 ms; a sequence typed while the debugger is frozen waits until the calculator runs.
- Web: a letter key types the letter the calculator key carries.
- A preset waits after each item until the calculator is idle and fails an item not finished within `timeout:` (default one hour of emulated time); the last item is not waited for unless `let-it-finish:` follows. A refused preset shows only its first error, with its line (2.3.0: all errors were listed). Presets gain the steps `expect:`, `not-expect:` and `screenshot:`.
- Two media names that differ only in upper and lower case are one name (2.3.0: the file system decided); a failed card write is reported with a notice (2.3.0: silent). Web: Import Media… and Saved Media… (list, download, delete after asking) replace the per-card download and the upload in the card picker.
- On Windows, Linux, macOS, iOS and the web, an on-screen key stays the one first pressed; dragging the pointer onto another key no longer moves the press there.
- A control that cannot be used is shown disabled and says why. While a preset loads, the model, module, printer and card controls are disabled; a file dropped while a preset loads is kept and opened afterwards.
- Web: the paper's text/dots control shows the view that is on (2.3.0: it showed the other).
- Windows and Linux: the file named at the start of the program is the last argument that does not start with `-` and names an existing file, as in Calc-U-1600 (2.3.0: the last argument that was not an option value, used without a check).
- Windows, Linux and web: Copy of the paper gives a picture without the white margin around the print, and "Copied to clipboard" or "Copy failed" shows in the paper panel (2.3.0: no message). On Windows and Linux the paper follows the newest line only while the view is near the end (2.3.0: it jumped to the end on every line). The trace size limit counts the setting's megabytes as MiB (2.3.0: 1,000,000 bytes).
- In a preset, `card:` or `arm:` with a card file or name that does not exist refuses the preset (2.3.0: a blank card went in and was saved under that name at its first write); a new card in a preset is an in-file card of `magnetic-cards:`, saved by `saveas:`.
- The card buttons' names (for a screen reader), the File menu items and the card dialogs read "Arm Card" and "Arm Fresh Card" (2.3.0: "Fit Card", "Fit Fresh Card").
- The card dialogs list the saved cards by the names inside them (2.3.0: by file name, `card.U59` by default); a card's file is named after its name (`<name>.magnetcard.yaml`).
- Web: Help and About are in the Help menu (wide layout) or behind the More button (portrait), no longer in the settings dialog. About shows the summary and links to the source, to support and to the privacy statement, as on Windows and Linux (2.3.0: a line and a link to calc-u.org/59). Behind the More button are also Import Media… and Saved Media… (the TI-59's cards) and About, besides Examples, Cold boot, Resume and Help.
- Windows, Linux and web: the settings dialog has "Startup model" with "Last used" first (2.3.0: "Startup Calculator", with "Last Used" last on Windows and Linux), "Save folder" (2.3.0: "Card Folder"), the Windows and Linux rows grouped in Storage, Folders and Tracing (2.3.0: "Files"), a Startup presets section, every change applied at once (2.3.0: the module and, on the web, the trace size when the dialog closed), and the version as a line under General.
- Windows and Linux: when a ROM file is missing, the window opens and shows which files are missing in place of the calculator, and Help and About work (2.3.0: a message box, then the app quit). A notice is shown without a title (2.3.0: a title taken from the sentence).
- Web: while what is saved is not kept (another tab, unreadable or unavailable storage), the storage sentence stays in a line above the calculator, besides its notice (2.3.0: a status line until dismissed).
- macOS and iOS: a notice is an alert with only its sentence and OK, and several are shown one after the other (2.3.0: a titled alert, a second one replacing the first). "Loading…" is shown during a long load. Settings show the family's rows (Startup model with "Last used" first, a Startup presets section) and no Solid State Module; the section "Trace File" is "Tracing", and on iOS it also offers where traces go (Local or iCloud). The paper panel shows its printer's name above the paper, and an empty paper while no printer is connected.
- A library program without a pre-printed cue card shows the stand-in card of its module, titled with the module's name as the menus show it, such as "Applied Statistics" (2.3.0: the module's printed title, such as "MASTER LIBRARY MODULE -1-"). A card a preset feeds with `card:` as its last item is left at the reader when the program does not take it.
- Developers: the harness names Calc-U-59 `c59`: `calcu --product c59` (`59` is refused; `Calc-U-59` still works). The product commands are `c59 asm|dump|log-level|heatmap|partition|program|coverage|line-times|run-time|hot-steps|step-trace|step-cost|keycodes` (were `ti59 …`), and the data registers are `c59 data-registers` (was `ti59 registers`; session command `c59.data-registers`, was `c59.registers`). The common command `lcd` is `display-text`. The debug panel's topics carry the prefix: `c59.calculator-debug`, `c59.program-listing`, `c59.cpu-debug`, `c59.heatmap`, `c59.asm-overlay`, `c59.debug-view` (`state …` takes the new names); the command for the shown CPU tab is `c59.show-cpu-debug` (was `c59.cpu-debug`). `state c59.printer` is gone (the PC-100C's TRACE switch is in `control-state`, its place in `capabilities`), and `c59.led` is `c59.calc-indicator`. Scripts that call the old names must be changed.
- The magnetic card file (`.magnetcard.yaml`) and the TI-58C's constant memory file (`ti58c.memory.yaml`) start with a comment line (`# Calc-U-59 magnetic card`, `# Calc-U-59 TI-58C constant memory`); the constant memory file also carries `saved:`, the time of the write, as the card file does.
- Site authors: the component `calc.js` defines is `CalcU59` (file `src/CalcU59.jsx`), with `CalcU59.portraitWidth(h)`; 2.3.0's global `PlayCalculator` is gone, so a page that mounts it must rename it. The props are the family's shared ones in the order `width`, `keyboard`, `layout` (`portrait`, the default, `wide`, `auto`), `preset`, then `height`, `model`, `workerUrl`, then Calc-U-59's `settings`; `module` is still ignored. The release's `Calc-U-59-web.zip` is a release build; the page's test-only commands `c59.web.key-button-down`/`-up` are gone (the key grid sends `screen-key-down`/`-up`).
- Windows and Linux: a file dropped onto the window or opened with Open With brings the window to the front, the drop shows as accepted, and the file is opened after the drop, as in Calc-U-1600 (2.3.0: the window stayed where it was, and the drop was opened during the drag). Web: a drop of exactly one file is opened and a drop of more is ignored (2.3.0: the first file of a drop was opened), and no drop reaches the browser.
- The trace folder and the trace size limit are read at each trace start, so a change in Settings is used by the next trace (2.3.0: those of the app's start). The folder is created and written once at the start of a trace; where that fails, the start is refused with "The trace folder cannot be written: <reason>." (2.3.0: the TRACE button could not be pressed where the folder could not be created, and no write was tried). `[trace] max-file-size-mb` accepts zero and negatives, which mean 50 MiB (2.3.0: the settings dialog started at 1). macOS: the folder chosen is the one its bookmark grants, at each trace.
- Web: a preset's `screenshot:` step offers the picture as a download, as Calc-U-1600's page does (2.3.0: it was written into the browser's save folder, where nobody could reach it); the LOG tab lists only traces.
- Developers: the harness command and the preset step that set a control of the PC-100C (`set-control`) without a PC-100C plugged in are refused with "No PC-100C is plugged in." (2.3.0: accepted). No GUI can send it then.
- Windows, Linux and web: the paper panel's PRINT and ADV keys show as pressed while they are held (2.3.0: always drawn up); the TRACE switch shows its actual position.
- A save folder or screenshot folder changed in Settings is used at once, by the next save (a card, the TI-58C's `ti58c.memory.yaml`, a preset's screenshot), with no restart (2.3.0: the folders of the start were used until the next start).
- Developers (`calcu --product c59`): the command `switch <peripheral> <control> on|off` is `set-control <peripheral> <control> on|off` (`switch` is an unknown command); `key` without a key replies "refused: usage: key <key ID> ..." and the session's refusal reads "The key command needs one or more key IDs."; a preset's `key:` without a key is refused with "key: needs one or more key IDs.", and `key-down`/`key-up` without `key` with "The key command needs key: <key ID>."; `sync-clock` (was `syncclock`) is refused on every TI model, as before. Site authors: a `layout` that is not `portrait`, `wide` or `auto` is ignored with a console warning and draws portrait, as before (it used to be taken silently).

Still to add to the CHANGELOG entry:

- (partly) Files: a file named at the start of the program is opened as what it is (2.3.0: always as a preset); the CHANGELOG says this only for Open With.
- (partly) On the web, the time allowed per tick at full speed goes from 10 to 14 ms; a held key's release wait no longer counts as idle waiting.
- (partly) Windows and Linux: after the fixed folder is reset, the file dialog starts in the former fixed folder.
- (partly) Web: an error keeps its title, such as "State file load error: …" (2.3.0: the message alone for a preset).
- (partly) When the window loses focus, the held on-screen keys come up after their 100 ms.
- (partly) The settings dialog shows the version as a line under General; on the web, too, it has "Startup model" with "Last used" first, "Save folder", a Startup presets section, and every change applies at once.
- (partly) Windows and Linux: a notice is shown without a title (2.3.0: a title taken from the sentence).
- Developers: the harness names Calc-U-59 `c59` (`calcu --product c59`; `59` is refused), the product commands are `c59 …` (were `ti59 …`), `ti59 registers` is `c59 data-registers`, `lcd` is `display-text`, the debug panel's topics carry the `c59.` prefix, the command for the shown CPU tab is `c59.show-cpu-debug` (was `c59.cpu-debug`), `state c59.printer` is gone and `c59.led` is `c59.calc-indicator`. Scripts that call the old names must be changed.
- (partly) Site authors: the release's `Calc-U-59-web.zip` is a release build; the page's test-only commands `c59.web.key-button-down`/`-up` are gone.
- (partly) Web: the LOG tab lists only traces.
- Developers: the harness command and the preset step `set-control` for the PC-100C without a PC-100C plugged in are refused with "No PC-100C is plugged in." (2.3.0: accepted).
- (partly) Developers (`calcu --product c59`): `switch` is `set-control`; `key` without a key replies "refused: usage: key <key ID> ..."; a preset's `key:` without a key is refused with "key: needs one or more key IDs."; `sync-clock` (was `syncclock`) is refused on every TI model.

### Calc-U-1600 0.9.0 (against 0.8.1)

- Presets with `format-version: 1` are refused as files that cannot be read; the format is version 2. Module definitions are `.module.yaml` files with `format: memory-module`; `.card.yaml` is no file of the product. Floppy disks are version 2, and those of 0.8.x cannot be read until converted by hand (help: "Media files", "From 0.8").
- Settings start at their defaults (INI file); the settings of 0.8.1 are not carried over.
- Preset syntax: `- key:` takes several key IDs; `- wait:` needs a unit; the hardware key `plotter:` becomes `printer:`; references are written `{ name: }`, `{ file: }`, `{ folder: }`; a floppy disk is inserted by the step `- floppy:` (sides 0 and 1), a tape by `- tape:`, and `- eject:` empties either; `cut-paper:`, `skip-wait:` and `let-it-finish:` are new; the debugger steps are not available on the Sharp models.
- Choosing a module power-cycles the machine (0.8.1: a restart like a model switch). The memory is kept through the power cycle; the ROM then finds the changed memory and asks you to set BASIC up again (`NEW0? :CHECK`), as on the real device and as after the old restart.
- A media name with a character a file name cannot hold is refused (0.8.1: `/` and `:` silently became `-`).
- A media file that cannot be read is not listed in its picker; the log says once why.
- A held host key stays down until it is released, for every key, so the ROM's own auto-repeat works; the PC-1500's fixed key cadence is gone.
- Keys pressed while a preset, a load, a reset or a power cycle runs are dropped (0.8.1: held back on the desktop, passed on in the web version). Reset, Cold boot and a model switch stop what runs.
- Menus and the control bar follow the family structure: Settings… is in File; Help has a help item; Examples is a menu of its own in the web version; on the PC-1600's bar a printer picker (none, CE-150, CE-1600P) replaces the CE-1600P button and the CE-158 has a button; a notice has no title; a control that cannot be used says why. On the PC-1500 and PC-1500A the bar has the buttons CE-150 and CE-158; Reset All is Cold boot.
- A preset is checked as far as possible before the running machine is given up: a missing module or disk, a host drive folder that does not exist, a ROM image that does not fit, a key that is none and a character without a key refuse the preset and leave the machine alone (0.8.1: found after the old machine was gone). A preset opened without its folder is refused when it names a file.
- The paper panel is always shown: empty and without a name while no plotter is plugged in. The CE-150 and the CE-1600P each keep their paper until Cut, a preset that names them, or the end of the program: through detaching, a change of ROM, a module change and a model switch, also from a PC-1500 to a PC-1600 (0.8.1: the panel was there only with a plotter, and the paper was lost at each of these). In the web version's narrow window, where only one docked panel fits, the paper panel gives way to the INSPECTOR while no plotter is plugged in.
- Media: a failed autosave is reported with a notice (0.8.1: silent); every change is written when the web version's tab is hidden; a module change leaves the other slot and the disk alone; Name & Save asks before it replaces an instance of that name (0.8.1: the name was refused); deleting a saved medium in the web version asks first.
- Traces: the size limit holds for a preset's trace too, and a notice says when it ends a trace (0.8.1: a line in the INSPECTOR, for the button's trace only). The run control can pause the machine on the desktop and on the web (the harness has `pause`, `step`, `resume`); the Machine menu has Resume. "Copied to clipboard" is shown after Copy on the paper. The size limit counts MiB (0.8.1: 1,000,000 bytes per MB). The title "Paused (debugger)" shows whoever paused the machine.
- Files: a disk file can be dropped and is used where it lies (web: imported); a module file that is opened is plugged in when it is clear where it goes (one slot, or a module that fits only one), and refused with a notice otherwise; `.rom` is no longer a machine-code file name, on a drop or with Open With; a file named at the start of the program is opened (0.8.1: passed over); a drop no longer changes the folder a file dialog starts in; a file over 1 MiB and a file the app cannot open are refused with a notice (0.8.1: silently). A PC-1500 machine-code load that meets a byte that is not RAM writes nothing (0.8.1: the rest, then it failed); an empty file is refused as empty by every loader; each loader refuses with the same wording. A drag over the desktop window is accepted for any one file; a file the app does not open is refused with a notice at the drop (0.8.1: the drag itself refused it).
- A Cold boot also empties a RAM module that has no battery of its own (CE-151, CE-155); a battery-backed module keeps its content (0.8.1: Reset All left every module as it was).
- Saving a CE-161 with the write-protect switch set power-cycles the machine, as plugging in a module does (0.8.1: nothing followed).
- A cassette tape is a tape definition with its WAV: it has a name and a description, can be a template, and gets Name & Save; the picker offers the empty tape and the saved tapes. A WAV in the old tape folder is not listed as a tape until it is wrapped in a definition; a dropped or opened WAV is loaded as a program.
- The sides of a floppy disk are 0 and 1 (0.8.1: A and B), in presets, in the disk file and on screen. The disk file is version 2, so a disk saved with 0.8.x cannot be read until you convert it by hand (rename `a:`/`b:` to `0:`/`1:`).
- Debugging: when a debug session starts, a program started with `after: call` runs only after the client's breakpoints are set, not already during attach, so a breakpoint near the entry is not missed. `--dap` with a port outside 1024 to 65535 is reported, and the server does not listen in that run (0.8.1: passed over without a word). The harness accepts a debugger too (`calcu --product 1600 serve --dap <port>`). The VS Code extension's description of `program.after` names `none` as its default.
- Only the presets of the bundle file and the files it lists are bundled, not their whole folders: nine examples outside the list no longer start with `?preset=` on the web. The release no longer has `Calc-U-1600-examples.zip`; the help explains how to get every example from the source repository.
- Attaching the CE-1600P while the CE-158 is attached, or the other way round, is refused ("The CE-1600P and the CE-158 cannot be used together."), in presets and the harness too (0.8.1: the other one was detached without a word).
- After a stall the machine no longer catches up for up to 0.1 s; the lag is dropped whole.
- An operation runs in parts of 14 ms instead of 10 ms, so loading is slightly faster.
- On the web, a new release takes over at the next start, not while a page or the installed app is open.
- A folder picked in a file dialog while a fixed folder is set for that kind is no longer remembered; after the fixed folder is reset, the dialog starts in the former fixed folder (0.8.1: it went back to the last pick).
- While an operation runs (a preset, a reset, a model switch, a power cycle), the desktop window stays usable: it repaints, the plotter paper and the control bar follow the preset as it arms and runs, and a second operation is refused with "Another operation is running."; keys are dropped. A failed operation, a refused attach and a bay's error appear as a notice without a title, no longer under the operation's or the bay's name; on the web, the second tab's storage sentence is also such a notice.
- On macOS the desktop app keeps real-time pace while its window is not in front: it holds an activity against App Nap for the whole run (0.8.1: App Nap stretched its frame timer, so a background machine fell behind).
- `ent` is no longer a key name; every press is held at least 4 frames, live keys too (0.8.1: 2).
- Every preset item but the last waits until the machine is idle (0.8.1: `key:` did not wait, `type:` waited at most 1 s or 12 s); an item past `timeout:` fails; after a preset the machine runs at regular speed whatever ran before; `expect:` after `type: RUN` checks when the program has ended; a preset's screenshot goes to the save folder (0.8.1: the trace folder), on the web as a download. Presets gain `not-expect:` and regular expressions.
- Two media names that differ only in upper and lower case are one name (0.8.1: two); a saved media file that cannot be read, or that carries a bundled template's name, is named in the log at each start (0.8.1: left out silently).
- A paper feed without a mark shows as blank paper in the panel and in Copy (0.8.1: the paper ended at the last stroke); the plotter's Copy picture loses the 24 points of white above and below the plot.
- While something loads, the controls that would start another operation are disabled (desktop; on the web also the peripheral buttons, the disk picker and the tape controls); "Loading…" is shown over the faceplate without a title and blocks nothing (0.8.1: a window-modal dialog with the operation's title, and a wait cursor).
- After a Reset or a Cold boot a plotter goes on below what it drew, where the paper stands (0.8.1: the next plot started again at the top of the paper, over the earlier one).
- Web: the page binds no shortcut to a menu item: ⌘R / Ctrl+R, ⌘⇧R, ⌘, and ⌘C no longer act there (they are the browser's); pasting still types. Desktop: Ctrl+O (⌘O) is Load Preset…. The startup model is chosen by the model's name ("PC-1500"; 0.8.1: "PC-1500 (ROM A04)").
- When a ROM file is missing, the desktop window opens and shows which files are missing in place of the calculator, and Help and About work (0.8.1: a message box, then the app quit). On the web the same text stands in place of the calculator, without the title "Calc-U-1600 stopped".
- A mouse click in the gap between two keys, within 24 pixels of a key, presses that key, as a touch already did on the web (0.8.1: a click had to hit the key); the desktop faceplate also takes touch.
- File ▸ Mount Directory while a folder is mounted: the new folder replaces the host drive with a power cycle, as any change on the 60-pin connector (0.8.1: only the folder changed, live); the memory is kept either way. Machine ▸ Reset, Cold boot and a model switch each end when the machine is ready for keys; a rebuild of a PC-1500 or PC-1500A is a cold boot too (not visible: its memory is new either way).
- Load Machine Code…: the address dialog says where the code goes for every address that holds it, in the internal RAM too (0.8.1: only for a module or the work area). A preset's typed `program:` goes through the typing engine and waits for the prompt after each line (0.8.1: the keys were pressed directly, waiting for the program counter to settle): the stored program is the same, the time a load takes may differ.
- The cassette's tape picker and counter show whenever a CE-150 (PC-1500/1500A) or a CE-1600P is plugged in; a tape is taken out when its interface is detached.
- The CE-150's cassette is not offered on the PC-1600; its plotter works there.
- Detaching the CE-1600P takes the disk out of the CE-1600F: attached again, the drive is empty (0.8.1: the disk picker kept its disk and put it back). Its ROM switch keeps the disk, the same side up, and the tape, as before.
- A module slot's picker lists the empty slot, the battery-backed modules (bundled templates, your templates, the saved instances), then the modules that are not media: the CE-151, the CE-155 and the ROM modules (0.8.1: the CE-151 and CE-155 were among the templates, and the ROM modules in a section of their own); each where it fits the slot.
- On the PC-1600 a preset's item waits while the CE-1600F's motor runs: a disk command (`INIT"X:"`, `SAVE"X:…"`) is done before the next item starts (0.8.1 waited for BUSY for at most 1 s). A load with disk commands may take longer.
- File ▸ Load Preset…'s file dialog (desktop) and Upload Preset… (web) open the chosen file as what it is, like a drop: a BASIC or machine-code program goes to its loader, a module, disk or tape file into its place (0.8.1: the file was always read as a preset). A cassette WAV with several files that is dropped or opened asks which one, and a machine-code file on it goes to the machine-code flow (0.8.1: the BASIC loader took the file chosen).
- Machine ▸ CE-1600P ROM ▸ is shown while the CE-1600P is plugged in (0.8.1: also on a PC-1600 without it, to choose the ROM the next attach uses); the menus and the bar attach the CE-1600P with the New ROM unless a preset or the harness names the Old one. The desktop bar shows the tape controls before the disk controls (0.8.1: the disk first). The module, disk and tape pickers are the same on desktop and web: the empty entry reads "(empty)" (0.8.1: "–empty–") and the groups have no separators. On the web, Import Media… is one file picker for module, disk and tape files and the tape's `.wav` (picked with its definition), and "Calc-U-1600 is open in another tab" is shown as the banner only.
- Developers: the command-line tools `pc1500_cli` and `pc1600_cli`, the probes `pc1600_plotter_probe`, `pc1600_power_probe`, `pc1600_uart_probe`, their build scripts and the one-time checks `dev/tape-matrix/` and `dev/loader-matrix/` are gone. Their functions are commands of the harness `calcu --product c1600` (`preset`, `wait`, `display-text`, `screenshot`, `audio`, `peek`, `registers`, `history`, `profile`, `eject`, and under `c1600` `inspect`, `cassette record|status`, `serial ce158|rs232c feed|capture|link|report`, `host-drive`, `call`); the bare ROM-file mode of `pc1500_cli`, the probes and the matrices have no successor; what `--dump-basic` printed is in the INSPECTOR views `pointers` and `dump-basic` (`c1600 inspect <view>`). The harness gives a serial port no pseudo-terminal unless `link` asks (the old tools: `--ce158-pty`), and `display-text` reads the cell under the cursor as its character, as `--lcd-text` did.
- The user guide is the help: the family's ten chapters, the parts shared with Calc-U-59 written once, and Calc-U-1600's own documents (User Guide, Debugger, keyboard mapping, host drive, keyword modules, memory module definition spec and format) converted into it; each release carries it as `Calc-U-1600-help.zip`, which calc-u.org/1600 shows. The memory module definition documents are now "Module definitions" (chapter 9) and "Module definition format" (chapter 10); the help describes the menus and commands of this release, not 0.8's.
- Web: the storage banner says a sentence for each case of the save folder (another tab, unreadable, unavailable), not "Calc-U-1600 is open in another tab" for all three.
- Examples ▸ Plotter offers one more example, "Rectangles 4 pens" (`plotter/rectangles-1600.pc1600`): four nested rectangles on the CE-1600P, one in each colour. The help's paper and overview pictures show it instead of the Lissajous figure.
- Developers: the harness names Calc-U-1600 `c1600`: `calcu --product c1600` (`1600` is refused; `Calc-U-1600` still works), the product commands `c1600 serial|cassette|inspect|call|load-code|host-drive|file-kind|load-basic|type-basic` (were `pc1600 …`); the common command `lcd` is `display-text`, and the web page's request `c1600.lcd-text` is `c1600.display-text`. The debug server's own requests are `c1600/load`, `c1600/reset`, `c1600/screen`, `c1600/quit` (were `calcu1600/…`). The VS Code extension's debug type is `c1600` (was `calcu1600`), its commands `c1600.buildAndLoad`, `c1600.resetAndStop`, `c1600.allResetAndStop`, `c1600.createProject`, `c1600.createProjectHere`, its settings `c1600.port`, `c1600.zasmPath`, `c1600.sdccBinPath`, `c1600.romListings`, `c1600.romSymbols` and its problem matchers `$c1600-sdas`, `$c1600-zasm`: a project's `launch.json`, `tasks.json` and `settings.json` written for the old extension must be made again (Create Project, or the configurations VS Code offers) or edited to the new names. The extension's name and publisher (`calcu1600-debug`) stay.
- Module definitions are `.module.yaml` files with `format: memory-module`; the bundled modules, the examples' modules and the CE-502B's definition in the ROM folder (`CE-502B.module.yaml`, the same bytes) are renamed, and a module saved by Name & Save or autosave carries `saved:` (the time of the write) and the leading comment `# Calc-U-1600 memory module instance …`. The cassette is called a cassette everywhere: its file is a cassette definition `.cassette.yaml` with `format: cassette` and `cassette-name:` (a `.tape.yaml` is not read), and each one written carries `saved:`; the preset step is `cassette:` and the eject word `cassette` (`eject: cassette`, `saveas: { place: cassette }`); the cassette folder is `[storage] cassette-folder`, default `Cassettes` in the save folder (was `tape-folder`, `Tapes`); the control bar, the menus and the dialogs say "Cassette" ("Cassette:", "Save Cassette", "Cassette name:"); the harness command is `c1600 cassette record|status`, its reply `cassette: …`.
- Site authors: `CalcU1600` takes the shared prop `layout`; it has one layout and takes only `"auto"` (its default), another value is ignored with a console warning. The props are in the family's order `width`, `keyboard`, `layout`, `preset`, then `height`, `model` (its index or its name, as before), `workerUrl`. The release's `Calc-U-1600-web.zip` is a release build: its session answers neither `c1600.upload` nor `c1600.display-text`, the web tests' commands.
- A trace that cannot be started because the trace folder cannot be written is refused with one sentence, "The trace folder cannot be written: cannot write to the folder …." `[trace] max-file-size-mb` accepts zero and negatives, which mean 50 MiB (0.8.1: the settings dialog started at 1). The folder and the limit are read at each trace start, as before.
- Labels: the INSPECTOR's button "Card YAML" is "Module YAML" (its tooltip "… YAML for a module definition"), and the lines it prints begin "Module dump:" (was "Module card dump:"; "no module plugged in" was "no card attached"); File ▸ "Mount Directory…" is "Host Drive…" (its folder dialog "Host Drive (S3:)", was "Mount Directory as S3:"); Settings ▸ Folders reads "Presets folder" and "BASIC folder" (were "Examples folder", "Basic folder"); the web page's Settings ▸ Storage sentence says "Saved media (modules, disks and cassettes) …" (was "tapes"). A cassette or disk name with a `"` or a line end is refused when it is written, with "A cassette name has no " and no line end." or "A disk name has no " and no line end." (0.8.1: it was written, and the file could not be read again).
- Developers (`calcu --product c1600`): the command `switch <peripheral> <control> on|off` is `set-control <peripheral> <control> on|off` (`switch` is an unknown command); `key` without a key replies "refused: usage: key <key ID> ..." and the session's refusal reads "The key command needs one or more key IDs."; a preset's `key:` without a key is refused with "key: needs one or more key IDs.", and `key-down`/`key-up` without `key` with "The key command needs key: <key ID>.". Presets: the step `syncclock:` is `sync-clock:` (and so is the harness command); a preset with `syncclock:` is refused as an unknown keyword; the bundled startup preset `default-pc1600.pc1600` uses `sync-clock:`, and its log line reads `sync-clock`. Debug server (VS Code users of custom requests): the request `c1600/screen` is `c1600/display-text` and answers with the keys of the harness's `display-text --json`: `on`, `rows`, `indicators` (`[{name, brightness}]`) and `notText` (was `rows`, `status`, `unparsed`, `poweredOn`, `cursor`); the extension sends neither. Site authors: `model` and `?model=` take the model's name only (`PC-1600`; an index such as `2` gives the notice "There is no model …"); `preset` and `?preset=` take a bundled preset's path with or without `examples:` (a given prefix used to be doubled, and the preset was not found).
- Presets and harness: the PC-1600's `ctrl`, `kbii` and `bs` can be pressed by name, as in 0.8.1 (no change against 0.8.1).
- Developers: automated tests with pytest: the guide `docs/testing/README.md` (setup, a test project of your own, three worked examples, path names), the driver's module files and media of your own, printer, bus ROMs, `load_basic`/`type_basic`, `listing()` with the new harness command `c1600 list-basic`. The driver began as the one "spellbound" wrote and is developed further together with spellbound.

Still to add to the CHANGELOG entry:

- (partly) Files: each loader refuses with the same wording. A drag over the desktop window is accepted for any one file; a file the app does not open is refused with a notice at the drop (0.8.1: the drag itself refused it).
- (partly) After the fixed folder is reset, the file dialog starts in the former fixed folder (0.8.1: it went back to the last pick).
- (partly) Developers: the web page's request `c1600.lcd-text` is `c1600.display-text`; the harness's product command list also includes `load-code`, `file-kind`, `load-basic` and `type-basic`; the VS Code extension's `settings.json` must be edited too, and its name and publisher (`calcu1600-debug`) stay.
- (partly) Developers: a preset's `key:` without a key is refused with "key: needs one or more key IDs.", and `key-down`/`key-up` without `key` with "The key command needs key: <key ID>."; the bundled startup preset `default-pc1600.pc1600` uses `sync-clock:`, and its log line reads `sync-clock`.

## Closing check

Phase F ends when:
- [ ] both releases are out (GitHub releases; Calc-U-59 in the App Store);
- [ ] both old repositories are archived;
- [ ] calc-u.org shows both products with the help and Web frontend of
      their releases.

Then:
- [ ] move `docs/phase-f.md` and `docs/help-site.md` to the branch
      `history` (added there, deleted on `main`);
- [ ] remove their pointers from `CLAUDE.md` and from the top of
      `docs/Process.md`;
- [ ] from then on, work by `docs/Process.md` only.
