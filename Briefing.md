# Calc-U — Briefing

One repository for the Calc-U emulators (Calc-U-59, Calc-U-1600, and later
possibly an SC61860 product). The emulators still ship as separate products,
each on its own release cycle. The goal is to remove duplication and drift in
the features the products have in common.

## 1. Goals

### 1.1 Products keep working
- Calc-U-59 and Calc-U-1600 continue to be released, without breaking any
  user-visible feature.
  - Each emulator keeps its functionality. Its GUI changes where family
    coherence (§1.5) or shared GUI concepts (§1.3) need it.
- Each product keeps its own release cycle and its supported platforms (§4.2).
- Each product keeps its own release material, such as App Store screenshots.
  Screenshots are retaken only where something visible changed.

### 1.2 Supporting files (presets, cards, floppies, state files, …)
- These rules apply to Calc-U's **own** formats: presets, state files,
  settings and its own card/floppy containers. **External** formats (from
  the real machines or outside tools: tape WAV, ROM dumps, SDE exchange
  formats, raw BASIC/binary images) must never break, in either product.
  The inventory tags every format as own or external.
- **Calc-U-59 stays backwards compatible.** Anything it reads may be in the
  old format. Anything it writes uses the new format. Writing an item that
  was just read therefore upgrades it.
  - "Old format" means every explicitly versioned format the latest
    *released* Calc-U-59 reads. This includes the files bundled in the app
    and the files hosted on calc-u.org.
  - The preset format (`.ti59`, `.ti58`, `.ti58c`) and `ti58c.mem` carry no
    version in 2.3.0. Exactly as 2.3.0 reads them, they are **version 1**.
    From now on these formats require a version tag. A file without one is
    read as version 1 and written back unchanged, except for the added
    version 1 tag.
  - The same applies to the ASM overlay files (`.asm`, own format, 4 of
    them hosted on calc-u.org): as 2.3.0 reads them they are version 1.
  - **Settings become a file format.** Today both products keep settings in
    the platform stores (UserDefaults, QSettings, `localStorage`). They are
    replaced by one documented settings file per product (for example
    `Calc-U-59.ini`, as MAME and similar programs do), read and written by
    one shared implementation; an exporter follows later. The first start
    of the new release converts the existing platform store into the file.
    A lookup hierarchy (dot directory, home, …) may follow later. Details
    (file syntax, location per platform, the web) → phase 3/4.
  - Remnants from before versioning started are dropped: any other
    unversioned legacy format is no longer read.
  - No forward compatibility: an upgraded file need not open in an older
    Calc-U-59.
- **Calc-U-1600 needs no backwards compatibility.** Files saved by 0.8.0 may
  not load in the first release from this repository. A release-note
  warning is enough; no converter.
  - In preparation for 1.0.0, its preset, card and floppy formats already
    carry `format-version: 1`. Existing version numbers stay as they are,
    even where a format changes.
  - `.floppy.yaml` is an own format, although SharpDataExchange (SDE) also
    reads and writes it: Calc-U owns the spec, and SDE follows a change.
  - The legacy `.floppy.img` is dropped; no converter.

### 1.3 One source of truth for shared features
- Features that both products have (for example presets or keyboard support)
  have exactly one implementation, in the shared layer.
- **What stays per product:**
  - The calculator core.
  - Product material: faceplates, icons, the keyboard hit map and similar.
  - The GUIs themselves, as far as they really differ. Code duplicated
    between them moves to the shared layer (for example the preset loader).
- **GUI concepts are shared as far as possible.** The GUIs have much in
  common: a calculator, a printer, a debugger, a preset loader, and a loader
  for the machine's removable items (cards on the TI-59; memory modules,
  floppies and others on the PC-1600). Both load and save files, and both
  come with a collection of bundled material. How much of this can be shared
  beyond duplicated code (concepts, structure, components) is to be
  explored, not assumed (§6).
  - The products already drift apart: TI-59 Solid State Software modules
    are loaded via the settings, PC-1600 memory modules via a drop-down.
- **Shared layer vs. product scope.** A product uses the shared layer for a
  feature, but it only exposes as much of that feature as it supports today.
  Example: keyboard support has a shared mechanism plus a per-product
  definition of which host key does what on the emulator. Calc-U-59 must use
  the shared keyboard layer, but its key definitions stay at what Calc-U-59
  supports today. Extending them is not a goal of this project. The same
  applies to the DAP debugger: Calc-U-59 does not have to gain it.

