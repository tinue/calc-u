# Scripted screenshots

The pictures of the help ([help/](../../../help/), its
pictures in `help/images/`) are generated, not hand-made. There is one
scenario per help chapter that has pictures, in [guide/](guide/), named
after the chapter (`02-getting-started`, `04-media`, `05-peripherals`,
`06-presets`, `07-debugger`, `08-settings`, `09-product-topics`,
`01-introduction`), each with `out: ../../../../help/images`. A
**scenario** (`*.shots.yaml`) walks the real
app through a list of shots: each starts from a **preset**, which sets up
the emulator (model, cards, plotter, typed input). **Steps** then drive the
Qt UI, for example opening a drop-down, triggering a menu item or holding a
faceplate key, and a **capture** writes the PNG. The pictures are taken
again on purpose only (the Checkpoint entry `CalcU1600HelpPictures`, or by
hand); the Longer entries `CalcU1600Shots.<scenario>` run the same
scenarios as checks into the build tree.

```sh
tools/make_screenshots.sh                    # every scenario in guide/
tools/make_screenshots.sh 04-media           # just one (name without .shots.yaml)

# or directly:
Calc-U-1600.app/Contents/MacOS/Calc-U-1600 --shots docs/developer/screenshots/guide/04-media.shots.yaml \
    [--shots-out <dir>] [--shots-only name1,name2] [--shots-fail-fast]
```

The app shows its window, plays the scenario, prints one `[shots]` line per
shot or image to stderr, and quits. The exit code is:

- `0`: every image was written.
- `1`: at least one shot failed. Its line number and reason are printed,
  and the run carries on with the next shot unless `--shots-fail-fast` is
  given.
- `2`: the scenario itself doesn't parse.

That makes the same scenarios usable as a GUI smoke test.

`--shots` (and `--pacing-probe`) exist in development and test builds only
(`CALCU_ATTACH`: a POSIX tree with the tests, as `tools/run_qt.sh 1600`
at the repository root makes it); release builds and Windows have neither.

**A client of harness attach.** The app opens harness
attach on a port the system picks and the runner talks to it as a line
client (`library/qt6/AttachLineClient`, the harness's serve protocol): every
step that loads a preset, lets time pass, presses keys or types is the
harness's `preset`, `wait`, `key`, `type`, `reset`, `cold-boot` (and
`expect`, `pause`, `resume`), so the runner never steps the machine
itself. UI steps find the shell's widgets by their
object names (`library/shell/ObjectNames.hpp`).

## What a run guarantees

- **Captures after the machine is idle.** The app runs on its own clock
  between steps, as it does for a user (a preset at full speed, `run:` in
  wall time). Put `wait-idle` before a capture that must show the end of
  what the steps started; a cursor that blinks or a clock that runs may
  differ from run to run, so the pictures are not byte-identical.
- **Isolated settings.** A run uses a throwaway settings file and an empty
  card/disk folder. Your startup presets, last model, folders and saved
  cards never show up in the images, and the run never changes them. The
  Settings dialog shows the default folder (`~/Calc-U-1600`), as a fresh
  install does.
- **Qt's own file dialogs.** A run uses these instead of the native ones,
  so a script can capture them and close them again.
- **Fixed appearance and geometry.** A run uses light mode (see
  `appearance:`), the scenario's window size, and a fixed position on the
  primary screen.

## Scenario format

```yaml
out: ../../../../help/images   # output folder, relative to this file (default: this folder)
window: 900x1000        # main-window size (default: leave as is)
appearance: light       # light (default) | dark | system
scale: 2                # device px per logical px for Qt captures (default 2)
settle: 150             # ms to let the UI settle after each UI step (default 150)

shots:
  - name: model-dropdown                       # unique; used by --shots-only
    window: 900x1000                           # optional: this shot's size (default: the scenario's)
    preset: ../presets/pc1500a-ready.pc1500a   # optional: load it first
    steps:                                     # optional
      - open: bar-model
    capture: { target: control-bar, file: model-dropdown.png, padding: 4 }
```

A shot without `preset:` continues from where the previous one left off.

The window is set to the shot's size when the shot starts, and again after
each `preset:` or `reset`, since a model switch changes the minimum size.
Two limits apply. A window can't be narrower than its layout: the PC-1600's
control bar needs about 1430, or 1620 with the Tape group showing. It also
can't be taller than the screen. If the real size differs from the
requested one, the capture logs a `note: window is …`. Give the shot a `window:` that fits: the plotter shot
uses `1430x1280`, so the paper panel shows the whole figure.
`capture:` on the shot is a final capture step. `capture: name.png` alone
captures the whole window. More captures can go in `steps:` as
`- capture: {...}`.

### Steps

