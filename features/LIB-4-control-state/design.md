# LIB-4 Control state: design
Status: approved by the user at the Part C1/C2 boundary (2026-11-15)

## 1. Scope

| Column | Step | Tests from (plan §3) | Done in (plan §5.1) |
|---|---|---|---|
| Library | M-37 | AC-1 to AC-9; EC-1, EC-3 to EC-5 | M-37 |
| Calc-U-1600 | M-49 (with LIB-14 1600) | AC-16 to AC-20 | M-49 |
| Calc-U-59 | M-57 (Qt6, Web, with LIB-14 59), M-58 (Swift, with C59-8) | AC-10 to AC-15 | M-58 |

Part C2 works on the Library column only. The product columns are
sketched in §4 so that the review sees where each lands. Their tasks
are set out at the boundaries of Parts D and E, with the designs of
LIB-14 and C59-8, which draw what the control state holds (the spec:
"a column is complete together with the rows that draw it"). There is
no EC-2 in the spec.

M-37 waits for M-22 (LIB-3 L: topics, parts, busy) and M-23 (LIB-2 L:
places, fit, `canAttach`). It uses M-35 (LIB-18 L: `Machine::canReplace`,
the commands `reset` and `cold-boot`, and which commands end a running
operation, LIB-18 AC-16), so the plan's "Waits for" of M-37 should name
M-35. It needs no code of LIB-6, LIB-7 or LIB-8.
The actions "load a preset", "open a file" and "insert a medium", and
the paper panel, are computed from the busy state, the descriptors and
the product's list of printers (§3.4, D-3). Their commands come later
(M-38, M-41, M-42).

M-37 changes only `library/`. No product frontend reads the new topic
before M-49. The harness sends it from M-37 for every product (§7).

## 2. Modules and files

Library (M-37), new folder `library/control/` of `calcu_library`,
namespace `calcu::control` (LIB-1 §2a names it). It includes `machine`
and `sessions` only. The line `control: machine, sessions` is added to
LIB-1 §2a's dependency rule in the same step.

