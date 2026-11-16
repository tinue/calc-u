# C59-7 Magnetic cards: design
Status: approved by the user at the Part D/E boundary (2026-11-16)

Paths without a prefix are under `products/calc-u-59/`; line numbers at
`7bdd51c` (main after the Part D simplification). The spec cites
2.3.0's `Shared/`, which is `app/` here (`docs/glossary-mapping.md`,
"App layer"). The library side of the step is LIB-7's design, §11 (the
Calc-U-59 column); this design holds the product row.

## 1. Scope

| Row | Step | Tests from (plan §3) | Done in |
|---|---|---|---|
| C59-7 | M-55 (with LIB-7 59) | AC-1–30; EC-1–7 | proposed M-58 (D-11; plan §5.1 says M-55) |

M-55 waits for M-3 (FS 10: the empty side, EC-2), M-9 (the reference
runs of the four card examples, AC-30), M-41 (LIB-7 Library) and M-54
(LIB-6 59, C59-2, LIB-18 59, C59-3: the library's loader for every
Calc-U-59 preset, the `cue-card:` section, the start of the product, the
constant memory file). It also uses M-53 (LIB-10 59: the TI debug
target, through which the product kind of AC-21 is no longer read) and
the Part D simplification (LIB-7 D-22: the binding of a pass-through
place is the fitted card).

What M-55 finds, taken as done by the earlier designs (if a step left it
otherwise, M-55 adapts and says so in the qa-report):
- M-54: the four card examples (`examples/globe-print.ti59`,
  `examples/historic/characters-fastmode.ti59`, `pdcm-card-tricks.ti59`,
  `pdcm-make-cards.ti59`) not upgraded and excluded from
  `calcu_preset_check.CalcU59Presets` (C59-2 AC-10: "the four card
  examples in M-55"); the steps `card:`, `arm:` and the header key
  `magnetic-cards:` not registered; `app/CardFile.*` still the reader of
  `.u59` cards for the interim card flow; the `cue-card:` section and its
  cue card block reader (C59-2 AC-5, AC-6); `ti58c.memory.yaml` with its
  cue card (C59-3 AC-7); `TIAppPart` filling a `parts::LibraryPartsSetup`
  (`docs/reviews/d-simplification.md` §9).
- M-53: the debug target `TMC0501` (LIB-10 AC-28) behind
  `machine::DebugTarget`; `TIDebugTargetAdapter` gone.

Also passes in M-55 (clauses of other features, plan §1 and §5.1):
C59-2 AC-10's four card examples; LIB-1 AC-33 for the card reader (one
media place capability, LIB-1 59 "Done in M-55"); LIB-15 AC-20's cards
(LIB-15 59 "Done in M-55"); LIB-3 AC-29's rows `CueCardChanged` and
`CardChanged` (the topic `c59.cue-card` and the media topic); LIB-12
AC-14's clause "`cuecards.txt` … leaves the source repository" (LIB-12
design §10).

Not here: the media model, the save rules, the pickers and the saved
media list (LIB-7); a card file dropped or opened with Open With (LIB-13
59, M-56: the card goes through `media.open-file` there); the drawing of
the control bar and the menus on the family structure (LIB-14 59, M-57;
C59-8, M-58). The hardware facts of the card reader are in the
`ti59-research` repository and `rom/TI59-ROM-Guide.md:487-517`; they are
not restated.

## 2. Modules and files

**New**

| File | Holds |
|---|---|
| `core/TIMagneticCard.hpp/.cpp` | `TIMagneticCardData` (the content of a card, §3.1) and `buildCardBank` (an in-file side as the bank the ROM writes, AC-19; moved from `app/CardFile.cpp`, D-2) |
| `core/TICardReaderPlace.hpp/.cpp` | `TICardReaderPlace : calcu::media::MediaPlace`, the card reader of the TI-59 (§3.2; AC-6 to AC-10) |
| `app/TIMagneticCardKind.hpp/.cpp` | the `KindDecl` `magnetic-card` and its codec for magnetic card format 3 (§3.3; AC-2 to AC-5, AC-29, EC-5) |
| `app/TIMagneticCardsKey.hpp/.cpp` | the header key `magnetic-cards:` (§3.4; AC-18, AC-20) |
| `app/ModuleCueCards.hpp/.cpp` | the module cue cards from the `cue-cards:` of a module file (§3.5; AC-22, AC-23) |
| `tests/magnetic_card_tests.cpp` | §6, T1 to T4 |
| `tests/card_flows_tests.cpp` | §6, T5 to T10 (the cases of `tests/part_flows_tests.cpp` that are about cards move here, AC-27) |
| `tests/fixtures/cards/*.magnetcard.yaml` | good and damaged card files in format 3 (T1, T2) |

**Changed**