### 1.4 Robust abstractions
- The abstractions between the layers keep redundancy and feature drift to a
  minimum.
- They are good enough to support, later:
  - adding a new GUI target (for example a Swift frontend for Calc-U-1600);
  - adding a new core (for example SC61860 machines in Calc-U-1600).
- Designing for these is a goal. Building them is not (§2).

### 1.5 Coherent for end users
- **One family.** All Calc-U products are recognisable as members of the
  same family. Learning to operate one immediately helps with operating the
  others. This changes the existing GUIs (§1.1).
- Vocabulary is the same across all emulators: a preset means the same thing
  everywhere.
  - A glossary term may rename a visible label, menu item or help text in a
    product. This counts as "same functionality" (§1.1). Every rename is
    listed in the phase 2 mapping table.
- The help sites are coherent. Common material is shared, not duplicated.

### 1.6 SC61860 preparation
- Prepare for later support of SC61860-based machines such as the PC-1403H.
  Whether they become part of Calc-U-1600 or a third product is open (§6).

### 1.7 Existing backlogs are carried over
- Both TODO files (§4.4) are reviewed item by item. Items relevant to the
  shared layer or the abstractions become part of the design.
- No idea gets lost. Every idea listed there is kept, either in the shared
  backlog or in the backlog of one product, even if this project doesn't
  implement it.

### 1.8 Guiding spirit
- One codebase, several products. PockEmul and MAME are the model for the
  first part (one project), not for the second: they ship one product, Calc-U
  ships several.

### 1.9 Modelling depth and explicit interfaces
- **A spectrum of abstraction.** At one extreme every chip and its wiring
  is modelled; at the other the whole machine is one "core" block. The
  products sit somewhere in between, and the level differs per machine. The
  architecture must allow moving a machine along this spectrum without
  restructuring the shared layer or the other machines.
  - Example: Calc-U-59 does not model the TI-59's chips individually. This
    is the main reason why the assembler program
    `examples/assembly/Decoder.asm` does not run; a further cause is in the
    shared seven-segment decoder (`docs/inventory/calc-u-59.md`). Refining
    the model so that it runs must be possible. Making it run is not a goal
    of this project (§2).
- **Guideline:** the model must be able to reproduce the observable
  behaviour of the machine correctly. In every case the machine's original
  ROM runs.
  - Functional emulation is allowed where it makes sense. Example: a
    printer emulates its output, not its mechanics.
- **Explicit interfaces.** As soon as several parts or devices are plugged
  together, the interface between them is modelled explicitly.
  - Consequence for Calc-U-59: today neither the printer interface to the
    PC-100C nor the 8-pin slot for the Solid State Software module is
    modelled. This may change. It is an internal restructuring: the
    user-visible behaviour stays (§1.1). Whether and when it happens is
    decided in phase 3 (§6).
  - Printer interface: the PC-100C has 12 contact springs, one of them
    unconnected; the calculator has 11 pads that mate with the 11 connected
    springs. Sources therefore call it an 11-pin or a 12-pin interface.
    These counts were made by hand on the devices. The "14-contact" in the
    PC-100A service manual (`ti59-research/PC100C/service_manual.txt:2`) is
    a typo.
  - Sources: the TMS0500 chip set as a whole is outlined at
    https://www.datamath.org/Chips/TMS0500-CS.htm; research notes and code
    are corrected against it where they disagree (chip names, clock,
    display decode). Chip names come as a family and its instances, for
    example family TMC0540 (Solid State Software ROMs) and instance TMC0541
    (Master Library). The module slot pin-out is at
    https://www.datamath.org/Chips/TMC0540.htm.

### 1.10 One testing concept
- Calc-U has a single testing concept that covers both products.
- The calculator cores and the shared layer have unit tests.
- Tests that involve a GUI need their own strategy. It must take into
  account that there are three GUIs (Qt6, Swift, Web) and that testing
  happens on macOS and on iOS (in the simulator). The strategy is open (§6).
- Ideally there are also installation and smoke tests for Windows and
  Linux, for example in Parallels virtual machines or on a remotely running
  Raspberry Pi. Whether and how: open (§6).