| Step | What it does |
|---|---|
| `preset: <file>` | Load a preset: attach's `preset` with a file path, or with a bundled example's path `examples:<path>`; the next step follows when it is loaded. |
| `reset` / `cold-boot` | Machine ▸ Reset / Cold boot: attach's `reset`, `cold-boot`. |
| `key: <name>` | Tap a calculator key: attach's `key`. Key names are the preset `key:` names. |
| `hold-key: <name>` | Press a key and keep it down: a pointer down on the faceplate at the key's centre (the shell's live input), then attach's `wait 100ms`. The faceplate shows the press highlight. |
| `release` | Let go of the held key. |
| `type: <text>` | Type one line and ENTER: attach's `type`. |
| `run: <seconds>` | Let that much time pass: attach's `wait <s>` (the app runs at regular speed, so as much wall time). |
| `wait-idle` | Until the machine is idle: attach's bare `wait` (at most an hour). Use it before a capture. |
| `settle: <ms>` | Wait wall-clock time for the UI; the app runs on. |
| `click: <objectName>` | Click a button. |
| `open: <objectName>` | Open a combo box's drop-down, or click a button. |
| `select: <objectName> = <item>` | Pick a combo-box item by its text. |
| `action: Menu > … > Item` | Trigger a menu item. `&` mnemonics are ignored, and `...` matches `…`. |
| `menu: Menu > Submenu` | Open a menu and leave it open. See the macOS note below. |
| `close` | Close the topmost popup, dialog, or native menu. |
| `choose-file: <path>` | Pick that file (relative to the scenario) in the open file dialog and accept it, e.g. after `action: File > Load Machine Code…`. In a Save dialog (`File > Tape > Record…`) the file may not exist yet; its folder must. |
| `enter-text: <text>` | Type into the text field of the open dialog, e.g. a name in Name & Save. |
| `accept` | Accept the open dialog (its OK button), e.g. after `enter-text`. |
| `capture: …` | Write an image; see below. |
| `expect-panel: { name, title, empty }` | Check a paper panel (by objectName): shown, with that title (the plotter's name; `""` none), empty or not; the shot fails otherwise. For the check scenarios in `tests/gui/`. |
| `expect-title: <text>` | Check the window's title. |
| `expect-widget: { name, text, enabled }` | Check a label or button (by objectName): its text, and whether it can be used; the shot fails otherwise. |
| `dap-client: <port>\|disconnect` | A DAP client of this process connects to 127.0.0.1:`<port>` (the app started with `--dap <port>`), or goes. |
| `command: <name>` | A harness command through attach (`pause`, `resume`). |
| `host-key-down: <key>` / `host-key-up: <key>` | A key of the host's keyboard goes down / up, as the window gets it from the keyboard: `shift` or one character (sent with Shift while `shift` is down). No settle follows, so a down and an up in a row are a tap. For the Shift-tap check scenario. |
| `expect-lcd: <text>` / `expect-no-lcd: <text>` | Attach's `expect` / `not-expect`: the text is in one row of the LCD, or in none; the shot fails otherwise. |

A dialog opened by `action:` or `click:` stays open and scriptable: the
script carries on inside it, captures it, then `close`s it. Anything still
open when a shot ends is closed automatically, and a held key is released.

### Captures

`capture: { file, target, method, padding, scale }`

- `file` (required): a `.png` path relative to the output folder.
- `target`: one of these (default `window`):
  - `window`: the main window's contents.
  - `dialog`: the open dialog.
  - `plot`: the plugged-in plotter's whole paper at physical size (up to
    1200 DPI), as Copy puts it on the clipboard: the session's
    `paper.picture`.
  - `lcd-image`: the LCD dot matrix at physical size, as Edit ▸ Copy Screen
    puts it on the clipboard. (`lcd` is the on-screen LCD widget.)
  - `screen-region`: `method: system` only; see below.
  - any **objectName** (listed below).
- `method`:
  - `qt` (default): the widget is rendered through Qt at `scale`, together
    with any open drop-down or menu, placed where it is on screen. This
    needs no permissions and doesn't depend on what else is on screen.
    There is no title bar, and native menu-bar menus can't be seen.
  - `system` (macOS): uses `screencapture`, so what's actually on screen,
    title bar and native menus included, at the screen's own resolution:
    - `target: window` or `dialog`: that window with its title bar, no
      shadow.
    - `target: screen-region`: while a `menu:` is open, the open menus plus
      the menu-bar strip above them (so the menu title shows); otherwise
      every window the app owns.
    - any other target: its on-screen rectangle.
- `padding`: logical px of margin. It's transparent with `qt`.
- `scale`: overrides the scenario's `scale` for this capture.

### Native menus (macOS)

With the macOS menu bar, `menu:` opens the real menu through System Events,
the way a user's click would. It can only be captured with
`method: system, target: screen-region`. The terminal, or whatever
launches the run, then needs these permissions under **System Settings ▸
Privacy & Security**:

- **Screen Recording**, for `screencapture`.
- **Accessibility** and **Automation ▸ System Events**, to open the menu.

A missing permission fails the shot with that message, never a silently
wrong picture. The guide uses `system` for only two shots: the title-bar
window in `01-introduction` and the Machine menu in `02-getting-started`.
Everything else is Qt-rendered and byte-identical from run to run.

Two macOS details:
- The app adds items of its own to any menu titled "Edit" (writing tools,
  dictation and so on, in the system language), so the guide doesn't show
  the Edit menu.
- The menu script brings the app to the front itself, because the menu bar
  belongs to the frontmost app.

### Widget names

| objectName | Widget |
|---|---|
| `faceplate`, `lcd` | calculator face / its LCD |
| `menu-bar`, `menu-file`, `menu-edit`, `menu-machine`, `menu-help`, `item-<id>` | the shell's menus and items (`item-load-preset`, `item-settings`, `item-reset`, `item-cold-boot`, `item-resume`, `item-help`, `item-about`, `item-load-basic-program`, `item-load-machine-code`, `item-mount-directory`, `item-unmount-directory`, …) |
| `control-bar` | the whole control bar (the shell's) |
| `bar-model` | model picker |
| `bar-slot-slot-1`, `bar-slot-slot-2` | a module slot's control; in it `media-picker-slot-1`, `media-save-slot-1` (Name & Save), and the same for `slot-2` |
| `bar-place-printer`, `bar-place-interface` | the button (one peripheral fits: CE-150, CE-158 on a PC-1500/1500A; CE-158 on a PC-1600) or picker (several: the PC-1600's printer, None, CE-150, CE-1600P) of a place |
| `bar-disk`; `media-picker-floppy`, `media-side-floppy`, `media-save-floppy`, `media-lamp-floppy` | the CE-1600F's controls (every PC-1600; usable with the CE-1600P) |
| `bar-media-cassette-recorder`; `media-picker-cassette`, `media-save-cassette`, `media-counter-tape` | the cassette's controls (with a CE-150 on a PC-1500/1500A, or a CE-1600P) |
| `debugpanel`, `debugpanel.pointers`, `debugpanel.memory`, `debugpanel.dump`, `debugpanel.module-yaml`, `debugpanel.clear`, `debugpanel.trace` | Inspector panel and its buttons (`open:` on `debugpanel.memory` / `debugpanel.dump` shows the menu) |
| `panel-area`, `paper-panel`, `paper-copy`, `paper-cut` | the shell's panel area, the paper panel (the library's `PaperPanel`, always there) and its buttons |
| `ce158printer`, `ce158printer.save`, `ce158printer.clear` | CE-158 printer panel |
| `settings-dialog`, `setting-<section>-<key>`, `settings-close`, `settings-serial` | the shell's Settings dialog, a row, Close, the Serial ports section |
| `about-dialog`, `notice`, `notice-ok`, `loading`, `roms-missing` | the shell's About, a notice, "Loading…", the ROMs-missing text |

## Check scenarios

`tests/gui/*.shots.yaml` are scenarios that check what the GUI does: the
paper panel and its plotter name (`paper-panel-kept-through-detach`,
`plotter-example-paper-panel`), "Paused (debugger)" in the title and
Machine ▸ Resume (`paused-title-and-resume`), a tap of the host's Shift
(`shift-tap-types-second-legend`), a bundled example loaded by its bundle
path (`example-by-bundle-path`), a saved module template and its picker
(`picker-lists-instance`, `module-template-saved`), and the DAP status
line and Disconnect in Settings (`debugger-settings-disconnect`). They run
as the CTests `CalcU1600Shots.<name>` (Longer) with the guide's scenarios,
through attach like the guide's, write their pictures into the build tree,
and use `expect-panel`, `expect-title` and `command`.

## Presets

[presets/](presets/) holds the presets written for screenshots, so the
pictures don't change when an example preset does.

**Leave the calculator in RUN mode.** PRO mode is only right when the shot
shows a LIST or program entry. A capture taken in PRO mode logs a note. Start
from a clean `NEW0`, or the BASIC pointers (and so the INSPECTOR's Pointers, MEM
and so on) are garbage:

- PC-1500/1500A cold boot lands in PRO mode, so use `key: cl`, `type: NEW0`,
  then `key: mode`.
- The PC-1600 boots in RUN mode, and `NEW0` is only accepted in PRO mode,
  so use `key: mode`, `type: NEW0`, then `key: mode`. They are ordinary
presets of format 2 (see [Presets](../../../help/06-presets.md)) and may point at
files elsewhere in the repo, e.g. `../../../../examples/plotter/lissajou-1600.bas`.
