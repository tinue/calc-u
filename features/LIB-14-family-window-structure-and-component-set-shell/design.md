# LIB-14 Family window structure and component-set shell: design
Status: Library column approved at the Part C1/C2 boundary (2026-11-15); Calc-U-1600 column approved at the Part C2/D boundary (2026-11-16)

## 1. Scope

| Column | Step | Tests from (plan §3) | Done in (plan §5.1) |
|---|---|---|---|
| Library | M-43 (Part C2) | AC-1–25, AC-42, AC-43; EC-1–8 | M-43 |
| Calc-U-1600 | M-49 (Part D) | AC-34–41, AC-44 | M-49 |
| Calc-U-59 | M-57 (Part E; Qt6, Web) | AC-26–33 | M-57 |

Part C2 works on the Library column only. §4 lays out where the product
columns land so that the library seams fit them; it is completed with
the Part D and Part E designs at those boundaries. The Swift frontend is
C59-8 (M-58) and takes nothing from the component sets.

- M-43 changes `library/` and `docs/test-groups.md` only. No product
  links `calcu_qt6` or loads the React component set before M-49.
- What M-43 uses, all merged before it in plan order:
  - M-20 (LIB-9 L): `SettingsFile`, `Paths`, `ProductDecl`, `HostDirs`;
  - M-22 (LIB-3 L): the busy topic (`BusyState`), `notices`, `log`,
    `speed`, `TopicMirror`, `SessionPart`;
  - M-29 (LIB-15 L): `createShell()`, `useTopic()`, `hostServices.js`,
    `dist.js`, the product made for the tests;
  - M-33 (C59-5): `AttachEndpoint`, `AttachedHarness`, `attachPort`;
    M-16 (FS 4): attach holds (§3.9);
  - M-35 (LIB-18 L): the commands `model`, `variant`, `attach`,
    `detach`, `reset`, `cold-boot` of `StartupPart` (LIB-18 design §3.5);
  - M-36 (LIB-5): the on-screen key commands `screen-key-down`,
    `screen-key-up` and `paste` (LIB-5 §3.5);
  - M-37 (LIB-4 L): the control state topic `control-state`;
  - M-38 (LIB-6 L): the request `preset-list` (LIB-6 AC-32) and the
    command `load-preset`;
  - M-39 (LIB-8 L): the component sets' folders, `calcu_qt6`,
    `calcu_qt6_tests` and `COMPONENT_SOURCES` (D-16), and the paper panel;
  - M-40, M-41, M-42: the topic `run-state` and the command `resume`
    (LIB-10 AC-6), the topic `media` with the module and media pickers
    (LIB-7), the open file command `open` (LIB-13). The shell places
    them; it does not build them.
- AC-44 is a Calc-U-1600 criterion. M-43 builds the Qt6 attach host and
  tests it with support cases (§6); the criterion passes in M-49, where
  the screenshot scenarios become its client (plan M-43, M-49).
- Not here: the control state's content (LIB-4), the settings keys
  (LIB-9), what the commands do (LIB-18), the pickers and panels of
  other features, the layouts, faceplates and hit maps (product
  material, ADR 0023), how a display is drawn.

## 2. Modules and files

Three parts. The rules are the shell's once, in C++ without Qt; each
component set draws them.

### 2.1 The models: `library/shell/`, namespace `calcu::shell` (M-43)

A new area of `calcu_library` (LIB-1 §2a). It includes `sessions`,
`settings` and `machine` (key IDs) only. It reads every topic it draws
in its Value form (D-2), so it includes no area of LIB-4, LIB-6 or
LIB-10. Tests in `calcu_library_tests` (Frequent).

| File | Holds |
|---|---|
| `library/shell/ShellProduct.hpp/.cpp` | `ShellProduct`: what the product gives the shell (name, version, build, help address, About texts and links, variant menu labels, preset file filter, its menu items and bar controls); `checkShellProduct()` (§3.2) |
| `library/shell/Inputs.hpp/.cpp` | `ShellInputs`: the topics and replies the shell draws from, as Values, and the names under which it finds them (§3.1) |
| `library/shell/MenuModel.hpp/.cpp` | `menus(inputs, product, traits)`: menus and items of AC-1–7, AC-19, EC-4 |
| `library/shell/BarModel.hpp/.cpp` | `bar(inputs, product)`: the groups and controls of AC-8–10, AC-42 |
| `library/shell/PanelModel.hpp/.cpp` | `panels(inputs)`: the paper panel of AC-23, EC-5 |
| `library/shell/SettingsPart.hpp/.cpp` | `SettingsPart`, a session part: the state topic `settings` and the command `set-setting` (§3.5) |
| `library/shell/SettingsModel.hpp/.cpp` | `dialog(settingsTopic, presetList, spec)`: sections and rows of AC-11–13, EC-3 |
| `library/shell/Loading.hpp/.cpp` | `LoadingRule` (AC-15, EC-1, EC-7) |
| `library/shell/Notices.hpp/.cpp` | `NoticeQueue` (AC-17, EC-2) |
| `library/shell/HitMap.hpp/.cpp` | `HitMap`, `keyAt()` (AC-24, EC-8), `PointerTracker` (AC-24, AC-43) |
| `library/shell/ObjectNames.hpp` | the stable names of the shell's widgets and elements, for the scenarios (§3.10) |
| `library/shell/tests/TestShellPart.hpp/.cpp` | a session part that publishes scripted Values on the topics of §3.1 and records the commands it gets: the control state, settings and preset list "built for the tests" (AC-22) |
| `library/shell/tests/hit_cases.txt` | the cases of the hit rule, one per line, read by the C++ and the JS tests alike (§6) |
| `library/shell/tests/*_tests.cpp` | §6 |

### 2.2 The Qt6 component set: `library/qt6/`, target `calcu_qt6`, namespace `calcu::qt6` (created in M-39; the shell in M-43)

A static library on `Qt6::Widgets`, linking `calcu_library`; the fourth
exception in LIB-1 §2a's list (after the `calcu` CLI, `calcu_web_glue`
and the `CALCU_ATTACH` option; D-1). M-39 (LIB-8 D-1) defines it in
`library/qt6/CMakeLists.txt`, with `CALCU_FRONTEND_QT`, when
`find_package(Qt6 COMPONENTS Widgets)` succeeds and the tree is neither
Emscripten nor iOS; M-43 adds the files below.

| File | Holds |
|---|---|
| `library/qt6/SessionHost.hpp/.cpp` | `SessionHost`: hosts an app session on the GUI thread: the 60 Hz timer, the listener over `TopicMirror`, commands with their reply handlers, the host-service asks, the attach host (§3.8, §3.9) |
| `library/qt6/Shell.hpp/.cpp` | `Shell`: puts the parts below on a product's `QMainWindow` (§3.8) |
| `library/qt6/ShellMenus.hpp/.cpp` | `QMenuBar` from `MenuModel`; roles, shortcuts, tooltips |
| `library/qt6/ShellControlBar.hpp/.cpp` | the control bar from `BarModel`; the group containers for product controls |
| `library/qt6/ShellSettingsDialog.hpp/.cpp` | the settings dialog from `SettingsModel` |
| `library/qt6/ShellLoading.hpp/.cpp` | "Loading…" as an overlay (D-6) |
| `library/qt6/ShellNotices.hpp/.cpp` | the notice dialog over `NoticeQueue` |
| `library/qt6/ShellAbout.hpp/.cpp` | the About dialog |
| `library/qt6/PanelArea.hpp/.cpp` | the area for panels (AC-23) |
| `library/qt6/FaceplateInput.hpp/.cpp` | an event filter on the product's faceplate widget: mouse and touch → `PointerTracker` → commands (AC-24, AC-43) |
| `library/qt6/RomsMissingView.hpp/.cpp` | the text of `roms-missing` in place of the faceplate (AC-19) |
| `library/qt6/tests/` | `calcu_qt6_tests` (§6), the test window `TestWindow.hpp/.cpp` over `TestShellPart` |
| `library/qt6/tests/check_no_product_code.sh` | no include from `products/`, no model name, in `library/shell`, `library/qt6`, `library/web/components` (AC-22) |

The components of LIB-8 (paper panel, M-39), LIB-10 (run-control
buttons, M-40) and LIB-7 (pickers and dialogs, M-41) live in the same
folder and target (D-16). LIB-8's `ClipboardService` is answered through
`SessionHost`'s service handler from M-43.

### 2.3 The React component set: `library/web/components/` (created in M-39; the shell in M-43)

Classic scripts in the one closure of a product's `calc.js` (LIB-15
§2), on `createShell()` and `useTopic()`. Every top-level name of the set
starts with `Shell`, `Calcu` or `calcu` (the shell's with `Shell` or
`calcuShell`) so that none collides with a product component of the same
closure (D-17). M-39 creates the folder and `COMPONENT_SOURCES` with
LIB-8's paper panel; M-40 and M-41 add LIB-10's and LIB-7's components.

| File | Holds |
|---|---|
| `library/web/components/shellModel.js` | the same models as §2.1 in plain JS, no React, no DOM: `calcuShellMenus`, `calcuShellBar`, `calcuShellPanels`, `calcuShellDialog`, `calcuShellLoading`, `calcuShellNotices`, `calcuShellKeyAt`, `calcuShellPointer`; loadable by `node --test` |
| `library/web/components/useShell.jsx` | `useShell(shell, product, traits)`: the models from the topics, the command senders |
| `library/web/components/ShellMenuBar.jsx` | the menu bar (AC-1–7) |
| `library/web/components/ShellControlBar.jsx` | the control bar (AC-8–10, AC-42) |
| `library/web/components/ShellModal.jsx` | the one modal frame of the set |
| `library/web/components/ShellSettings.jsx`, `ShellAbout.jsx`, `ShellNotice.jsx`, `ShellLoading.jsx` | the dialogs and "Loading…" |
| `library/web/components/ShellPanelArea.jsx` | the area for panels |
| `library/web/components/useShellFaceplate.jsx` | pointer events on the product's faceplate element → `calcuShellPointer` |
| `library/web/components/ShellRomsMissing.jsx` | AC-19 |
| `library/web/components/shell.css` | scoped under `.calcu-client` (LIB-15 AC-17) |

### 2.4 Changed files (M-43)