- Tests are grouped by how often they run:
  - **Frequent:** run often, so they must be very fast and have a short
    elapsed time; they run in parallel where that is needed to stay fast.
  - **Longer:** tests that need more time, such as some GUI tests.
  - **Checkpoint:** tests that run only at dedicated checkpoints, such as
    before a release. Examples: producing screenshots, the installation test
    on Windows.

### 1.11 Agent knowledge follows the glossary
- Once the glossary (`docs/GLOSSARY.md`) is defined, every skill and every
  other element of agent memory and guidance (`CLAUDE.md` files, memory
  files, developer docs written for agents) is updated to use its terms.
  This includes the research skills (§5.1).
- Plans and other historical records (for example Calc-U-1600
  `docs/background/plans/`) keep their wording; only living guidance gets
  the new terms.
- A skill that lives in a research repository (for example
  `Sharp1500-1600-Ref/skills/sharp-pc1500-1600/`) must work stand-alone,
  with only that repository checked out. The personal router skills (for
  example `PC1500`) are a superset that also point to it. Both are updated.
- Memory directories of paths that no longer exist (`claude/ti59-emul`,
  `sharp/Calc-U-1600-Binary`): what is still valid moves into the live
  memory, then they are deleted.
- Out of scope, for now: Calc-U-16 (`tinue/Calc-U-16`, a separate
  programmer-calculator project) and the SC61860 prototype
  `calc-u-1403h-ui`, including their memory directories.

## 2. Non-goals

- Frequent releases of Calc-U-59 or Calc-U-1600 during this project.
- One single emulator product.
- Redesign or rework of the research repositories. TI, PC-1500/1600 and
  SC61860 keep their repositories, knowledge bases and skills (§5). Their
  content may change as the project progresses: new findings, corrections,
  glossary terms (§1.11).
- Where a research note and a product disagree on a fact, the product code
  is authoritative. The discrepancy is recorded and the research note is
  corrected.
- Integration of SDE (SharpDataExchange). It stays a separate project and is
  pulled in where needed.
- Integration of ReleaseIt. It stays a separate project, but it is an
  upstream dependency like SDE: it may be extended in its own repository
  where releasing several products from one repository needs it.
- Adding or removing build targets. Every existing target stays, including
  the Calc-U-1600 web client. The design must allow new targets later
  (§1.4), but this project adds none.
- Mixing families that are too far apart. TI might later gain a TI-57 core,
  but an HP-41 core would be a separate product (Calc-U-41).
- Extending product features beyond today's set, for example DAP for
  Calc-U-59 or more host-key definitions (§1.3).

## 3. Decisions already made (not to be reopened)

- **One repository**, initialised in this directory. On GitHub it is
  `tinue/calc-u`, initially private.
- **Products:** two products as they are today, Calc-U-59 and Calc-U-1600.
  The design must allow adding a third product (for example for SC61860
  machines) without restructuring.
- **calc-u-site stays its own repository.**
- **Hotfixes:** until development in this repository starts, urgent fixes
  may still be released from the existing repositories. Once development
  starts, no more hotfixes are allowed.
  - "Development starts" means the first commit in this repository that
    imports product code (phase 6, step 1).
  - Until then, every hotfix in an old repository is carried into the
    inventory as a delta against its pinned baseline.
- **History:** the full history of both repositories must be archived (the
  mechanism is open, §6).
- **Baselines:** all imports, inventory and decisions use the release tags
  Calc-U-59 `v2.3.0` and Calc-U-1600 `v0.8.1`. 0.8.1 is assumed to be
  released; until it is tagged, the pin is the tip of `dev-0.8.1`. Pins:
  `docs/Process.md`, phase 1.
- **Working language is English**, for specs, design documents, code,
  research documents and skills. Original source material (PDFs, manuals) may
  be in other languages.

## 4. Starting point

### 4.1 Both repositories
- `main` is squashed. The full history only exists in the various
  `dev-x.y.z` branches.
- Few users: fewer than 1000, possibly fewer than 100. A niche product for
  retro enthusiasts.
- Both were built for personal use first and then shared, because such
  emulators are rare and the existing ones are not very good. The few users
  regard both as very good, better than any other emulator around.
- Both are released with ReleaseIt (`/Users/me/Development/public/ReleaseIt`);
  see `release.yaml` in each repository.

### 4.2 Products and platforms

