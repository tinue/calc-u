# LIB-18 Machine control and startup: design
Status: Library column approved at the Part C1/C2 boundary (2026-11-15); Calc-U-1600 column approved at the Part C2/D boundary (2026-11-16)

## 1. Scope

Part C2 works the Library column. The product columns are listed here
so that the seams fit them; they are designed in full by M-46 and M-54.

| Column | Step | Tests from (plan §3) | Done in (plan §5.1) | In this design |
|---|---|---|---|---|
| Library | M-35 | AC-1–16; EC-1–7 | M-35 (proposed: M-41, D-19) | designed |
| Calc-U-1600 | M-46 (with LIB-6 1600, C1600-4) | AC-25–33 | M-46 | listed (§4) |
| Calc-U-59 | M-54 (with LIB-6 59, C59-2, C59-3) | AC-17–24 | M-54 | listed (§4) |

M-35 waits for M-20 (LIB-9 L: `settings::startupModel`, `startupPreset`,
`family::startupLastModel`), M-23 (LIB-2 L: `Machine::attach`, `detach`,
`replace`, `AfterChange`, `PeripheralsPart`), M-24 (LIB-12 L: `RomSet`)
and M-28 (LIB-11 L: the command registry). It also uses M-22 (LIB-3 L:
operations, `endsRunningOperation`, `SessionPart::machineChanged`) and
M-21 (LIB-17: `PacingProfile` handed with each machine).

M-35 changes `library/` and the two harness product entries
(`products/calc-u-59/app/HarnessProduct.cpp`,
`products/calc-u-1600/app/HarnessProduct.cpp`), because the harness's
start becomes LIB-18's (LIB-11 AC-2; LIB-11 §5 A-1: "the library side
arrives in M-35"). No frontend and no core changes. The frontends keep
their own start, switches and resets as product commands (§5) until
M-46 and M-54.

Clauses of the Library ACs that name a feature built later (plan §1):

| Clause | Passes in | M-35 tests |
|---|---|---|
| AC-2 "what was saved as a medium stays saved (LIB-7)"; AC-7 "the same media in them" | M-41 (LIB-7 L) | the plugs only; media through the seam of §3.2 |
| AC-14 "the keys the typing engine holds" | M-36 (LIB-5 L) | a test `SliceHook` that holds text and keys (§3.4) |
| AC-4, AC-6, EC-1, EC-6 "loading a preset (LIB-6)" | M-38 (LIB-6 L) | the stand-in loader the spec allows |
| LIB-11 AC-2 "the machine the product starts with (LIB-18)" | M-35 | the harness start (§3.6) |
| LIB-11 exit check of M-28, "the PC-1600 follows in M-35" | M-35 | `harness_drives_a_pc1600` (§6) |

Not here: loading a preset (LIB-6), the "available now" of a model,
variant or peripheral (LIB-4), where the commands sit on screen
(LIB-14), what a reset or power cycle does in a core (C59-1, C1600-1),
media (LIB-7), the typing engine (LIB-5).

## 2. Modules and files

### Library (M-35): `library/startup/`, namespace `calcu::startup`

The folder LIB-1 §2a expects for LIB-18. It uses `machine`, `sessions`,
`settings`, `roms`, `pacing`. Its harness commands live in
`library/harness/`, as every C2 feature's do (LIB-5, LIB-6, LIB-8, LIB-10),
so `startup` does not include `harness`. Its tests are compiled into
`calcu_library_tests` (Frequent).

| File | Holds |
|---|---|
| `library/startup/MachinePlan.hpp` | `MachinePlan`, `BuildOrigin`, `BuildRefusal`, `PlugChange`, `BuiltWith` (§3.1, §3.4) |
| `library/startup/ProductMachines.hpp` | `ProductMachines`: what a product hands LIB-18 (§3.2) |
| `library/startup/PresetLoader.hpp` | `PresetLoader`, `PresetLoad`: the seam LIB-6 fills (§3.3) |
| `library/startup/MachineControl.hpp/.cpp` | `MachineControl`: build a plan into the session, change a plug, run to key readiness, carry plugs over, the last build (§3.4) |
| `library/startup/StartupPart.hpp/.cpp` | `StartupPart : sessions::SessionPart`, `StartupSetup`, the command names and handlers, the start (§3.5) |
| `library/harness/StartupCommands.cpp`, `Commands.hpp` (changed) | `addStartupCommands(CommandRegistry&)`: the commands of AC-15 (§3.6) |
| `library/machine/Machine.hpp`, `Attach.cpp` (changed) | `Machine::canReplace`: the answer `replace()` would give, without the change (AC-12 for a full place; LIB-4 uses it from M-37, its design §3.2) |
| `library/startup/tests/TestProductMachines.hpp` | `ProductMachines` over `machine::test::testCatalogue()`: counts builds, a settable readiness bound, built-in defaults per test model |
| `library/startup/tests/TestPresetLoader.hpp` | the stand-in for LIB-6's loader: builds a scripted plan through `MachineControl`, or fails reading, or fails after building |
| `library/startup/tests/TestHook.hpp` | a `SliceHook` that holds typed text and keys and records `drop()` |
| `library/startup/tests/*_tests.cpp` | one file per area (§6) |
| `library/sessions/MachineSession.hpp/.cpp` (changed) | `SliceHook::drop()` (default: nothing); `MachineSession::dropInput()` calls it on every hook (§3.4, D-12) |
| `library/sessions/AppSession.cpp` (changed) | `runCommand`: a command declared `endsRunningOperation` stops the running operation only when its handler returns `StartOperation` (D-6) |
| `library/machine/tests/TestMachines.hpp/.cpp` (changed) | build option `TestStartUp{Ticks}` (handed as `BuildRequest::content`; `kNever` = never key ready, EC-7); a build counter on `TestBuilder` |
| `library/harness/Product.hpp` (changed) | `startupRequest()` and `startUp()` (LIB-11 A-1) go; `virtual startup::ProductMachines& machines() = 0` comes |
| `library/harness/Run.hpp/.cpp` (changed) | `Server::start` builds no machine itself: it adds a `StartupPart` after `PeripheralsPart` and sends `start`; `RunSetup::settings()` gets a non-const overload (AC-9 writes `last-model`); `addStartupCommands` with the first commands |
| `library/harness/tests/TestProduct.hpp/.cpp` (changed) | implements `machines()` with the test machines; `startModel` becomes the `fallbackModel` of its declaration ("under the default settings", LIB-11 AC-2) |
| `library/CMakeLists.txt` (changed) | the sources of `library/startup/` and its tests |

### Products (M-35): the harness entries only

| File | Change |
|---|---|
| `products/calc-u-59/app/HarnessProduct.cpp` | `startupRequest()` goes; `HarnessMachines` (adapter, §5) implements `ProductMachines` over `tiCatalogue()`, `TIMachineBuilder`, `cu59::pacingProfile(Model)`, the 25 s bound |
| `products/calc-u-1600/app/HarnessProduct.cpp` | `startupRequest()` and `startUp()` go; `HarnessMachines` over `sharp::sharpCatalogue()` with the module definitions, `sharp::SharpMachineBuilder`, `shared::pacingProfile(Model)`, `sharp::kSoundRate`, the 7 s / 7 s / 27 s bounds; `ProductMachines::startUp` makes the first cold boot (adapter `SharpBuildColdBoot`, §5) |
| `products/calc-u-59/tests/harness_tests.cpp` | `harness_switches_to_a_ti58` (§6) |
| `products/calc-u-1600/tests/harness_tests.cpp` | `harness_drives_a_pc1600` (§6) |