| File | Change |
|---|---|
| `library/CMakeLists.txt`, `library/qt6/CMakeLists.txt` | `shell/*.cpp` and `shell/tests/*.cpp` in the globs; the files of §2.2 in `calcu_qt6` and `calcu_qt6_tests` (`add_subdirectory(qt6)` is M-39's) |
| `library/web/build/dist.js` | the files of §2.3 added to `COMPONENT_SOURCES` (M-39's list, not in `LIBRARY_SOURCES`' default, D-17) in load order, and `componentCss()` |
| `library/web/CMakeLists.txt` | `LibraryWebNodeTests` also runs `tests/shell_model.test.js` |
| `library/web/tests/product/TestProduct.cpp`, `src/CalcUTest.jsx`, `build.js`, `calc.css` | the test product adds `TestShellPart` and `SettingsPart` and composes its page from the React set (§6) |
| `library/web/tests/shell.spec.js` (new) | the Playwright scenarios (§6) |
| `docs/test-groups.md` | the new entries |
| `features/LIB-1-machine-interface-and-descriptors/design.md` §2a | the area `shell` in the table (the `calcu_qt6` exception is M-39's; agent knowledge follows the code) |

## 3. Seams and interfaces

Signature sketches. Everything is single-threaded on the thread that
hosts the session (the GUI thread for Qt6, the page for React); nothing
throws across the interface (LIB-1 §2a).

### 3.1 What the shell draws from (AC-1–3, AC-8–10, AC-15–19, AC-23)

| Input | Kind | Owner | What the shell reads |
|---|---|---|---|
| control state | state topic `control-state` | LIB-4 (`kControlStateTopic`) | models and the running one; variants of the running model; places with what is plugged in (and its variant), the empty choice, the peripherals that fit, each with its variants and "available now" with the reason; media places with owner and kind; panels; the actions Reset, Cold boot, load preset, open file, each available now with the reason; product entries (passed on unread to the product's code, AC-42) |
| `busy` | state topic | LIB-3 | `operationRuns`, `speed` |
| `notices` | stream topic | LIB-3 | each sentence |
| `log` | stream topic | LIB-3 | each line (Qt6, AC-18) |
| `preset-list` | request | LIB-6 AC-32 | per entry: bundle path, model, name, description, startup preset or example, group |
| `run-state` | state topic | LIB-10 | paused or not (Resume) |
| `media` | state topic | LIB-7 | whether the product has kinds of medium (File ▸ Import Media…, Saved Media…); the pickers read it themselves |
| `roms-missing` | state topic | LIB-12 | `text` (today `RomsMissingPayload::toValue()`, `products/calc-u-1600/app/SharpAppPart.cpp:1135`) |
| `settings` | state topic | this feature (§3.5) | the rows |

```cpp
namespace calcu::shell {

struct ShellInputs {               // the last Value of each, Null while none came
    sessions::Value controlState, busy, presetList, runState, romsMissing, settings, media;
    bool started() const;          // a control state came: AC-19 is the opposite
};

struct InputNames {                // the owners' names (LIB-4, LIB-10, LIB-12, LIB-7, LIB-6)
    sessions::TopicName controlState{"control-state"}, runState{"run-state"}, romsMissing{"roms-missing"},
                        settings{"settings"}, media{"media"};
    sessions::RequestName presetList{"preset-list"};
};

}  // namespace calcu::shell
```

**Module slots and places without a control.** LIB-4's design leaves
to this feature how a module slot is told from another place (LIB-4
O-3); its place entry carries the connector and pin-out. The shell takes
both marks from the product (D-23): a place is a module slot when its
pin-out is in `ShellProduct::moduleSlotPinOuts`, and has no control when
it is in `ShellProduct::placesWithoutControl` (the host drive).

### 3.2 What the product gives (AC-2, AC-7, AC-20, AC-21, AC-42)

```cpp
enum class MenuGroup { FileOpen, FileKinds, FileMedia, FileEnd, Edit, MachineEnd, Help };
enum class BarGroup  { AfterModel, Slots, Places, Media, End };

struct ProductItem {               // AC-7
    MenuGroup group;
    std::string id, label;         // id: stable, for the object name
    std::string entry;             // a product entry of the control state that says available now; empty = always
    std::function<void()> act;     // Qt6; the React set takes a JS callback of the same shape
};

struct ShellProduct {
    std::string name, version, build;          // "Calc-U-1600"
    std::string helpUrl;                       // AC-20; "https://calc-u.org/1600/"
    std::string aboutLabel;                    // empty: "About <name>"
    std::vector<std::string> aboutLines;       // AC-21, the texts in order
    std::vector<std::pair<std::string, std::string>> aboutLinks;   // label, address
    std::map<std::string, std::string> variantLabels;   // model or peripheral ID → submenu label (AC-2)
    std::string presetFilter;                  // the file dialog's filter for Load Preset…
    bool copyScreen = false, pasteText = false;   // Edit items (AC-1; D-12)
    std::set<std::string> moduleSlotPinOuts;   // the pin-outs of module slots (D-23)
    std::set<std::string> placesWithoutControl;   // AC-9: "host-drive" (D-23)
    std::vector<ProductItem> items;
};

// The problems of a product's shell data: a model or peripheral with
// variants and no label, an empty help address, duplicate item ids.
std::vector<std::string> checkShellProduct(const ShellProduct&, const sessions::Value& catalogueControlState);

}
```

### 3.3 Menus (AC-1–7, AC-19, EC-4)

```cpp
struct MenuItem {
    enum class Kind { Action, Submenu, Separator } kind;
    std::string id, label;
    bool checked = false, checkable = false, enabled = true;
    std::string reason;                // the tooltip of a disabled item (AC-5)
    enum class Role { None, Preferences, About, Quit } role = Role::None;
    enum class Shortcut { None, LoadPreset, Reset, ColdBoot, Preferences, Quit, Copy, Paste } shortcut{};
    sessions::CommandName command;     // sent when chosen; empty: a shell action (dialog, help, product act)
    sessions::Value args;
    std::vector<MenuItem> children;
};
struct Menu { std::string id, label; std::vector<MenuItem> items; };
struct Traits { bool hasQuit, shortcuts, savedMedia; };   // Qt6: true, true, false; Web: false, false, true (AC-4, AC-6; LIB-7 OQ-4)

std::vector<Menu> menus(const ShellInputs&, const ShellProduct&, Traits);
```

| # | Rule | AC |
|---|---|---|
| M1 | Menus File, Edit, Machine, Help in this order; a menu without items is left out; inside File the groups FileOpen, FileKinds, FileMedia, FileEnd with a separator between groups that have items | AC-1, AC-7 |
| M2 | File: Load Preset… (the `load-preset` action entry; opens the file dialog, then sends `load-preset {ref}`); Examples ▸ with one submenu per group of `preset-list`, when it has examples; each item sends `load-preset {ref: "examples:<path>"}`; then the product's items; in group FileMedia, when the `media` topic lists a kind and the frontend has the saved media list (`Traits::savedMedia`: Web), Import Media… (`media.import`) and Saved Media… (LIB-7's saved media dialog; LIB-7 AC-29); Settings… (`Preferences` role and shortcut); Quit (`Quit`, only with `hasQuit`) | AC-1, AC-6 |
| M3 | Edit: Copy Screen and Paste Text when the product has them; Paste Text sends LIB-5's `paste {text}` with the clipboard's text | AC-1 |
| M4 | Machine ▸ Model: the models, the running one checked; another sends `model {name}` | AC-2 |
| M5 | A submenu for the running model when it has more than one variant, and one for each plugged-in peripheral with more than one, under `variantLabels[id]`, the running variant checked; another sends `variant {name}` for the model, `attach {place, peripheral, variant}` for a peripheral (the replace, LIB-18 §3.5, LIB-2 AC-35) | AC-2 |
| M6 | Machine ▸ Peripherals: for each place that has a control by B3, its fitting peripherals in descriptor order, a separator between places, the plugged-in one checked; an unplugged one sends `attach {place, peripheral}`, the checked one `detach {place}` | AC-3 |
| M7 | Then Reset (`reset`, Ctrl+R), Cold boot (`cold-boot`, Ctrl+Shift+R), Resume (LIB-10's `resume`, enabled while `run-state` says paused); then the product's MachineEnd items | AC-1, AC-4 |
| M8 | Help: `<name>` Help opens `helpUrl`; About (`About` role) opens the About dialog | AC-20, AC-21 |
| M9 | An item drawn from an entry is enabled exactly when the entry is available now; otherwise disabled with the entry's reason. A product item with `entry` follows that product entry | AC-5, AC-7 |
| M10 | No control state (`!started()`): no Machine menu; File keeps Settings… and Quit; Help stays | AC-19 |
| M11 | The model is computed afresh at each change of an input; the component set replaces what changed in place, without a new window | EC-4 |
| M12 | Every other item has the role `None`, so Qt's heuristic moves nothing else into the application menu | AC-6 |

### 3.4 Control bar and panels (AC-8–10, AC-23, AC-42, EC-5)

```cpp
struct BarControl {
    enum class Kind { ModelPicker, SlotPicker, PlaceButton, PlacePicker, Media, Product } kind;
    std::string id;                    // place, media place or product id
    std::string label;                 // "1:" before a slot when the model has more than one; the peripheral's name on a button
    bool plugged = false;              // a button: shown as pressed
    std::vector<MenuItem> choices;     // a picker: the empty place ("None", D-11) and what fits, with availability
};
struct BarGroupModel { BarGroup group; std::vector<BarControl> controls; };
std::vector<BarGroupModel> bar(const ShellInputs&, const ShellProduct&);

struct Panel { std::string id; std::string owner; };   // owner empty: none
std::vector<Panel> panels(const ShellInputs&);
```

| # | Rule | AC |
|---|---|---|
| B1 | Groups left to right: the model picker; the module slots (LIB-7's picker in each, "n:" before it when the model has more than one slot); the other places; the media places, each with its owner's controls (a factory registered per medium kind, §3.8); inside a group the descriptor order | AC-8 |
| B2 | A product's own control goes into the group the product names (`addBarControl(group, after, widget)`); the shell reads no product entry | AC-42 |
| B3 | A place that is not a module slot: exactly one peripheral fits → a button with its name, pressed while plugged in; press sends `attach` when empty, `detach` when not. Several fit → a picker with the empty place. A place marked as needing more than a choice has no control and no Peripherals ▸ entry | AC-9 |
| B4 | No entry, no control, no gap | AC-10 |
| B5 | Availability as M9, the reason as the tooltip | AC-5 |
| B6 | The paper panel the control state lists is shown in the panel area always, with its owner or none; a product panel is put into the same area by the product's code | AC-23, EC-5 |
| B7 | No control state: no bar | AC-19 |

### 3.5 Settings (AC-11–14, EC-3)

```cpp
struct SettingsPartSetup {
    settings::SettingsFile& file;      // the product's, on the session's thread (LIB-9: one owner)
    const settings::Paths& paths;
    settings::FrontendKind frontend;
    // Product folders in use (a [folders] key's start folder; Calc-U-1600's
    // host drive folder): section, key → the folder in use; nullopt = the value.
    std::function<std::optional<std::string>(std::string_view, std::string_view)> inUse;
    std::function<void(std::string_view section, std::string_view key)> changed;   // the product applies it
};
class SettingsPart final : public sessions::SessionPart {
public:
    explicit SettingsPart(SettingsPartSetup);
    void attach(sessions::SessionSetup&) override;
    // state topic `settings`: {home, rows: [{section, key, kind, value, default, choices, min, max,
    //                          inUse, isDefault}]} for every declared key but [window],
    //                          [startup] last-model and format-version
    // command `set-setting` {section, key, value | null}: null = the default. Done; a value
    //   the key does not accept is Refused with the reason; a file that cannot be written
    //   gives a notice with LIB-9's problem and keeps the value for this run (D-19).
    //   Not refused while an operation runs (AC-14: it starts none).
};

struct DialogRow {
    enum class Widget { StartupModel, StartupPreset, Folder, Integer, Choice, Text, Note } widget;
    std::string section, key, label;
};
struct DialogSection { std::string title; std::vector<DialogRow> rows; };
struct DialogSpec { std::vector<DialogSection> sections; };   // the frontend's, in order

// The family sections of AC-11 for the keys this frontend has; the product appends its own.
DialogSpec familySections(const settings::ProductDecl&, const std::vector<std::string>& keysShown);

struct ShownRow {                      // what a widget shows
    DialogRow row;
    std::string shown;                 // a folder with `~` for home (AC-13)
    std::vector<std::pair<std::string, std::string>> choices;   // value, label
    bool resettable = false;           // AC-13: value differs from the default
};
std::vector<std::pair<std::string, std::vector<ShownRow>>>
dialog(const sessions::Value& settings, const sessions::Value& presetList, const DialogSpec&);
```

| # | Rule | AC |
|---|---|---|
| S1 | Title "Settings"; sections General, Startup presets, Storage, Folders, Tracing, then the product's; a section without a row is left out; the only button is Close | AC-11 |
| S2 | A widget change sends `set-setting` at once; nothing waits for Close | AC-11 |
| S3 | Startup model: "Last used" (`last`) and the product's models | AC-11 |
| S4 | A startup preset row: "None" (empty), the preset list's startup presets for that model as "name — description", and the file set now when it is none of them, also when it no longer exists; Qt6 adds "Choose…" (a file dialog in the preset folder) | AC-12, EC-3 |
| S5 | A folder row: the folder in use with `~` for home, "Change…" (a folder dialog), "Reset" enabled only while `!isDefault`, which sends `set-setting` with `null` | AC-13 |
| S6 | The dialog follows the `settings` topic; a change from elsewhere shows at once | AC-11 |
| S7 | Neither the dialog nor the part is held back by the busy topic | AC-14 |

Labels of the family rows are fixed in `SettingsModel.cpp` (and
mirrored in `shellModel.js`): "Startup model", "Startup presets", "Save
folder", "Trace folder", "Maximum file size (MB)". The product gives the
labels of its own rows (the folders of Calc-U-1600, "Segment Style").

### 3.6 "Loading…" and notices (AC-15–17, EC-1, EC-2, EC-7)

```cpp
class LoadingRule {                     // wall time from the caller (pacing::WallTime)
public:
    static constexpr std::chrono::milliseconds kDelay{500};
    void busy(bool operationRuns, pacing::Speed, pacing::WallTime now);
    bool shown(pacing::WallTime now) const;   // runs at Full for kDelay without a break
    std::optional<pacing::WallTime> due() const;  // when to look again (a timer)
};
class NoticeQueue {
public:
    void push(std::string sentence);
    const std::string* current() const;       // shown; nullptr when none
    void close();                              // the next one, in order
    // A refusal reply to a command the shell sent is pushed unless it equals the
    // last notice (as MachineController::m_lastNotice today; D-7) or begins
    // with `asks:` (N4).
    void refusal(std::string reason);
};
```

| # | Rule | AC |
|---|---|---|
| L1 | Shown when `operationRuns && speed == Full` has held for 500 ms without a break; hidden at once when it ends or goes to Regular; the next Full stretch counts from its start | AC-15, EC-1, EC-7 |
| L2 | An overlay on the faceplate below its display (the hit map's display area), "Loading…" with a busy sign, no button, transparent to input; menus, bar and keys stay usable | AC-15, AC-16 (D-6) |
| N1 | A notice is shown as it is in a dialog with OK and no title, window-modal; the session goes on calling behind it (Qt6: `QMessageBox::open()`, no nested loop) | AC-17 |
| N2 | Several: one after the other, in arrival order; none lost | AC-17 |
| N3 | A notice over "Loading…": both kept; OK closes only the notice | EC-2 |
| N4 | A refusal whose reason begins with `asks:` (LIB-7 D-6, LIB-13 D-4) is no notice: the shell shows the question with Yes and No; Yes sends the same command again with `confirmed: true`, No sends nothing | AC-17 (LIB-7 AC-17, AC-31; LIB-13 AC-4) |
| G1 | The log: no view; with `ShellOptions::logToPlatform` (Qt6) each line goes to `qInfo()`, which Calc-U-1600's `AppLogging.cpp` routes as today | AC-18 |

### 3.7 Hit map and pointers (AC-24, AC-43, EC-8)

```cpp
struct Rect { double x, y, w, h; };            // in the hit map's own units
struct HitMap {
    std::vector<std::pair<machine::KeyId, Rect>> keys;   // in the product's order
    std::optional<Rect> display;
    double distance = 0;                       // in logical pixels of the shown faceplate (D-8)
};
// `scale`: logical pixels per hit-map unit as shown now; `p` in logical pixels.
std::optional<machine::KeyId> keyAt(const HitMap&, double scale, double px, double py);

class PointerTracker {                         // per faceplate
public:
    struct Out { std::vector<sessions::Command> commands; };   // screen-key-down/up {key, pointer} (LIB-5), speed {full}
    Out down(std::int64_t pointer, double px, double py);
    Out move(std::int64_t pointer, double px, double py);
    Out up(std::int64_t pointer);              // also cancel
    void releaseAll(Out&);                     // a model switch, the window losing focus
};
```

| # | Rule | AC |
|---|---|---|
| H1 | Inside an area (edges included) → that key; inside several → the first in the hit map | AC-24 |
| H2 | Else the key whose area is nearest (Euclidean distance to the rectangle) if no farther than `distance`; equal distances → the first | AC-24 |
| H3 | Else no key; nothing is sent for that pointer, also when it moves onto a key | EC-8 |
| H4 | Down on a key → `screen-key-down {key, pointer}`; up or cancel → `screen-key-up {key, pointer}`; the typing engine keeps the holders, so a key held by two pointers stays down until the last lets go (LIB-5 E5); a move changes no key (D-9) | AC-24 |
| H5 | Down on the display → `speed {full: true}`; up, or a move off the display → `speed {full: false}` when no other pointer is on it | AC-43 |
| H6 | The product draws pressed keys from live input (LIB-5 AC-19); the shell draws none | AC-24 |

The command names are LIB-5's `screen-key-down` and `screen-key-up`
(M-36; LIB-3's raw `key-down`/`key-up` are not the typing engine's, LIB-5
AC-39) and LIB-3's `speed`.

### 3.8 The Qt6 component set

```cpp
namespace calcu::qt6 {

class SessionHost : public QObject, private sessions::MessageSink {
    Q_OBJECT
public:
    SessionHost(sessions::AppSession&, sessions::HostServices, pacing::WallClock, QObject* parent = nullptr);
    void start();                       // the 60 Hz QTimer (Qt::PreciseTimer): one call() per tick
    void stop();
    void call();                        // one host call; then the attach harness's afterHostCall()
    using Reply = std::function<void(const sessions::ReplyMessage&)>;
    sessions::CallId send(const sessions::CommandName&, sessions::Value args = {}, Reply = {});
    sessions::CallId ask(const sessions::RequestName&, sessions::Value args = {}, Reply = {});
    void setServiceHandler(std::function<void(const sessions::ServiceAsk&)>);   // the frontend answers
    const sessions::TopicMirror& topics() const;
    sessions::Value value(const sessions::TopicName&) const;   // toValue() of the last state, or Null
    sessions::AppSession& session();
    // Builds with CALCU_ATTACH only (§3.9). Returns the problem; empty when open.
    std::string openAttach(harness::HarnessProduct&, std::uint16_t port);
    std::uint16_t attachPort() const;   // the bound port (port 0: the one the system picked)
signals:
    void topicChanged(const QString& name);
    void called();
};

struct ShellOptions { bool logToPlatform = true; };

class Shell : public QObject {
    Q_OBJECT
public:
    Shell(QMainWindow&, SessionHost&, shell::ShellProduct, shell::DialogSpec, ShellOptions = {});
    QWidget* controlBar();                   // the product places it below its faceplate
    PanelArea* panelArea();                  // the product places it in its layout
    QWidget* romsMissingView();              // shown by the shell in place of the faceplate (AC-19)
    void setFaceplate(QWidget*, shell::HitMap, std::function<double()> scale);   // AC-24, AC-43
    void addBarControl(shell::BarGroup, std::string after, QWidget*);           // AC-42
    using Factory = std::function<QWidget*(const sessions::Value& entry, QWidget* parent)>;
    void registerMediaControls(std::string mediumKind, Factory);   // LIB-7 kinds by default; C59-7's card
    void registerSlotPicker(Factory);                              // LIB-7's module picker by default
    void registerPanel(std::string kind, Factory);                 // LIB-8's paper panel by default
};

}  // namespace calcu::qt6
```

- **Menus.** `ShellMenus` builds `QMenuBar` from `menus()` and rebuilds
  the menus whose model changed (EC-4). Shortcuts: `QKeySequence(Qt::CTRL
  | Qt::Key_O)`, `Qt::CTRL | Qt::Key_R`, `Qt::CTRL | Qt::SHIFT |
  Qt::Key_R` (Qt's `CTRL` is the Command key on macOS); Settings…, Quit,
  Copy, Paste keep `QKeySequence::Preferences`, `Quit`, `Copy`, `Paste`
  (D-14). They are window shortcuts, so they reach the action before the
  focused widget's `keyPressEvent`; `FaceplateInput` and the products'
  key handlers must not accept `ShortcutOverride` for them (AC-4).
  `QMenu::setToolTipsVisible(true)` and `QAction::setToolTip(reason)`
  (AC-5). Roles per M2, M8, M12 (AC-6).
- **Bar.** `QComboBox` pickers with disabled items carrying the reason
  as `Qt::ToolTipRole`; checkable `QToolButton` for B3 buttons.
- **Dialogs.** `ShellSettingsDialog` and `ShellAbout` are window-modal
  through `open()`; `QFileDialog` for Load Preset… and "Choose…",
  `QFileDialog::getExistingDirectory` for "Change…"; the start folders
  from LIB-9's `[folders]` (`OpenFolders`). Help:
  `QDesktopServices::openUrl`; its result is ignored (EC-6).
- **Panels.** `PanelArea` is a `QWidget` with a box layout the product
  may replace (its splitter in Calc-U-59, AC-33).

### 3.9 Harness attach in Qt6 (AC-44, built here)

FS 4's result (M-16, `docs/walkthroughs/first-steps.md` step 4) holds
for a host loop on any thread: one port per running app, `bind` on a
port in use fails and the app goes on, a client in the same machine
reaches `127.0.0.1:<port>`. C59-5 §3.2 left the GUI-thread host to this
feature: `SessionHost` implements `AttachInbox` and calls
`afterHostCall`, so `AttachEndpoint` and `AttachedHarness` are reused
unchanged.

| Case | Rule |
|---|---|
| builds | compiled when `CALCU_ATTACH` is defined (`library/CMakeLists.txt`: POSIX trees with `CALCU_TESTS`); the release trees (`build/release`, no tests) have none; Windows has none (the endpoint is POSIX; D-5) |
| post | `AttachInbox::post` → `QMetaObject::invokeMethod(this, …, Qt::QueuedConnection)` → `AttachedHarness::handle` on the GUI thread |
| after a call | `call()` ends with `afterHostCall()` (a due `wait` replies) |
| suspended | never: Qt6 sends no suspend (LIB-3 AC-36), so `callsSuspended` is `false` |
| port | `harness::attachPort(QCoreApplication::arguments())`: `--attach-port N`, else 59059 (C59-5); `--attach-port 0` lets the system pick, for an in-process client (M-49's shot runner) |
| bind fails | a line in the session's log, "Attach is not available on port N: <problem>." (C59-5 EC-1); the app runs on |
| product | the product passes its `HarnessProduct` (LIB-11; Calc-U-1600's builds through `SharpAppPart` from M-51) |

### 3.10 The React component set

```js
// In a product component, after createShell() (LIB-15 §3.1):
const s = useShell(shell, product, { hasQuit: false, shortcuts: false, savedMedia: true });
// s.menus, s.bar, s.panels, s.dialog, s.loading, s.notice, s.started, s.romsMissing
// s.choose(item), s.setSetting(section, key, value), s.closeNotice()
<ShellMenuBar shell={s} />            // AC-1 to AC-7
<ShellControlBar shell={s} mediaControls={…} slotPicker={…} extra={{[group]: [...]}} />
<ShellPanelArea shell={s} panels={{paper: PaperPanel, ...}} />
<ShellDialogs shell={s} />            // settings, about, notice, loading
const pointer = useShellFaceplate(shell, hitMap, faceplateRef);   // pointerdown/up/cancel/move, setPointerCapture
```

`product` is the JS form of `ShellProduct` (§3.2). A Web frontend binds
no shortcut (AC-4) and has no Quit (AC-6). The disabled reason is the
element's `title` and `aria-disabled` (AC-5). Help opens through
`hostServices.js`'s open-url function (AC-20). The models are those of
`shellModel.js`, which mirrors §3.3 to §3.7 rule by rule.

**Object names** (`library/shell/ObjectNames.hpp`; Qt `objectName`, Web
`data-calcu`): `menu-file`, `menu-edit`, `menu-machine`, `menu-help`,
`item-<id>` (`item-load-preset`, `item-settings`, `item-reset`,
`item-cold-boot`, `item-resume`, `item-help`, `item-about`,
`item-model-<model>`, `item-peripheral-<place>-<peripheral>`),
`control-bar`, `bar-model`, `bar-slot-<place>`, `bar-place-<place>`,
`bar-media-<media place>`, `settings-dialog`, `setting-<section>-<key>`,
`loading`, `notice`, `notice-ok`, `about-dialog`, `panel-area`,
`roms-missing`. The scenarios of AC-25 and M-49's shot scenarios find
widgets by these names (AC-44 owns them).

## 4. Product side (outline; completed with the Part D and E designs)

**Calc-U-1600 (M-49).** Both frontends are put together from the shell
on LIB-4's control state.
- Qt6: `MachineController` hosts through `calcu::qt6::SessionHost`;
  `EmulationPacer.*` goes (its timer is the host's; `setTurbo` is H5).
  `MainWindow::buildMenuBar` gives way to the shell's menus with the
  product's items (Load BASIC Program…, Load Machine Code…, Mount
  Directory…, Unmount; Copy Screen, with Copy Log Selection while text of
  the INSPECTOR is selected; the variant labels "ROM Revision", "ROM
  Version", "CE-1600P ROM"). `ControlBar.*` and `NamedComboBox.*` go; the
  floppy disk controls are a product control (AC-42) on their product
  entry. `SettingsDialog.*` keeps only the sections Serial ports and
  Debugger as product rows; `AboutDialog.*` goes into `ShellProduct`.
  `Operations.*` loses "Loading…", the wait cursor and its warning boxes.
  `FaceplateWidget` gives its hit map (`app/KeyLayout`, distance 24) to
  `Shell::setFaceplate`. `--pacing-probe` goes (LIB-17 §5): the probe is
  an attach client.
- AC-44: `screenshots/ShotRunner.*` opens attach on port 0 and talks to
  it with a `QTcpSocket` line client: `preset`, `wait`, `key`, `type`;
  the frozen mode (`MachineController::setFrozen`, `runFor`,
  `runUntilTyped`, `finishOperation`) goes. UI steps find widgets by the
  names of §3.10; `Edit > Settings…` becomes `File > Settings…` in
  `docs/developer/screenshots/guide/*.shots.yaml`.
- Web: `src/MenuBar.jsx`, `ControlBar.jsx`, the settings, About,
  loading and notice parts of `Dialogs.jsx` and the hit test of
  `Faceplate.jsx` give way to the React set; `src/engine.js` (LIB-15's
  translation) goes; `SharpWebPart`'s `c1600.settings-state`,
  `c1600.set-startup-model`, `c1600.set-default-preset` give way to
  `SettingsPart`.

**Calc-U-59 (M-57).**
- Qt6: `EmulatorController` hosts through `SessionHost`;
  `MainWindow::buildMenus` → the shell (card items of C59-7 as product
  items); `ControlBar.*` (Reset with Ctrl, Preset) → the shell bar with
  LIB-7's module picker, the "PC-100C" button by B3, C59-7's card
  controls; `SettingsDialog.*` keeps Display ("Segment Style");
  `AboutDialog.*` → `ShellProduct`; `KeyboardWidget`'s use of
  `PointerTracker` (`app/KeyboardLayout.hpp`) → `FaceplateInput` with
  the product's widened areas and distance 0. The splitter layout stays
  (AC-33). `--pacing-probe` goes; Qt6 gets attach.
- Web: `PlayCalculator.jsx` composes the set; the wide layout gets
  `ShellMenuBar`, the portrait layout keeps Reset, Preset, Settings and
  gets a More button (built in M-57, D-15); the notice line
  (`PlayCalculator.jsx:81-90`) becomes the dialog; `TIWebPart`'s
  `c59.web.pointer-*` and `c59.web.settings`,
  `c59.web.set-startup-model`, `c59.web.set-keyboard-feedback`,
  `c59.web.set-trace-max-mb` give way to the shell's pointer and
  `SettingsPart`.
- `cu59::PointerTracker` stays for the Swift bridge
  (`frontends/swift/Bridge/TISharedTables.mm`) until C59-8 (M-58).

## 5. Adapters

M-43 changes no product code and leaves no new adapter. The old calls
that the product steps take behind the shell:

| Adapter (owner) | Wraps | Removed in |
|---|---|---|
| `Operations` popup (`products/calc-u-1600/frontends/qt6/app/Operations.cpp`, LIB-3 §4) | "Loading…" as a `QProgressDialog` and the wait cursor | M-49 |
| `--pacing-probe <s>` (LIB-17 §5) | the Qt6 emulated-time check | M-49 (1600), M-57 (59) |
| Calc-U-1600 frozen mode (`MachineController`, `EmulationPacer`) | `--shots` stepping the machine itself | M-49 (AC-44) |
| W16-1, W59-1 page data requests (LIB-15 §5) | faceplate and LCD data for the page | M-49, M-57. The hit map stays a product request: it is product material (ADR 0023), one source for Qt6 and Web (D-22) |
| `src/engine.js` of Calc-U-1600 (LIB-15 1600) | 0.8.1's method names over the shell | M-49 |
| `c1600.control-state` (LIB-3 §5) | the control state | M-49 (LIB-4 1600) |
| settings commands of `SharpWebPart`, `TIWebPart` | the settings dialog of the page | M-49, M-57 |
| `c59.web.pointer-*` (`TIWebPart`) | the pointer on the page | M-57 |
| A-4 drawing of "ROMs missing" (LIB-12 §5) | the frontends' own message | M-49, M-57 |

## 6. Tasks

Tests come before the code. Case names: `acN_…`, `ecN_…` (LIB-1 §2a);
support cases have no prefix. The C++ model tests and the JS model tests
carry the same names, so a rule that one set lacks shows in the chain.
The control states, settings and preset lists are Values built in the
tests (and, for the window tests, published by `TestShellPart`).

| Task | What | ACs/ECs | Test (file, cases, group) |
|---|---|---|---|
| L1 | `MenuModel` | AC-1, AC-2, AC-3, AC-4, AC-5, AC-6, AC-7, AC-19, EC-4 | `library/shell/tests/menu_model_tests.cpp`: `ac1_menus_and_items_in_order`, `ac1_item_without_function_and_empty_menu_left_out`, `ac2_model_menu_marks_running_and_sends_model`, `ac2_variant_submenus_for_model_and_peripheral`, `ac3_peripherals_attach_replace_and_detach`, `ac4_shortcuts_qt_and_none_on_web`, `ac5_unavailable_disabled_with_reason`, `ac6_roles_and_no_quit_on_web`, `ac7_product_item_in_its_group`, `media_items_where_saved_media_and_kinds` (M2; LIB-7 AC-29), `ac19_no_machine_menu_without_control_state`, `ec4_model_switch_changes_entries`; Frequent. Mirror in `library/web/tests/shell_model.test.js`; Frequent (`LibraryWebNodeTests`) |
| L2 | `BarModel`, `PanelModel` | AC-8, AC-9, AC-10, AC-23, AC-42, EC-5 | `bar_model_tests.cpp`: `ac8_groups_and_order`, `ac8_slot_numbers_with_two_slots`, `ac9_button_for_one_picker_for_several`, `ac9_button_attaches_and_detaches`, `ac9_place_needing_more_has_no_control`, `ac10_no_entry_no_gap`, `ac23_paper_panel_always_with_or_without_owner`, `ac42_shell_reads_no_product_entry`, `ec5_printer_change_moves_paper_owner`; Frequent; JS mirror |
| L3 | `SettingsPart`, `SettingsModel` | AC-11, AC-12, AC-13, AC-14, EC-3 | `settings_dialog_tests.cpp`: `ac11_sections_shown_for_keys_present`, `ac11_change_written_at_once`, `ac12_startup_preset_choices`, `ac13_folder_row_home_change_reset`, `ac14_set_setting_while_operation_runs`, `ec3_missing_startup_preset_still_shown`, `set_setting_unwritable_gives_notice`; on LIB-9's test product declaration (`library/settings/tests/TestProduct.hpp`); Frequent; JS mirror of the model part |
| L4 | `LoadingRule`, `NoticeQueue` | AC-15, AC-17, EC-1, EC-2, EC-7 | `loading_notice_tests.cpp`: `ac15_loading_after_500_ms_at_full_speed`, `ec1_short_operation_shows_nothing`, `ec7_back_to_full_speed_counts_again`, `ac17_notices_one_after_the_other`, `ec2_notice_over_loading`, `refusal_equal_to_last_notice_not_repeated`, `asks_refusal_is_a_question_yes_resends_confirmed` (N4); scripted wall clock (`library/testing/ScriptedClock.hpp`); Frequent; JS mirror |
| L5 | Hit rule, `PointerTracker` | AC-24, AC-43, EC-8 | `hit_map_tests.cpp`: `ac24_hit_cases` (every line of `hit_cases.txt`), `ac24_several_pointers_hold_keys`, `ac43_display_hold_full_speed_until_up_or_leave`, `ec8_no_key_at_down_nothing_on_move`; Frequent; `shell_model.test.js` reads the same `hit_cases.txt` |
| L6 | Help, About, log | AC-18, AC-20, AC-21, EC-6 | model cases `ac20_help_opens_product_address`, `ac21_about_shows_product_texts`; Qt cases in L7 `ac18_log_to_platform_no_view`, `ec6_help_failure_changes_nothing`; Frequent |
| L7 | Qt6 widgets on the test window (`QT_QPA_PLATFORM=offscreen`) | AC-1–21, AC-23, AC-24, AC-43 (the drawing of L1–L6) | `library/qt6/tests/shell_widgets_tests.cpp`, the one runner; CTest `calcu_qt6_tests`, Frequent, registered with Qt6: `ac1_…` to `ac21_…` on widgets (menus from `QMenuBar`, `QAction` roles, shortcuts and tooltips, bar widgets, the dialog's rows and Close, the overlay transparent to input with bar and Reset enabled `ac16_loading_blocks_nothing`, `QMessageBox` sequence), `ac24_mouse_and_touch_on_test_faceplate`, `ac43_…` |
| L8 | Qt6 attach host | (AC-44, for M-49) | `library/qt6/tests/attach_host_tests.cpp` (with `CALCU_ATTACH`): `qt_attach_host_line_runs_on_gui_thread`, `qt_attach_host_wait_on_app_clock`, `qt_attach_host_port_zero`, `qt_attach_host_bind_fails_logs_and_runs`; with LIB-11's test product (`library/harness/tests/TestProduct.*`); Frequent |
| L9 | React set in the test product | AC-22, AC-25 | `library/web/tests/shell.spec.js`, Chromium and WebKit: `ac1 the menus`, `ac4 no shortcut is bound`, `ac6 no quit`, `ac8 the bar`, `ac11 the settings dialog`, `ac15 loading`, `ac16 the bar works while loading`, `ac17 a notice`, `ac19 roms missing`, `ac20 help opens`, `ac22 the shell without product code`, `ac24 a key under the pointer`, `ac43 holding the display`; Longer (`LibraryWebPlaywright`) |
| L10 | Qt6 scenarios of AC-25 | AC-25 | `library/qt6/tests/shell_scenarios.cpp`: the IDs of L9 that apply to Qt6 (`ac1`, `ac4`, `ac6` roles, `ac8`, `ac11`, `ac15`, `ac17`, `ac20`, `ac24`) on the test window; CTest `calcu_qt6_scenarios`, Frequent (offscreen, about 2 s with the 500 ms waits). The pictures per layout (AC-33, AC-40) are the products' |
| L11 | No product code | AC-22 | `LibraryShellNoProduct`: `library/qt6/tests/check_no_product_code.sh`; Frequent; not on Windows |

Coverage: L1–L11 cover AC-1–25, AC-42, AC-43 and EC-1–8 (35 of 35).
AC-26–41 and AC-44 are tested in M-49 and M-57 (§4), with the same
scenario IDs per frontend (AC-25, ADR 0032).

## 7. Tools and agent knowledge

Today's tools are unaffected in M-43. `--shots` and `--pacing-probe`
change in M-49 and M-57 (§5).

| Step | Files |
|---|---|
| M-43 | LIB-1 design §2a (areas, the `calcu_qt6` exception); `docs/test-groups.md` (`calcu_qt6_tests`, `calcu_qt6_scenarios`, `LibraryShellNoProduct`, the new cases of `calcu_library_tests`, `LibraryWebNodeTests`, `LibraryWebPlaywright`). The glossary has shell, faceplate and hit map already |
| M-49 | `products/calc-u-1600/docs/developer/screenshots/README.md` (attach, object names, `File > Settings…`), `Shared-Architecture.md`, `Web-Client.md`; Calc-U-1600 memory files on the menus and the shot runner; visible renames of `docs/glossary-mapping.md` |
| M-57 | the `calc-u-59-qt` and `calc-u-59-web` skills, `reference/NewGUIGuide.md`, `FrontendParity.md`; visible renames |

## 8. Golden runs and risks

- **Golden runs** touch no frontend; M-43 touches no core. Nothing to
  compare.
- **Value form as the contract** (D-2): a change of a LIB-4, LIB-6 or
  LIB-10 payload's `toValue()` breaks the shell silently. The model
  tests build their inputs from the owners' payload types where they
  exist (`toValue()` of a real payload), so such a change fails a test.
- **Names of other features.** `preset-list`, `load-preset` (LIB-6),
  `run-state`, `resume` (LIB-10), `paste`, `screen-key-down`,
  `screen-key-up` (LIB-5), `media`, `media.import` (LIB-7), `open`
  (LIB-13) are taken from their designs; a rename there changes
  `InputNames` and the menu model in the same step.
- **Qt offscreen.** Native macOS menus are not exercised by
  `calcu_qt6_tests`; the application-menu placement (AC-6) is seen in
  M-49's shot scenarios on macOS.
- **CI.** `calcu_qt6` is new code compiled on every Qt platform
  (Linux, Windows, macOS) of both product workflows, so M-43 is a step
  that changes platform code: one CI dispatch at its merge (plan §1).
- **Shots through attach (M-49).** Today's frozen clock makes captures
  the same run to run; with `wait` on the app's own clock a blinking
  cursor may differ. The scenarios capture after `wait-idle` and the
  checks compare structure, not pixels (as the `CalcU1600Shots` checks do
  now); to be confirmed in M-49.

## 9. Decided (best guess)

- **D-1.** Three parts: the rules in `library/shell/` (`calcu_library`),
  the Qt6 set in `library/qt6/` as target `calcu_qt6` (the fourth
  exception in LIB-1 §2a's list, because Qt cannot be in
  `calcu_library`; created in M-39), the React set in
  `library/web/components/`.
- **D-2.** Both sets read the topics in their Value form. The JS set
  must, and so both models take the same input and the shell depends on
  no C++ type of LIB-4, LIB-6 or LIB-10.
- **D-3.** Settings reach the dialog through the session: the library's
  `settings` topic and `set-setting` command (`SettingsPart`), because
  the Web's settings file lives in the worker. They replace the
  products' interim settings commands (§5).
- **D-4.** The Qt6 host loop is library code (`SessionHost`); both
  products' controllers use it from M-49 / M-57. It is where attach
  lives.
- **D-5.** Qt6 attach: `--attach-port`, default 59059 as in C59-5, port
  0 for an in-process client; POSIX trees with `CALCU_ATTACH` only, so
  Windows and release builds have none. Two apps on the default port:
  the second logs it and runs on (FS 4).
- **D-6.** "Loading…" is an overlay on the faceplate below the display
  that takes no input, without a title; the wait cursor goes. Today
  Calc-U-1600 shows a window-modal `QProgressDialog` titled with the
  operation (`Operations.cpp`) and a dialog on the Web. AC-16 asks that
  nothing be blocked and that the display be seen.
- **D-7.** A refused reply to a command the shell sent is shown like a
  notice, unless it repeats the last notice (`MachineController`'s
  `m_lastNotice` rule today) or is an `asks:` question (N4).
- **D-8.** Hit-map areas are in the hit map's own units, scaled to the
  shown faceplate; the one distance is in logical pixels, so Calc-U-1600
  keeps its 24 px (`TOUCH_SLOP_PX`) and Calc-U-59 its widened areas with
  0. Edges count as inside; the first area wins among several.
- **D-9.** A key is fixed at pointer down; a drag does not move it to
  another key (AC-24, EC-8). Calc-U-59's Qt6 and Web move it today
  (`cu59::PointerTracker::move`): a ledger row is proposed.
- **D-10.** Peripherals ▸ is one flat list, place after place with
  separators, as Calc-U-1600's menu today.
- **D-11.** The empty place in a picker reads "None", as the startup
  preset rows do (AC-12).
- **D-12.** Paste Text is a shell item the product switches on (it sends
  LIB-5's command with the clipboard's text); Copy Screen is a product
  item (AC-41).
- **D-13.** A variant label is required from the product;
  `checkShellProduct()` reports a missing one in the product's tests.
- **D-14.** Beside AC-4's three shortcuts, the platform's Preferences,
  Quit, Copy and Paste stay as today; every other item has no role.
- **D-15.** The More button of a frontend without a menu bar (AC-29) is
  added to the React set in M-57, on the same menu model.
- **D-16.** The components of LIB-7, LIB-8 and LIB-10 live in
  `library/qt6/` and `library/web/components/`. Their steps (M-39 to
  M-41) come before M-43, so M-39 (LIB-8) creates `calcu_qt6`,
  `calcu_qt6_tests`, the React folder and `COMPONENT_SOURCES` in this
  layout, M-40 and M-41 add their components, and M-43 adds the shell.
- **D-17.** The component scripts are in `COMPONENT_SOURCES`, not in
  `dist.js`'s default library sources; a product takes them in its step,
  so no C2 step changes a product's `calc.js`. All their names are
  prefixed (`Shell`, `Calcu`, `calcu`).
- **D-18.** The settings dialog and About are window-modal (`open()`),
  which AC-14 allows because `set-setting` starts no operation.
- **D-19.** A settings file that cannot be written: the value holds for
  the run and a notice says so with LIB-9's problem ("says so", LIB-9).
- **D-20.** The family row labels are fixed in the shell; the product
  labels its own rows.
- **D-21.** The object names of §3.10 are the scenarios' handle in every
  frontend.
- **D-22.** The hit map is product material, served to the page by a
  product request from the same table the Qt6 frontend uses; LIB-15's
  W16-1 / W59-1 go as adapters, but the hit-map request stays.
- **D-23 (LIB-4 O-3).** A module slot and a place without a control are
  product data in `ShellProduct` (the pin-outs of module slots; the
  host drive), not marks of the control state: LIB-4 carries the pin-out
  and leaves the rule here, and the component sets then name no model.
  Calc-U-59 gives the TI module pin-out, Calc-U-1600 its memory-module
  pin-out and `host-drive`.
- **D-24.** The commands are LIB-18's (design §3.5): `model {name}`,
  `variant {name}`, `attach {place, peripheral, variant?}`, `detach
  {place}`, `reset`, `cold-boot`; the control-state topic is LIB-4's
  `control-state`. The other features' names are LIB-6's `load-preset`
  and `preset-list`, LIB-10's `run-state` and `resume`, LIB-5's `paste`,
  `screen-key-down` and `screen-key-up`, LIB-7's `media` and
  `media.import`, LIB-13's `open` (the former open point P-1).

## Part D: the Calc-U-1600 column

Step M-49, with LIB-4 1600 (the Part D section of LIB-4's design holds
the control state's product side; this section holds the frontends).
Tests from LIB-14 AC-34–41, AC-44; Done in M-49 (plan §5.1). The same
step makes LIB-3 1600 Done (its AC-38 re-pointed; §5.1, "LIB-3 1600 /
M-49") and C1600-2 Done (its AC-15, the two File items; §5.1).

### D.1 Scope, and what the earlier Part D steps leave

M-49 is the step in which both Calc-U-1600 frontends are put together
from the shell. It comes after M-44 to M-48, and this design takes what
their designs say as done:

| Step | What M-49 finds | What M-49 uses it for |
|---|---|---|
| M-44 (C1600-2) | the loaders on the loader memory kind and one classifier; "Load BASIC Program…" and "Load Machine Code…" with their dialogs in the product's code | the two File items as `ProductItem`s in the group FileKinds (AC-34, AC-35; C1600-2 AC-15) |
| M-45 (LIB-8, LIB-10 1600) | the library's `PaperPanel` / `CalcuPaperPanel` in both frontends; `run-state`, `resume` and the `DebuggerPart` in the session; the Qt `DebugController` still in the frontend (until M-50) | the paper panel through `Shell::registerPanel` (its default) in the shell's panel area; Machine ▸ Resume from the shell (M7); the title "Paused (debugger)" from `run-state` |
| M-46 (LIB-6, LIB-18, C1600-4) | the library's commands `start`, `model`, `variant`, `attach`, `detach`, `reset`, `cold-boot`, `load-preset`; `PresetsPart` and the request `preset-list`; a `ProductMachines` in `app/`; the operation commands `c1600.*` of `app/SharpTopics.hpp:36-50` gone | every menu item and bar control sends a library command (D-24); Examples ▸ from `preset-list` |
| M-47 (LIB-7 1600) | `MediaPart` with `memory-module`, `floppy-disk` and `cassette-tape`; `MemoryModuleManager`, `FloppyDiskManager` and `TapeManager` re-pointed onto `media`; the bays gone | LIB-7's `MediaPicker` per slot (`registerSlotPicker`, its default) and per media place (`registerMediaControls`); the product's disk and tape controls (D.3) |
| M-48 (LIB-13 1600) | `open` for drops, Open With and the pickers; `core/DropFile.*` gone | nothing new: the window's drop sends `open`; the shell's Load Preset… sends `load-preset` |

What is left for M-49, in both frontends: the window structure (menus,
bar, settings, About, Help, "Loading…", notices, the faceplate input,
"ROMs missing"); the session host; the control state (LIB-4 1600); the
display painted from the `display` topic (C1600-1's adapter
`toDisplayFrame`); the app enum `shared::Model` (C1600-1's adapter
`modelIdOf`, `modelOf`, `variantIdOf`); and AC-44's shot runner. If a
step of M-44 to M-48 leaves one of its frontend pieces undone, M-49
takes it over and the qa-report says so (D-25).

Not in M-49: the DAP server and `DebugController` (C1600-3, M-50); the
product commands of the harness (C1600-5, M-51); the help content and
its pictures (LIB-16, M-52, whose AC-12 uses this step's attach).

### D.2 The code of C2 against the approved text

Read from `library/` on main (`60e5b09`) and from the "Deviations" of
the M-43 qa-report. Where M-49 relies on something that differs from
§2 to §9:

- The React hook is `useCalcuShell` (`useShell` is LIB-15's). The panel
  model is in `BarModel.*`; there is no `PanelModel.*`.
- `SettingsPart` has the extra command `remember-folder {kind, folder}`.
  A `[folders]` row is of kind `folder`, labelled "<Kind> folder" unless
  the product gives its own section. Calc-U-1600 gives its own Folders
  section (D-26).
- `Shell` has `setChoosers()`, `sendChecked()`, `choose()` and
  `inputs()`; `addBarControl(group, "", w)` puts `w` at the end of the
  group. The React `extra` prop puts a product control at the end of
  the group it names.
- `SessionHost` answers clipboard and open-url itself unless a handler
  is set, has `addMessageHandler()` (the product's sound feed goes there,
  D.3), and reads `--attach-port 0` through `attachPortFrom()`.
- **Found while writing this design.** The attached harness registers
  only the first commands and the product's
  (`library/harness/attach/AttachedHarness.cpp:27-28`), and it refuses
  `wait` without a duration (`:138-139`). The serve form has the full
  set (`library/harness/Run.cpp:260-267`). AC-44 needs `preset`, `key`,
  `type`, `reset`, `cold-boot` and the bare `wait` through attach, so
  M-49 widens the attached registry (D.6; a library change, D-27).
- `library/shell/ObjectNames.hpp` holds the names of §3.10, which
  AC-44's scenarios use.

### D.3 The Qt6 frontend (`products/calc-u-1600/frontends/qt6/`)

The window becomes a `QMainWindow` with a `calcu::qt6::Shell` over a
`calcu::qt6::SessionHost`, plus the faceplate and the product's panels.

| File | After M-49 |
|---|---|
| `app/MachineController.hpp/.cpp` | Stays as the product's glue, thinner. It makes the session (`shared::makeSharpSession`, `app/SharpSession.hpp:24`) and a `calcu::qt6::SessionHost` over it. `call()`, `send()`, `m_listener`, `now()` and the busy and display bookkeeping (`MachineController.cpp:65-98, 170-229`) go to the host. The frozen mode goes: `setFrozen`, `finishOperation`, `runFor`, `runUntilTyped` (`:100-137`; adapter of §5). The sound feed becomes a `SessionHost::addMessageHandler`. `controlState()` (`:231-234`) goes with LIB-4's interim topic. Stays until M-50: `DebugController` and the `SharpDebugger` hooks (`:251-263`), and `typeCommand` (`:158`, the debugger's auto-start) |
| `app/MainWindow.hpp/.cpp` | `buildMenuBar` (`MainWindow.cpp:844-1004`) goes, and so do `applyModelSelection`, the `apply*RomVersionSelection` and `syncMachineMenuFrom*` functions, `syncControlBarForModel`, `syncPeripherals`, `togglePeripheral`, `syncHostDriveActions`, `onPresetArmed`, `resetMachine`, `isLoading`, `loadPresetForShots` and `resetForShots` (`:308-329, 381-389, 480-531, 834-843, 1006-1057`): the shell draws the menus and the bar from the control state. What stays: the layout of AC-40 (the faceplate, the bar, the panel row), the host keys and the input method (`:579-646, 689-750`; LIB-5), the drop through `open` (M-48), Copy Screen (`:647-688`, a product item, D-12), and the SHIFT tap (LIB-5 AC-33) |
| `app/ControlBar.hpp/.cpp`, `app/NamedComboBox.hpp/.cpp` | deleted: the shell's bar (B1 to B7) with LIB-7's pickers |
| `app/SettingsDialog.hpp/.cpp` | becomes `app/SharpSettingsRows.hpp/.cpp`: the product's sections of AC-37 for `ShellSettingsDialog`. Folders with its four rows and labels (D-26); Serial ports with the link folder and the two status lines; Debugger with `[debug] dap-enabled` and `dap-port`, beside which C1600-3 puts its status and Disconnect (`SettingsDialog.cpp:306-383`). The rows of General, Startup presets, Storage and Tracing are the shell's (S1 to S7) |
| `app/AboutDialog.hpp/.cpp` | deleted; its texts (`AboutDialog.cpp:21-27`) are `ShellProduct::aboutLines` (D.5) |
| `app/Operations.hpp/.cpp` | deleted (adapter of §5): "Loading…" is the shell's overlay (D-6), a refusal is the shell's notice (D-7). The debugger's `runAndWait` (`Operations.hpp:31-38`) becomes a reply handler on `SessionHost::send` in `DebugController`, an adapter of M-45's that M-50 removes with the DAP move |
| `app/EmulationPacer.hpp/.cpp` | deleted: the frame timer is `SessionHost::start()`; `setTurbo` is the shell's H5 (`FaceplateInput` on the display area) |
| `app/FaceplateWidget.hpp/.cpp` | keeps the art, the pressed-key flash (from `held-keys`, LIB-5 AC-19) and the LCD widget. `mousePressEvent` and its hit test (`FaceplateWidget.cpp:79-107`) go: `Shell::setFaceplate(widget, hitMap, scale)` with the hit map of `app/KeyLayout` and the distance 24 (D-8) |
| `app/LcdWidget.hpp/.cpp` | paints the `display` topic's `DisplayOutput` through `app/LcdLayout` (C1600-1's adapter `toDisplayFrame` goes, D-28); `turboRequested` (`LcdWidget.cpp:48-53`) goes (H5) |
| `app/DebugPanel.*` (the INSPECTOR), `app/Ce158PrinterWidget.*` | product panels put into `Shell::panelArea()` (AC-23, AC-42): the INSPECTOR always, the CE-158's printer panel while the product entry `c1600.ce158-printer-panel` says so (LIB-4 Part D). `setDockedPane` (`MainWindow.cpp:511-521`) becomes the product's layout of the panel row (AC-40) |
| `app/MemoryModuleManager.*`, `app/FloppyDiskManager.*`, `app/TapeManager.*` | deleted, as far as M-47 left them as thin wrappers: the pickers are LIB-7's components, and the disk controls are a product control (below) |
| `app/main.cpp` | `--shots*` only with `CALCU_ATTACH` (D.6); `--pacing-probe` (`main.cpp:57-99, 138-159`) becomes an attach client (D.6); `--attach-port` is read by `SessionHost` |
| `app/screenshots/ShotRunner.*`, `ShotScenario.*` | the shot runner as an attach client (D.6) |
| `app/AppLogging.*` | the shell's G1 (`ShellOptions::logToPlatform`) routes into it as today |
| `CMakeLists.txt` | links `calcu_qt6`; `app/screenshots/*` only `if(CALCU_ATTACH)`; the deleted files out; the new `tests/` (D.8) |

**The disk and tape controls (AC-36, AC-42).** The floppy disk controls
are shown on every PC-1600 and can be used only while the CE-1600P is
plugged in (the product entry `c1600.disk-controls`, LIB-4 Part D). The
shell would draw LIB-7's controls for the media place `floppy` only
while the control state lists it (LIB-4 EC-4). So the product registers
no shell control for the kind `floppy-disk`; it puts its own disk
control (LIB-7's `MediaPicker` for `floppy`, the side toggle, Name &
Save) into `BarGroup::Media` with `addBarControl`, drawn from the
product entry and disabled with its reason while the CE-1600P is not
plugged in (D-29). If `registerMediaControls` does not take a factory
that gives no widget, `ShellControlBar` gets that one-line change in
M-49. The cassette controls follow the family rule (B1): LIB-7's tape
controls with the product's counter and motor lamp (`c1600.lamps`)
through `registerMediaControls("cassette", …)`, shown while a
peripheral with a cassette is plugged in.

### D.4 The Web frontend (`products/calc-u-1600/frontends/web/`)

| File | After M-49 |
|---|---|
| `src/CalcU1600.jsx` | composes the React set: `useCalcuShell(shell, product, {hasQuit: false, shortcuts: false, savedMedia: true})`, `ShellMenuBar`, the faceplate with `useShellFaceplate`, `ShellControlBar` with the disk control in `extra` (Media), `ShellPanelArea` with the INSPECTOR and the CE-158's printer panel, `ShellDialogs`. Gone: the menu table (`CalcU1600.jsx:243-298`), the page's shortcuts for Reset, Cold boot, Settings and Copy Screen (`:310-321`; AC-4, D-30), the "stopped" modal (`:410-417`; AC-19) |
| `src/MenuBar.jsx`, `src/ControlBar.jsx` | deleted |
| `src/Dialogs.jsx` | `Modal`, `NoticeDialog`, `SettingsDialog`, `AboutDialog` and `BusyPopup` (`Dialogs.jsx:7-37, 189-251, 300-310`) give way to `ShellModal`, `ShellNotice`, `ShellSettings`, `ShellAbout` and `ShellLoading`. What stays is the product's: C1600-2's machine-code dialogs (M-44) |
| `src/Faceplate.jsx` | keeps the art and `LcdCanvas`; its hit test and `TOUCH_SLOP_PX` (`Faceplate.jsx:18, 77-115`) give way to `useShellFaceplate` with the hit map of the request `c1600.faceplate-map` (D-31) |
| `src/Panels.jsx` | `Inspector` and `Ce158Printer` stay as product panels in `ShellPanelArea`; `Panel` and `PillMenu` stay as their helpers |
| `src/engine.js` | deleted (LIB-15 1600's adapter): the page talks to the shell (`createShell`, `useTopic`), sends the library's commands, and asks the product's requests by name |
| `src/painters.js` | the LCD painter reads the `display` topic's value (dots, indicators) with the layout of `c1600.faceplate-map` |
| `products/calc-u-1600/app/SharpWebPart.hpp/.cpp` | gone: the topic `c1600.ui` (`SharpWebPart.cpp:142, 412-425`), `c1600.lcd`, the requests `c1600.faceplate` and `c1600.lcd-constants` (`:397-402`; LIB-15's W16-1), `c1600.settings-state` and the commands `c1600.set-startup-model`, `c1600.set-default-preset` (`:367-395`; replaced by `SettingsPart`). Stay: `c1600.lamps`, `c1600.printer`, `c1600.inspector-menu`, `c1600.inspector-view`, `c1600.program` (C1600-2); new: `c1600.faceplate-map` and `c1600.shell-product` |
| `bindings.cpp` | adds `SettingsPart` to the parts (`bindings.cpp:44-48`) and tells `SharpAppPart` that the page cannot mount a folder (LIB-4 Part D, AC-18) |
| `tests/` | `shell.spec.js` (new, D.8); `play.spec.js`, `typing.spec.js`, `session.spec.js` and `rom-missing.spec.js` re-pointed to the object names of §3.10 |

### D.5 What Calc-U-1600 gives the shell (`app/SharpShellProduct.hpp/.cpp`, new)

One source for Qt6 and the Web (D-32): `shared::shellProduct()` gives
the data of `ShellProduct`. The Qt6 frontend adds the acts of the
product items; the page gets the data through the request
`c1600.shell-product` and adds its own JS acts.

| Field | Value |
|---|---|
| `name`, `version`, `build` | "Calc-U-1600", `APP_VERSION`, `APP_BUILD_ID` (Qt6); the page's version on the Web |
| `helpUrl` | `https://calc-u.org/1600/` (AC-39) |
| `aboutLines` | "A Sharp PC-1500/1500A/1600 pocket-computer emulator."; "See LICENSE, NOTICE and THIRD-PARTY-NOTICES.md, shipped alongside this app, for licensing details." (`AboutDialog.cpp:21-25`) |
| `variantLabels` | `PC-1500` → "ROM Revision", `PC-1600` → "ROM Version", `CE-1600P` → "CE-1600P ROM" (AC-34) |
| `presetFilter` | "Presets (*.pc1500 *.pc1500a *.pc1600)" |
| `copyScreen`, `pasteText` | true, true |
| `moduleSlotPinOuts` | the three memory slot pin-outs (`core/Machine/SharpCatalogue.cpp:77, 107-108`) |
| `placesWithoutControl` | `host-drive` (AC-9) |
| `items` | FileKinds: "Load BASIC Program…", "Load Machine Code…" (C1600-2); FileMedia (Qt6 only): "Host Drive…" (entry `c1600.host-drive`), "Unmount" (entry `c1600.host-drive-folder`); Edit: "Copy Screen", which reads "Copy Log Selection" while text of the INSPECTOR is selected (Qt6) |

`checkShellProduct()` runs in `lib14_ac34_shell_product_complete` (D.8).

Departure (Part D simplification S-5 (2026-11-16)): the Value forms the page reads are the
library's, `library/shell/ValueForms.hpp/.cpp` (`calcu::shell::toValue` of
`ShellProduct`, `DialogSpec`, `HitMap` and `Rect`; tests
`library/shell/tests/value_forms_tests.cpp`), so Calc-U-59's Web part need
not write them again. `SharpShellProduct` keeps the data and the product's
part of `c1600.faceplate-map` (the art's size, the LCD layout) around the
library's hit-map form; the replies are unchanged byte for byte. The form
of `ShellProduct` leaves out `version`, `build` and `aboutLinks` and
always says `copyScreen: true`, as `shellProductValue` did.

### D.6 Harness attach and the screenshot scenarios (AC-44)

**The attached registry (library; D-27).** `AttachedHarness` registers
the same commands as `Server::start` (`Run.cpp:260-267`) for the session
it is attached to: the first, startup, preset, typing, paper, debug,
media and open commands, then the product's. They find the session's
parts (the `PresetsPart`, `MediaPart` and `TypingPart` the product
built) through a small lookup, `harness::SessionParts`, which
`SessionHost::openAttach` fills from the frontend's session. The bare
`wait` (LIB-6 AC-34) becomes a pending reply on the app's clock, like
the timed one: it ends when the busy topic and the request `typing` say
idle, after at most one hour of emulated time. Library tests:
`attach_registry_matches_serve`, `attach_bare_wait_ends_when_idle`
(`library/harness/attach/tests/`), `qt_attach_host_preset_key_type`
(`library/qt6/tests/attach_host_tests.cpp`). C59-5's Swift attach gains
the same commands; its tests stay green.

**The shot runner (`app/screenshots/ShotRunner.*`).** It opens attach
on port 0 inside its own process (`SessionHost::openAttach(product, 0)`)
and talks to it through a `QTcpSocket` line client
(`app/screenshots/AttachLineClient.hpp/.cpp`, new; the serve protocol of
LIB-11 §3.9, message lines skipped). Machine steps go through attach,
UI steps through the widgets found by their object names:

| Step (README) | Today (`ShotRunner.cpp`) | After M-49 |
|---|---|---|
| `preset: <file>` | `loadPresetForShots` (`:234`) | `preset <path>` |
| `reset`, `reset: all` | `resetForShots` (`:239`) | `reset`, `cold-boot`; `reset: all` is renamed `cold-boot` in the scenarios |
| `key: <name>` | a faceplate press and `runFor` (`:245-250`) | `key <name>` |
| `hold-key: <name>`, `release` | a faceplate press and a frozen run (`:222-253`) | a pointer down and up on the faceplate at the key's centre in the hit map (a UI step: the shell's live input), with `wait 100ms` after the press (D-33) |
| `type: <text>` | `pasteText`, ENTER and a run (`:255-265`) | `type <text>` |
| `run: <seconds>` | `pacer()->runFor` (`:267`) | `wait <seconds>s` |
| `wait-idle` (new) | — | the bare `wait`; every shot ends with it before its captures |
| `settle`, `click`, `open`, `select`, `action`, `menu`, `close`, `choose-file`, `enter-text`, `accept`, `capture` | widgets | unchanged, with the names of §3.10 (`bar-model`, `bar-slot-slot-1`, `item-settings`, …) in place of `controlbar.model` and the like; `action: Edit > Settings…` becomes `File > Settings…` |

The runner uses neither `MainWindow::pacer()` nor `controller()` nor a
frozen mode; the CTest `CalcU1600ShotsThroughAttach` (D.8) keeps it so.

**The pacing probe (LIB-17 §5).** `--pacing-probe <s>` stays as an
option but becomes an attach client in its own process, as the runner
is: it reads `ticks` through attach after the start and again after `s`
seconds of wall time, and prints today's line (`main.cpp:87`). The
Checkpoint entry `CalcU1600QtPacingProbe` is unchanged.

**Builds.** The runner, the line client and the probe are compiled only
with `CALCU_ATTACH` (POSIX trees with `CALCU_TESTS`, D-5). The tree of
`tools/run_qt.sh 1600` (`build/app-1600`; was `tools/build_app.sh`) has it; release builds
and Windows have neither `--shots` nor `--pacing-probe`.

### D.7 Adapters removed in M-49

| Adapter | Where | Named by |
|---|---|---|
| the `Operations` popup and the wait cursor | `frontends/qt6/app/Operations.*` | §5; LIB-3 §4 |
| `--pacing-probe` as a test hook outside attach | `frontends/qt6/app/main.cpp:57-99` | LIB-17 §5 (it becomes an attach client) |
| the frozen mode | `MachineController.cpp:100-137`, `EmulationPacer.*` | §5 |
| W16-1, the page data requests `c1600.faceplate`, `c1600.lcd-constants` | `app/SharpWebPart.cpp:397-402` | LIB-15 §5 (the hit map stays, as `c1600.faceplate-map`; D-22, D-31) |
| `src/engine.js` | `frontends/web/src/engine.js` | LIB-15 1600 |
| the interim topic `c1600.control-state` | `app/SharpTopics.hpp:26, 54-60` | LIB-3 §5, LIB-4 §5 |
| the settings commands of `SharpWebPart` | `app/SharpWebPart.cpp:367-395` | §5 |
| A-4, the frontends' own "ROMs missing" | `MachineController.cpp:44-53` (a message box, then exit); the Web's "stopped" modal | LIB-12 §5 |
| `toDisplayFrame` (`displayFrameOf`, `shared::DisplayFrame`) | `app/DisplayFrame.hpp`, `app/SharpAppPart.hpp:62` | C1600-1 §5 |
| `modelIdOf`, `modelOf`, `variantIdOf` with `shared::Model`, `PC1600RomVersion`, `CE1600PRomVersion`, `modelKey` | `app/MachineModel.hpp` and its users (D-28) | C1600-1 §5; LIB-1 §5 |
| what M-46 and M-47 left of the interim topic `c1600.machine` | `app/SharpTopics.hpp:62-76` | LIB-3 §5 (LIB-4 and LIB-18 1600) |

### D.8 Tasks

Tests come before the code. A new test program for the Qt6 frontend:
`CalcU1600QtTests` (`frontends/qt6/tests/`, the one runner, offscreen,
built only with Qt; Frequent, about 5 s with the 500 ms waits). It runs
the real window with the session in `Hosting::Paced` on a scripted wall
clock and the choosers of `Shell::setChoosers` as fakes. The Web cases
are in `frontends/web/tests/shell.spec.js` (Chromium and WebKit; Longer,
in `CalcU1600WebPlaywright`). Case names are `lib14_acN_…` (product
rows, LIB-1 §2a); each Qt6 case and its Web case carry the scenario ID
of AC-25.

| Task | What | ACs | Test (program: cases) |
|---|---|---|---|
| Q1 | `SharpShellProduct`; the session host in `MachineController` | (support); AC-39 | `CoreTests`, `app/tests/shell_product_tests.cpp`: `lib14_ac34_shell_product_complete` (`checkShellProduct` against each model's control state), `lib14_ac39_help_address_and_about_texts` |
| Q2 | Qt6 menus | AC-34; AC-1, AC-4, AC-6 for this product | `CalcU1600QtTests`: `lib14_ac34_qt_menus_in_family_order` (File, Edit, Machine, Help and their items per model; ROM Revision only on the PC-1500, ROM Version and CE-1600P ROM on the PC-1600; Cold boot; Resume), `lib14_ac34_copy_log_selection_while_inspector_text_selected`, `lib14_ac34_shortcuts_ctrl_o_r_shift_r`. The macOS application menu is seen in the shot `02-machine-menu` (Longer) |
| Q3 | Web menus | AC-35 | `shell.spec.js`: `lib14_ac35 the web menus` (no Host Drive…, Unmount, Quit; Import Media… and Saved Media…; Load Preset… and Examples ▸ as two items), `lib14_ac35 no shortcut is bound` |
| Q4 | The bar per model | AC-36; AC-8 to AC-10 | `CalcU1600QtTests`: `lib14_ac36_bar_pc1500_pc1500a` (the model picker, the slot picker without a number and with Name & Save, the buttons CE-150 and CE-158, the tape controls only with a CE-150), `lib14_ac36_bar_pc1600` (the slot pickers "1:" and "2:"; the printer picker with None, CE-150, CE-1600P; the CE-158 button; the disk controls, disabled with the reason until the CE-1600P is plugged in; the tape controls with a cassette); `shell.spec.js`: `lib14_ac36 the bar per model` |
| Q5 | Settings | AC-37; AC-11 to AC-13 | `CalcU1600QtTests`: `lib14_ac37_settings_sections_and_rows` (General: Startup model; Startup presets: three rows; Storage: Save folder, Tapes; Folders: four rows; Tracing: two; Serial ports: the link folder and two status lines, not on Windows; Debugger); `shell.spec.js`: `lib14_ac37 the web settings` (General, Startup presets, the sentence on saved media) |
| Q6 | "Loading…" and notices | AC-38; AC-15 to AC-17 | `CalcU1600QtTests`: `lib14_ac38_loading_after_500_ms_blocks_nothing` (a preset at full speed: nothing at 300 ms, the overlay at 600 ms; the model picker, Reset and Cold boot enabled; Reset ends it), `lib14_ac38_notice_without_title` |
| Q7 | Help and About | AC-39 | `CalcU1600QtTests`: `lib14_ac39_help_opens_calc_u_org_1600`, `lib14_ac39_about_as_today`; `shell.spec.js`: `lib14_ac39 help and about` |
| Q8 | The layout | AC-40 | `CalcU1600QtTests`: `lib14_ac40_layout_faceplate_bar_panel_row` (the order; the paper panel always; the CE-158's panel only with a CE-158); `shell.spec.js`: `lib14_ac40 panels only with room` (two viewport sizes). One picture per layout (AC-25): the shots `01-app-overview` and `02-control-bar-pc1600` (Longer) |
| Q9 | Copy Screen | AC-41 | `CalcU1600QtTests`: `lib14_ac41_copy_screen_picture_and_text` (the fake clipboard gets the PNG and the text of `LcdText::plainText`); `shell.spec.js`: `lib14_ac41 copy screen` |
| Q10 | The faceplate's input | AC-24, AC-43 for this product | `CalcU1600QtTests`: `lib14_ac24_key_under_pointer_pc1500_pc1600` (key centres; a point in a gap within 24 px; touch with Qt Test), `lib14_ac43_display_hold_full_speed`; `shell.spec.js`: `lib14_ac24 a key under the pointer` |
| Q11 | The attached registry, the bare `wait` | (for AC-44; library) | `calcu_library_tests`: `attach_registry_matches_serve`, `attach_bare_wait_ends_when_idle`; `calcu_qt6_tests`: `qt_attach_host_preset_key_type`; Frequent |
| Q12 | The shot runner on attach | AC-44 | `CalcU1600QtTests`: `lib14_ac44_scenario_through_attach` (two shots with `preset`, `key`, `type`, `wait-idle` and a widget capture; nothing else steps the machine); the CTest `CalcU1600ShotsThroughAttach` (`cmake -P frontends/qt6/tests/check_shots_through_attach.cmake`: `ShotRunner.cpp` names no `controller()`, `pacer()`, `setFrozen`, `runFor`, `pasteText`; Frequent); the nine `CalcU1600Shots.*` scenarios upgraded (Longer) |
| Q13 | ROMs missing | AC-19 for this product | `CalcU1600QtTests`: `lib14_ac19_roms_missing_in_window` (a ROM folder without one file: the text in place of the faceplate, no bar, no Machine menu, About works, the app runs on); `rom-missing.spec.js` re-pointed |
| Q14 | LIB-3 1600 Done | LIB-3 AC-38 | `CoreTests`: `lib3_ac38_mirror_holds_every_callback` re-pointed to the owners' names (`control-state` for `presetArmed` and the model, `media`, `run-state`, `notices`, `log`, `busy`, `roms-missing`, LIB-2's serial link topic) |
| Q15 | C1600-2 Done | C1600-2 AC-15 | `CalcU1600QtTests`: `c1600_2_ac15_file_items_and_filters` (the two items in FileKinds, the filters, the start folders of `[folders]`); `shell.spec.js`: `c1600_2_ac15 the file items` |

Coverage: AC-34 to AC-41 and AC-44 in Q2 to Q12, with AC-19, AC-24 and
AC-43 for this product. The Longer group runs at the end of Part D with
the upgraded `CalcU1600Shots.*` and Playwright.

### D.9 Ledger rows

Visible from M-49: 1600-10 (menus and bar), 1600-14 (its "Copied to
clipboard" and Resume parts), 1600-27 (the operation's title gone from
messages), 1600-33 ("Loading…" without a title; controls disabled while
something loads). New rows M-49 adds to `docs/behaviour-changes.md` in
the same step, consequences of the spec that the ledger lacks:

- **1600-35.** Web: the page binds no shortcut to a menu item: ⌘R /
  Ctrl+R, ⌘⇧R, ⌘, and ⌘C no longer act there (they are the browser's);
  pasting still types (the browser's paste). Qt6: Ctrl+O (⌘O) is Load
  Preset…. The startup model is chosen by the model's name ("PC-1500";
  today "PC-1500 (ROM A04)"). Sources: LIB-14 AC-4, AC-11; D-30.
- **1600-36.** When a ROM file is missing, the Qt6 window opens and
  shows which files are missing in place of the calculator, and Help and
  About work (today a message box, then the app quits). On the Web the
  same text stands in place of the calculator, without the title
  "Calc-U-1600 stopped". Sources: LIB-14 AC-19, LIB-12 AC-5.
- **1600-37.** A mouse click in the gap between two keys, within 24
  pixels of a key, presses that key, as a touch does on the Web today
  (today a click must hit the key); the Qt6 faceplate also takes touch.
  Sources: LIB-14 AC-24, D-8. OQ-5 called such a change too small to
  list; it is listed because it is seen with a mouse.

### D.10 Tools and agent knowledge (changed in the same step)

| File | Change |
|---|---|
| `products/calc-u-1600/docs/developer/screenshots/README.md` | the runner on attach (D.6): the step table, `cold-boot`, `wait-idle`, the object names, `File > Settings…`; "Frozen emulation" becomes "captures after the machine is idle" |
| `products/calc-u-1600/docs/developer/screenshots/guide/*.shots.yaml` | the steps and object names as above |
| `products/calc-u-1600/docs/developer/Shared-Architecture.md`, `Web-Client.md` | the shell, `SessionHost`, `SharpShellProduct` and the control state; `ControlState`, `controlStateOf`, `engine.js` and `Operations` gone |
| `products/calc-u-1600/docs/background/Decisions.md` | the entry on ROMs and non-native peripherals in the Machine menu only gets a line: superseded by LIB-14 AC-9 (the bar by the family rule) |
| `products/calc-u-1600/CLAUDE.md` | "Every frontend hosts…": the Qt6 frontend through `calcu::qt6::SessionHost` and the shell, `MachineController` as the product's glue; the Web page on the React set; `--shots` an attach client |
| `docs/glossary-mapping.md` | visible renames for Calc-U-1600 done (Cold boot, Startup preset, Save folder, Startup model, Examples folder); code names: Model (`shared::Model` gone), Shell |
| `docs/test-groups.md` | `CalcU1600QtTests`, `CalcU1600ShotsThroughAttach`, the new library cases, `shell.spec.js` |
| memory `calc-u-1600/project_scripted_screenshots.md` | "frozen, byte-identical" becomes "through attach, captured after `wait-idle`"; the steps |
| memory `calc-u-1600/feedback_machine_config_on_control_bar.md` | the bar follows LIB-14 AC-9 (the printer picker and the CE-158 on the PC-1600's bar); the 2026-10-04 exception is superseded |
| memory `calc-u-1600/project_dap_debugger_plan.md` | "the window's Operations" becomes the shell; `runAndWait` gone |
| memory `calc-u-1600/feedback_gui_test_launch_quit.md` | scripted launches of the GUI use `--shots` or attach; the quit rule stays until M-50 |
| `features/LIB-1-machine-interface-and-descriptors/design.md` §2a | `harness/attach` includes the other command areas of `harness` |

### D.11 Golden runs and risks

- **Golden runs** run no frontend. M-49 changes no core except the
  catalogue's pin-outs (LIB-4 Part D). The replays
  (`c1600_1_ac4_golden_runs_through_the_interface`) pass unchanged; a
  difference is a finding. The reference runs of M-4 are compared in
  M-46, not here.
- **Shots on the app's clock.** Captures are no longer byte-identical
  from run to run (a cursor blinks, a clock runs). Every shot ends with
  `wait-idle` before its captures; the Longer checks compare that every
  image is written, as today. The help's pictures (LIB-16 AC-12, M-52)
  are taken from these runs. A shot that shows a blinking cursor may
  need a `wait` that lands on the same phase; to be seen in M-49.
- **Run time of the scenarios** grows: a preset runs in parts on the
  app's clock, and `run:` takes wall time. Measured in M-49 for
  `docs/test-groups.md`; Longer has room (40 s for the nine today).
- **The attached registry** changes C59-5's attach, which gains
  commands. Its tests stay; the Swift XCUITests run in Part E.
- **CI.** M-49 changes the build of the Qt6 and Web frontends (new
  links, files gone, a new test program): one CI dispatch at the merge
  (plan §1).

### D.12 Decided (best guess)

- **D-25.** M-49 takes over any frontend piece that M-44 to M-48 leave
  undone; their designs say what they do.
- **D-26 (AC-37).** Calc-U-1600 gives its own Folders section with
  today's labels and "Samples" renamed: "Examples folder", "Basic
  folder" ("Presets folder", "BASIC folder" since the release
  simplification, ledger 1600-54), "Assembly folder", "Host drive folder". Storage keeps "Tapes" ("Cassettes" since release prep step 2)
  beside "Save folder".
- **D-27 (AC-44).** The attached harness registers the serve form's
  commands, and its bare `wait` ends at idle on the app's clock. It is a
  library change in a product step because AC-44 is its first user.
- **D-28 (C1600-1 §5).** `shared::Model` and its ROM version enums go:
  `app/` uses `ModelId` and `VariantId`, the frontends the control
  state. `KeyLayout::layoutFor`, `LcdLayout` and the faceplate art are
  chosen by `ModelId`. The users left after M-48 change in this step;
  today they are `app/ControlState.*`, `KeyLayout/FaceplateLayout.cpp`,
  `LcdLayout.cpp`, `Settings.cpp`, `SharpAppPart.*`, `SharpTopics.hpp`,
  `SharpWebPart.cpp`, `PresetCatalog.*`, `CardYamlDump.cpp`,
  `MachineCodeLoad.cpp`, the Qt6 `ControlBar`, `DebugPanel`,
  `FaceplateWidget`, `LcdWidget`, `MainWindow`, `SettingsDialog`,
  `MachineController.hpp`, and the tests of `app/tests/` and `tests/`
  that name `Model::`.
- **D-29 (AC-36, AC-42).** The disk controls are one product control
  on the bar of every PC-1600, not the shell's media control, so they
  do not appear twice once the CE-1600P is plugged in.
- **D-30 (AC-4).** The Web page drops all its menu shortcuts, ⌘C
  included; the paste event stays (LIB-5, not a menu binding). Ledger
  1600-35.
- **D-31 (D-22).** The Web's static faceplate data is one product
  request, `c1600.faceplate-map {model}`: the shell's hit map, the
  LCD's place and the layout of its indicators, from the tables Qt6
  uses (`app/KeyLayout`, `app/LcdLayout`).
- **D-32.** The shell data of §3.2 is written once in C++
  (`SharpShellProduct`) and sent to the page by `c1600.shell-product`,
  so the two frontends cannot disagree on labels and addresses.
- **D-33 (AC-44).** `hold-key` stays a UI step (a pointer on the
  faceplate), because the pressed-key flash is drawn from live input;
  the machine is stepped only through attach.

### D.13 Plan notes (proposals; the plan is not edited)

- M-49 "Waits for" should also name M-44, M-45 and M-48 (the File
  items; the paper panel and Resume; `open`). Today it names M-43,
  M-46 and M-47.
- M-49 "Does" gains: the attached harness's full command set and the
  bare `wait` (D-27); the Sharp 60-pin places with a pin-out each (LIB-4
  Part D, O-2); `shared::Model` gone (C1600-1 §5). "Completes" gains
  LIB-3 1600 and C1600-2, which §5.1 already has Done in M-49.
- M-49 "Knowledge, ledger" gains the memory files of D.10 and the new
  rows 1600-35 to 1600-37.

### D.14 Questions for the user

None that split the design.

## 10. Part E: the Calc-U-59 column

Status: approved by the user at the Part D/E boundary (2026-11-16)

Step M-57, with LIB-4 59 (§10 of LIB-4's design holds the control
state's product side; this section holds the frontends and what the
library gains for them). Tests from LIB-14 AC-26–33; Done in M-57 (plan
§5.1). The Swift frontend is C59-8 (M-58); it draws the same models
that this section adds (the More button, the labelled choices) through
its bridge. Paths are at `7bdd51c`; product paths are under
`products/calc-u-59/` unless named otherwise.

### 10.1 Scope, and what the earlier Part E steps leave

M-57 puts both Calc-U-59 frontends together from the shell. It comes
after M-53 to M-56 and takes what their designs say as done:

| Step | What M-57 finds | What M-57 uses it for |
|---|---|---|
| M-53 (LIB-8, LIB-10 59) | the library's `PaperPanel` / `CalcuPaperPanel` for the PC-100C in Qt6 (in place of `frontends/qt6/app/PrinterWidget.*`) and on the Web (in place of `src/Printer.jsx`); `run-state` and the run-control buttons in the debug panel | the paper panel through `Shell::registerPanel` (its default); Machine ▸ Resume (M7) |
| M-54 (LIB-6, LIB-18, C59-2, C59-3) | the library's commands `model`, `attach`, `detach`, `reset`, `cold-boot`, `load-preset`; `preset-list` with C59-2's examples and the three startup presets; `TIAppPart` on `parts::LibraryParts` | every menu item and bar control sends a library command (D-24); Examples ▸ and the startup preset rows from `preset-list` |
| M-55 (LIB-7 59, C59-7) | `media` with the kind `magnetic-card`; LIB-7's `MediaPicker` with the TI labels of LIB-7 AC-8; C59-7's card controls (a Qt6 widget and a React component) and its two card items with their dialogs (today `frontends/qt6/app/CardPickerDialog.*`, `frontends/web/src/Dialogs.jsx` `CardPicker`); the Web's saved media for cards (LIB-7 AC-36) | the slot picker (`registerSlotPicker`, its default); the card controls through `registerMediaControls("magnetic-card", …)`; the card items as product items (10.6) |
| M-56 (LIB-13 59, C59-4) | `open` for drops, Open With and the start file (`opening::startFile`, row 59-47); C59-4's debug panel with its product entries | the window's drop sends `open`; the debug panel as a product panel beside the paper (AC-33) |

What is left for M-57, in both frontends: the window structure (menus,
bar, the More button, settings, About, Help, "Loading…", notices, the
faceplate input, "ROMs missing", the storage banner); the session host
in Qt6; Qt6 attach and the pacing probe as an attach client; the library
additions of 10.3. If a step of M-53 to M-56 leaves one of its frontend
pieces undone, M-57 takes it over and the qa-report says so (D-25).

Not in M-57: the Swift frontend (C59-8, M-58); the harness product
commands (C59-9, M-59); the help (LIB-16 59, M-60).

### 10.2 The library's code against the approved text

Read from `library/` at `7bdd51c`. Where Calc-U-59 needs what the code
does not have yet:

1. **No model for a frontend without a menu bar.** D-15 promised the
   More button "on the same menu model" in M-57. `Traits` has only
   `hasQuit`, `shortcuts`, `savedMedia` (`library/shell/MenuModel.hpp:38-42`);
   a bar control is a picker, a button or a medium
   (`library/shell/BarModel.hpp:17-26`), and no group comes before the
   model picker (`library/shell/ShellProduct.hpp:21`).
2. **Choices without labels.** A choice row offers each stored value as
   its own label (`library/shell/SettingsModel.cpp:104-106`). Calc-U-59
   shows "Modernized", "Classic" for `modernized`, `classic`, and the
   Web offers only "Off" and "Click Sound" of the three values of
   `[keyboard] feedback` (`app/Settings.cpp:49-52`;
   `frontends/web/src/Dialogs.jsx:177-183`).
3. **The page's form of `ShellProduct`** leaves out `aboutLinks` and
   always says `copyScreen: true` (`library/shell/ValueForms.hpp:15-20`).
   Calc-U-59 has no Copy Screen (AC-26) and three About links (AC-32;
   `docs/reviews/d-simplification.md` §9, M-57).
4. **A product item always appears** (`library/shell/MenuModel.cpp:51-67`);
   the card items must not appear on the TI-58 and TI-58C (LIB-4 AC-12).
5. **The faceplate input** takes widget coordinates without an origin
   and consumes every left press (`library/qt6/FaceplateInput.cpp:22-44`;
   `library/web/components/useShellFaceplate.jsx:14`). Calc-U-59 draws
   its faceplate centred in the widget (`frontends/qt6/app/KeyboardWidget.cpp:46-52`),
   and a press on the fitted card feeds it before any key
   (`KeyboardWidget.cpp:87-94`; C59-7).
6. **About** has no icon (`library/qt6/ShellAbout.cpp`); Calc-U-59's Qt6
   About shows the app's icon today (`frontends/qt6/app/AboutDialog.cpp:10-13`).
7. **The storage banner** is Calc-U-1600's own sentence
   (`products/calc-u-1600/frontends/web/src/CalcU1600.jsx:490-494`)
   though `web.storage` carries the library's (`library/web/src/storage.js:48-53`,
   `sentences.js:12-17`): the user's Q-4 (`docs/reviews/d-simplification.md`
   §10), ledger 1600-48, "with M-57's banner or before M-61".
8. Part D's D.2 holds as written (`useCalcuShell`, `remember-folder`,
   the panel model in `BarModel.*`, the attached registry of D-27).

### 10.3 What the library gains in M-57

#### 10.3.1 A window without a menu bar: the More button (AC-29; C59-8 AC-5)

One model for the Web's portrait layout and for iOS: the C++ model is
what C59-8's bridge calls (C59-8 §3.1), and `shellModel.js` mirrors it
rule by rule for the React set.

```cpp
// library/shell/MenuModel.hpp, changed
struct Traits {
    bool hasQuit = true, shortcuts = true, savedMedia = false;
    bool menuBar = true;    // false: the Web's portrait layout, iOS (M13)
    bool settings = true;   // false: the Web prop `settings` off (LIB-15 AC-19): no Settings… (M18)
    bool about = true;      // false: iOS, which has no About (C59-8 AC-11)
};
struct MenuItem {
    // … as today, and:
    std::string confirm;        // a question asked before the item is carried out; empty: none (M15)
    bool destructive = false;   // shown as such where the platform has it (iOS; C59-8 AC-5)
};
// The items of the More button, in menu order (M14).
std::vector<MenuItem> moreItems(const std::vector<Menu>&, const ShellProduct&);

// library/shell/ShellProduct.hpp, ProductItem, added
bool onBar = false;        // the product draws its own control for it on the bar (M14)
std::string mediaPlace;    // the item exists while this media place is listed and follows its `insert` (M17)

// library/shell/BarModel.hpp, changed
enum class BarGroup { Start, AfterModel, Slots, Places, Media, End };   // Start: before the model picker (B8)
struct BarControl {
    enum class Kind { ModelPicker, SlotPicker, PlaceButton, PlacePicker, Media, Action, More };
    // … as today, and:
    MenuItem item;            // Action: the menu item it carries out (id, label, enabled, reason, command)
};
std::vector<BarGroupModel> bar(const ShellInputs&, const ShellProduct&, Traits = {});
```

| # | Rule | AC |
|---|---|---|
| M13 | With `menuBar == false` the menus are still computed (for the More button, and for the iPad's system menu bar, C59-8 EC-6), but the frontend draws no menu bar, and the bar follows B8 | AC-29; C59-8 AC-5 |
| M14 | `moreItems`: the items of File, Edit, Machine and Help in that order, leaving out those that have a control on a bar without a menu bar: `load-preset`, `settings`, `quit`, `reset`, the `model` submenu (the model picker), the `peripherals` submenu (the place controls) and every product item with `onBar`. Separators stay between the groups that remain, never first, last or twice. For Calc-U-59 on the Web: Examples ▸; Import Media… and Saved Media… (while `media` lists a kind, M2); Cold boot; Resume; Calc-U-59 Help; About Calc-U-59 (D-36) | AC-29 |
| M15 | In the More menu, Cold boot carries `confirm` "Cold boot the machine? What it holds in memory is lost." and `destructive`. Choosing it shows the question with the buttons Cold boot and Cancel; Cold boot sends `cold-boot`, Cancel sends nothing (the form of N4) | AC-29; C59-8 AC-5 |
| M16 | The items of the More menu follow availability like menu items (M9) and are computed again at every change of an input, also while the menu is open | AC-29; C59-8 EC-1 |
| M17 | A product item with `mediaPlace` is listed only while the control state lists that media place, and is enabled exactly when its `insert` is available, with its reason (M9) | LIB-4 AC-12; AC-7 |
| M18 | With `settings == false` there is no Settings… item and no Settings button | AC-29 |
| B8 | With `menuBar == false`: Start holds an Action for `reset` ("Reset"); End holds the Actions `load-preset` ("Preset", today's label) and `settings` ("Settings"), then More ("More", the ellipsis) whose `choices` are `moreItems`. Each Action follows its menu item (M9). With a menu bar, B1 to B7 as before: no Start, no Action, no More | AC-29 |
| B9 | Object names (`library/shell/ObjectNames.hpp`): `bar-action-reset`, `bar-action-load-preset`, `bar-action-settings`, `bar-more`, and `more-item-<id>` for each item of the More menu | AC-25 |

Drawing in the React set: `ShellControlBar` takes a `compact` prop and
draws an Action as a button with the set's icon for its id (↺ for
`reset`, ▤ for `load-preset`, ⚙ for `settings`, ⋯ for More; today's
characters, `frontends/web/src/ControlBar.jsx:68, 113, 116`) and its
label unless `compact`; More opens `ShellMoreMenu` (new), a popup of the
menu bar's `ShellMenuItems` (`library/web/components/ShellMenuBar.jsx:10`)
anchored to the button; a `confirm` item opens `ShellModal` with the
question. The Qt6 set does not draw Action or More: no Qt6 frontend is
without a menu bar (D-35).

#### 10.3.2 Labelled choices (AC-30; C59-8 AC-7)

```cpp
// library/shell/SettingsModel.hpp, DialogRow, added
std::vector<std::pair<std::string, std::string>> choices;   // Choice: value, label; empty: every declared value as written
```

| # | Rule | AC |
|---|---|---|
| S8 | A choice row with `choices` offers exactly those values, with those labels, in that order; a value in the file that is not among them is shown as written, as S4 shows a startup preset that is none of the list (EC-3) | AC-30 |

`toValue(DialogSpec)` writes `choices: [[value, label], …]` only when
the list is not empty, so Calc-U-1600's `settingsSpec` reply does not
change. `shellModel.js` (`calcuShellDialog`) and `ShellSettings.jsx`
mirror it; `ShellSettingsDialog` fills its `QComboBox` from it.

#### 10.3.3 The product's data in the page's form

`library/shell/ValueForms.cpp`: `copyScreen` says whether the product
has the item; `aboutLinks` is written when there is one; a product
item's `onBar` and `mediaPlace` are written when set. Calc-U-1600's
`shellProduct()` data (`products/calc-u-1600/app/SharpShellProduct.cpp`)
gets a placeholder act for Copy Screen that each frontend replaces with
its own, so `c1600.shell-product` stays the same byte for byte (D-37).

#### 10.3.4 The faceplate's geometry

```cpp
// library/qt6/Shell.hpp, added beside today's setFaceplate (which stays, Calc-U-1600)
struct FaceplateGeometry {
    std::function<double()> scale;                                    // logical pixels per hit-map unit
    std::function<QPointF()> origin = {};                             // the hit map's (0, 0) in the widget; none: (0, 0)
    std::function<bool(double x, double y)> productFirst = {};        // a press the product takes, in hit-map units
};
void setFaceplate(QWidget*, shell::HitMap, FaceplateGeometry);
```

A press that `productFirst` takes is not consumed by `FaceplateInput`
(the filter returns false), so the product's widget gets it, and the
moves and the release of that pointer pass through as well; every other
press goes to the hit rule (H1 to H5) after the origin is subtracted.
The React hook takes the same as options:
`useShellFaceplate(shell, hitMap, ref, {scale, origin, productFirst})`
(a number or a function as the fourth argument keeps meaning `scale`).

#### 10.3.5 About and the storage banner

- `ShellAbout` shows `QApplication::windowIcon()` above the name when
  the application has one; the React set shows no icon, as Calc-U-59's
  Web About does not today (`frontends/web/src/Dialogs.jsx:200-209`; D-38).
- `library/web/components/ShellStorageBanner.jsx` (new, in
  `COMPONENT_SOURCES`): reads LIB-15's state topic `web.storage`
  (`{mode, sentence}`, posted by `library/web/src/worker.js:139-141`) and
  shows `sentence` in a `role="status"` line while `mode` is not `kept`;
  object name `storage-banner`. Calc-U-1600's page uses it in place of
  its own (`CalcU1600.jsx:490-494`), which puts ledger row 1600-48 in
  effect in M-57 (D-39).

#### 10.3.6 Files

| File | Change |
|---|---|
| `library/shell/MenuModel.hpp/.cpp` | `Traits`, `MenuItem::confirm`, `destructive`, `moreItems`, M13 to M18 |
| `library/shell/BarModel.hpp/.cpp` | `BarGroup::Start`, `Action`, `More`, `bar(…, Traits)`, B8 |
| `library/shell/ShellProduct.hpp/.cpp` | `ProductItem::onBar`, `mediaPlace`; `checkShellProduct` also reports a `mediaPlace` that no model's control state lists |
| `library/shell/SettingsModel.hpp/.cpp` | `DialogRow::choices`, S8 |
| `library/shell/ValueForms.cpp` | 10.3.3 |
| `library/shell/ObjectNames.hpp` | B9 |
| `library/qt6/Shell.hpp/.cpp`, `FaceplateInput.hpp/.cpp` | `FaceplateGeometry` |
| `library/qt6/ShellAbout.cpp`, `ShellSettingsDialog.cpp` | the icon; S8 |
| `library/web/components/shellModel.js`, `useShell.jsx`, `ShellControlBar.jsx`, `ShellMenuBar.jsx`, `ShellMoreMenu.jsx` (new), `ShellSettings.jsx`, `useShellFaceplate.jsx`, `ShellStorageBanner.jsx` (new), `shell.css` | the mirrors and the drawing |
| `library/web/build/dist.js` | the two new files in `COMPONENT_SOURCES` |
| `products/calc-u-1600/app/SharpShellProduct.cpp`, `frontends/qt6/app/MainWindow.cpp`, `frontends/web/src/CalcU1600.jsx` | the placeholder act for Copy Screen (10.3.3); the storage banner |

### 10.4 The Qt6 frontend (`frontends/qt6/`)

The window becomes a `QMainWindow` with a `calcu::qt6::Shell` over a
`calcu::qt6::SessionHost`, the faceplate, and today's three columns.

| File | After M-57 |
|---|---|
| `app/EmulatorController.hpp/.cpp` | Stays as the product's glue, thinner. It makes the session (`cu59::makeAppSession`, `EmulatorController.cpp:62-68`) and a `SessionHost` over it, with `SettingsPart` among the parts. Gone to the host: the frame timer and the listener (`:69-75`), `send`. Gone to the shell: the ROM dialog (`:55-61`, then `main.cpp:67` quits; LIB-12's A-4) and the notice as a titled warning (`:89-99`, `cu59::splitNotice`; N1). The log lines keep going to `qDebug` (`:100-104`) as the shell's G1. The model, module, printer and reset members (`EmulatorController.hpp:32-40, 65`) go: the shell sends the library's commands. What stays: the display, the cue card, the debug panel's topics and commands as C59-4 left them |
| `app/MainWindow.hpp/.cpp` | `buildMenus` (`MainWindow.cpp:53-76`) and `openPreset` (`:78-85`) go: the shell's menus and Load Preset… with the filter of 10.6. `loadCard` and `newCard` (`:89-99`) are C59-7's product items (10.6). The splitter layout stays (`:27-38`; AC-33): the calculator column (the faceplate and the shell's control bar), the shell's panel area holding the paper panel, and the debug panel. `restoreWindowState` and `saveWindowState` (`:119-143`) stay; the drop (`:145-156`) is M-56's `open` |
| `app/ControlBar.hpp/.cpp` | deleted (Reset with Ctrl for Clean, the model combo, the card buttons, Preset; `ControlBar.cpp:26-101`): the shell's bar with LIB-7's slot picker, the "PC-100C" button (B3) and C59-7's card controls |
| `app/CalculatorPanel.hpp/.cpp` | holds the faceplate and `Shell::controlBar()` (`CalculatorPanel.cpp:15-18`) |
| `app/SettingsDialog.hpp/.cpp` | deleted: `ShellSettingsDialog` with the spec of 10.6. The constant memory written at a save folder change (`SettingsDialog.cpp:78-81`) becomes the product's `SettingsPartSetup::changed` for `[storage] save-folder`, doing what C59-3's design (M-54) names for it. The module row and its apply on close (`:41-45, 136-140`) go (row 59-9) |
| `app/AboutDialog.hpp/.cpp` | deleted: its texts and links are `ShellProduct`'s (10.6); "Built with Qt (LGPL-3.0)." is an about line the Qt6 frontend adds (`AboutDialog.cpp:28`) |
| `app/AppSettings.hpp`, `AppPaths.*` | the start folders and their memory (`AppSettings.hpp:21-33`) go to `SettingsPart` (`remember-folder`) and `Shell::startFolder`; the save and trace folder overrides (`:35-39`) to its folder rows. What stays: the `[window]` numbers (LIB-9 AC-35) and the paths the session is made with |
| `app/KeyboardWidget.hpp/.cpp` | keeps the art, the LED and the cue card. The mouse handlers and `cu59::PointerTracker` (`KeyboardWidget.cpp:87-119`) go: `Shell::setFaceplate(widget, cu59::faceplateHitMap(model), {scale, origin, productFirst})`, `origin` the canvas's corner (`:46-52`), `productFirst` C59-7's card tap (`:92`). The pressed key is drawn from `held-keys` (LIB-5 AC-19) alone (`:75`). The host keys go through `calcu::qt6::HostKeys` (`:121-160`); the TI keyboard map already refuses a key with Control, Alt or Command (`app/TITyping.cpp:156`), so the frontend's own check (`KeyboardWidget.cpp:136`) goes with no change seen (D-40). The LED style is read from the `settings` topic, not from `AppSettings` (`:37-40`) |
| `app/main.cpp` | `--pacing-probe` (`main.cpp:83-85, 97-127`) becomes an attach client (10.7); the start file is M-56's; `--grab` stays |
| `app/HoldRepeatButton.*`, `PrinterWidget.*`, `CardPickerDialog.*` | M-53's and M-55's, as their designs say |
| `CMakeLists.txt` | links `calcu_qt6`; `CALCU59_PACING_PROBE` gives way to `CALCU_ATTACH` (`CMakeLists.txt:129-140`); the deleted files out; `tests/` (10.9) |

### 10.5 The Web frontend (`frontends/web/`)

| File | After M-57 |
|---|---|
| `src/PlayCalculator.jsx` | Composes the React set. `useCalcuShell(shell, product, traits)` with the traits of the layout: `wide` `{hasQuit: false, shortcuts: false, savedMedia: true, menuBar: true, settings, about: true}`, `portrait` the same with `menuBar: false`; `layout="auto"` takes `portrait` while its rules are paged (`:195`). `ShellMenuBar` above the wide layout; in each `CalculatorColumn` (`:95-102`) the shell's bar in place of `ControlBar`, `compact` in portrait, with LIB-7's slot picker and C59-7's card controls; `ShellDialogs`; `ShellStorageBanner` above the layout. The status line (`:84-93`) goes: a notice is the dialog (AC-31). `hostKeyOf` and `claimed` (`:35-46, 235-239`) give way to `calcuHostKeyEvent` (`library/web/components/hostKeys.js`), with the page's own rule of which events are the calculator's (D-40). The drop (`:265-272`) is M-56's `open`. The keyboard feedback (`:48-82`) stays the product's; it reads `[keyboard] feedback` from `settings` and the held key from `held-keys` |
| `src/ControlBar.jsx` | deleted (Reset with the long press and the modifier, the model select, the card buttons, Preset, the gear; `ControlBar.jsx:7-120`) |
| `src/Dialogs.jsx` | `PresetPicker` (`:110-144`) gives way to the shell's Load Preset… and Examples ▸ from `preset-list`; `Settings` and `About` (`:148-209`) to `ShellSettings` and `ShellAbout`; `DialogHost` (`:212-222`) goes; the card dialogs are C59-7's (M-55) |
| `src/CalculatorFace.jsx` | keeps the art, the LED canvas and the cue card; its hit test gives way to `useShellFaceplate` with `c59.faceplate-map`, `origin` and `productFirst` (C59-7's card tap) |
| `src/engine.js` | deleted (LIB-15 59's translation, `engine.js:1-8` says "until LIB-14 59 (M-57)"): the page talks to the shell (`createShell`, `useTopic`), sends the library's commands and asks the product's requests by name |
| `app/TIWebPart.hpp/.cpp` | gone: `c59.web.model` (`TIWebPart.hpp:42`), `c59.web.settings` (`:54`) and the commands of the page's settings and pointer (`c59.web.set-startup-model`, `c59.web.set-keyboard-feedback`, `c59.web.set-trace-max-mb`, `c59.web.pointer-*`) with `PointerTracker m_pointer` (`:97`); the request `c59.static-data` (`app/WebPageData.hpp:15`; LIB-15's W59-1). New: `c59.shell-product {layout}` (`{product, settingsSpec}`, 10.6) and `c59.faceplate-map {model}` (the hit map in the library's form, the display and card areas, the canvas's aspect; D-41). Stay as product material: `c59.led-geometry`, `c59.mobile-rules` (`WebPageData.hpp:18-19`), the display, cue card and debug topics |
| `bindings.cpp` | adds `SettingsPart` to the parts (`bindings.cpp:72`) |
| `tests/` | `shell.spec.js` (new, 10.9); `play-calculator.spec.js`, `session.spec.js`, `storage.spec.js`, `rom-missing.spec.js` re-pointed to the object names of §3.10 and B9 |

### 10.6 What Calc-U-59 gives the shell (`app/TIShellProduct.hpp/.cpp`, new)

One source for Qt6, the Web and, through the bridge, Swift (D-32):
`cu59::shellProduct()` and `cu59::settingsSpec(frontend, layout)`. The
Qt6 frontend adds the acts; the page gets the data through
`c59.shell-product`.

| Field | Value |
|---|---|
| `name`, `version`, `build` | "Calc-U-59", `APP_VERSION`, `APP_BUILD_ID` (Qt6, `frontends/qt6/app/Version.h.in`); the page's version on the Web |
| `helpUrl` | `https://calc-u.org/59/` (AC-32) |
| `aboutLines` | "Calc-U-59 Calculator Emulation"; "Cycle-accurate TI-59, TI-58, and TI-58C emulation with magnetic cards, paper tape printer, presets, and debugger tools." (`AboutDialog.cpp:22-24`, with "preset state files" in the glossary's word; D-42) |
| `aboutLinks` | GitHub `https://github.com/tinue/Calc-U-59`, Support `https://github.com/tinue/Calc-U-59/issues`, Privacy `https://github.com/tinue/Calc-U-59/blob/main/PRIVACY.md` (`AboutDialog.cpp:25-27`) |
| `variantLabels` | none (no model or peripheral of Calc-U-59 has variants) |
| `presetFilter` | "Presets (*.ti59 *.ti58 *.ti58c)" (`MainWindow.cpp:81`) |
| `copyScreen`, `pasteText` | none, false: no Edit menu (AC-26) |
| `moduleSlotPinOuts` | `module-slot` (`core/TICatalogue.cpp:71`) |
| `placesWithoutControl` | none |
| `items` | FileMedia: C59-7's two card items (today "Fit Card…" and "Fit Fresh Card…", `MainWindow.cpp:56-57`), each with `mediaPlace = "card-reader"` and `onBar` (C59-7's card controls stand for them on the bar) |

The settings dialog (AC-30), from `settingsSpec`:

| Section | Row | Qt6 | Web wide | Web portrait |
|---|---|---|---|---|
| General | Startup model ("Last used", TI-59, TI-58, TI-58C) | yes | yes | yes |
| General | a note "Version <v> (<build>)" | yes | yes | yes |
| Startup presets | TI-59, TI-58, TI-58C (S4; "Choose…" in Qt6) | yes | yes | yes |
| Storage | Save folder | yes | no | no |
| Folders | the preset folder, labelled "Preset folder" | yes | no | no |
| Tracing | Trace folder | yes | no | no |
| Tracing | Maximum file size (MB) | yes | yes | no (today's, `PlayCalculator.jsx:296`) |
| Display | Segment Style: `modernized` "Modernized", `classic` "Classic" | yes | no | no |
| Keyboard | Keyboard Feedback: `off` "Off", `click` "Click Sound" | no | no | yes |

The `[folders]` kinds `asm-overlays` and `other` (`app/Settings.cpp:102-104`)
get no row, as today (D-43). With the Web prop `settings` off, neither
the item nor the button is there (M18).

### 10.7 Harness attach in Qt6 and the pacing probe

The Qt6 frontend opens attach as Calc-U-1600's does (§3.9, Part D D.6):
`SessionHost::openAttach(cu59::harnessProduct(), attachPortFrom(arguments))`,
compiled only with `CALCU_ATTACH`. `--pacing-probe <s>` stays as an
option but becomes an attach client in its own process: it reads
`ticks` after the start and again after `s` seconds of wall time and
prints today's lines (`main.cpp:119-121`). The Checkpoint entry
`CalcU59QtPacingProbe` (`frontends/qt6/CMakeLists.txt:138`) is
unchanged. The release tree and Windows have neither.

### 10.8 Adapters removed in M-57

| Adapter | Where | Named by |
|---|---|---|
| `--pacing-probe` outside attach | `frontends/qt6/app/main.cpp:83-85, 97-127` | LIB-17 §5; §5 |
| W59-1, the page data request `c59.static-data` | `app/WebPageData.*`, `app/TIWebPart.cpp` | LIB-15 §5 (the hit map stays, as `c59.faceplate-map`; D-22, D-41) |
| `src/engine.js` | `frontends/web/src/engine.js` | LIB-15 59 |
| the interim topic `c59.web.model` | `app/TIWebPart.*` | LIB-3 §5, LIB-4 §5 |
| the settings commands and `c59.web.settings` of `TIWebPart` | `app/TIWebPart.*` | §5 |
| `c59.web.pointer-*` | `app/TIWebPart.*` | §5 |
| A-4, the frontends' own "ROMs missing" | `frontends/qt6/app/EmulatorController.cpp:55-61` (a message box, then the app quits); the Web's start failure line | LIB-12 §5 |
| `cu59::PointerTracker` in Qt6 and the Web | `app/KeyboardLayout.hpp:103-…` | §4 (it stays for the Swift bridge until M-58) |

### 10.9 Tasks

Tests come before the code. Library cases in `calcu_library_tests`,
`calcu_qt6_tests` and `LibraryWebNodeTests` (Frequent) and
`library/web/tests/shell.spec.js` (Longer), as §6. A new test program
for Calc-U-59's Qt6 frontend: `CalcU59QtTests` (`frontends/qt6/tests/`,
the one runner, offscreen, built only with Qt; Frequent), on the real
window with the session in `Hosting::Paced` on a scripted wall clock and
fake choosers, as `CalcU1600QtTests`. The Web cases are in
`frontends/web/tests/shell.spec.js` (Chromium and WebKit; Longer, in
`CalcU59WebPlaywright`). Product case names are `lib14_acN_…`; each Qt6
case and its Web case carry the scenario ID of AC-25.

| Task | What | ACs | Test (program: cases) |
|---|---|---|---|
| L12 | The More model and the bar without a menu bar | (support; AC-29) | `menu_model_tests.cpp`: `more_items_are_the_items_without_a_control`, `more_cold_boot_asks_first`, `more_follows_availability`, `media_place_item_only_with_its_media_place`, `no_settings_item_without_settings`; `bar_model_tests.cpp`: `bar_without_menu_bar_keeps_reset_preset_settings_and_more`; the same names in `shell_model.test.js` |
| L13 | Labelled choices, value forms | (support; AC-30, AC-26, AC-32) | `settings_dialog_tests.cpp`: `choice_row_with_labels_offers_only_those`, `choice_value_not_listed_shown_as_written`; `value_forms_tests.cpp`: `shell_product_form_has_links_and_copy_screen_flag`, `calc_u_1600_forms_unchanged` (the replies of `c1600.shell-product` before and after, byte for byte); JS mirror |
| L14 | Faceplate geometry, About icon | (support; AC-24) | `calcu_qt6_tests`: `faceplate_origin_moves_the_hit_map`, `faceplate_press_the_product_takes_passes_through`, `about_shows_the_window_icon`; `library/web/tests/shell.spec.js`: `faceplate origin and a press the product takes` |
| L15 | The storage banner | (support; 1600-48) | `library/web/tests/shell.spec.js`: `storage banner shows the library's sentence while not kept`; Calc-U-1600 `frontends/web/tests/storage.spec.js` re-pointed to `storage-banner` and the three sentences |
| Q1 | `TIShellProduct`; the session host in `EmulatorController` | (support); AC-32 | `CalcU59Tests`, `tests/shell_product_tests.cpp`: `lib14_ac26_shell_product_complete` (`checkShellProduct` against each model's control state), `lib14_ac32_help_address_about_texts_and_links`, `lib14_ac30_settings_spec_per_frontend_and_layout` |
| Q2 | Qt6 menus | AC-26; AC-1, AC-4, AC-6 for this product | `CalcU59QtTests`: `lib14_ac26_qt_menus_in_family_order` (File: Load Preset…, Examples ▸, the two card items on the TI-59 only, Settings…, Quit; no Edit; Machine: Model ▸, Peripherals ▸ with the PC-100C (the module slot is the bar's picker, M6), Reset, Cold boot, Resume; Help), `lib14_ac26_shortcuts_ctrl_o_r_shift_r`, `lib14_ac26_roles_settings_about_quit` |
| Q3 | The bar | AC-27; AC-8 to AC-10 | `CalcU59QtTests`: `lib14_ac27_bar_per_model` (the model picker; the slot picker with the 14 modules and None; the "PC-100C" button pressed while plugged in; the card controls on the TI-59 only); `shell.spec.js`: `lib14_ac27 the bar per model` |
| Q4 | Buttons gone where there is a menu bar | AC-28 | `CalcU59QtTests`: `lib14_ac28_no_reset_preset_settings_on_the_bar`, `lib14_ac28_cold_boot_is_the_menu_item` (Ctrl held changes nothing on the bar); `shell.spec.js`: `lib14_ac28 the wide layout has the menus and no reset preset or settings on the bar` |
| Q5 | The portrait layout | AC-29 | `shell.spec.js`: `lib14_ac29 portrait bar keeps reset preset settings and more`, `lib14_ac29 more holds examples cold boot resume and help`, `lib14_ac29 cold boot asks first`, `lib14_ac29 no long press for clean`, `lib14_ac29 settings prop off` |
| Q6 | Settings | AC-30; AC-11 to AC-13 | `CalcU59QtTests`: `lib14_ac30_settings_sections_and_rows` (10.6's Qt6 column; no Solid State Module row; a change written at once); `shell.spec.js`: `lib14_ac30 the web settings per layout` |
| Q7 | "Loading…" and notices | AC-31; AC-15 to AC-17 | `CalcU59QtTests`: `lib14_ac31_loading_after_500_ms_blocks_nothing` (a preset at full speed: nothing at 300 ms, the overlay at 600 ms; the model picker, Reset and Cold boot enabled), `lib14_ac31_notice_without_title`; `shell.spec.js`: `lib14_ac31 loading and the notice dialog` |
| Q8 | Help and About | AC-32 | `CalcU59QtTests`: `lib14_ac32_help_opens_calc_u_org_59`, `lib14_ac32_about_icon_name_version_summary_links`; `shell.spec.js`: `lib14_ac32 help and about in the help menu and not in the settings` |
| Q9 | The layouts | AC-33 | `CalcU59QtTests`: `lib14_ac33_three_columns_with_dividers` (the order; the dividers move; the paper panel always); `shell.spec.js`: `lib14_ac33 the layouts portrait wide and auto`. One picture per layout (AC-25): the Qt6 window (`QWidget::grab` into the test's output folder) and the Web's three layouts (Playwright screenshots), Longer |
| Q10 | The faceplate's input | AC-24, AC-43 for this product | `CalcU59QtTests`: `lib14_ac24_key_under_pointer_ti59_ti58` (key centres; a point in a widened area; a point in a gap is no key, distance 0; a press on the fitted card feeds it), `lib14_ac43_display_hold_full_speed`; `shell.spec.js`: `lib14_ac24 a key under the pointer` |
| Q11 | ROMs missing | AC-19 for this product | `CalcU59QtTests`: `lib14_ac19_roms_missing_in_window` (the text in place of the faceplate, no bar, no Machine menu, Help and About work, the app runs on); `rom-missing.spec.js` re-pointed |
| Q12 | Qt6 attach, the probe | (LIB-17 §5) | `CalcU59QtTests`: `qt_attach_drives_calc_u_59` (`--attach-port 0`; `model`, `key`, `wait`); `CalcU59QtPacingProbe` (Checkpoint) through attach |

Coverage: AC-26 to AC-33 in Q2 to Q9, with AC-1, AC-4, AC-6, AC-8,
AC-11, AC-15, AC-17, AC-19, AC-20, AC-24 and AC-43 for this product. The
Longer group runs at the end of Part E with Playwright and the pictures.

### 10.10 Ledger rows

Visible from M-57: 59-5 ("Loading…"), 59-9 and 59-14 (Qt6 and Web),
59-44, 59-45 (Qt6 and Web), and 1600-48 (10.3.5). Rows proposed for
`docs/behaviour-changes.md`, consequences of the spec that the ledger
lacks (numbers given at the D/E boundary, 2026-11-16):

- **59-52 (Web).** Help and About are in the Help menu (wide) or
  behind the More button (portrait), no longer in the settings dialog;
  About shows the summary and the links to the source, to support and
  to the privacy statement, as the Qt6 About does (today a line and a
  link to calc-u.org/59). Behind the More button are also Import Media…
  and Saved Media… (the TI-59's cards) and About, besides Examples, Cold
  boot, Resume and Help. Sources: LIB-14 AC-29, AC-32; D-36.
- **59-53 (Qt6, Web).** The settings dialog: "Startup model" with
  "Last used" first (today "Startup Calculator", "Last Used" last in
  Qt6), "Save folder" (today "Card Folder"), the Qt6 rows grouped in
  Storage, Folders and Tracing (today "Files"), a Startup presets
  section, every change applied at once (today the module and, on the
  Web, the trace size when the dialog closes), the version as a line of
  General. Sources: LIB-14 AC-11, AC-30.
- **59-54 (Qt6).** When a ROM file is missing the window opens and
  shows which files are missing in place of the calculator, and Help
  and About work (today a message box, then the app quits). A notice is
  shown without a title (today a title taken from the sentence). Sources:
  LIB-14 AC-17, AC-19; LIB-12 AC-5 (as 1600-36).
- **59-55 (Web).** While what is saved is not kept (another tab,
  unreadable or unavailable storage), the storage sentence stays in a
  line above the calculator, besides its notice (today a status line
  until it is dismissed). Sources: LIB-15 AC-8, EC-2, EC-3; D-39.

### 10.11 Tools and agent knowledge (changed in the same step)

| File | Change |
|---|---|
| `products/calc-u-59/.claude/skills/calc-u-59-qt/SKILL.md` | the class map (`:62`: `CalculatorPanel`, `ControlBar` → the faceplate and the shell's bar); hosting through `SessionHost` and the shell; attach and `--pacing-probe` as its client; `CalcU59QtTests` |
| `products/calc-u-59/.claude/skills/calc-u-59-web/SKILL.md` | the page on the React set, the two layouts' traits, the More button, `c59.shell-product`, `c59.faceplate-map`; `engine.js` gone |
| `products/calc-u-59/reference/NewGUIGuide.md`, `FrontendParity.md`, `WebClient.md` | the shell's menus, bar and More button as the family structure; the requests of the page; the props unchanged |
| `products/calc-u-59/reference/SharedArchitecture.md` | `TIShellProduct`, `TIWebPart`'s topics after M-57 |
| `products/calc-u-59/CLAUDE.md` | the Qt6 and Web frontends on the shell; a new item goes into the menu model, not into a frontend |
| `docs/glossary-mapping.md` | visible renames for Calc-U-59 done: Cold boot (Qt6, Web), Startup model, Save folder, Load Preset… ("Open Preset…") |
| `docs/test-groups.md` | `CalcU59QtTests`, the new library cases, `shell.spec.js` of both places |
| `features/LIB-1-machine-interface-and-descriptors/design.md` §2a | nothing new (the areas exist) |

### 10.12 Golden runs and risks

- **Golden runs** run no frontend; M-57 changes no core. A difference is
  a finding.
- **Two frontends of Calc-U-1600 touched.** The value forms and the
  banner change Calc-U-1600's files; L13's byte-for-byte check and its
  Playwright and `CalcU1600QtTests` runs guard them.
- **The More menu on small screens.** The React popup must stay inside
  the client's width at the narrowest portrait (`FACE_MIN.portrait`,
  `PlayCalculator.jsx:17`); checked in Q5 at 280 px.
- **A press on the card.** `productFirst` decides before the hit rule;
  a card area that overlaps a key would take that key's presses. The
  card area lies above the top row of keys (`app/KeyboardLayout.hpp`,
  `kCardRect`); Q10 checks a press on the top row with a card fitted.
- **CI.** M-57 changes no workflow and no packaging. The local Linux CI
  with `web-59` and `web-1600` runs at the merge; GitHub CI at the end
  of Part E (plan §1).

### 10.13 Decided (best guess)

- **D-34 (AC-29; C59-8 AC-5).** The More button is a model of the
  library (`moreItems`, B8), computed from the menu model, so the Web's
  portrait layout and iOS show the same items from one rule; what is on
  the bar is left out by id, not by a product list.
- **D-35.** The Qt6 set draws no Action and no More: no Qt6 frontend is
  without a menu bar. The C++ model serves the Swift bridge (C59-8).
- **D-36 (AC-29, AC-32; LIB-7 AC-29, AC-36).** The More menu holds every
  item without a control of its own, so it also holds Import Media…,
  Saved Media… and About on the Web; AC-29's list names the items of
  its time, and the saved media of LIB-7 AC-36 and the Web's About
  (AC-32) would otherwise not be reachable in portrait. iOS has no About
  (C59-8 AC-11), so `Traits::about` is false there. Ledger 59-52.
- **D-37 (AC-26).** The page's form of `ShellProduct` says whether the
  product has Copy Screen; Calc-U-1600's data names a placeholder act,
  so its reply does not change.
- **D-38 (AC-32).** The About icon is the application's window icon in
  Qt6; the Web shows none, as today. `ShellProduct` gets no icon field.
- **D-39 (Q-4; ledger 1600-48).** M-57 makes the storage banner a
  component of the React set, from `web.storage`'s sentence, used by
  both products; 1600-48 is in effect from M-57, not left to M-61.
  Calc-U-59 gets the banner too (59-55).
- **D-40 (LIB-5 AC-7).** Host keys go through `qt6::HostKeys` and
  `hostKeys.js`; the TI keyboard map already refuses a key with a
  command modifier (`app/TITyping.cpp:156`), so dropping the frontends'
  own check changes nothing seen.
- **D-41 (D-22).** The page's static faceplate data is one product
  request, `c59.faceplate-map {model}`, as Calc-U-1600's D-31; the LED
  shapes and the mobile layout rules stay product requests under their
  names: they are drawing material of the product (ADR 0023), not data
  a library topic carries.
- **D-42.** The About summary is the Qt6 and Swift text of today
  (`AboutDialog.cpp:22-24`, `App/AppInfo.swift`), one for all
  frontends, with "presets" for "preset state files" (the glossary's
  term).
- **D-43 (AC-30).** Qt6 shows the one folder row of today (presets);
  the `[folders]` kinds `asm-overlays` and `other` keep their remembered
  folder and get no row. The Web's Maximum file size stays in the wide
  layout only, as today.
- **D-44 (AC-24).** The faceplate's geometry (origin, a press the
  product takes) is a library option, so Calc-U-59 keeps its letterboxed
  canvas and C59-7's card tap; Calc-U-1600's call is unchanged.

### 10.14 Plan notes (proposals; the plan is not edited)

- M-57 "Waits for" should also name M-53 (the paper panel, Resume) and
  M-49 (the shell's Part D changes and the attached registry).
- M-57 "Does" gains: the library's More model, labelled choices, the
  value forms, the faceplate geometry, the About icon and the storage
  banner (10.3), Calc-U-1600's banner on it (row 1600-48); Qt6 attach
  and `--pacing-probe` as an attach client.
- M-57 "Knowledge, ledger" gains the files of 10.11 and the rows
  59-52 to 59-55, 1600-48.

### 10.15 Questions for the user

None that split the design.

## Part E simplification E-1 (2026-11-17, branch `es-assembly`)

`docs/reviews/e-simplification.md` §1 S-1 (D-27). The attached harness
takes the session's `parts::LibraryParts&` (`qt6::SessionHost::openAttach`,
Calc-U-59's `SessionThread::openAttach`) instead of `harness::SessionParts`:
the preset steps and the media commands are those of the app's own parts
in every frontend (before, the Swift frontend's attach named none, and its
preset commands were refused with "This build reads no presets of format 2
yet."). The session order puts the control part before the shell's part and
`SettingsPart` (LIB-3, this step).

## Part E simplification E-2 (2026-11-17, branch `es-host-session`)

`docs/reviews/e-simplification.md` §1 S-1, §2 item 1 (host half). No host
makes `SettingsPart` itself: `shell::makeHostSession` (LIB-3's E-2 section)
makes it from `HostSessionSetup::settings` (a `shell::HostSettings`, LIB-9's
E-2 section), `settingsChanged` (Calc-U-1600's serial folder) and
`settingsAlsoShown` (the Swift frontend's portrait row), after the host's
own parts (the Web's shell part), and the session owns it.
`qt6::ProductHost<Part>` (`library/qt6/ProductHost.hpp`) is the common half
of both Qt6 controllers: the product's session and the `SessionHost` over
it, `start()` (the host's timer, then `start`), the end (stop the timer,
`close()`, the host before the session) and the request answered at once
(`ask`). `MachineController` and `EmulatorController` keep a
`ProductHost` and end it first in their destructors, while their own
members (the sound feed, the LOG tab's view) are still there, as before.

## Part E simplification E-4 (2026-11-17, branch `es-qt-glue`)

`docs/reviews/e-simplification.md` §1 S-4 (Qt6, survey A S-10). The
measurement of both products' `--pacing-probe <s>` (§10.7; LIB-17 AC-19,
AC-22) is `calcu::qt6::runPacingProbe` (`library/qt6/PacingProbe`, with
`AttachLineClient` only in `CALCU_ATTACH` builds): attach not open, the
bare `wait`, `ticks`, `s` seconds of wall time, `ticks` again, a
`PacingSample`. What is printed, the bound and the exit code stay each
product's (`report`): Calc-U-1600's one line and 25 ms
(`frontends/qt6/app/screenshots/PacingProbe.cpp`), Calc-U-59's seven lines
and 20 ms + 1/60 s (`main.cpp`), so `CalcU1600QtPacingProbe` and
`CalcU59QtPacingProbe` read the same output as before. LIB-13's and
LIB-7's sections of the same name have the file opening and the `asks:`
round trip of this step.

## Part E simplification E-8 (2026-11-17, branch `es-dead`)

`docs/reviews/e-simplification.md` §1 S-8. `SessionHost::setServiceHandler` and `answer` (§3,
"the frontend answers") are gone: no frontend or test set a handler. The
host answers the session's asks itself, as without one: the clipboard
(LIB-8's `ClipboardService`) and `open-url` (`QDesktopServices`); anything
else is not given.

## Part E simplification E-5 (2026-11-17, branch `es-web-glue`)

`docs/reviews/e-simplification.md` §1 S-4 (Web pages), §2 item 3. What
both products' pages wrote each is in the React set
(`library/web/components/`, `COMPONENT_SOURCES`):

- `useShellProduct.jsx`: `useShellProduct(shell, {request, args, fallback,
  version, build, copyScreen, presetAccept, openFiles, acts})` (D-32: the
  request `<prefix>.shell-product`, once per distinct `args` while the
  session runs; the fallback until it answered; the merge with the page's
  version, build, Copy Screen, Load Preset… and each item's act, read when
  used) and `useFaceplateMaps(shell, request, model)` (D-31, one request
  per model). The opening helpers are LIB-13's section of the same name.
- `ShellMedia.jsx`: `ShellSavedMedia` (File ▸ Saved Media…: LIB-7's list in
  `ShellModal`, `data-calcu="saved-media"`; was `C1600SavedMedia` and
  `C59SavedMedia`, word for word) and `ShellSlotPicker` (B1: LIB-7's
  picker of the slot, disabled by `empty`; `label` for the fieldset,
  Calc-U-59's "Solid State Module"; an optional Name & Save button,
  Calc-U-1600's, with the product's icon).
- The notice queue (`calcuShellNotices`) keeps `CALCU_SHELL_ASKS = "asks:"`
  with the spaces after it dropped: it mirrors `shell::kAsksPrefix` of
  `Notices.cpp`, which reads a reason the same way; every sender writes
  LIB-7's one prefix `CALCU_ASKS` ("asks: ").

Kept per product: the three modal frames (Calc-U-1600's `Modal`,
`.calcu-modal-backdrop > .calcu-modal[data-dialog]`, focus on its first
control and Escape inside it; Calc-U-59's `Dialog`, `.calcu-modal >
.calcu-dialog`, Escape from the window and a click outside closing it;
`ShellModal`, `.calcu-shell-modal*` with `data-calcu`) differ in their DOM
classes, their CSS (each product's `calc.css`) and their keys, so one frame
would change pixels or the tests' selectors; `hitMapOf`, as the products'
`faceplate-map` values differ in shape (flat vs `{map, card, canvas}`).
Not done: a C++ helper that registers `<prefix>.shell-product` (the pages'
requests are unchanged; C++ stayed out of this step). Every pixel and the
DOM of both pages are as before (screenshots of both pages in their main
states, before and after, compared pixel by pixel and by `outerHTML`).
