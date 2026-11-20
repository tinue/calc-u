# Changelog

## [0.9.0] - work in progress

### New

- **The help is the user guide.** The family's ten chapters, with the
  parts Calc-U-1600 shares with Calc-U-59 written once, replace the
  User Guide, Debugger, keyboard mapping, host drive, keyword module and
  memory module documents of `docs/`. The memory module definition
  documents are now "Module definitions" (chapter 9) and "Module
  definition format" (chapter 10). The help describes the menus and
  commands of this release; each release carries it as
  `Calc-U-1600-help.zip`, which calc-u.org/1600 shows.
- **Pause and Resume in both versions.** The run control can pause the
  machine in the desktop and the Web version, Machine ▸ Resume continues
  it, and the title "Paused (debugger)" shows whoever paused it.
- **A new plotter example:** Examples ▸ Plotter ▸ "Rectangles 4 pens"
  (`plotter/rectangles-1600.pc1600`) draws four nested rectangles on the
  CE-1600P, one in each colour. The help's paper and overview pictures
  show it instead of the Lissajous figure.
- **The harness `calcu --product c1600` replaces the command-line
  tools.** `pc1500_cli`, `pc1600_cli`, the probes `pc1600_plotter_probe`,
  `pc1600_power_probe` and `pc1600_uart_probe`, their build scripts and
  the one-time checks `dev/tape-matrix/` and `dev/loader-matrix/` are
  gone. Their functions are harness commands (`preset`, `wait`,
  `display-text`, `screenshot`, `audio`, `peek`, `registers`, `history`,
  `profile`, `eject`, and under `c1600` `inspect`,
  `cassette record|status`, `serial`, `host-drive`, `call`); the bare
  ROM-file mode of `pc1500_cli`, the probes and the matrices have no
  successor. What `--dump-basic` printed is in the INSPECTOR's views
  `pointers` (the BASIC pointers) and `dump-basic` (the BASIC area),
  `c1600 inspect <view>`. A serial port gets a pseudo-terminal only when `link` asks
  for one (was `--ce158-pty`). For developers only.
- **Automated tests with pytest.** A user guide
  (`docs/testing/README.md`): setting up on macOS, Linux and WSL, a test
  project of your own, three worked examples (MODE 1 with memory modules,
  RENUM on the PC-1500, the serial ports) that run as a test project in
  the checkout, the
  path names (`$c1600`, `$saved`, your own), the reference. The driver
  (`tools/machine/`) takes module files and media of your own (a module
  file is plugged as a copy, so the machine's writes never reach it), a printer,
  bus ROMs, `load_basic`/`type_basic`, and reads the program with
  `listing()` through the new harness command `c1600 list-basic`. The
  driver began as the one "spellbound" wrote for the superRAM tests and
  is developed further together with spellbound.

### Changed

- **At full speed a waiting machine runs at real time**, as in Calc-U-59,
  so the prompt and a program waiting for a key no longer spin; a preset,
  a load or a reset still runs as fast as the host allows throughout.
- **Presets are format 2.** A preset of version 1 is refused as a file
  that cannot be read (nothing loaded); convert it by hand, as the User
  Guide's "Media files", "From 0.8", describes. The changes
  (`docs/formats/preset.md` §9.2 at the repository root):
  `format-version: 2`; `plotter:` is `printer:`; `slot-1: CE-1600M` is
  `slot-1: { name: CE-1600M }`; `slot-1-rom: x.bin` is
  `slot-1-rom: { file: x.bin }`; `host-drive: dir` is
  `host-drive: { folder: dir }`; `- wait: 2` is `- wait: 2s` (a unit is
  required); `- key: ent` is `- key: enter`; the clock step is
  `- sync-clock:` (was `- syncclock:`, now refused as an unknown
  keyword). `- key:` takes several key IDs. A floppy disk is inserted by
  `- floppy:` (sides 0 and 1), a cassette by `- cassette:`, and
  `- eject:` empties either; `- cut-paper:` is new.
- **Floppy disks are version 2.** Their sides are 0 and 1 (were A and
  B), in presets, in the disk file and on screen. A disk saved with
  0.8.x cannot be read until you convert it by hand: rename `a:` and
  `b:` to `0:` and `1:`.
- **Module definitions are `.module.yaml`** with `format: memory-module`;
  a `.card.yaml` is not read (rename it and add the line). The bundled
  and example modules and the CE-502B's file in `roms/`
  (`CE-502B.module.yaml`, fetched by `roms/fetch_roms.sh`) are renamed. A
  module saved by Name & Save or autosave carries `saved:` with the time
  of the write and the comment line `# Calc-U-1600 memory module
  instance …`.
- **Cassettes:** a cassette is a cassette definition (`.cassette.yaml`
  with `format: cassette` and `cassette-name:`) that names its WAV; a
  `.tape.yaml` is not read. A cassette has a name and a description, can
  be a template and gets Name & Save, and each one written carries
  `saved:`. The picker offers the empty cassette and the saved ones; a
  WAV in the old tape folder is not listed until it is wrapped in a
  cassette definition, and a dropped or opened WAV is loaded as a
  program. The preset step is `cassette:` and `eject: cassette`; the
  folder is `Cassettes` in the save folder (`[storage] cassette-folder`);
  the control bar and its dialogs say "Cassette" ("Cassette:", "Save
  Cassette", "Cassette name:"). The cassette picker and the tape counter
  show whenever a CE-150 (PC-1500/1500A) or a CE-1600P is plugged in,
  and the cassette is taken out when its interface is detached.
- **The CE-150's cassette is not offered on the PC-1600.** Its plotter
  works there; a preset step `cassette:` with it is refused ("the
  CE-150's cassette cannot be used on the PC-1600"), and the control bar,
  the harness and the test driver have no cassette for it.
- **Settings start at their defaults** (the INI file
  `Calc-U-1600.ini`); those of 0.8.x are not carried over. The startup
  model is chosen by the model's name ("PC-1500", was "PC-1500 (ROM
  A04)"). A folder picked in a file dialog while a fixed folder is set
  for that kind is no longer remembered.
- **A media file that cannot be read** (broken, or of another version)
  is not listed in its picker, and the log says once why.
- **Waiting in presets.** The bare `- wait:` is gone: every item but the
  last waits until the machine is idle, at most `timeout:` (a header key,
  default 3600 s of emulated time); an item past it fails.
  `- let-it-finish:` as the last item waits for it; `- skip-wait:` drops
  the wait after the item before it. `expect:` checks once the item
  before has finished (after `type: RUN`, when the program has ended). On
  the PC-1600 an item also waits while the CE-1600F's motor runs, so a
  disk command such as `SAVE"X:…"` is done before the next item starts. A
  typed `program:` goes through the typing engine and waits for the
  prompt after each line: the stored program is the same, a load may
  take longer. A preset always ends at regular speed.
- **New in presets:** `not-expect:`, and regular expressions `/…/` for
  `expect:` and `not-expect:`. `screenshot:` writes Copy Screen's picture
  into the save folder (in the Web version, a download).
- **Not in presets:** the debugger steps are not available on the Sharp
  models.
- **A preset is checked before the machine is given up.** A missing
  module or disk, a host drive folder that does not exist, a ROM image
  that does not fit, a key that is none and a character without a key
  refuse the preset and leave the running machine alone. A preset opened
  without its folder is refused when it names a file.
- **Renamed:** Machine ▸ Reset All is **Cold boot** (⇧⌘R / Ctrl-Shift-R
  in the desktop version, no shortcut on the Web); Settings ▸ Default
  presets is **Startup presets** (one per model, loaded when the model
  starts or is switched to; one that can't be loaded gives the built-in
  default machine and a notice); "Startup device" is **Startup model**;
  "Battery-card saves:" is **Save folder:**; Settings ▸ Folders reads
  **Presets folder** and **BASIC folder** (were "Examples folder" and
  "Basic folder"); File ▸ Mount Directory… is **Host Drive…** (its
  dialog "Host Drive (S3:)"); the INSPECTOR's "Card YAML" is **Module
  YAML**, and its lines begin "Module dump:". The help says "memory
  module" and "module definition" for "memory card" and "custom memory
  cards".
- **Menus and the control bar follow the family structure.** Settings…
  is in File, Help has a help item, and the Web version has an Examples
  menu of its own; File ▸ Load Preset… is Ctrl+O (⌘O) in the desktop
  version. On the PC-1600's bar a printer picker (none, CE-150,
  CE-1600P) replaces the CE-1600P button and the CE-158 has a button; on
  the PC-1500 and PC-1500A the bar has the buttons CE-150 and CE-158. The
  desktop bar shows the cassette controls before the disk controls.
  Machine ▸ CE-1600P ROM ▸ is shown only while the CE-1600P is plugged
  in, and the menus and the bar attach it with the New ROM. On the Web
  the page binds no shortcut to a menu item: ⌘R / Ctrl+R, ⌘⇧R, ⌘, and ⌘C
  are the browser's again; pasting still types.
- **Notices and loading.** A notice has no title; a control that cannot
  be used is disabled and says why. While something loads, the controls
  that would start another operation are disabled, and "Loading…" shows
  over the faceplate without a title and blocks nothing (was a modal
  dialog and a wait cursor). While an operation runs the desktop window
  repaints, the paper and the control bar follow the preset, and a
  second operation is refused with "Another operation is running.".
- **Keys.** A held host key stays down until it is released, for every
  key, so the ROM's own auto-repeat works; the PC-1500's fixed key
  cadence is gone. Every press is held at least 4 frames (was 2), and
  `ent` is no longer a key name. Keys pressed while a preset, a load, a
  reset or a power cycle runs are dropped, and Reset, Cold boot and a
  model switch stop what runs. A mouse click in the gap between two
  keys, within 24 pixels of a key, presses that key, and the desktop
  faceplate also takes touch.
- **Modules and peripherals.** Choosing a module power-cycles the
  machine; the memory is kept, and the ROM asks to set BASIC up again
  (`NEW0? :CHECK`), as on the real device. A Cold boot also empties a
  RAM module without a battery of its own (CE-151, CE-155). Saving a
  CE-161 with the write-protect switch set power-cycles the machine, as
  plugging in a module does. File ▸ Host Drive… while a folder is
  mounted replaces the host drive with a power cycle; the memory is
  kept. The CE-1600P and the CE-158 cannot be attached together ("The
  CE-1600P and the CE-158 cannot be used together.", in presets and the
  harness too). Detaching the CE-1600P takes the disk out of the
  CE-1600F; its ROM switch keeps the disk, the same side up, and the
  cassette.
- **Media.** A module slot's picker lists the empty slot ("(empty)",
  was "–empty–"), the battery-backed modules (bundled templates, your
  templates, the saved instances), then the CE-151, the CE-155 and the
  ROM modules, each where it fits the slot. A module change leaves the
  other slot and the disk alone. Name & Save asks before it replaces an
  instance of that name (it used to refuse the name). Two media names
  that differ only in upper and lower case are one name; a name with a
  character a file name cannot hold is refused (`/` and `:` were turned
  into `-`).
  A failed autosave is reported with a notice, and a saved file that
  cannot be read, or that carries a bundled template's name, is named in
  the log at each start.
