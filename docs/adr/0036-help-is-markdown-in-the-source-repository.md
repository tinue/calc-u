# Help is Markdown in the source repository; the site pins it

Status: accepted (2026-11-09)

User help is written in Markdown in tinue/calc-u: chapters for concepts
all products share (presets, media, Name & Save, the debugger, settings)
are written once with the library, and each product adds its own chapters.
A product release ships its help as a release asset, and calc-u-site
imports and renders it at the release it pins, the same way it already
pins the web client. Help therefore always matches a released version
(Briefing §4.3), and common material is shared, not duplicated (§1.5).
Calc-U-59's help, today React JSX pages in calc-u-site, is converted to
Markdown once; Calc-U-1600's `docs/` is already Markdown. calc-u-site
keeps its own repository, design and layout (§3).

## Considered options

- JSX in calc-u-site for both products: help would be edited outside the
  product and decoupled from its releases.
- Each product as today, sharing only common chapters by copy: duplicates
  the material §1.5 wants shared.
