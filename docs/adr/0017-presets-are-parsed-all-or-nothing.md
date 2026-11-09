# Presets are parsed all-or-nothing

Status: accepted (2026-11-09)

Every preset, in every version and product, is parsed completely before
the machine is built; any error refuses the whole preset with a message
that names the line. This is Calc-U-1600's rule. Calc-U-59 today reports a
bad line and keeps loading; that tolerance goes for version 1 files too.

## Consequences

- A Calc-U-59 version 1 file that loads today despite a bad line is
  refused afterwards. A malformed file is not valid version 1, so this
  does not break Briefing §1.2, but every bundled preset and every preset
  hosted on calc-u.org must pass the strict reader before the first
  release; any that fails is corrected.