| File | Holds |
|---|---|
| `library/control/ControlState.hpp/.cpp` | the payload `ControlState` and its entries (§3.1), `same`, `toValue`; the topic name `control-state` |
| `library/control/ControlRules.hpp` | `EntryKey`, `ControlRules`: what a product's app layer adds or refuses (§3.3, AC-7) |
| `library/control/Compute.hpp/.cpp` | `computeControlState(...)`, a pure function of the machine, the catalogue, the setup and "an operation runs" (§3.4) |
| `library/control/ControlPart.hpp/.cpp` | `ControlPart : sessions::SessionPart`: declares the topic, computes at every flush (§3.5) |
| `library/control/tests/control_state_tests.cpp` | AC-1, AC-5, EC-1, EC-3, EC-4 |
| `library/control/tests/availability_tests.cpp` | AC-2, AC-3, AC-7, EC-5 |
| `library/control/tests/control_part_tests.cpp` | AC-4, AC-6, AC-8 |
| `library/control/tests/TestControlRules.hpp` | a product's rules for the tests: one product entry, one refusal on a named entry |
| `library/machine/tests/TestMachines.cpp` (changed) | `test-hub` gets a holding media place `hub-disk` (kind `test-disk`, side 0) for EC-4 |
| `library/sessions/SessionPart.hpp`, `AppSession.cpp` (changed) | `SessionPart::beforeFlush` (§3.5; in the list of C2's LIB-3 additions, LIB-18 §8) |
| `library/sessions/tests/topic_tests.cpp` (changed) | `before_flush_runs_at_every_flush` |
| `library/harness/Run.cpp`, `Product.hpp` (changed) | the harness session gets a `ControlPart`; `HarnessProduct::controlSetup()` (§7) |
| `library/harness/tests/commands_tests.cpp` (changed) | AC-9 |
| `library/CMakeLists.txt` (changed) | the sources of `library/control/` and its tests in `calcu_library` and `calcu_library_tests` |

## 3. Seams and interfaces

Sketches. Everything is single-threaded on the session's thread and
throws nothing across its interface (LIB-1 §2a).

### 3.1 The payload (AC-1, AC-2, AC-9)

```cpp
namespace calcu::control {

inline const sessions::TopicName kControlStateTopic{"control-state"};

struct Availability {                       // AC-2: available now, or one reason
    bool now = true;
    std::string reason;                     // one English sentence when !now (EC-5)
    bool operator==(const Availability&) const = default;
};

struct VariantChoice   { machine::VariantId id; Availability available; /* == */ };
struct ModelEntry      { machine::ModelId id; Availability available; /* == */ };

struct PeripheralChoice {                   // one peripheral that fits a place (LIB-2 AC-2)
    machine::PeripheralId id;
    std::string name;                       // the descriptor's name
    bool plugged = false;                   // EC-1
    std::optional<machine::VariantId> variant;   // plugged: the one that runs; else the one an attach uses
    std::vector<VariantChoice> variants;    // empty: the peripheral has none
    Availability available;                 // not plugged: its attach (a replace when the place is full); plugged: its detach
};

struct PlaceEntry {
    machine::PlacePath path;                // {"printer"}; {"aux", "jack"} on a peripheral (LIB-2 AC-9)
    machine::ConnectorId connector;
    machine::PinOutId pinOut;
    Availability available;                 // the place as a whole: only a product rule refuses it (AC-7, AC-18)
    Availability empty;                     // the empty place as a choice (D-4)
    std::vector<PeripheralChoice> fitting;  // catalogue order, modules read later at the end (EC-3)
};

struct MediaPlaceEntry {
    machine::MediaPlaceId id;
    machine::PlacePath ownerPlace;          // empty: the machine
    std::optional<machine::PeripheralId> owner;   // the peripheral plugged in at ownerPlace (EC-4)
    std::string mediumKind;
    machine::MediaPassage passage;
    std::vector<int> sides;
    Availability insert;                    // the action "insert a medium" here (AC-1, OQ-1)
};

struct PanelEntry {                         // AC-1; LIB-8 AC-12
    std::string kind;                       // "paper": the only panel the library knows
    std::optional<machine::PlacePath> ownerPlace;
    std::optional<machine::PeripheralId> owner;   // the plugged-in printer, or none
};

struct ActionEntry { sessions::CommandName command; Availability available; };   // reset, cold-boot, load-preset, open
struct ProductEntry { std::string name; sessions::PayloadPtr value; };   // "c1600.disk-controls"; carried unread (AC-7)

struct ControlState final : sessions::PayloadOf<ControlState> {
    static constexpr sessions::PayloadKind kKind{"control-state"};
    std::vector<ModelEntry> models;         machine::ModelId model;
    std::vector<VariantChoice> variants;    std::optional<machine::VariantId> variant;
    std::vector<PlaceEntry> places;
    std::vector<MediaPlaceEntry> mediaPlaces;
    std::vector<PanelEntry> panels;
    std::vector<ActionEntry> actions;
    std::vector<ProductEntry> productEntries;
    bool same(const ControlState&) const;   // fields by ==, product entries by name and Payload::equals
    sessions::Value toValue() const override;   // §3.6
};
}
```

Every entry is named as commands and presets name it: the model and
variant IDs, the place path as `placeText` writes it (`slot-1`,
`aux/jack`), the peripheral IDs, the media place IDs, the command names.

### 3.2 Asking the machine (AC-2; LIB-2 AC-6, AC-35)

`Machine::canAttach` answers for an empty place. A choice in a place
that holds a peripheral is a replace (LIB-18 AC-10, LIB-14 AC-3), and
`canAttach` refuses it because the place is full. LIB-18 adds one member
in M-35 (its §2, for its own AC-12), next to `replace`; the control state
uses it:

```cpp
// Machine.hpp, LIB-2 design §3.1: the answer replace() would give, without the change.
std::optional<Refusal> canReplace(const PeripheralId& old, const PeripheralRequest&, const Catalogue&) const;
```

It runs `replace`'s own checks (the place holds `old`, then
`check(req, cat, old)`), so the answer and the refusal are the same
(LIB-2 AC-6).

The request the control state asks with names the place, the
peripheral and the variant. Its ROM images carry the file names of the
variant's entries and no bytes, and it carries no content and no
switches. `check` compares ROM images by file name only (`Attach.cpp`,
step 5). With the full ROM set required at the start (LIB-12), no entry
is ever refused for a ROM (spec decision log). No ROM byte is copied 60
times a second (D-2).

### 3.3 A product's rules (AC-7, AC-18)