| File | Change |
|---|---|
| `core/TICatalogue.cpp:89-90` | unchanged descriptor (`card-reader`, kind `magnetic-card`, pass-through, sides 0 and 1); the TI-59 machine offers the `TICardReaderPlace` as the capability of that place (owner component `card-reader`) |
| `core/TI59Machine.hpp:77-87`, `.cpp` | unchanged calls (`insertCard`, `cardEject`, `isCardPresent`, `isWaitingForCard`, `cardMode`); the place calls them |
| `core/TIAdapters.hpp:43-59, 154, 158`, `.cpp` | `TICardReaderAdapter` and `cardReaderOf` go (A-6 of LIB-7). `libExecPC` goes with them: its readers are the debug panel's (`app/DebugSnapshots.cpp:161`), which read it from the debug target of M-53 as a register (§3.6) |
| `core/` (TMC0501 product kinds) | the product kind `c59.library-program` (AC-21; §3.6) |
| `app/TIAppPart.hpp`, `TIAppPart.cpp:102-104, 158-166`, `TIAppPartFlows.cpp:60-62, 99-115, 183-185, 260-262, 306-327, 376-560`, `TIAppPartInternal.hpp:34-38` | the card flow goes (§3.7): `CardState`, `TICardReaderState` and the topic `c59.card-reader`; the commands `c59.fit-card`, `c59.fit-blank-card`, `c59.feed-side`, `c59.eject-card`; `m_fitted`, `m_pass`, `m_memoryCards`, `m_queuedCardInReader`, `fit`, `fitCard`, `fitBlankCard`, `feed`, `startSwipe`, `ejectCard`, `feedQueuedCard`, `dropPresetCards`, `queuedCardDone`, `writeFittedSide`, `memoryCardBanks`; the `waitsForMedium` of `Hooks` (`TICardWaitAdapter`, A-6). Stays and changes: `c59.tap-card` sends `media.feed` (D-12); `refreshCueCard` follows §3.5; `m_moduleCards` from the module file; `TIAppSetup::cuecards` goes |
| `app/TIAppPart.cpp` (the parts) | the app layer's `LibraryPartsSetup` gets `media.kinds = {magneticCardKind()}` and the module listing (C59-6 AC-8, LIB-7 AC-47); the header key of §3.4 joins the preset vocabulary through the setup's preset extensions |
| `app/CueCard.hpp:149-154`, `CueCard.cpp:402-418` | `resolveCueCard` loses the stand-in (AC-22: "No card is made up at run time"); `activeLibraryProgram` moves to the product kind (§3.6) |
| `app/ModuleCatalog.hpp:10-16, 42-48`, `.cpp` | `ModuleCards moduleCards(std::string_view cuecardsTxt, …)` becomes `moduleCards(const calcu::roms::RomSet&, std::string_view moduleID)` over §3.5; the comment on `cuecards.txt` goes |
| `core/ModuleFile.hpp:7-12` | comment only: the cue cards of the file are read by the app layer (§3.5); the core reads what it needs |
| `app/TIWebPart.cpp:88, 186-250, 333, 396`, `TIWebPart.hpp:47` | `c59.web.fit-card`, `c59.web.fit-blank-card`, `c59.web.card-bytes`, `c59.web.fresh-card-name`, the topic `c59.web.card` and its folder scan go: the library's `media.*` commands and topic (§4) |
| `app/FrameSampler.cpp:17` | `cardAutoEjected` goes: the place gives the card back (`takeGivenBack`) |
| `app/EmulatorSession.hpp/.cpp` (queued cards, `takeQueuedCard` `:366-385`, `clearQueuedCards`, `hostDone` for cards) | go: the library's `card:` and `arm:` steps (LIB-7 §3.8) |
| `app/DebugSnapshots.cpp:161`, `ModuleImage.hpp:63-65` | `libExecPC` from the debug target's registers (§3.6) |
| `tests/SessionRig.hpp` | the rig gets a media part and a save folder of its own (`library/testing/TempFolder`) |
| `tests/reference_runs_tests.cpp` | the four card examples are compared (AC-30); the in-file banks are read from the media part's in-file store, no longer `TIAppPart::memoryCardBanks()` |
| `examples/globe-print.ti59`, `examples/historic/characters-fastmode.ti59`, `pdcm-card-tricks.ti59`, `pdcm-make-cards.ti59` | upgraded by `preset.md` §9.1 and AC-30 (§3.8); their suffix follows M-54's rule for the other 53 |
| `CMakeLists.txt` (`calcu_add_preset_check(CalcU59Presets …)`) | the four leave `EXCLUDE` |
| `roms/solid-state/cuecards.txt` | deleted (LIB-12 AC-14; LIB-12 design §10) |
| `roms/fetch_roms.sh:26`, `roms/README.md` | the comment and the paragraph on `cuecards.txt` go |
| `tests/rom_missing_build.sh:22` | no longer copies `cuecards.txt` |

**Deleted:** `app/CardFile.hpp/.cpp` (format 1.0, 2.0 and the binary
layouts; `isCardFileName`, `withDefaultCardExtension`, `freshCardCue`;
`buildCardBank` moves to the core, D-2), `app/TIFileAccess.*`'s card
part (A-5; the file goes whole if M-54 took the constant memory out of
it), `tests/fixtures/cards/*.u59`, `*.bin`, the `CARD` records of
`tests/golden/formats.txt` (`:30937` on) and the card part of
`tools/goldens/formats_main.swift:29` (spec, Technical Requirements).

## 3. Seams and interfaces

### 3.1 The content of a card (AC-1, AC-19; LIB-7 AC-46, D-2)

