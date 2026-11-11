# Contract: help structure

Status: draft (2026-11-10). Forks HL-1 to HL-3 are decided (decision
log). Settles the help open point of Briefing §6.

Inherits: ADR 0036 (help is Markdown in the source repository; shared
chapters once with the library, product chapters per product; shipped as
a release asset; calc-u-site imports and renders it at the release it
pins; Calc-U-59's JSX help converted once; the site keeps its design),
ADR 0024 (one menu and control-bar structure), Briefing §1.5 (family
coherence), §4.3, §6 (the chapter structure and the rendering are open
for phase 4).

## Today (facts)

| | Calc-U-59 | Calc-U-1600 |
|---|---|---|
| Source | React JSX pages in calc-u-site `59/Pages.jsx` (2246 lines), prerendered by `59/.seo/build.js`; site pins the Web frontend by sha256 (`docs/inventory/calc-u-site.md`) | Markdown in `docs/`: `User-Guide.md` (chapters 1 Introduction, 2 Getting started, 3 Plotters, 4 Loading a BASIC program, 5 Memory modules and floppy disks, 6 Machine-code programs, 7 COM ports, 8 Presets, 9 Appendix), `Debugger.md`, `Keyboard-Mapping.md`, `PC1600-Host-Drive.md`, module definition spec and format; Mermaid diagrams (`User-Guide.md` 8, `Debugger.md` 4); generated screenshots |
| Site | full help site at `/59/` | placeholder at `/1600/` |
| Format references | `reference/StateFileFormat.md` linked from the site | in `docs/` |

## Responsibilities

- The source repository holds every help page in Markdown: shared
  chapters once, product chapters per product (ADR 0036).
- Each product release ships its help as one release asset; calc-u-site
  pins and renders it.
- The help follows the family's structure (ADR 0024): what is the same
  in two products reads the same.

## Non-responsibilities

- The site's design and layout (calc-u-site, Briefing §3).
- Developer docs (they are not user help; they stay with the code).

## Invariants

- I-1: The help on the site always matches a released version (ADR
  0036).
- I-2: A concept shared by two products is described once.
- I-3: Help uses only glossary terms.

## Open forks

### HL-1: Product material inside shared chapters

**Decided (2026-11-10): option (b), fully generic** (the user's choice;
(a) was recommended). Shared chapters carry no product material and no
include mechanism exists; product examples live in the product chapters.

Facts: a shared chapter such as Presets needs product examples (TI key
IDs vs Sharp `type:` lines).

Options:
- (a) **Recommended:** a shared chapter may include a named snippet that
  each product supplies (one Markdown include directive, for example
  `<!-- include: product/presets-example.md -->`), resolved when the help
  asset is built; nothing else is templated. A product without the
  snippet leaves that part out.
- (b) Shared chapters fully generic, all examples in product chapters:
  no mechanism, but readers jump between chapters for an example.
- (c) A templating language with conditions: more power, harder to
  write and check.

### HL-2: Chapter structure (Briefing §6)

**Decided (2026-11-10): option (a)**, with HL-1 (b): the "snippets" in
the list below are product chapters or product sections instead.

Proposal, following ADR 0024's menu and control-bar order; S = shared,
P = per product:
1. Introduction (P): the product, its models, what is emulated.
2. Getting started (S + P): window, control bar and menus (S), the
   faceplate and keyboard map (P).
3. Models and machine control (S): model, variant, Reset, Cold boot,
   power cycle on attach, ROMs and the ROM folder (missing ROMs).
4. Media (S): templates, instances, Name & Save, insert and eject,
   outdated files (with the TI card reader and fit, the Sharp modules,
   disks and tapes as product sections or snippets).
5. Peripherals and paper (S + P): printers and plotters, the paper
   panel, product devices (PC-100C; CE-150, CE-1600P, CE-158).
6. Presets (S + P snippets): format, steps, startup presets; reference.
7. Debugger (S + P): pause, step, resume, presets' debugger steps (S);
   Calc-U-59's panel and ASM overlay, Calc-U-1600's VS Code debugger (P).
8. Settings (S): the INI file, where it lives, every key (product keys
   as snippets).
9. Product topics (P): Calc-U-59: cue cards, Solid State ROM modules,
   TI-58C constant memory, fast mode; Calc-U-1600: loading BASIC and
   machine code, COM ports, host drive, memory module definitions.
10. Reference (S + P): file formats (current versions), keyboard maps,
    glossary.

Options:
- (a) **Recommended:** this structure.
- (b) Another order or cut (to be given).

### HL-3: The release asset and its rendering

**Decided (2026-11-10): option (a)**, with HL-1 (b): there are no includes
to resolve.

Options:
- (a) **Recommended:** each release attaches `<Product>-help.zip`:
  Markdown files with includes resolved, images, and a table of contents
  file in chapter order. calc-u-site pins it by version and sha256 as it
  pins the Web frontend, and renders Markdown and Mermaid itself with its
  own design. The in-app Help menu links to the product's site
  (`https://calc-u.org/59/`, `/1600/`).
- (b) The release ships rendered HTML: the site loses control of its
  design.

## Fit table

| | Calc-U-59 | Calc-U-1600 |
|---|---|---|
| Source today | JSX in calc-u-site, converted once (ADR 0036) | `docs/` Markdown, moved |
| Product chapters (HL-2, 9) | cue cards, modules, constant memory, fast mode | BASIC and machine code, COM ports, host drive, module definitions |
| Product examples (HL-1) | in product chapters: TI key examples, card reader | in product chapters: Sharp `type:` examples, modules, disks, tapes |
| Site | `/59/` | `/1600/` (new, Briefing §4.3) |

(Per product, not per model: help is per product.)

## Open questions

- Q-1 (answered 2026-11-10): Whether the paper machines (SC61860) need anything here before
  a third product exists: no (help is per product).

## Decision log

| Date | ID | Decision |
|---|---|---|
| 2026-11-10 | HL-1 | Shared chapters are fully generic; product examples live in product chapters; no include mechanism. |
| 2026-11-10 | HL-2 | The ten-chapter structure above, in menu and control-bar order (shared and product chapters). |
| 2026-11-10 | HL-3 | Each release attaches `<Product>-help.zip` (Markdown, images, table of contents); calc-u-site pins it by version and sha256 and renders Markdown and Mermaid itself; the in-app Help links to the product's site. |
| 2026-11-10 | HL-3 (completed, user, 2026-11-10, batch 8 gate) | The asset's table of contents is `toc.json`. A help tool in the source repository builds and checks the asset (`tools/help build <product>`, `tools/help check <product>`); words the glossary lists under "Avoid" are warnings. Of calc-u.org/59, the install pages go into chapter 2 as a product part and the FAQ into chapter 9; the software pages, their articles and the page with the Web frontend stay pages of the site. Pictures that show what changed are taken anew by hand for the first release. On the site each chapter has an address of its own and today's addresses redirect; the Web frontend of Calc-U-1600 is at `/1600/play/` and `/1600/calc/app/`; each release carries its examples as a file, which the site pins; the design of `/1600/` is decided in calc-u-site. |