- **Opening files.** File ▸ Load Preset… (Web: Upload Preset…) and a
  drop open a file as what it is: a BASIC or machine-code program goes
  to its loader, a module, disk or cassette file into its place. A disk
  file can be dropped and is used where it lies (Web: imported); a
  module file is plugged in when it is clear where it goes. A file named
  at the start of the program is opened. A cassette WAV with several
  files asks which one, and a machine-code file on it goes to the
  machine-code flow. `.rom` is no longer a machine-code file name. A file
  over 1 MiB and a file the product cannot open are refused with a
  notice, and a drop no longer changes the folder a file dialog starts
  in. Load Machine Code…'s address dialog says where the code goes for
  every address, in the internal RAM too.
- **The paper panel is always shown,** empty and without a name while
  no plotter is plugged in (in the Web version's narrow window it gives
  way to the INSPECTOR then). The CE-150 and the CE-1600P keep their
  paper until Cut, a preset that names them, or the end of the program,
  through detaching, a ROM, module or model switch. A paper feed without
  a mark shows as blank paper, the plotter's Copy picture loses the white
  above and below the plot, and "Copied to clipboard" is shown after
  Copy.
- **Traces:** the size limit holds for a preset's trace too, counts MiB,
  and a notice says when it ends a trace. Zero or less means 50 MiB. A
  trace folder that cannot be written refuses the start with "The trace
  folder cannot be written: …".
- **Pacing:** after a stall the machine no longer catches up for up to
  0.1 s, and an operation runs in parts of 14 ms instead of 10 ms, so
  loading is slightly faster.
- **A missing ROM file** no longer ends the desktop app: the window
  opens and shows which files are missing in place of the calculator,
  and Help and About work. On the Web the same text stands there,
  without the title "Calc-U-1600 stopped".
- **Bundled examples:** the desktop and Web versions bundle exactly the
  files `examples/bundle.yaml` lists, not whole folders, so nine examples
  outside the list no longer start with `?preset=` on the Web. The
  release no longer has `Calc-U-1600-examples.zip`; the help explains
  how to get every example from the source repository.
- **Web version:** a new release takes over at the next start, not
  while a page or the installed app is open. Import Media… is one file
  picker for `.module.yaml`, `.floppy.yaml`, `.cassette.yaml` and a
  cassette's `.wav`; every change is written when the tab is hidden, and
  deleting a saved medium asks first. The storage banner says which case
  holds (another tab, unreadable, unavailable), and the second tab's
  sentence is also a notice; Settings ▸ Storage says "Saved media
  (modules, disks and cassettes) …".
