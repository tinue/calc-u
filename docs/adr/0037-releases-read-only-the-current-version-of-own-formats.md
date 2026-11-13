# Each release reads only the current version of its own formats

Status: accepted (2026-11-10); scope made explicit 2026-11-10 (user,
phase 4 review): a one-time exception for the migration, see "Scope".
Made explicit again 2026-11-13 (user): the regular rule upgrades a file
silently and in place, and a version number changes only when the
requirements change (`docs/formats/README.md` §2).
Amended 2026-11-13 (user): the 2.3.0 card files (`.u59`) and
`ti58c.mem` are ignored, not refused, and get no hint; the new files
have new names (`.magnetcard.yaml`, `ti58c.memory.yaml`; C59-7 AC-4,
C59-3 EC-2, behaviour change 59-1).

## Scope: a one-time exception

This rule applies once, to each product's first release from the source
repository, because of the size of the rebuild. It is not the rule for
later releases. For every release after that first one, the regular
rule holds, for every product:

- only an incompatible change to an own format gets a new version
  number;
- a release reads at least the previous version of each of its own
  formats and silently upgrades the file in place to the current
  version; what it writes is in the current version;
- a format whose requirements stay the same keeps its version number,
  even when its specification is rewritten (user, 2026-11-13).

So the one violation is Calc-U-59's: its first release from this
repository does not read what 2.3.0 wrote. (Calc-U-1600 needed no
backwards compatibility before this release, Briefing §1.2.)

## Decision

Every product reads only the current version of each of its own formats
(presets, magnetic cards, `ti58c.mem`, ASM overlays, module definitions,
floppy images, settings files, …). A file in an older version, or
without a version, is refused with a clear hint that it is outdated and
must be upgraded, pointing to the format's documentation; there is no
converter. A format that changes in the migration gets a new version
number: the family preset format does. As part of the implementation,
every file a product ships and every copy hosted on calc-u.org is
upgraded to the current version. This applies to Calc-U-59 and
Calc-U-1600 alike. External formats (cassette WAV, ROM dumps, SDE
exchange formats, raw BASIC and binary images) are not affected and must
never break (Briefing §1.2). Decided by the user on 2026-11-10; it
replaces Calc-U-59's read-old/write-new rule.

## Considered options

- Calc-U-59 reads every old version forever, writes the current one
  (Briefing §1.2 until now; ADR 0018's v1 reader, ADR 0020's version 1
  cards): rejected by the user; old readers would have to be kept for
  ever.
- A converter, in the harness or in the app: rejected by the user; a
  hint is enough.

## Consequences

- After the migration release, the library's readers for own formats
  keep one earlier version each; "the oddities kept for the v1 reader"
  and the other points below concern the migration release only.
- Calc-U-1600's own rule "from 1.0 on, the previous format version keeps
  working" is replaced by the regular rule above: its first release from
  this repository is the baseline, and every later release reads one
  version back (user, 2026-11-10), whether or not it is 1.0 yet.
- Briefing §1.2 ("Calc-U-59 stays backwards compatible", the version 1
  rules for presets, `ti58c.mem` and ASM overlays, Calc-U-1600's
  "existing version numbers stay as they are, even where a format
  changes") and Calc-U-1600's own rule "from 1.0 on, the previous format
  version keeps working" (`docs/background/Decisions.md`, File formats)
  and the success criterion "Calc-U-59 reads all of its existing
  files" in §7 are replaced. The Briefing carries a note; Calc-U-1600's
  `Decisions.md` is updated with the code in phase 6.
- Superseded parts of earlier ADRs: ADR 0018's v1 reader; ADR 0016's
  consequence that the v1 reader fills in the module and printer; ADR
  0017's consequence for version 1 files; ADR 0020's reading of version 1
  cards. Each carries a note.
- Users' own files saved by Calc-U-59 2.3.0 or Calc-U-1600 0.8.x
  (presets, magnetic cards in the save folder, `ti58c.mem`, saved module
  instances and floppy disks) are refused after the update until the user
  upgrades them by hand. The release notes say so, with the hint text.
- `ti58c.mem` refused means the TI-58C starts with empty constant memory
  after the update.
- The oddities kept "for the v1 reader" (ODD59-9, ODD59-13) have no v1
  reader to stay in; the preset contract decides whether the family
  format keeps them.
- Every bundled and hosted file is upgraded and checked by the current
  reader before the first release (as ADR 0017 already required for the
  strict reader).
