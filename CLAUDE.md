# Calc-U

One repository for the Calc-U products: Calc-U-59 (TI-58/58C/59) and
Calc-U-1600 (PC-1500/1500A/1600), later possibly Calc-U-1403
(PC-1401/1402/1403H/1421, SC61860), which so far holds only its faceplates
(`products/calc-u-1403/faceplate/`). Each product is released separately, with its own version and tags; all of them
build on one shared library.

Until Calc-U-59 3.0.0 and Calc-U-1600 0.9.0 are out, `docs/phase-f.md`
applies on top of these rules. The help site is
tracked in `docs/help-site.md`.

## Layout

```
CMakeLists.txt          one root CMake project for every product
library/<area>/         the shared library, calcu_library
products/<product>/     everything of one product: release.yaml, CHANGELOG.md,
                        core/ app/ frontends/ tests/ examples/ roms/ help/ docs/
                        faceplate/ (generators of the faceplate art and key tables)
docs/                   formats/, GLOSSARY.md, Process.md, test-groups.md
tools/  ci/  .github/workflows/
```

- `calcu_library` is one static C++20 library with one folder per area
  (`machine`, `sessions`, `presets`, `media`, …). Headers and sources sit
  together, file names in PascalCase, namespace `calcu::<area>`, includes
  rooted at `library/` (`"machine/Machine.hpp"`). Other targets: the
  `calcu` harness, `calcu_web_glue`, `calcu_qt6` (Qt builds only) and the
  attach endpoint (development builds only).
- Dependencies point one way: an area uses the standard library and areas
  of the levels below its own, never one of its own level or above. A new
  include against this order is a bug; tests and products may use any area.
  1. `files`, `formats`, `machine`: the standard library only
  2. `settings` (on `files`); `pacing`, `roms` (on `machine`)
  3. `sessions`
  4. `typing`, `peripherals`, `shell` (its `SessionThread` uses `parts`
     and `harness` only for attach, in `CALCU_ATTACH` builds), `testing`
     (the tests' runner and rigs, used only by tests)
  5. `startup`, `control`, `web` (only `sessions`, `settings` and `shell`)
  6. `presets`
  7. `paper`, `debugger`, `media`: each adds its steps to the presets'
     vocabulary
  8. `opening` (calls the presets and media parts)
  9. `parts`, the composition root that builds and wires the parts
  10. `harness`
  11. `qt6` (`harness` only for attach, in `CALCU_ATTACH` builds)
- Library code throws no exception across an interface and uses no RTTI,
  no threads and no locks; failures are return values. Sessions are
  single-threaded. The one exception are the host adapters at a
  session's edge, which may hold threads and locks and say so in their
  header: `shell/SessionThread`, `harness/attach/`, the pseudo-terminal
  link of `peripherals`, and `qt6`.
- A product's code that implements a library interface stays in the
  product (`core/`, `app/`), in the product's style.

## The products must not drift apart

- Anything that is not truly specific to one machine belongs in the
  library. Only what is truly specific stays in a product (the TI-59's
  magnetic cards, the PC-1600's own peripherals).
- A change to shared behaviour (library, a frontend structure, harness,
  help, settings, formats) is carried to every product in the same
  branch. Every product builds and tests green on `main`.
- When in doubt, it is shared. A divergence that is found is a bug.

## No jumping controls (every GUI, every frontend)

- A control never moves because another control came or went. Bars,
  rows and dialogs keep their alignment (left stays left) whatever is
  shown.
- What the machine as configured can never have is not shown (no disk
  controls next to a CE-150, no card controls on a TI-58). Leave it out
  only where the controls around it stay where they are: such controls
  go at the end of their row, or after the controls that never change.
- What is only unavailable for now is shown disabled, with the reason
  as its tooltip.
- The one exception: the PC-1500/1500A and the PC-1600 differ too much
  for one layout; switching between them may reorder controls.

## Desktop, mobile and bars

The rules live in `library/shell/BarModel`, `library/shell/MenuModel` and
`library/shell/View`; the code is the rule. The Web page draws the models
its worker serves (`shell::ModelHost`); `library/web/components/shellModel.js`
mirrors only the rules that run on the page's clock and measurements (the
view, `barFit`, "Loading…", notices, the pointer).

- Every window is desktop or mobile (`shell::Traits::mobile`). Desktop:
  Qt always, a Web page in a desktop browser's tab; it has a menu bar.
  Mobile: the iOS app, the installed Web app, a browser asking for the
  mobile site (`calcuShellMobile`); it never has a menu bar.
- More is on every bar, at the right edge (End pushed, More alone in it).
  Load Preset… and Settings… are inside More, never bar buttons.
- More holds every menu item, in menu order under a heading per menu
  (FILE, EDIT, MACHINE, HELP); submenus stay submenus. Cold boot asks
  first there.
- The mobile bar keeps what is used while the machine runs: the model
  picker, the module slots and the media of the product's
  `mobileBarMedia`. Places go to More ▸ Peripherals ▸; every other media
  place gets a submenu in File (so in More): its entries, then Name & Save….
- A place picker's empty entry names its place ("No printer").
- One view rule for every GUI, `shell::view` (`library/shell/View`, in
  JS `calcuShellView`): the faceplate as large as the window allows; the
  room left beside or below it picks the view (panels next to it or
  underneath) when it takes the panels' minimum (the chrome's
  `panelsMinWidth`/`panelsMinHeight`), else the product's fallback
  (Calc-U-59: its pages; Calc-U-1600: the calculator alone, the panels
  alone on mobile with the room below). Desktop follows its window's
  size, mobile its rotation; where an app can lock its orientation it is
  locked to the faceplate's (Calc-U-59: portrait), Calc-U-1600 is not.
- Look and layout values live in the product's chrome (`app/TIChrome`,
  `app/SharpChrome`, the library's `shell/Chrome`), read by every GUI: Qt
  builds its stylesheets of it, Swift reads it through the bridge, the Web
  gets it with the shell-product answer as CSS custom properties. Panels
  side by side share the header height, the button-bar height and the
  pill. A colour literal in a frontend fails its product's CTest
  `…ChromeColours`.

## Working

- Working language English, for documents, code, comments and commit
  messages. Use the terms of `docs/GLOSSARY.md`.
- Short feature branches from `main`, merged with `--no-ff`; development
  and releases follow `docs/Process.md`. Pushing is fine; force-pushing
  is not.
- Tests: every test is in a group of `docs/test-groups.md` (Frequent on
  every change). Golden runs and reference runs are references: a
  difference is a finding to report, never a new reference.
- Code explains itself where it stands: a short comment that says why,
  not a pointer to another document.
- Help is Markdown in `library/help/` (shared parts) and
  `products/<product>/help/` (product parts), checked and built by
  `tools/help check|build <product>`. A user-visible change updates its
  chapter in the same branch.
- The formal specifications are binding: the file formats in
  `docs/formats/`, and the product references next to the code
  (`products/calc-u-59/docs/developer/`, `products/calc-u-1600/docs/developer/`).
  A change to a format, protocol or interface updates its specification
  in the same branch.
- Hardware and firmware facts come from the research skills (`PC1500`,
  `SC61860`, `pc1500-build`) and the research repository
  `~/Development/ti59-research`. Don't invent facts
  (sizes, behaviour, limits); ask.
- Agent knowledge follows the code: a rename also changes the skills,
  `CLAUDE.md` files and memory that use the old name.
- The code and these rules are the current state. Earlier decisions are
  not an argument. A change is proposed as "today it is X, proposed Y";
  once agreed, the code or the rule changes, without a log of decisions.
