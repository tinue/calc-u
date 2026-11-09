# Media live in the save folder

Status: accepted (2026-11-09)

Every medium of every product, magnetic cards included, follows
Calc-U-1600's model: templates are bundled, instances are made by Name &
Save and live in the product's save folder, where they autosave. The
pickers list only templates and the save folder. A medium file elsewhere
is reached in two ways only: a preset names it by path (relative to the
preset, `~/` allowed, as Calc-U-1600's `slot-1-file:` and `floppy-file:`
do), or the user puts it into the save folder; on the web, Saved Media
imports, exports and deletes save-folder media. Presets themselves still
load from anywhere and are never written back. Calc-U-59 today opens
`.u59` files anywhere and writes back to them in place; that ends.

## Considered options

- The media model plus opening and writing back any card file in place:
  closer to Calc-U-59 today, but two save behaviours for the same kind of
  medium.

## Consequences

- How a medium that a preset names by path is saved (written back, or
  saved as an instance) is part of the media contract (phase 4).