| | Calc-U-59 | Calc-U-1600 |
|---|---|---|
| Repository | `/Users/me/Development/public/Calc-U-59` (`tinue/Calc-U-59`) | `/Users/me/Development/sharp/Calc-U-1600` (`tinue/Calc-U-1600`) |
| State | 2.3.0 released (2026-11-07); `dev-2.3.1` holds only the version bump | 0.8.0 released (2026-11-07); 0.8.1 (CE-161 write-protect switch) on `dev-0.8.1`, assumed released |
| Core / shared layer | C++17, CMake | C++20, CMake |
| Release tooling | ReleaseIt: Swift apps built locally (macOS: built and notarised; iOS: archived and uploaded to TestFlight), everything else from GitHub Actions (`build.yml`, dispatched) | ReleaseIt: everything from GitHub Actions (`build.yml`, dispatched); nothing built locally |

| Product | Frontend | Platform | Minimum version | Distribution | Status |
|---|---|---|---|---|---|
| Calc-U-59 | Swift / SwiftUI | iPhone, iPad | iOS 18 | App Store (`ch.erzberger.calcu59`), TestFlight | Released |
| Calc-U-59 | Swift / SwiftUI | macOS | macOS 15.6 | DMG on GitHub (Developer ID, notarised) | Released |
| Calc-U-59 | Qt6 (6.8.2 in CI) | Windows x86_64 / arm64 | — | Installer on GitHub (unsigned) | Released |
| Calc-U-59 | Qt6 (Ubuntu 24.04 package, 6.4.2 in CI) | Linux x86_64 / aarch64 | — | AppImage on GitHub | Released |
| Calc-U-59 | Qt6 | macOS | macOS 15.6 (CMake) | Not shipped; local builds only | Local only |
| Calc-U-59 | Web (React + WASM) | Browser | — | Zip on GitHub, embedded by calc-u.org | Released |
| Calc-U-1600 | Qt6 (Homebrew, unpinned; 6.12.0 in CI) | macOS arm64 | macOS 15.8 | DMG on GitHub (Developer ID, signed in CI) | Released |
| Calc-U-1600 | Qt6 (6.8.2 in CI) | Windows x86_64 / arm64 | — | Installer on GitHub (unsigned) | Released |
| Calc-U-1600 | Qt6 (Ubuntu 24.04 package, 6.4.2 in CI) | Linux x86_64 / aarch64 | — | AppImage on GitHub | Released |
| Calc-U-1600 | Web (React + WASM) | Browser | — | Zip on GitHub | Released, not on calc-u.org yet (see below) |

The Calc-U-1600 web client is built as a zip and attached to each release.
It is not published on calc-u.org yet, because the Calc-U-1600 help site
does not exist yet (§4.3); it will end up at calc-u.org/1600. The preview
at https://tinue.github.io/1600/ (private repository `tinue/tinue.github.io`)
stays, but it is not part of this project.

Calc-U-1600 downloads SDE (SharpDataExchange) release assets in its Linux
and Windows CI builds. The macOS and WASM libraries are committed binaries.

### 4.3 calc-u-site
- Repository: `/Users/me/Development/public/calc-u-site`. Builds
  https://calc-u.org (documentation; it also embeds the web clients).
- The site must always reflect a released product, so it is in scope, but it
  stays a separate repository (§3).
- `/59/` is the full Calc-U-59 site and its only source. calcu59.ch is only a
  passive redirect to it, run at a hosting provider for as long as the
  domain is paid for; the new project never uses or mentions the old domain.
  Calc-U-59 has no `gh-pages` branch. The calc-u-site README is stale: it
  still describes a `gh-pages`/calcu59.ch setup. `/59/` embeds the 2.3.0
  web client (calc-u-site `acd629c`).
- `/1600/` is a placeholder. Building the Calc-U-1600 help site is in scope:
  it starts from Calc-U-1600's `docs/`, uses the shared-material structure,
  and publishes the Calc-U-1600 web client. This is new documentation, not a
  new product feature (§2).

### 4.4 Backlogs

| Source | Size | Content |
|---|---|---|
| `Calc-U-59/TODO.md` | 35 lines | Core emulation, settings, UI, CPU and calculator debugger, various, "far out" ideas |
| `Calc-U-59/ideas/` | 5 documents | `CardRecovery`, `CardSound`, `KeypressLatch`, `Pico`, `PresetConvergence` |
| `Calc-U-1600/TODO.md` (`v0.8.1`) | 732 lines | Known issues, LH5803/SC7852 interface, serial port, loading, expansion connectors ("one model on both machines"), cassette tape, 1.0 announcement, shared layer and web GUI, feature ideas, code cleanup backlog |