## 3. Seams and interfaces

Signature sketches. Every class is single-threaded and lives on the
session's thread; nothing throws across the interface (LIB-1 §2a; ADR
0009).

### 3.1 Machine plan (AC-1, AC-4, AC-6, AC-7)

```cpp
namespace calcu::startup {

// Preset: a user's load (LIB-6 `load-preset`). StartupPreset: a load through
// PresetLoader::load, at the start or a model switch (LIB-6 D-24; LIB-8 AC-8).
enum class BuildOrigin { Start, ModelSwitch, VariantSwitch, Preset, StartupPreset };

// A build or change that is refused (AC-12, LIB-1 EC-1). `place`: the place
// whose request was refused, none for the model itself (LIB-6 AC-52).
struct BuildRefusal { std::string reason; std::optional<machine::PlacePath> place; };

// What one machine is built with, in one step (AC-1). ROM images are not
// in it: MachineControl takes them from the RomSet (LIB-12).
struct MachinePlan {
    machine::ModelId model;
    std::optional<machine::VariantId> variant;            // nullopt: the default
    std::vector<machine::PeripheralRequest> peripherals;   // `roms` left empty; filled by MachineControl
    std::shared_ptr<const machine::ProductData> content;   // e.g. the TI-58C's constant memory (AC-19)
};

}  // namespace calcu::startup
```

### 3.2 Product machines (AC-3, AC-4, AC-7, EC-7)

```cpp
class ProductMachines {   // implemented by the product's app layer; in M-35 by the harness entries
public:
    virtual const machine::Catalogue& catalogue() const = 0;
    virtual machine::MachineBuilder& builder() = 0;
    // ADR 0021: the machine of `model` when no startup preset is set or it
    // cannot be loaded (AC-4, EC-1; AC-18, AC-19, AC-26 in the products).
    virtual MachinePlan builtInDefault(const machine::ModelId&) = 0;
    virtual pacing::PacingProfile pacingProfile(const machine::ModelId&) const = 0;   // LIB-17 AC-14
    virtual int soundRate(const machine::ModelId&) const = 0;                          // BuildRequest::soundRate
    // EC-7: the emulated time after which a wait for key readiness ends.
    virtual machine::Seconds readinessBound(const machine::ModelId&) const = 0;
    // AC-7: the request that plugs `entry` into the machine of the other
    // variant. Default: its place, peripheral and variant. A product adds
    // the content and switch positions it keeps; LIB-7 adds the media (M-41).
    virtual machine::PeripheralRequest carried(const machine::PlugEntry&, machine::Machine& old);
    // Adapter SharpBuildColdBoot (§5): what the product does with a machine
    // it has just built. Nothing by default (D-3).
    virtual void startUp(machine::Machine&) {}
protected:
    ~ProductMachines() = default;
};
```

The fallback model, `last` and the startup preset's key are LIB-9's
(`settings::startupModel`, `startupPreset`, the product's
`ProductDecl::fallbackModel`). LIB-18 does not repeat them.

### 3.3 Preset loader (AC-4, AC-6, EC-1, EC-2, EC-6)

```cpp
struct PresetLoad {
    enum class Outcome { Built, CannotLoad, FailedWhileRunning };
    Outcome outcome = Outcome::Built;
    std::string reason;   // an English sentence (CannotLoad, FailedWhileRunning)
};

class PresetLoader {   // LIB-6 (M-38); TestPresetLoader in M-35
public:
    // Reads the preset `ref` (a bundled path or the [preset] value). On a
    // read error, an outdated file or a preset for another model than
    // `forModel` (when given), or a build that is refused: CannotLoad, nothing
    // built (EC-1). Otherwise it builds the preset's machine through
    // control.build(plan, StartupPreset), from a cold boot (ADR 0016), and runs
    // its steps; a failure from then on is FailedWhileRunning and leaves the
    // machine as far as it got (EC-6).
    virtual sessions::Task<PresetLoad> load(std::string ref, std::optional<machine::ModelId> forModel,
                                            MachineControl& control, sessions::OperationContext&) = 0;
protected:
    ~PresetLoader() = default;
};
```

LIB-6's own preset command (a preset that names another model, AC-24)
calls the same `MachineControl::build`, so AC-9 holds for it too.

### 3.4 Machine control (AC-1, AC-2, AC-7, AC-10–AC-14, EC-3–EC-5, EC-7)

```cpp
struct PlugChange {                       // AC-10–AC-12; LIB-7's module media (its §3.10)
    enum class Kind { Attach, Replace, Detach } kind;
    machine::PlacePath place;
    machine::PeripheralRequest request;   // Attach, Replace: full, content included; `roms` filled by MachineControl
};
struct BuiltWith { MachinePlan plan; BuildOrigin origin; };   // the last build (LIB-8 A-3)

class MachineControl {
public:
    // AC-1, AC-2: fills the ROM images from the RomSet, builds through
    // machine::build(catalogue, builder, request), calls startUp(), and
    // hands the machine to the machine session with the model's profile and
    // sound rate; the old machine is destroyed with its memory. Before
    // that: MachineSession::dropInput() (AC-2, EC-4) and the commanded speed
    // back to regular (AC-2, D-11). A build that is refused changes nothing
    // and gives the refusal (missing ROMs named, LIB-1 EC-1; the place, LIB-6 AC-52).
    std::optional<BuildRefusal> build(sessions::SessionContext&, MachinePlan, BuildOrigin);
    // The plan and origin of the last build that succeeded; read by parts in
    // machineChanged (LIB-8: a user's load empties the papers it names).
    const std::optional<BuiltWith>& lastBuild() const;
    // AC-10: the request for a peripheral by place, ID and variant, from the
    // catalogue (no content: the peripheral's default). The `attach` command uses it.
    machine::PeripheralRequest requestFor(const machine::PlacePath&, const machine::PeripheralId&,
                                          std::optional<machine::VariantId>) const;
    // AC-12: the refusal the change would get, without it (canAttach,
    // Machine::canReplace); asked before an operation starts.
    std::optional<BuildRefusal> check(const machine::Machine&, const PlugChange&) const;
    // AC-10, AC-11, EC-5: the change and what follows (§3.4 below), inside an operation.
    sessions::Task<void> change(sessions::OperationContext&, PlugChange);
    // AC-1, AC-10, AC-13, EC-3, EC-7: runs one run piece (OperationContext::
    // runPiece, 1/60 s) after another until the machine reports key
    // readiness, or until readinessBound(model) of emulated time has run;
    // then one log line, and the machine is left as it is (D-4, D-5).
    sessions::Task<bool> untilKeyReady(sessions::OperationContext&);
    // AC-7: the plan of the running machine in another variant: carried()
    // for each plug of Machine::plugged() in place order; no content.
    MachinePlan variantPlan(machine::Machine&, machine::VariantId);
};
```

**Added by later C2 steps** (each in the step that first needs it; the
consolidated list of LIB-3 additions is in §8):
- M-40 (LIB-10 §3.11): after `Machine::reset()` and `coldBoot()`, before
  `untilKeyReady`, `MachineControl` calls `SessionPart::machineRestarted`
  on every part.