```cpp
// core/TIMagneticCard.hpp
struct TICardSideDef {                       // an in-file side, before it goes in (AC-18, AC-19)
    int bank = 1;                            // 1–4
    bool isProtected = false;                // written as −n Write
    std::optional<int> partition;            // none: the calculator's when the side goes in
    std::map<int, std::uint8_t> steps;       // program: step → keycode, from the bank's first step
    std::map<int, std::array<std::uint8_t, 16>> registers;   // data: register → BCD
};
struct TIMagneticCardData final : calcu::machine::ProductData {
    // Per side: nothing (fresh), the 246 bytes the ROM writes, or an in-file definition.
    using Side = std::variant<std::monostate, std::vector<std::uint8_t>, TICardSideDef>;
    std::array<Side, 2> sides;
    std::optional<int> lastWritten;          // the side the last pass wrote (the codec writes only it, AC-5)
    // The app layer's: the card's name, description and cue card. The core
    // carries it through a pass untouched and never looks into it (D-1).
    std::shared_ptr<const calcu::machine::ProductData> label;
};
std::vector<std::uint8_t> buildCardBank(const TICardSideDef&, int programRegs);   // AC-19, D-2
std::optional<int> sideBank(const TIMagneticCardData::Side&);                     // the corner's number (AC-11)
```

