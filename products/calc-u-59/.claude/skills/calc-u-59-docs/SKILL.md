---
name: calc-u-59-docs
description: Use this skill when working on the Calc-U-59 documentation — the user help (help/, with the shared parts in library/help/ at the repository root), the developer documentation (reference/, root markdown files) — or deciding where documentation belongs. Covers the help-vs-developer split, the help tool (tools/help check|build calc-u-59) and the reference architecture docs. Trigger on any task that touches help/, reference/*.md, README.md, CHANGELOG.md, TODO.md, or PRIVACY.md. Also trigger when the user says "update documentation" (or similar) after finishing core/, app/, frontends/swift/App/, frontends/qt6/ or frontends/web/ work on a release branch — see the Release Documentation Update Workflow section, which updates the help as well.
user-invocable: true
---

# Calc-U-59 Documentation

This skill covers the user help (`help/`), the reference architecture docs (`reference/`) and root markdown files.

---

## Documentation Policy: two audiences, two locations

- **The help is the *only* place for end-user documentation** —
  installing, operating, syncing, modules, settings, the file formats and
  the debug panel from a user's point of view. It is Markdown in this
  repository (`docs/formats/help.md`): the product parts in `help/` (one file or
  more per chapter, `help/help.yaml`), the parts both products share in
  `library/help/` at the repository root. If you're about to write
  something an app user (not a contributor) would want to read, it goes
  there, not in `reference/` or a root markdown file.
- **Everything in the main repository (`reference/`, root markdown files,
  `.claude/skills/`) is developer documentation** — for contributors, the
  project owner, and AI agents working on the code. It assumes the reader
  can read Swift/C++/Qt and doesn't need app-usage instructions.
- **`README.md` points to the help** (calc-u.org/59/ and `help/`) rather
  than re-explaining or indexing end-user content.
- The specifications of the file formats are `docs/formats/` at the
  repository root; the help's chapter 10 describes them for users
  (`help/10-*.md`). The debug API and the internals live in `reference/`.
- **Skill files (`.claude/skills/*/SKILL.md`) must not duplicate content
  that already has a home in a developer-accessible markdown file.** A human
  developer reads `reference/*.md`; only the agent reads skill files. If a
  skill and a reference doc say the same thing, delete it from the skill and
  link to the doc instead — see `calc-u-59-presets/SKILL.md` for the
  pattern (a few navigation aids, everything else is one link to
  `docs/formats/preset.md`).

---

## The help (`help/`)

The help has the family's ten chapters (`docs/formats/help.md` at the
repository root): a chapter has a shared part
(`library/help/NN-*.md`, which names no product, model or peripheral of
one product) and/or Calc-U-59's part (`help/NN-*.md`). It was converted
once from calc-u-site's `59/Pages.jsx` at `acd629c`; from then on
`help/` is the source, and the site's pages are not edited for it.

- Check it with `tools/help check calc-u-59` (CTest `CalcU59HelpCheck`,
  Frequent); `tools/help build calc-u-59` builds `Calc-U-59-help.zip`
  (CTest `CalcU59HelpBuild`; the workflow's `help` job; `release.yaml`
  attaches it). The tool and its checks: `tools/README.md`.
- `help/settings-keys.txt` is written by `CalcU59Tests`
  (`CALCU59_HELP_KEYS=1 CalcU59Tests settings_keys_file_matches_declaration`) from `settingsDecl()`; every
  key needs its line in chapter 8. `help.yaml`'s `names:` must cover
  `tiCatalogue()` (`help_names_cover_the_catalogue`).
- `help/downloads/` holds the preset tutorial's six presets
  (`tutorial_presets_load` loads them); `help/images/` the
  pictures, each used by a file.
- A user-visible change updates its chapter in the same step. Use the
  glossary's terms (`docs/GLOSSARY.md`).

The website https://calc-u.org/59/ is the separate repository
`calc-u-site` (`../calc-u-site`); it renders the help asset of a release
and keeps its own pages (the software collection, the articles,
the web calculator's page), under its own `CLAUDE.md` and skills. Nothing
about the site's build, design or URL map belongs in this repository.

The playable calculator on the site (`/59/play/`, the installable app at
`/59/calc/app/`) is this repository's web client, imported from a release's
`Calc-U-59-web.zip` — calc-u-59-web skill and `reference/WebClient.md`.

---

## Reference Architecture Docs (`reference/`)

These are AI-generated documents produced by reading the source code alongside hardware documentation. They describe the emulator's internals, not the user-facing app.

The list of documents with a line on each is `README.md` § "Appendix:
Documentation Index" (`reference/` and `rom/`); keep it current when adding
or removing a document.

The Qt6 app has no reference doc of its own: its class map and build live in
the `calc-u-59-qt` skill, its feature placement in `FrontendParity.md`.
ROM-level documentation (`rom/*-ROM-Guide.md`, `rom/fastmode.md`,
`rom/printerinterrupt.md`) sits beside the annotated listings in `rom/`.

There is no end-user guide in `reference/` — that content is the help
(`help/07-debugger.md` for the debug panel, for one).

When updating these files, keep the provenance notice at the top intact — it signals that the content is best-effort reverse-engineering, not authoritative hardware documentation.

---

## Root Markdown Files

| File | Purpose |
|------|---------|
| `README.md` | Project overview for GitHub, how to build the apps and the web client, and the index of every developer document — keep the index current when adding or removing a document |
| `CHANGELOG.md` | Version history (keep entries under the correct version heading) |
| `TODO.md` | Development task list |
| `PRIVACY.md` | App Store privacy policy |
| `THIRD-PARTY-NOTICES.md` | Qt6 (LGPLv3), web-client (React, Archivo) and ROM-image notices; shipped with the Windows and Linux packages |

---

## Release Documentation Update Workflow

Triggered by the user saying **"update documentation"** (or similar) after finishing work on `core/`, `app/`, `frontends/swift/App/`, `frontends/qt6/` or `frontends/web/` for a release. The current dev branch (`dev-X.Y.Z`) always starts at the HEAD of the previous release tag — that tag, not `main` and not an arbitrary commit count, is the diff base.

1. **Find the base commit — verify, don't trust a single command blindly.**
   `git describe --tags --abbrev=0` alone can mislead (a mistagged or extra tag), so cross-check two ways:
   ```bash
   grep -m1 MARKETING_VERSION frontends/swift/Calc-U-59.xcodeproj/project.pbxproj   # the target release
   git tag --list 'calc-u-59-v*' --sort=-v:refname   # find the entry just below the target version
   git merge-base HEAD calc-u-59-v<previous-version>  # should equal...
   # The first release from tinue/calc-u (3.0.0) has no previous tag here: its base is
   # the commit that imported 2.3.0 (v2.3.0 stays in the archived tinue/Calc-U-59).
   git rev-parse v<previous-version>                  # ...this, exactly
   ```
   If `merge-base` and `rev-parse` don't match, the dev branch has extra commits beyond a clean fork point (rebasing, cherry-picks, etc.) — stop and ask the user which commit is the real base rather than guessing.

2. **Review what actually changed.**
   ```bash
   git log <base-commit>..HEAD --oneline
   git diff <base-commit>..HEAD --stat
   ```
   Read the full diff for `core/`, `app/`, `frontends/swift/App/`, `frontends/swift/Bridge/`, `frontends/qt6/`, `frontends/web/` and `roms/` — that's where documented facts live. Skip doc-only or test-only commits already made earlier in the same session.

3. **Update `CHANGELOG.md`.** Confirm every user-facing change from step 2 has an entry under the current `[X.Y.Z] - work in progress` heading, in the existing style (bold **Area** prefix, `### Fixes` subsection for bug fixes). Don't add a release date — that happens at release time.

4. **Update reference docs** (`reference/CoreArchitecture.md`, `reference/AppLayer.md`, `reference/AppArchitecture.md`, `reference/DebugAPI.md`, `reference/FrontendParity.md`, `reference/WebClient.md`, `reference/NewGUIGuide.md`, and `rom/*.md`) for anything the diff invalidated: register model, SCOM layout, PC encoding, machine variants, bridge/debug API surface, settings, file formats. Keep each file's provenance notice intact.

5. **Update the help (`help/`, and `library/help/` where both products change).** Update its chapters for anything user-facing in any GUI: new settings, module behaviour, install steps (Mac, iOS, Windows, Linux), preset keys and steps, file formats, speed or accuracy; run `tools/help check calc-u-59`. The site renders the help asset of the release. What stays the site's (the software collection, the articles derived from `rom/*.md`, the example copies under `59/software-files/`) is updated in `../calc-u-site` under its own `site-59` skill and `CLAUDE.md`, committed there. Leave the web client alone: the site fetches `59/calc/` from a release by its own rules.

6. **The web client** (`frontends/web/`): if the diff touched `core/`, `app/`, `roms/` or `frontends/web/`, build and test it as the calc-u-59-web skill says. The site picks up a new build by pinning the release's `Calc-U-59-web.zip`.

7. **Commit, never push, in both repositories.** Main-repo changes (`CHANGELOG.md`, `help/`, `reference/`, `README.md`, `TODO.md`, `tools/`) commit from this repo's root; site changes commit in `../calc-u-site`, where a push publishes the site, so push there only when the user asks.

---

## Global Rules

1. **Compile, don't run.** Do not launch the Xcode iOS Simulator or run the built Mac app — that is the user's job. When runtime verification is needed, ask the user to launch the app and explain precisely what behaviour or output to look for.

2. **Commit often, never push.** Commit after each logical unit of work. Never run `git push`.

3. **Build on macOS.** Verify Swift-facing changes with a macOS build; build for iOS only when explicitly required (commands: `calc-u-59-swift` skill, § "Build Verification"). Doc-only changes need no build.

4. **Cite your sources.** When referencing a hardware spec, standard, or external document, include the https URL (or a pointer to the local reference file) inline in the relevant documentation paragraph or code comment.