- M-41 (LIB-7 §3.12, §3.13): `build` and `change` call
  `SessionPart::beforeMachineChange` on every part before the machine or
  a plug changes; `MachineControl::addCarry(fn)` lets a part add to the
  request of a plug that `variantPlan` carries over (a module medium's
  content). Media in media places are put back by LIB-7's part at
  `machineChanged`.

**Input (AC-2, AC-14, EC-4).** `SliceHook` gets
`virtual void drop() {}`: drop the text still waiting and release the
keys held. `MachineSession::dropInput()` calls it on every hook. LIB-18
calls it before each build, Reset, Cold boot and power cycle. LIB-5's
typing engine implements it (M-36). Of the product hooks of today,
`SharpTypingHook` and the key part of `TIKeySequencerHook` leave in M-36.
The KEYSTROKES playback of format 1 stays until M-54 as LIB-5's adapter
`TIKeystrokePlayback` (LIB-5 §5, D-2), with the default `drop()`: its
cancel on a reset is Calc-U-59's own flow until M-54, which removes it in
the same step that puts `StartupPart` into Calc-U-59's sessions.

**What follows a change (AC-10–AC-12, EC-5; LIB-2 A-3).** One
operation, in this order: `check` (`canAttach`, or `Machine::canReplace`
for a full place; or the place is empty) → refused at once, before the
operation starts, nothing changed (AC-12); `attach`, `replace` or `detach` →
`ChangeResult::follows`; on `PowerCycle`: `dropInput()`,
`Machine::powerCycle()`, `untilKeyReady`; on `Nothing`: done (the
switched-off machine of AC-33; `followsNow`). `Machine::detach` asks
what follows once for a chain (EC-5). The capability pointers are
re-read after the change (LIB-1 §3.5); `PeripheralsPart` rebinds the
host links on `machineChanged`, which the session sends for a new
machine only, so `StartupPart` calls `PeripheralsPart` nothing: an
attach on the same machine is seen by the host links' own pump (LIB-2
AC-20).

### 3.5 Startup part, commands, the start (AC-3–AC-16)

```cpp
struct StartupSetup {
    ProductMachines& product;
    std::shared_ptr<const roms::RomSet> roms;   // the full set (LIB-12); the caller does not start without it
    settings::SettingsFile& settings;           // [startup] model, last-model; [preset]
    PresetLoader* loader = nullptr;             // nullptr until LIB-6 (D-9)
};

class StartupPart final : public sessions::SessionPart {
public:
    explicit StartupPart(StartupSetup);
    MachineControl& control();
    void attach(sessions::SessionSetup&) override;              // the commands below
    void machineChanged(sessions::SessionContext&) override;   // AC-9: [startup] last-model
};
```

Session commands (library names, no prefix; LIB-3 §3.8):

| Command | Args | Ends a running operation | Does |
|---|---|---|---|
| `start` | `{model?, preset?}` | no; accepted once (D-8) | AC-3–AC-5 below |
| `model` | `{name}` | yes (AC-16) | AC-6, AC-8 |
| `variant` | `{name}` | no | AC-7, AC-8 |
| `attach` | `{place, peripheral, variant?}` | no | AC-10, AC-11 |
| `detach` | `{place}` | no | AC-10, AC-11, EC-5 |
| `reset` | — | yes (AC-16) | AC-13, AC-14 |
| `cold-boot` | — | yes (AC-16) | AC-13, AC-14 |
| `power-cycle` | — | no | AC-13, AC-14 |

Every handler returns `StartOperation`, `Done` (nothing to do: AC-8, an
empty place, the plugged peripheral again) or `Refused`. A handler of a
command that does not end a running operation is refused while one runs,
with LIB-3's "Another operation is running." (AC-16), also where it
would be `Done` (D-7). Names of models and variants are compared in any
case, as presets write them (`docs/formats/preset.md`; D-16).

**The start (`start`).** One operation (AC-5):
1. The model: `args.model` if it names a model of the catalogue; an
   unknown one gives a notice and is ignored (LIB-15 EC-4); else
   `settings::startupModel(settings)`, which gives `[startup] model`,
   `last` → `last-model`, or the fallback model (AC-3; LIB-9 AC-9, EC-1).
2. The machine, built once (AC-5): with `args.preset`, `loader->load(preset)`;
   else with `startupPreset(settings, model)` not empty and a loader,
   `loader->load(that, model)`; else `build(builtInDefault(model), Start)`
   (AC-4). `CannotLoad` → `build(builtInDefault(model))` and a notice
   (EC-1); `FailedWhileRunning` → the machine stays, a notice (EC-6).
3. `untilKeyReady` (AC-1, AC-5). The busy topic says
   `operationRuns` from the command to here (LIB-3 AC-14).

`[startup] model = last` takes the model built last whoever built it,
then its startup preset or built-in default (EC-2): `last-model` holds a
model, never a preset.

**Model switch (`model`).** The same model → `Done` (AC-8). Otherwise
step 2 and 3 of the start for that model, with no `args.preset`
(AC-6; `machine.md` I-6). An unknown name: "There is no model
"<name>"." (D-18).

**Variant switch (`variant`).** The running variant → `Done`; not a
variant of the running model → refused (D-18); else
`build(variantPlan(machine, v), VariantSwitch)` and `untilKeyReady`: the
same plugs in the same places, own memory empty, no startup preset
(AC-7).

**Attach, detach (`attach`, `detach`).** §3.4: the handler makes the
`PlugChange` with `requestFor`, asks `check`, then starts an operation
that runs `change`. `attach` to a place
whose `takesAtOnce` is 1 and that holds a peripheral is a `replace` of
it (AC-10; LIB-2 AC-35); the same peripheral and variant again is
`Done`. `detach` of an empty place is `Done` (LIB-2 EC-2; D-14).