The app layer's label is `TICardLabel { std::string name, description;
std::optional<CueCardContent> cueCard; bool isTemplate; }` (in
`app/TIMagneticCardKind.hpp`). LIB-7's `Content` of a card is a
`shared_ptr<const TIMagneticCardData>`.

### 3.2 The card reader as a media place (AC-6 to AC-10, EC-1, EC-2, EC-7)

`TICardReaderPlace` (core) on the TI-59 only; the TI-58 and TI-58C have
no capability of the kind (AC-6; the descriptor has no `card-reader`).

| `MediaPlace` | Does | AC |
|---|---|---|
| `id()` | `card-reader` | AC-6 |
| `insert(content, side)` | refused while a card passes ("A card is in the reader."); else the side's bytes go in: the 246 bytes, a definition built by `buildCardBank` with its `partition` or the machine's program registers now, or zeroes for a fresh side (EC-2: an empty side gives the ROM zeroes, `insertCard(nullptr, 0)` as today, `app/TIAppPartFlows.cpp:444-449`); the content is kept for the give-back | AC-7, AC-19, EC-2 |
| `takeGivenBack()` | once the core has given the card back by itself (`isCardPresent()` false after an insert; today `app/FrameSampler.cpp:17` and `TIAppPartFlows.cpp:260`): the kept content with the passed side replaced by `cardEject()`'s bytes when they are a whole bank (246 bytes) and `lastWritten` set, `written = true`; a read gives the content back unchanged, `written = false` | AC-7, AC-12 (LIB-7) |
| `eject()` | Eject Card while a card passes: `cardEject()`; a whole bank written is kept as in `takeGivenBack` and reported through the next `takeGivenBack()` (LIB-7 D-22's open point); a part of a bank is dropped (AC-10, D-8); returns the content | AC-10 |
| `side()` | the side in the reader, or none | AC-12 (lit corner) |
| `content()`, `revision()` | the kept content; the revision counts the passes that wrote | LIB-7 AC-6 |
| `waitsForMedium()` | `isInCardWait() && isWaitingForCard()` as today (`app/TIAppPartInternal.hpp:34-38`): the ROM waits in the card wait of `n Write`, `n INV Write`, not while it only waits for a key | AC-9 |
| `writeProtected()` | none: a bank's protection is the calculator's (AC-29) | AC-29, EC-7 |

A card the ROM does not take at once stays in the reader with the card
switch closed (AC-7; row 59-20); the place never gives it back by a time
of its own; `card:`'s `timeout:` is the library's (LIB-7 §3.8). The
calculator's reaction to a partition that is not its own (EC-1), to a
protected bank (EC-7) and to zeroes (EC-2) is the ROM's; FS 10's result
(M-3) is the expectation of EC-2's test.

### 3.3 The kind and its codec (AC-2 to AC-5, AC-29, EC-3, EC-5)

`KindDecl magneticCardKind()`:

| Field | Value |
|---|---|
| `id`, `label`, `suffix` | `magnetic-card` (the descriptor's, `core/TICatalogue.cpp:90`), "magnetic card", `.magnetcard.yaml` |
| `folder`, `bundledFolder` | Save; none (no bundled card files, LIB-7 AC-32) |
| `formatName`, `describedAt`, `currentVersion` | "Magnetic card format", `docs/formats/media.md` §1 (the help's reference chapter from M-60), 3 |
| `implicitTemplates` | one: "Fresh card", "A fresh magnetic card", `make()` = both sides fresh, no cue card (D-10) |
| `words` | `insertStep = "card"`, `armStep = "arm"`, `inFileKey = "card"`, `insertNeedsTime = true`; no `ejectWord` (there is no `eject:` step on the TI-59, LIB-7 AC-26) |
| new fields (LIB-7 §11, library additions) | `rereadBeforeWrite = true` (D-3), `offersNameAndSave = false` (D-5), `replaceQuestion` = "Replace card “{name}”? Its first write replaces both sides and the cue card." (AC-15, D-4) |

The codec (format 3, `docs/formats/media.md` §1.1, on
`calcu::formats` of LIB-11 D-3):
- **read** (AC-2, AC-3, AC-4, EC-5): `format: ti59-magnetic-card`; a
  `format-version` below 3 → `Outdated` (LIB-7 AC-9's hint); any other
  breach of §1.1 or `README.md` §1 → `Unreadable` with one sentence: a
  missing side key, a side without exactly 30 rows, an unknown key at
  any level, the cue card included (`cue-card.md` §3, through C59-2's
  cue card block reader). Keys are matched exactly, fixed values in any
  case. A byte-order mark or bytes that are not UTF-8 → `Unreadable`
  ("… is not a text file in UTF-8."), no longer a message about its size
  (EC-5). The info: `card-name`, `description`, `template`,
  `formatVersion`, no write-protect switch (AC-29), the content with the
  label.
- **write** (AC-5, EC-3): the file whole, in the key order of §1.1. With
  `source` (re-read just before, D-3): the source's sides, `card-name`,
  `description`, `template` and `cue-card` kept, only `lastWritten`'s
  side replaced; without one (the first write of a card started by Fit
  Fresh Card, a `saveas:` of an in-file card): both sides as the content
  has them and its label's cue card, or a cue card titled with the card
  name when it has none (`media.md` §1.4). `saved` from
  `Write::savedAt`. Comments are not kept. An in-file side that never
  passed is written as the bank `buildCardBank` makes with the
  partition of its definition or the machine's now (`saveas:` of an
  in-file card, AC-20).
- `.u59` and `.bin` are not of the kind's suffix, so the shelf neither
  lists nor reads them (AC-4, OQ-2); nothing touches them.

### 3.4 `magnetic-cards:` (AC-17 to AC-20)

A header key of the product on LIB-6's vocabulary, registered with the
app layer's preset extensions (`LibraryPartsSetup`, M-54):
- reads each item (`card-name`; optional `description`, `cue-card`;
  `sides` with `0`, `1` or both; per side `bank` 1–4 required, `protected`
  default `false`, `partition` as C59-2 AC-2, `program: |` in the keycode
  notation of `preset.md` §4.1, whose bare keycodes start at the bank's
  first step, and `data:` in §4.2's notation) into a `TICardSideDef`;
  contents outside the bank, two items with one `card-name` and an
  unknown key refuse the preset with the line (AC-18; LIB-6 AC-7);
- puts each item into the library's in-file store as a medium of the
  kind (`ReadContext::keep`, `InFileSet`; LIB-7 M-41 deviations), its
  content a `TIMagneticCardData` of definitions with its label;
- is refused on a TI-58 and TI-58C by the TI rules (`ModelRules::applies`
  marks it `[59: TI-59]`, as `card:` and `arm:`; `preset.md` §7; AC-17).

What follows is the library's: a `{ mem: X }` that no item defines is a
fresh in-file card from the implicit template, with a log line (LIB-7
AC-23; C59-7 AC-20); the store keeps what was written to a side until the
next load or model switch, through a Reset, a power cycle and a Cold
boot (LIB-7 D-10; AC-19, AC-25); a name stands for one card throughout
the preset (AC-20). A `{ name: }` or `{ file: }` that does not exist,
is outdated or cannot be read refuses the preset at read time (LIB-7
AC-23; EC-6).

### 3.5 The cue card that shows (AC-11, AC-22 to AC-24)

- **Module cue cards** (`app/ModuleCueCards`): `std::map<int,
  CueCardContent> moduleCueCards(const calcu::roms::RomSet&, std::string_view
  moduleID)` reads the `cue-cards:` items of the module's `<ID>.rom.yaml`
  from the ROM set (`modules.md` §2.3: `program` plus a cue card block),
  with C59-2's cue card block reader; a module file that cannot be read
  gives none and a log line. `roms/solid-state/cuecards.txt` and
  `TIAppSetup::cuecards` go (Qt6 `EmulatorController.cpp:64`, Web
  `bindings.cpp:60`, Swift `EmulatorViewModel.swift:240`,
  `ROMLoader.swift:36-41`, `Calc-U-59Tests/TestSession.swift:23`).
- **The fitted cue card** is one at a time (AC-22). The part keeps which
  source fitted last: the card reader's fitted card (a change of the
  binding of `card-reader` seen at a flush: another card, or none after
  Eject Card with no card passing), a `cue-card:` section (C59-2), the
  constant memory file's (C59-3 AC-7). Each takes the place of the one
  before (D-6). A fitted magnetic card shows its label's cue card, or a
  plain card titled with its name (row 59-27), with the bank numbers of
  its sides in the corners (`sideBank`, AC-11).
- **The topic `c59.cue-card`** (`TICueCardState`, kept; LIB-3 AC-29's
  `CueCardChanged`): while a library program is selected (§3.6), the
  module's cue card of that program, or, when the module file defines
  none, the fitted cue card (AC-22: "no module cue card"); otherwise the
  fitted cue card. New fields: `onShow` (a fitted magnetic card that no
  module cue card covers, AC-24) and `litCorner` (the side passing now,
  from the media topic's `passing`, while on show; AC-12). Computed at
  each `beforeFlush`, so it follows a change within one flush, well
  inside AC-22's half second.

### 3.6 The library program selected now (AC-21)

The product kind `c59.library-program` (owner component `TMC0501`):
`std::optional<int> selected() const`, from SCOM register 9's digits 4
and 3 as today (`app/CueCard.hpp:154`, `activeLibraryProgram`), none for
0. Listed with the TI machine's product kinds (`capabilities.md`, product kinds: "the cue card of the running library program"). The latched
module address that the debug panel shows (`libExecPC`, today on
`TICardReaderAdapter`) is read from the `TMC0501` debug target of M-53
(a register of its processor frame, named as M-53 names it), so the card
reader adapter can go whole; if M-53 made it one already, nothing
changes.

### 3.7 The flows, before and after

| Today (`app/TIAppPart*.cpp`) | After M-55 |
|---|---|
| `fitCard`, `c59.fit-card` (bytes, path) | the Arm Card dialog sends `media.choose {place: card-reader, entry}` (LIB-7 D-22: fitted, not fed) |
| `fitBlankCard`, `c59.fit-blank-card` | `media.start-new {place: card-reader, template: "Fresh card", name}`; the `asks:` reply carries `replaceQuestion` (AC-15) |
| `feedSide`, `c59.feed-side`; `tapCard`, `c59.tap-card` | `c59.tap-card {x, y}` stays (the corner rule, `app/KeyboardLayout.hpp:43`, is the product's) and sends `media.feed {place, side}`; `c59.feed-side` goes (a frontend sends `media.feed`) (D-12) |
| `ejectCard`, `c59.eject-card` | `media.eject {place: card-reader}`: a passing card is taken out and stays fitted (LIB-7 D-22, user 2026-11-16) |
| `writeFittedSide` (re-read, merge, write, `error(...)`) | LIB-7's autosave A3 with the codec (§3.3); a failed write is LIB-7's notice (EC-4) |
| KEYSTROKES `InsertCard:`, `ArmCard:`, `CARD:` blocks, `mem:` cards (`feedQueuedCard`, `m_memoryCards`) | `card:`, `arm:`, `magnetic-cards:` (LIB-7 §3.8, §3.4 here); the step logs and waits are the library's |
| `dropPresetCards` ("was never read or written") | LIB-7 AC-27's line at the next build ("… was armed and never fed.") |
| `clearCardAndCue` on a new machine | the library clears the binding on a model switch and a preset load (LIB-7 D-10); the part's fitted cue card is cleared with it (AC-25: a new machine has nothing fitted until the preset or the constant memory file fits something); a Reset, a power cycle and a Cold boot keep it (AC-25) |
| a card dropped (`c59.open-dropped`) | until M-56 the drop flow sends `media.open-file {name, place: card-reader, path or bytes}` for a `.magnetcard.yaml` (LIB-13 59 takes it over in M-56) |

## 4. Product side and frontends

The commands and the topic are the library's; the frontends draw them.
The labels follow the question of §10 (designed on its recommendation:
"Arm Card", "Arm Fresh Card").

**Qt6** (`frontends/qt6/app/`)

| File | Change |
|---|---|
| `ControlBar.cpp:55-67, 95-98` | the two "Crd" buttons (names "Arm Card", "Arm Fresh Card") while no card passes, "Eject Card" while one does, from the media topic's `card-reader` place (`passing`) (AC-13) |
| `MainWindow.cpp:41-42, 56-57, 89-99` | File ▸ "Arm Card…", "Arm Fresh Card…" open the two dialogs; `AppPaths::cardDir()` goes (A-7) |
| `CardPickerDialog.hpp/.cpp` (89 lines) | two modes kept: Arm Card lists the place's picker entries but the empty and the implicit template (the saved cards by name, an outdated one disabled with its hint as tooltip; "No cards found" when none, AC-14); Arm Fresh Card asks the name, shows the saved cards, sends `media.start-new`, and on `asks:` shows the question with Replace and Cancel (AC-15) |
| `EmulatorController.cpp:40-42, 64-66, 122-132, 235-256` | the topic `c59.card-reader` → `media`; `cuecards` and the card folder go from the setup; `fitCard`, `fitBlankCard` → `media.choose`, `media.start-new`; `tapCard` stays; `litCardCorner` from `c59.cue-card` |
| `AppPaths.hpp:29-30`, `.cpp` | `cardDir()` goes; the save folder is LIB-9's `Paths` (LIB-7 A-7) |
| `SettingsDialog.cpp` (Solid State Module) | the module list from the media topic's `slot-1` picker (LIB-7 AC-47; C59-6 §5 "module list from module files for today's menus", M-55); choosing sends `media.choose {place: slot-1}` |
| `CMakeLists.txt:107-111` | `cuecards.txt` leaves the resource glob |

**Web** (`frontends/web/`)

| File | Change |
|---|---|
| `src/Dialogs.jsx:35-103, 215-216` | `CardPicker` on the media topic: Arm Card lists the saved cards (outdated disabled with the hint), Arm Fresh Card asks the name and handles `asks:`; the note "Cards last for this page session — download one to keep it." (`:62`), the per-card download (`:94`) and "Upload card file…" (`:100`) go; in their place the dialog has "Import…" (`media.import`) and "Saved Media…", which opens the library's `CalcuSavedMediaDialog` wrapped as the Web test product does (LIB-7 AC-36; the File menu items with M-57, LIB-7 D-29) |
| `src/ControlBar.jsx:84-110` | the buttons from the media topic; names "Arm Card", "Arm Fresh Card" |
| `src/engine.js:40-44, 85, 124, 184-187` | `fitCard`, `fitBlankCard`, `freshCardFileName`, `ejectCard`, the `card` state and the `card` download go; `media` state and `media.*` calls; `cueCard` stays |
| `bindings.cpp:60`, `build.sh:43-48`, `calc-engine-worker.js:6` | `cuecards.txt` no longer bundled |
| the module list in the settings dialog | from the `slot-1` picker, as Qt6 |

**Swift** (named for M-58; M-55 keeps it building and working on the
topic, drawn natively in C59-8)

| File | Change |
|---|---|
| `Bridge/TIAppSession.h:47-56, 216-217`, `.mm` | `cuecards` goes from the start; `cardSwiping` and `cardWritten` → the media topic's `card-reader` place; `fitCard`, `fitBlankCard`, `ejectCard` → `media.choose`, `media.start-new`, `media.eject`; the saved cards as a list of names for the picker |
| `Bridge/TICoordinatedFiles.h/.mm` (C59-5 D-6) | implements the library's file access of LIB-7 §11 (D-9) instead of `cu59::FileIO` |
| `App/CardStorage.swift` (62 lines) | the iCloud Documents folder stays the save folder; `fileName = "card.U59"`, `hasCard`, `cardExists` go |
| `App/Views/CardPickerView.swift` (131 lines) | lists the saved cards by name from the bridge; Arm Fresh Card with the replace question (AC-14, AC-15) |
| `App/EmulatorViewModel.swift:156-175, 235-248, 323-345` | `cardState`, `hasCardFile`, `cardFileName = "card.U59"` and the dropped card's security scope (until LIB-13 59, M-56) on the topic |
| `App/ROMLoader.swift:36-41`, the Xcode project (`project.pbxproj:31, 89, 184`) | `cuecardsText` and the resource go |
| `Calc-U-59Tests/TestSession.swift:23` | `start.cuecards` goes |

## 5. Adapters

New: none. Removed in M-55 (LIB-7 §5): A-4 interim TI card flow
(`c59.card-reader`, `c59.fit-card`, `c59.fit-blank-card`,
`c59.eject-card`), A-5 `TIFileAccess` cards, A-6 `TICardWaitAdapter`
and `TICardReaderAdapter`, A-7 Qt6 `AppPaths::cardDir()`; LIB-3 §5's
interim topics `c59.card-reader` (gone) and `c59.cue-card` (kept as the
product topic of C59-2 AC-6 and AC-22, no longer interim); C59-6 §5's
"module list from module files for today's menus" (the `slot-1` picker);
LIB-1's "media place capability (card reader)" (`TIAdapters` row of
LIB-1 §5, line 576 of its design).

## 6. Tasks

Tests before the code. `CalcU59Tests` (Frequent) unless the row says
otherwise; `SKIP_UNLESS` the ROMs are there for cases that run the ROM;
the session cases run `PartRig` (`tests/SessionRig.hpp`) with a save
folder of their own, the harness cases `cu59::makeHarnessProduct()`
with `--save-folder`. Names `c59_7_acN_…`, `c59_7_ecN_…`.

| Task | What | ACs/ECs | Test (file: cases) |
|---|---|---|---|
| T1 | Codec, read | AC-2, AC-3, AC-4, AC-29, EC-5 | `magnetic_card_tests.cpp`: `c59_7_ac2_reads_format_3_both_sides_and_one_side`, `c59_7_ac3_refused_whole_missing_side_key_rows_unknown_key_cue_card_key`, `c59_7_ac3_keys_exact_fixed_values_any_case`, `c59_7_ac4_older_version_outdated_with_hint`, `c59_7_ac4_u59_and_bin_not_listed_not_read_not_changed`, `c59_7_ac29_info_name_description_template_no_switch`, `c59_7_ec5_bom_or_not_utf8_unreadable` |
| T2 | Codec, write | AC-5, EC-3 | `c59_7_ac5_written_whole_in_key_order_only_written_side`, `c59_7_ac5_saved_set_comments_dropped`, `c59_7_ac5_template_never_written`, `c59_7_ac5_opened_card_written_back_where_it_lies`, `c59_7_ec3_other_side_kept_as_the_file_holds_it` (the file changed between fit and write) |
| T3 | Content, bank builder | AC-1, AC-19 | `c59_7_ac1_side_holds_any_bank_either_side`, `c59_7_ac19_built_bank_is_the_roms_bank` (a program and a data side written by the ROM with `n Write` and the same content built from a definition: equal bytes, header and checksum included), `c59_7_ac19_partition_of_the_item_or_the_calculators` |
| T4 | Module cue cards | AC-22, AC-23 | `c59_7_ac23_ml_25_and_le_21_preprinted` (items whose title is not the module's `description` or whose ID is not its `module-name`), `c59_7_ac23_every_program_of_every_module_has_a_card` (the program count of each module's ROM header, `app/ModuleImage.hpp:46-55`), `c59_7_ac22_no_stand_in_made_at_run_time` |
| T5 | The place | AC-6, AC-7, AC-8, AC-9, AC-10, EC-1, EC-2, EC-7 | `card_flows_tests.cpp`: `c59_7_ac6_reader_on_ti59_only`, `c59_7_ac7_pass_reads_or_writes_and_gives_back`, `c59_7_ac7_card_not_taken_stays_until_the_rom_takes_it`, `c59_7_ac8_bank_read_at_idle_without_a_key`, `c59_7_ec1_other_partition_not_stored_display_blinks`, `c59_7_ec2_empty_side_zeroes_rom_errors_nothing_written` (FS 10's result), `c59_7_ac9_waits_for_medium_in_write_not_at_idle`, `c59_7_ac10_eject_keeps_whole_bank_only`, `c59_7_ec7_protected_bank_as_the_rom_does` |
| T6 | Fitting | AC-11, AC-12, AC-15, AC-16, AC-24, AC-25 | `c59_7_ac11_fitted_cue_card_with_bank_numbers_machine_sees_nothing`, `c59_7_ac12_corner_feeds_side_lit_while_in_reader_stays_fitted`, `c59_7_ac12_corner_size_fifth_width_27_7_height`, `c59_7_ac15_fresh_card_title_is_name_replace_asked_written_at_first_write`, `c59_7_ac16_fit_replaces_nothing_while_a_card_passes`, `c59_7_ac24_module_card_covers_corners_do_nothing`, `c59_7_ac25_reset_power_cycle_cold_boot_keep_fitted_passing_card_taken_out`, `c59_7_ac25_new_machine_nothing_fitted` |
| T7 | The cue card shown | AC-21, AC-22 | `c59_7_ac21_product_kind_gives_selected_program`, `c59_7_ac22_one_fitted_at_a_time_card_section_constant_memory`, `c59_7_ac22_module_card_of_selected_program_or_fitted`, `c59_7_ac22_follows_within_half_a_second` |
| T8 | Presets and the harness | AC-17, AC-18, AC-19, AC-20, EC-6 | `c59_7_ac17_card_finishes_when_passed_and_fitted`, `c59_7_ac17_arm_finishes_when_fitted`, `c59_7_ac17_card_not_taken_fails_after_timeout_last_item_not_waited`, `c59_7_ac17_ti58_refuses_card_arm_and_magnetic_cards`, `c59_7_ac17_harness_commands_card_and_arm` (`card { mem: X, side: 1 }`), `c59_7_ac18_items_read_bank_program_data_protected_partition`, `c59_7_ac18_outside_the_bank_or_two_names_refused`, `c59_7_ac19_side_last_written_kept_through_reset_until_load`, `c59_7_ac20_undefined_mem_is_fresh_logged_saved_only_by_saveas`, `c59_7_ac20_one_name_one_card_two_steps_fill_two_sides`, `c59_7_ec6_missing_outdated_unreadable_card_refuses_preset` |
| T9 | Today's tests | AC-27 | the card cases of `tests/part_flows_tests.cpp:93-803` and `tests/core_tests.cpp:272` ported as `c59_7_ac27_<old name>` without the earlier formats (`version_1_card_stays_version_1_on_a_write`, `preset_side_on_a_version_1_file_feeds_its_one_side` and the `.u59` layouts of `shared_formats_tests.cpp:205-464` go): a blank card written and read back; fed only on a corner; a read writes nothing; both sides in one file; the other side kept; a fresh card replacing a file only at its first write; `arm:` finished once fitted, `card:` once passed; a preset's card read by a program; a card written then inserted; an armed card never used logged |
| T10 | Every frontend | AC-13, AC-14, AC-26 | Qt6 (`calcu_qt6`-based tests of the product, offscreen, if M-57's shell is not there yet the dialog alone): `c59_7_ac13_buttons_and_eject_by_card_state`, `c59_7_ac14_dialog_lists_saved_cards_outdated_disabled_none_found`; Web (`play-calculator.spec.js`, Longer): `c59_7_ac13 buttons`, `c59_7_ac14 arm card lists the saved cards`, `c59_7_ac26 cards kept from one visit to the next` (a reload), the existing cases `:338, 353, 370, 398` on the media topic; Swift (`Calc-U-59Tests`, run from the main checkout): `c59_7_ac26_swift_cards_in_the_icloud_save_folder` through the bridge |
| T11 | GUI scenarios | AC-28 | Web: `play-calculator.spec.js` scenarios `C59-7 AC-15` (a fresh card armed, written by a program, listed by Arm Card) and `C59-7 AC-12` (a card armed and read on a corner) (Longer; today `:370-420`); Qt6 with M-57's attach scenarios, Swift with M-58's (D-11) |
| T12 | The four examples | AC-30; C59-2 AC-10 | `calcu_preset_check.CalcU59Presets` without exclusions; `CalcU59ReferenceRuns` (Longer) with the four examples compared with their M-9 recordings (display, paper, the in-file banks), every card fitted after it has passed (the one expected difference, AC-30) |

Coverage: T1 to T12 cover AC-1 to AC-30 and EC-1 to EC-7. AC-28's Qt6
and Swift scenarios pass in M-57 and M-58 (D-11).

## 7. Tools and agent knowledge (M-55)

| File | Change |
|---|---|
| `products/calc-u-59/.claude/skills/calc-u-59-statefiles/SKILL.md` | cards: format 3, `.magnetcard.yaml`, `magnetic-cards:`, `card:`, `arm:`, `saveas: { card: }`; `CARD:` blocks and `InsertCard:` gone (if M-54 left a card paragraph) |
| `products/calc-u-59/.claude/skills/calc-u-59-shared/SKILL.md` | the card flow is the library's media part with `app/TIMagneticCardKind`; `CardFile` gone; module cue cards from the module files |
| `products/calc-u-59/.claude/skills/calc-u-59-qt`, `-web`, `-swift` `SKILL.md` | the card dialogs on the media topic; `c59.card-reader`, `c59.web.card` gone |
| `products/calc-u-59/.claude/skills/calc-u-59-core/SKILL.md` | `TICardReaderPlace`, `TIMagneticCardData`, `buildCardBank` in the core; `TICardReaderAdapter` gone |
| `products/calc-u-59/reference/StateFileFormat.md` (or its successor of M-54) | § "Card files" → a pointer to `docs/formats/media.md` §1 |
| `products/calc-u-59/reference/SharedArchitecture.md` § "Card Reader Flow" | the media place, the codec, the fitted card |
| `docs/glossary-mapping.md` | rows "Magnetic card" (`CardDocument`, `CardFile`, `.u59` → `TIMagneticCardData`, `app/TIMagneticCardKind`, `.magnetcard.yaml`: done M-55), "Medium, kind of medium, media place" (`CardFile`, `TIFileAccess` → done M-55), "Arm" (done M-55, as §10's answer says), "Save folder" (Calc-U-59's "Card Folder", if a label remains) |
| memory `calc-u-59/card_timing_audio.md` | "the emulator has no card transport timing" stays true; the place is `TICardReaderPlace` |
| memory `calc-u-59/cuecard_template_semantics.md` | `CUECARD` `Template` values → the cue card block's (C59-2); SolidStateCard cards come from the module files |
| memory `calc-u-59/solid_state_pc_display.md` | the program number is the product kind `c59.library-program`; `checkProgramNumber` gone |
| `calc-u-59-memories.md` (index) | the three descriptions |
| `docs/test-groups.md` | the new files and cases |

## 8. Golden and reference runs, risks

- **Golden runs** (M-8) do not use cards: the core's calls are the same;
  the replays pass unchanged; a difference is a finding.
- **Reference runs** of the four card examples (M-9): the in-file banks,
  the display and the paper are compared; the one change AC-30 names (a
  `mem:` card is fitted after it has passed) is no difference of the
  machine. A difference in a bank is a finding.
- **AC-19's byte-for-byte bank** rests on `buildCardBank` as 2.3.0 has
  it (`tests/shared_preset_stages_tests.cpp:119-138` at the pin); T3
  compares it with what the ROM writes, which 2.3.0 did not test.
- **The re-read before a write** (D-3) is a read of the save folder at
  each pass that writes; on iCloud through the coordinated access. A
  file that cannot be read then fails the write with LIB-7's notice
  (EC-4), as today's error does.
- **Card dropped before M-56**: the interim drop sends `media.open-file`;
  LIB-13 59 replaces the drop flow in M-56.
- **Swift** cannot be built from a worktree (handover finding); its
  tests run from the main checkout before the merge.

## 9. Decided (best guess)

- **D-1 (AC-1; LIB-7 AC-46).** The content of a card is a core type
  (`TIMagneticCardData`), because the place must read the sides; the
  card's name, description and cue card ride along as an opaque label
  the core never reads, so the given-back card keeps them.
- **D-2 (AC-19).** An in-file side is a definition until it goes in; the
  place builds the bank then, with the partition of that moment, so
  `buildCardBank` moves into the core next to the card reader.
- **D-3 (AC-5, EC-3).** "The side that was not written is kept as the
  file holds it" is met by re-reading the instance's file just before
  each write and splicing the written side into it (a kind flag of the
  library, LIB-7 §11). LIB-7 EC-2's "nothing is merged" holds for the
  written side.
- **D-4 (AC-15).** The replace question's words are the kind's
  (`replaceQuestion`); the library's default sentence stays for the
  other kinds.
- **D-5 (AC-20; LIB-7 AC-16).** A card offers no Name & Save in the GUI:
  C59-7 names the card buttons (AC-13) and saves an in-file card only by
  `saveas:` (AC-20); the kind says `offersNameAndSave = false`, so the
  topic does not offer it.
- **D-6 (AC-22).** "Each takes the place of the one before" is kept as
  the last of three events: the card reader's fitted card changed, a
  `cue-card:` section ran, the constant memory file brought its cue
  card.
- **D-7 (AC-21, AC-22).** The library program is read at each flush
  from the product kind, not at 2 Hz in wall time; the topic follows a
  change within one flush.
- **D-8 (AC-10).** A bank counts as written by the ROM when the reader
  captured all 246 bytes (`kCardBankBytes`); fewer are dropped.
- **D-9 (C59-5 D-6).** The iCloud coordination stays for cards: the
  library's media part reads and writes through a file access the host
  may hand it (LIB-7 §11); Swift hands the bridge's coordinated one, the
  others the plain one. If M-54 already made such a seam for the
  constant memory file, the media part takes that one.
- **D-10 (AC-15; LIB-7 AC-32).** The implicit template is named "Fresh
  card"; the Arm Card dialog lists the saved cards only, not the fresh
  card (Arm Fresh Card is its way in).
- **D-11 (AC-28; plan §1).** The Qt6 frontend of Calc-U-59 gets
  scenarios through attach in M-57 (LIB-14 AC-44) and Swift's
  scenarios move to attach in M-58 (C59-8): AC-28 passes there, so the
  row is Done in M-58 (plan note).
- **D-12 (AC-12).** The corner rule stays a product command
  (`c59.tap-card {x, y}`, the hit test of `app/KeyboardLayout.hpp:43`),
  which sends `media.feed`; `c59.feed-side` goes, a frontend that knows
  the side sends `media.feed` itself.
- **D-13 (AC-4).** `.u59` and `.bin` are simply not the kind's suffix;
  no code looks at them, and their readers, fixtures and golden records
  go.

## 10. Questions for the user

- **Q-1 (AC-13, AC-14, AC-15 against `docs/glossary-mapping.md`).** The
  spec names the buttons, menu items and dialogs "Fit Card" and "Fit
  Fresh Card" (text of 2026-11-10). The label mapping, changed with the
  formats on 2026-11-12, renames them "Arm Card" and "Arm Fresh Card",
  which is the glossary's word for what they do (the card is made ready
  for the user to feed). *Recommendation, designed on:* the mapping's
  "Arm Card…", "Arm Fresh Card…" (buttons' names, menu items, dialog
  titles), a decision-log row in C59-7 and LIB-14 (AC-26 says "the
  magnetic card items of C59-7"), and a ledger row; the scenario IDs do
  not change. The other answer keeps "Fit Card" and strikes the mapping
  row.
  Answered (user, 2026-11-16, with the approval of the Part E designs): "Arm Card";
  decision-log rows in C59-7 and LIB-14, ledger row 59-50.
