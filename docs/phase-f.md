# Phase F: the road to the first releases

The first release of each product from this repository: Calc-U-1600
0.9.0 and Calc-U-59 3.0.0. This document applies on top of `CLAUDE.md`
and `docs/Process.md` until the closing check at its end has passed.

## Where things stand

- **Released 2026-11-28** on tinue/Calc-U:
  - Calc-U-1600 0.9.0, tag `calc-u-1600-v0.9.0`: the GitHub release with
    seven assets (Linux x86_64 and aarch64 AppImages, macOS DMG, Windows
    x86_64 and arm64 installers, the Web frontend zip, the help zip).
  - Calc-U-59 3.0.0, tag `calc-u-59-v3.0.0`: the GitHub release with
    seven assets ("Latest"), the Qt desktop app on macOS, Windows and
    Linux, and the iPhone/iPad app in the App Store.
- The pre-releases, their testing and the open items of the release
  period are closed with these releases.
- Left: the two site folders, the two old repositories, then the closing
  check.

## `/1600/` on calc-u.org

- [ ] Per `docs/help-site.md`: the help pages from the pinned help asset
      of 0.9.0, the Web frontend at `/1600/play/`, the installable app, the
      site's checks and tests.

## `/59/` on calc-u.org

- [ ] Per `docs/help-site.md`: the pinned help asset and 3.0.0 Web
      frontend, redirects for every address of today, the upgraded hosted
      files, exact release links, the new embed code. The site branch
      `mac-is-qt` (the Mac app is the Qt DMG) goes live with it.

## Archive the old repositories

- [ ] tinue/Calc-U-1600 and tinue/Calc-U-59 each get a README pointing to
      tinue/Calc-U and are archived on GitHub (read only; their history
      stays).
- [ ] Their old memory directories are deleted; what is still valid is
      already in this repository's memory.

## Closing check

Phase F ends when:
- [x] both releases are out (GitHub releases; Calc-U-59 in the App Store);
- [ ] both old repositories are archived;
- [ ] calc-u.org shows both products with the help and Web frontend of
      their releases.

Then:
- [ ] move `docs/phase-f.md` and `docs/help-site.md` to the branch
      `history` (added there, deleted on `main`);
- [ ] remove their pointers from `CLAUDE.md` and from the top of
      `docs/Process.md`;
- [ ] from then on, work by `docs/Process.md` only.
