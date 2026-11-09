# Media live in the save folder

Status: accepted (2026-11-09)

Every medium of every product, magnetic cards included, follows one
model: templates are bundled, instances are made by Name & Save and live
in the product's save folder, where they autosave. A file the user opens
from elsewhere (Finder, iCloud, a download) is imported, that is copied
into the save folder as an instance, before it is used; sharing a medium
means exporting it. Calc-U-59 today opens `.u59` files anywhere and
writes back to them in place; that ends, so a card the user changes in
the emulator no longer changes the original file outside the save folder.

## Considered options

- The media model plus writing back in place for files opened from
  elsewhere: closer to Calc-U-59 today, but two save behaviours for the
  same kind of medium.