- **The product word is `c1600`.** The VS Code extension's debug type is
  `c1600` (was `calcu1600`), and so are its commands (`c1600.buildAndLoad`,
  …), settings (`c1600.port`, `c1600.zasmPath`, …) and problem matchers
  (`$c1600-sdas`, `$c1600-zasm`): re-create a project's `launch.json` and
  `tasks.json`, or edit them, and rename the settings. The debug server's
  own requests are `c1600/load`, `c1600/reset`, `c1600/display-text`,
  `c1600/quit`; `c1600/display-text` answers with the keys of the
  harness's `display-text` (`on`, `rows`, `indicators`, `notText`). The
  harness is `calcu --product c1600`, its product commands `c1600 …` (were
  `pc1600 …`), and `lcd` is `display-text`; its `switch` is `set-control`,
  and `key` without a key replies "refused: usage: key <key ID> ...".
- **Debugging:** a program started with `after: call` runs only after
  the client's breakpoints are set, so a breakpoint near the entry is
  not missed. `--dap` with a port outside 1024 to 65535 is reported, and
  the server does not listen in that run. The harness serves a debugger
  too (`calcu --product c1600 serve --dap <port>`). The VS Code
  extension's `program.after` names `none` as its default.
- **Web embedding:** `CalcU1600` takes the shared prop `layout`, only
  `"auto"` (another value is ignored with a console warning); the props
  are `width`, `keyboard`, `layout`, `preset`, `height`, `model`,
  `workerUrl`. `model` (and `?model=`) takes the model's name only, not
  its number; `preset` (and `?preset=`) takes a bundled path with or
  without `examples:`. The release's `Calc-U-1600-web.zip` is a release
  build without the Web tests' commands.
- **Build targets:** the core and the app layer are the CMake targets
  `calcu1600_core` and `calcu1600_shared` (were `calcu_core` and
  `calcu_shared`), as Calc-U-59's are `calcu59_core` and
  `calcu59_shared`. For developers only.
- **Web build (developers):** `frontends/web/build.sh` and `run-tests.sh`
  run the library's steps (`library/web/build/product.js`,
  `library/web/tools/playwright.sh`); `build.js` holds the product's
  values, the worker's configuration too (`calc-engine-worker.js` is
  written into `dist/`). Another output folder is `CALCU_WEB_DIST` (was
  `CALCU1600_WEB_DIST`).

### Fixed

- **Desktop: Tab is MODE again.** The buttons and pickers of the control
  bar and the paper panel no longer take the keyboard focus, so Tab
  reaches the calculator instead of moving the focus to them.
- **macOS: the desktop app keeps real-time pace in the background.** It
  holds an activity against App Nap, so a machine whose window is not in
  front no longer falls behind.
- **After a Reset or a Cold boot a plotter goes on below what it drew**,
  where the paper stands, instead of starting again at the top over the
  earlier plot.
- **A PC-1500 machine-code load that meets a byte that is not RAM
  writes nothing**, instead of writing the rest and then failing; an
  empty file is refused as empty by every loader.
- **A cassette or disk name with a `"` or a line end** is refused when it
  is written; it used to be written, and the file could not be read
  again.
- **Web: `?preset=examples:…`** finds the preset; the prefix was doubled.
- **Idle is the same question on every model.** A preset item, the
  harness's `wait` and the test driver wait until the machine waits for
  the user and nothing is busy: the prompt, INPUT, a PRINT stop, a program
  reading INKEY$ or calling the key wait. A running program, a `WAIT n`
  delay, BEEP, a direct `LOAD`, `SAVE` or `COPY` and plotting are waited
  for; the PC-1600 used to count any running program as idle, and the
  PC-1500 an INKEY$ loop as busy. The signals per model are in
  `docs/developer/Idle.md`.
- **A tape or the serial port awaited counts as waiting**, as Calc-U-59's
  card wait does: CLOAD with no cassette in (or at its end), INPUT, LOAD or
  CLOAD from the PC-1600's port or the CE-158 with nothing coming. The step
  returns instead of running into its timeout; while a tape plays or bytes
  come, it is waited for.
- **BREAK goes in while a program runs**, from the harness or the driver
  (a preset's keys wait for the machine as before); it used to wait for the
  key wait and was refused after an hour on the PC-1500.
- **Harness: `key-down` and `key-up` take `break` for `on`**, as `key`
  does; `key … hold=<duration>` holds each key that long, `wait-finished`
  waits until the program has ended, `idle-state` shows whether the machine
  takes keys, waits and has finished. The driver has `key(names, hold=)`
  and `wait_finished()`.
- **Harness: `peek` and `poke` reach both ROM halves of the PC-1600's bank
  3**: `vbank=` on 4000–7FFF is port 3DH's bit 2 (`bank=3 vbank=1` the
  hidden bank 3b), whatever the port selects now.

## [0.8.1] - 2026-11-14

### New

- **CE-161 write-protect switch.** The CE-161 is now battery-backed and
  has its "Set Switch": Name & Save offers **Write-protect**, which saves
  the card read-only for good. On the PC-1500 a prepared, protected CE-161
  works as a program module, as in its manual. Presets:
  `saveas: live protected slot-1:<name>`.

### Changed

- **Card files:** `write-protect:` on regular content is now the switch
  position (`protected` / `unprotected`), not a `default`/`persisted` map.

## [0.8.0] - 2026-11-07

### New

- **A web version, as a preview** at <https://tinue.github.io/1600>. The
  same emulator, compiled to WebAssembly, runs in the browser with the
  desktop app's faceplate, menus, control bar, INSPECTOR, plotter paper
  and CE-158 printer. It is made for tablets and desktop browsers; a phone
  works too (a tap takes the nearest key, the menus are taller), and in
  landscape shows the calculator alone. The browser can install it as an
  app that runs offline. Saved cards, disks and tapes stay in the
  browser: **File ▸ Import Card, Disk or Tape…** brings files in, **File ▸
  Saved Media…** downloads or deletes them, and **File ▸ Load Preset ▸
  Upload Preset…** runs your own preset. Only one tab saves; a second tab
  says so. Drag and drop, copy and paste, the buzzer and turbo (hold the
  LCD) work as in the desktop app; `?model=` and `?preset=` choose what it
  starts with. Not in the browser: the debugger, the serial ports, the
  host drive `S3:`, and TRACE/LOG. A release attaches it as
  `Calc-U-1600-web.zip`, to put on your own web server.

- **Presets, cards and disks name themselves.** A preset can carry
  `preset-name:` (short, shown once chosen) and `description:` (the line
  lists and menus show); cards and disks a `description:`, shown as the
  picker's tooltip and kept by Name & Save. **File ▸ Examples** (in the
  browser: **File ▸ Load Preset**) loads the bundled example presets, and **Settings ▸ Default presets** offers the
  bundled startup presets by description and shows the chosen one by its
  short name -- in the desktop app and in the browser. Which presets are
  bundled is `examples/bundled.yaml`.

- **Fast-load cassette WAVs.** File ▸ Load BASIC Program…, Load Machine
  Code… and a drop open a WAV of a PC-1500 (CE-150) or PC-1600 (CE-1600P)
  tape and write the file on it straight into memory, as for a `.bbin` or
  `.bin` -- no `CLOAD`, no tape time. Real recordings work too: quiet,
  noisy, a little fast or slow. A tape with several files asks which one; a
  damaged file is refused with where it is damaged. Decoding is
  libsharpdx's (SharpDataExchange 0.3.5).