```cpp
struct ModelKey   { machine::ModelId id; };
struct VariantKey { machine::VariantId id; };                       // of the running model
struct PlaceKey   { machine::PlacePath path; };
struct ChoiceKey  { machine::PlacePath path;
                    std::optional<machine::PeripheralId> peripheral;   // nullopt: the empty place
                    std::optional<machine::VariantId> variant; };
struct InsertKey  { machine::MediaPlaceId id; machine::PlacePath ownerPlace; };
struct ActionKey  { sessions::CommandName command; };
using EntryKey = std::variant<ModelKey, VariantKey, PlaceKey, ChoiceKey, InsertKey, ActionKey>;

class ControlRules {                        // implemented by a product's app layer
public:
    // A reason of the product's own why the entry cannot be chosen now, or none.
    virtual std::optional<std::string> unavailable(const EntryKey&, const machine::Machine&) const { return std::nullopt; }
    // The product's own entries, appended in order; names carry the product's prefix.
    virtual void productEntries(const machine::Machine&, std::vector<ProductEntry>&) const {}
protected:
    ~ControlRules() = default;
};
```

- The library calls these and never reads what they give back: a
  product entry is a `Payload` like a product topic's (LIB-3 AC-12).
- A place the product refuses (`PlaceKey`) gives its reason to the
  place, to its empty choice and to every peripheral and variant in it,
  in the product's place of the order of AC-2 (D-5). That is how
  Calc-U-1600 refuses `host-drive` on the Web (AC-18).
- What depends on the frontend is the product's own setup. The
  Calc-U-1600 frontends tell `SharpAppPart` at construction whether they
  can mount a host folder (spec OQ-6). The library knows nothing of it,
  so AC-8 holds for everything else by construction.

### 3.4 The computation (AC-1 to AC-3, AC-5, EC-1, EC-3 to EC-5)

```cpp
struct ControlSetup {
    const machine::Catalogue* catalogue = nullptr;   // the product's (LIB-18's ProductMachines::catalogue()); read at each computation (EC-3)
    std::vector<machine::PeripheralId> printers;     // who can own the paper panel (D-3)
    std::vector<const ControlRules*> rules;          // asked in this order
};

ControlState computeControlState(const machine::Machine&, const ControlSetup&, bool operationRuns);
```

**Lists, in the order of the descriptors.**
- Models: the catalogue's, in its order; `model` is the running one.
- Variants: the running model's descriptor's; `variant` is
  `machine.variant()`, or the default. A model without variants lists
  none (the TI models, AC-10). The PC-1500A lists its one (AC-16).
- Places: a walk over the running machine's places in descriptor
  order. After a place that holds a peripheral with places of its own,
  those places follow at once, depth first (LIB-2 AC-9). A place is
  listed only if the machine or a plugged-in peripheral has it (AC-5).
  `fitting` is `Catalogue::place(model or peripheral, name)->fitting`:
  the pin-out rule of LIB-2 AC-2, also for a module definition added
  later (EC-3). What is plugged in comes from `machine.plugged().byPlace`
  (EC-1).
- Media places: the machine's, then those of each plugged-in
  peripheral, in the order of the place walk, with the peripheral as
  owner (EC-4: the CE-1600F's `floppy` is listed with the CE-1600P as
  owner, and no place for the CE-1600F exists). A media place on a
  model that lacks it is not listed (AC-5).
- Panels: one paper panel when `printers` is not empty and a printer
  of the list fits a place the running machine lists. Its owner is the
  first plugged-in printer of the place walk, or none (AC-13, LIB-8
  AC-12). A model on which no printer fits has none (AC-5, D-3).
- Actions: `reset`, `cold-boot`, `load-preset`, `open`, in this order,
  by their command names (the LIB-18, LIB-6 and LIB-13 drafts of
  2026-11-15).
  "Insert a medium" is the `insert` of each media place entry.
- Product entries: each rule's `productEntries`, in rule order.

**Availability, the order of AC-2 (EC-5).** For each entry the first
of these that applies gives the reason:

| # | Applies when | Entries | Reason |
|---|---|---|---|
| 1 | an operation runs (AC-3) | every entry but the models, `reset` and `cold-boot` (they end the operation, LIB-18 AC-16) | `sessions::kAnotherOperationRuns`, "Another operation is running." (D-6) |
| 2 | a rule says so (AC-7) | any entry; a refused place passes its reason down (§3.3) | the rule's sentence |
| 3 | the machine says no (LIB-2 AC-6) | a peripheral or variant choice that is not what is plugged in: `canAttach` for an empty place, `canReplace` for a full one (§3.2) | `Refusal::reason` |