## 5. Research base (repositories unchanged; content evolves, §2)

### 5.1 Global research skills

| Skill | Covers | Points to |
|---|---|---|
| `PC1500` | PC-1500/1500A/1600 hardware, firmware, ROM disassemblies, BASIC | `Sharp1500-1600-Ref`, `SharpBasicReference`, `pc1600`, `PC-1600-ROM`, `pc1500`, `PC-1500-ROM` |
| `pc1500-build` | Assembling LH5801 sources to `.bin` | `sdcc-pc1500` |
| `SC61860` | SC61860 machines: CPU, models, 11-pin port, tape and BASIC formats, ROMs | `SC61860` |
| (no research router) | TI-58/59: TMC0501, ROM, PC-100C | `ti59-research`; plus `rom/` and `reference/` inside Calc-U-59. There is no TI research skill: TI knowledge lives in the Calc-U-59 product skills (mainly `calc-u-59-core`, `calc-u-59-statefiles`). A router to the PDFs (like `PC1500`/`SC61860`) does not exist, and none is planned. |

### 5.2 Product skills (to be restructured, §6)

| Product | Skills |
|---|---|
| Calc-U-59 (`.claude/skills/`) | `calc-u-59-core`, `-shared`, `-swift`, `-qt`, `-web`, `-docs`, `-statefiles`, `-uitesting`, `pre-release-cleanup` |
| Calc-U-1600 | None as skills; guidance lives in `CLAUDE.md` and `docs/developer/` |

### 5.3 Repositories (all under `~/Development/`)

| Path | GitHub | Content |
|---|---|---|
| `ti59-research` | `tinue/ti59-research` | TI-59 research: architecture, quirks, ROM disassembly, PC-100C |
| `sharp/Sharp1500-1600-Ref` | `tinue/Sharp1500-1600-Ref` | Consolidated PC-1500/1600 reference |
| `sharp/SharpBasicReference` | `tinue/SharpBasicReference` | PC-1500/1600 BASIC references |
| `sharp/pc1500` | `tinue/sharp-pc1500` | PC-1500 ROM annotation, LH5801 disassembler toolchain |
| `sharp/PC-1500-ROM` | `tinue/PC-1500-ROM` | PC-1500, CE-150 and CE-158 ROM dumps and disassemblies |
| `sharp/pc1600` | `tinue/pc1600` | PC-1600 ROM annotation project |
| `sharp/PC-1600-ROM` | `tinue/PC-1600-ROM` | PC-1600 and CE-1600P ROM dumps and disassemblies |
| `sharp/SC61860` | `tinue/SC61860` | SC61860 knowledge base, ROMs, disassemblies |
| `sharp/sdcc-pc1500` | `pchambre/sdcc-pc1500` (third party) | sdas/sdld toolchain for the LH5801 |
| `sharp/SharpDataExchange` | `tinue/SharpDataExchange` | SDE: data exchange with real hardware (stand-alone, §2) |

### 5.4 NAS directories (`~/SynologyDrive/Dokumente/PDF/Vintage/`)

| Directory | Content |
|---|---|
| `TI/` | TI manuals, `TI-57/`, PPC notes, PPX Exchange, Solid State ROM material |
| `Sharp/PC-1500/`, `Sharp/PC-1600/`, `Sharp/CE-158X_Users_Manual/`, `Sharp/CE163x_material/` | PC-1500/1600 and peripheral manuals |
| `Sharp/SC61860/` | SC61860 documents (about 230 files, 1.4 GB; indexed in `SC61860/research/`) |
| `Sharp/` (other) | Newsletters, magazines, textbooks, general Sharp material |

## 6. Open points

Each point names the phase that settles it (`docs/Process.md`).

- SC61860: part of Calc-U-1600 or a third product; the name of the Sharp
  product. → Phase 3.
- How the full history of both repositories is archived. → Phase 3.
- The core seam: whether the machine is the only core seam, with
  composition instead of family cores. → Phase 3.