- **Cassette tape: CLOAD and CSAVE.** The CE-150 (PC-1500/1500A) and the
  CE-1600P (PC-1600, MODE 0) now drive a cassette recorder. Its tapes are
  WAV files in a tape folder (Settings ▸ Storage ▸ Tapes): pick one on the
  control bar to `CLOAD` it, or name a new one with the save button to
  `CSAVE` onto it -- saved each time the motor stops, further `CSAVE`s
  appended. A counter shows where the tape is, green while the motor runs. The
  calculator's own ROM does the bit timing, the remote relay runs the tape,
  and the tape runs in emulated time, so turbo doesn't change the WAV.
  The WAVs work with Pocket Tools' `bin2wav` / `wav2bin` (give `bin2wav`
  `-s 3` or `-l 0x400`) and with real recordings; the headless CLIs take
  `--tape-in` / `--tape-out`. As on the real machines, the buzzer sounds
  the tape while saving and loading. Not yet: the PC-1600 reading PC-1500
  tapes in MODE 1, and the CE-150 on a PC-1600.

- **A 3D-printable memory-slot cover** for the PC-1500/1600 (OpenSCAD,
  `3d-print/memory-slot-cover.scad`), all dimensions adjustable.

### Changed

- **The Debug panel is now the INSPECTOR.** Pointers lists many more of
  the work-area variables, in groups and with flags decoded, checked
  against the ROM disassembly; three wrong PC-1500 entries are gone
  (`BREAK_STAT` was the evaluator's type byte, the real BREAK flag is
  `BREAKPARAM` at 788AH; `WARM_START`/`STK_SAVE` lie in an arithmetic
  register). Dump Mem is replaced by two menus. **Memory ▾** has tables:
  Inventory, Z80 View, LH5803/LH5801 View, BASIC Area, Program Areas, Disk
  Areas and Physical RAM. **Dump ▾** has hex dumps of the BASIC area, the
  S1/S2 program areas, the S1:/S2: RAM disks and the PC-1500's
  machine-language area. The views are built in Core
  and the headless CLIs print them with `--inspect <view>`. Dump Card YAML
  is now Card YAML.
- **Presets and memory-card files carry `format-version: 1`**, and it is
  required: add the line to your own presets and to saved cards
  (`.card.yaml` in the Battery-card saves folder). A card without it no
  longer appears in the module picker. Floppies (`.floppy.yaml`) always
  had it. A file written for a newer format is refused with its version
  instead of being misread.
- **A ROM module under development gets its ROM from the preset:**
  `slot-1-rom: build/module.bin` (or `slot-2-rom:`) next to `slot-1-file:`
  replaces the bytes of the module's ROM, read again at every clean start.
  Card files no longer take `encoding: file`; the card keeps a placeholder
  ROM (`$0000: FF...`). docs/Debugger.md has the example.
- **`saveas:` follows the same rules everywhere.** The app and the
  headless tools save cards and floppies through one set of rules:
  `pc1500_cli` now runs `saveas:` too (`--save-dir` for a by-name save),
  and the CLIs refuse a bundled or template name and never overwrite a
  template file, as the app always did.

- **The emulator's app logic is now a GUI-neutral layer (`Shared/`)**
  under the Qt app, which the web version runs on too: model and ROM
  switches, presets, peripheral power cycles, pacing, the module, disk and
  tape bays, settings and paths. Long operations run in slices, so a
  model, module or ROM switch now shows the "Loading…" popup like a preset
  instead of freezing the window. Building needs a C++20 compiler.
- **A narrower control bar.** The ROM pickers (PC-1500 ROM Revision,
  PC-1600 ROM Version, CE-1600P ROM) are in the Machine menu only, and the
  new **Machine ▸ Peripherals** submenu attaches the CE-150, CE-158 and
  CE-1600P. The bar keeps buttons for each model's own peripherals.
- **`PEEK#` and `XPEEK#` read ME1 as the real machines do** (measured):
  ME1 is no longer a mirror of ME0. On the PC-1500 an address nothing
  answers at in ME1 reads 255 (the internal LH5811 answers only at
  F000-FFFF); on the PC-1600 `XPEEK#` returns the last byte on the bus
  (37).
- **Quick taps register.** A key clicked, tapped or typed is held at least
  two frames, and the next press waits for it, so a fast tap or double tap
  is never lost.

### Fixed

- **PC-1600 `.bas` files: `MODE` loads as `MODE` again.** `10 MODE 0`
  loaded as `10 MOD E0` in 0.7.0 (libsharpdx 0.3.4 fixes it).

- **PC-1600: the CE-158 terminal works in MODE 1.** `TERMINAL` and `DTE`
  now show their menu, take the soft keys (F4 = Ent) and display received
  text, scrolling long lines as the real machine does. They used to stop
  after a character or two. The LH-5803 now reaches the LCD, the 1/64 s
  pulse and the keyboard the way the PC-1600's wiring connects them.

- **PC-1600: an OFF in PC-1500 machine code no longer freezes MODE 1.** On
  the LH-5803, OFF (and SDP/RDP) does nothing, as the TRM states.

- **PC-1500: the ON key reads high on PB7 while pressed.** The CE-150's
  tape reader saw BREAK all the time before.

- **The floppy side button's A/B letter is no longer clipped** on macOS.

- **Examples:** `flashtest_ce163f.pc1500a` no longer stops with ERROR 1
  IN 10, and in `dampflok.bas` wagon W2 shows its own picture.

## [0.7.0] - 2026-10-04

### New

- **RENUM for the PC-1500**, as a worked ROM extension: the debugger's
  *PC-1500 ROM extension* template (Create Debug Project…) is now a keyword
  module at &8800 that adds `RENUM [new][,[old][,step]]` to PC-1500 BASIC,
  following the PC-1600's own RENUM. It replaces the `CALL &8000` stub.
  docs/Debugger.md walks through debugging it, and
  docs/PC1500-Keyword-Modules.md describes how the PC-1500 ROM finds such
  modules.
- **The display as text.**
  - **Edit ▸ Copy Screen** puts the screen on the clipboard as text as
    well as the image; a text editor pastes the text.
  - **`- expect: <text>` in presets** stops the preset with an error,
    showing the screen, unless one line of the display contains the text.
  - The headless CLIs write the display text with `--lcd-text <file|->`,
    and the debug server answers `calcu1600/screen`.

  The text is read with the calculator's own ROM font, so it is exact.
- **Create Debug Project Here…** in VS Code's Explorer context menu creates
  the project in the folder you right-clicked; its launch configuration
  points into that folder.
- **CE-151 (4 KB) and CE-161 (16 KB) memory modules.** The CE-151 fits the
  PC-1500/1500A and PC-1600 Slot 1, where it adds &B000-&BFFF as on a real
  PC-1600. The CE-161 fits the PC-1500/1500A and both PC-1600 slots.

### Changed

- **New app icon**, a simplified PC-1600. On macOS it now has the same
  size and shape as the other Dock icons.
- **The debugger's history shows the registers before each instruction**,
  like the live frame. Call Stack frames 1–20 read `C0EE  call KEYGET`
  (no "after"); what an instruction did shows in the next newer frame.
  `TRACE.bin` captures follow the same rule (format v3);
  `tools/read_trace.py` refuses older captures.