Step 3 is asked only when 1 and 2 do not apply, so while an operation
runs no machine check runs. A detach (the plugged-in choice, the empty
choice of a full place) is never refused by the machine (LIB-2 EC-2).
The choice that is already in force (the running variant, the plugged
peripheral's running variant, the empty choice of an empty place)
follows the same rules (D-7).

**When an operation ends** the next flush computes from the machine as
it is then (AC-3): there is nothing cached between computations (D-1).

### 3.5 The part and the session (AC-4, AC-6, AC-8, AC-9)

```cpp
// SessionPart.hpp, added (LIB-3 §3.8): called at the start of every flush,
// after the parts' afterCall, on the session's thread.
virtual void beforeFlush(SessionContext&) {}

class ControlPart final : public sessions::SessionPart {
public:
    explicit ControlPart(ControlSetup);
    void attach(sessions::SessionSetup&) override;          // declares the state topic `control-state`
    void beforeFlush(sessions::SessionContext&) override;   // computes and sets (AC-4)
};
```

- `AppSession::Impl::flush()` calls each part's `beforeFlush` first.
  A flush ends every host call, command, request and service answer
  (LIB-3 §3.3), so AC-4 holds by construction. Whether the topic is sent
  is LIB-3's rule: only when it differs from what was last sent (LIB-3
  AC-5).
- No machine: nothing is set, and nothing is sent. A product that
  could not start for a missing ROM (LIB-12 AC-5) builds no machine,
  so no control state is ever sent (AC-6). A machine taken out while a
  new one is built leaves the last state sent, and the next one follows
  at the flush after it.
- `computeControlState` reads only its arguments. Two sessions of a
  product on machines in the same state send equal payloads (AC-8).
- The topic is a state topic like the others. A late listener gets it
  (LIB-3 AC-6), and so does the harness through `state control-state`
  (AC-9, LIB-11).
- Ordering: `ControlPart` comes among the library's parts, before the
  product's (LIB-3 §3.9). Every part's `afterCall` has run before any
  `beforeFlush`, so a product's state is up to date when its rules are
  asked.

### 3.6 `toValue` (the translators: bridge, Web glue, harness)