**Reset, Cold boot, power cycle.** `dropInput()`, then
`Machine::reset()`, `coldBoot()` or `powerCycle()`, then `untilKeyReady`
(AC-13, AC-14). What is plugged in stays (LIB-2 AC-10). The real-time
clocks get the host's time at the end through `PeripheralsPart::
operationEnded` (AC-13; LIB-2 AC-23), and at a build through
`PeripheralsPart::machineChanged` (AC-1). Both need `PeripheralsPart`
before `StartupPart` in `AppSessionSetup::parts`.

**Last used model (AC-9).** `machineChanged` writes
`[startup] last-model` with `SettingsFile::set` whenever the session has
a new machine, whoever built it (a command, a preset, the start). A file
that is not writable keeps it until the product quits (LIB-9; D-10).

**While an operation runs (AC-16).** `model`, `reset` and `cold-boot`
are declared `endsRunningOperation`. With the change of §2 (D-6) the
session first runs the handler and stops the running operation only
when the handler starts one: the stopped command gets "Stopped." (LIB-3
D-7), then the new operation starts. A `model` to the running model is
`Done` and stops nothing.

### 3.6 Harness (AC-15; LIB-11 AC-2)

`addStartupCommands` adds to the registry, at the top level, each sent
as the session command of §3.5 with the reply of its operation (LIB-11
AC-7: run in one go):

| Harness | Session command |
|---|---|
| `model <name>` | `model {name}` |
| `variant <name>` | `variant {name}` |
| `attach <place> <peripheral>[:<variant>]` | `attach {place, peripheral, variant}`; the place as `placeText` writes it (`a/jack`); the variant as in a preset (`CE-1600P:old`) |
| `detach <place>` | `detach {place}` |
| `reset`, `cold-boot`, `power-cycle` | the same names |

`Server::start` (`library/harness/Run.cpp:145-187`) no longer calls
`startupRequest`, `machine::build` and `startUp`. After the lookup
(LIB-12 EC-1) it makes `StartupPart{product.machines(), roms.set,
setup.settings()}`, puts it after `PeripheralsPart` and `MachineStatePart`,
builds the session and sends `start` through the host. A start that is
not done ends the run with its reason (LIB-11 AC-2). The harness has no
loader until LIB-6 (M-38) gives one, and its settings are the run's own
(LIB-9 AC-18), so `[preset]` is empty there.

## 4. Product side (later steps)

Nothing of a product's frontends or app session moves in M-35. The
product columns replace these, and each adds `StartupPart` to its app
session with the product's `ProductMachines` and LIB-6's loader in the
same step:

| Today | Replaced in | By |
|---|---|---|
| Calc-U-1600 `SharpAppPart` commands `start`, `c1600.pick-model`, `c1600.reset {all}`, `c1600.pc1500-rom`, `c1600.pc1600-rom`, `c1600.ce1600p-rom`, `c1600.select-module`, `c1600.toggle-peripheral`, `c1600.mount-host-drive`, `c1600.default-preset`, `c1600.load-default-preset` and their jobs (`startJob`, `pickModelJob`, `resetJob`, `switchModelJob`, `powerCycleAroundJob`, `resetToPromptJob`, `select*RomJob`, `selectModuleJob`, `togglePeripheralJob`; `app/SharpAppPart.cpp`) | M-46 | `start`, `model`, `variant`, `attach`/`detach` (a module choice and the CE-1600P ROM as `replace`, AC-29, AC-31), `reset`, `cold-boot`; a `ProductMachines` in `app/` (AC-25, AC-26); `LegacyCalls::runToPromptJob` → `untilKeyReady` (C1600-1 §5); the CE-1600P choice while unplugged (AC-31) stays a product setting of the run |
| Calc-U-1600 harness entry's `HarnessMachines` | M-46 (the app's `ProductMachines`); M-51 the entry builds through `app/SharpAppPart` (LIB-11 D-6, plan M-51) | |
| Calc-U-59 `TIAppPart` commands `c59.start-machine`, `c59.reset`, `c59.clean-reset`, `c59.change-module`, `c59.printer-connected`, `c59.toggle-printer-connection` and `TIWebPart`'s `start`; the flows `startMachine`, `resetMachine`, `cleanResetMachine`, `changePlaces`, `setPrinterConnected` (`app/TIAppPartFlows.cpp:31-356`) | M-54 | the library commands; the TI `ProductMachines` gives the built-in default (Master Library in `slot-1`, no printer: AC-18) and the constant memory as `MachinePlan::content` for a TI-58C built without a preset (AC-19, C59-3); what a reset keeps beside the machine (AC-23) stays `TIAppPart`'s, on `machineChanged` and `operationEnded` |
| Calc-U-59 harness entry's `HarnessMachines` | M-54 | the app's `ProductMachines`; the entry's other duties go in M-59 |

LIB-15's N3 says `start` belongs to the product parts "before M-35".
With this design they keep it until M-46 and M-54, where their own
`start` goes in the same step that adds `StartupPart`, so no session has
two parts that declare one name.

## 5. Adapters

| Adapter | File | Wraps | Removed in |
|---|---|---|---|
| `HarnessMachines` (successor of LIB-11's A-1) | `products/calc-u-59/app/HarnessProduct.cpp`, `products/calc-u-1600/app/HarnessProduct.cpp` | the harness's built-in default per model as A-1 gives it today: Calc-U-59 the model with the PC-100C (TRACE off) and no module (`MachinePeripherals` of 2.3.0's core); Calc-U-1600 the bare model in its default variant. Profiles, sound rates and bounds as §2 | M-46 (1600), M-54 (59), when the app layer's `ProductMachines` takes over |
| `SharpBuildColdBoot` | `ProductMachines::startUp` in the Calc-U-1600 entry | the first cold boot that `SharpMachineBuilder` does not make (LIB-11 qa-report, deviations; today `HarnessProduct::startUp`) | M-46: `SharpMachineBuilder::build` leaves its machine as a cold boot does (LIB-1 AC-42), and `startUp` leaves `ProductMachines` |
| the products' machine-control commands | `app/SharpAppPart.*`, `app/TIAppPart*.cpp`, `app/TIWebPart.*` | §4's table (LIB-3 §5: "Sharp operations as product commands") | M-46, M-54 |

M-35 removes LIB-11's A-1 (`HarnessProduct::startupRequest`,
`HarnessProduct::startUp`) from the library interface; its product
halves become `HarnessMachines` and `SharpBuildColdBoot` above.

## 6. Tasks

Tests come before the code. Library cases are in `library/startup/tests/`,
in `calcu_library_tests`, group Frequent, named `acN_`/`ecN_` (LIB-1
§2a). They use `test-seg` (memory: its key presses, kept by reset and
power cycle, cleared by a cold boot; places `slot-1`, `printer`, `aux`)
and `test-dot` (variants V1, V2; switched off by `off`, then a change is
followed by nothing; `chain`, `card`, `rtc`), with `TestStartUp`,
`TestProductMachines`, `TestPresetLoader`, `TestHook`, `library/testing/`'s
`TestFrontend` and `ScriptedClock`, and the settings product of
`library/settings/tests/TestProduct.hpp`. Paced and virtual-time hosting
both, where the operation matters (LIB-3 AC-19).

| Task | What | ACs/ECs | Test (file: cases) |
|---|---|---|---|
| L1 | Test support, `TestStartUp`, build counter, CMake | (support) | `build_tests.cpp`: `test_product_machines_build_each_test_model` |
| L2 | `MachineControl::build` | AC-1, AC-2, EC-4 | `build_tests.cpp`: `ac1_one_build_with_model_variant_and_plugs`, `ac1_no_power_cycle_for_what_it_was_built_with` (`operationsTold` 0), `ac1_clock_given_host_time_at_build` (`rtc` `setCalls`), `ac1_ready_when_key_ready`, `ac1_refused_build_changes_nothing`, `refusal_names_the_refused_place` (LIB-6 AC-52), `last_build_gives_plan_and_origin`, `ac2_old_machine_and_its_memory_gone`, `ac2_full_speed_and_held_keys_do_not_carry_over`, `ec4_text_being_typed_dropped_with_the_old_machine` |
| L3 | The start | AC-3, AC-4, AC-5, EC-1, EC-2, EC-6 | `start_tests.cpp`: `ac3_startup_model_named`, `ac3_last_gives_last_model`, `ac3_nothing_recorded_gives_fallback`, `ac4_startup_preset_loaded`, `ac4_no_startup_preset_builtin_default`, `ac4_no_loader_builtin_default`, `ac5_start_is_one_operation_busy_until_ready`, `ac5_one_build_when_model_named`, `ac5_one_build_when_preset_named`, `ac5_unknown_model_notice_and_startup_model`, `ac5_second_start_refused`, `ec1_preset_gone_outdated_or_other_model_gives_default_and_notice`, `ec2_last_built_by_preset_uses_its_startup_preset`, `ec6_preset_fails_while_running_machine_stays_notice` |
| L4 | Model and variant | AC-6, AC-7, AC-8, AC-9 | `switch_tests.cpp`: `ac6_model_switch_from_startup_preset`, `ac6_model_switch_builtin_default`, `ac7_variant_switch_keeps_plugs_in_places`, `ac7_variant_switch_memory_empty_no_preset`, `ac8_same_model_done_nothing_built`, `ac8_same_variant_done`, `ac9_last_model_after_command_preset_and_start`, `ac9_unwritable_settings_keep_it_for_the_run`, `variant_not_of_the_model_refused`, `unknown_model_refused` |
| L5 | Attach, detach, replace | AC-10, AC-11, AC-12, EC-5 | `change_tests.cpp`: `ac10_attach_is_one_operation_over_at_key_readiness`, `ac10_attach_named_and_default_variant`, `ac10_detach_place`, `ac10_attach_to_full_single_place_is_one_replace_one_power_cycle`, `ac10_switched_off_machine_nothing_follows` (test-dot after `off`), `ac11_memory_through_power_cycle_and_new_plug_found` (presses kept; the new plug told one operation), `ac12_refusal_changes_nothing_no_power_cycle_reason` (`kTogether`), `ac12_full_place_refused_by_can_replace_before_the_operation`; `library/machine/tests/attach_tests.cpp`: `can_replace_answers_as_replace_does`; `ec5_detach_chain_one_power_cycle`, `detach_empty_place_done` |
| L6 | Reset, Cold boot, power cycle; readiness | AC-13, AC-14, EC-3, EC-7 | `reset_tests.cpp`: `ac13_reset_cold_boot_power_cycle_are_operations_over_at_key_readiness`, `ac13_plugs_stay`, `ac13_clock_given_host_time_at_end`, `ac14_text_and_held_keys_dropped` (`TestHook`), `ec3_ready_at_a_prompt_ends_the_wait`, `ec7_never_ready_ends_after_bound_with_log_line_machine_left` (bound 0.01 s on test-seg; checked in ticks) |
| L7 | While an operation runs | AC-16 | `running_tests.cpp`: `ac16_reset_cold_boot_model_stop_running_operation_first`, `ac16_stopped_command_replied_stopped`, `ac16_other_commands_refused_while_running` (`variant`, `attach`, `detach`, `power-cycle`, `start`), `ac16_model_to_running_model_leaves_operation_running`; `library/sessions/tests/operation_tests.cpp`: `ac16_ending_command_that_starts_nothing_stops_nothing` (D-6) |
| L8 | Harness commands, harness start | AC-15; LIB-11 AC-2 | `library/harness/tests/startup_commands_tests.cpp` (harness `TestProduct`): `ac15_each_command_at_top_level_with_its_operations_reply`, `ac15_attach_variant_as_in_a_preset`, `ac15_attach_place_path`, `ac15_refusal_exits_nonzero` (one-shot); `library/harness/tests/run_tests.cpp`: `ac2_run_starts_with_the_startup_model_under_default_settings` |
| P59 | Calc-U-59 harness entry | (LIB-11 AC-2 through a product) | `products/calc-u-59/tests/harness_tests.cpp`: `harness_switches_to_a_ti58` (`model TI-58 then lcd then capabilities`: done, the TI-58 named); `SKIP_UNLESS` the ROMs; Frequent |
| P16 | Calc-U-1600 harness entry | (LIB-11 exit check, D-1) | `products/calc-u-1600/tests/harness_tests.cpp`: `harness_drives_a_pc1600` (`model PC-1600 then lcd then capabilities`: done, the PC-1600 named, the display read with its font); `harness_variant_switch_old` (`model PC-1600 then variant old`); `SKIP_UNLESS` the ROMs; Frequent |

Coverage, Library: AC-1 to AC-16 (16 of 16) and EC-1 to EC-7 (7 of 7).
The product columns' AC-17 to AC-33 are M-46's and M-54's.

LIB-11's existing harness tests that read `ticks` or the display right
after the start now see the machine at key readiness (§8): their
expected values change in M-35 with this note in LIB-11's qa-report.

## 7. Tools and agent knowledge

**Today's tools.** `tools/headless/`, `tools/agentic/`, `pc1500_cli`,
`pc1600_cli` and the probes build their machines directly and are
unaffected (plan §1). `calcu` gains the commands of AC-15.

**Agent knowledge in M-35.**

| File | Change |
|---|---|
| `docs/test-groups.md` | the `startup/tests/` files and counts under `calcu_library_tests`; the two new harness cases |
| `features/LIB-11-harness/qa-report.md` | AC-2's clause "the machine the product starts with (LIB-18)" passes; adapter A-1 replaced (§5); the PC-1600 of the M-28 exit check |
| the harness section for agents in both products' guidance (LIB-11, M-28) | the seven commands |

No rename: "Reset" and "Cold boot" are already the glossary's; the
renames of "clean reset" and "Reset All" (`docs/glossary-mapping.md`)
happen with the product commands in M-46 and M-54.

## 8. Golden runs and risks

- **Golden runs** are not touched: M-35 changes no core and no
  frontend. The harness entries build through the same builders.
- **Harness start moves by the start-up.** A run now begins at key
  readiness, not at tick 0 after the build (LIB-11 AC-2 as specified).
  Expected `ticks` and display values of LIB-11 tests on the test
  machines move by the 50-tick start-up; `harness_drives_a_sharp_machine`
  still reads `NEW0? :CHECK` after `wait 2s`.
- **Readiness at a piece boundary** (D-4). The wait ends at the first
  run piece (1/60 s) after readiness, not at the tick. In the product
  columns the tick at which a boot ends may move against today's fixed
  waits (300,000 instructions; 2 s plus up to 5 or 20; spec EC-7). That
  is M-46's and M-54's golden question (C1600-1 §8 says the same of
  `runToPromptJob`).
- **LIB-3 dispatch change** (D-6): a command declared
  `endsRunningOperation` that is `Refused` or `Done` no longer stops
  what runs. Today only `SharpAppPart`'s `start`, `c1600.reset` and
  `c1600.pick-model` are declared so; each returns `StartOperation` in
  every path but its refusals, so Calc-U-1600 keeps its behaviour.
- **LIB-3 additions of Part C2, consolidated.** LIB-3 is Done (M-22);
  Part C2 adds to its sessions area, each addition in the earliest step
  that needs it, with an empty default where it is a hook, so no product
  part changes. The other C2 designs refer to this list:

  | Addition | Step (design) |
  |---|---|
  | `SliceHook::drop()`, `MachineSession::dropInput()` | M-35 (LIB-18 §3.4) |
  | `endsRunningOperation` stops a running operation only when the handler starts one | M-35 (LIB-18 D-6) |
  | `CommandOutcome::Deferred`, `SessionContext::currentCall()`, `SessionContext::reply(CallId, …)`; `HarnessHost::command` advancing until a deferred reply, at most one hour of emulated time | M-36 (LIB-5 §3.5) |
  | `SessionPart::beforeFlush` | M-37 (LIB-4 §3.5) |
  | `OperationContext::hostWait` | M-38 (LIB-6 D-17) |
  | `AppSession::advanceUnpaced` calls the parts' `afterCall` | M-39 (LIB-8 A-6) |
  | `SessionPart::machineRestarted(SessionContext&, Restart)` | M-40 (LIB-10 §3.11) |
  | `SessionPart::beforeOperation`, `beforeMachineChange`, `closing` | M-41 (LIB-7 §3.12) |
  | `OperationResult::result`, `SessionContext::startOperation` | M-42 (LIB-13 §3.6) |
- **One name, two parts.** A product that adds `StartupPart` while its
  own part still declares `start` would declare one command twice. §4
  removes the product's command in the same step.
- **Settings written by the harness.** The harness's run folder has its
  own INI file; writing `last-model` there touches nothing of the user's
  (LIB-9 AC-18).

## 9. Decided (best guess)

- **D-1.** `library/startup/`, `calcu::startup`, as LIB-1 §2a expects.
- **D-2.** The session commands carry the harness names of AC-15
  (`model`, `variant`, `attach`, `detach`, `reset`, `cold-boot`,
  `power-cycle`) plus `start` (LIB-15 N3), without a prefix (LIB-3
  §3.8). LIB-14 binds its menu and control bar to these names.
- **D-3.** A built machine is in the state of a cold boot; LIB-18 makes
  no call after `machine::build` (LIB-1 AC-42; the TI builder already
  does it, `core/TIMachine.cpp:14-21`). The Sharp gap stays behind
  `SharpBuildColdBoot` until M-46.
- **D-4.** Key readiness is checked after each run piece (1/60 s,
  LIB-3 `kRunPiece`); no finer wait.
- **D-5.** EC-7: the operation ends as done, with one log line ("The
  <model> was not ready for keys after <n> seconds; it is left as it
  is."), no notice. The bound is product data (`readinessBound`), counted
  in emulated time (OQ-8).
- **D-6.** LIB-3 change: `endsRunningOperation` stops the running
  operation only when the handler starts one. AC-8 ("nothing happens")
  and a refused model switch then leave a running preset alone.
- **D-7.** While an operation runs, `variant`, `attach`, `detach`,
  `power-cycle` and `start` are refused even where they would be `Done`
  (AC-16 is the narrower rule).
- **D-8.** `start` is accepted once per app session; a second is
  refused ("The product has started already."). It does not end a
  running operation (AC-16 does not name it). Today `SharpAppPart`'s
  `start` ends one; nothing sends it twice.
- **D-9.** Without a loader (before LIB-6 and in the products before
  their columns) `[preset]` is not read; the built-in default is built
  without a notice.
- **D-10.** `last-model` is written on every new machine in the session,
  from `machineChanged`.
- **D-11.** Every build sets the commanded speed back to regular (AC-2:
  full speed does not carry over).
- **D-12.** `SliceHook::drop()` is the seam for AC-2, AC-14 and EC-4;
  LIB-5 implements it.
- **D-13.** A variant switch carries plugs through
  `ProductMachines::carried`; the default carries place, peripheral and
  variant. The content of a plug (a RAM module's memory, a card file) is
  the product's to add; media come with LIB-7.
- **D-14.** `detach` of an empty place and `attach` of what is plugged in
  already are `Done` and change nothing.
- **D-15.** `attach` to a full place that takes one is a `replace`; to a
  full place that takes more, an attach to a free position as LIB-2
  decides (`canAttach`).
- **D-16.** Model and variant names in any case; the reply uses the
  catalogue's spelling.
- **D-17.** The harness entries keep their present built-in defaults in
  M-35 (Calc-U-59: PC-100C, no module; Calc-U-1600: bare model), so the
  harness does not change behaviour beyond the start-up; AC-18 and AC-26
  arrive with the product columns.
- **D-18.** Sentences: "There is no model "<name>"." (as `SharpAppPart`
  today); "The <model> has no variant "<name>"."; start with an unknown
  model: "There is no model "<name>"; the startup model is used." (as
  today); EC-1: "The startup preset of the <model> could not be loaded:
  <reason>. The <model> starts as its built-in default machine."; EC-6:
  "The startup preset of the <model> stopped: <reason>."
- **D-19.** The Library column is tested in M-35 and becomes Done in
  M-41, where AC-2's and AC-7's media clauses pass with LIB-7 (plan §1,
  "Completes" and Done). AC-14's typing clause passes in M-36. This
  moves plan §5.1's "Done in" (proposal to the user).
- **D-20.** What the other C2 designs need of `MachineControl` is in it
  from M-35: `BuildRefusal` with the place (LIB-6 AC-52),
  `BuildOrigin::StartupPreset` (LIB-6 D-24, LIB-8 AC-8), `lastBuild()`
  (LIB-8), `PlugChange` with a full request, `check` and `change` (LIB-7's
  module media), and `Machine::canReplace` (AC-12; LIB-4). The hooks
  that only later steps call (`machineRestarted`, `beforeMachineChange`,
  `addCarry`) come with those steps (§3.4).
- **Open point.** None of sizes or limits. The bounds of EC-7 and the
  built-in defaults are the spec's; LIB-18 takes them from the product.

## 10. Part D: the Calc-U-1600 column

Draft for the Part C2/D boundary review (2026-11-16). Paths without a
prefix are under `products/calc-u-1600/`; line numbers at `60e5b09`.

### 10.1 Scope

| Column | Step | Tests from (plan §3) | Done in |
|---|---|---|---|
| Calc-U-1600 | M-46, with LIB-6 1600 (§10 of its design) and C1600-4 | AC-25–33 | M-46 (plan §5.1) |

- M-46 replaces §4's first two rows: Calc-U-1600's own start, model
  switch, resets, ROM switches and peripheral toggles become the
  library's commands, the app layer gets its `ProductMachines`, and the
  harness entry uses it instead of `HarnessMachines`.
- The bays stay until M-47 (LIB-7 A-1): a module choice and the disk
  are still `ModuleBay` and `FloppyBay`, which now hand their requests
  to `MachineControl` (§10.3). AC-28's "the disk is in its drive again"
  and the modules' content after a variant switch therefore pass in
  M-46 through the bays and again in M-47 through LIB-7's carry
  (`MachineControl::addCarry`).
- The code built in C2 differs from §2–§9 as the qa-report's
  "Deviations" say: `build` sets regular speed when the operation ends
  (`StartupPart::operationEnded`); `build` checks each plug on the
  machine's places before the builder; `start` with a named preset and
  no loader builds the built-in default; `change` fails the operation
  with the machine's reason. Calc-U-1600 relies on none of them beyond
  what §3 says.

### 10.2 Modules and files

**New**

| File | Holds |
|---|---|
| `app/SharpProductMachines.hpp/.cpp` | `shared::SharpProductMachines : startup::ProductMachines` (§10.3): the catalogue with the module definitions, `sharp::SharpMachineBuilder`, `builtInDefault` (AC-26), `pacingProfile` (`app/PacingProfiles`), `soundRate` (`sharp::kSoundRate`), `readinessBound` (7 s, 7 s, 27 s, as the harness entry has them), `carried` (AC-28) |
| `tests/machine_control_tests.cpp` | AC-25–33 (§10.6) |

**Changed**

| File | Change |
|---|---|
| `app/SharpAppPart.hpp/.cpp` | the commands `start` (206-219), `c1600.reset` (234-240), `c1600.pick-model` (241-248), `c1600.pc1500-rom`, `c1600.pc1600-rom` (254-264), `c1600.toggle-peripheral` (270-278) and their jobs go: `startJob` (359-365), `resetJob` (434-438), `selectModelJob` (440-451), `pickModelJob` (460-464), `switchModelJob` (578-597), `setPC1600RomVersionJob`, `setPC1500RomRevisionJob` (599-607), `togglePeripheralJob` (502-523), `resetToPromptJob` (712-719), `powerCycleAroundJob` (721-732), `buildMachine` (609-623), `discardMachine` (695-704), `reattach` (945-952), `seedClockFromHost` (706-710; LIB-2's `PeripheralsPart` sets the clocks). Kept as product commands, each now one `MachineControl::change` inside an operation: `c1600.select-module` (until M-47: the bay's request with its card, AC-29), `c1600.mount-host-drive` (the host drive with `HostDriveRequest`, AC-30), `c1600.ce1600p-rom` (AC-31, §10.3). `machineChanged` (307-313) follows every new machine, whoever built it |
| `app/SharpTopics.hpp:35-50` | the names of the commands that go are removed |
| `app/Settings.hpp/.cpp:120-139` | `startupModel()`, `lastUsedModel()`, `setLastUsedModel()` go: LIB-9's `settings::startupModel` and `StartupPart::machineChanged` write `[startup] last-model`; `defaultPresetPath` (80-82) goes: `settings::startupPreset` |
| `app/HarnessProduct.cpp:65-90` | `HarnessMachines` and `SharpBuildColdBoot` go; `machines()` returns a `SharpProductMachines` |
| `app/SharpSession.cpp`, `frontends/web/bindings.cpp:35-60` | `StartupPart` in the parts (LIB-6 §10.2, `addLibraryParts`), with `SharpProductMachines`, the ROM set of the lookup, the settings file and `PresetsPart` as the loader |
| `core/Machine/SharpMachineBuilder` (`core/Machine/SharpMachine.cpp`) | `build` leaves the machine as a cold boot does (LIB-1 AC-42; D-3 of this design), so `startUp` is no longer needed |
| `core/Machine/LegacyCalls.hpp:61-65` | `runToPromptJob`, `runToPrompt` go (`untilKeyReady`); `golden_runs_tests.cpp` keeps 0.8.1's boot run as a test function of its own (§10.8) |
| `frontends/qt6/app/MachineController.cpp:46` | `start` is now the library's (same name, same arguments) |
| `frontends/qt6/app/MainWindow.cpp` | Reset and Cold boot (325, 996) send `reset` and `cold-boot`; the menu item "Reset All" reads "Cold boot" (`docs/glossary-mapping.md` row "Cold boot"); the model picker (1011) sends `model {name}`; the ROM pickers (1023, 1029) send `variant {name}`; the CE-1600P ROM (1039) `c1600.ce1600p-rom`; the plotter and CE-158 toggles (484) `attach {place, peripheral, variant}` and `detach {place}` |
| `frontends/qt6/app/debug/DebugController.cpp:206-235` | the clean start without a preset sends `cold-boot` |
| `frontends/qt6/app/screenshots/ShotScenario.hpp:35`, `ShotRunner.cpp` | the `Reset` step sends `reset` or `cold-boot` |
| `frontends/web/src/engine.js:81-86`, `src/CalcU1600.jsx:295` | the same commands; "Reset All" reads "Cold boot" |
| `app/ControlState.cpp` | the control state reads the CE-1600P's ROM choice from `SharpAppPart` (AC-31) |

### 10.3 The product's machines and the kept commands (AC-25–33)

- **AC-25, AC-26.** `builtInDefault(model)` is `{model, default variant
  (A04, New), no peripherals}` (today `Shared/MachineSession.hpp:367-370`
  at the pin). The startup model is LIB-9's (`[startup] model`, `last`,
  the PC-1500A as `ProductDecl::fallbackModel`, LIB-9 AC-27). A start
  without settings builds a PC-1500A, cold booted; its ROM shows
  `NEW0? :CHECK`.
- **AC-27.** `model` builds the startup preset or the built-in default:
  nothing of the old machine is carried. The bays forget their slots at
  `machineChanged` when the model changed (today `selectModelJob`
  441-450: `onModelChanged`, `selectDisk({})`); the CE-1600P ROM choice
  goes back to New (AC-31: "until … the model is switched").
- **AC-28.** `variant` (the PC-1500's ROM A01/A03/A04, the PC-1600's
  New/Old) builds `variantPlan`: `carried` keeps each plug's place,
  peripheral and variant, adds the host drive's folder (`HostDriveRequest`)
  and, until M-47, a module's card from the bay (`ModuleBay` gives the
  slot's instance as `ModuleCard`, adapter "bays on `machineChanged`",
  LIB-6 §10.5); the disk goes back into the new CE-1600F from the bay
  at `machineChanged`. No startup preset is loaded (AC-7).
- **AC-29.** `c1600.select-module {slot, name}` asks the bay for the
  request (a template's definition or an instance's card) and runs
  `MachineControl::check`, then `change` with `Replace` (or `Attach`,
  `Detach` for the empty entry): one power cycle, the memory kept,
  `NEW0? :CHECK` from the ROM (row 1600-4). M-47 replaces the command by
  `media.choose` (LIB-7 §3.10).
- **AC-30.** `attach`/`detach` of `printer` (CE-150, CE-1600P) and
  `interface` (CE-158), and `c1600.mount-host-drive {dir}` (attach or
  detach of `host-drive` with the folder; a second folder while one is
  mounted is a `Replace`): what follows is the pin-out's (LIB-2 AC-28), a
  power cycle with the memory kept.
- **AC-31.** `c1600.ce1600p-rom {version}`: with a CE-1600P plugged in,
  a `Replace` by `attach printer CE-1600P:<version>` (C1600-1 AC-18);
  without one, the choice is kept in `SharpAppPart` and used by the next
  `attach printer CE-1600P` that names no variant, until the product
  quits or a model switch (D-21).
- **AC-32.** `reset` keeps the memory; `cold-boot` empties it and a RAM
  module without its own battery (CE-151, CE-155), a battery-backed
  module keeps its content (C1600-1 AC-6, `CardBase::powerLoss()`, row
  1600-16). Both end at key readiness (`untilKeyReady`), no longer
  after `runToPromptJob`'s fixed run.
- **AC-33.** A machine that is off: LIB-18's `change` with `followsNow`
  `Nothing` (§3.4); the C1600-1 machine makes the change at its off
  point (`LegacyCalls::atOffPoint`, until M-47) and stays off.
- **Reset, Cold boot and a model switch stop what runs** (AC-16; row
  1600-9's second sentence): `reset`, `cold-boot` and `model` are
  `endsRunningOperation`, as today's three commands are.

### 10.4 Startup presets

`StartupPart` loads `settings::startupPreset(settings, model)` (`[preset]
<model>`, a path or a bundle path) through LIB-6's `PresetsPart`. A
version 1 file there is `CannotLoad` (the outdated hint, LIB-6 AC-8):
the built-in default and the notice of EC-1 (row 1600-1). The key and
its values are today's (`app/Settings.hpp:80-82`), so a user's setting
keeps working once the file is upgraded. The bundled startup presets are
the `Startup` entries of `preset-list` (C1600-4 §4; `default-pc1600`
from M-47).

### 10.5 Adapters

| Adapter | Removed here | Was |
|---|---|---|
| `HarnessMachines`, `SharpBuildColdBoot` | yes | §5 |
| the products' machine-control commands (Calc-U-1600 rows of §4) | yes, except the three of §10.3 that become `MachineControl::change` calls | §5 |
| `LegacyCalls::runToPromptJob`, `runToPrompt` | yes | C1600-1 §5 |
| `c1600.select-module` over the bays | M-47 (LIB-7 A-1) | — |
| `LegacyCalls::atOffPoint`, `moduleIn` | M-47 (LIB-7 1600) | C1600-1 qa-report |

New: none. The harness entry's machine still comes from its own
`SharpProductMachines` object, not from `app/SharpAppPart`, until M-51
(LIB-11 D-6, plan M-51).

### 10.6 Tasks

Tests come before the code. `CoreTests` (Frequent); ROM-gated cases
`SKIP_UNLESS` the ROMs. The cases run the app session of
`app/tests/SessionFixture.hpp` with the library's parts (paced and in
virtual time where the operation matters) or the harness entry.
Names `lib18_acN_…`.

| Task | What | ACs | Test (file: cases) |
|---|---|---|---|
| P16-1 | Start, built-in default | AC-25, AC-26 | `machine_control_tests.cpp`: `lib18_ac25_no_settings_pc1500a_new0_check`, `lib18_ac26_builtin_default_variant_and_nothing_plugged` (each model), `lib18_ac26_version1_startup_preset_gives_default_and_notice` |
| P16-2 | Model and variant | AC-27, AC-28 | `lib18_ac27_model_switch_keeps_no_module_or_peripheral`, `lib18_ac27_ce1600p_rom_choice_back_to_new`, `lib18_ac28_variant_switch_replugs_printer_interface_host_drive_modules`, `lib18_ac28_disk_in_drive_again_same_side`, `lib18_ac28_no_startup_preset_loaded` |
| P16-3 | Module, peripherals, host drive | AC-29, AC-30, AC-33 | `lib18_ac29_module_replace_power_cycle_memory_kept_new0_check`, `lib18_ac30_printer_ce158_attach_detach_power_cycle_memory_kept`, `lib18_ac30_host_drive_mount_unmount_power_cycle`, `lib18_ac33_off_machine_change_nothing_follows_found_on_switch_on` |
| P16-4 | CE-1600P ROM | AC-31 | `lib18_ac31_plugged_is_replace_by_other_variant`, `lib18_ac31_unplugged_choice_holds_for_next_attach` |
| P16-5 | Reset, Cold boot | AC-32, AC-16 | `lib18_ac32_reset_keeps_memory`, `lib18_ac32_cold_boot_empties_memory_ce151_ce155_battery_module_kept`, `ledger_1600_9_reset_and_model_switch_stop_a_preset` (extends `session_tests.cpp`'s case) |
| P16-6 | Harness entry | (LIB-11 AC-2) | `harness_tests.cpp`: `harness_drives_a_pc1600`, `harness_variant_switch_old` unchanged; `harness_starts_with_the_product_machines` |
| P16-7 | Frontends | AC-25, AC-32 | Web `play.spec.js:44-47` (`NEW0? :CHECK` at the start) unchanged; `play.spec.js` `lib18_ac32 cold boot menu item` (Longer); Qt6 through the screenshot scenarios (Longer) |

Coverage: P16-1 to P16-5 cover AC-25 to AC-33.

### 10.7 Tools and agent knowledge (M-46)

| File | Change |
|---|---|
| `products/calc-u-1600/CLAUDE.md` | the Harness section: "It … starts the startup model … cold booted" stays; `HarnessMachines` no longer named; the machine-control commands are the frontends' too |
| `products/calc-u-1600/docs/developer/Shared-Architecture.md`, `Web-Client.md` | the start, switches and resets as the library's commands; `SharpProductMachines` |
| `products/calc-u-1600/docs/User-Guide.md` | "Reset All" → "Cold boot"; "Default presets" → "Startup presets" (the visible renames of `docs/glossary-mapping.md`) |
| `docs/glossary-mapping.md` | row "Cold boot": the Calc-U-1600 column `resetJob(allReset = true)` → the library's `cold-boot`; the visible rename "Reset All" → "Cold boot" marked done |
| memory `calc-u-1600/feedback_machine_config_on_control_bar.md` | unchanged in substance; the model picker sends `model` |
| `calc-u-1600-memories.md` | no change of descriptions |

The memory file `calc-u-1600/feedback_paste_vs_inbound_typing.md` is
changed with LIB-6 §10.7.

**Ledger rows that become visible in M-46:** 1600-9's second sentence
(Reset, Cold boot and a model switch stop what runs; its first sentence
since M-36); 1600-4 and 1600-16 are in effect since M-26 and stay. No
new row: the boot to key readiness and the cold boot at every build are
not visible (D-23); "Reset All" → "Cold boot" is a visible rename of
`docs/glossary-mapping.md`.

### 10.8 Golden runs and risks

- **Golden runs** (`tests/golden_runs_tests.cpp`, 23 runs) build through
  the builder and boot with `coldBoot()` and `legacy::runToPrompt`
  (`:147-148, 199, 254`). `runToPrompt` goes; the driver gets the same
  run as a function of its own (0.8.1's boot run, unchanged), so the
  golden runs keep 0.8.1's timing, as the driver's key taps do (LIB-6
  §10.10, question 1). The builder now leaves a cold-boot state; the
  driver's `coldBoot()` after `build()` then runs on a machine already
  in that state. A difference is a finding (CLAUDE.md), not a new
  reference.
- **The app's boot moves.** Today a rebuild runs `runToPromptJob` and
  the PC-1500 models get a Reset, not a Cold boot (`switchModelJob`
  589-591, "as in 0.8.1 only the PC-1600 gets a Cold boot"). Now every
  build is a cold boot and ends at key readiness. The PC-1500A still
  shows `NEW0? :CHECK` (AC-25); the startup presets answer it (LIB-6
  AC-46). The reference runs (C1600-4 AC-16) compare at the idle
  machine, tick counts left out.
- **Readiness bound.** 7 s and 27 s of emulated time (EC-7) as in the
  harness since M-35; a PC-1600 cold boot with the CE-1600P takes the
  longest. A bound hit is a log line, not a failure (D-5).
- **Bays and `MachineControl`** (until M-47). The bays now learn of a
  new machine only through `machineChanged`; a missed rebind would leave
  a slot's instance unsaved. P16-2 and P16-3 check that the bay writes
  after a variant switch and a module change.

### 10.9 Decided (best guess)

- **D-21 (the CE-1600P ROM while unplugged, AC-31).** A product setting
  of the run in `SharpAppPart`, not a settings key: AC-31 says the
  choice holds until the product quits or the model is switched.
- **D-22 (kept product commands).** Module choice (until M-47), host
  drive and CE-1600P ROM stay `c1600.*` commands because each needs
  product data (a card, a folder, a remembered variant); each runs
  `MachineControl::check` and `change`, so what follows is the
  library's.
- **D-23 (a cold boot at every build).** As LIB-1 AC-42 and D-3; the
  0.8.1 rule "only the PC-1600 gets a Cold boot" at a rebuild goes. Not
  visible: a new machine has empty memory either way.
- **D-24 (golden boot run).** Kept in the test as 0.8.1's run, so that
  the golden runs measure the cores and not LIB-18's readiness wait.

### 10.10 Questions for the user

None that splits this column. (The golden driver's key timing is LIB-6
§10.10, question 1.)