- **The preset log** shows the display as text after each step
  (`lcd=[...]`). What it used to call `screen="..."` was the ROM's input
  line, not the display; it is now `input="..."`.
- **Machine code with a bank in its PC-1600 header** loads into that bank
  (1-3) instead of being refused, and the proposed command is
  `CALL #bank,&addr`. Load Machine Code… shows an error when that bank has
  no RAM at the load address. Presets and the debugger follow the header's
  bank too.
- **Debugger settings are checked**, in a preset's `debug:` block (with the
  line number) and in a launch configuration alike:
  - Numbers follow the preset rule: `&`, `0x` or `$` is hex, a bare number
    decimal. `"address": "1234"` in a launch configuration is now decimal
    (it was hex), and hex without a prefix is refused. `bank: &7` and
    `"bank": "7"` now take effect; they used to be ignored.
  - Bad values (`after: foo`, `cpu: x86`, `boot: foo`, a quoted `"true"`)
    are refused instead of falling back to a default.
- **The PC-1600's serial port file is `calcu1600-rs232c.serial`** (was
  `calcu1600.serial`), named after its connector like
  `calcu1600-ce158.serial`. SharpDataExchange 0.3.2 looks for the new name
  with `--device pc1600emul`.
- **The LH5801/LH5803's undocumented "V register" opcodes** behave as on a
  real PC-1500A and PC-1600 (VH reads the high byte of the last 16-bit
  operand, VL reads 00, writes are ignored). The debugger disassembles them
  as `.db`, marked undocumented.

### Fixed

- **PC-1600: loading into a CE-155 put the program in the wrong place.**
  A BASIC program loaded into a CE-155 (Load BASIC Program, presets) came
  out garbled, and machine code was refused. All loaders now store the
  bytes where a CPU write would.
- **PC-1600: machine code that runs from a module on past &BFFF loads.**
  It continues into internal RAM, as `BLOAD "COM1:"` and `CLOAD M` write
  it. In MODE 1, PC-1500 machine code bigger than the module part was
  refused before.
- **PC-1600: DEGREE, RADIAN and GRAD change the status line at once**, not
  at the next scroll. As on a real PC-1600, the LH5803's writes to the
  PC-1500 display memory (&7600–&764F, and its aliases &7400–&754F) show
  on the LCD straight away, so `XPOKE &7600,…` and PC-1500 machine code
  drawing there now show up too.
- **The PC-1600 status line showed "RAD" in GRAD mode.** Both models now
  name the status symbols the same way (`I`, `II`, `III`, `KANA` in the
  text read-out too).
- **PC-1600 MODE 1: `XPEEK`/`XPOKE` on a CE-163F in Slot 2** read 255
  instead of the stored value.
- **PC-1600 clock: 29 February.** The real clock keeps no year, so both
  02/28 and 02/29 roll over into 03/01; the emulator followed a calendar
  year. A date set to 02/29 is still accepted.
- **PC-1600 faceplate:** some key labels had the wrong colour, and
  Shift+0 now shows a broken bar instead of two dots.
- **Debugging a project from VS Code** (a launch configuration with
  `project`) stopped at no breakpoint: the breakpoints were armed too late
  and `command` never ran, and the extension's (usually empty) ROM listing
  settings replaced the project's listings.
- **The VS Code extension failed to install on Node 18** (Debian 12,
  Ubuntu 24.04). It is now packaged without `vsce`.

### Internal

- `tools/fetch_roms.sh` fetches the PC-1500 ROMs from
  [tinue/PC-1500-ROM](https://github.com/tinue/PC-1500-ROM) (same files;
  `PC1500_ROM_BASE` overrides it). libsharpdx 0.3.2.
- Tests build with `-O1`; the suite runs in 66 s instead of 172 s.
- Loader matrix (dev/loader-matrix/): every loader checked against the ROM's
  own loading, on both machines.
- Docs and comments: primary sources only, measured connector pins
  (PU/PV), resolved handoffs removed.

## [0.6.0] - 2026-09-30

### New

- **Debugger** VS Code debugs machine code in the emulator: breakpoints
  (with conditions, hit counts, logpoints), data breakpoints, stepping in
  the `.asm` source or the disassembly, registers and memory, on the
  LH5801, the Z-80 and the LH5803, ROM code included. Build & Load
  rebuilds, reloads and restarts a program without detaching. Turn it on
  under **Settings ▸ Debugger** (or `--dap <port>`); the extension is in
  `vscode/calcu1600-debug/`. See docs/Debugger.md.
  - The VS Code extension works in any folder, without a `launch.json`:
    **Debug current file** on a PC-1600 (zasm) or PC-1500A (sdas), built-in
    build tasks, and settings for the port and assembler paths.
  - **Create Debug Project…** writes a starting point for a program or a
    ROM extension on either machine.
  - A preset's `debug:` block holds a project's debugger settings, and the
    launch configuration names it as `project`. `command` types a BASIC
    line to start the code.
  - **ROM extensions**: `bus-rom:` plugs ROM files into the expansion bus,
    Build & Load restarts with the rebuilt ROM, and `boot: debug` stops in
    the ROM's power-on code.
- **Drag and drop** Drop a preset, a BASIC program (`.bas` / `.bbin`) or
  machine code onto the window to load it; the content picks the loader,
  anything else is ignored. On macOS also onto the Dock icon, and presets
  open from Finder. See the User Guide, "Dropping files".
- **Keyboard** PC-1600: accented characters (`ä`, `é`, `ñ`, …) type
  through `KB II`, from host keys, Paste Text and a preset's `type:`, in
  the case typed. Dead keys compose (`¨` then `U` gives `Ü`).
- **Mount Directory** (File menu, PC-1600): a folder on the computer
  becomes drive `S3:` (also `Y:` without a CE-1600F). LOAD/SAVE, OPEN,
  FILES, KILL, NAME, COPY, SET and DSKF work on it. Only 8.3 names are
  shown, and INIT is refused. Subfolders work as on a MEP USB drive:
  `CDIR "path"` changes the current folder and `LDIR` lists subfolders.
  Programs written for the MEP module, such as FILEX, browse it. See
  docs/PC1600-Host-Drive.md and docs/PC1600-FILEX.md. The dialog opens in
  the directory last mounted, or in a fixed **Settings ▸ Host drive
  folder**.
- **Presets** New keys:
  - `host-drive: <folder>` mounts a folder as S3:.
  - `floppy-file: <file>` loads a `.floppy.yaml` next to the preset.
- **Presets** `saveas:` can write a file next to the preset
  (`saveas: template s2:file:Card.card.yaml`) and can save a template.
  Running the preset again replaces your own template of that name; a
  bundled name is refused.