- The testing concept (§1.10): the strategy for tests that involve a GUI
  across Qt6, Swift and Web, on macOS and in the iOS simulator; the time
  budget of each test group and where each group runs; whether and how
  installation and smoke tests run on Windows and Linux (Parallels, a
  remote Raspberry Pi). → Phase 1 (existing tests), phase 3 (decision).
- Skills: one set of skills per product, plus a shared skill. Structure
  open. → Phase 3.
- How calc-u-site shares common material across products. → Phase 3/4.
- Versioning and releases. Source repository and release repository are
  distinct: `tinue/calc-u` holds the source of both products and becomes
  public once the migration is done. Each product must be releasable on its
  own: work on one product and release it for a while without touching the
  other. Open: which versions exist (shared layer, Calc-U-59,
  Calc-U-1600?), and where releases are published; most likely the old
  repositories stay as pure release repositories (each has one "latest"
  release, which calc-u-site relies on). Needs analysis and research.
  ReleaseIt gets per-product subdirectories (`release -C <product dir>`,
  own `release.yaml`, CHANGELOG and version file); CI stays dispatched by
  ReleaseIt, no build on tag push. → Phase 3.
- Modelling depth per machine and which interfaces become explicit, for
  example the TI-59 printer connector and module slot (§1.9). Closely tied
  to the core seam above. → Phase 3, contracts in phase 4.
- How far GUI sharing goes across products and frontends (concepts,
  structure, components), and which existing GUI differences are aligned
  for family coherence, for example how modules are loaded (§1.3, §1.5).
  → Phase 1 (facts), phase 3 (decision).
- Calc-U-59 version 1 cards are written back as version 1 today, not in
  the new format (§1.2). → Phase 3.
- PC-100C printer connector: which of the 12 springs is unconnected is not
  recorded (§1.9). → Phase 3.

## 7. Success criteria

- One release each of Calc-U-59 and Calc-U-1600 is live, built from the new
  repository. Calc-U-59 is also live in the App Store.
- Each product supports at least its current set of features (§1.1). Its
  GUI changes only where §1.3 or §1.5 need it.
- Each product uses the new shared layer for every feature listed as shared.
  The list is `features/INDEX.md` (the KIT-n entries).
  A product that still has its own implementation of a shared feature does
  not meet this criterion.
- Calc-U-59 reads all of its existing files (§1.2) and writes the new
  format.
- calc-u.org reflects both releases and shares common material between the
  products instead of duplicating it. This includes the new Calc-U-1600 help
  site with the published web client (§4.3).
- A design for SC61860 support exists and has been validated against the
  abstractions: adding it needs no changes to the shared layer.
- Every item of both TODO files (§4.4) is accounted for: it is part of the
  design, or it is in the shared backlog or a product backlog, or it is
  recorded as done or dropped, with a reason.
- The testing concept exists and covers both products (§1.10). The
  calculator cores and the shared layer have unit tests, and every test is
  assigned to the frequent, longer or checkpoint group.
- Every skill and every other element of agent guidance and memory uses the
  glossary terms (§1.11).
- New product features are not required (§1.3, §2).

## 8. Constraints

- One developer, working with heavy Claude Code support.
- No time constraints.
- No commercial constraints: this is a hobby project.
- License stays GPL-3.0. Released products contain ROM files, with a
  takedown notice in this repository.
  - The design must allow removing the ROMs, with the user adding them after
    downloading the product. Design only; do not implement.
- Keep the existing language versions (C++, Swift, Xcode, Qt). Always
  consider whether they can be upgraded to stay current. Versions may be
  raised, never lowered; every raise is an ADR (for example Calc-U-59 from
  C++17 to C++20). The minimum OS versions below stay fixed.
- Swift app: iOS 18 and macOS 15 or later; keep this for now.
- Qt6 macOS app (Calc-U-1600): macOS 15.8 or later; keep this for now.
- Each artifact keeps its current build location: the Calc-U-59 Swift apps
  are built locally (macOS notarised, iOS uploaded to TestFlight),
  everything else on GitHub Actions. No
  cross-compile scaffolding in this repository.
- A local build must always be possible, for rapid iteration.
- The test harnesses and headless emulators must keep working, for rapid
  iteration by an AI agent. Their capabilities must stay (headless run,
  scripted input, state dump, golden tests, …); their interfaces may change
  if the skills and docs that use them change in the same step.