One object, keys in the order of §3.1: `models` (`[{id, available,
reason?}]`), `model`, `variants`, `variant`, `places` (`[{place:
"aux/jack", connector, pinOut, available, reason?, empty: {available,
reason?}, fitting: [{id, name, plugged, variant?, variants, available,
reason?}]}]`), `mediaPlaces` (`[{id, owner?: {place, peripheral},
kind, passage: "pass-through" | "holding", sides, insert: {…}}]`),
`panels` (`[{kind: "paper", owner?: {place, peripheral}}]`), `actions`
(`[{command, available, reason?}]`), `product` (`{name: value}` by each
payload's `toValue`). `reason` is present only when `available` is
false.

## 4. Product side (Parts D and E; sketch)

Today's code that LIB-4 replaces, and where each piece goes. The
product tasks are set out with LIB-14's and C59-8's designs at the
Part D and E boundaries.

**Calc-U-1600 (M-49, with LIB-14 1600).**

| Today | After M-49 |
|---|---|
| `products/calc-u-1600/app/ControlState.hpp/.cpp` (`ControlState`, `PeripheralState`, `controlStateOf`) | deleted; the generic topic `control-state` from a `ControlPart` with `SharpAppPart::catalogue()` and `printers = {CE-150, CE-1600P}` |
| `app/SharpTopics.hpp`: `ControlStatePayload`, topic `c1600.control-state` (LIB-3 1600's interim topic) | gone (adapter, §5) |
| `ce150/ce158/ce1600p.offered, attached, enabled` | the places `printer` and `interface`: fitting, plugged, available with the machine's reason (`SharpMachine::productRefusal`, `PC1600InterfaceMachine::modelRefusal`, LIB-2 AC-29) |
| `onControlBar` | goes (LIB-14 AC-9, spec AC-17) |
| `pc1500RomMenu`, `pc1600RomMenu`, `ce1600pRomMenu` | the variants of the model; the variants of the CE-1600P in `printer` |
| `floppyShown`, `floppyEnabled`, `tapeShown` | the media places `floppy` and `cassette` with their owner; `floppyShown` a product entry `c1600.disk-controls` (`app/SharpControlRules.hpp/.cpp`, new) |
| `hostDriveOffered`, `hostDriveDir` | the place `host-drive`, refused by `SharpControlRules` when the frontend cannot mount a folder (AC-18; today's `hostDirectories` flag of `controlStateOf`); the folder a product entry `c1600.host-drive-folder` |
| `paper` | the paper panel and its owner |
| `printerShown` | a product entry `c1600.ce158-printer-panel` |

The frontends draw from it: `frontends/qt6/app/MainWindow.cpp`,
`ControlBar.cpp`; `frontends/web/src/ControlBar.jsx`, `CalcU1600.jsx`,
`MenuBar.jsx`; `frontends/web/bindings.cpp` translates by `toValue`
(AC-19, AC-20). The CE-1600P variant chosen while it is not plugged in
(LIB-18 AC-31) stays a product setting (LIB-18 draft §4), so it is a
product entry `c1600.ce1600p-next-variant` (D-10).

**Calc-U-59 (M-57 Qt6 and Web; M-58 Swift).**

| Today | After |
|---|---|
| `products/calc-u-59/app/TIAppPart.hpp` topic `c59.model` (LIB-3 59's interim topic); `TIWebPart.hpp` `c59.web.model` | `control-state` from a `ControlPart` with `tiCatalogue()`, `printers = {PC-100C}` |
| Qt6 `frontends/qt6/app/ControlBar.cpp`, `MainWindow.cpp` ("Fit Card…" on every model) | drawn from the models, `slot-1` (14 modules and the empty slot, OQ-5), `printer` (button, LIB-14 AC-9), the media place `card-reader` (TI-59 only) |
| Web `frontends/web/src/ControlBar.jsx` | the same |
| Swift `frontends/swift/App/MachineModel.swift:16-33`, `App/Views/CalculatorView.swift` (M-58, C59-8) | gone; drawn from the control state (AC-15) |
| the debug panel's conditions (C59-4, M-56) | product entries of `app/TIControlRules.hpp/.cpp` (new; AC-14) |

## 5. Adapters

M-37 adds no adapter: no product code changes. The old control state
of each product stays behind the interim topics LIB-3's design names:

| Adapter | File | Wraps | Removed in |
|---|---|---|---|
| interim topic `c1600.control-state` | `products/calc-u-1600/app/SharpAppPart.*`, `SharpTopics.hpp` | `controlStateOf` (`app/ControlState.cpp`) | M-49 |
| interim topics `c59.model`, `c59.web.model` | `products/calc-u-59/app/TIAppPart.*`, `TIWebPart.*` | the model and module flags the frontends read | M-57 (Qt6, Web), M-58 (Swift) |

From M-37 to M-49 / M-58 both the interim topic and `control-state`
are sent in the harness session (§7), never in a frontend's.

## 6. Tasks

Library, M-37. Files in `library/control/tests/` unless named; program
`calcu_library_tests`; every case Frequent. The tests use the test
catalogue (`library/machine/tests/TestMachines.hpp`): `test-seg` (no
variant; places `slot-1`, `printer`, `aux`; media place `reader`,
pass-through, sides 0 and 1), `test-dot` (variants V1, V2; places `a`,
`b`, `card-1`, `card-2`; media place `drive`), `test-printer`,
`test-module` (variants 4k, 8k), `test-hub` (place `jack`; from M-37
the media place `hub-disk`), `test-uart`, and the rule "the test printer
and the test UART are not used together". The session tests use `library/testing/`'s
`TestFrontend.hpp` and `ScriptedClock.hpp` and LIB-3's
`sessions/tests/TestParts.hpp` (its command that starts an
operation). Tests come before the code.

| Task | What | ACs/ECs | Test (file: cases) |
|---|---|---|---|
| L1 | CMake, `ControlState`, `same`, `toValue` | (support) | `control_state_tests.cpp`: `control_state_value_round_trip`, `same_compares_product_entries_by_equals` |
| L2 | (none: `canReplace` and its test `can_replace_answers_as_replace_does` are M-35's, LIB-18 L5) | — | — |
| L3 | `beforeFlush` | (support) | `library/sessions/tests/topic_tests.cpp`: `before_flush_runs_at_every_flush` (host call, command, request, answer, `advanceUnpaced`, `listen`) |
| L4 | The lists | AC-1, EC-1 | `control_state_tests.cpp`: `ac1_models_and_running_model`, `ac1_variants_of_running_model`, `ac1_places_fitting_and_plugged_with_variant`, `ac1_nested_place_after_its_peripheral` (`test-hub` in `aux`, its `jack`), `ac1_empty_place_is_a_choice`, `ac1_media_places_owner_kind_passage_sides`, `ac1_paper_panel_owned_by_plugged_printer_or_none`, `ac1_actions_in_order`, `ac1_entries_in_descriptor_order`, `ec1_plugged_peripheral_says_plugged` |
| L5 | Lacking | AC-5 | `control_state_tests.cpp`: `ac5_place_media_place_panel_absent_where_model_lacks_them` (`test-seg` and `test-dot` against each other's places; a catalogue without printers has no panel) |
| L6 | Later definitions, peripheral media | EC-3, EC-4 | `control_state_tests.cpp`: `ec3_module_added_to_catalogue_listed_next_time`, `ec4_peripheral_media_place_owned_by_peripheral_no_place_for_it` (`test-hub` and `hub-disk`) |
| L7 | Availability and its order | AC-2, EC-5 | `availability_tests.cpp`: `ac2_machine_reason_for_peripheral` (printer and UART together), `ac2_full_place_asked_as_replace` (the second module variant in `slot-1` is available), `ac2_variant_of_plugged_peripheral`, `ac2_no_entry_refused_for_rom`, `ec5_operation_before_product_before_machine` |
| L8 | While an operation runs | AC-3 | `availability_tests.cpp`: `ac3_choices_and_actions_refused_while_operation_runs`, `ac3_models_reset_cold_boot_stay_available`, `ac3_recomputed_from_machine_after_operation` (the operation attaches a peripheral; afterwards the entry says plugged) |
| L9 | A product's rules | AC-7 | `availability_tests.cpp`: `ac7_product_entry_carried_unread`, `ac7_product_refuses_generic_entry_with_its_reason`, `ac7_refused_place_passes_reason_to_its_choices`; `TestControlRules.hpp` |
| L10 | The part | AC-4, AC-6 | `control_part_tests.cpp`: `ac4_computed_at_end_of_call_and_command`, `ac4_sent_only_when_it_differs` (a host call that changes nothing sends no control state), `ac6_no_machine_no_control_state` |
| L11 | Two frontends | AC-8 | `control_part_tests.cpp`: `ac8_two_sessions_same_machine_same_control_state` (two sessions, one script of commands; payloads `equals` and `toValue` equal after each step) |
| L12 | The harness | AC-9 | `library/harness/tests/commands_tests.cpp`: `ac9_state_control_state_reads_the_topic` (the harness test product; `state control-state` after `attach`) |

Coverage, Library: AC-1 to AC-9 (9 of 9), EC-1, EC-3, EC-4, EC-5 (4 of
4). Product tasks: §4, at the Part D and E boundaries.

## 7. Tools and agent knowledge

**Tools.** The harness gets the control state in M-37:
`library/harness/Run.cpp:179` adds a `ControlPart` to the library's
parts, with `product.catalogue()` and `HarnessProduct::controlSetup()`
(new, default: no printers and no rules). The products override it in
their column steps (M-49, M-57) to name their printers and rules. Until
then `state control-state` on a product shows the generic entries
without a panel and without product entries. `topics` lists
`control-state` from M-37. Today's tools (`tools/headless/`,
`pc1500_cli`, `pc1600_cli`, the probes) use no session and are
unaffected.

**Agent knowledge changed in the same step.**

| Step | Files |
|---|---|
| M-37 | `docs/test-groups.md` (the new cases of `calcu_library_tests`, by file); LIB-1 design §2a (the dependency line of `control`) |
| M-49 | `products/calc-u-1600/docs/developer/Shared-Architecture.md` (`ControlState`, `controlStateOf`); `docs/glossary-mapping.md` if it names the old fields |
| M-57, M-58 | `products/calc-u-59/reference/SharedArchitecture.md`, the `calc-u-59-qt`, `calc-u-59-web`, `calc-u-59-swift` and `calc-u-59-shared` skills where they name `c59.model` or `MachineModel.swift` |

`products/calc-u-1600/docs/background/*` (`Decisions.md`,
`Web-GUI-Plan.md`, `Preset-Review-Plan.md`) is history and stays.

## 8. Golden runs and risks

- **Golden runs** advance machines directly; the control state reads
  the machine and changes nothing. `canReplace` changes no state. The
  golden runs are not affected. A difference is a finding.
- **Cost per host call.** The state is computed at every flush, about
  60 times a second per window. The work is a walk over the places, and
  one `canAttach` or `canReplace` per fitting peripheral and variant
  (each calls `plugged()`). That is about 30 checks on a TI-59 and about
  10 plus two per module definition on a PC-1600. While an operation
  runs no check runs (§3.4). If the Frequent timing or a profile shows
  it, the part keeps a key (machine generation, `plugged()`, busy,
  catalogue size) and recomputes only when it changes. That is allowed
  only if the rules declare that they do not change between keys (D-1).
- **The Sharp printer place lists too much.** On Calc-U-1600 the places
  `printer`, `interface` and `host-drive` present one pin-out
  (`PC-1600 60-pin`, `core/Machine/SharpCatalogue.cpp`), so by LIB-2
  AC-2 each lists the CE-150, CE-158, CE-1600P and the host drive. The
  wrong ones are refused by `SharpMachine::productRefusal` ("The CE-158
  goes in the place interface."), but they are still listed. Ledger row
  1600-10 says the PC-1600's printer picker offers none, CE-150 and
  CE-1600P. The fix is catalogue data: the 60-pin connector carries one
  pin-out per place. That is a change to C1600-1's descriptors in M-49,
  checked by AC-17's test (O-2).
- **Commands named by other designs.** The action names are the
  session commands of the C2 drafts: LIB-18 `reset`, `cold-boot`; LIB-6
  `load-preset`; LIB-13 `open`. If an approved design renames one, the
  constant follows it.
- **`canReplace` and LIB-18.** LIB-18 adds `Machine::canReplace` in
  M-35 for its own refusal of a replace before the operation (LIB-18
  AC-12); M-37 uses it.

## 9. Decided (best guess)

- **D-1 (AC-3, AC-4).** Computed afresh at every flush, with no cache;
  LIB-3's topic compares and sends only a change. A cache is an
  optimisation for later (§8).
- **D-2 (AC-2; LIB-12).** The check request carries ROM file names and
  no bytes. The full ROM set is there before any machine is built, so
  no entry is refused for a ROM. A product rule that would need a
  peripheral's content gives its reason through `ControlRules`.
- **D-3 (AC-1, AC-5, AC-13; LIB-8 AC-12).** The paper panel's owners are
  the printers the product names in `ControlSetup::printers`. A
  descriptor does not say "printer", and LIB-8 (M-39) comes after M-37.
  The panel is listed on a model on which one of them fits a place, so
  "always" in a product with printers and "none on a model that lacks
  it" both hold. LIB-8 may later derive the list from its paper output
  kind; the payload stays the same.
- **D-4 (AC-1).** Every place has the empty choice. No descriptor says
  that a place must be filled, and no model of either product has such
  a place.
- **D-5 (AC-7, AC-18).** A place refused by the product passes its
  reason to all its choices, so a frontend need not combine two
  entries.
- **D-6 (AC-3).** The reason while an operation runs is LIB-3's
  sentence "Another operation is running.". It is the refusal the
  command would get, so the tooltip and the refusal read the same.
- **D-7 (AC-3; LIB-18 AC-8).** The choice already in force follows the
  same rules as the others. Choosing it again does nothing (LIB-18), but
  a frontend need not tell it apart.
- **D-8 (AC-1, OQ-1).** "Insert a medium" is an availability on each
  media place entry, because a medium goes into a place. "Load a preset"
  and "open a file" are two actions with the same rule, named by their
  commands.
- **D-9 (AC-9).** The harness session carries the control state from
  M-37 for every product, so an agent can read it before the product
  columns. The products name printers and rules in M-49 and M-57.
- **D-10 (LIB-18 AC-31).** Calc-U-1600's CE-1600P variant chosen while
  the CE-1600P is not plugged in stays a product setting (LIB-18 draft
  §4). The control state shows it as a product entry, and
  `PeripheralChoice::variant` of an unplugged peripheral is its default.
- **O-2 (open; C1600-1, ledger 1600-10).** The Sharp 60-pin places need
  one pin-out each, so that the printer place lists only printers
  (§8). A change to the descriptors in M-49.
- **O-3 (open; LIB-14).** How the shell tells a module slot from
  another place (LIB-14 AC-8, AC-9) is LIB-14's. The control state
  carries each place's connector and pin-out, and the media places of
  a slot (Sharp `slot-1`, `slot-2`), so that LIB-14 can decide.