- **`pc1600_cli --preset`** runs `saveas:` for cards too.
- **examples/dwx/** holds DiskWorks v3, self-contained: its card, floppy and
  S3: folder live next to the preset, and `make_diskworks_media.pc1600`
  replaces the two `setup/make_diskworks_*` presets.
- **Load BASIC Program** and a preset's `format: basic-binary` also take a
  tokenized program (`.bbin`, CE-158 or PC-1600 header). A PC-1500 one
  loads on the PC-1600 in MODE 1 only.
- **Debug panel** On the PC-1600, Pointers shows the program area `TITLE`
  selects, `MEM`, and each slot's program module with its free space.
- **Memory cards** A card definition's `encoding: file` block reads its
  binary next to the definition at every clean start, so a ROM module in a
  memory slot can be rebuilt.
- **Examples** Grouped by topic (`plotter/`, `basic/`, `interfaces/`,
  `machine-code/`, `memory/`, `dwx/`), with an index in
  `examples/README.md`.

### Changed

- **Presets** The format follows one naming rule; old presets need these
  changes (the old forms are refused):
  - `memory-expansion:` / `-1:` / `-2:` with `- modulespec:` becomes
    `slot-1: CE-1600M`, and `- modulespec-file:` becomes `slot-1-file:`
    (`slot-2` likewise). `saveas:` targets are `slot-1`, `slot-2`, `floppy`.
  - Product names are Sharp's, with the hyphen: `plotter: CE-1600P`,
    `CE-150`, `interface: CE-158`, in any case. `none` / `off` are gone;
    leave the key out.
  - `program:` takes `file:` (was `path:`) and has no `format:`: the file's
    content picks the loader. `text: |` or `typed: true` types a listing
    in.
  - Numbers: `&`, `0x` or `$` is hex, a bare number decimal, also for
    `address:` (it used to be hex without a prefix). `bus-rom:` takes
    `me: 0|1` (was `me1:`).
  - `type:` types quotes too; `wait:` takes a plain number (`1s` is
    refused); `~/` works in every path.
  - The bundled presets, examples and VS Code presets are converted; four
    PC-1500A presets saved as `.pc1500` are now `.pc1500a`. See the User
    Guide, chapter 8.
- **Loaders** On the PC-1600, Load BASIC Program, Load Machine Code and
  presets follow the calculator's MODE and `TITLE`, as `LOAD` does: in
  MODE 1 a listing is PC-1500 BASIC and a headerless file LH5801 code at
  an LH5803 address, and a program goes into the selected program area
  (S0, or an S1/S2 program module). The Load Machine Code dialog says
  which CPU and address space it assumes, and the advice after loading
  follows (`NEW &addr`, `XCALL`). An address in the work area
  (&F000-&FFFF) loads with a warning.
- **Presets** `program:` has no `slot:` any more; MODE and `TITLE` decide.
- **Debugger** The default port is 32168 (was 4711 in 0.6.0-pre). A port
  already saved in Settings is kept.
- **CE-1600P ROM** Switching it with the plotter attached swaps the box
  with an OFF/ON cycle instead of an ALL RESET: RAM and the inserted disk
  stay.
- **PC-1600 power** OFF really switches the machine off. ON, the WAKE$
  timer and an incoming call (CI) switch it on through the sub-CPU, as on
  the real unit; after an auto power-off, ON resumes where it stopped.
- **Presets** `saveas:` now needs `live` or `template` first:
  `saveas: live s2:My Card`.
- **Presets** `- modulespecfile:` is now `- modulespec-file:`, like
  `floppy-file:`.

- **Loaders** What a file is now comes from SharpDataExchange's library
  (0.3.1): Load Machine Code refuses a BASIC listing or tokenized BASIC
  (instead of loading it as raw bytes), skips `00` bytes before a header,
  and no longer checks the PC-1600 header's end marker. Building now
  requires the library on every platform.

- **Settings** Folder and serial-port paths show as `~/Calc-U-1600/…`
  (and `~/…` elsewhere under the home folder), as the User Guide writes
  them.
- **Documentation** Split by reader: the README and `docs/` are for
  users, `docs/developer/` for building and working on the emulator. Each
  level starts at its own README.
- **Plotter paper** Copy / Cut stay at 1200 DPI up to about 136 mm of
  CE-1600P paper and scale down beyond that (was ~339 mm), so a long plot
  no longer needs gigabytes of memory to copy.

### Fixed

- **Examples** `lissajou-ce150.pc1600` plots again (its listing used the
  PC-1600 keyword `LLINE` in MODE 1). The text-and-frame and CE-150
  presets wait until the plot is done instead of a fixed time. The
  chapter-5 screenshot preset parses again.
- **Presets** On the PC-1500, a `program:` line that replaces a line of
  the same length (`10 A=1`, then `10 A=2`) is no longer reported as
  rejected.
- **Machine code** A header auto-run address of `&FFFF` (what the
  calculators write for "none") no longer proposes `CALL &FFFF`.
- **Presets** A machine-code header's auto-run into slot S2 now types
  `CALL #2,&<addr>`, as Load Machine Code proposes, instead of
  `CALL &<addr>`, which ran whatever bank 0 maps there.
- **Loaders** On the PC-1600, a BASIC program made of several program
  segments (a `#SEGMENT` or `99999` line in the listing) loads instead of
  failing with "the tokenized program is malformed".
- **Loaders** A fast-loaded BASIC program that crosses a memory module's
  16 KB bank boundary no longer splits a line across the two banks; it is
  laid out as the calculator's own `LOAD` does.
- **PC-1600 serial port** The TC8576F follows its data sheet: a CTS check
  (`RCVSTAT`/`SNDSTAT` 24) no longer blocks sending, the baud rate comes
  from the chip's registers, and the peer's RI reaches the calculator as
  CI.
- **Debug panel** Dump Card YAML also dumps an unbanked card (CE-1600M).
- **CLIs** `pc1500_cli` and `pc1600_cli` find the bundled memory cards
  (`Qt6/resources/cards`) without `--modules-dir`.
- **macOS** After the app had been killed, the "reopen windows?" prompt
  could hang the next start.

## [0.5.0] - 2026-09-25

### New

- **CE-158** RS-232C / Centronics interface, on the PC-1500/1500A and on
  the PC-1600 (MODE 1). Not yet together with the CE-1600P.
- **CE-1600P** New or old ROM, selectable.
- **Presets** `- saveas:` saves a memory card or floppy from the script.
- **Cards & floppies** `template: true` marks a template; the app never
  writes to one.
- **Keyboard** Scroll Lock / Insert -> `RSV`, more Shift variants, and a
  lone Shift tap latches the calculator's `SHIFT`.
- **Plotter paper** Long plots scroll; Copy / Cut export at 1200 DPI,
  without anti-aliasing, at physical size (scaled down past ~339 mm).
- **Docs** User Guide rewritten with screenshots; complete keyboard map;
  `.floppy.yaml` disk format spec.
- **Examples** `dampflok.bas`, a steam-locomotive sound demo.
- **Build** One root CMake project for app, CLIs and tests (debuggable
  in CLion, see `docs/developer/Building.md`); scripted screenshots (`--shots`,
  `docs/developer/screenshots/README.md`); `pc1600_cli --save-dir` for
  `saveas: floppy:`.

### Changed

- **Presets** The ROM is part of the model and plotter
  (`model: PC-1600:old`); `firmware:` is gone.
- **Paste** Paste Text types only the first line and doesn't press ENTER.
  After Load Machine Code, its `CALL` is typed in, ready for ENTER.
- **Presets** On the PC-1500, `format: basic-text` no longer presses `CL`
  and types `NEW0` itself -- add those steps to the preset, as on the
  PC-1600.
- **Control bar** Narrower ROM and floppy widgets.
- **Requirements** macOS 15.8 or later; SharpDataExchange 0.3.0.

### Fixed

- **Timing and sound** The PC-1600 now matches a real unit much more
  closely: BASIC speed, BEEP pitch and repeat rhythm, display and sub-CPU
  response times. The buzzer follows the real signal on both machines,
  shaped by a model of each one's piezo. BASIC on the PC-1600 still runs
  about 0.65% fast (see TODO.md).
- **PC-1600 interrupts and power** Interrupt handling follows the ROM and
  the Z-80 rules. A timer interrupt no longer runs in the middle of
  switching off, and an ON press during the handover between the two CPUs
  is no longer lost.
- **CPU cores** Chained Z-80 index prefixes, the R register, LH5801
  interrupt entry, and LH5803 I/O accesses to &8000-&BFFF.
- **PC-1600 serial port** An error reset clears the error flags; closing
  the serial peer no longer hangs a transmission.
- **BASIC and machine-code loading**
  - PC-1600: after a fast (`basic-binary`) load, LIST in PRO mode showed
    nothing until a MODE switch.
  - PC-1600: re-typing a line with the same length (`10 A=1` then
    `10 A=2`) failed the preset.
  - PC-1500: a `format: binary` block past &FFFF or into ROM was
    reported as loaded. Addresses like `41zz` are now errors.
  - A comment right after a block key (`memory-expansion-1:   # slot 1`)
    was a parse error.
  - `saveas:` failed on a slot loaded from a saved card, and on a second
    save of the same slot.
- **PC-1500 clock** `TIME=` followed at once by reading the time kept the
  old time.
- **App**
  - Crash on quit with TRACE on.
  - TRACE lost most instructions, and kept running empty after a machine
    rebuild.
  - A preset asking for a missing old PC-1600 ROM quit the app.
  - A failed plotter/interface attach (e.g. missing ROM) gave no message.
  - Re-picking the already-checked model/ROM menu item rebuilt the
    machine.
  - A saved card was rewritten every 500 ms while loaded.
  - Reset typed still-queued keys into the rebooting machine.

### Details

- **CE-158** Control-bar toggle, a printer pane for the Centronics
  output, and its own serial port (`calcu1600-ce158.serial`, see
  Settings). Presets attach it with `interface: ce158`.
- **CE-1600P ROM** Independent of the PC-1600 ROM; the CE-1600F follows
  it. Run `tools/fetch_roms.sh` again to get the old dump.
- **Preset ROM syntax** `model: PC-1500:A01`, `model: PC-1600:old`,
  `plotter: ce1600p:old`. A preset that still has `firmware:` fails with
  a hint. `model: PC-1500A` accepts only A04.
- **`saveas:`** `s1:<name>`, `s2:<name>` or `floppy:<name>` (the last
  two PC-1600 only); overwrites a same-named file. The DiskWorks setup
  presets use it.
- **Templates** No longer tied to shipping with the app: a template in
  the save folder (e.g. `examples/memory-cards/pc1500-maxed-out.card.yaml`)
  is listed with the bundled ones and offers Name & Save. Files without
  the key are instances, autosaved in place; existing ones need no change.
- **Keyboard** Shift+Delete -> Shift+`CL`, Shift+Home -> `CTRL`,
  Shift+End -> `KB II` (PC-1600), Shift+Page Up -> `DEF`. A Shift tap
  counts if released within 0.4 s with nothing in between. Full table in
  `docs/Keyboard-Mapping.md`.
- **Floppy format** SharpDataExchange (`sde dir/get/put/del`) reads and
  writes it. Eject a disk before editing its file with such a tool: the
  emulator rewrites the file after every disk access.

## [0.4.0] - 2026-09-20

- **Keyboard** Fixed a key getting stuck down and freezing the
  calculator (Reset didn't help). This happened when a host key that
  needs Shift (e.g. Shift-3 for `*` on a Swiss keyboard) was released
  after Shift. A key held while the window loses focus is now let go too,
  and Reset / Reset All release every key.
- **Keyboard** PC-1600: `_` is now typed as SHIFT + `.`, as on the real
  keyboard -- it used to come out as `9`.
- **ROM modules** Memory modules can now be ROM (`content: rom` in a
  card definition: read-only, bytes carried in the file). First one:
  Sharp's **CE-502B** Statistics module for the PC-1500/1500A. ROM
  modules have their own section at the bottom of the slot picker.
- **Presets** The built-in `- module: <name>` form (`ce155`, `ram16`,
  `ram32`, `ce1638plus`, `ce163f`) is gone -- name a module by its
  definition instead, e.g. `- modulespec: CE-155`. Every memory module
  now comes from a `.card.yaml` definition.
- **RAM** Powers up as zeros instead of &FF -- internal RAM and fresh
  memory modules, matching how CMOS RAM comes back after a power loss.
  Flash memory still starts erased (&FF). The exception is the PC-1500's
  &7600-&7BFF (display RAM and system RAM), which reads &FF after a power
  loss on the real machine and now does here too -- including the lock
  register at &79FF. On the PC-1500/1500A, Reset now
  keeps RAM (the BASIC program survives, like the real reset button) and
  Reset All clears it.
- **Settings** moved from the control bar to the menu bar: the
  application menu on macOS (Cmd-,), Edit > Settings… (Ctrl-,) elsewhere.
  The control bar's Reset button is gone too: Machine > Reset (Cmd-R)
  and Reset All (Cmd-Shift-R).
- **Keyboard** Keys held with Cmd/Ctrl (macOS) or Ctrl/Alt/Windows key
  (Windows/Linux) no longer type on the calculator -- e.g. Cmd-C no
  longer puts a "C" on the display. Option and AltGr still type.
- **Copy** With text selected in the debug panel's output, Edit > Copy
  (Cmd-C) copies that text instead of the screen.
- **Reset** Every reset runs the boot at full speed until the prompt
  appears, including a plotter's power-on init (the CE-1600P pen
  calibration), then sets the clock from the computer's time: Reset and
  Reset All, and also the reset that follows choosing a memory module, a
  ROM or a model. On the PC-1600 with a plotter the boot now really runs
  until the cursor appears; it used to stop early and finish in real time.
- **Paste** Edit > Paste Text (Cmd-V) types the clipboard's text into
  the machine, paced so no character is lost. Nothing is added or
  checked: a line break presses ENTER and waits for the line to finish,
  a trailing line break is dropped, and characters without a key are
  skipped. Pressing a machine key stops a paste in progress.
- **Copy** Edit > Copy Screen (Cmd-C) puts a PNG of the display on the
  clipboard: dot matrix only (no status indicators), black on white, at
  the real display's size (PC-1600 89 x 18 mm, PC-1500 104 x 5 mm).
- **Presets** New `- screenshot: <file.png>` step writes the same image
  into the trace directory. `trace:` and `screenshot:` now honour the
  trace directory set in Settings.
- **Presets** New `- syncclock:` step sets the calculator's clock to the
  host's current date and time. A preset load runs at full speed, which
  leaves the clock ahead; put `- syncclock:` last to correct it.
- **Presets** PC-1600 presets with a plotter (CE-1600P or CE-150) load
  much faster. The loader waited the full 15 s safety timeout before
  every `key:` and `type:` step (over 3 minutes for a short preset); it
  now continues as soon as the machine is ready for keys.
- **Settings** Separate start folders for the open dialogs: Samples
  (Load Preset), Basic (Load BASIC Program) and Assembly (Load Machine
  Code). Each defaults to `<last used>` -- the dialog
  opens in the folder a file was last loaded from. Choosing a folder
  fixes it; Reset returns to `<last used>`.
- **Machine code** File > Load Machine Code… loads a `.bin` into the
  running machine: with a CE-158 (PC-1500) or PC-1600 header, or raw.
  It asks only for the start address of a raw file. On the PC-1600 the
  code always goes into BASIC's program area (the one `NEW "S0:"`
  reserves in), and a raw file's address defaults to its start: &C0C5,
  or &80C5 when a RAM module is folded in as extension memory. Code
  meant for a program module (`INIT"Sx:","P"`, `NEW"Sx:",n`) needs a
  preset. Afterwards it shows the `NEW` that keeps BASIC from
  overwriting the code and the `CALL` that starts it. The code is never run automatically. A
  CE-158 file on a PC-1600 (LH5803 side) is not supported yet.
- **PC-1600** The calculator ROM version is selectable: the new ROM
  (default) or the old one (Machine > ROM Version, or `firmware: new|old`
  in a preset). If the old ROM can't be loaded, the app warns and uses
  the new one.
- **Plotters** Attaching or detaching a plotter now runs its OFF/ON
  power cycle at full speed (like Reset), including the power-on
  rotation of the pen header, and then sets the clock from the
  computer's time.
- **Control bar** Choosing a memory module or a ROM version no longer
  detaches the plotter or changes any other selector; the rebuild that
  applies it keeps the CE-150/CE-1600P you had (its paper starts blank).
  Switching the model starts from a default machine instead: PC-1500
  ROM A04 / PC-1600 new ROM, nothing attached, then the startup preset.
- **Debug** The pointer dump shows the PC-1500 lock register (&79FF) as
  locked or unlocked. The MODE key is ignored while it isn't &FF; `NEW0`
  unlocks it, so the startup presets now begin with `NEW0`.
- **Examples** Added CalCula, an RPN calculator program.
- **Build** `tools/fetch_roms.sh` fetches the PC-1600 new and old ROM
  sets and the CE-1600P ROMs from the updated PC-1600-ROM dumps.

## [0.3.0] - 2026-09-19

- **PC-1600F** Added the CE-1600F pocket floppy drive for the
  PC-1600. It attaches with the CE-1600P plotter/printer (no separate
  switch) and works like the real one: `INIT"X:"` formats a blank disk,
  and `SAVE`, `LOAD`, `FILES` and `DSKF` work on it. Disks are
  double-sided (64 KB per side); a control-bar button flips the disk to
  the other side, a green lamp shows when the drive motor runs, and
  drive timing follows the Service Manual.
- **Floppy disk** Two templates are available: A formatted floppy,
  and a blank one. Select one in the drop down to insert into the
  drive. Any changes you make on these templates are in-memory only.
  If you want to persist, click the "save" button and give the floppy
  a name.
- **Presets** Settings has a default preset per model (PC-1500,
  PC-1500A, PC-1600), applied at startup and whenever that model is
  selected. Ready-made ones are in `examples/startup/`; the PC-1600 one
  adds a CE-1600M and CE-1601M, mounts a floppy, sets up the RAM
  disk and COM1.
- **Loading** Long preset loads show a "Loading…" popup and the window
  keeps updating, instead of freezing.
- **Settings** The Settings dialog is reorganized into titled sections
  (General, Default presets, Storage, Tracing, Serial port).
- **Control bar** Common controls (Reset, Settings, model/ROM, memory
  slots) stay left-aligned, so switching models no longer shifts them.
  The redundant "Load Preset…" button is gone; use the File menu.
- **Build** Windows ARM64 is now built natively and gains BASIC preset
  loading; BASIC tokenizer updated to SharpDataExchange 0.2.1.

## [0.2.0] - 2026-09-18

- **Platforms** Added a native Windows ARM64 build, alongside the
  existing Windows x86_64, Linux, and macOS builds.
- **Keyboard** PC-1500/1500A live typing is now buffered, so typing
  quickly no longer drops keystrokes. Cursor keys (Left/Right/Up/Down)
  still pass straight through, so the ROM's own auto-repeat still works
  when held.
- **Startup** A missing or corrupted bundled ROM now shows a dialog
  naming the file and where it looked, instead of silently crashing.
- **Loading** Support loading segmented PC-1600 BASIC programs.
- **Loading** File ▸ Load Preset… no longer asks for confirmation before
  resetting the machine — picking a preset loads and resets immediately.
- **Loading** File ▸ Load BASIC Program… now behaves like a real LOAD:
  it no longer resets the machine, switches RUN/PRO mode, or types NEW0.
  It loads into whatever BASIC program area is currently live, so you can
  prepare the machine yourself first (fit a memory card, run `NEW`,
  attach a plotter) exactly as you would on real hardware.
- **Display** Press and hold anywhere on the LCD to fast-forward the
  emulation at full, unthrottled speed; release to return to normal
  speed.
- **Windows installer** Fixed a "missing MSVCP140.dll" failure on a
  clean machine by running the bundled Visual C++ redistributable
  during install.

## [0.1.0] - 2026-09-15
First public release. Calc-U-1600 is an emulator for the Sharp PC-1500,
PC-1500A, and PC-1600 pocket computers, built as a Qt6 desktop app for
macOS, Linux, and Windows.

- **Machines** All three models boot to BASIC, with keyboard, dot-matrix
  display, real-time clock, expansion-card, and serial-port support.
- **Plotters** CE-150 (PC-1500/1500A/1600) and CE-1600P (PC-1600) pen
  plotters, rendered on a virtual paper roll/sheet you can copy or export
  as an image.
- **Memory modules** Battery-backed and anonymous expansion-card modules
  per model, selectable and nameable from the control bar; state persists
  across sessions for named modules.
- **Presets** `.pc1500`/`.pc1500a`/`.pc1600` preset files describe a full
  machine configuration (model, ROM revision, modules, plotter, a resident
  program) and load it in one step via File ▸ Load Preset….
- **BASIC programs** File ▸ Load BASIC Program… tokenizes and loads a
  plain `.bas` listing directly into the running machine, without needing
  a preset wrapper.
- **Serial port** The PC-1600's RS-232C port is always live, backed by a
  host pseudo-terminal any serial application can connect to.
- **Native menu bar** File, Machine, and Help menus alongside the existing
  control bar, including an About dialog with version/build information.
